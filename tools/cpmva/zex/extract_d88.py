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

"""Extract the files of a CP/MVA disk image written by vaeg or a real machine.

usage: extract_d88.py IMAGE OUTPUT_DIR [--lowercase] [--skip-com]

Unlike the installer's strict reader, this reads any D88 container (for
example one imaged by the HxC Floppy Emulator, with another header name,
media type and track order). Each sector is placed by its C/H/R address. The
CP/MVA geometry must be complete: 40 cylinders x 2 heads x 16 sectors of 256
bytes, no duplicates and no sector errors. Files are written with their
128-byte record length, as CP/M stores them.
"""

from __future__ import annotations

import argparse
import hashlib
import importlib.util
from pathlib import Path
import struct
import sys

CYLINDERS = 40
HEADS = 2
SECTORS = 16
SECTOR_SIZE = 256
TRACK_SLOTS = 164
HEADER_SIZE = 0x2B0


class ExtractError(Exception):
    def __init__(self, code: str, message: str) -> None:
        super().__init__(f"{code}: {message}")
        self.code = code


def load_installer():
    path = Path(__file__).resolve().parent.parent / "install_cpmva.py"
    spec = importlib.util.spec_from_file_location("cpmva_installer_extract", path)
    module = importlib.util.module_from_spec(spec)
    assert spec.loader is not None
    spec.loader.exec_module(module)
    return module


def raw_from_d88(image: bytes) -> bytes:
    """Rebuild the linear CP/M image from sector C/H/R addresses."""
    if len(image) < HEADER_SIZE:
        raise ExtractError("D88_SIZE", "image is shorter than a D88 header")
    raw = bytearray(CYLINDERS * HEADS * SECTORS * SECTOR_SIZE)
    seen = set()
    for slot in range(TRACK_SLOTS):
        offset = struct.unpack_from("<I", image, 0x20 + slot * 4)[0]
        if offset == 0:
            continue
        if offset < HEADER_SIZE or offset + 16 > len(image):
            raise ExtractError("D88_TRACK", f"track {slot} offset {offset:#x} is out of range")
        count = struct.unpack_from("<H", image, offset + 4)[0]
        cursor = offset
        for _ in range(count):
            if cursor + 16 > len(image):
                raise ExtractError("D88_SECTOR", f"sector header beyond the image in track {slot}")
            c, h, r, _n = struct.unpack_from("<BBBB", image, cursor)
            status = image[cursor + 8]
            size = struct.unpack_from("<H", image, cursor + 14)[0]
            data = image[cursor + 16 : cursor + 16 + size]
            if status != 0:
                raise ExtractError("D88_STATUS", f"sector C{c} H{h} R{r} has status {status:#x}")
            if size != SECTOR_SIZE or len(data) != SECTOR_SIZE:
                raise ExtractError("D88_GEOMETRY", f"sector C{c} H{h} R{r} is {size} bytes")
            if c >= CYLINDERS or h >= HEADS or not 1 <= r <= SECTORS:
                raise ExtractError("D88_GEOMETRY", f"sector C{c} H{h} R{r} is outside CP/MVA geometry")
            if (c, h, r) in seen:
                raise ExtractError("D88_DUPLICATE", f"sector C{c} H{h} R{r} appears twice")
            seen.add((c, h, r))
            start = ((c * HEADS + h) * SECTORS + (r - 1)) * SECTOR_SIZE
            raw[start : start + SECTOR_SIZE] = data
            cursor += 16 + size
    missing = CYLINDERS * HEADS * SECTORS - len(seen)
    if missing:
        raise ExtractError("D88_MISSING", f"{missing} sectors are missing")
    return bytes(raw)


def extract(image: bytes) -> dict[str, bytes]:
    installer = load_installer()
    return installer.parse_cpm_raw(raw_from_d88(image))


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("image", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--lowercase", action="store_true", help="write lower-case file names")
    parser.add_argument("--skip-com", action="store_true", help="do not write .COM programs")
    args = parser.parse_args()
    try:
        files = extract(args.image.read_bytes())
    except ExtractError as error:
        print(f"FAIL {error}", file=sys.stderr)
        return 1
    args.output.mkdir(parents=True, exist_ok=True)
    for name, data in sorted(files.items()):
        if args.skip_com and name.endswith(".COM"):
            continue
        target = name.lower() if args.lowercase else name
        (args.output / target).write_bytes(data)
        print(f"{hashlib.sha256(data).hexdigest()}  {target}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
