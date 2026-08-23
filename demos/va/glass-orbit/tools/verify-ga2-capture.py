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

"""Validate the visible GLASS ORBIT GA-2 packed-4bpp CPU-fill proof.

The captured renderer has a 22-pixel frontend menu strip. In the selected
200-line mode VAEG places each logical G0 row on an even output row of the
640-by-400 composition canvas and leaves its odd neighbour black. This checker
validates that exact 200-row extent plus the first eight independently written
palette indices. It does not accept a non-empty image as proof.
"""

import argparse
import json
import struct
from pathlib import Path
from typing import Optional


EXPECTED_REGISTERS = {
    "schema": "vaeg-registers-v1",
    "ax": "4742",
    "cs": "2000",
    "ds": "2000",
    "es": "2000",
    "ss": "2000",
    "sp": "f000",
    "ip": "0100",
}
MENU_HEIGHT = 22
GUEST_WIDTH = 640
GUEST_HEIGHT = 400


def read_tsv(path: Path) -> dict[str, str]:
    values = {}
    for line in path.read_text(encoding="utf-8").splitlines():
        key, separator, value = line.partition("\t")
        if not separator or key in values:
            raise ValueError("GA2_CAPTURE_SCHEMA")
        values[key] = value
    return values


def read_bmp(path: Path) -> tuple[int, int, list[list[tuple[int, int, int]]]]:
    data = path.read_bytes()
    if len(data) < 54 or data[:2] != b"BM":
        raise ValueError("GA2_BMP_HEADER")
    offset = struct.unpack_from("<I", data, 10)[0]
    dib_size = struct.unpack_from("<I", data, 14)[0]
    if dib_size < 40 or len(data) < 14 + dib_size:
        raise ValueError("GA2_BMP_DIB")
    width, signed_height = struct.unpack_from("<ii", data, 18)
    planes, bpp, compression = struct.unpack_from("<HHI", data, 26)
    if (width <= 0 or signed_height == 0 or planes != 1 or bpp not in (24, 32) or
            compression not in (0, 3)):
        raise ValueError("GA2_BMP_FORMAT")
    if compression == 3:
        if bpp != 32 or dib_size < 52:
            raise ValueError("GA2_BMP_BITFIELDS")
        red_mask, green_mask, blue_mask = struct.unpack_from("<III", data, 54)
        if (red_mask, green_mask, blue_mask) != (0x00FF0000, 0x0000FF00, 0x000000FF):
            raise ValueError("GA2_BMP_BITFIELDS")
    height = abs(signed_height)
    stride = ((width * bpp + 31) // 32) * 4
    if offset + stride * height > len(data):
        raise ValueError("GA2_BMP_TRUNCATED")
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


def color_class(pixel: tuple[int, int, int]) -> Optional[int]:
    red, green, blue = pixel
    high = 160
    low = 80
    if red < low and green < low and blue >= high:
        return 1
    if red >= high and green < low and blue < low:
        return 2
    if red >= high and green < low and blue >= high:
        return 3
    if red < low and green >= high and blue < low:
        return 4
    if red < low and green >= high and blue >= high:
        return 5
    if red >= high and green >= high and blue < low:
        return 6
    if red >= high and green >= high and blue >= high:
        return 7
    if max(pixel) - min(pixel) <= 12 and 80 <= red <= 200:
        return 8
    return None


def find_probe(rows: list[list[tuple[int, int, int]]], start_y: int) -> Optional[tuple[int, int]]:
    for y in range(start_y, min(start_y + 8, len(rows))):
        for x in range(0, 9):
            classes = [color_class(rows[y][x + index]) for index in range(8)]
            if classes == list(range(1, 9)):
                return x, y
    return None


def black(pixel: tuple[int, int, int]) -> bool:
    return max(pixel) <= 4


def validate(capture_dir: Path) -> dict[str, object]:
    errors = []
    registers_path = capture_dir / "ga2-idle.registers.tsv"
    screen_path = capture_dir / "ga2-idle.screen.bmp"
    events_path = capture_dir / "events.tsv"
    values: dict[str, str] = {}
    if not registers_path.is_file():
        errors.append("GA2_REGISTERS_MISSING")
    else:
        try:
            values = read_tsv(registers_path)
        except (OSError, UnicodeError, ValueError):
            errors.append("GA2_CAPTURE_SCHEMA")
    for key, expected in EXPECTED_REGISTERS.items():
        if values.get(key) != expected:
            errors.append(f"GA2_{key.upper()}_MISMATCH")
    try:
        flags = int(values.get("flags", ""), 16)
    except ValueError:
        errors.append("GA2_FLAGS_INVALID")
    else:
        if flags & 0x0400:
            errors.append("GA2_DIRECTION_FLAG_SET")
        if not flags & 0x0200:
            errors.append("GA2_INTERRUPTS_DISABLED")
    if not events_path.is_file():
        errors.append("GA2_EVENTS_MISSING")
    elif not any(line.startswith("pc\t") for line in events_path.read_text(encoding="utf-8").splitlines()):
        errors.append("GA2_IDLE_NOT_REACHED")

    image_size: Optional[list[int]] = None
    fill_pixels = 0
    blank_pixels = 0
    viewport_pixels = 0
    probe = None
    if not screen_path.is_file() or screen_path.stat().st_size == 0:
        errors.append("GA2_SCREEN_MISSING")
    else:
        try:
            width, height, rows = read_bmp(screen_path)
        except (OSError, ValueError) as error:
            errors.append(str(error))
        else:
            image_size = [width, height]
            if width != GUEST_WIDTH or height < MENU_HEIGHT + GUEST_HEIGHT:
                errors.append("GA2_VIEWPORT_DIMENSIONS")
            else:
                viewport_pixels = GUEST_WIDTH * GUEST_HEIGHT
                for logical_y in range(GUEST_HEIGHT // 2):
                    visible_y = MENU_HEIGHT + logical_y * 2
                    separator_y = visible_y + 1
                    for x in range(GUEST_WIDTH):
                        expected_class = 5
                        if logical_y == 0 and x < 8:
                            expected_class = x + 1
                        if color_class(rows[visible_y][x]) == expected_class:
                            fill_pixels += 1
                        if black(rows[separator_y][x]):
                            blank_pixels += 1
                if fill_pixels != viewport_pixels // 2:
                    errors.append("GA2_FILL_NOT_UNIFORM")
                if blank_pixels != viewport_pixels // 2:
                    errors.append("GA2_200_LINE_BOUNDS")
                probe = find_probe(rows, MENU_HEIGHT)
                if probe is None:
                    errors.append("GA2_NIBBLE_PROBE_MISSING")

    return {
        "schema": "glass-orbit-ga2-v1",
        "status": "PASS" if not errors else "FAIL",
        "errors": errors,
        "image_size": image_size,
        "viewport_pixels": viewport_pixels,
        "filled_logical_pixels": fill_pixels,
        "blank_separator_pixels": blank_pixels,
        "nibble_probe": list(probe) if probe is not None else None,
        "registers": {key: values.get(key) for key in EXPECTED_REGISTERS},
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("capture_dir", type=Path)
    args = parser.parse_args()
    try:
        result = validate(args.capture_dir)
    except OSError as error:
        result = {"schema": "glass-orbit-ga2-v1", "status": "FAIL", "errors": [str(error)]}
    print(json.dumps(result, sort_keys=True))
    return 0 if result["status"] == "PASS" else 1


if __name__ == "__main__":
    raise SystemExit(main())
