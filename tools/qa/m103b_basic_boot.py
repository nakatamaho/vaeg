#!/usr/bin/env python3
# Copyright (c) 2026 Nakata Maho
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
# 1. Redistributions of source code must retain the above copyright notice,
#    this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR IMPLIED
# WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
# EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
# DAMAGE.

"""V2 disk BASIC acceptance (M103b, extended in M103c).

The run mode boots maintainer-local VA2 ROMs and V2 BASIC media without any
mode override, answers the first prompt, types `PRINT 1+1`, and checks the
88-mode text area in the TVRAM dump written at exit: the answer ` 2` follows
the typed line and an `Ok` prompt is present. Private ROMs and media are
never copied into the tree; the media is copied to a temporary directory.
M103b's original check classified a trace of the interrupt-free key-input
wait loop; M103c delivers interrupts, so the visible screen content is the
criterion instead.
"""

from __future__ import annotations

import argparse
import os
import pathlib
import shutil
import struct
import subprocess
import sys
import tempfile
from typing import List, Optional

SKIP_RETURN_CODE = 77

# 88-mode text: TVRAM byte 6000h is compatible F000h (BNN manual 8.2.1).
# N88-BASIC programs the text DMA start F3C8h with 120-byte rows (80
# characters and 20 attribute pairs).
TEXT_AREA = 0x6000
TEXT_START = 0x63C8
ROW_PITCH = 120
ROW_CHARS = 80
MAX_ROWS = 25

ERR_NO_DUMP = "M103B_NO_TVRAM_DUMP"
ERR_NO_PROMPT = "M103B_NO_BASIC_PROMPT"
ERR_NO_ANSWER = "M103B_NO_ANSWER"
ERR_WORKER = "M103B_WORKER_FAILED"

INPUT_SCRIPT = "@wait 3000\n3\n@wait 1200\nPRINT 1+1\n@wait 1200\n"


def textmem_from_dump(dump: bytes) -> Optional[bytes]:
    if len(dump) < 12 or dump[:8] != b"VAEGSCN1":
        return None
    run_id_length = struct.unpack_from("<I", dump, 12)[0]
    pos = 16 + run_id_length + 6 * 4
    if len(dump) < pos + 4:
        return None
    size = struct.unpack_from("<I", dump, pos)[0]
    data = dump[pos + 4 : pos + 4 + size]
    if len(data) != size or size < TEXT_AREA + 0x1000:
        return None
    return data


def screen_rows(textmem: bytes) -> List[str]:
    rows = []
    for row in range(MAX_ROWS):
        start = TEXT_START + row * ROW_PITCH
        if start + ROW_CHARS > TEXT_AREA + 0x1000:
            break
        chars = textmem[start : start + ROW_CHARS]
        rows.append("".join(chr(c) if 0x20 <= c < 0x7F else " " for c in chars).rstrip())
    return rows


def classify(dump: bytes) -> Optional[str]:
    """Return None when BASIC answered, otherwise one stable error code."""
    textmem = textmem_from_dump(dump)
    if textmem is None:
        return ERR_NO_DUMP
    rows = screen_rows(textmem)
    if not any(row.startswith("Ok") for row in rows):
        return ERR_NO_PROMPT
    for index, row in enumerate(rows[:-1]):
        if row.startswith("PRINT 1+1") and rows[index + 1].strip() == "2":
            return None
    return ERR_NO_ANSWER


def _fixture(rows: List[str], magic: bytes = b"VAEGSCN1") -> bytes:
    textmem = bytearray(0x40000)
    for index, text in enumerate(rows):
        start = TEXT_START + index * ROW_PITCH
        textmem[start : start + len(text)] = text.encode("ascii")
    header = magic + struct.pack("<II", 1, 0) + struct.pack("<6I", 0, 0, 20, 0, 0, 0)
    return header + struct.pack("<I", len(textmem)) + bytes(textmem)


PASSING_ROWS = ["", "Ok", "PRINT 1+1", " 2", "Ok"]


