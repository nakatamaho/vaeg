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

"""M152 CLI contracts plus an independently sampled native guest key probe."""

import argparse
import csv
import os
from pathlib import Path
import re
import struct
import subprocess
import tempfile

HEADER = "debug-script 1\nlimit-frame 10\n"
KEYS = """ESC 0 1 2 3 4 5 6 7 8 9 MINUS CARET YEN BS TAB
Q W E R T Y U I O P AT BRACKETLEFT RETURN A S D F G H J K L SEMICOLON COLON
BRACKETRIGHT Z X C V B N M COMMA PERIOD SLASH UNDERSCORE SPACE XFER ROLLUP
ROLLDOWN INS DEL UP LEFT RIGHT DOWN HOME/CLR HELP KP_SUB KP_DIVIDE KP_7 KP_8
KP_9 KP_MULTIPLY KP_4 KP_5 KP_6 KP_ADD KP_1 KP_2 KP_3 KP_EQUAL KP_0 KP_COMMA
KP_PERIOD NFER SHIFTR KP_ENTER PC ZENKAKU STOP COPY F1 F2 F3 F4 F5 F6 F7 F8 F9
F10 SHIFT CAPS KANA GRPH CTRL""".split()


def require(condition, code, detail):
    if not condition:
        raise AssertionError(f"{code}: {detail}")


def run(worker, model, root, script):
    (root / "case.debug").write_text(script, encoding="ascii")
    output = root / "out"
    output.mkdir()
    completed = subprocess.run(
        [worker, "--model", model, "--roms", str(root / "fixture"), "--no-cfg",
         "--no-bkupmem", "--debug-script", str(root / "case.debug"),
         "--debug-output-dir", str(output)],
        cwd=root, capture_output=True, text=True, timeout=25,
        env={**os.environ, "SDL_VIDEODRIVER": "dummy", "SDL_AUDIODRIVER": "dummy"},
    )
    return completed, output


def successful_run(worker, model, root, script):
    result, output = run(worker, model, root, script)
    require(result.returncode == 0, "M152_WORKER", result.stderr)
    with (output / "events.tsv").open(encoding="ascii", newline="") as events:
        rows = list(csv.reader(events, delimiter="\t"))
    require(rows[0] == ["event", "frame", "id", "value"], "M152_EVENTS_HEADER", rows[0])
    return rows[1:], output


def key_events(events):
    return [row for row in events if row[0].startswith("key-")]


def parser_contract(worker, work):
    for label in ("valid", "unknown", "host-only", "exit", "end", "limit"):
        (work / label / "fixture").mkdir(parents=True)
    actions = "".join(f"key-down {key}\nkey-up {key}\n" for key in KEYS)
    good = HEADER + actions + "exit\n"
    events, _ = successful_run(worker, "va", work / "valid", good)
    transitions = key_events(events)
    require(len(transitions) == 2 * len(KEYS) and all(
        make[:2] == ["key-down", "0"] and brk[:2] == ["key-up", "0"] and make[2:] == brk[2:]
        for make, brk in zip(transitions[::2], transitions[1::2])
    ), "M152_KEY_PARSE_ORDER", transitions)
    for label, name in (("unknown", "unknown-key"), ("host-only", "fast_forward")):
        bad = good.replace("key-down ESC\n", f"key-down {name}\n", 1)
        result, output = run(worker, "va", work / label, bad)
        codes = re.findall(r"^(M152_DEBUG_\w+): line (\d+)$", result.stderr, re.M)
        require(result.returncode != 0 and codes == [("M152_DEBUG_KEY_NAME", "3")]
                and not (output / "events.tsv").exists(), "M152_KEY_PARSE_ERROR", result.stderr)

    for label, end, expected_frame, terminal in (
        ("exit", "wait-frame 2\nexit\n", "2", "exit"),
        ("end", "wait-frame 2\n", "2", "frame-limit"),
        ("limit", "wait-frame 11\n", "10", "frame-limit"),
    ):
        script = HEADER + "key-down SHIFT\nkey-down shiftl\nkey-up CTRL\n" + end
        events, _ = successful_run(worker, "va", work / label, script)
        require(key_events(events) == [["key-down", "0", "shiftl", "112"],
                                       ["key-release", expected_frame, "shiftl", "112"]]
                and events[-1][0] == terminal, "M152_KEY_CLEANUP", events)
    print("PASS M152 parser: matrix names, line diagnostics, alias/idempotence, exit/end/limit cleanup")


def guest_fixture(root, model):
    # Standalone native reset program, no BIOS calls. Initialize the keyboard,
    # set distinctive video values via OUT, then sample three I/O matrix rows
    # at each rising VRTC edge. SI is the guest's independent sample index.
    code = bytearray.fromhex("fa ba9701 b001 ee 31f6")  # CLI; OUT 197h,1; XOR SI,SI
    writes = [(0x100, 0x3000), (0x106, 0x008A), (0x108, 0x0089),
              (0x10C, 0x01C0), (0x30A, 0x6ABC)]
    for i in range(4):
        base = 0x200 + 0x20 * i
        writes.extend(((base + 4, 0x140 + 4 * i), (base + 0xE, 0x1234 + 4 * i),
                       (base + 0x10, i)))
    for port, value in writes:
        code += struct.pack("<BHBHB", 0xBA, port, 0xB8, value, 0xEF)  # MOV DX; MOV AX; OUT DX,AX
    loop = len(code)
    code += bytes.fromhex(
        "e440 a820 75fa"  # Wait while VRTC (port 40h bit 5) is high.
        "e440 a820 74fa"  # Then wait while it is low.
        "46"             # INC SI
        "e408 88c3"       # IN AL,08h; MOV BL,AL (RIGHT is active-low bit 2).
        "e402 88c7"       # IN AL,02h; MOV BH,AL (A is active-low bit 1).
        "e40e"            # IN AL,0Eh (right SHIFT is active-low bit 3).
    )
    sample_ready = len(code)
    code += struct.pack("<Bb", 0xEB, loop - sample_ready - 2)  # JMP SHORT loop
    image = bytearray(0x20000)
    image[:len(code)] = code
    image[0xFFF0:0xFFF5] = bytes.fromhex("ea 0000 00f0")  # JMP FAR F000:0000
    fixture = root / "fixture"
    fixture.mkdir()
    filename = "varom1.rom" if model == "va" else "varom1_va2.rom"
    (fixture / filename).write_bytes(image)
    return f"f000:{sample_ready:04x}"


