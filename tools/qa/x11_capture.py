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

"""Capture the actual X11 client owned by an explicitly identified QA worker.

Requires an existing X11 server/window manager and xdotool, xprop, xwininfo,
xwd. Private inputs and outputs stay outside Git. Virtual X11 is supported
and must be labeled explicitly; it does not prove physical GPU/macOS behavior.
This adapter does not approve reference images or promote pixel matches to a
milestone PASS. Exit zero means CAPTURED, not visual acceptance.
"""

from __future__ import annotations

import argparse
import configparser
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import signal
import struct
import subprocess
import time

from m103b_basic_boot import screen_rows, textmem_from_dump
from screen_compare import png_bytes, require, QaError

ROOT = Path(__file__).resolve().parents[2]
CASE_KEYS = {"id", "width", "height", "status", "panel", "fullscreen", "ui_scale", "renderer"}
GUEST_LINES = ['10 CLS:LOCATE 0,0,0:PRINT "LOCAL DISPLAY QA"', '20 GOTO 20', 'RUN']


def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def validate_case(case):
    require(isinstance(case, dict) and set(case) == CASE_KEYS, "M132_UI_CASE")
    require(isinstance(case["id"], str) and re.fullmatch(r"[a-z0-9-]+", case["id"]), "M132_UI_CASE")
    require(type(case["width"]) is int and 320 <= case["width"] <= 4096 and
            type(case["height"]) is int and 240 <= case["height"] <= 4096, "M132_UI_CASE")
    require(all(type(case[key]) is bool for key in ("status", "panel", "fullscreen")), "M132_UI_CASE")
    require(case["ui_scale"] in (100, 200) and case["renderer"] in ("software", "opengl"), "M132_UI_CASE")
    return case


def default_cases():
    return [validate_case({"id": f"{renderer}-{size}-s{int(status)}p{int(panel)}",
                          "width": width, "height": height, "status": status, "panel": panel,
                          "fullscreen": fullscreen, "ui_scale": scale, "renderer": renderer})
            for renderer in ("software", "opengl")
            for size, width, height, fullscreen, scale in (
                ("normal", 640, 422, False, 100), ("narrow", 320, 240, False, 100),
                ("fullscreen", 640, 422, True, 100), ("large-ui", 960, 640, False, 200))
            for status in (False, True) for panel in (False, True)]


