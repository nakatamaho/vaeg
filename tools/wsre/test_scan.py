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
"""Unicorn control tests, not hardware/vaeg/timing evidence. Needs NASM and Unicorn."""
import csv
import importlib.util
import io
import os
from pathlib import Path
import subprocess
import tempfile
import unittest
import shutil
import sys

if importlib.util.find_spec('unicorn') is None or shutil.which(os.environ.get('NASM', 'nasm')) is None:
    print('SKIP: W1SCAN controls require NASM and Unicorn')
    sys.exit(77)

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location('wsre_smoke', ROOT/'demos/wsr-e-pseudo-sprite/smoke.py')
smoke = importlib.util.module_from_spec(spec)
spec.loader.exec_module(smoke)
SETTINGS = (0, 1, 2, 3, 5, 6, 7)


class Model(smoke.Dos):
    def __init__(self, binary, args='', boards=(), asleep=False, fixed=None, failure=None):
        super().__init__(binary, False, None, 1000)
        command = args.encode('ascii')
        self.mu.mem_write(smoke.BASE+0x80, bytes([len(command)])+command+b'\r')
        self.chips = {n: {'index': 3, 'unlock': True, 'awake': not asleep, 'misc': 0x67} for n in boards}
        self.armed = set()
        self.fixed = fixed
        self.setup = 8
        self.failure = failure
        self.writes = 0

    def read_port(self, port):
        if port == 0x46e8:
            return self.setup if any(c['awake'] for c in self.chips.values()) else 0xff
        for n, c in self.chips.items():
            if not c['awake']:
                continue
            if port == 0x44e0+2*n:
                return c['index']
            if port == 0x45e0+2*n:
                if self.fixed is not None:
                    return self.fixed
                return (0x12 if c['unlock'] else 0x0f) if c['index'] == 6 else 0x55
            if port == 0x4ce0+2*n:
                return c['misc']
        return super().read_port(port)

    def write_port(self, port, v):
        if port == 0x46e8:
            self.setup = v
            if v == 8:
                for n in self.armed:
                    self.chips[n]['awake'] = True
        for n, c in self.chips.items():
            if port == 0x42e0+2*n:
                c['misc'] = v
                if v == 1 and self.setup == 0x18:
                    self.armed.add(n)
            if c['awake']:
                if port == 0x44e0+2*n:
                    c['index'] = v
                if port == 0x45e0+2*n and c['index'] == 6:
                    c['unlock'] = v == 0x12

    def intr(self, mu, intno, extra):
        ah = mu.reg_read(smoke.UC_X86_REG_AX) >> 8
        if ah == 0x3d and self.failure == 'denied':
            mu.reg_write(smoke.UC_X86_REG_AX, 5)
            self.flags(True)
            return
        if ah == 0x40 and mu.reg_read(smoke.UC_X86_REG_BX) != 1:
            self.writes += 1
            if self.failure == 'short' and self.writes == 5:
                mu.reg_write(smoke.UC_X86_REG_AX, 0)
                self.flags(False)
                return
        super().intr(mu, intno, extra)
        if ah == 0x3e and self.failure == 'close':
            self.flags(True)


def rows(model):
    lines = model.log.decode('ascii').splitlines()
    return list(csv.reader(line for line in lines[1:] if not line.startswith('#')))


class ScanTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.tmp = tempfile.TemporaryDirectory()
        cls.binary = Path(cls.tmp.name)/'W1SCAN.COM'
        subprocess.run([os.environ.get('NASM', 'nasm'), '-f', 'bin',
                        '-I'+str(ROOT/'demos/wsr-e-pseudo-sprite')+'/', "-DBUILD_ID='12345678'",
                        str(ROOT/'demos/wsr-e-pseudo-sprite/tests/w1scan.asm'), '-o', str(cls.binary)], check=True)

    @classmethod
    def tearDownClass(cls):
        cls.tmp.cleanup()

    def run_model(self, *args, **kw):
        m = Model(self.binary, *args, **kw)
        m.run(limit=500000)
        self.assertIsNotNone(m.exit_code, 'WSRE_SCAN_TIMEOUT')
        self.assertEqual(m.mask, 0xbf)
        self.assertEqual(m.rom_bank, 0)
        self.assertEqual(bytes(m.mu.mem_read(0x28, 4)), b'\x00\x05\x00\x00')
        return m

    def test_read_only(self):
        m = self.run_model(boards=(7,))
        self.assertEqual(m.exit_code, 0)
        self.assertEqual(m.io_log, [])
        r = rows(m)
        self.assertTrue(all(len(x) == 12 for x in r))
        self.assertEqual([x[7] for x in r if x[6] == 'RAW'], list(map(str, SETTINGS)))
        self.assertEqual(r[-1][-1], 'READ_ONLY')

    def test_all_seven_settings(self):
        for setting in SETTINGS:
            with self.subTest(setting=setting):
                m = self.run_model('/P', boards=(setting,))
                self.assertEqual(m.exit_code, 0)
                self.assertEqual(rows(m)[-1][-1], f'MATCH {setting}')
                signatures = [x for x in rows(m) if x[6] == 'SR06']
                self.assertEqual(len(signatures), 7)
                self.assertEqual(sum(x[-1].endswith(' MATCH') for x in signatures), 1)
                self.assertEqual(m.chips[setting]['index'], 3)
                self.assertTrue(m.chips[setting]['unlock'])
                allowed = {p+2*n for n in SETTINGS for p in (0x44e0, 0x45e0)}
                self.assertTrue(all(p in allowed and size == 1 for p, size, _ in m.io_log))

    def test_no_board_and_constant_responses(self):
        control = self.run_model('/P', boards=(7,))
        self.assertEqual(rows(control)[-1][-1], 'MATCH 7')
        for fixed in (0xff, 0x12, 0x0f):
            m = self.run_model('/P', boards=(7,), fixed=fixed)
            self.assertEqual(m.exit_code, 2)
            self.assertEqual(rows(m)[-1][-1], 'NONE_INCONCLUSIVE')
        m = self.run_model('/P')
        self.assertEqual(m.exit_code, 2)
        self.assertEqual(rows(m)[-1][-1], 'NONE_INCONCLUSIVE')

    def test_multiple(self):
        control = self.run_model('/P', boards=(0,))
        self.assertEqual(rows(control)[-1][-1], 'MATCH 0')
        m = self.run_model('/P', boards=(0, 7))
        self.assertEqual(m.exit_code, 1)
        self.assertEqual(rows(m)[-1][-1], 'MULTIPLE')

    def test_wake_is_single_candidate(self):
        for n in SETTINGS:
            m = self.run_model(f'/W {n}', boards=(n,), asleep=True)
            self.assertEqual(m.exit_code, 0)
            self.assertEqual(rows(m)[-1][-1], f'MATCH {n}')
            self.assertEqual([x[7] for x in rows(m) if x[6] == 'SR06'], [str(n)])
            allowed = {0x46e8, 0x42e0+2*n, 0x44e0+2*n, 0x45e0+2*n, 0x5ae0+2*n}
            self.assertTrue(all(p in allowed and size == 1 for p, size, _ in m.io_log))
            self.assertEqual(m.chips[n]['index'], 3)
            self.assertTrue(m.chips[n]['unlock'])
            self.assertIn('FF_PRIOR_NOT_RESTORED', m.log.decode())
        m = self.run_model('/W 7', boards=(7,))
        self.assertEqual(m.setup, 8)
        self.assertEqual(m.chips[7]['misc'], 0x67)

    def test_wrong_wake_candidate(self):
        control = self.run_model('/W 7', boards=(7,), asleep=True)
        self.assertEqual(rows(control)[-1][-1], 'MATCH 7')
        m = self.run_model('/W 0', boards=(7,), asleep=True)
        self.assertEqual(m.exit_code, 2)
        self.assertEqual(rows(m)[-1][-1], 'NONE_INCONCLUSIVE')
        self.assertFalse(m.chips[7]['awake'])

    def test_arguments(self):
        for command in ('/W 4', '/W 8', '/W', '/Q', '/P junk', '/W 7 junk'):
            m = self.run_model(command)
            self.assertEqual(m.exit_code, 3)
            self.assertEqual(m.io_log, [])
            self.assertIn(b'WSRE_SCAN_USAGE', m.stdout)
        m = self.run_model(' /p ')
        self.assertEqual(m.exit_code, 2)

    def test_target_guard(self):
        control = self.run_model('/P', boards=(7,))
        self.assertEqual(control.exit_code, 0)
        m = Model(self.binary, '/P', boards=(7,))
        m.mu.mem_write(0xffffe, b'\xfc\xff')
        m.run(limit=500000)
        self.assertEqual(m.exit_code, 3)
        self.assertEqual(m.io_log, [])
        self.assertIn(b'WSRE_SCAN_TARGET', m.stdout)
        m = Model(self.binary, '/P', boards=(7,))
        original = m.read_port
        m.read_port = lambda port: 1 if port == 0x153 else original(port)
        m.run(limit=500000)
        self.assertEqual(m.exit_code, 3)
        self.assertEqual(m.io_log, [])
        self.assertIn(b'WSRE_SCAN_TARGET', m.stdout)

    def test_log_failures(self):
        control = self.run_model('/P', boards=(7,))
        self.assertEqual(control.exit_code, 0)
        for failure in ('denied', 'short', 'close'):
            m = self.run_model('/P', boards=(7,), failure=failure)
            self.assertEqual(m.exit_code, 3)
            self.assertIn(b'WSRE_SCAN_LOG_IO', m.stdout)
            self.assertTrue(m.chips[7]['unlock'])
            if failure == 'denied':
                self.assertEqual(m.io_log, [])
                self.assertFalse(m.log_exists)


if __name__ == '__main__':
    unittest.main()