def read_tsv(path, code):
    raw = path.read_bytes()
    require(raw.endswith(b"\n") and b"\r" not in raw, code, "expected LF-terminated lines")
    rows = [line.split("\t") for line in raw.decode("ascii").splitlines()]
    require(all(len(row) == 2 for row in rows), code, rows)
    values = dict(rows)
    require(len(values) == len(rows), code, "duplicate keys")
    return values


def check_video(path):
    code = "M152_VIDEO_SCHEMA"
    values = read_tsv(path, code)
    sizes = {"crtmode": 2, "hsync_mode": 1, "palette_mode": 1}
    sizes.update(dict.fromkeys("""
        grmode grres colcomp rgbcomp mskmode palmode dropcol pagemsk xpar_g0
        xpar_g1 xpar_txtspr mskleft mskrit msktop mskbot blinkcnt
    """.split(), 4))
    sizes.update({"txtmode8": 2, "txtmode": 2})
    for i in range(4):
        for field in "fsa fbw fbl dot ofx ofy dsa dsh dsp".split():
            sizes[f"fb.{i}.{field}"] = 8 if field in ("fsa", "dsa") else 4
    sizes.update({f"palette.{i}": 4 for i in range(32)})
    sizes.update({f"tsp.syncparam.{i}": 2 for i in range(14)})
    sizes.update({f"layer.{i}.enabled": 1 for i in range(4)})
    sizes.update(dict.fromkeys("monitor_digital mode_88 pc8001 n80.color hfplus.active vab.active".split(), 1))
    sizes.update(dict.fromkeys("n80.model n80.port33 n80.video.0 n80.video.1".split(), 2))
    require(values.get("schema") == "vaeg-video-v1" and values.keys() == {"schema", *sizes},
            code, values.keys())
    for field, width in sizes.items():
        require(re.fullmatch(f"[0-9a-f]{{{width}}}", values[field]), code, field)
    expected = {"crtmode": "01", "hsync_mode": "2", "palette_mode": "3", "palmode": "01c0",
                "grmode": "3000", "colcomp": "008a", "rgbcomp": "0089", "palette.5": "6ebd"}
    for i in range(4):
        expected[f"fb.{i}.fbw"] = f"{0x140 + 4 * i:04x}"
        expected[f"fb.{i}.dsa"] = f"{(i << 16) + 0x1234 + 4 * i:08x}"
    actual = {field: values[field] for field in expected}
    require(actual == expected, "M152_VIDEO_LIVE_VALUES", actual)


def guest_contract(worker, model, root):
    address = guest_fixture(root, model)
    script = HEADER
    steps = ["", "key-down RIGHT\nkey-down A\nkey-down SHIFTR\n",
             "key-up RIGHT\nkey-up SHIFTR\n", "", "key-up a\n"]
    for i, keys in enumerate(steps, 1):
        if i > 1:
            script += f"wait-frame {i}\n" + keys
        items = "registers video" if i == 1 else "registers"
        script += f"wait-pc {address} 1\ncapture sample-{i} {items}\n"
    events, output = successful_run(worker, model, root, script + "exit\n")
    expected_keys = [["key-down", "2", "right", "60"], ["key-down", "2", "a", "29"],
                     ["key-down", "2", "shiftr", "88"], ["key-up", "3", "right", "60"],
                     ["key-up", "3", "shiftr", "88"], ["key-up", "5", "a", "29"]]
    require(key_events(events) == expected_keys and events[-1][0] == "exit",
            "M152_KEY_FRAME_ORDER", events)
    captures = [row for row in events if row[0] == "capture"]
    require(captures == [["capture", str(i), f"sample-{i}", "17" if i == 1 else "1"]
                         for i in range(1, 6)], "M152_CAPTURE_ORDER", captures)
    matrix = []
    for i in range(1, 6):
        registers = read_tsv(output / f"sample-{i}.registers.tsv", "M152_GUEST_REGISTERS")
        require(int(registers["si"], 16) == i, "M152_VRTC_SAMPLE", registers)
        matrix.append((registers["bx"], int(registers["ax"], 16) & 0xFF))
    # Right Shift also clears the combined-SHIFT bit (port 08h bit 6).
    require(matrix == [("ffff", 0xFF), ("fdbb", 0xF7), ("fdff", 0xFF), ("fdff", 0xFF),
                       ("ffff", 0xFF)], "M152_GUEST_MATRIX_DURATION", matrix)
    check_video(output / "sample-1.video.tsv")
    print(f"PASS M152 {model}: five consecutive VRTC samples; RIGHT/SHIFTR 1 frame, A 3 frames; live video v1")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("worker", type=lambda path: str(Path(path).resolve()))
    parser.add_argument("case", choices=("parser", "va", "va2"))
    args = parser.parse_args()
    with tempfile.TemporaryDirectory(prefix=f"m152-{args.case}-", dir=Path.cwd()) as work:
        if args.case == "parser":
            parser_contract(args.worker, Path(work))
        else:
            guest_contract(args.worker, args.case, Path(work))


if __name__ == "__main__":
    main()
