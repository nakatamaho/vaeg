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

"""Fail-closed oracle for the M98k guest capture."""

from __future__ import annotations

import argparse
import hashlib
import json
import struct
from pathlib import Path


PREFIXES = ("m98k-settled-a", "m98k-settled-b")
GVRAM_SIZE = 0x40000
G0_SIZE = 320 * 200
G1_OFFSET = 0x20000
G1_WIDTH = 320
G1_HEIGHT = 400
MARKER_X = 152
MARKER_Y = 92
MARKER_WIDTH = 16
MARKER_HEIGHT = 16
MARKER_NONZERO_COUNT = 90


def marker_rows() -> tuple[bytes, ...]:
    rows: list[bytes] = []
    for y in range(MARKER_HEIGHT):
        row = [0] * MARKER_WIDTH
        for x in range(MARKER_WIDTH):
            if x in (0, MARKER_WIDTH - 1) or y in (0, MARKER_HEIGHT - 1):
                row[x] = 0x1C
        row[y] = 0xE0
        if 4 <= y <= 7:
            for x in range(10, 14):
                row[x] = 0x03
        rows.append(bytes(row))
    return tuple(rows)


EXPECTED_MARKER = marker_rows()


def expected_g0() -> bytes:
    row_a = bytes((0x24,) * 8 + (0x49,) * 8)
    row_b = bytes((0x49,) * 8 + (0x24,) * 8)
    output = bytearray()
    for y in range(200):
        for tile in range(20):
            output.extend(row_a if ((tile + y // 16) & 1) == 0 else row_b)
    return bytes(output)


def expected_g1() -> bytes:
    surface = bytearray(G1_WIDTH * G1_HEIGHT)
    for y, row in enumerate(EXPECTED_MARKER):
        start = (MARKER_Y + y) * G1_WIDTH + MARKER_X
        surface[start:start + MARKER_WIDTH] = row
    return bytes(surface)


EXPECTED_G0 = expected_g0()
EXPECTED_G1 = expected_g1()


def add_error(errors: list[str], code: str) -> None:
    if code not in errors:
        errors.append(code)


def read_tsv(path: Path) -> dict[str, str]:
    values: dict[str, str] = {}
    for line in path.read_text(encoding="utf-8").splitlines():
        key, separator, value = line.partition("\t")
        if not separator or not key or key in values:
            raise ValueError("M98K_REGISTERS_SCHEMA")
        values[key] = value
    return values


def check_registers(directory: Path, errors: list[str]) -> list[dict[str, str]]:
    captures: list[dict[str, str]] = []
    required = {
        "schema": "vaeg-registers-v1",
        "ax": "984b",
        "bx": "0140",
        "cx": "00c8",
        "dx": "0808",
        "si": "0101",
        "di": "005c",
        "bp": "0098",
        "cs": "3000",
        "ds": "3000",
        "es": "3000",
        "ip": "0800",
    }
    for prefix in PREFIXES:
        try:
            values = read_tsv(directory / f"{prefix}.registers.tsv")
        except (OSError, UnicodeError, ValueError):
            add_error(errors, "M98K_REGISTERS_SCHEMA")
            continue
        captures.append(values)
        if any(values.get(key) != value for key, value in required.items()):
            add_error(errors, "M98K_MODE_SIGNATURE")
        try:
            flags = int(values.get("flags", ""), 16)
        except ValueError:
            add_error(errors, "M98K_FLAGS")
        else:
            if (flags & 0x0400) or not (flags & 0x0200):
                add_error(errors, "M98K_FLAGS")
    return captures


def parse_events(directory: Path, errors: list[str]) -> list[int]:
    try:
        lines = (directory / "events.tsv").read_text(encoding="utf-8").splitlines()
    except (OSError, UnicodeError):
        add_error(errors, "M98K_EVENTS_SCHEMA")
        return []
    if not lines or lines[0] != "event\tframe\tid\tvalue":
        add_error(errors, "M98K_EVENTS_SCHEMA")
        return []
    rows = [line.split("\t") for line in lines[1:]]
    if any(len(row) != 4 for row in rows):
        add_error(errors, "M98K_EVENTS_SCHEMA")
        return []
    if any(row[0] == "frame-limit" for row in rows):
        add_error(errors, "M98K_EVENTS_TIMEOUT")
    pc_frames = [int(row[1]) for row in rows if row[0] == "pc" and row[1].isdigit()]
    captures = [(row[1], row[2]) for row in rows if row[0] == "capture"]
    expected_captures = [(str(pc_frames[0]), PREFIXES[0]),
                         (str(pc_frames[1]), PREFIXES[1])] if len(pc_frames) == 2 else []
    if len(pc_frames) != 2 or captures != expected_captures or pc_frames[1] <= pc_frames[0]:
        add_error(errors, "M98K_EVENTS_SEQUENCE")
    return pc_frames


def marker_occurrences(surface: bytes) -> list[tuple[int, int]]:
    occurrences: list[tuple[int, int]] = []
    first_row = EXPECTED_MARKER[0]
    for y in range(G1_HEIGHT - MARKER_HEIGHT + 1):
        for x in range(G1_WIDTH - MARKER_WIDTH + 1):
            start = y * G1_WIDTH + x
            if surface[start:start + MARKER_WIDTH] != first_row:
                continue
            if all(
                surface[(y + row) * G1_WIDTH + x:
                        (y + row) * G1_WIDTH + x + MARKER_WIDTH] == expected
                for row, expected in enumerate(EXPECTED_MARKER)
            ):
                occurrences.append((x, y))
    return occurrences


def check_gvram(
    directory: Path,
    errors: list[str],
) -> tuple[list[bytes], int, int, list[int] | None]:
    images: list[bytes] = []
    for prefix in PREFIXES:
        try:
            raw = (directory / f"{prefix}.gvram.bin").read_bytes()
        except OSError:
            add_error(errors, "M98K_GVRAM_MISSING")
            continue
        if len(raw) != GVRAM_SIZE:
            add_error(errors, "M98K_GVRAM_SIZE")
            continue
        images.append(raw)
    if len(images) == 2 and images[0] != images[1]:
        add_error(errors, "M98K_GVRAM_UNSTABLE")
    if not images:
        return images, 0, 0, None
    raw = images[0]
    if raw[:G0_SIZE] != EXPECTED_G0:
        add_error(errors, "M98K_G0_CONTENT")
    g1 = raw[G1_OFFSET:G1_OFFSET + len(EXPECTED_G1)]
    occurrences = marker_occurrences(g1)
    if (MARKER_X, MARKER_Y) not in occurrences:
        add_error(errors, "M98K_MARKER_LAYOUT")
    elif len(occurrences) != 1:
        add_error(errors, "M98K_MARKER_MULTIPLE")
    elif g1 != EXPECTED_G1:
        add_error(errors, "M98K_G1_BACKGROUND")
    nonzero = [(index % G1_WIDTH, index // G1_WIDTH)
               for index, value in enumerate(g1) if value]
    bbox = None
    if nonzero:
        bbox = [min(x for x, _ in nonzero), min(y for _, y in nonzero),
                max(x for x, _ in nonzero), max(y for _, y in nonzero)]
    return images, len(nonzero), len(occurrences), bbox


def bmp_rgb_nonblack(path: Path) -> bool:
    data = path.read_bytes()
    if len(data) < 54 or data[:2] != b"BM":
        raise ValueError("M98K_SCREEN_FORMAT")
    pixel_offset = struct.unpack_from("<I", data, 10)[0]
    dib_size = struct.unpack_from("<I", data, 14)[0]
    width = struct.unpack_from("<i", data, 18)[0]
    height = struct.unpack_from("<i", data, 22)[0]
    planes, bits = struct.unpack_from("<HH", data, 26)
    if dib_size < 40 or width < 320 or abs(height) < 200 or planes != 1 or bits not in (24, 32):
        raise ValueError("M98K_SCREEN_FORMAT")
    pixel_size = bits // 8
    row_size = ((width * bits + 31) // 32) * 4
    needed = pixel_offset + row_size * abs(height)
    if needed > len(data):
        raise ValueError("M98K_SCREEN_FORMAT")
    pixels = data[pixel_offset:needed]
    for row in range(abs(height)):
        base = row * row_size
        for column in range(width):
            offset = base + column * pixel_size
            if pixels[offset] or pixels[offset + 1] or pixels[offset + 2]:
                return True
    return False


def check_screens(directory: Path, errors: list[str]) -> list[bytes]:
    screens: list[bytes] = []
    nonblack: list[bool] = []
    for prefix in PREFIXES:
        path = directory / f"{prefix}.screen.bmp"
        try:
            screens.append(path.read_bytes())
            nonblack.append(bmp_rgb_nonblack(path))
        except OSError:
            add_error(errors, "M98K_SCREEN_MISSING")
        except ValueError:
            add_error(errors, "M98K_SCREEN_FORMAT")
    if len(screens) == 2 and screens[0] != screens[1]:
        add_error(errors, "M98K_SCREEN_UNSTABLE")
    if nonblack and not all(nonblack):
        add_error(errors, "M98K_SCREEN_BLACK")
    return screens


def check_source(path: Path, errors: list[str]) -> None:
    try:
        source = path.read_text(encoding="utf-8")
    except (OSError, UnicodeError):
        add_error(errors, "M98K_SOURCE_ASSEMBLY")
        return
    lowered = source.lower()
    forbidden = ("incbin", "%include", "bms", "ems", "xms")
    if any(token in lowered for token in forbidden):
        add_error(errors, "M98K_SOURCE_FORBIDDEN")
    if source.count("call run_sgp_command_list") != 1:
        add_error(errors, "M98K_SOURCE_SGP_SUBMISSION_COUNT")
    required = (
        "%define MODE_320X200_G0_G1      0xe00e",
        "%define PIXEL_SIZE_G0_G1_8BPP   0x0808",
        "%define SGP_BITBLT_COPY_XPAR    0x0105",
        "%define MARKER_WIDTH            16",
        "%define MARKER_HEIGHT           16",
        "%define MARKER_PITCH            16",
        "%define MARKER_X                152",
        "%define MARKER_Y                92",
    )
    if any(line not in source for line in required):
        add_error(errors, "M98K_SOURCE_CONTRACT")


def artifact_records(directory: Path) -> dict[str, dict[str, object]]:
    names = ["ZUNDORB.COM", "zundamon-orbit-m98k-pristine.d88",
             "zundamon-orbit-m98k.d88", "events.tsv"]
    for prefix in PREFIXES:
        names.extend((f"{prefix}.registers.tsv", f"{prefix}.gvram.bin",
                      f"{prefix}.screen.bmp"))
    records: dict[str, dict[str, object]] = {}
    for name in names:
        path = directory / name
        if path.is_file():
            data = path.read_bytes()
            records[name] = {
                "sha256": hashlib.sha256(data).hexdigest(),
                "size": len(data),
            }
    return records


def verify(directory: Path, source: Path) -> dict[str, object]:
    errors: list[str] = []
    registers = check_registers(directory, errors)
    pc_frames = parse_events(directory, errors)
    gvram, nonzero_count, occurrence_count, bbox = check_gvram(directory, errors)
    screens = check_screens(directory, errors)
    check_source(source, errors)
    result: dict[str, object] = {
        "artifacts": artifact_records(directory),
        "errors": errors,
        "g1_nonzero_bbox": bbox,
        "g1_nonzero_count": nonzero_count,
        "marker_nonzero_values": [0x03, 0x1C, 0xE0],
        "gvram_stable": len(gvram) == 2 and gvram[0] == gvram[1],
        "marker_occurrences": occurrence_count,
        "marker_transparent_count": MARKER_WIDTH * MARKER_HEIGHT - MARKER_NONZERO_COUNT,
        "mode": {"height": 200, "layers": ["G0", "G1"], "pixel_bits": 8,
                 "width": 320},
        "pc_frames": pc_frames,
        "register_captures": len(registers),
        "schema": "zundamon-orbit-m98k-oracle-v1",
        "screen_stable": len(screens) == 2 and screens[0] == screens[1],
        "sgp_submissions_completed": 1 if len(registers) == 2 and not errors else 0,
        "status": "PASS" if not errors else "FAIL",
    }
    marker_errors = {"M98K_MARKER_LAYOUT", "M98K_MARKER_MULTIPLE",
                     "M98K_G1_BACKGROUND"}
    if nonzero_count != MARKER_NONZERO_COUNT and not marker_errors.intersection(errors):
        add_error(errors, "M98K_MARKER_COUNT")
        result["status"] = "FAIL"
    return result


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("directory", type=Path)
    parser.add_argument("--source", type=Path)
    parser.add_argument("--report", type=Path)
    args = parser.parse_args()
    source = args.source
    if source is None:
        source = Path(__file__).resolve().parents[1] / "256" / "zundamon_orbit_256.asm"
    result = verify(args.directory, source)
    encoded = json.dumps(result, indent=2, sort_keys=True) + "\n"
    if args.report is not None:
        if args.report.parent != args.directory or args.report.exists():
            raise SystemExit("M98K_REPORT_PATH")
        args.report.write_text(encoded, encoding="utf-8")
    print(json.dumps(result, sort_keys=True))
    return 0 if result["status"] == "PASS" else 1


if __name__ == "__main__":
    raise SystemExit(main())
