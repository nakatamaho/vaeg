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
import subprocess
import sys
import tempfile
from pathlib import Path
import unittest
import compare_logs as c


def fixture():
    rows = [c.HEADER, '# W1PORT VA2/3 FFFE 12345678 15kHz']
    rows += [f'W1,12345678,VA2/3,15,-,--,R0,LOOP1024,{i},4000,600,' for i in (1, 2, 3)]
    rows += ['W1,12345678,VA2/3,15,-,--,P,00E0,,,,FF',
             'W1,12345678,VA2/3,15,-,--,SUM,W1PORT,,,,PASS']
    return ('\r\n'.join(rows) + '\r\n').encode('ascii')


class CompareTests(unittest.TestCase):
    def setUp(self):
        self.data = fixture()
        self.control = c.parse_log(self.data)
        self.assertEqual(c.compare_logs(self.control, self.control)[1], [])

    def test_undriven_class_and_build(self):
        other = self.data.replace(b',,,,FF', b',,,,FLOAT A=55B3 B=AAB1 C=33B4')
        self.assertEqual(c.compare_logs(self.control, c.parse_log(other))[1], [])
        other = self.data.replace(b'12345678', b'abcdef01')
        self.assertEqual(c.compare_logs(self.control, c.parse_log(other))[1], [])

    def test_single_mutation_content_codes(self):
        cases = [
            (self.data.replace(b'stage,build', b'stage,wrong', 1), 'WSRE_LOG_HEADER'),
            (self.data.replace(b',1,4000,600,', b',1,0,600,'), 'WSRE_LOG_TIMING'),
            (self.data.replace(b',15,-,--,', b',16,-,--,', 1), 'WSRE_LOG_METADATA'),
            (self.data.replace(b',,,,FF', b',,,,FLOAT malformed'), 'WSRE_LOG_RESULT'),
            (self.data + self.data.splitlines()[-1] + b'\r\n', 'WSRE_LOG_DUPLICATE'),
            (self.data.replace(b',SUM,W1PORT,', b',OTHER,W1PORT,'), 'WSRE_LOG_SUMMARY'),
        ]
        for mutated, code in cases:
            with self.subTest(code=code):
                with self.assertRaises(c.LogError) as error:
                    c.parse_log(mutated)
                self.assertEqual(str(error.exception), code)

    def test_single_mutation_comparison_codes(self):
        for before, after, code in [
            (b',,,,FF', b',,,,DRIVEN 00', 'WSRE_LOG_CHECK_MISMATCH'),
            (b',P,00E0,', b',P,00E1,', 'WSRE_LOG_ROW_SET'),
            (b',15,-,--,', b',24,-,--,', 'WSRE_LOG_METADATA_MISMATCH'),
        ]:
            with self.subTest(code=code):
                other = c.parse_log(self.data.replace(before, after))
                if code == 'WSRE_LOG_ROW_SET':
                    with self.assertRaises(c.LogError) as error:
                        c.compare_logs(self.control, other)
                    self.assertEqual(str(error.exception), code)
                else:
                    self.assertEqual(c.compare_logs(self.control, other)[1], [code])

    def test_timing_ratio_and_explicit_fixture_mode(self):
        other = c.parse_log(self.data.replace(b',4000,600,', b',8000,600,'))
        self.assertEqual(c.compare_logs(self.control, other)[1], ['WSRE_LOG_TIMING_RATIO'])
        reports, errors = c.compare_logs(self.control, other, True)
        self.assertEqual(errors, [])
        self.assertIn('IGNORED_NOT_TIMING_PASS', '\n'.join(reports))

    def test_timing_variation(self):
        other = c.parse_log(self.data.replace(b',2,4000,', b',2,4080,'))
        self.assertEqual(c.compare_logs(self.control, other)[1], ['WSRE_LOG_TIMING_VARIATION'])

    def test_cli_integration(self):
        with tempfile.TemporaryDirectory() as tmp:
            left, right = Path(tmp)/'left.txt', Path(tmp)/'right.txt'
            left.write_bytes(self.data)
            right.write_bytes(self.data)
            command = [sys.executable, str(Path(c.__file__)), str(left), str(right)]
            self.assertEqual(subprocess.run(command, capture_output=True).returncode, 0)
            right.write_bytes(self.data.replace(b',,,,FF', b',,,,DRIVEN 00'))
            result = subprocess.run(command, capture_output=True, text=True)
            self.assertEqual(result.returncode, 1)
            self.assertEqual(result.stderr.strip(), 'WSRE_LOG_CHECK_MISMATCH')


if __name__ == '__main__':
    unittest.main()
