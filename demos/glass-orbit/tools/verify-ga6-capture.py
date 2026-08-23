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

"""Validate the two documented GLASS ORBIT GA-6 source-window selections.

Each capture is a fresh boot of a deterministic GA-6 page-select variant.
The guest has first cleared both 640-by-200 source regions through SGP and
then selected its requested Y=0 or Y=200 source window through Graphics BIOS
$RollTo during vertical blank.  This checker verifies the resulting composed
viewport for whole-page identity, not merely a changed pixel count.  It makes
no claim about real PC-88VA page-flip timing or hardware conformance.
"""

import argparse
import json
import struct
from pathlib import Path


MENU_HEIGHT = 22
GUEST_WIDTH = 640
COMPOSITION_HEIGHT = 400
LOGICAL_HEIGHT = 200
EXPECTED_COMMON = {
    "schema": "vaeg-registers-v1",
    "ax": "4746",
    "cs": "2000",
    "ds": "2000",
    "es": "2000",
    "ss": "2000",
    "sp": "f000",
    "ip": "0100",
}
CAPTURES = (
    ("page_a", "ga6-page-a", "0000", (0, 0, 255)),
    ("page_b", "ga6-page-b", "0001", (255, 0, 0)),
)


def read_tsv(path: Path) -> dict[str, str]:
    values: dict[str, str] = {}
    for line in path.read_text(encoding="utf-8").splitlines():
        key, separator, value = line.partition("\t")
        if not separator or key in values:
            raise ValueError("GA6_CAPTURE_SCHEMA")
        values[key] = value
    return values


def read_bmp(path: Path) -> tuple[int, int, list[list[tuple[int, int, int]]]]:
    data = path.read_bytes()
    if len(data) < 54 or data[:2] != b"BM":
        raise ValueError("GA6_BMP_HEADER")
    offset = struct.unpack_from("<I", data, 10)[0]
    dib_size = struct.unpack_from("<I", data, 14)[0]
    if dib_size < 40 or len(data) < 14 + dib_size:
        raise ValueError("GA6_BMP_DIB")
    width, signed_height = struct.unpack_from("<ii", data, 18)
    planes, bpp, compression = struct.unpack_from("<HHI", data, 26)
    if (width <= 0 or signed_height == 0 or planes != 1 or bpp not in (24, 32)
            or compression not in (0, 3)):
        raise ValueError("GA6_BMP_FORMAT")
    if compression == 3:
        if bpp != 32 or dib_size < 52:
            raise ValueError("GA6_BMP_BITFIELDS")
        masks = struct.unpack_from("<III", data, 54)
        if masks != (0x00FF0000, 0x0000FF00, 0x000000FF):
            raise ValueError("GA6_BMP_BITFIELDS")
    height = abs(signed_height)
    stride = ((width * bpp + 31) // 32) * 4
    if offset + stride * height > len(data):
        raise ValueError("GA6_BMP_TRUNCATED")
    bytes_per_pixel = bpp // 8
    rows: list[list[tuple[int, int, int]]] = []
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


def black(pixel: tuple[int, int, int]) -> bool:
    return max(pixel) <= 4


def validate_one(directory: Path, case_id: str, capture_id: str,
                 expected_page: str, expected_colour: tuple[int, int, int]) -> dict:
    errors: list[str] = []
    registers_path = directory / (capture_id + ".registers.tsv")
    screen_path = directory / (capture_id + ".screen.bmp")
    events_path = directory / "events.tsv"
    values: dict[str, str] = {}
    if not registers_path.is_file():
        errors.append("GA6_%s_REGISTERS_MISSING" % case_id.upper())
    else:
        try:
            values = read_tsv(registers_path)
        except (OSError, UnicodeError, ValueError):
            errors.append("GA6_%s_CAPTURE_SCHEMA" % case_id.upper())
    expected_registers = dict(EXPECTED_COMMON, bx=expected_page)
    for key, expected in expected_registers.items():
        if values.get(key) != expected:
            errors.append("GA6_%s_%s_MISMATCH" % (case_id.upper(), key.upper()))
    try:
        flags = int(values.get("flags", ""), 16)
    except ValueError:
        errors.append("GA6_%s_FLAGS_INVALID" % case_id.upper())
    else:
        if flags & 0x0400:
            errors.append("GA6_%s_DIRECTION_FLAG_SET" % case_id.upper())
        if not flags & 0x0200:
            errors.append("GA6_%s_INTERRUPTS_DISABLED" % case_id.upper())
    if not events_path.is_file():
        errors.append("GA6_%s_EVENTS_MISSING" % case_id.upper())
    elif not any(line.startswith("pc\t")
                 for line in events_path.read_text(encoding="utf-8").splitlines()):
        errors.append("GA6_%s_CHECKPOINT_NOT_REACHED" % case_id.upper())

    image_size = None
    sample = None
    matching_pixels = 0
    separator_pixels = 0
    if not screen_path.is_file() or screen_path.stat().st_size == 0:
        errors.append("GA6_%s_SCREEN_MISSING" % case_id.upper())
    else:
        try:
            width, height, rows = read_bmp(screen_path)
        except (OSError, ValueError) as error:
            errors.append("GA6_%s_%s" % (case_id.upper(), error))
        else:
            image_size = [width, height]
            if width != GUEST_WIDTH or height < MENU_HEIGHT + COMPOSITION_HEIGHT:
                errors.append("GA6_%s_VIEWPORT_DIMENSIONS" % case_id.upper())
            else:
                sample = rows[MENU_HEIGHT][GUEST_WIDTH // 2]
                if sample != expected_colour:
                    errors.append("GA6_%s_PAGE_COLOUR" % case_id.upper())
                for logical_y in range(LOGICAL_HEIGHT):
                    visible_y = MENU_HEIGHT + logical_y * 2
                    separator_y = visible_y + 1
                    for x in range(GUEST_WIDTH):
                        if rows[visible_y][x] == expected_colour:
                            matching_pixels += 1
                        if black(rows[separator_y][x]):
                            separator_pixels += 1
                expected_pixels = GUEST_WIDTH * LOGICAL_HEIGHT
                if matching_pixels != expected_pixels:
                    errors.append("GA6_%s_VISIBLE_PAGE_PARTIAL_OR_STALE" % case_id.upper())
                if separator_pixels != expected_pixels:
                    errors.append("GA6_%s_200_LINE_BOUNDS" % case_id.upper())
    return {
        "case": case_id,
        "errors": errors,
        "image_size": image_size,
        "page": values.get("bx"),
        "sample_rgb": list(sample) if sample is not None else None,
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("page_a_directory", type=Path)
    parser.add_argument("page_b_directory", type=Path)
    args = parser.parse_args()
    directories = {"page_a": args.page_a_directory, "page_b": args.page_b_directory}
    results = []
    for case_id, capture_id, expected_page, expected_colour in CAPTURES:
        results.append(validate_one(
            directories[case_id], case_id, capture_id, expected_page, expected_colour))
    errors = []
    for result in results:
        errors.extend(result["errors"])
    if results[0]["sample_rgb"] == results[1]["sample_rgb"]:
        errors.append("GA6_PAGE_IDENTITY_NOT_DISTINCT")
    outcome = {
        "captures": results,
        "errors": errors,
        "schema": "glass-orbit-ga6-v1",
        "status": "PASS" if not errors else "FAIL",
    }
    print(json.dumps(outcome, sort_keys=True))
    return 0 if outcome["status"] == "PASS" else 1


if __name__ == "__main__":
    raise SystemExit(main())
