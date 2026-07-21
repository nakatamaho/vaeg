#!/usr/bin/env python3
# Copyright (c) 2026 Nakata Maho
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
# 1. Redistributions of source code must retain the above copyright notice,
#    this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
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

import argparse
import hashlib
import struct
from pathlib import Path


EXPECTED_BPB = bytes.fromhex("00040200000280000020f0070090")


def fail(message: str) -> None:
    raise SystemExit(f"HOSTFAT.SYS check failed: {message}")


def require_count(data: bytes, marker: bytes, expected: int, description: str) -> None:
    actual = data.count(marker)
    if actual != expected:
        fail(f"{description} was found {actual} times, expected {expected}")


def main() -> None:
    parser = argparse.ArgumentParser(description="Validate the M54 HOSTFAT.SYS")
    parser.add_argument("--input", type=Path, required=True)
    args = parser.parse_args()
    data = args.input.read_bytes()
    if len(data) < 128 or len(data) > 4096:
        fail(f"unexpected size {len(data)}")
    next_driver, attributes, strategy, interrupt = struct.unpack_from("<IHHH", data)
    if next_driver != 0xFFFFFFFF:
        fail("next-driver pointer is not FFFFFFFF")
    if attributes != 0x2000:
        fail(f"attributes are {attributes:04x}, expected 2000")
    if not (15 <= strategy < len(data)) or not (15 <= interrupt < len(data)):
        fail("strategy or interrupt offset is outside the driver")
    if strategy == interrupt:
        fail("strategy and interrupt entry points are identical")
    if data[10] != 1:
        fail(f"unit count is {data[10]}, expected 1")
    name_offset, name_segment = struct.unpack_from("<HH", data, 11)
    if name_segment != 0 or name_offset + 8 > len(data):
        fail("device-name far pointer is invalid")
    if data[name_offset : name_offset + 8] != b"HOSTFAT\0":
        fail("device name is not HOSTFAT")
    require_count(data, EXPECTED_BPB, 1, "RDBMS-compatible BPB")
    bpb_offset = data.find(EXPECTED_BPB)
    bpb_pointer_list_offset = bpb_offset - 2
    if bpb_pointer_list_offset < 15:
        fail("BPB pointer list is outside the driver data area")
    if struct.unpack_from("<H", data, bpb_pointer_list_offset)[0] != bpb_offset:
        fail("BPB pointer list does not point to the BPB")

    # PC-Engine's non-IBM block request packet has eight reserved bytes at
    # offsets 5..12. Validate the emitted field displacements themselves so
    # an IBM-sized 18-byte packet layout cannot pass this structural check.
    request_layout_markers = (
        (bytes.fromhex("26c6470e01"), 1, "media-check result at 0EH"),
        (bytes.fromhex("26c6470df0"), 1, "Build-BPB media byte at 0DH"),
        (bytes.fromhex("26c6470d01"), 1, "initialize media byte at 0DH"),
        (bytes.fromhex("26c6470d00"), 1, "unavailable media byte at 0DH"),
        (b"\x26\xc7\x47\x12" + struct.pack("<H", bpb_offset), 1,
         "Build-BPB pointer offset at 12H"),
        (b"\x26\xc7\x47\x12" + struct.pack("<H", bpb_pointer_list_offset), 1,
         "initialize BPB-list pointer offset at 12H"),
        (bytes.fromhex("268c4f14"), 2, "BPB pointer segment at 14H"),
        (bytes.fromhex("26c747120000"), 2, "zero sector-count/BPB offset at 12H"),
        (bytes.fromhex("26c747140000"), 1, "zero BPB segment at 14H"),
        (bytes.fromhex("26c7470e0000"), 1, "resident-end offset at 0EH"),
        (bytes.fromhex("26894710"), 1, "resident-end segment at 10H"),
        (bytes.fromhex("3c0d"), 1, "open-command comparison"),
        (bytes.fromhex("3c0e"), 1, "close-command comparison"),
    )
    for marker, expected, description in request_layout_markers:
        require_count(data, marker, expected, description)

    for marker in (b"check_hostfat", b"read_hostfat1", b"H1"):
        require_count(data, marker, 1, f"protocol marker {marker!r}")
    digest = hashlib.sha256(data).hexdigest()
    print(f"HOSTFAT.SYS ok: {len(data)} bytes sha256={digest}")


if __name__ == "__main__":
    main()
