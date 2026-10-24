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

"""Run with VAEG_Z80ASM pointing to the repository's cached z80asm 1.8."""

import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest

ASSEMBLER = os.environ.get("VAEG_Z80ASM") or shutil.which("z80asm")
BUILD = Path(__file__).with_name("build.py")


@unittest.skipUnless(ASSEMBLER, "z80asm 1.8 is required")
class ProbeBuildTests(unittest.TestCase):
    def test_loader_and_reproducibility(self):
        with tempfile.TemporaryDirectory(prefix="m131-probe-") as work:
            outputs = []
            for name in ("first", "second"):
                output = Path(work) / name
                subprocess.run([sys.executable, str(BUILD), "--assembler", ASSEMBLER,
                                "--output-dir", str(output)], check=True, capture_output=True)
                files = {p.name: p.read_bytes() for p in output.iterdir()}
                outputs.append(files)
            self.assertEqual(outputs[0], outputs[1])
            binary = outputs[0]["probe.bin"]
            lines = outputs[0]["m131tv.bas"].decode("ascii").splitlines()
            numbers = [int(line.split()[0]) for line in lines]
            self.assertEqual(numbers, sorted(set(numbers)))
            data = bytes(int(value) for line in lines if line.split()[1] == "DATA"
                         for value in line.split(" DATA ", 1)[1].split(","))
            self.assertEqual(data, binary)
            report = json.loads(outputs[0]["build.json"])
            self.assertEqual(report["checksum"], sum(data))
            self.assertEqual(report["binary_sha256"], hashlib.sha256(data).hexdigest())
            self.assertLess(0xB000 + len(data), 0xBF00)
            self.assertIn("1000 CLEAR ,&HAFFF", lines)
            self.assertIn("1010 SCREEN 1:WIDTH 80,25:CONSOLE 0,25,0,1:COLOR 7,0,0:CLS", lines)
            self.assertIn("1050 A=&HB000:CALL A", lines)
            self.assertEqual(binary[:3], bytes([0xC3, 0x0C, 0xB0]))
            self.assertEqual(binary[3:10], bytes(7))
            self.assertEqual(binary.count(bytes([0xDB, 0x09, 0xE6, 0x40])), 2)
            self.assertIn('1030 IF S<>' + str(sum(data)) +
                          ' THEN PRINT "BAD DATA CHECKSUM":END', lines)
            self.assertTrue(any("Copyright (c) 2026 Nakata Maho" in line for line in lines))
            self.assertTrue(any('THIS SOFTWARE IS PROVIDED' in line for line in lines))


if __name__ == "__main__":
    unittest.main()
