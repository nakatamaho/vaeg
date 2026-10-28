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
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
"""Seed content negatives and optional assembled mapped-host RAM capture."""
import argparse
from pathlib import Path
import struct
import subprocess
import tempfile
import unittest
import build
import results
import seed


def fixture(changed_row=None):
    text = bytearray(b'M140 FDC SEED 1\r\n')
    raw = bytearray(seed.MAGIC)
    for i, (s, op, af) in enumerate(seed.ROWS):
        after = results.zilog_af(af >> 8, af & 255)[3 if op == 'S' else 4]
        if i == changed_row:after ^= 8  # One captured X bit, encoded on both paths.
        text += f'B {s:02X} {op} {af:04X} {af:04X} {after:04X}\r\n'.encode()
        raw += struct.pack('<BBHHH', s, ord(op), af, af, after)
    return bytes(text) + seed.MARKER + raw + b'END!' + seed.RESTORED + b'\x1a'


class SeedContent(unittest.TestCase):
    def test_complete(self):
        report = seed.content(fixture(), host_model=True)
        self.assertEqual(report['records'], 80)
        self.assertEqual(report['seed_dependent_groups'], [])
        self.assertTrue(report['text_raw_agreement'])

    def mutate_raw(self, offset, code):
        good = fixture()
        seed.content(good, host_model=True)
        changed = bytearray(good)
        start = good.index(seed.MARKER) + len(seed.MARKER)
        changed[start + offset] ^= 1
        with self.assertRaises(results.RecordError) as caught:
            seed.content(bytes(changed))
        self.assertEqual(caught.exception.code, code)

    def test_frame(self):
        self.mutate_raw(0, 'M140_SEED_BINARY_FRAME')

    def test_descriptor(self):
        self.mutate_raw(8, 'M140_SEED_DESCRIPTOR')

    def test_precondition(self):
        self.mutate_raw(12, 'M140_SEED_PRECONDITION')

    def test_documented_flags(self):
        self.mutate_raw(14, 'M140_SEED_DOCUMENTED_FLAGS')

    def test_independent_path_mismatch(self):
        self.mutate_raw(14 + 1, 'M140_SEED_DOCUMENTED_FLAGS')
        good = fixture()
        seed.content(good)
        changed = good.replace(b'B 00 S 0008 0008 0001', b'B 00 S 0008 0008 0009', 1)
        self.assertNotEqual(changed, good)
        with self.assertRaises(results.RecordError) as caught:
            seed.content(changed)
        self.assertEqual(caught.exception.code, 'M140_SEED_PATH_MISMATCH')

    def test_recovery(self):
        good = fixture()
        seed.content(good)
        with self.assertRaises(results.RecordError) as caught:
            seed.content(good.replace(seed.RESTORED, b'UNKNOWN'))
        self.assertEqual(caught.exception.code, 'M139_RECOVERY_RECORD')

    def test_padding(self):
        good = fixture()
        seed.content(good)
        with self.assertRaises(results.RecordError) as caught:
            seed.content(good[:-1] + b'!')
        self.assertEqual(caught.exception.code, 'M140_SEED_PADDING')

    def test_physical_xy_observation_not_host_pass(self):
        seed.content(fixture(), host_model=True)
        changed = fixture(changed_row=0)
        report = seed.content(changed)
        self.assertEqual(len(report['host_baseline_differences']), 1)
        self.assertEqual(report['seed_dependent_groups'], [{'af': '0008', 'operation': 'S'}])
        with self.assertRaises(results.RecordError) as caught:
            seed.content(changed, host_model=True)
        self.assertEqual(caught.exception.code, 'M140_SEED_HOST_MISMATCH')

    def test_dat_directory_contract(self):
        rows = ['A>DIR', 'FDCSEED DAT', 'A>']
        results.terminal(rows, 'FDCSEED', extension='DAT')
        with self.assertRaises(results.RecordError) as caught:
            results.terminal(['A>DIR', 'FDCSEED TXT', 'A>'], 'FDCSEED', extension='DAT')
        self.assertEqual(caught.exception.code, 'M139_INTEGRATION_DIR_RESULT')

    def test_text_set(self):
        good = fixture()
        seed.content(good)
        changed = good.replace(good.splitlines(keepends=True)[1], b'', 1)
        with self.assertRaises(results.RecordError) as caught:
            seed.content(changed)
        self.assertEqual(caught.exception.code, 'M140_SEED_TEXT_SET')

    def test_loader_recovery_order_and_capacity(self):
        text = seed.loader_source(b'\xc9')
        read = text.index('seed_read_loop:')
        restore = text.index('restore_loop:')
        verify = text.index('verify_loop:')
        save = text.index('    ld c,22\n')
        self.assertLess(read, restore)
        self.assertLess(restore, verify)
        self.assertLess(verify, save)
        self.assertIn('    jp nc,log_overflow\n', text[text.index('restore:'):read])
        self.assertIn('    ld hl,7000h\n', text[text.index('restore:'):read])
        self.assertIn('    jp nz,fatal\n', text[verify:save])

    def test_source_adjacency(self):
        source = seed.source()
        for i, (s, op, af) in enumerate(seed.ROWS):
            for suffix in ('pre', 'post'):
                span = source.split(f'case_{i}_{suffix}:\n', 1)[1]
                instructions = ['di', seed.RECIPES[s], 'push bc', 'pop af']
                if suffix == 'post':instructions.append('scf' if op == 'S' else 'ccf')
                instructions += ['push af', 'pop de']
                self.assertTrue(span.startswith('\n'.join(instructions)))


def payloads(runner, assembler):
    with tempfile.TemporaryDirectory(prefix='m140-seed-') as td:
        work = Path(td)
        seed.assemble(assembler, work)
        binary = (work/'fdcseed.bin').read_bytes()
        _, symbols = build.zex.assemble_with_symbols(seed.source(), assembler, 'seed-adjacency')
        recipes = (b'\xaf', b'\x3e\x00\xb7', b'\x3e\x08\xb7',
                   b'\x3e\x20\xb7', b'\x3e\x28\xb7')
        for i, (s, op, af) in enumerate(seed.ROWS):
            for suffix in ('pre', 'post'):
                expected = b'\xf3' + recipes[s] + b'\xc5\xf1'
                if suffix == 'post':expected += b'\x37' if op == 'S' else b'\x3f'
                expected += b'\xf5\xd1'
                offset = symbols[f'case_{i}_{suffix}'] - 0x4000
                assert binary[offset:offset+len(expected)] == expected
        raw = work/'raw.bin'
        run = subprocess.run([runner, str(work/'fdcseed.bin'), str(raw)], capture_output=True, timeout=30)
        assert run.returncode == 0, run.stderr
        snapshot = raw.read_bytes()
        assert len(snapshot) == 0x3f00
        seed.content(run.stdout + snapshot[0x3000:0x3000+seed.RAW_SIZE], recovery=False, host_model=True)
    print('M140_SEED_PAYLOAD_PASS (mapped software RAM, not silicon)')


if __name__ == '__main__':
    p = argparse.ArgumentParser()
    p.add_argument('--runner')
    p.add_argument('--assembler')
    args, remaining = p.parse_known_args()
    if args.runner:payloads(args.runner, args.assembler)
    else:unittest.main(argv=['test_seed', *remaining])
