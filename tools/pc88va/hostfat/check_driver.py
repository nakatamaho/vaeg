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
    if data.count(EXPECTED_BPB) != 1:
        fail("RDBMS-compatible BPB was not found exactly once")
    for marker in (b"check_hostfat", b"read_hostfat1", b"H1"):
        if data.count(marker) != 1:
            fail(f"protocol marker {marker!r} was not found exactly once")
    digest = hashlib.sha256(data).hexdigest()
    print(f"HOSTFAT.SYS ok: {len(data)} bytes sha256={digest}")


if __name__ == "__main__":
    main()
