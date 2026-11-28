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
import os
import struct
import tempfile
import unittest
from datetime import datetime
from pathlib import Path
from types import SimpleNamespace
from unittest.mock import Mock, patch

spec = importlib.util.spec_from_file_location(
    'multiboot', Path(__file__).with_name('build-sasi-multiboot-disk.py'))
m = importlib.util.module_from_spec(spec)
spec.loader.exec_module(m)


STAMP = (0, 0x10d1)  # 1988-06-17, midnight


def fixture():
    v = m.Volume(5)
    v.image[m.HEADER + 255] = 1
    captures = []
    state = bytearray(4096)
    state[:8] = b'VBM1\0\0\0\0'
    state[118] = 3
    for entry in (1, 2):
        ipl = bytes([entry]) * 1024
        files = [f'entry {entry}, file {index}'.encode() for index in range(4)]
        captures.append((ipl, files))
        state[1024 * entry:1024 * (entry + 1)] = ipl
        base = 16 + (entry - 1) * 34
        struct.pack_into('<H', state, base + 16, m.crc16(ipl))
        struct.pack_into('<24s', state, 138 + (entry - 1) * 24, m.LABELS[entry].encode())
        for index, payload in enumerate(files):
            name = f'VA{entry}{index}0000.SYS'
            v.put(name, payload, 0x27, stamp=STAMP)
            pos = v.entries[m.short_name(name)]
            struct.pack_into('<HH', state, base + 18 + 4 * index,
                             (pos - v.root) // 32, m.crc16(v.image[pos + 12:pos + 32]))
    struct.pack_into('<H', state, 254, m.crc16(state[:254]))
    v.put('BOOTMENU.DAT', state, 0x27, stamp=STAMP)
    v.put('BOOTMENU.SYS', b'Synthetic stage', 0x27, stamp=STAMP)
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
                v.put('FILE.BIN', payload, stamp=STAMP)
                v.put('EMPTY.TXT', b'', stamp=STAMP)
                v = m.Volume(mb, v.image)
                self.assertEqual(v.read('FILE.BIN'), payload)
                self.assertEqual(v.read('EMPTY.TXT'), b'')
                v.check_fat()

    def test_startup_insertion_preserves_root_end_marker(self):
        v, captures = fixture()
        m.validate(m.Volume(5, v.image), captures, 0, 0)
        end = v.slots[0]
        stale = v.entries[m.short_name('BOOTMENU.SYS')]
        # A stale duplicate beyond the end marker is invisible to FAT readers.
        v.image[end + 32:end + 64] = v.image[stale:stale + 32]
        v = m.Volume(5, v.image)
        m.validate(v, captures, 0, 0)

        v.put('AUTOEXEC.BAT', b'@ECHO OFF\r\n', stamp=STAMP)
        reopened = m.Volume(5, bytes(v.image))
        m.validate(reopened, captures, 0, 0)
        self.assertEqual(reopened.read('AUTOEXEC.BAT'), b'@ECHO OFF\r\n')
        self.assertEqual(reopened.image[end + 32], 0)

    def test_floppy_directory_timestamp(self):
        root = bytearray(64)
        root[:11] = m.short_name('SYS.COM')
        stamp = (0x645a, STAMP[1])
        struct.pack_into('<HHHI', root, 22, *stamp, 2, 3)
        disk = Mock(root=root)
        disk.is_msdos.return_value = True
        disk.cluster_chain.return_value = [2]
        disk.read_cluster.return_value = b'abc\0'
        with tempfile.TemporaryDirectory() as scratch, patch.object(m, 'PcEngineDisk', return_value=disk):
            work = Path(scratch)
            source = work / 'source.d88'
            source.write_bytes(bytes(0x2b0))
            (file,) = m.copy_floppy(work, source, 'scratch.d88', ('SYS.COM',), m.LABELS[0])
        self.assertEqual(file, (b'abc', stamp))
        v, captures = fixture()
        v.put('SYS.COM', file.payload, stamp=file.stamp)
        m.validate(m.Volume(5, bytes(v.image)), captures, 0, 0, host_stamps={'SYS.COM': stamp})

    def test_startup_timestamps(self):
        v, captures = fixture()
        supplied_time = datetime(2000, 2, 29, 23, 58, 59)
        supplied_stamp = (0xbf5d, 0x285d)
        with tempfile.TemporaryDirectory() as scratch:
            path = Path(scratch) / 'startup.txt'
            path.write_bytes(b'REM caller supplied\r\n')
            epoch = supplied_time.timestamp()
            os.utime(path, (epoch, epoch))
            args = SimpleNamespace(msdos_config=None, msdos_autoexec=None,
                                   pce105_config=path, pce105_autoexec=None,
                                   pce11_config=None, pce11_autoexec=None)
            stamps = m.install_startup(v, args, 0, STAMP)
        self.assertEqual(stamps, {'AUTOEXEC.BAT': STAMP, 'VACFG1.SYS': supplied_stamp})
        for name, stamp in stamps.items():
            offset = v.entries[m.short_name(name)]
            self.assertEqual(struct.unpack_from('<HH', v.image, offset + 22), stamp)
        m.validate(m.Volume(5, bytes(v.image)), captures, 0, 0, host_stamps=stamps)

    def test_zero_builder_date(self):
        v, captures = fixture()
        v.put('AUTOEXEC.BAT', b'@ECHO OFF\r\n', stamp=STAMP)
        stamps = {'AUTOEXEC.BAT': STAMP}
        m.validate(v, captures, 0, 0, host_stamps=stamps)
        offset = v.entries[m.short_name('AUTOEXEC.BAT')]
        struct.pack_into('<H', v.image, offset + 24, 0)
        with self.assertRaises(m.BuildError) as raised:
            m.validate(m.Volume(5, bytes(v.image)), captures, 0, 0, host_stamps=stamps)
        self.assertEqual(raised.exception.code, 'FAT_TIMESTAMP')

    def test_prepare_requires_empty_reserved_placeholders(self):
        v, captures = fixture()
        at = v.entries[m.short_name('BOOTMENU.DAT')]
        cluster, = struct.unpack_from('<H', v.image, at + 26)
        at = v.data + (cluster - 2) * v.cluster_size
        v.image[at + 5:at + 7] = bytes((1, 2))  # Last choice 1, prepared phase.
        for index, (name, attr) in enumerate(zip(m.ENGINE, (0x27, 0x27, 0x27, 0x21))):
            v.put(name, b'', attr, stamp=STAMP)
            offset = v.entries[m.short_name(name)]
            struct.pack_into('<H', v.image, at + 8 + index * 2, (offset - v.root) // 32)
        struct.pack_into('<H', v.image, at + 254, m.crc16(v.image[at:at + 254]))
        m.validate(v, captures[:1], 0, 1, prepared=True)

        offset = v.entries[m.short_name(m.ENGINE[-1])]
        v.image[offset + 28] = 1  # The placeholder is no longer empty.
        with self.assertRaises(m.BuildError) as raised:
            m.validate(v, captures[:1], 0, 1, prepared=True)
        self.assertEqual(raised.exception.code, 'D5_PREPARE_SLOT')

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

    def test_pre_d51_state_version_rejected(self):
        v, captures = fixture()
        m.validate(v, captures, 0, 0)
        offset = v.entries[m.short_name('BOOTMENU.DAT')]
        cluster, = struct.unpack_from('<H', v.image, offset + 26)
        base = v.data + (cluster - 2) * v.cluster_size
        v.image[base + 118] = 2
        struct.pack_into('<H', v.image, base + 254, m.crc16(v.image[base:base + 254]))
        with self.assertRaises(m.BuildError) as raised:
            m.validate(v, captures, 0, 0)
        self.assertEqual(raised.exception.code, 'D5_LABEL_FORMAT')
        self.assertIn(m.DOS_REQUIREMENT, str(raised.exception))

    def test_truncated_label_rejected(self):
        v, captures = fixture()
        m.validate(v, captures, 0, 0)
        offset = v.entries[m.short_name('BOOTMENU.DAT')]
        cluster, = struct.unpack_from('<H', v.image, offset + 26)
        base = v.data + (cluster - 2) * v.cluster_size
        v.image[base + 138 + 15] = 0
        struct.pack_into('<H', v.image, base + 254, m.crc16(v.image[base:base + 254]))
        with self.assertRaises(m.BuildError) as raised:
            m.validate(v, captures, 0, 0)
        self.assertEqual(raised.exception.code, 'D5_LABEL_REJECTED')

    def test_old_sys_label_prompt_rejected(self):
        text = 'Label (up to 23 ASCII characters, Enter for PC-Engine n):\nSystem transferred'
        m.expect_capture(text, m.LABELS[1])
        with self.assertRaises(m.BuildError) as raised:
            m.expect_capture(text.replace('23', '15'), m.LABELS[1])
        self.assertEqual(raised.exception.code, 'D5_LABEL_REJECTED')

    def test_fat_mirror(self):
        v, captures = fixture()
        m.validate(v, captures, 0, 0)
        v.image[v.fat + v.fat_size + 4] ^= 1
        with self.assertRaises(m.BuildError) as raised:
            m.Volume(5, v.image)
        self.assertEqual(raised.exception.code, 'FAT_COPIES')

    def test_chkdsk_checks_counts_without_localized_captions(self):
        v = m.Volume(5)
        v.put('HIDDEN.SYS', b'h', 0x22, stamp=STAMP)
        v.put('USER.TXT', b'u', stamp=STAMP)
        text = ('C>CHKDSK C:\n\n5046272\n8192 (1)\n8192 (1)\n5029888\n\n'
                '8192\n616\n614\n\n655360\n522656\n\nC>')
        m.check_chkdsk(text, v)
        with self.assertRaises(m.BuildError) as raised:
            m.check_chkdsk(text.replace('\n614\n', '\n613\n'), v)
        self.assertEqual(raised.exception.code, 'GUEST_CHKDSK')

    def test_timeout_rejects_dos_fallback_without_menu(self):
        with tempfile.TemporaryDirectory() as scratch:
            work = Path(scratch)
            guest = SimpleNamespace(work=work, disk=work / 'disk.hdi',
                                    args=SimpleNamespace(model='va2'), run=Mock())
            guest.disk.write_bytes(b'synthetic HDI')
            boot = 'MS-DOS version 4.00'
            menu = '\n'.join((m.MODELS['va2'], '1: PC-Engine 1.05 (VA)', '2: PC-Engine 1.1 (VA2/3)',
                              '3: MS-DOS 4.00', 'Enter = 3'))
            early_screen = menu

            def run(*args, **kwargs):
                (work / 'timeout-checkpoint.tvram.bin').write_bytes(early_screen.encode())
                return boot

            guest.run.side_effect = run
            with patch.object(m, 'screen_text', bytes.decode):
                m.check_timeout(guest, 0)
                guest.run.assert_called_once_with('timeout', frames=1900, checkpoint=900)
                guest.run.reset_mock()
                early_screen = boot  # The final screen and HDI remain unchanged.
                with self.assertRaises(m.BuildError) as raised:
                    m.check_timeout(guest, 0)
                self.assertEqual(raised.exception.code, 'GUEST_TIMEOUT_MENU')
                guest.run.assert_called_once_with('timeout', frames=1900, checkpoint=900)

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
