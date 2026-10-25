#!/usr/bin/env python3
# Copyright (c) 2026 Nakata Maho
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
# 1. Redistributions of source code must retain the above copyright notice,
#    this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR IMPLIED
# WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
# EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
# OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
# OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
# ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

"""Real-client G132 operations. X11 input, production clipboard copy and saves.

No emulator test hooks, headless scripts, direct guest-memory writes or synthetic
state files. Requires xclip and the x11_capture dependencies. Captures and state
files are private, out-of-tree outputs. A suite PASS is not whole-gate approval.
"""
from __future__ import annotations

import argparse
import ctypes
from functools import lru_cache
import json
import os
from pathlib import Path
import re
import shutil
import signal
import struct
import subprocess
import time

from x11_capture import command, decode_xwd, digest, observe, ROOT
from screen_compare import QaError, require, png_bytes, read_png
from m103b_basic_boot import screen_rows


def sections(data):
    require(len(data) >= 48 and data.startswith(b"VA-EG\0"), "G132_STATE_FORMAT")
    result, pos = {}, 48
    while pos < len(data):
        require(pos + 16 <= len(data), "G132_STATE_FORMAT")
        name, version, size = struct.unpack_from("<10sHI", data, pos)
        name = name.rstrip(b"\0").decode("ascii")
        require(name not in result and pos + 16 + size <= len(data), "G132_STATE_FORMAT")
        result[name] = data[pos + 16:pos + 16 + size]
        pos += 16 + ((size + 15) & ~15)
    require(pos == len(data) and {"PCCORE", "MEMORY", "TVRAMVA", "MEM88MODE"} <= result.keys(),
            "G132_STATE_FORMAT")
    return result


