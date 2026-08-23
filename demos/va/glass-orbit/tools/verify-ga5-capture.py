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

"""Validate the GLASS ORBIT GA-5 SGP CLS proof against its CPU reference.

The guest reports success only after it has read every 16-bit GVRAM word from
the CPU aperture following SGP completion. This host checker independently
compares the 640-by-400 composed guest viewport against a fresh GA-2 CPU-fill
run. It does not infer SGP timing or real-PC-88VA conformance.
"""

import argparse
import json
import struct
from pathlib import Path
from typing import Optional


MENU_HEIGHT = 22
GUEST_WIDTH = 640
COMPOSITION_HEIGHT = 400
LOGICAL_HEIGHT = 200
SGP_EXPECTED_REGISTERS = {
    "schema": "vaeg-registers-v1",
    "ax": "4745",
    "bx": "7d00",
    "cs": "2000",
    "ds": "2000",
    "es": "2000",
    "ss": "2000",
    "sp": "f000",
    "ip": "0100",
}
CPU_EXPECTED_REGISTERS = dict(SGP_EXPECTED_REGISTERS, ax="4743")


def read_tsv(path: Path) -> dict[str, str]:
    values = {}
    for line in path.read_text(encoding="utf-8").splitlines():
        key, separator, value = line.partition("\t")
        if not separator or key in values:
            raise ValueError("GA5_CAPTURE_SCHEMA")
        values[key] = value
    return values


def read_bmp(path: Path) -> tuple[int, int, list[list[tuple[int, int, int]]]]:
    data = path.read_bytes()
    if len(data) < 54 or data[:2] != b"BM":
        raise ValueError("GA5_BMP_HEADER")
    offset = struct.unpack_from("<I", data, 10)[0]
    dib_size = struct.unpack_from("<I", data, 14)[0]
    if dib_size < 40 or len(data) < 14 + dib_size:
        raise ValueError("GA5_BMP_DIB")
    width, signed_height = struct.unpack_from("<ii", data, 18)
    planes, bpp, compression = struct.unpack_from("<HHI", data, 26)
    if (width <= 0 or signed_height == 0 or planes != 1 or bpp not in (24, 32) or
            compression not in (0, 3)):
        raise ValueError("GA5_BMP_FORMAT")
    if compression == 3:
        if bpp != 32 or dib_size < 52:
            raise ValueError("GA5_BMP_BITFIELDS")
        masks = struct.unpack_from("<III", data, 54)
        if masks != (0x00FF0000, 0x0000FF00, 0x000000FF):
            raise ValueError("GA5_BMP_BITFIELDS")
    height = abs(signed_height)
    stride = ((width * bpp + 31) // 32) * 4
    if offset + stride * height > len(data):
        raise ValueError("GA5_BMP_TRUNCATED")
    bytes_per_pixel = bpp // 8
    rows = []
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


def black(pixel: tuple[int, int, int]) -> bool:
    return max(pixel) <= 4


def validate_capture(directory: Path, capture_name: str,
                     expected_registers: dict[str, str], prefix: str
                     ) -> tuple[list[str], Optional[list[list[tuple[int, int, int]]]], dict[str, str]]:
    errors = []
    values: dict[str, str] = {}
    registers_path = directory / (capture_name + ".registers.tsv")
    screen_path = directory / (capture_name + ".screen.bmp")
    events_path = directory / "events.tsv"
    if not registers_path.is_file():
        errors.append(prefix + "_REGISTERS_MISSING")
    else:
        try:
            values = read_tsv(registers_path)
        except (OSError, UnicodeError, ValueError):
            errors.append(prefix + "_CAPTURE_SCHEMA")
    for key, expected in expected_registers.items():
        if values.get(key) != expected:
            errors.append("%s_%s_MISMATCH" % (prefix, key.upper()))
    try:
        flags = int(values.get("flags", ""), 16)
    except ValueError:
        errors.append(prefix + "_FLAGS_INVALID")
    else:
        if flags & 0x0400:
            errors.append(prefix + "_DIRECTION_FLAG_SET")
        if not flags & 0x0200:
            errors.append(prefix + "_INTERRUPTS_DISABLED")
    if not events_path.is_file():
        errors.append(prefix + "_EVENTS_MISSING")
    elif not any(line.startswith("pc\t") for line in events_path.read_text(encoding="utf-8").splitlines()):
        errors.append(prefix + "_IDLE_NOT_REACHED")
    rows = None
    if not screen_path.is_file() or screen_path.stat().st_size == 0:
        errors.append(prefix + "_SCREEN_MISSING")
    else:
        try:
            width, height, rows = read_bmp(screen_path)
        except (OSError, ValueError) as error:
            errors.append(prefix + "_" + str(error))
        else:
            if width != GUEST_WIDTH or height < MENU_HEIGHT + COMPOSITION_HEIGHT:
                errors.append(prefix + "_VIEWPORT_DIMENSIONS")
            else:
                for logical_y in range(LOGICAL_HEIGHT):
                    visible_y = MENU_HEIGHT + logical_y * 2
                    separator_y = visible_y + 1
                    for x in range(GUEST_WIDTH):
                        if color_class(rows[visible_y][x]) != 5:
                            errors.append(prefix + "_VISIBLE_PATTERN_MISMATCH")
                            break
                        if not black(rows[separator_y][x]):
                            errors.append(prefix + "_200_LINE_BOUNDS")
                            break
                    if errors and errors[-1] in (
                            prefix + "_VISIBLE_PATTERN_MISMATCH",
                            prefix + "_200_LINE_BOUNDS"):
                        break
    return errors, rows, values


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("cpu_capture", type=Path)
    parser.add_argument("sgp_capture", type=Path)
    args = parser.parse_args()
    errors, ga5_rows, values = validate_capture(
        args.sgp_capture, "ga5-idle", SGP_EXPECTED_REGISTERS, "GA5")
    cpu_errors, cpu_rows, cpu_values = validate_capture(
        args.cpu_capture, "ga5-cpu-idle", CPU_EXPECTED_REGISTERS, "GA5_CPU")
    errors.extend(cpu_errors)
    viewport_identical = False
    if ga5_rows is not None and cpu_rows is not None and not cpu_errors:
        ga5_viewport = ga5_rows[MENU_HEIGHT:MENU_HEIGHT + COMPOSITION_HEIGHT]
        cpu_viewport = cpu_rows[MENU_HEIGHT:MENU_HEIGHT + COMPOSITION_HEIGHT]
        viewport_identical = ga5_viewport == cpu_viewport
        if not viewport_identical:
            errors.append("GA5_CPU_SGP_VIEWPORT_MISMATCH")
    outcome = {
        "cpu_sgp_viewport_identical": viewport_identical,
        "errors": errors,
        "schema": "glass-orbit-ga5-v1",
        "status": "PASS" if not errors else "FAIL",
        "cpu_reference_words": cpu_values.get("bx"),
        "verified_words": values.get("bx"),
    }
    print(json.dumps(outcome, sort_keys=True))
    return 0 if outcome["status"] == "PASS" else 1


if __name__ == "__main__":
    raise SystemExit(main())