def selftest() -> int:
    failures = []
    if classify(_fixture(PASSING_ROWS)) is not None:
        failures.append("passing fixture rejected")
    # Each case applies exactly one controlled mutation to the passing fixture.
    cases = [
        ("bad magic", _fixture(PASSING_ROWS, b"VAEGSCN0"), ERR_NO_DUMP),
        ("truncated", _fixture(PASSING_ROWS)[:-0x3A000], ERR_NO_DUMP),
        ("no prompt", _fixture([r.replace("Ok", "OK") for r in PASSING_ROWS]), ERR_NO_PROMPT),
        ("wrong answer", _fixture([r.replace(" 2", " 3") for r in PASSING_ROWS]), ERR_NO_ANSWER),
        ("answer not after the line", _fixture(["Ok", " 2", "PRINT 1+1", "Ok"]), ERR_NO_ANSWER),
    ]
    for name, dump, expected in cases:
        actual = classify(dump)
        if actual != expected:
            failures.append(f"{name}: expected {expected}, got {actual}")
    for failure in failures:
        print(f"m103b-basic-boot selftest: {failure}", file=sys.stderr)
    if failures:
        return 1
    print("m103b-basic-boot selftest: all classifications passed")
    return 0


def run(worker: str, roms: str, media: str, frames: int, timeout: int) -> int:
    # The worker runs in a temporary directory, so resolve every input first.
    worker = os.path.abspath(worker)
    roms = os.path.abspath(roms)
    media = os.path.abspath(media)
    with tempfile.TemporaryDirectory(prefix="vaeg-m103b-") as work:
        workdir = pathlib.Path(work)
        disk = workdir / "boot.d88"
        shutil.copyfile(media, disk)
        script = workdir / "input.txt"
        script.write_text(INPUT_SCRIPT, encoding="ascii")
        dump_path = workdir / "tvram.bin"
        env = dict(os.environ)
        env.update({"SDL_VIDEODRIVER": "dummy", "SDL_AUDIODRIVER": "dummy"})
        command = [
            worker,
            "--nowait",
            "--no-cfg",
            "--no-bkupmem",
            "--roms",
            roms,
            "--model",
            "VA2",
            "--fdd1",
            str(disk),
            "--headless-input-script",
            str(script),
            "--screen-tvram-dump",
            str(dump_path),
            "--screenshot",
            f"{frames}:{workdir / 'final.png'}",
        ]
        completed = subprocess.run(
            command,
            cwd=work,
            env=env,
            capture_output=True,
            text=True,
            errors="replace",
            timeout=timeout,
            check=False,
        )
        if completed.returncode != 0:
            print(f"{ERR_WORKER}: worker exit status {completed.returncode}", file=sys.stderr)
            return 1
        dump = dump_path.read_bytes() if dump_path.exists() else b""
    error = classify(dump)
    if error is not None:
        print(f"{error}: 88-mode text area does not show the expected answer", file=sys.stderr)
        return 1
    print("m103b-basic-boot: disk BASIC answered PRINT 1+1 on the 88-mode text screen")
    return 0


def main(argv: Optional[List[str]] = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="mode", required=True)
    sub.add_parser("selftest")
    skip_parser = sub.add_parser("skip")
    skip_parser.add_argument("--reason", required=True)
    run_parser = sub.add_parser("run")
    run_parser.add_argument("--worker", required=True)
    run_parser.add_argument("--roms", required=True)
    run_parser.add_argument("--media", required=True)
    run_parser.add_argument("--frames", type=int, default=6600)
    run_parser.add_argument("--timeout", type=int, default=600)
    args = parser.parse_args(argv)
    if args.mode == "selftest":
        return selftest()
    if args.mode == "skip":
        print(f"m103b-basic-boot: skipped: {args.reason}")
        return SKIP_RETURN_CODE
    return run(args.worker, args.roms, args.media, args.frames, args.timeout)


if __name__ == "__main__":
    sys.exit(main())
