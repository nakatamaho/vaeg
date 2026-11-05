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
"""W1INIT COM controls. The fake wake/gating model is NOT a hardware specification."""
import os
from pathlib import Path
import subprocess
import tempfile
import unittest

from test_scan import Model as ScanModel, ROOT, SETTINGS, rows, smoke
from unicorn import UC_HOOK_CODE


class Model(ScanModel):
    def __init__(self, binary, args='', boards=(), *, asleep=True, ack=True,
                 chip=0x98, require_gate=False, require_enable=False,
                 readable_priors=True, fixed=None, failure=None, fail_at=None):
        super().__init__(binary, args, boards, asleep=asleep, fixed=fixed, failure=failure)
        self.ack = ack
        self.chip = chip
        self.require_gate = require_gate
        self.require_enable = require_enable
        self.readable_priors = readable_priors
        self.fail_at = fail_at
        for c in self.chips.values():
            c.update(cr_index=3, control=0x7b, enable=int(not asleep),
                     gate=not asleep, phase=0, enables=0)
        self.opcodes = set()
        self.interrupts = set()
        self.mu.hook_add(UC_HOOK_CODE, self.instruction)

    def instruction(self, mu, address, size, extra):
        instruction = bytes(mu.mem_read(address, size))
        for byte in instruction:
            if byte not in (0x26, 0x2e, 0x36, 0x3e, 0xf2, 0xf3):
                self.opcodes.add(byte)
                break

    def read_port(self, port):
        if port == 0x46e8:
            return self.setup if self.readable_priors and self.chips else 0xff
        for n, c in self.chips.items():
            if port == 0x40e1+2*n:
                return c['control'] if self.ack and (self.readable_priors or c['gate']) else 0xff
            if port == 0x43e0+2*n:
                return c['enable'] if self.readable_priors else 0xff
            if port == 0x4ce0+2*n:
                return c['misc'] if self.readable_priors else 0xff
            if c['awake']:
                if port == 0x54e0+2*n:
                    return c['cr_index']
                if port == 0x55e0+2*n:
                    return self.chip if c['cr_index'] == 0x27 else 0
        return super().read_port(port)

    def write_port(self, port, v):
        if port == 0x46e8:
            self.setup = v
            for c in self.chips.values():
                if v == 0x18:
                    c['phase'], c['enables'] = 1, 0
                elif v == 8 and c['phase'] == 2:
                    c['phase'] = 3
        for n, c in self.chips.items():
            if port == 0x40e1+2*n:
                c['control'] = v
                if v == 0x7a and self.ack:
                    c['gate'] = True
            if port == 0x43e0+2*n:
                c['enable'] = v
                if v == 1 and c['phase'] == 3:
                    c['enables'] += 1
                if v == 0:
                    c['awake'] = False
            if port == 0x42e0+2*n:
                c['misc'] = v
                if v == 1 and c['phase'] == 1:
                    c['phase'] = 2
                if c['phase'] == 3:
                    windows = v == 0xe3 and c['enables'] >= 2
                    dos = v == 0xef and c['gate'] and not self.require_enable
                    if (windows or dos) and (c['gate'] or not self.require_gate):
                        c['awake'] = True
            if c['awake']:
                if port == 0x44e0+2*n:
                    c['index'] = v
                if port == 0x45e0+2*n and c['index'] == 6:
                    c['unlock'] = v == 0x12
                if port == 0x54e0+2*n:
                    c['cr_index'] = v

    def intr(self, mu, intno, extra):
        self.interrupts.add(intno)
        ah = mu.reg_read(smoke.UC_X86_REG_AX) >> 8
        if ah == 0x40 and mu.reg_read(smoke.UC_X86_REG_BX) != 1:
            if self.fail_at == self.writes+1:
                self.writes += 1
                mu.reg_write(smoke.UC_X86_REG_AX, 0)
                self.flags(False)
                return
        super().intr(mu, intno, extra)


def result(model, row):
    return [r[-1] for r in rows(model) if r[6] == row]


class InitTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.tmp = tempfile.TemporaryDirectory()
        cls.binary = Path(cls.tmp.name)/'W1INIT.COM'
        subprocess.run([os.environ.get('NASM', 'nasm'), '-f', 'bin',
                        '-I'+str(ROOT/'demos/wsr-e-pseudo-sprite')+'/', "-DBUILD_ID='12345678'",
                        str(ROOT/'demos/wsr-e-pseudo-sprite/tests/w1init.asm'), '-o', str(cls.binary)], check=True)

    @classmethod
    def tearDownClass(cls):
        cls.tmp.cleanup()

    def run_model(self, *args, **kw):
        m = Model(self.binary, *args, **kw)
        m.run(limit=500000)
        self.assertIsNotNone(m.exit_code, 'WSRE_INIT_TIMEOUT')
        self.assertEqual(m.mask, 0xbf)
        self.assertEqual(m.rom_bank, 0)
        self.assertEqual(bytes(m.mu.mem_read(0x28, 4)), b'\x00\x05\x00\x00')
        self.assertFalse(m.opcodes & {0x0f, 0x64, 0x65, 0x66, 0x67}, 'WSRE_INIT_386_INSTRUCTION')
        self.assertEqual(m.interrupts, {0x21}, 'WSRE_INIT_BIOS_DEPENDENCY')
        if not kw.get('failure') and kw.get('fail_at') is None and m.log:
            self.assertTrue(all(len(r) == 12 for r in rows(m)), 'WSRE_INIT_CSV')
            self.assertEqual(rows(m)[-1][6:8], ['SUM', 'W1INIT'])
        return m

    def test_read_only(self):
        m = self.run_model(boards=(7,))
        self.assertEqual(m.exit_code, 0)
        self.assertEqual(m.io_log, [])
        self.assertEqual(result(m, 'SUM'), ['READ_ONLY'])
        self.assertEqual([r[7] for r in rows(m) if r[6] == 'RAW'], list(map(str, SETTINGS)))

    def test_enable_all_settings_and_exact_sequence(self):
        for n in SETTINGS:
            m = self.run_model(f'/E {n}', boards=(n,))
            self.assertEqual(m.exit_code, 0)
            self.assertEqual(result(m, 'SUM'), [f'CHIP_CANDIDATE {n}'])
            self.assertTrue(result(m, 'CR27_PRE_WAKE')[0].startswith('R=FF '))
            self.assertTrue(result(m, 'CR27_POST_WAKE')[0].startswith('R=98 '))
            self.assertIn(' MATCH ', result(m, 'SR06')[0])
            start = m.io_log.index((0x46e8, 1, 0x18))
            self.assertEqual(m.io_log[start:start+7], [
                (0x46e8,1,0x18),(0x42e0+2*n,1,1),(0x46e8,1,8),
                (0x43e0+2*n,1,1),(0x43e0+2*n,1,1),(0x42e0+2*n,1,0xe3),
                (0x5ae0+2*n,1,0)])
            allowed = {0x46e8} | {p+2*n for p in (0x42e0,0x43e0,0x44e0,0x45e0,0x54e0,0x5ae0)}
            self.assertTrue(all(p in allowed and size == 1 for p,size,_ in m.io_log))
            self.assertEqual(m.chips[n]['misc'], 0x67)
            self.assertEqual(m.chips[n]['enable'], 0)
            self.assertEqual(m.chips[n]['index'], 3)
            self.assertEqual(m.chips[n]['cr_index'], 3)
            self.assertTrue(m.chips[n]['unlock'])

    def test_enable_skips_wake_on_non_ff_cr27(self):
        for chip in (0x98, 0x00):
            m = self.run_model('/E 7', boards=(7,), asleep=False, chip=chip)
            self.assertEqual(result(m, 'WAKE_SKIP'), ['CR27_NOT_FF'])
            self.assertEqual(result(m, 'WAKE'), [])
            self.assertTrue(all(p in (0x54ee,0x44ee,0x45ee) for p,_,_ in m.io_log))
            self.assertEqual(m.exit_code, 0 if chip == 0x98 else 2)

    def test_dos_all_settings_and_exact_sequence(self):
        for n in SETTINGS:
            m = self.run_model(f'/D {n}', boards=(n,), require_gate=True)
            self.assertEqual(m.exit_code, 0)
            self.assertEqual(result(m, 'SUM'), [f'CHIP_CANDIDATE {n}'])
            self.assertEqual(result(m, 'CONTROL_GATE'), [' BEFORE=7B ACK=7A ACCEPT'])
            start = m.io_log.index((0x46e8,1,0x18))
            self.assertEqual(m.io_log[start:start+4], [
                (0x46e8,1,0x18),(0x42e0+2*n,1,1),(0x46e8,1,8),(0x42e0+2*n,1,0xef)])
            self.assertEqual(m.io_log[0], (0x40e1+2*n,1,0x7a))
            allowed = {0x46e8} | {p+2*n for p in (0x40e1,0x42e0,0x44e0,0x45e0,0x54e0)}
            self.assertTrue(all(p in allowed and size == 1 for p,size,_ in m.io_log))
            self.assertEqual(m.chips[n]['control'], 0x7b)
            self.assertEqual(m.chips[n]['misc'], 0x67)
            self.assertTrue(m.chips[n]['unlock'])

    def test_explicit_dos_enumeration(self):
        m = self.run_model('/D ALL', boards=(7,), require_gate=True)
        self.assertEqual(m.exit_code, 0)
        self.assertEqual(result(m, 'SUM'), ['CHIP_CANDIDATE 7'])
        self.assertEqual([r[7] for r in rows(m) if r[6] == 'CONTROL_GATE'], list(map(str, SETTINGS)))
        self.assertEqual([r[7] for r in rows(m) if r[6] == 'CR27_POST_WAKE'], ['7'])
        self.assertFalse(any(p == 0x40e9 for p,_,_ in m.io_log))

    def test_gate_failure_never_wakes(self):
        self.assertEqual(self.run_model('/D 7', boards=(7,)).exit_code, 0)
        m = self.run_model('/D 7', boards=(7,), ack=False)
        self.assertEqual(m.exit_code, 2)
        self.assertEqual(result(m, 'CONTROL_GATE'), [' BEFORE=FF ACK=FF REJECT_NO_WAKE'])
        self.assertEqual(m.io_log, [(0x40ef,1,0x7a)])
        self.assertEqual(result(m, 'SUM'), ['NONE_INCONCLUSIVE'])

    def test_no_device_and_multiple(self):
        control = self.run_model('/D ALL', boards=(7,))
        self.assertEqual(control.exit_code, 0)
        for cmd in ('/E 7', '/D ALL'):
            m = self.run_model(cmd)
            self.assertEqual(m.exit_code, 2)
            self.assertEqual(result(m, 'SUM'), ['NONE_INCONCLUSIVE'])
        m = self.run_model('/D ALL', boards=(0,7))
        self.assertEqual(m.exit_code, 1)
        self.assertEqual(result(m, 'SUM'), ['MULTIPLE'])

    def test_path_requirements_are_not_combined(self):
        self.assertEqual(self.run_model('/E 7', boards=(7,)).exit_code, 0)
        m = self.run_model('/E 7', boards=(7,), require_gate=True)
        self.assertEqual(result(m, 'SUM'), ['NONE_INCONCLUSIVE'])
        self.assertEqual(self.run_model('/D 7', boards=(7,)).exit_code, 0)
        m = self.run_model('/D 7', boards=(7,), require_enable=True)
        self.assertEqual(result(m, 'SUM'), ['NONE_INCONCLUSIVE'])

    def test_candidate_is_cr27_not_sr06(self):
        self.assertEqual(self.run_model('/E 7', boards=(7,)).exit_code, 0)
        m = self.run_model('/E 7', boards=(7,), chip=0xff)
        self.assertIn(' MATCH ', result(m, 'SR06')[0])
        self.assertEqual(m.exit_code, 2)
        self.assertEqual(result(m, 'SUM'), ['NONE_INCONCLUSIVE'])
        m = self.run_model('/E 7', boards=(7,), fixed=0xff)
        self.assertIn(' NO_MATCH ', result(m, 'SR06')[0])
        self.assertEqual(m.exit_code, 0)
        self.assertEqual(result(m, 'SUM'), ['CHIP_CANDIDATE 7'])
        m = self.run_model('/E 7', boards=(7,), chip=0x9b)
        self.assertEqual(result(m, 'SUM'), ['CHIP_CANDIDATE 7'])

    def test_unknown_cleanup_is_not_ff_writeback(self):
        for cmd in ('/E 7','/D 7'):
            m = self.run_model(cmd, boards=(7,), readable_priors=False)
            self.assertEqual(m.exit_code, 0)
            self.assertFalse(any(v == 0xff for _,_,v in m.io_log))
            self.assertIn('UNKNOWN_PRIOR_NOT_RESTORED', m.log.decode())
            self.assertEqual(m.chips[7]['misc'], 0xe3 if cmd.startswith('/E') else 0xef)
        self.assertEqual(m.chips[7]['control'], 0x7a)

    def test_arguments(self):
        for command in ('/E', '/E ALL', '/E 4', '/D 4', '/D 8', '/D A', '/D AL',
                        '/D ALL junk', '/D 7 junk', '/P', '/W 7', '/D'):
            m = self.run_model(command)
            self.assertEqual(m.exit_code, 3)
            self.assertEqual(m.io_log, [])
            self.assertIn(b'WSRE_INIT_USAGE', m.stdout)
        self.assertEqual(self.run_model(' /d all ', boards=(7,)).exit_code, 0)

    def test_target_guard(self):
        self.assertEqual(self.run_model('/E 7', boards=(7,)).exit_code, 0)
        m = Model(self.binary, '/E 7', boards=(7,))
        m.mu.mem_write(0xffffe, b'\xfc\xff')
        m.run(limit=500000)
        self.assertEqual(m.exit_code, 3)
        self.assertEqual(m.io_log, [])
        self.assertIn(b'WSRE_INIT_TARGET', m.stdout)
        for word in (0xffff,0xfffe,0xfffd):
            m = Model(self.binary, '/E 7', boards=(7,))
            m.mu.mem_write(0xffffe, word.to_bytes(2,'little'))
            m.run(limit=500000)
            self.assertEqual(m.exit_code, 0)
        m = Model(self.binary, '/D ALL', boards=(7,))
        original = m.read_port
        m.read_port = lambda port: 1 if port == 0x153 else original(port)
        m.run(limit=500000)
        self.assertEqual(m.exit_code, 3)
        self.assertEqual(m.io_log, [])
        self.assertIn(b'WSRE_INIT_TARGET', m.stdout)

    def test_logger_failure_still_cleans_up(self):
        for cmd in ('/E 7','/D 7'):
            control = self.run_model(cmd, boards=(7,))
            self.assertEqual(control.exit_code, 0)
            for count in range(1,control.writes+1):
                m = self.run_model(cmd, boards=(7,), fail_at=count)
                self.assertEqual(m.exit_code, 3)
                self.assertIn(b'WSRE_INIT_LOG_IO', m.stdout)
                self.assertEqual(m.chips[7]['control'], 0x7b)
                self.assertEqual(m.chips[7]['misc'], 0x67)
                self.assertTrue(m.chips[7]['unlock'])
            for failure in ('denied','close'):
                m = self.run_model(cmd, boards=(7,), failure=failure)
                self.assertEqual(m.exit_code, 3)
                self.assertIn(b'WSRE_INIT_LOG_IO', m.stdout)
                if failure == 'denied':
                    self.assertEqual(m.io_log, [])


if __name__ == '__main__':
    unittest.main()
