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

"""Checks of the committed bootable MS-DOS 4.0 all-demos disk.

The content validator of build-all-demos-msdos4-disk.py must accept
demos/disks/all-demos-msdos4.d88.xz and reject each single mutation with its
own error code.
"""

import importlib.util
import lzma
from pathlib import Path
import struct
import unittest

ROOT = Path(__file__).resolve().parents[2]
BUILDER = ROOT / "tools" / "pc88va" / "build-all-demos-msdos4-disk.py"
SPEC = importlib.util.spec_from_file_location("all_demos_msdos4_under_test", BUILDER)
builder = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(builder)
DISTRIBUTIONS = ROOT / "demos" / "disks"
IMAGE = lzma.decompress((DISTRIBUTIONS / "all-demos-msdos4.d88.xz").read_bytes())


def locate(name):
    """Image offset of the first data byte of a root file."""
    module = builder.load_module("pc88va_all_demos_bootable_builder",
                                 builder.BOOTABLE_BUILDER).load_pcengine_module()
    disk = module.PcEngineDisk(IMAGE)
    offset, _ = module.find_entry(disk.root, module.short_name(name))
    cluster = struct.unpack_from("<H", disk.root, offset + 26)[0]
    lba = module.DATA_START_LBA + cluster - 2
    return disk.normal_sectors[lba][0]


def mutated(offset):
    image = bytearray(IMAGE)
    image[offset] ^= 0x01
    return bytes(image)


class AllDemosMsdos4DiskTest(unittest.TestCase):
    def test_committed_image_passes(self):
        self.assertEqual(builder.validate_image(IMAGE, DISTRIBUTIONS), [])

    def test_changed_system_file(self):
        self.assertEqual(builder.validate_image(mutated(locate("MSDOS.SYS") + 100),
                                                DISTRIBUTIONS),
                         ["M106_MSDOS4_SYSTEM_FILE_MISMATCH"])

    def test_changed_demo_file(self):
        offset = locate("GLASS")  # the directory's cluster: "." and ".." entries
        module = builder.load_module("pc88va_all_demos_bootable_builder",
                                     builder.BOOTABLE_BUILDER).load_pcengine_module()
        disk = module.PcEngineDisk(IMAGE)
        _, entry = list(module.iter_entries(IMAGE[offset:offset + 1024]))[2]
        lba = module.DATA_START_LBA + struct.unpack_from("<H", entry, 26)[0] - 2
        self.assertEqual(builder.validate_image(mutated(disk.normal_sectors[lba][0] + 100),
                                                DISTRIBUTIONS),
                         ["M106_MSDOS4_DEMO_PAYLOAD_MISMATCH"])

    def test_changed_boot_sector(self):
        module = builder.load_module("pc88va_all_demos_bootable_builder",
                                     builder.BOOTABLE_BUILDER).load_pcengine_module()
        offset = module.PcEngineDisk(IMAGE).normal_sectors[0][0]
        self.assertEqual(builder.validate_image(mutated(offset + 0x100), DISTRIBUTIONS),
                         ["M106_MSDOS4_BOOT_SECTOR_MISMATCH"])

    def test_broken_parameter_block(self):
        module = builder.load_module("pc88va_all_demos_bootable_builder",
                                     builder.BOOTABLE_BUILDER).load_pcengine_module()
        offset = module.PcEngineDisk(IMAGE).normal_sectors[0][0]
        self.assertEqual(builder.validate_image(mutated(offset + 16), DISTRIBUTIONS),
                         ["M106_MSDOS4_NOT_A_SYSTEM_DISK"])


if __name__ == "__main__":
    unittest.main()
