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

"""Validate the visible GLASS ORBIT GA-3 sixteen-colour palette proof.

The renderer's 640-by-400 canvas has a 22-pixel frontend menu strip. In this
200-line mode every logical G0 row occupies the even output row and the odd
row is black. The checker validates all sixteen 40-pixel bars on every logical
row and requires sixteen distinct sampled RGB values.
"""

import argparse
import json
import struct
from pathlib import Path


EXPECTED_REGISTERS = {
    "schema": "vaeg-registers-v1",
    "ax": "4743",
    "cs": "2000",
    "ds": "2000",
    "es": "2000",
    "ss": "2000",
    "sp": "f000",
    "ip": "0100",
}
MENU_HEIGHT = 22
GUEST_WIDTH = 640
COMPOSITION_HEIGHT = 400
LOGICAL_HEIGHT = 200
BAR_COUNT = 16
BAR_WIDTH = 40


def read_tsv(path):
    values = {}
    for line in path.read_text(encoding="utf-8").splitlines():
        key, separator, value = line.partition("\t")
        if not separator or key in values:
            raise ValueError("GA3_CAPTURE_SCHEMA")
        values[key] = value
    return values


def read_bmp(path):
    data = path.read_bytes()
    if len(data) < 54 or data[:2] != b"BM":
        raise ValueError("GA3_BMP_HEADER")
    offset = struct.unpack_from("<I", data, 10)[0]
    dib_size = struct.unpack_from("<I", data, 14)[0]
    if dib_size < 40 or len(data) < 14 + dib_size:
        raise ValueError("GA3_BMP_DIB")
    width, signed_height = struct.unpack_from("<ii", data, 18)
    planes, bpp, compression = struct.unpack_from("<HHI", data, 26)
    if (width <= 0 or signed_height == 0 or planes != 1 or bpp not in (24, 32) or
            compression not in (0, 3)):
        raise ValueError("GA3_BMP_FORMAT")
    if compression == 3:
        if bpp != 32 or dib_size < 52:
            raise ValueError("GA3_BMP_BITFIELDS")
        red_mask, green_mask, blue_mask = struct.unpack_from("<III", data, 54)
        if (red_mask, green_mask, blue_mask) != (0x00FF0000, 0x0000FF00, 0x000000FF):
            raise ValueError("GA3_BMP_BITFIELDS")
    height = abs(signed_height)
    stride = ((width * bpp + 31) // 32) * 4
    if offset + stride * height > len(data):
        raise ValueError("GA3_BMP_TRUNCATED")
    rows = []
    bytes_per_pixel = bpp // 8
    for source_y in range(height):
        start = offset + source_y * stride
        row = []
        for x in range(width):
            blue, green, red = data[start + x * bytes_per_pixel:start + x * bytes_per_pixel + 3]
            row.append((red, green, blue))
        rows.append(row)
    if signed_height > 0:
        rows.reverse()
    return width, height, rows


def black(pixel):
    return max(pixel) <= 4


def validate(capture_dir):
    errors = []
    registers_path = capture_dir / "ga3-idle.registers.tsv"
    screen_path = capture_dir / "ga3-idle.screen.bmp"
    events_path = capture_dir / "events.tsv"
    values = {}
    if not registers_path.is_file():
        errors.append("GA3_REGISTERS_MISSING")
    else:
        try:
            values = read_tsv(registers_path)
        except (OSError, UnicodeError, ValueError):
            errors.append("GA3_CAPTURE_SCHEMA")
    for key, expected in EXPECTED_REGISTERS.items():
        if values.get(key) != expected:
            errors.append("GA3_%s_MISMATCH" % key.upper())
    try:
        flags = int(values.get("flags", ""), 16)
    except ValueError:
        errors.append("GA3_FLAGS_INVALID")
    else:
        if flags & 0x0400:
            errors.append("GA3_DIRECTION_FLAG_SET")
        if not flags & 0x0200:
            errors.append("GA3_INTERRUPTS_DISABLED")
    if not events_path.is_file():
        errors.append("GA3_EVENTS_MISSING")
    elif not any(line.startswith("pc\t") for line in events_path.read_text(encoding="utf-8").splitlines()):
        errors.append("GA3_IDLE_NOT_REACHED")

    image_size = None
    palette_samples = []
    matched_bar_pixels = 0
    matched_separator_pixels = 0
    if not screen_path.is_file() or screen_path.stat().st_size == 0:
        errors.append("GA3_SCREEN_MISSING")
    else:
        try:
            width, height, rows = read_bmp(screen_path)
        except (OSError, ValueError) as error:
            errors.append(str(error))
        else:
            image_size = [width, height]
            if width != GUEST_WIDTH or height < MENU_HEIGHT + COMPOSITION_HEIGHT:
                errors.append("GA3_VIEWPORT_DIMENSIONS")
            else:
                visible_y = MENU_HEIGHT
                palette_samples = [rows[visible_y][bar * BAR_WIDTH + BAR_WIDTH // 2]
                                   for bar in range(BAR_COUNT)]
                if len(set(palette_samples)) != BAR_COUNT:
                    errors.append("GA3_PALETTE_NOT_DISTINCT")
                for logical_y in range(LOGICAL_HEIGHT):
                    visible_y = MENU_HEIGHT + logical_y * 2
                    separator_y = visible_y + 1
                    for bar in range(BAR_COUNT):
                        expected = palette_samples[bar]
                        for x in range(bar * BAR_WIDTH, (bar + 1) * BAR_WIDTH):
                            if rows[visible_y][x] == expected:
                                matched_bar_pixels += 1
                    for x in range(GUEST_WIDTH):
                        if black(rows[separator_y][x]):
                            matched_separator_pixels += 1
                expected_bar_pixels = GUEST_WIDTH * LOGICAL_HEIGHT
                if matched_bar_pixels != expected_bar_pixels:
                    errors.append("GA3_BAR_GEOMETRY")
                if matched_separator_pixels != expected_bar_pixels:
                    errors.append("GA3_200_LINE_BOUNDS")

    return {
        "schema": "glass-orbit-ga3-v1",
        "status": "PASS" if not errors else "FAIL",
        "errors": errors,
        "image_size": image_size,
        "palette_samples_rgb": [list(sample) for sample in palette_samples],
        "matched_bar_pixels": matched_bar_pixels,
        "matched_separator_pixels": matched_separator_pixels,
        "registers": {key: values.get(key) for key in EXPECTED_REGISTERS},
    }


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("capture_dir", type=Path)
    args = parser.parse_args()
    try:
        result = validate(args.capture_dir)
    except OSError as error:
        result = {"schema": "glass-orbit-ga3-v1", "status": "FAIL", "errors": [str(error)]}
    print(json.dumps(result, sort_keys=True))
    return 0 if result["status"] == "PASS" else 1


if __name__ == "__main__":
    raise SystemExit(main())
