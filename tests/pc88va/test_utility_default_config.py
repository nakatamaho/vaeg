#!/usr/bin/env python3
"""
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
"""

import importlib.util
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[2]
SPEC = importlib.util.spec_from_file_location(
    'sasi_build', ROOT / 'tools/pc88va/build-sasi-utility-disk.py')
sasi = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(sasi)


class Tests(unittest.TestCase):
    def assert_safe(self, lines):
        self.assertTrue(lines)
        self.assertFalse(any('RDBMS.SYS' in line.upper() for line in lines))
        self.assertIn(r'DEVICE = A:\SYS\BMSDRVA.SYS /P', lines)
        self.assertIn(r'DEVICE = A:\SYS\MSE352B.COM /A /B', lines)
        self.assertIn(r'DEVICE = A:\SYS\RDEMS.SYS -P128 -A', lines)

    def test_sasi_profiles(self):
        for profile in sasi.SCSI_PROFILES:
            for two_hc in (False, True):
                with self.subTest(profile=profile, two_hc=two_hc):
                    self.assert_safe(sasi.config_lines(two_hc, 0, profile))

    def test_fdd_policy_and_retained_payload(self):
        text = (ROOT / 'tools/pc88va/build-utility-disk.sh').read_text()
        block = text.rsplit("printf '%s\\r\\n' \\", 2)[-2]
        lines = re.findall(r"^\s*'([^']*)'", block, re.M)
        self.assert_safe(lines)
        self.assertIn('copy_payload "$work_dir/rdbms/RDBMS.SYS" sys/RDBMS.SYS', text)
        self.assertIn('copy_payload "$work_dir/rdbms/RDBMS.DOC" doc/RDBMS.DOC', text)


if __name__ == '__main__':
    unittest.main()
