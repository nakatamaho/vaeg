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
"""Tests for the usable capacity of pcengine_disk.py images.

PC-Engine reads only 77 of the image's 80 cylinders; MS-DOS disks declare
all 80 in their parameter block.  The disks are synthetic.
"""

import importlib.util
from pathlib import Path
import sys
import unittest

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import test_pcengine_disk_msdos as msdos  # noqa: E402

disk_tool = msdos.disk_tool


def pcengine_image():
    """The synthetic MS-DOS disk with its parameter block cleared."""
    logical = bytearray(msdos.make_logical())
    logical[11:11 + len(disk_tool.MSDOS_BPB)] = bytes(len(disk_tool.MSDOS_BPB))
    return msdos.make_d88(bytes(logical))


class CapacityTest(unittest.TestCase):
    def test_pcengine_limit_is_cylinder_76(self):
        self.assertEqual(disk_tool.PCENGINE_LAST_DATA_CLUSTER, 1222)
        self.assertEqual(disk_tool.DATA_START_LBA + 1222 - 2, 77 * 16 - 1)

    def test_pcengine_disk_never_allocates_past_cylinder_76(self):
        disk = disk_tool.PcEngineDisk(pcengine_image(), require_system_files=False)
        free = disk.free_bytes() // disk_tool.SECTOR_SIZE
        clusters = disk.allocate_clusters(free)
        self.assertEqual(max(clusters), disk_tool.PCENGINE_LAST_DATA_CLUSTER)
        with self.assertRaisesRegex(disk_tool.DiskError, "not enough free space"):
            disk.allocate_clusters(1)

    def test_msdos_disk_uses_all_80_cylinders(self):
        disk = disk_tool.PcEngineDisk(msdos.make_d88(msdos.make_logical()))
        free = disk.free_bytes() // disk_tool.SECTOR_SIZE
        self.assertEqual(max(disk.allocate_clusters(free)), disk_tool.LAST_DATA_CLUSTER)


if __name__ == "__main__":
    unittest.main()