def decode_xwd(data):
    """XWD v7 ZPixmap, TrueColor/DirectColor; DirectColor uses its actual LUT."""
    require(len(data) >= 100, "M132_UI_XWD_INVALID")
    header = struct.unpack_from(">25I", data)
    size, version, form, depth, w, h, offset, order = header[:8]
    bpp, stride, visual, red, green, blue = header[11:17]
    colours = header[19]
    require(100 <= size <= len(data) and version == 7, "M132_UI_XWD_INVALID")
    require(form == 2 and visual in (4, 5) and offset == 0 and order in (0, 1) and
            bpp in (16, 24, 32) and 0 < depth <= bpp, "M132_UI_XWD_UNSUPPORTED")
    require(0 < w <= 8192 and 0 < h <= 8192 and w * h <= 32000000 and
            stride >= w * (bpp // 8) and colours <= 65536, "M132_UI_XWD_INVALID")
    start = size + colours * 12
    require(start + h * stride == len(data), "M132_UI_XWD_INVALID")
    masks = []
    for mask in (red, green, blue):
        require(mask > 0 and mask < 1 << bpp, "M132_UI_XWD_UNSUPPORTED")
        shift = (mask & -mask).bit_length() - 1
        maximum = mask >> shift
        require(maximum & (maximum + 1) == 0, "M132_UI_XWD_UNSUPPORTED")
        masks.append((mask, shift, maximum))
    require(not (red & green or red & blue or green & blue), "M132_UI_XWD_UNSUPPORTED")
    tables = [{}, {}, {}]
    if visual == 5:
        require(colours > 0, "M132_UI_XWD_UNSUPPORTED")
        for i in range(colours):
            pixel, r, g, b, flags, _ = struct.unpack_from(">IHHHBB", data, size + i * 12)
            for channel, ((mask, shift, _), value) in enumerate(zip(masks, (r, g, b))):
                if flags & (1 << channel):
                    index, value8 = (pixel & mask) >> shift, (value + 128) // 257
                    require(index not in tables[channel] or tables[channel][index] == value8,
                            "M132_UI_XWD_UNSUPPORTED")
                    tables[channel][index] = value8
    pixels = bytearray()
    for y in range(h):
        for x in range(w):
            pos = start + y * stride + x * (bpp // 8)
            value = int.from_bytes(data[pos:pos + bpp // 8], "little" if order == 0 else "big")
            for channel, (mask, shift, maximum) in enumerate(masks):
                index = (value & mask) >> shift
                if visual == 5:
                    require(index in tables[channel], "M132_UI_XWD_UNSUPPORTED")
                    pixels.append(tables[channel][index])
                else:
                    pixels.append(index * 255 // maximum)
            pixels.append(255)
    return w, h, bytes(pixels)


def observe(properties, geometry, pid):
    require(re.search(r"_NET_WM_PID\(CARDINAL\) = " + str(pid) + r"\s*$", properties, re.M),
            "M132_UI_WINDOW_OWNER")
    require("Map State: IsViewable" in geometry, "M132_UI_WINDOW_NOT_VIEWABLE")
    width = re.search(r"^\s*Width: (\d+)$", geometry, re.M)
    height = re.search(r"^\s*Height: (\d+)$", geometry, re.M)
    require(width is not None and height is not None, "M132_UI_WINDOW_GEOMETRY")
    title = re.search(r'^_NET_WM_NAME\(UTF8_STRING\) = "(.*)"$', properties, re.M)
    require(title is not None and title[1].startswith("88VA Eternal Grafx "), "M132_UI_WINDOW_TITLE")
    require(not re.search(r"PC-88|\bCPU\b|MHz|V[12][HS]", title[1]), "M132_UI_TITLE_DUPLICATION")
    return {"width": int(width[1]), "height": int(height[1]), "title": title[1],
            "fullscreen": "_NET_WM_STATE_FULLSCREEN" in properties}


def persisted_settings(text):
    parser = configparser.ConfigParser(interpolation=None)
    parser.read_string(text)
    section = parser["NekoProjectII"]
    require("MachineStatus" in section and "FrontPanelMode" in section and "GUI_ui_scale" in section,
            "M132_UI_PERSISTED_SETTINGS")
    return {"status": bool(section.getint("MachineStatus")), "panel": bool(section.getint("FrontPanelMode")),
            "ui_scale": section.getint("GUI_ui_scale")}


def command(argv, env, timeout=10):
    return subprocess.run(argv, env=env, check=True, text=True, stdout=subprocess.PIPE,
                          stderr=subprocess.PIPE, timeout=timeout).stdout


def basic_boot_action(rows):
    # Copy Screen can include the non-ASCII blinking cursor after ASCII Ok.
    # Normalize as the interactive adapter does, without ignoring ASCII errors.
    rows = [''.join(c if 32 <= ord(c) < 127 else ' ' for c in row) for row in rows]
    if any(row.strip() == 'Ok' for row in rows):
        return 'ready'
    if any(re.search(r'How\s+many\s+files\s*\(0-15\)\?', row, re.I) for row in rows):
        return 'enter'
    return 'wait'


def clipboard_rows(env):
    # --screen-tvram-dump is an exit-time dump, not a live observation.
    # Use the real Copy Screen shortcut; a fresh sentinel prevents accepting
    # another client's stale clipboard when this SDL client has not replied.
    sentinel = 'X11_BOOT_QUERY_' + str(time.monotonic_ns())
    subprocess.run(['xclip', '-selection', 'clipboard', '-i'], input=sentinel.encode(),
                   env=env, check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, timeout=5)
    command(['xdotool', 'key', '--clearmodifiers', 'ctrl+shift+c'], env)
    deadline = time.monotonic() + 1
    while time.monotonic() < deadline:
        value = command(['xclip', '-selection', 'clipboard', '-o'], env)
        if value != sentinel:
            return value.splitlines()
        time.sleep(0.05)
    return []


def wait_basic_boot(worker, env, enter, timeout):
    deadline, answered, action = time.monotonic() + timeout, False, 'wait'
    while worker.poll() is None and time.monotonic() < deadline:
        action = basic_boot_action(clipboard_rows(env))
        if action == 'ready':
            return answered
        if action == 'enter' and not answered:
            enter()
            answered = True
        time.sleep(0.1)
    require(worker.poll() is None, 'M132_UI_WORKER_EARLY_EXIT')
    raise QaError('M132_UI_BOOT_NOT_READY')


def capture(args):
    require(args.display and args.display_kind in ("virtual-x11", "physical-x11"), "M132_UI_DISPLAY_REQUIRED")
    for tool in ("xdotool", "xwininfo", "xprop", "xwd", "xclip"):
        require(shutil.which(tool) is not None, "M132_UI_DEPENDENCY_MISSING")
    case = validate_case(json.loads(args.case.read_text()))
    output = args.output_dir.resolve()
    require(output != ROOT and ROOT not in output.parents and not output.exists(),
            "M132_UI_OUTPUT_MUST_BE_NEW_AND_OUT_OF_TREE")
    require(re.fullmatch(r"[0-9a-f]{40}", args.evaluated_commit) and
            args.evaluated_commit.encode() in args.worker.read_bytes(), "M132_UI_WORKER_COMMIT")
    require(args.roms.is_dir() and args.backup_seed.is_file(), "M132_UI_INPUTS")
    env = {**os.environ, "DISPLAY": args.display, "SDL_VIDEODRIVER": "x11",
           "SDL_RENDER_DRIVER": case["renderer"], "SDL_AUDIODRIVER": "dummy"}
    screen_size = [int(n) for n in command(["xdotool", "getdisplaygeometry"], env).split()]
    require(len(screen_size) == 2, "M132_UI_DISPLAY_GEOMETRY")
    output.mkdir(parents=True)
    shutil.copyfile(args.backup_seed, output / "backup.bin")
    config = ("[NekoProjectII]\nMain_RAM=640\nMain_RAM_Auto=false\nBootMode=1\nCPU_Speed=100\n"
              "V1V2_Standard=false\nDspClock=5\nF12_bind=8\nNativeCRT=false\n"
              f"MachineStatus={int(case['status'])}\nFrontPanelMode={int(case['panel'])}\n"
              f"GUI_win_cx={case['width']}\nGUI_win_cy={case['height']}\nGUI_ui_scale={case['ui_scale']}\n")
    (output / "config.ini").write_text(config)
    (output / "config.initial.ini").write_text(config)
    lines = GUEST_LINES
    (output / "input.txt").write_text("\n".join(lines) + "\n")
    argv = [str(args.worker.resolve()), "--roms", str(args.roms.resolve()), "--cfg", str(output / "config.ini"),
            "--bkupmem", str(output / "backup.bin"), "--model", "va2", "--mute", "--nowait", "--debug",
            "--fullscreen" if case["fullscreen"] else "--windowed", "--cpumult", "2",
            "--keyboard-layout", "us", "--screen-tvram-dump", str(output / "tvram.bin")]
    for slot in ["fdd1", "fdd2", "sasi1", "sasi2", *(f"scsi{i}" for i in range(7))]:
        argv.extend(["--" + slot, "none"])
    report = {"contract": "x11-client-capture-v1", "status": "INCOMPLETE", "gate_pass": False,
              "case": case, "argv": argv, "display": args.display, "display_kind": args.display_kind,
              "evaluated_commit": args.evaluated_commit, "worker_sha256": digest(args.worker),
              "adapter_sha256": digest(__file__), "backup_seed_sha256": digest(args.backup_seed),
              "rom_files": {str(path.relative_to(args.roms)): digest(path)
                            for path in sorted(args.roms.rglob("*"))
                            if path.is_file() and path.suffix.lower() == ".rom"}}
    worker = None
    stopped = False
    try:
        with (output / "stdout.log").open("wb") as out, (output / "stderr.log").open("wb") as err:
            worker = subprocess.Popen(argv, cwd=output, env=env, stdout=out, stderr=err)
            deadline, window = time.monotonic() + args.timeout, None
            while time.monotonic() < deadline and worker.poll() is None:
                search = subprocess.run(["xdotool", "search", "--onlyvisible", "--pid", str(worker.pid)],
                                        env=env, text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
                candidates = search.stdout.split()
                if len(candidates) == 1:
                    window = candidates[0]
                    break
                time.sleep(0.1)
            require(window is not None, "M132_UI_WINDOW_UNAVAILABLE")
            command(["xdotool", "windowactivate", "--sync", window], env)
            command(["xdotool", "mousemove", str(screen_size[0] - 2), str(screen_size[1] // 2)], env)
            def enter():
                command(["xdotool", "keydown", "Return"], env)
                time.sleep(0.10)
                command(["xdotool", "keyup", "Return"], env)
                time.sleep(0.10)
            # A fixed delay can consume program line 10 as the file-count
            # response on a slower presenter. Observe both guest transitions.
            report['boot_prompt_answered'] = wait_basic_boot(
                worker, env, enter, args.timeout)
            report['boot_ready'] = True
            for line in lines:
                command(["xdotool", "type", "--clearmodifiers", "--delay", "100", line], env)
                enter()
            time.sleep(args.settle)
            require(worker.poll() is None, "M132_UI_WORKER_EARLY_EXIT")
            os.kill(worker.pid, signal.SIGSTOP)
            stopped = True
            # The X server remains live while the client's pixels/state are frozen.
            properties = command(["xprop", "-id", window, "_NET_WM_PID", "_NET_WM_NAME", "_NET_WM_STATE"], env)
            geometry = command(["xwininfo", "-id", window], env)
            state = observe(properties, geometry, worker.pid)
            require(Path(f"/proc/{worker.pid}/exe").resolve() == args.worker.resolve(), "M132_UI_PROCESS_IMAGE")
            command(["xwd", "-silent", "-id", window, "-out", str(output / "client.xwd")], env)
            image = decode_xwd((output / "client.xwd").read_bytes())
            require(image[:2] == (state["width"], state["height"]), "M132_UI_IMAGE_GEOMETRY")
            require(state["fullscreen"] == case["fullscreen"], "M132_UI_FULLSCREEN_STATE")
            (output / "client.png").write_bytes(png_bytes(image))
            (output / "xprop.txt").write_text(properties)
            (output / "xwininfo.txt").write_text(geometry)
            os.kill(worker.pid, signal.SIGCONT)
            stopped = False
            command(["xdotool", "windowactivate", "--sync", window, "key", "--clearmodifiers", "alt+F4"], env)
            report["worker_exit_status"] = worker.wait(timeout=10)
            require(worker.returncode == 0, "M132_UI_WORKER_EXIT")
        settings = persisted_settings((output / "config.ini").read_text())
        require(all(settings[key] == case[key] for key in ("status", "panel", "ui_scale")),
                "M132_UI_SETTINGS_MISMATCH")
        memory = textmem_from_dump((output / "tvram.bin").read_bytes())
        require(memory is not None and any(row.strip() == "LOCAL DISPLAY QA" for row in screen_rows(memory)),
                "M132_UI_GUEST_NOT_READY")
        renderer = re.search(r"SDL renderer backend: (\S+) \(([^)]+)\)", (output / "stderr.log").read_text())
        require(renderer is not None and renderer[1] == case["renderer"], "M132_UI_RENDERER_MISMATCH")
        metadata = {"capture_kind": "final-client", "image_sha256": digest(output / "client.png"),
                    "target": {"platform": "linux", "backend": "sdl-" + renderer[1], "video_driver": "x11",
                               "dpi_scale": 1, "logical_size": [state["width"], state["height"]],
                               "fullscreen": state["fullscreen"], "status_visible": settings["status"],
                               "panel_visible": settings["panel"] and not state["fullscreen"]},
                    "observations": state, "saved_settings": settings, "display_kind": args.display_kind,
                    "screen_size": screen_size,
                    "settings_basis": "persisted application settings plus EWMH fullscreen; pixel rendering checked separately",
                    "worker_sha256": report["worker_sha256"], "evaluated_commit": args.evaluated_commit,
                    "adapter_sha256": report["adapter_sha256"], "xwd_sha256": digest(output / "client.xwd")}
        (output / "capture.json").write_text(json.dumps(metadata, indent=2) + "\n")
        report["status"] = "CAPTURED_UNREVIEWED"
    except (QaError, subprocess.SubprocessError, OSError, ValueError) as error:
        report.update(status="FAIL", error_code=error.code if isinstance(error, QaError) else "M132_UI_EXECUTION")
    finally:
        if worker is not None and worker.poll() is None:
            if stopped:
                os.kill(worker.pid, signal.SIGCONT)
            worker.terminate()
            try:
                worker.wait(timeout=5)
            except subprocess.TimeoutExpired:
                worker.kill()
                worker.wait(timeout=5)
        report["output_digests"] = {path.name: digest(path) for path in output.iterdir() if path.is_file()}
        (output / "result.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps({"status": report["status"], "case": case["id"], "error_code": report.get("error_code")}))
    return 0 if report["status"] == "CAPTURED_UNREVIEWED" else 1


def selftest():
    require(basic_boot_action(['How many files(0-15)?']) == 'enter' and
            basic_boot_action(['How many files(0-15)?', 'Ok']) == 'ready' and
            basic_boot_action(['booting']) == 'wait' and
            basic_boot_action(['NotOk']) == 'wait' and
            basic_boot_action(['Ok\ufffd']) == 'ready' and
            basic_boot_action(['NotOk\ufffd']) == 'wait' and
            basic_boot_action(['OkA']) == 'wait', 'M132_UI_BOOT_SELFTEST')
    cases = default_cases()
    require(len(cases) == 32 and len({case["id"] for case in cases}) == 32, "M132_UI_SELFTEST")
    header = [100, 7, 2, 24, 2, 1, 0, 0, 32, 0, 32, 32, 8, 4, 0xff0000, 0xff00, 0xff, 8, 256, 0, 2, 1, 0, 0, 0]
    fixture = struct.pack(">25I", *header) + bytes((3, 2, 1, 0, 6, 5, 4, 0))
    require(decode_xwd(fixture) == (2, 1, bytes((1, 2, 3, 255, 4, 5, 6, 255))), "M132_UI_SELFTEST")
    direct_header = [100, 7, 2, 3, 2, 1, 0, 0, 16, 0, 16, 16, 4, 5, 4, 2, 1, 1, 2, 2, 2, 1, 0, 0, 0]
    direct = (struct.pack(">25I", *direct_header) +
              struct.pack(">IHHHBB", 0, 0, 0, 0, 7, 0) +
              struct.pack(">IHHHBB", 7, 65535, 65535, 65535, 7, 0) + b"\x01\x00\x06\x00")
    require(decode_xwd(direct) == (2, 1, bytes((0, 0, 255, 255, 255, 255, 0, 255))), "M132_UI_SELFTEST")
    props = '_NET_WM_PID(CARDINAL) = 123\n_NET_WM_NAME(UTF8_STRING) = "88VA Eternal Grafx Rel.test"\n_NET_WM_STATE(ATOM) = \n'
    geom = '  Width: 640\n  Height: 422\n  Map State: IsViewable\n'
    require(observe(props, geom, 123)["width"] == 640, "M132_UI_SELFTEST")
    negatives = [
        (lambda: decode_xwd(fixture[:-1]), "M132_UI_XWD_INVALID"),
        (lambda: observe(props.replace("= 123", "= 124"), geom, 123), "M132_UI_WINDOW_OWNER"),
        (lambda: observe(props, geom.replace("IsViewable", "IsUnMapped"), 123), "M132_UI_WINDOW_NOT_VIEWABLE"),
        (lambda: observe(props.replace("Rel.test", "Rel.test CPU 8MHz"), geom, 123), "M132_UI_TITLE_DUPLICATION"),
        (lambda: validate_case({**cases[0], "width": 0}), "M132_UI_CASE"),
    ]
    for action, expected in negatives:
        try:
            action()
        except QaError as error:
            require(error.code == expected, "M132_UI_SELFTEST")
        else:
            raise QaError("M132_UI_SELFTEST")
    print("PASS: case matrix, XWD decoder, X11 observations and exact-code negatives")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--selftest", action="store_true")
    parser.add_argument("--list-cases", action="store_true")
    for name in ("worker", "roms", "backup-seed", "case", "output-dir"):
        parser.add_argument("--" + name, type=Path)
    parser.add_argument("--evaluated-commit")
    parser.add_argument("--display")
    parser.add_argument("--display-kind", choices=("virtual-x11", "physical-x11"))
    parser.add_argument("--settle", type=float, default=1)
    parser.add_argument("--timeout", type=float, default=15)
    args = parser.parse_args()
    try:
        if args.selftest:
            selftest()
            return 0
        if args.list_cases:
            print(json.dumps(default_cases(), indent=2))
            return 0
        require(all((args.worker, args.roms, args.backup_seed, args.case, args.output_dir,
                     args.evaluated_commit)) and 0 < args.settle <= 60 and 0 < args.timeout <= 60,
                "M132_UI_ARGUMENTS")
        return capture(args)
    except (QaError, OSError, ValueError, subprocess.SubprocessError) as error:
        print(json.dumps({"status": "BLOCKED", "error_code": error.code if isinstance(error, QaError)
                          else "M132_UI_PREFLIGHT"}))
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
