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
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
# OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
# NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
# THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

"""Build the M131 source-only N88 BASIC TVRAM probe with z80asm 1.8.

Writes a DATA loader and binary outside the repository, never a system disk.
The loader saves no file by itself; media preparation is a separate private step.
"""

import argparse
import hashlib
import json
from pathlib import Path
import subprocess


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--assembler", required=True, type=Path)
    parser.add_argument("--output-dir", required=True, type=Path)
    args = parser.parse_args()
    source = Path(__file__).resolve().parent / "probe.asm"
    root = source.parents[3]
    output = args.output_dir.resolve()
    if output == root or root in output.parents:
        parser.error("output must be outside the repository")
    version = subprocess.run(
        [str(args.assembler), "--version"], capture_output=True, text=True, check=True
    )
    if (version.stdout + version.stderr).splitlines()[0] != "Z80 assembler version 1.8":
        parser.error("z80asm 1.8 is required")
    output.mkdir(parents=True, exist_ok=False)
    binary = output / "probe.bin"
    command = [str(args.assembler), "-o", str(binary), str(source)]
    subprocess.run(command, check=True, capture_output=True, text=True)
    data = binary.read_bytes()
    if not 32 <= len(data) < 0xF00 or data[0] != 0xC3:
        parser.error("unexpected probe layout or overlap with private stack")
    # The generated standalone BASIC program carries the assembler's license.
    header = source.read_text(encoding="utf-8").split(";\n; z80asm", 1)[0]
    lines = [f"{10 + i * 10} REM {s.removeprefix(';').lstrip()}"
             for i, s in enumerate(header.splitlines())]
    lines += [
        '1000 CLEAR ,&HAFFF',
        '1010 SCREEN 1:WIDTH 80,25:CONSOLE 0,25,0,1:COLOR 7,0,0:CLS',
        f'1020 S=0:FOR I=0 TO {len(data)-1}:READ V:POKE &HB000+I,V:S=S+V:NEXT I',
        f'1030 IF S<>{sum(data)} THEN PRINT "BAD DATA CHECKSUM":END',
        '1040 PRINT "M131 TVRAM PROBE - TWO SPACE PRESSES TO RESTORE"',
        '1050 A=&HB000:CALL A',
        '1060 PRINT "PORT31=";HEX$(PEEK(&HB003));" ORIGINAL32=";HEX$(PEEK(&HB004))',
        '1070 PRINT "CPU P0=";HEX$(PEEK(&HB005));" P1=";HEX$(PEEK(&HB006))',
        '1080 PRINT "PORT32 P0=";HEX$(PEEK(&HB007));" P1=";HEX$(PEEK(&HB008))',
        '1090 PRINT "RESTORE=";PEEK(&HB009);" (MUST BE 1)"',
        '1100 IF PEEK(&HB009)<>1 THEN PRINT "RESTORE CHECK FAILED - RESET":STOP',
        '1110 END',
    ]
    for start in range(0, len(data), 16):
        lines.append(f"{2000 + start // 16 * 10} DATA " +
                     ",".join(str(b) for b in data[start:start + 16]))
    basic = ("\n".join(lines) + "\n").encode("ascii")
    (output / "m131tv.bas").write_bytes(basic)
    report = {
        "schema": 1, "assembler": "z80asm 1.8", "load_address": 0xB000,
        "bytes": len(data), "checksum": sum(data),
        "source_sha256": hashlib.sha256(source.read_bytes()).hexdigest(),
        "binary_sha256": hashlib.sha256(data).hexdigest(),
        "basic_sha256": hashlib.sha256(basic).hexdigest(),
        "reservation": "B000h-DFFFh", "disk_access": "none",
    }
    (output / "build.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps(report, indent=2))


if __name__ == "__main__":
    main()
