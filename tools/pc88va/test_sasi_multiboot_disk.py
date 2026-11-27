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

"""Synthetic FAT12 and D5 validation contracts; no emulator needed."""

import importlib.util
import struct
import tempfile
import unittest
from pathlib import Path

spec = importlib.util.spec_from_file_location(
    'multiboot', Path(__file__).with_name('build-sasi-multiboot-disk.py'))
m = importlib.util.module_from_spec(spec)
spec.loader.exec_module(m)


def fixture():
    v = m.Volume(5)
    v.image[m.HEADER + 255] = 1
    captures = []
    state = bytearray(4096)
    state[:8] = b'VBM1\0\0\0\0'
    state[118] = 2
    for entry in (1, 2):
        ipl = bytes([entry]) * 1024
        files = [f'entry {entry}, file {index}'.encode() for index in range(4)]
        captures.append((ipl, files))
        state[1024 * entry:1024 * (entry + 1)] = ipl
        base = 16 + (entry - 1) * 34
        struct.pack_into('<16sH', state, base, m.LABELS[entry].encode(), m.crc16(ipl))
        for index, payload in enumerate(files):
            name = f'VA{entry}{index}0000.SYS'
            v.put(name, payload, 0x27)
            pos = v.entries[m.short_name(name)]
            struct.pack_into('<HH', state, base + 18 + 4 * index,
                             (pos - v.root) // 32, m.crc16(v.image[pos + 12:pos + 32]))
    struct.pack_into('<H', state, 254, m.crc16(state[:254]))
    v.put('BOOTMENU.DAT', state, 0x27)
    v.put('BOOTMENU.SYS', b'Synthetic stage', 0x27)
    return v, captures


class MultibootTest(unittest.TestCase):
    def test_crc_known_vector(self):
        self.assertEqual(m.crc16(b'123456789'), 0xbb3d)

    def test_geometry_and_round_trip(self):
        for mb, sectors, clusters in ((5, 22, 616), (10, 36, 1233),
                                      (20, 56, 2468), (40, 105, 2468)):
            with self.subTest(mb=mb):
                v = m.Volume(mb)
                self.assertEqual(v.data, m.HEADER + sectors * 1024)
                self.assertEqual(v.end_cluster - 2, clusters)
                payload = bytes(range(256)) * (v.cluster_size // 256 + 1)
                v.put('FILE.BIN', payload)
                v.put('EMPTY.TXT', b'')
                v = m.Volume(mb, v.image)
                self.assertEqual(v.read('FILE.BIN'), payload)
                self.assertEqual(v.read('EMPTY.TXT'), b'')
                v.check_fat()

    def test_dropped_parked_file(self):
        v, captures = fixture()
        m.validate(v, captures, 0, 0)
        v.image[v.entries[m.short_name('VA100000.SYS')]] = 0xe5
        with self.assertRaises(m.BuildError) as raised:
            m.validate(m.Volume(5, v.image), captures, 0, 0)
        self.assertEqual(raised.exception.code, 'D5_FILE_MISSING')

    def test_corrupted_state(self):
        v, captures = fixture()
        m.validate(v, captures, 0, 0)
        offset = v.entries[m.short_name('BOOTMENU.DAT')]
        cluster, = struct.unpack_from('<H', v.image, offset + 26)
        v.image[v.data + (cluster - 2) * v.cluster_size + 5] ^= 1
        with self.assertRaises(m.BuildError) as raised:
            m.validate(m.Volume(5, v.image), captures, 0, 0)
        self.assertEqual(raised.exception.code, 'D5_STATE_CRC')

    def test_fat_mirror(self):
        v, captures = fixture()
        m.validate(v, captures, 0, 0)
        v.image[v.fat + v.fat_size + 4] ^= 1
        with self.assertRaises(m.BuildError) as raised:
            m.Volume(5, v.image)
        self.assertEqual(raised.exception.code, 'FAT_COPIES')

    def test_publish_does_not_clobber(self):
        with tempfile.TemporaryDirectory() as scratch:
            work = Path(scratch)
            output = work / 'result.hdi'
            m.publish(b'synthetic HDI', output, work)
            self.assertEqual(output.read_bytes(), b'synthetic HDI')
            with self.assertRaises(FileExistsError):
                m.publish(b'must not be published', output, work)
            self.assertEqual(output.read_bytes(), b'synthetic HDI')

    def test_startup_filename_ownership(self):
        self.assertEqual(m.startup_names(0, 0), ('CONFIG.SYS', 'AUTOEXEC.BAT'))
        self.assertEqual(m.startup_names(1, 0), ('VACFG1.SYS', 'VAEXEC1.BAT'))
        self.assertEqual(m.startup_names(0, 2), ('VACFG0.SYS', 'VAEXEC0.BAT'))


if __name__ == '__main__':
    unittest.main()
