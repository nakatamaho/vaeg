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
"""Tests for the VZ Editor extractor on a synthetic PC-98 2HD FAT12 D88.

The real release image is not in the repository, so the content layer is
exercised with its own name/hash mapping; only the image-identity check is
driven through the command line.
"""

import hashlib
import importlib.util
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import unittest

TOOL = Path(__file__).resolve().parents[2] / "tools" / "pc88va" / "extract-vz-editor.py"
SPEC = importlib.util.spec_from_file_location("extract_vz_under_test", TOOL)
extractor = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(extractor)

SECTOR = 1024
PER_TRACK = 8
CYLINDERS = 77
FILES = {"VZVA.COM": b"editor\n" * 300, "VZ.DEF": b"definitions\n"}


def make_logical(files=FILES, chain_loop=False, bad_bpb=False):
    per_fat = 2
    root_entries = 192
    image = bytearray(CYLINDERS * 2 * PER_TRACK * SECTOR)
    image[0:3] = b"\xeb\x1f\x90"
    struct.pack_into("<HBHBHHBH", image, 11, SECTOR, 1, 1, 2, root_entries,
                     CYLINDERS * 2 * PER_TRACK, 0xFE, per_fat)
    if bad_bpb:
        struct.pack_into("<H", image, 11, SECTOR * 2)  # bytes per sector
    fat = bytearray(per_fat * SECTOR)
    root = bytearray(root_entries * 32)
    root_start = 1 + 2 * per_fat
    root_sectors = root_entries * 32 // SECTOR
    data_start = root_start + root_sectors

    def fat_set(cluster, value):
        offset = cluster * 3 // 2
        word = struct.unpack_from("<H", fat, offset)[0]
        word = ((word & 0x000F) | (value << 4)) if cluster & 1 else \
               ((word & 0xF000) | value)
        struct.pack_into("<H", fat, offset, word)

    cluster = 2
    for index, (name, contents) in enumerate(files.items()):
        count = max(1, (len(contents) + SECTOR - 1) // SECTOR)
        for step in range(count):
            following = cluster + step + 1 if step < count - 1 else 0xFFF
            if chain_loop and index == 0:
                following = cluster
            fat_set(cluster + step, following)
            begin = (data_start + cluster - 2 + step) * SECTOR
            image[begin:begin + SECTOR] = contents[step * SECTOR:(step + 1) * SECTOR] \
                .ljust(SECTOR, b"\x1a")
        base, _, extension = name.partition(".")
        entry = bytearray(32)
        entry[:11] = base.ljust(8).encode() + extension.ljust(3).encode()
        entry[11] = 0x20
        struct.pack_into("<H", entry, 26, cluster)
        struct.pack_into("<I", entry, 28, len(contents))
        root[index * 32:(index + 1) * 32] = entry
        cluster += count
    image[1 * SECTOR:(1 + per_fat) * SECTOR] = fat
    image[(1 + per_fat) * SECTOR:(1 + 2 * per_fat) * SECTOR] = fat
    image[root_start * SECTOR:(root_start + root_sectors) * SECTOR] = root
    return bytes(image)


def make_d88(logical):
    header = bytearray(0x2B0)
    header[0x1B] = 0x20
    tracks = bytearray()
    for track in range(CYLINDERS * 2):
        struct.pack_into("<I", header, 0x20 + track * 4, len(header) + len(tracks))
        for record in range(1, PER_TRACK + 1):
            tracks += struct.pack("<BBBBH", track // 2, track % 2, record, 3, PER_TRACK)
            tracks += bytes(8) + struct.pack("<H", SECTOR)
            index = track * PER_TRACK + record - 1
            tracks += logical[index * SECTOR:(index + 1) * SECTOR]
    image = header + tracks
    struct.pack_into("<I", image, 0x1C, len(image))
    return bytes(image)


def names_of(files=FILES):
    return {name: hashlib.sha256(data).hexdigest() for name, data in files.items()}


class ExtractVzTest(unittest.TestCase):
    def test_extracts_the_named_files(self):
        self.assertEqual(extractor.extract(make_d88(make_logical()), names_of()),
                         FILES)

    def test_missing_file(self):
        with self.assertRaisesRegex(extractor.ExtractError,
                                    r"^M106_VZ_MISSING_FILE VZFL\.DEF$"):
            extractor.extract(make_d88(make_logical()),
                              {**names_of(), "VZFL.DEF": "00" * 32})

    def test_changed_file(self):
        names = {**names_of(), "VZ.DEF": "11" * 32}
        with self.assertRaisesRegex(extractor.ExtractError, r"^M106_VZ_FILE_SHA256 "):
            extractor.extract(make_d88(make_logical()), names)

    def test_broken_parameter_block(self):
        with self.assertRaisesRegex(extractor.ExtractError, r"^M106_VZ_GEOMETRY "):
            extractor.extract(make_d88(make_logical(bad_bpb=True)), names_of())

    def test_fat_chain_loop(self):
        with self.assertRaisesRegex(extractor.ExtractError, r"^M106_VZ_GEOMETRY "):
            extractor.extract(make_d88(make_logical(chain_loop=True)), names_of())

    def test_vzva_def_sets_em0_only(self):
        vz_def = b"x\r\nEM\t\t\t;ems\r\nXM0\t\t\t;xms\r\n"
        original = extractor.VZVA_DEF_SHA256
        expected = vz_def.replace(b"EM\t\t\t", b"EM0\t\t")
        extractor.VZVA_DEF_SHA256 = hashlib.sha256(expected).hexdigest()
        try:
            self.assertEqual(extractor.vzva_def(vz_def), expected)
        finally:
            extractor.VZVA_DEF_SHA256 = original

    def test_vzva_def_without_em_line(self):
        with self.assertRaisesRegex(extractor.ExtractError, r"^M106_VZ_EMS_OPTION "):
            extractor.vzva_def(b"x\r\nXM0\t\t\t;xms\r\n")

    def test_vzva_def_with_unexpected_result(self):
        with self.assertRaisesRegex(extractor.ExtractError,
                                    r"^M106_VZ_FILE_SHA256 VZVA\.DEF "):
            extractor.vzva_def(b"x\r\nEM\t\t\t;ems\r\n")

    def test_other_image_is_refused_by_identity(self):
        with tempfile.TemporaryDirectory() as directory:
            image = Path(directory) / "other.d88"
            image.write_bytes(make_d88(make_logical()))
            result = subprocess.run(
                [sys.executable, "-I", str(TOOL), "--image", str(image),
                 "--output", str(Path(directory) / "out")],
                capture_output=True, text=True)
            self.assertEqual(result.returncode, 1)
            self.assertIn("M106_VZ_IMAGE_SHA256", result.stderr)


if __name__ == "__main__":
    unittest.main()
