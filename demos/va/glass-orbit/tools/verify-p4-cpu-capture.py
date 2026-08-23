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

"""Validate two fixed-frame GLASS ORBIT P4-1 CPU-reference captures.

The guest returns a rolling checksum of the raw 640-by-200 packed G0 page in
BX.  The host also checks byte-identical composed captures and a conservative
visible-cube shape envelope.  The BMP is diagnostic evidence only; it is not
the raw framebuffer oracle and cannot establish real PC-88VA conformance.
"""

from __future__ import annotations

import argparse
import json
import struct
from pathlib import Path


EXPECTED_PAYLOAD_SEGMENT = "3000"
EXPECTED_REGISTERS = {
    "schema": "vaeg-registers-v1",
    "ax": "4750",
    "cs": EXPECTED_PAYLOAD_SEGMENT,
    "ds": EXPECTED_PAYLOAD_SEGMENT,
    "es": EXPECTED_PAYLOAD_SEGMENT,
    "ss": EXPECTED_PAYLOAD_SEGMENT,
    "sp": "f000",
    "ip": "0200",
}
EXPECTED_RAW_CHECKSUM = "6dd9"
SCREEN_NAME = "glass-p4-cpu.screen.bmp"
REGISTERS_NAME = "glass-p4-cpu.registers.tsv"
EVENTS_NAME = "events.tsv"
WIDTH = 640
HEIGHT = 422
MENU_HEIGHT = 22
VIEWPORT_HEIGHT = 400


def read_registers(path: Path) -> dict[str, str]:
    values: dict[str, str] = {}
    for line in path.read_text(encoding="utf-8").splitlines():
        key, separator, value = line.partition("\t")
        if not separator or key in values:
            raise ValueError("P4_CPU_CAPTURE_SCHEMA")
        values[key] = value
    return values


def read_bmp_pixels(path: Path) -> tuple[int, int, list[list[tuple[int, int, int]]]]:
    data = path.read_bytes()
    if len(data) < 122 or data[:2] != b"BM":
        raise ValueError("P4_CPU_BMP_HEADER")
    offset = struct.unpack_from("<I", data, 10)[0]
    dib_size = struct.unpack_from("<I", data, 14)[0]
    width, signed_height = struct.unpack_from("<ii", data, 18)
    planes, bpp, compression = struct.unpack_from("<HHI", data, 26)
    if (dib_size < 108 or width <= 0 or signed_height == 0 or planes != 1 or
            bpp != 32 or compression != 3):
        raise ValueError("P4_CPU_BMP_FORMAT")
    red_mask, green_mask, blue_mask = struct.unpack_from("<III", data, 54)
    if (red_mask, green_mask, blue_mask) != (0x00FF0000, 0x0000FF00, 0x000000FF):
        raise ValueError("P4_CPU_BMP_BITFIELDS")
    height = abs(signed_height)
    stride = width * 4
    if offset + stride * height > len(data):
        raise ValueError("P4_CPU_BMP_TRUNCATED")
    rows = []
    for source_y in range(height):
        row = []
        begin = offset + source_y * stride
        for x in range(width):
            blue, green, red = data[begin + x * 4:begin + x * 4 + 3]
            row.append((red, green, blue))
        rows.append(row)
    if signed_height > 0:
        rows.reverse()
    return width, height, rows


