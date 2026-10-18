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
# EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
# OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
# OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
# OF THE POSSIBILITY OF SUCH DAMAGE.
"""Consistency tests for the SGPTEST guest program (M109a).

The expected-value include must define exactly T01-T20, the program must
have twenty CRC tests and compare them in order, and both assembly forms
must build with the expected values embedded in test order.
"""

import re
import shutil
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
SGPTEST = REPO / "tools/pc88va/sgptest"
TESTS = 20


def expected_values(text):
    return re.findall(r"^%define EXP_T(\d\d) 0x([0-9A-F]{4})$", text, re.M)


class SgptestConsistency(unittest.TestCase):
    def test_expected_include_defines_each_test_once(self):
        text = (SGPTEST / "sgptest-expected.inc").read_text()
        numbers = [int(n) for n, _ in expected_values(text)]
        self.assertEqual(numbers, list(range(1, TESTS + 1)))
        self.assertEqual(len(re.findall(r"^%define", text, re.M)), TESTS)

    def test_program_has_twenty_crc_tests_in_order(self):
        source = (SGPTEST / "sgptest.asm").read_text()
        table = re.findall(r"^    TEST C_", source, re.M)
        # the table tests, then LINE (T19) and CLS (T20)
        self.assertEqual(len(table) + 2, TESTS)
        self.assertEqual(re.findall(r"^    EXP T(\d\d)$", source, re.M),
                         [f"{n:02d}" for n in range(1, TESTS + 1)])
        script = (SGPTEST / "update-expected.py").read_text()
        self.assertIn(f"TESTS = {TESTS}\n", script)

    @unittest.skipUnless(shutil.which("nasm"), "nasm is not installed")
    def test_assembles_with_expected_values_embedded(self):
        values = [int(v, 16) for _, v in
                  expected_values((SGPTEST / "sgptest-expected.inc").read_text())]
        table = b"".join(v.to_bytes(2, "little") for v in values)
        with tempfile.TemporaryDirectory() as tmp:
            plain = Path(tmp) / "plain.com"
            full = Path(tmp) / "full.com"
            subprocess.run(["nasm", "-f", "bin", "-DEXPECTED_NONE", "-o", str(plain),
                            str(SGPTEST / "sgptest.asm")], check=True)
            subprocess.run(["nasm", "-f", "bin", f"-I{SGPTEST}/", "-o", str(full),
                            str(SGPTEST / "sgptest.asm")], check=True)
            self.assertIn(table, full.read_bytes())
            self.assertNotIn(table, plain.read_bytes())
            self.assertEqual(len(plain.read_bytes()), len(full.read_bytes()))


if __name__ == "__main__":
    sys.exit(unittest.main())
