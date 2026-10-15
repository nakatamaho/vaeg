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

"""Tests for pcengine_disk.py on a PC-88VA MS-DOS system disk.

The disk is synthetic: an MS-DOS boot sector parameter block, a volume
label, the three system files with IO.SYS at cluster 2, the start-up files
and one utility, all with placeholder contents.
"""

import importlib.util
from pathlib import Path
import struct
import tempfile
import unittest

TOOL = Path(__file__).resolve().parents[2] / "tools" / "pc88va" / "pcengine_disk.py"
SPEC = importlib.util.spec_from_file_location("pcengine_disk_under_test", TOOL)
disk_tool = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(disk_tool)

SECTOR = 1024
FILES = (  # name, attributes, first cluster, clusters
    ("IO.SYS", 0x07, 2, 2),
    ("MSDOS.SYS", 0x07, 4, 3),
    ("COMMAND.COM", 0x20, 7, 2),
    ("UTIL.COM", 0x20, 9, 2),
    ("CONFIG.SYS", 0x20, 11, 1),
    ("LICENSE.TXT", 0x20, 12, 1),
)


def make_logical(io_cluster=2):
    image = bytearray(160 * 8 * SECTOR)
    image[0:3] = b"\xeb\x3c\x90"
    image[3:11] = b"MSDOS-VA"
    image[11:11 + len(disk_tool.MSDOS_BPB)] = disk_tool.MSDOS_BPB
    fat = bytearray(2 * SECTOR)
    fat[:3] = b"\xfe\xff\xff"

    def fat_set(cluster, value):
        offset = cluster * 3 // 2
        word = struct.unpack_from("<H", fat, offset)[0]
        if cluster & 1:
            word = (word & 0x000F) | (value << 4)
        else:
            word = (word & 0xF000) | value
        struct.pack_into("<H", fat, offset, word)

    root = bytearray(6 * SECTOR)
    root[0:32] = disk_tool.make_entry(b"MSDOS-VA   ", 0x08, 0, 0)
    for index, (name, attributes, first, count) in enumerate(FILES, start=1):
        if name == "IO.SYS":
            first = io_cluster
        for cluster in range(first, first + count):
            fat_set(cluster, cluster + 1 if cluster < first + count - 1 else 0xFFF)
            lba = 11 + cluster - 2
            image[lba * SECTOR:(lba + 1) * SECTOR] = name.encode().ljust(SECTOR, b".")
        root[index * 32:(index + 1) * 32] = disk_tool.make_entry(
            disk_tool.short_name(name), attributes, first, count * SECTOR)
    image[1 * SECTOR:3 * SECTOR] = fat
    image[3 * SECTOR:5 * SECTOR] = fat
    image[5 * SECTOR:11 * SECTOR] = root
    return image


def make_d88(logical):
    header = bytearray(0x2B0)
    header[0x1B] = 0x20
    tracks = bytearray()
    for track in range(160):
        struct.pack_into("<I", header, 0x20 + track * 4, len(header) + len(tracks))
        for record in range(1, 9):
            tracks += struct.pack("<BBBBH", track // 2, track % 2, record, 3, 8)
            tracks += bytes(8) + struct.pack("<H", SECTOR)
            lba = track * 8 + record - 1
            tracks += logical[lba * SECTOR:(lba + 1) * SECTOR]
    image = header + tracks
    struct.pack_into("<I", image, 0x1C, len(image))
    return bytes(image)


def root_names(image):
    disk = disk_tool.PcEngineDisk(image, require_system_files=False)
    return [disk_tool.display_name(entry[:11]) for _, entry in disk_tool.iter_entries(disk.root)]


class MsdosSystemDiskTest(unittest.TestCase):
    def test_identifies_msdos(self):
        disk = disk_tool.PcEngineDisk(make_d88(make_logical()))
        self.assertEqual(disk.system, "MS-DOS")

    def test_io_sys_off_cluster_two_is_not_msdos(self):
        with self.assertRaisesRegex(disk_tool.DiskError, "missing ENGINEIO.SYS"):
            disk_tool.PcEngineDisk(make_d88(make_logical(io_cluster=13)))

    def test_vanilla_keeps_system_files_in_order_and_drops_utilities(self):
        with tempfile.TemporaryDirectory() as directory:
            source = Path(directory) / "dos.d88"
            output = Path(directory) / "vanilla.d88"
            source.write_bytes(make_d88(make_logical()))
            disk_tool.create_vanilla(source, output)
            image = output.read_bytes()
            self.assertEqual(
                root_names(image),
                ["MSDOS-VA", "IO.SYS", "MSDOS.SYS", "COMMAND.COM", "CONFIG.SYS",
                 "LICENSE.TXT"])
            disk = disk_tool.PcEngineDisk(image)
            self.assertEqual(disk.system, "MS-DOS")
            self.assertEqual(disk.cluster_chain(2), [2, 3])
            self.assertEqual(bytes(disk.read_cluster(2))[:6], b"IO.SYS")
            self.assertEqual(disk.fat_get(9), 0)  # UTIL.COM released
            self.assertEqual(bytes(disk.read_cluster(9)), bytes(SECTOR))

    def test_install_accepts_vanilla_msdos(self):
        with tempfile.TemporaryDirectory() as directory:
            work = Path(directory)
            (work / "dos.d88").write_bytes(make_d88(make_logical()))
            disk_tool.create_vanilla(work / "dos.d88", work / "out.d88")
            payload = work / "payload" / "DEMO"
            payload.mkdir(parents=True)
            (payload / "DEMO.COM").write_bytes(b"\xcd\x20")
            disk_tool.install_payload(work / "out.d88", work / "payload")
            self.assertEqual(root_names((work / "out.d88").read_bytes())[-1], "DEMO")

    def test_install_refuses_the_full_release_disk(self):
        with tempfile.TemporaryDirectory() as directory:
            work = Path(directory)
            (work / "dos.d88").write_bytes(make_d88(make_logical()))
            (work / "payload").mkdir()
            with self.assertRaisesRegex(disk_tool.DiskError, "neither an empty data disk"):
                disk_tool.install_payload(work / "dos.d88", work / "payload")


if __name__ == "__main__":
    unittest.main()