def visible_shape(rows: list[list[tuple[int, int, int]]]) -> tuple[dict[str, object], list[str]]:
    errors: list[str] = []
    foreground = []
    colors: set[tuple[int, int, int]] = set()
    for y in range(MENU_HEIGHT, MENU_HEIGHT + VIEWPORT_HEIGHT):
        for x, pixel in enumerate(rows[y]):
            if pixel != (0, 0, 0):
                foreground.append((x, y))
                colors.add(pixel)
    if not foreground:
        errors.append("P4_CPU_FOREGROUND_MISSING")
        return {"bbox": None, "pixels": 0, "colors": 0}, errors
    min_x = min(x for x, _ in foreground)
    max_x = max(x for x, _ in foreground)
    min_y = min(y for _, y in foreground)
    max_y = max(y for _, y in foreground)
    # These intentionally broad bounds reject full-screen corruption while
    # allowing a renderer implementation to retain legal raster edge choices.
    if not (160 <= min_x <= 280 and 360 <= max_x <= 480):
        errors.append("P4_CPU_FOREGROUND_X_ENVELOPE")
    if not (MENU_HEIGHT <= min_y <= 120 and 180 <= max_y <= 320):
        errors.append("P4_CPU_FOREGROUND_Y_ENVELOPE")
    if len(foreground) < 1000 or len(foreground) > 30000:
        errors.append("P4_CPU_FOREGROUND_AREA")
    if len(colors) < 4:
        errors.append("P4_CPU_FOREGROUND_COLORS")
    for y in range(MENU_HEIGHT + 1, MENU_HEIGHT + VIEWPORT_HEIGHT, 2):
        if any(pixel != (0, 0, 0) for pixel in rows[y]):
            errors.append("P4_CPU_200_LINE_SEPARATOR")
            break
    return {
        "bbox": [min_x, min_y, max_x, max_y],
        "pixels": len(foreground),
        "colors": len(colors),
    }, errors


def validate_capture(directory: Path, prefix: str) -> tuple[dict[str, str], bytes | None, dict[str, object], list[str]]:
    errors: list[str] = []
    registers: dict[str, str] = {}
    screen_bytes: bytes | None = None
    shape: dict[str, object] = {"bbox": None, "pixels": 0, "colors": 0}
    registers_path = directory / REGISTERS_NAME
    screen_path = directory / SCREEN_NAME
    events_path = directory / EVENTS_NAME
    try:
        registers = read_registers(registers_path)
    except (OSError, UnicodeError, ValueError):
        errors.append(prefix + "_REGISTERS")
    for key, expected in EXPECTED_REGISTERS.items():
        if registers.get(key) != expected:
            errors.append(prefix + "_" + key.upper())
    if registers.get("bx") != EXPECTED_RAW_CHECKSUM:
        errors.append(prefix + "_RAW_CHECKSUM")
    try:
        flags = int(registers.get("flags", ""), 16)
    except ValueError:
        errors.append(prefix + "_FLAGS")
    else:
        if flags & 0x0400:
            errors.append(prefix + "_DIRECTION_FLAG")
        if not flags & 0x0200:
            errors.append(prefix + "_INTERRUPTS")
    try:
        events = events_path.read_text(encoding="utf-8")
    except (OSError, UnicodeError):
        errors.append(prefix + "_EVENTS")
    else:
        if "pc\t" not in events:
            errors.append(prefix + "_IDLE")
    try:
        screen_bytes = screen_path.read_bytes()
        width, height, rows = read_bmp_pixels(screen_path)
    except (OSError, ValueError):
        errors.append(prefix + "_SCREEN")
    else:
        if (width, height) != (WIDTH, HEIGHT):
            errors.append(prefix + "_SCREEN_SIZE")
        else:
            shape, shape_errors = visible_shape(rows)
            errors.extend(prefix + "_" + error for error in shape_errors)
    return registers, screen_bytes, shape, errors


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("first", type=Path)
    parser.add_argument("second", type=Path)
    args = parser.parse_args()
    first_registers, first_screen, first_shape, errors = validate_capture(args.first, "P4_CPU_FIRST")
    second_registers, second_screen, second_shape, second_errors = validate_capture(args.second, "P4_CPU_SECOND")
    errors.extend(second_errors)
    if first_registers.get("bx") != second_registers.get("bx"):
        errors.append("P4_CPU_RAW_CHECKSUM_REPEAT")
    if first_screen is None or second_screen is None or first_screen != second_screen:
        errors.append("P4_CPU_COMPOSED_CAPTURE_REPEAT")
    result = {
        "checksum": first_registers.get("bx"),
        "errors": errors,
        "first_shape": first_shape,
        "schema": "glass-orbit-p4-cpu-v1",
        "second_shape": second_shape,
        "status": "PASS" if not errors else "FAIL",
    }
    print(json.dumps(result, sort_keys=True))
    return 0 if result["status"] == "PASS" else 1


if __name__ == "__main__":
    raise SystemExit(main())
