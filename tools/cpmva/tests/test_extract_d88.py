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

"""Tests for the tolerant CP/MVA D88 extractor."""

import importlib.util
from pathlib import Path
import struct
import unittest

HERE = Path(__file__).resolve().parent


def load(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    assert spec.loader is not None
    spec.loader.exec_module(module)
    return module


installer = load("installer_for_extract_test", HERE.parent / "install_cpmva.py")
extractor = load("extract_d88", HERE.parent / "zex" / "extract_d88.py")

FILES = {"HELLO.COM": b"\xc9" * 128, "OUT.TXT": b"text\r\n" + b"\x1a" * 122}


def make_image():
    return installer.build_tools_disk(installer.pad_cpm_records(dict(FILES), b"\x00"))


def reorder_like_hxcfe(image):
    """Same sectors, another header name/type and a reversed track order."""
    offsets = struct.unpack_from("<164I", image, 0x20)
    tracks = []
    for index, offset in enumerate(offsets):
        if not offset:
            continue
        end = next((o for o in sorted(offsets) if o > offset), len(image))
        tracks.append(image[offset:end])
    header = bytearray(image[:0x2B0])
    header[0:17] = b"HxCFE".ljust(17, b"\0")
    header[0x1B] = 0x00
    body = bytearray()
    new_offsets = [0] * 164
    for index, track in reversed(list(enumerate(tracks))):
        new_offsets[index] = 0x2B0 + len(body)
        body.extend(track)
    for index, offset in enumerate(new_offsets):
        struct.pack_into("<I", header, 0x20 + index * 4, offset)
    return bytes(header) + bytes(body)


class ExtractTest(unittest.TestCase):
    def test_installer_layout(self):
        self.assertEqual(extractor.extract(make_image()), FILES)

    def test_reordered_foreign_header(self):
        image = reorder_like_hxcfe(make_image())
        with self.assertRaises(installer.InstallerError):
            installer.unwrap_cpm_d88(image)
        self.assertEqual(extractor.extract(image), FILES)

    def test_missing_sector(self):
        image = bytearray(make_image())
        struct.pack_into("<I", image, 0x20 + 79 * 4, 0)
        with self.assertRaises(extractor.ExtractError) as context:
            extractor.extract(bytes(image))
        self.assertEqual(context.exception.code, "D88_MISSING")

    def test_sector_error_status(self):
        image = bytearray(make_image())
        first = struct.unpack_from("<I", image, 0x20)[0]
        image[first + 8] = 0xE0
        with self.assertRaises(extractor.ExtractError) as context:
            extractor.extract(bytes(image))
        self.assertEqual(context.exception.code, "D88_STATUS")

    def test_duplicate_sector(self):
        image = bytearray(make_image())
        struct.pack_into("<I", image, 0x20 + 1 * 4, struct.unpack_from("<I", image, 0x20)[0])
        with self.assertRaises(extractor.ExtractError) as context:
            extractor.extract(bytes(image))
        self.assertEqual(context.exception.code, "D88_DUPLICATE")


if __name__ == "__main__":
    unittest.main()