def footer_glyphs(image):
    """Independent thresholded glyph oracle from the human-approved H footer.

    Status-font hyphen is below this threshold; exclude it in the known alphabet.
    This checks V/1/2/H/S pixels, not OCR substitutions such as S -> 5 or V -> Y.
    """
    w, h, pixels = image
    top = h - 24 - 14 * max(1, (w + 320) // 640)
    columns = [x for x in range(1, w-1) if any(pixels[((top+y)*w+x)*4] > 130 for y in range(2, 22))]
    runs = []
    for x in columns:
        if runs and runs[-1][-1] == x-1:
            runs[-1].append(x)
        else:
            runs.append([x])
    return [(len(run), bytes(pixels[((top+y)*w+x)*4] > 130 for y in range(2, 22) for x in run))
            for run in runs]


def status_glyphs(reference, candidate, model, mode):
    alphabet = "PC88VA2|V1H|CPU8MHz|SGP8MHz"
    require(len(reference) == len(alphabet), "G132_REFERENCE_GLYPHS")
    atlas = {c: [g for letter, g in zip(alphabet, reference) if letter == c] for c in set(alphabet)}
    prefix = "PC88VA2|" if model == "va2" else "PC88VA|"
    expected = prefix + mode + "|CPU"
    require(len(candidate) >= len(expected), "G132_STATUS_GLYPHS")
    require(all(glyph in atlas[letter] for letter, glyph in zip(expected, candidate)), "G132_STATUS_GLYPHS")


def panel_lamps(reference, candidate):
    def crop(image):
        w, h, pixels = image
        require(w == 960 and h >= 52, "G132_LAMP_GEOMETRY")
        return b"".join(pixels[((h-28+y)*w+700)*4:((h-28+y)*w+960)*4] for y in range(28))
    require(crop(reference) == crop(candidate), "G132_LAMP_RELEASE")


def tooltip_text(text):
    for label in ("CPU", "Base", "SGP"):
        require(re.search(r"(?m)^"+label+r":\s*7\.987200\s*MHz\s*$",text), "G132_TOOLTIP_PRECISION")
    require(re.search(r"Speed:\s*100%",text) and "88 compatible" in text, "G132_TOOLTIP_DOMAIN")
    require(re.search(r"Multiplier:\s*x1\.00",text) and "Draws:" in text and "Guest frames:" in text,
            "G132_TOOLTIP_FIELDS")


def dynamic_fields(text):
    compact=re.sub(r"\s","",text)
    fps=re.search(r"([0-9]+\.[0-9])FPS",compact)
    frame=re.search(r"FRAME([0-9]+)",compact)
    require(fps and 0<float(fps[1])<=1000 and frame, "G132_DYNAMIC_FIELDS")
    return float(fps[1]),int(frame[1])


def paste_released(observation):
    require(all(observation[k] == 0 for k in ("count", "index", "skipped", "phase", "shift_down", "key_down", "scripted")),
            "G132_PASTE_RELEASE")


def check_text(text, pattern, code):
    require(not re.search(r"(?:Syntax error|Overflow|Illegal function|Type mismatch|Undefined)", text),
            "G132_GUEST_ERROR")
    require(re.search(pattern, text, re.M) is not None, code)


class PasteState(ctypes.Structure):
    _fields_ = [("actions", ctypes.c_void_p), ("count", ctypes.c_size_t), ("index", ctypes.c_size_t),
                ("skipped", ctypes.c_uint), ("next_tick", ctypes.c_uint32), ("frames", ctypes.c_uint),
                ("phase", ctypes.c_int), ("shift_down", ctypes.c_int), ("key_down", ctypes.c_int),
                ("scripted", ctypes.c_int), ("status", ctypes.c_char * 160)]


@lru_cache(maxsize=16)
def symbol_address(pid, worker, symbol, size):
    """Read-only observation of our own ELF64 child's production queue.

    No injected calls or writes. Symbol size, ELF ABI and process ownership are
    checked before reading. This is a Linux-only observer, not a portable ABI.
    """
    require(Path(f"/proc/{pid}/exe").resolve() == worker, "G132_PROCESS_IMAGE")
    elf = worker.read_bytes()[:20]
    require(elf[:6] == b"\x7fELF\x02\x01" and ctypes.sizeof(PasteState) == 216, "G132_OBSERVER_ABI")
    require((symbol, size) in (("paste", 216), ("pccore_cpu_multiple_value", 4),
                               ("pccore_cpu_speed_value", 4)), "G132_OBSERVER_ABI")
    matches = re.findall(r"^([0-9a-f]+)\s+([0-9a-f]+)\s+[bdBD] " + re.escape(symbol) + "$",
                         subprocess.check_output(["nm", "-S", "-a", str(worker)], text=True), re.M)
    require(len(matches) == 1 and int(matches[0][1], 16) == size, "G132_OBSERVER_ABI")
    base = 0
    if struct.unpack_from("<H", elf, 16)[0] == 3:
        for line in Path(f"/proc/{pid}/maps").read_text().splitlines():
            if line.endswith(str(worker)) and line.split()[2] == "00000000":
                base = int(line.split("-")[0], 16)
                break
        require(base > 0, "G132_OBSERVER_ABI")
    return base + int(matches[0][0], 16)


def read_paste(pid, worker):
    address = symbol_address(pid, worker, "paste", 216)
    os.kill(pid, signal.SIGSTOP)
    try:
        os.waitpid(pid, os.WUNTRACED)
        with open(f"/proc/{pid}/mem", "rb", buffering=0) as memory:
            memory.seek(address)
            state = PasteState.from_buffer_copy(memory.read(216))
    finally:
        os.kill(pid, signal.SIGCONT)
    require(state.index <= state.count and state.phase in range(5) and state.scripted == 0,
            "G132_OBSERVER_ABI")
    return {key: getattr(state, key) for key in ("count", "index", "skipped", "frames", "phase",
                                               "shift_down", "key_down", "scripted")}, bool(state.actions)


def read_clock(pid, worker):
    addresses = [symbol_address(pid, worker, name, 4) for name in
                 ("pccore_cpu_multiple_value", "pccore_cpu_speed_value")]
    os.kill(pid, signal.SIGSTOP)
    try:
        os.waitpid(pid, os.WUNTRACED)
        with open(f"/proc/{pid}/mem", "rb", buffering=0) as memory:
            values = []
            for address in addresses:
                memory.seek(address)
                values.append(struct.unpack("<I", memory.read(4))[0])
    finally:
        os.kill(pid, signal.SIGCONT)
    return tuple(values)


class Client:
    def __init__(self, args, output, resume=False):
        self.args, self.output = args, output
        if not resume:
            output.mkdir(parents=True)
            (output / "bin").mkdir()
        self.worker = output / "bin/vaeg"
        if not resume:
            shutil.copy2(args.worker, self.worker)
            shutil.copyfile(args.backup_seed, output / "backup.bin")
            (output / "config.ini").write_text(
            "[NekoProjectII]\nMain_RAM=640\nMain_RAM_Auto=false\nBootMode=1\nCPU_Speed=100\n"
            "V1V2_Standard=false\nDspClock=5\nF12_bind=9\nNativeCRT=false\nMachineStatus=1\n"
            "FrontPanelMode=1\nGUI_win_cx=960\nGUI_win_cy=640\nGUI_ui_scale=100\n")
        self.env = {**os.environ, "DISPLAY": args.display, "SDL_VIDEODRIVER": "x11",
                    "SDL_RENDER_DRIVER": "software", "SDL_AUDIODRIVER": "dummy"}
        self.argv = [str(self.worker), "--roms", str(args.roms.resolve()), "--cfg", str(output / "config.ini"),
                     "--bkupmem", str(output / "backup.bin"), "--model", "va2",
                     "--mute", "--windowed", "--keyboard-layout", "us", "--debug"]
        for slot in ["fdd1", "fdd2", "sasi1", "sasi2", *(f"scsi{i}" for i in range(7))]:
            self.argv += ["--" + slot, "none"]
        if not resume:
            self.argv += ["--cpumult", "2"]
            if args.probe_disk:
                shutil.copyfile(args.probe_disk, output / "probe-copy.d88")
        if args.probe_disk:
            position = self.argv.index("--fdd1") + 1
            self.argv[position] = str(output / "probe-copy.d88")
        self.out = (output / "stdout.log").open("ab" if resume else "wb")
        self.err = (output / "stderr.log").open("ab" if resume else "wb")
        self.process = subprocess.Popen(self.argv, env=self.env, cwd=output, stdout=self.out, stderr=self.err)
        self.events = []
        try:
            self.connect()
        except BaseException:
            self.process.terminate()
            try:
                self.process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                self.process.kill()
                self.process.wait()
            self.out.close()
            self.err.close()
            raise

    def connect(self):
        self.window = None
        deadline = time.monotonic() + 15
        while self.process.poll() is None and time.monotonic() < deadline:
            result = subprocess.run(["xdotool", "search", "--onlyvisible", "--pid", str(self.process.pid)],
                                    env=self.env, capture_output=True, text=True)
            if len(result.stdout.split()) == 1:
                self.window = result.stdout.strip()
                break
            time.sleep(.1)
        require(self.window is not None, "G132_WINDOW_UNAVAILABLE")
        self.xdo("windowactivate", "--sync", self.window)
        require(Path(f"/proc/{self.process.pid}/exe").resolve() == self.worker, "G132_PROCESS_IMAGE")
        self.wait_text("boot-query", r"How many files", 60)
        self.key("Return")
        self.wait_text("boot-ready", r"^Ok\s*$", 15)

    def xdo(self, *argv):
        return command(["xdotool", *map(str, argv)], self.env)

    def key(self, key, hold=.12):
        self.xdo("keydown", key)
        time.sleep(hold)
        self.xdo("keyup", key)
        time.sleep(.15)

    def start_paste(self, text):
        self.events.append({"paste": text, "monotonic": time.monotonic()})
        subprocess.run(["xclip", "-selection", "clipboard", "-i"], input=text.encode(),
                       env=self.env, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
                       check=True, timeout=10)
        self.key("ctrl+v")

    def finish_paste(self, text):
        deadline, observations, active_seen = time.monotonic() + max(20, len(text) * .8), [], False
        while time.monotonic() < deadline:
            observation, active = read_paste(self.process.pid, self.worker)
            observations.append(observation)
            require(observation["skipped"] == 0, "G132_PASTE_SKIPPED")
            active_seen |= active
            if active_seen and not active:
                break
            time.sleep(.25)
        else:
            raise QaError("G132_PASTE_TIMEOUT")
        paste_released(observations[-1])
        self.events[-1]["queue_observations"] = observations
        time.sleep(.3)

    def paste(self, text, marker):
        self.start_paste(text)
        self.finish_paste(text)
        check_text(self.text(f"paste-{len(self.events)}"), rf"^{re.escape(marker)}\s*$", "G132_PASTE_CONTENT")

    def line(self, line):
        marker = f"QAEND{len(self.events)}"
        self.paste(line + f':PRINT "{marker}"\n', marker)

    def text(self, name):
        self.xdo("key", "--clearmodifiers", "ctrl+shift+c")
        time.sleep(.2)
        raw = command(["xclip", "-o", "-selection", "clipboard"], self.env)
        (self.output / (name + ".clipboard.txt")).write_text(raw)
        text = "".join(c if c == "\n" or 32 <= ord(c) < 127 else " " for c in raw)
        (self.output / (name + ".txt")).write_text(text)
        return text

    def wait_text(self, name, pattern, timeout=15):
        deadline, count = time.monotonic() + timeout, 0
        while time.monotonic() < deadline:
            text = self.text(f"{name}-{count}")
            if re.search(pattern, text, re.M):
                return text
            time.sleep(.5)
            count += 1
        raise QaError("G132_GUEST_TIMEOUT")

    def snapshot(self, name):
        os.kill(self.process.pid, signal.SIGSTOP)
        try:
            props = command(["xprop", "-id", self.window, "_NET_WM_PID", "_NET_WM_NAME", "_NET_WM_STATE"], self.env)
            geom = command(["xwininfo", "-id", self.window], self.env)
            metadata = observe(props, geom, self.process.pid)
            path = self.output / (name + ".xwd")
            command(["xwd", "-silent", "-id", self.window, "-out", str(path)], self.env)
            (self.output / (name + ".png")).write_bytes(png_bytes(decode_xwd(path.read_bytes())))
            (self.output / (name + ".json")).write_text(json.dumps(metadata, indent=2) + "\n")
        finally:
            os.kill(self.process.pid, signal.SIGCONT)

    def click(self, x, y):
        self.xdo("mousemove", "--window", self.window, x, y)
        time.sleep(.3)
        self.xdo("click", "1")
        time.sleep(.3)

    def hover(self, x, y):
        self.xdo("mousemove", "--window", self.window, x, y)
        time.sleep(.6)

    def boot(self, model, version, standard, name):
        self.click(20, 10)
        self.hover(80, 58)
        self.hover(260, 58 if model == "va2" else 78)
        self.snapshot(name + "-selection")
        index = (0 if version == 2 else 2) + int(standard)
        self.click(355, (78 if model == "va2" else 98) + 20 * index)
        self.hover(950, 350)
        self.wait_text(name + "-boot", r"How many files", 60)
        self.key("Return")
        self.wait_text(name + "-ready", r"^Ok\s*$", 15)
        self.identity(version, standard, name)

    def identity(self, version, standard, name):
        self.line('CLS:PRINT "QAID=";INP(&H6E);INP(&H31)')
        expected = rf"^QAID=\s*{255 if standard else 127}\s+{249 if version == 1 else 121}\s*$"
        check_text(self.text(name + "-identity"), expected, "G132_IDENTITY")

    def status(self, name, model, version, standard, cpu_mhz=None, dynamic=False):
        self.hover(950, 350)
        self.snapshot(name)
        w, h, pixels = read_png((self.output / (name + ".png")).read_bytes())
        top = h - 24 - 14 * max(1, (w + 320) // 640)
        scaled = b"".join(b"".join(pixels[((top+y)*w+x)*4:((top+y)*w+x)*4+4]*4
                                   for x in range(w))*4 for y in range(24))
        result = subprocess.run(["tesseract", "stdin", "stdout", "--psm", "7", "-c",
                                 "tessedit_char_whitelist=0123456789PCVAHSMzUG|-FR.E"],
                                input=png_bytes((w*4, 96, scaled)), capture_output=True, check=True, timeout=15)
        text = result.stdout.decode()
        (self.output / (name + ".ocr.txt")).write_text(text)
        compact = re.sub(r"\s", "", text)
        reference = self.args.reference_dir / "software-normal-s1p1/reference.png"
        manifest = json.loads((self.args.reference_dir / "manifest.json").read_text())
        require(digest(self.args.reference_dir / "manifest.json") == self.args.manifest_sha256 and
                manifest["approval"]["status"] == "APPROVED" and manifest["approval"]["record"],
                "G132_REFERENCE_APPROVAL")
        require(digest(reference) == manifest["cases"]["software-normal-s1p1"]["reference_sha256"],
                "G132_REFERENCE_HASH")
        status_glyphs(footer_glyphs(read_png(reference.read_bytes())), footer_glyphs((w, h, pixels)),
                      model, f'V{version}{"S" if standard else "H"}')
        require(f'CPU{cpu_mhz or (4 if standard else 8)}MHz' in compact, "G132_STATUS_CLOCK")
        if not dynamic:
            require("FRAME" not in compact and "FPS" not in compact, "G132_STATUS_DEFAULT_FIELDS")
        return text

    def type_marker(self, name):
        # XTest 'type --delay' does not guarantee a guest-visible make interval.
        for character in 'PRINT 54321':
            self.key("space" if character == " " else character, hold=.25)
        self.key("Return", hold=.25)
        time.sleep(.5)
        check_text(self.text(name), r"^\s*54321\s*$", "G132_RESTORED_TYPING")

    def full_paste(self, name):
        value = "MiXeD10000111:;!?[]()_+-=*/"
        lines = ['NEW', '10 CLEAR ,&HAFFF', '20 DEFINT A-Z', f'30 A$="{value}"',
                 '40 T=0', '50 FOR I=1 TO 100', '60 T=T+I', '70 NEXT I',
                 '80 PRINT "QAPASTE=";A$', '90 PRINT "QASUM=";T', '100 END',
                 'CLS', 'LIST', 'RUN', 'PRINT "QAFULLDONE"']
        self.paste("\n".join(lines) + "\n", "QAFULLDONE")
        text = self.text(name)
        check_text(text, r"^QAPASTE=" + re.escape(value) + r"\s*$", "G132_PASTE_LITERAL")
        check_text(text, r"^QASUM=\s*5050\s*$", "G132_PASTE_SUM")
        require(all(re.search(r"^" + str(number) + r" ", text, re.M) for number in range(10, 101, 10)),
                "G132_PASTE_PROGRAM_LIST")
        self.snapshot(name)
        self.released_lamps(name + "-released")

    def released_lamps(self, name):
        self.hover(950, 350)
        self.snapshot(name)
        reference = self.args.reference_dir / "software-large-ui-s1p1/reference.png"
        manifest = json.loads((self.args.reference_dir / "manifest.json").read_text())
        require(digest(self.args.reference_dir / "manifest.json") == self.args.manifest_sha256 and
                manifest["approval"]["status"] == "APPROVED" and
                digest(reference) == manifest["cases"]["software-large-ui-s1p1"]["reference_sha256"],
                "G132_REFERENCE_HASH")
        panel_lamps(read_png(reference.read_bytes()), read_png((self.output / (name + ".png")).read_bytes()))

    def cancel_case(self, focus, name):
        self.start_paste("REM " + "AAaa0011" * 100)
        deadline = time.monotonic() + 3
        while time.monotonic() < deadline:
            observation, active = read_paste(self.process.pid, self.worker)
            if active and (observation["shift_down"] or observation["key_down"]):
                break
            time.sleep(.05)
        require(active and (observation["shift_down"] or observation["key_down"]), "G132_CANCEL_FIXTURE_NOT_ACTIVE")
        target = None
        if focus:
            title = "G132focus" + str(self.process.pid)
            target = subprocess.Popen(["xmessage", "-title", title, "-timeout", "5", "QA focus target"],
                                      env=self.env, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            try:
                window = self.xdo("search", "--sync", "--name", "^" + title + "$").split()[0]
                self.xdo("windowactivate", "--sync", window)
                require(self.xdo("getactivewindow").strip() == window, "G132_FOCUS_TARGET")
                time.sleep(.3)
            finally:
                target.terminate()
                target.wait(timeout=5)
            self.xdo("windowactivate", "--sync", self.window)
        else:
            self.click(180, 10)
            self.click(220, 78)
        deadline = time.monotonic() + 5
        while time.monotonic() < deadline:
            released, active = read_paste(self.process.pid, self.worker)
            if not active:
                break
            time.sleep(.1)
        require(not active and not released["shift_down"] and not released["key_down"], "G132_CANCEL_RELEASE")
        self.events[-1].update(cancel_kind="focus-loss" if focus else "menu", before=observation, after=released)
        self.key("Return", hold=.25)  # Finish the deliberately harmless partial REM line.
        self.type_marker(name + "-typed")
        self.released_lamps(name)

    def probe(self, name, version):
        text = 'CLS\nLOAD "M131TV"\n'
        self.start_paste(text)
        self.finish_paste(text)
        self.wait_text(name + "-loaded", r'^LOAD "M131TV"\s*\nOk\s*$', 45)
        self.start_paste("RUN\n")
        self.finish_paste("RUN\n")
        for phase in (0, 1):
            content = self.wait_text(name + f"-phase{phase}", rf"PHASE {phase}: CPU TMODE={phase}", 30)
            check_text(content, r"^A: WRITTEN THROUGH CPU BANK 0 \(TMODE CLEAR\)\s*$", "G132_PROBE_DISPLAY_BANK")
            self.snapshot(name + f"-phase{phase}")
            w, h, pixels = read_png((self.output / (name + f"-phase{phase}.png")).read_bytes())
            height = min(220, h-74)
            cropped = b"".join(pixels[((22+y)*w)*4:((22+y+1)*w)*4] for y in range(height))
            result = subprocess.run(["tesseract", "stdin", "stdout", "--psm", "6"],
                                    input=png_bytes((w, height, cropped)), capture_output=True, check=True, timeout=15)
            optical = result.stdout.decode()
            (self.output / (name + f"-phase{phase}.ocr.txt")).write_text(optical)
            require(re.search(r"^A:\s*WRITTEN.*CPU BANK", optical, re.M), "G132_PROBE_RENDERED_BANK")
            self.key("space", hold=.25)
        content = self.wait_text(name + "-returned", r"RESTORE=\s*1\s+\(MUST BE 1\)", 30)
        v1 = version == 1
        for pattern in (rf"^PORT31={'F9' if v1 else '79'} ORIGINAL32={'8A' if v1 else 'AA'}\s*$",
                        r"^CPU P0=41 P1=42\s*$",
                        rf"^PORT32 P0={'8A' if v1 else 'AA'} P1={'9A' if v1 else 'BA'}\s*$"):
            check_text(content, pattern, "G132_PROBE_CPU_RESULTS")
        self.line('PRINT "QAPRINT=";1+1')
        check_text(self.text(name + "-usable"), r"^QAPRINT=\s*2\s*$", "G132_PROBE_RETURN_INPUT")
        self.snapshot(name + "-return")

    def info_toggle(self, y):
        self.click(930, 10)
        self.click(850, y)
        self.hover(950, 350)

    def tooltip(self):
        self.hover(950, 350)
        self.snapshot("tooltip-before")
        w, h, before = read_png((self.output / "tooltip-before.png").read_bytes())
        self.hover(100, h-40)
        time.sleep(.7)
        self.snapshot("tooltip-visible")
        _, _, after = read_png((self.output / "tooltip-visible.png").read_bytes())
        changed = [(x,y) for y in range(max(22,h-300),h) for x in range(100,500)
                   if before[(y*w+x)*4:(y*w+x)*4+4] != after[(y*w+x)*4:(y*w+x)*4+4]]
        require(changed, "G132_TOOLTIP_NOT_RENDERED")
        x0,x1=min(x for x,y in changed),max(x for x,y in changed)
        y0,y1=min(y for x,y in changed),max(y for x,y in changed)
        width,height=x1-x0+1,y1-y0+1
        require(160 <= width <= 400 and 100 <= height <= 250 and y1 < h-1, "G132_TOOLTIP_CLIPPED")
        scaled=b"".join(b"".join(after[((y0+y)*w+x0+x)*4:((y0+y)*w+x0+x)*4+4]*4
                                  for x in range(width))*4 for y in range(height))
        result=subprocess.run(["tesseract","stdin","stdout","--psm","6"],
                              input=png_bytes((width*4,height*4,scaled)),capture_output=True,check=True,timeout=15)
        text=result.stdout.decode()
        (self.output/"tooltip.ocr.txt").write_text(text)
        tooltip_text(text)
        self.hover(950,350)

    def clock_menu(self):
        for _ in range(3):
            self.key("Escape")
        self.click(20, 10)
        self.hover(80, 170)

    def preset(self, index):
        self.clock_menu()
        self.snapshot(f"preset-{index}-menu")
        self.click(240, (170, 190, 210, 234, 254, 274)[index])
        self.hover(950, 350)

    def custom(self, multiple, speed):
        for y, value in ((297, multiple), (323, speed)):
            self.clock_menu()
            self.hover(280, 295)
            self.xdo("keydown", "ctrl")
            self.click(580, y)
            self.xdo("keyup", "ctrl")
            self.xdo("key", "ctrl+a")
            self.xdo("type", str(value))
            self.key("Return")
        for _ in range(3):
            self.key("Escape")
        self.hover(950, 350)

    def save(self, name):
        self.key("F12")
        time.sleep(1.2)
        path = self.output / "bin/states/common/quick1.sav"
        require(path.is_file(), "G132_STATE_NOT_WRITTEN")
        saved = self.output / (name + ".sav")
        shutil.copyfile(path, saved)
        return sections(saved.read_bytes())

    def close(self):
        if self.process.poll() is None:
            self.xdo("key", "--clearmodifiers", "alt+F4")
            self.process.wait(timeout=15)
        require(self.process.returncode == 0, "G132_WORKER_EXIT")
        self.out.close()
        self.err.close()


def run(args):
    output = args.output_dir.resolve()
    require(not output.exists() and output != ROOT and ROOT not in output.parents, "G132_OUTPUT_NOT_NEW")
    require(re.fullmatch("[a-f0-9]{40}", args.evaluated_commit) and
            args.evaluated_commit.encode() in args.worker.read_bytes(), "G132_WORKER_COMMIT")
    report = {"contract": "g132-real-x11-operations-v1", "status": "INCOMPLETE", "gate_pass": False,
              "worker_commit": args.evaluated_commit, "worker_sha256": digest(args.worker),
              "runner_sha256": digest(__file__), "backup_seed_sha256": digest(args.backup_seed),
              "roms": {str(p.relative_to(args.roms)): digest(p) for p in args.roms.rglob("*") if p.is_file()},
              "display": args.display, "display_scope": "virtual-x11", "normal_rate": True}
    client = None
    try:
        client = Client(args, output)
        client.line('CLS:PRINT "QAID=";INP(&H6E);INP(&H31)')
        check_text(client.text("initial"), r"^QAID=\s*127\s+249\s*$", "G132_IDENTITY")
        report["cases"] = {}
        if args.scenario in ("state", "state-h", "state-s"):
            transitions = ((False, 1234), (True, 5678))
            if args.scenario != "state":
                transitions = tuple(item for item in transitions if item[0] == (args.scenario == "state-s"))
            for standard, value in transitions:
                tag = "s" if standard else "h"
                if standard:
                    client.boot("va2", 1, True, "start-s")
                client.line(f'Q={value}:PRINT "QASAVED=";Q')
                check_text(client.text(tag + "-saved-variable"), rf"^QASAVED=\s*{value}\s*$", "G132_GUEST_VARIABLE")
                before = client.save(tag + "-initial")
                client.boot("va2", 1, not standard, "opposite-" + tag)
                client.line('Q=9876:PRINT "QAMUT=";Q')
                check_text(client.text("mutated-" + tag), r"^QAMUT=\s*9876\s*$", "G132_GUEST_VARIABLE")
                client.key("shift+F12")
                time.sleep(2)
                client.identity(1, standard, "restored-" + tag)
                client.line('PRINT "QAREST=";Q')
                check_text(client.text("restored-" + tag + "-variable"), rf"^QAREST=\s*{value}\s*$", "G132_STATE_VARIABLE")
                client.status("restored-" + tag, "va2", 1, standard)
                after = client.save(tag + "-restored")
                require(before["PCCORE"] == after["PCCORE"] and
                        before["SYSPORTVA"] == after["SYSPORTVA"] and
                        before["MEM88MODE"] == after["MEM88MODE"], "G132_STATE_LATCH")
                client.type_marker("restored-" + tag + "-typed")
                client.full_paste("restored-" + tag + "-paste")
                report["cases"]["state-" + tag] = {"status": "PASS", "guest_variable": value,
                    "opposite_preference": True, "actual_files": True, "typed_and_full_paste": True,
                    "status_ocr": True, "pccore_and_system_latches_equal": True}
                (output / "checkpoint.json").write_text(json.dumps(report, indent=2) + "\n")
        elif args.scenario in ("modes", "modes-va", "modes-va2"):
            models = ("va2", "va") if args.scenario == "modes" else (args.scenario.removeprefix("modes-"),)
            for model in models:
                for version, standard in ((1, False), (1, True), (2, True), (2, False), (1, False)):
                    name = f'{model}-v{version}{"s" if standard else "h"}-{len(report["cases"])}'
                    client.boot(model, version, standard, name)
                    client.status(name, model, version, standard)
                    state = client.save(name)
                    require(state["PCCORE"][28] == (2 if model == "va2" else 1), "G132_MODEL_LATCH")
                    require(bool(state["SYSPORTVA"][0] & 32) != standard, "G132_SPEED_LATCH")
                    report["cases"][name] = {"status": "PASS", "actual_reset": True, "identity_ports": True,
                                            "actual_saved_latch": True, "rendered_status_ocr": True}
                    (output / "checkpoint.json").write_text(json.dumps(report, indent=2) + "\n")
        elif args.scenario == "probe":
            require(args.probe_disk and args.probe_build, "G132_PROBE_INPUTS")
            binary = (args.probe_build / "probe.bin").read_bytes()
            require(len(binary) == 604 and sum(binary) == 62810, "G132_PROBE_SOURCE_BUILD")
            report["probe_build_sha256"] = digest(args.probe_build / "build.json")
            report["probe_disk_sha256"] = digest(args.probe_disk)
            for model in ("va2", "va"):
                for version, standard in ((1, False), (1, True), (2, False), (2, True)):
                    name = f'{model}-v{version}{"s" if standard else "h"}'
                    client.boot(model, version, standard, name)
                    client.probe(name, version)
                    report["cases"][name] = {"status": "PASS", "display_phases": ["A", "A"],
                        "cpu_phases": [65, 66], "restore": 1, "basic_answer": 2,
                        "full_604_byte_probe": True, "real_client_pixel_ocr": True}
                    (output / "checkpoint.json").write_text(json.dumps(report, indent=2) + "\n")
        elif args.scenario == "cancel":
            for focus in (False, True):
                name = "focus-cancel" if focus else "menu-cancel"
                client.cancel_case(focus, name)
                report["cases"][name] = {"status": "PASS", "active_queue_cancelled": True,
                                         "modifiers_released": True, "subsequent_typing": True,
                                         "approved_lamp_pixels": True}
        elif args.scenario == "display":
            client.status("display-initial", "va2", 1, False)
            client.tooltip()
            client.click(930,10)
            client.snapshot("information-menu")
            client.key("Escape")
            # Use a real guest redraw stimulus for the measured-rate check.
            redraw='10 LOCATE 0,5:I=I+1:PRINT I:GOTO 10\nRUN\n'
            client.start_paste(redraw)
            client.finish_paste(redraw)
            time.sleep(2)
            client.info_toggle(103)  # FPS, after the first separator.
            client.info_toggle(163)  # FRAME is opt-in, never assumed from process exit.
            # FRAME visibility changes reset metrics: wait a complete sample window.
            time.sleep(2)
            frames=[]
            for index in range(2):
                text=client.status(f"dynamic-{index}","va2",1,False,dynamic=True)
                fps,frame=dynamic_fields(text)
                frames.append(frame);time.sleep(.8)
            require(frames[1]>frames[0],"G132_FRAME_NOT_ADVANCING")
            client.info_toggle(163)
            text=client.status("frame-disabled","va2",1,False,dynamic=True)
            require("FRAME" not in text and "FPS" in text,"G132_FRAME_OPT_IN")
            dimensions=[]
            for name,y in (("status-off",38),("both-off",58),("status-only",38),("both-on",58)):
                client.info_toggle(y)
                client.snapshot(name)
                dimensions.append(read_png((output/(name+".png")).read_bytes())[:2])
            require(all(w==960 for w,h in dimensions) and
                    dimensions[0][1]-dimensions[1][1]==28 and
                    dimensions[2][1]-dimensions[1][1]==24 and
                    dimensions[3][1]-dimensions[2][1]==28,"G132_LIVE_TOGGLE_GEOMETRY")
            events=client.events;client.close()
            settings=(output/"config.ini").read_text()
            require(re.search(r"^DspClock=13\s*$",settings,re.M),"G132_DYNAMIC_PERSISTENCE")
            client=Client(args,output,resume=True);client.events=events
            text=client.status("display-restarted","va2",1,False,dynamic=True)
            require("FRAME" not in text and "FPS" in text,"G132_DYNAMIC_PERSISTENCE")
            report["cases"]["display"]={"status":"PASS","tooltip_precision":True,"tooltip_bounded":True,
                "fps_measured_positive":True,"frame_advances":True,"frame_opt_in":True,
                "real_information_menu_toggles":4,"restart_persistence":True}
        elif args.scenario in ("controls", "controls-high"):
            for index, (multiple, speed, mhz) in enumerate(((2, 100, 8), (2, 50, 4), (2, 100, 8),
                                                          (4, 100, 16), (8, 100, 32), (16, 100, 64))):
                if args.scenario == "controls-high" and index < 3:
                    continue
                name = f"preset-{index}"
                client.preset(index)
                state = client.save(name)
                require(read_clock(client.process.pid, client.worker) == (multiple, speed),
                        "G132_PRESET_CLOCK_CONFIGURATION")
                client.status(name, "va2", 1, False, mhz)
                report["cases"][name] = {"status": "PASS", "multiple": multiple, "speed": speed,
                                        "rendered_clock_mhz": mhz, "actual_menu_action": True}
                (output / "checkpoint.json").write_text(json.dumps(report, indent=2) + "\n")
            client.custom(3, 75)
            before = client.save("custom-h")
            require(read_clock(client.process.pid, client.worker) == (3, 75), "G132_CUSTOM_MULTIPLIER")
            client.status("custom-h", "va2", 1, False, 9)
            client.boot("va2", 1, True, "custom-s")
            client.status("custom-s", "va2", 1, True, 4)
            client.key("shift+F12")
            time.sleep(2)
            client.status("custom-h-restored", "va2", 1, False, 9)
            after = client.save("custom-h-restored")
            require(before["PCCORE"] == after["PCCORE"] and before["SYSPORTVA"] == after["SYSPORTVA"],
                    "G132_CUSTOM_RESTORE")
            events = client.events
            client.close()
            settings = (output / "config.ini").read_text()
            require(re.search(r"^clk_mult=3\s*$", settings, re.M) and
                    re.search(r"^CPU_Speed=75\s*$", settings, re.M) and
                    re.search(r"^V1V2_Standard=true\s*$", settings, re.M), "G132_CUSTOM_PERSISTENCE")
            client = Client(args, output, resume=True)
            client.events = events
            client.identity(1, True, "restarted-custom-s")
            client.status("restarted-custom-s", "va2", 1, True, 4)
            state = client.save("restarted-custom-s")
            require(read_clock(client.process.pid, client.worker) == (3, 75), "G132_CUSTOM_PERSISTENCE")
            report["cases"]["custom"] = {"status": "PASS", "multiple": 3, "speed": 75,
                "gui_custom_input": True, "reset": True, "state_restore_opposite_pending_preference": True,
                "real_process_restart": True, "persisted_configuration": True}
        require(bool(report["cases"]), "G132_EMPTY_SUITE")
        report["status"] = "PASS"
    except (QaError, OSError, ValueError, subprocess.SubprocessError) as error:
        report.update(status="FAIL", error_code=error.code if isinstance(error, QaError) else "G132_EXECUTION",
                      detail=str(error))
    finally:
        if client:
            (output / "events.json").write_text(json.dumps(client.events, indent=2) + "\n")
            try:
                client.key("Escape")
                client.close()
                report["worker_exit_status"] = client.process.returncode
            except (OSError, subprocess.SubprocessError, QaError):
                client.process.kill()
                client.process.wait()
        if output.exists():
            report["outputs"] = {str(p.relative_to(output)): digest(p) for p in output.rglob("*") if p.is_file()}
            (output / "result.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps({k: report[k] for k in ("status", "error_code", "detail") if k in report}), flush=True)
    return 0 if report["status"] == "PASS" else 1


def selftest():
    header = b"VA-EG\0".ljust(48, b"\0")
    fixture = header + b"".join(struct.pack("<10sHI", name, 0, 1) + b"x" + bytes(15)
                                for name in (b"PCCORE", b"MEMORY", b"TVRAMVA", b"MEM88MODE"))
    require(len(sections(fixture)) == 4, "G132_SELFTEST")
    alphabet = "PC88VA2|V1H|CPU8MHz|SGP8MHz"
    ref = [(1, bytes((ord(c),))) for c in alphabet]
    candidate = [(1, bytes((ord(c),))) for c in "PC88VA2|V2S|CPU"]
    status_glyphs(ref, candidate, "va2", "V2S")
    check_text("QAID= 127 249\nOk\n", r"^QAID=\s*127\s+249$", "G132_IDENTITY")
    check_text("QAREST= 1234\nOk\n", r"^QAREST=\s*1234$", "G132_STATE_VARIABLE")
    tip="CPU: 7.987200 MHz\nBase: 7.987200 MHz\nSGP: 7.987200 MHz\nMultiplier: x1.00\nSpeed: 100%\nDomain: 88 compatible\nDraws: 60.0 FPS\nGuest frames: 60.0 FPS\n"
    tooltip_text(tip)
    dynamic_fields("CPU 8MHz | 60.0FPS | FRAME 120")
    image = (960, 80, bytes(960*80*4))
    panel_lamps(image, image)
    changed = bytearray(image[2])
    changed[((80-10)*960+900)*4] = 1
    released=dict.fromkeys(("count", "index", "skipped", "phase", "shift_down", "key_down", "scripted"),0)
    paste_released(released)
    mutations = [
        (lambda: paste_released({**released,"shift_down":1}), "G132_PASTE_RELEASE"),
        (lambda: tooltip_text(tip.replace("CPU: 7.987200", "CPU: 8.000000")), "G132_TOOLTIP_PRECISION"),
        (lambda: tooltip_text(tip.replace("88 compatible", "native")), "G132_TOOLTIP_DOMAIN"),
        (lambda: dynamic_fields("CPU 8MHz | 60.0MHz | FRAME 120"), "G132_DYNAMIC_FIELDS"),
        (lambda: sections(b"bad" + fixture[3:]), "G132_STATE_FORMAT"),
        (lambda: sections(fixture[:-1]), "G132_STATE_FORMAT"),
        (lambda: panel_lamps(image, (960, 80, bytes(changed))), "G132_LAMP_RELEASE"),
        (lambda: status_glyphs(ref, candidate[:10] + [(1, b"5")] + candidate[11:], "va2", "V2S"),
         "G132_STATUS_GLYPHS"),
        (lambda: check_text("QAID= 255 249\nOk\n", r"^QAID=\s*127\s+249$", "G132_IDENTITY"), "G132_IDENTITY"),
        (lambda: check_text("QAREST= 9876\nOk\n", r"^QAREST=\s*1234$", "G132_STATE_VARIABLE"),
         "G132_STATE_VARIABLE"),
    ]
    for action, expected in mutations:
        try:
            action()
        except QaError as error:
            require(error.code == expected, "G132_SELFTEST")
        else:
            raise QaError("G132_SELFTEST")
    print("PASS: state schema, independent approved-glyph oracle, guest identity/variable and ten single-mutation exact-code negatives")
    return 0


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--selftest", action="store_true")
    for name in ("worker", "roms", "backup-seed", "output-dir", "reference-dir", "probe-disk", "probe-build"):
        parser.add_argument("--" + name, type=Path)
    parser.add_argument("--manifest-sha256")
    parser.add_argument("--evaluated-commit")
    parser.add_argument("--display")
    parser.add_argument("--scenario", choices=("state", "state-h", "state-s", "modes", "modes-va", "modes-va2", "controls", "controls-high", "cancel", "probe", "display"), default="state")
    args = parser.parse_args()
    if args.selftest:
        return selftest()
    require(all((args.worker, args.roms, args.backup_seed, args.output_dir, args.reference_dir,
                 args.manifest_sha256, args.evaluated_commit, args.display)), "G132_ARGUMENTS")
    return run(args)


if __name__ == "__main__":
    raise SystemExit(main())
