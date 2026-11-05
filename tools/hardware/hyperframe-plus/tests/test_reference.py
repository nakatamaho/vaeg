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
"""Small passing fixtures and single mutations, independent of real inputs/Git."""
import struct
import sys
import unittest
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import verify_reference as ref


def fixture():
    data = bytearray(64)
    data[:2] = b"MZ"
    struct.pack_into("<13H", data, 2, 64, 1, 1, 2, 0, 65535, 0, 64, 0, 0, 0, 28, 0)
    struct.pack_into("<HH", data, 28, 2, 0)
    data[32:] = b"\x90" * 31 + b"\xc3"
    return data


class ReferenceTests(unittest.TestCase):
    def expect(self, code, function, *args):
        with self.assertRaises(ref.ValidationError) as caught:
            function(*args)
        self.assertEqual(caught.exception.code, code)

    def setUp(self):
        self.data = fixture()
        self.assertEqual(ref.mz_metadata(self.data)["header_bytes"], 32)

    def test_hash(self):
        expected = ref.sha256(self.data)
        ref.verify_hash(self.data, expected)
        self.data[-1] ^= 1
        self.expect("HFRE_HASH_MISMATCH", ref.verify_hash, self.data, expected)

    def test_magic(self):
        self.data[0] ^= 1
        self.expect("HFRE_MZ_MAGIC", ref.mz_metadata, self.data)

    def test_declared_size(self):
        struct.pack_into("<H", self.data, 2, 65)
        self.expect("HFRE_MZ_FILE_SIZE", ref.mz_metadata, self.data)

    def test_header_size(self):
        struct.pack_into("<H", self.data, 8, 1)
        self.expect("HFRE_MZ_FILE_SIZE", ref.mz_metadata, self.data)

    def test_relocation_table(self):
        struct.pack_into("<H", self.data, 6, 2)
        self.expect("HFRE_MZ_RELOCATION_RANGE", ref.mz_metadata, self.data)

    def test_relocation_target(self):
        struct.pack_into("<H", self.data, 28, 31)
        self.expect("HFRE_MZ_RELOCATION_TARGET", ref.mz_metadata, self.data)

    def test_entry(self):
        struct.pack_into("<H", self.data, 20, 32)
        self.expect("HFRE_MZ_ENTRY_RANGE", ref.mz_metadata, self.data)

    def test_landmark(self):
        load = self.data[32:]
        ref.verify_landmarks(load, {31: b"\xc3"})
        load[31] ^= 1
        self.expect("HFRE_LANDMARK_BYTES", ref.verify_landmarks, load, {31: b"\xc3"})

    def test_integration(self):
        report, load = ref.analyze_executable(self.data, ref.sha256(self.data),
                                             2, {31: b"\xc3"})
        self.assertEqual(len(load), 32)
        self.assertEqual(report["mz"]["relocations"][0]["image_offset"], 2)
        self.assertEqual(report["landmarks"][0]["classification"],
                         "BYTE_MATCH_ONLY; CONTROL_FLOW_NOT_VERIFIED")


if __name__ == "__main__":
    unittest.main()
