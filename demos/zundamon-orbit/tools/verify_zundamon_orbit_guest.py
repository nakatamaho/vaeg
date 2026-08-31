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

"""Fail-closed oracle for the combined M98l BMS-to-G1 guest proof."""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import struct
import sys
from dataclasses import asdict
from pathlib import Path


TOOLS_DIRECTORY = Path(__file__).resolve().parent
if str(TOOLS_DIRECTORY) not in sys.path:
    sys.path.insert(0, str(TOOLS_DIRECTORY))
import inspect_zundamon_orbit_atlas as atlas_format  # noqa: E402


PHASE_PREFIXES = ("m98l-probe", "m98l-load", "m98l-transfer")
SETTLED_PREFIXES = ("m98l-settled-a", "m98l-settled-b")
ALL_PREFIXES = PHASE_PREFIXES + SETTLED_PREFIXES
GVRAM_SIZE = 0x40000
G0_SIZE = 320 * 200
G1_OFFSET = 0x20000
G1_WIDTH = 320
G1_HEIGHT = 400
STAGING_BYTES = 4096
BMS_WINDOW_BASE = 0x080000
BMS_WINDOW_END = 0x09FFFF
G1_PAGE_BASE = 0x220000
TRACE_SOURCE = re.compile(
    r"^SGP_SCAN: SET_SOURCE addr=([0-9a-fA-F]+) dot=(\d+) mode=(\d+) "
    r"width=(\d+) height=(\d+) fbw=(\d+)$"
)
TRACE_DESTINATION = re.compile(
    r"^SGP_SCAN: SET_DEST addr=([0-9a-fA-F]+) dot=(\d+) mode=(\d+) "
    r"width=(\d+) height=(\d+) fbw=(\d+)$"
)


def add_error(errors: list[str], code: str) -> None:
    if code not in errors:
        errors.append(code)


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def read_tsv(path: Path) -> dict[str, str]:
    values: dict[str, str] = {}
    for line in path.read_text(encoding="utf-8").splitlines():
        key, separator, value = line.partition("\t")
        if not separator or not key or key in values:
            raise ValueError("M98L_REGISTERS_SCHEMA")
        values[key] = value
    return values


def expected_registers(
    header: atlas_format.Header,
    descriptor: atlas_format.Descriptor,
) -> dict[str, dict[str, str]]:
    destination_x = (G1_WIDTH - descriptor.width) // 2
    destination_y = (200 - descriptor.height) // 2
    source = BMS_WINDOW_BASE + descriptor.bank_offset
    chunks = (header.payload_bytes + STAGING_BYTES - 1) // STAGING_BYTES
    return {
        "m98l-probe": {
            "ax": "98a1", "bx": "01d0", "cx": "0080", "dx": "0002",
            "si": "a55a", "di": "0081", "bp": "0000", "ip": "3000",
        },
        "m98l-load": {
            "ax": "98b1", "bx": f"{chunks:04x}",
            "cx": f"{header.payload_bytes & 0xffff:04x}",
            "dx": f"{header.payload_bytes >> 16:04x}", "si": "1000",
            "di": f"{header.file_size & 0xffff:04x}",
            "bp": f"{header.file_size >> 16:04x}", "ip": "3010",
        },
        "m98l-transfer": {
            "ax": "98c1", "bx": f"{destination_x:04x}",
            "cx": f"{destination_y:04x}", "dx": "0105",
            "si": f"{source & 0xffff:04x}",
            "di": f"{source >> 16:04x}", "bp": "0101", "ip": "3020",
        },
        "m98l-settled-a": {
            "ax": "984c", "bx": f"{descriptor.width:04x}",
            "cx": f"{descriptor.height:04x}", "dx": f"{descriptor.pitch:04x}",
            "si": "0101", "di": f"{destination_y:04x}",
            "bp": f"{destination_x:04x}", "ip": "3030",
        },
        "m98l-settled-b": {
            "ax": "984c", "bx": f"{descriptor.width:04x}",
            "cx": f"{descriptor.height:04x}", "dx": f"{descriptor.pitch:04x}",
            "si": "0101", "di": f"{destination_y:04x}",
            "bp": f"{destination_x:04x}", "ip": "3030",
        },
    }


def check_registers(
    directory: Path,
    expected: dict[str, dict[str, str]],
    errors: list[str],
) -> list[dict[str, str]]:
    captures: list[dict[str, str]] = []
    for prefix in ALL_PREFIXES:
        try:
            values = read_tsv(directory / f"{prefix}.registers.tsv")
        except (OSError, UnicodeError, ValueError):
            add_error(errors, "M98L_REGISTERS_SCHEMA")
            continue
        captures.append(values)
        required = {"schema": "vaeg-registers-v1", "cs": "3000", "ds": "3000"}
        required.update(expected[prefix])
        if any(values.get(key) != value for key, value in required.items()):
            add_error(errors, f"M98L_{prefix.split('-')[1].upper()}_SIGNATURE")
        try:
            flags = int(values.get("flags", ""), 16)
        except ValueError:
            add_error(errors, "M98L_FLAGS")
        else:
            if (flags & 0x0400) or not (flags & 0x0200):
                add_error(errors, "M98L_FLAGS")
    return captures


def parse_events(directory: Path, errors: list[str]) -> list[int]:
    try:
        lines = (directory / "events.tsv").read_text(encoding="utf-8").splitlines()
    except (OSError, UnicodeError):
        add_error(errors, "M98L_EVENTS_SCHEMA")
        return []
    if not lines or lines[0] != "event\tframe\tid\tvalue":
        add_error(errors, "M98L_EVENTS_SCHEMA")
        return []
    rows = [line.split("\t") for line in lines[1:]]
    if any(len(row) != 4 for row in rows):
        add_error(errors, "M98L_EVENTS_SCHEMA")
        return []
    if any(row[0] == "frame-limit" for row in rows):
        add_error(errors, "M98L_EVENTS_TIMEOUT")
    pc_frames = [int(row[1]) for row in rows if row[0] == "pc" and row[1].isdigit()]
    captures = [(row[1], row[2]) for row in rows if row[0] == "capture"]
    expected_captures = list(zip((str(frame) for frame in pc_frames), ALL_PREFIXES))
    frames_regress = any(
        right < left for left, right in zip(pc_frames, pc_frames[1:])
    )
    settled_not_consecutive = (
        len(pc_frames) == len(ALL_PREFIXES)
        and pc_frames[-1] != pc_frames[-2] + 1
    )
    if (len(pc_frames) != len(ALL_PREFIXES) or captures != expected_captures
            or frames_regress or settled_not_consecutive):
        add_error(errors, "M98L_EVENTS_SEQUENCE")
    return pc_frames


def expected_g0() -> bytes:
    row_a = bytes((0x24,) * 8 + (0x49,) * 8)
    row_b = bytes((0x49,) * 8 + (0x24,) * 8)
    output = bytearray()
    for y in range(200):
        for tile in range(20):
            output.extend(row_a if ((tile + y // 16) & 1) == 0 else row_b)
    return bytes(output)


def expected_g1(
    atlas: bytes,
    descriptor: atlas_format.Descriptor,
) -> tuple[bytes, int, int]:
    destination_x = (G1_WIDTH - descriptor.width) // 2
    destination_y = (200 - descriptor.height) // 2
    frame = atlas[
        descriptor.file_offset:descriptor.file_offset + descriptor.payload_bytes
    ]
    surface = bytearray(G1_WIDTH * G1_HEIGHT)
    for y in range(descriptor.height):
        row = frame[y * descriptor.pitch:y * descriptor.pitch + descriptor.width]
        start = (destination_y + y) * G1_WIDTH + destination_x
        surface[start:start + descriptor.width] = row
    return bytes(surface), destination_x, destination_y


def nonzero_bbox(surface: bytes) -> list[int] | None:
    positions = [(index % G1_WIDTH, index // G1_WIDTH)
                 for index, value in enumerate(surface) if value]
    if not positions:
        return None
    return [min(x for x, _ in positions), min(y for _, y in positions),
            max(x for x, _ in positions), max(y for _, y in positions)]


def check_gvram(
    directory: Path,
    expected_surface: bytes,
    errors: list[str],
) -> tuple[list[bytes], int, list[int] | None]:
    images: list[bytes] = []
    for prefix in SETTLED_PREFIXES:
        try:
            raw = (directory / f"{prefix}.gvram.bin").read_bytes()
        except OSError:
            add_error(errors, "M98L_GVRAM_MISSING")
            continue
        if len(raw) != GVRAM_SIZE:
            add_error(errors, "M98L_GVRAM_SIZE")
            continue
        images.append(raw)
    if len(images) == 2 and images[0] != images[1]:
        add_error(errors, "M98L_GVRAM_UNSTABLE")
    if not images:
        return images, 0, None
    raw = images[0]
    if raw[:G0_SIZE] != expected_g0():
        add_error(errors, "M98L_G0_CONTENT")
    g1 = raw[G1_OFFSET:G1_OFFSET + len(expected_surface)]
    if g1 != expected_surface:
        add_error(errors, "M98L_G1_CONTENT")
    return images, sum(value != 0 for value in g1), nonzero_bbox(g1)


def bmp_rgb_nonblack(path: Path) -> bool:
    data = path.read_bytes()
    if len(data) < 54 or data[:2] != b"BM":
        raise ValueError("M98L_SCREEN_FORMAT")
    pixel_offset = struct.unpack_from("<I", data, 10)[0]
    dib_size = struct.unpack_from("<I", data, 14)[0]
    width = struct.unpack_from("<i", data, 18)[0]
    height = struct.unpack_from("<i", data, 22)[0]
    planes, bits = struct.unpack_from("<HH", data, 26)
    if dib_size < 40 or width < 320 or abs(height) < 200 or planes != 1 or bits not in (24, 32):
        raise ValueError("M98L_SCREEN_FORMAT")
    pixel_size = bits // 8
    row_size = ((width * bits + 31) // 32) * 4
    needed = pixel_offset + row_size * abs(height)
    if needed > len(data):
        raise ValueError("M98L_SCREEN_FORMAT")
    for row in range(abs(height)):
        base = pixel_offset + row * row_size
        for column in range(width):
            offset = base + column * pixel_size
            if data[offset] or data[offset + 1] or data[offset + 2]:
                return True
    return False


def check_screens(directory: Path, errors: list[str]) -> list[bytes]:
    screens: list[bytes] = []
    nonblack: list[bool] = []
    for prefix in SETTLED_PREFIXES:
        path = directory / f"{prefix}.screen.bmp"
        try:
            screens.append(path.read_bytes())
            nonblack.append(bmp_rgb_nonblack(path))
        except OSError:
            add_error(errors, "M98L_SCREEN_MISSING")
        except ValueError:
            add_error(errors, "M98L_SCREEN_FORMAT")
    if len(screens) == 2 and screens[0] != screens[1]:
        add_error(errors, "M98L_SCREEN_UNSTABLE")
    if nonblack and not all(nonblack):
        add_error(errors, "M98L_SCREEN_BLACK")
    return screens


def check_trace(
    trace: Path,
    descriptor: atlas_format.Descriptor,
    errors: list[str],
) -> dict[str, object]:
    expected_source = BMS_WINDOW_BASE + descriptor.bank_offset
    destination_x = (G1_WIDTH - descriptor.width) // 2
    destination_y = (200 - descriptor.height) // 2
    expected_destination = G1_PAGE_BASE + destination_y * G1_WIDTH + destination_x
    try:
        lines = trace.read_text(encoding="utf-8", errors="replace").splitlines()
    except OSError:
        add_error(errors, "M98L_TRACE_MISSING")
        return {}
    sources = [match for line in lines if (match := TRACE_SOURCE.match(line))]
    destinations = [match for line in lines if (match := TRACE_DESTINATION.match(line))]
    bms_sources = [match for match in sources
                   if BMS_WINDOW_BASE <= int(match.group(1), 16) <= BMS_WINDOW_END]
    expected_source_rows = [match for match in bms_sources if (
        int(match.group(1), 16), int(match.group(2)), int(match.group(3)),
        int(match.group(4)), int(match.group(5)), int(match.group(6)),
    ) == (expected_source, 0, 2, descriptor.width,
          descriptor.height, descriptor.pitch)]
    expected_destination_rows = [match for match in destinations if (
        int(match.group(1), 16), int(match.group(2)), int(match.group(3)),
        int(match.group(4)), int(match.group(5)), int(match.group(6)),
    ) == (expected_destination, 0, 2, descriptor.width,
          descriptor.height, G1_WIDTH)]
    if len(bms_sources) != 1 or len(expected_source_rows) != 1:
        add_error(errors, "M98L_TRACE_BMS_SOURCE")
    if len(expected_destination_rows) != 1:
        add_error(errors, "M98L_TRACE_G1_DESTINATION")
    return {
        "bms_source_count": len(bms_sources),
        "destination_address": expected_destination,
        "expected_destination_matches": len(expected_destination_rows),
        "expected_source_matches": len(expected_source_rows),
        "source_address": expected_source,
    }


def listing_symbol_offset(path: Path, symbol: str) -> int | None:
    try:
        lines = path.read_text(encoding="utf-8", errors="replace").splitlines()
    except OSError:
        return None
    for index, line in enumerate(lines):
        if line.rstrip().endswith(f"{symbol}:"):
            for following in lines[index + 1:index + 4]:
                match = re.search(r"\s([0-9A-Fa-f]{8})\s", following)
                if match:
                    return int(match.group(1), 16) + 0x100
    return None


def check_source(source_path: Path, listing: Path, errors: list[str]) -> int | None:
    try:
        source = source_path.read_text(encoding="utf-8")
    except (OSError, UnicodeError):
        add_error(errors, "M98L_SOURCE_ASSEMBLY")
        return None
    lowered = source.lower()
    if "incbin" in lowered or "%include" in lowered:
        add_error(errors, "M98L_SOURCE_EMBEDDED_ATLAS")
    if source.count("mov ax, SGP_COMMAND_BITBLT") != 1:
        add_error(errors, "M98L_SOURCE_BITBLT_COUNT")
    if source.count("call run_sgp_command_list") != 1:
        add_error(errors, "M98L_SOURCE_SUBMISSION_COUNT")
    required = (
        "%define PORT_BMS_SELECTOR       0x01d0",
        "%define BMS_FIRST_SELECTOR      1",
        "%define STAGING_BYTES           4096",
        "%define SGP_BITBLT_COPY_XPAR    0x0105",
        "call poison_staging_buffer",
        "call verify_bms_payload_crc",
        "call select_ordinary_mapping",
    )
    if any(text not in source for text in required):
        add_error(errors, "M98L_SOURCE_CONTRACT")
    try:
        listing_text = listing.read_text(encoding="utf-8", errors="replace")
    except OSError:
        listing_text = ""
    if re.search(r"\b0F8[0-9A-Fa-f]", listing_text):
        add_error(errors, "M98L_VA2_INSTRUCTION_SET")
    offset = listing_symbol_offset(listing, "staging_buffer")
    if offset is None:
        add_error(errors, "M98L_LISTING_STAGING")
    return offset


def artifact_records(directory: Path) -> dict[str, dict[str, object]]:
    names = [
        "ZUNDORB.COM", "ZUNDORB.LST", "ZUNDORB.BIN",
        "zundamon-orbit-m98l-pristine.d88", "zundamon-orbit-m98l.d88",
        "events.tsv", "sgp-trace.log", "vaeg.stdout.log",
    ]
    for prefix in ALL_PREFIXES:
        names.append(f"{prefix}.registers.tsv")
    for prefix in SETTLED_PREFIXES:
        names.extend((f"{prefix}.gvram.bin", f"{prefix}.screen.bmp"))
    records: dict[str, dict[str, object]] = {}
    for name in names:
        path = directory / name
        if path.is_file():
            data = path.read_bytes()
            records[name] = {"sha256": sha256(data), "size": len(data)}
    return records


def verify(
    directory: Path,
    source: Path,
    atlas_path: Path,
    trace: Path,
) -> dict[str, object]:
    errors: list[str] = []
    try:
        atlas = atlas_path.read_bytes()
        header, descriptors = atlas_format.inspect_bytes(atlas)
    except (OSError, atlas_format.AtlasError):
        return {
            "errors": ["M98L_ATLAS_FORMAT"],
            "schema": "zundamon-orbit-m98l-oracle-v1",
            "status": "FAIL",
        }
    descriptor = descriptors[-1]
    expected_surface, destination_x, destination_y = expected_g1(atlas, descriptor)
    registers = check_registers(directory, expected_registers(header, descriptor), errors)
    pc_frames = parse_events(directory, errors)
    gvram, nonzero_count, bbox = check_gvram(directory, expected_surface, errors)
    screens = check_screens(directory, errors)
    trace_report = check_trace(trace, descriptor, errors)
    listing = directory / "ZUNDORB.LST"
    staging_offset = check_source(source, listing, errors)
    selected_frame = atlas[
        descriptor.file_offset:descriptor.file_offset + descriptor.payload_bytes
    ]
    expected_nonzero = sum(
        selected_frame[row * descriptor.pitch + column] != 0
        for row in range(descriptor.height)
        for column in range(descriptor.width)
    )
    if nonzero_count != expected_nonzero and "M98L_G1_CONTENT" not in errors:
        add_error(errors, "M98L_G1_NONZERO_COUNT")
    chunks = (header.payload_bytes + STAGING_BYTES - 1) // STAGING_BYTES
    result: dict[str, object] = {
        "artifacts": artifact_records(directory),
        "atlas": {
            "descriptor": asdict(descriptor),
            "file_class": "generated-public-fixture",
            "file_size": len(atlas),
            "first_bank_value": header.first_bank_value,
            "indexed_bits": 8,
            "payload_bytes": header.payload_bytes,
            "required_bank_count": header.required_bank_count,
            "selected_cell": ATLAS_SELECTED_CELL,
            "selected_cell_sha256": sha256(selected_frame),
            "sha256": sha256(atlas),
            "transparent_index": 0,
        },
        "bms": {
            "alias_boundary": "selector-129-open-bus-no-wrap",
            "bank_count": 128,
            "bank_size": 0x20000,
            "base_port": "01d0h",
            "capacity_bytes": 16 * 1024 * 1024,
            "ordinary_guard": "5aa5",
            "ordinary_selector": 0,
            "tested_selectors": [1, 2, 128, 129],
            "window": "80000h-9ffffh",
        },
        "errors": errors,
        "g1_nonzero_bbox": bbox,
        "g1_nonzero_count": nonzero_count,
        "gvram_stable": len(gvram) == 2 and gvram[0] == gvram[1],
        "mode": {"height": 200, "layers": ["G0", "G1"],
                 "pixel_bits": 8, "width": 320},
        "pc_frames": pc_frames,
        "register_captures": len(registers),
        "schema": "zundamon-orbit-m98l-oracle-v1",
        "screen_stable": len(screens) == 2 and screens[0] == screens[1],
        "sgp": {
            "bitblt_mode": "0105h",
            "destination": [destination_x, destination_y],
            "submission_count": 1,
            "trace": trace_report,
        },
        "staging": {
            "address": (f"3000:{staging_offset:04x}" if staging_offset is not None else None),
            "chunk_count": chunks,
            "maximum_bytes": STAGING_BYTES,
            "poison": "a5h",
        },
        "status": "PASS" if not errors else "FAIL",
    }
    return result


ATLAS_SELECTED_CELL = "level-30"


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("directory", type=Path)
    parser.add_argument("--source", type=Path)
    parser.add_argument("--atlas", type=Path, required=True)
    parser.add_argument("--trace", type=Path, required=True)
    parser.add_argument("--report", type=Path)
    args = parser.parse_args()
    source = args.source
    if source is None:
        source = Path(__file__).resolve().parents[1] / "256" / "zundamon_orbit_256.asm"
    result = verify(args.directory, source, args.atlas, args.trace)
    encoded = json.dumps(result, indent=2, sort_keys=True) + "\n"
    if args.report is not None:
        if args.report.parent != args.directory or args.report.exists():
            raise SystemExit("M98L_REPORT_PATH")
        args.report.write_text(encoded, encoding="utf-8")
    print(json.dumps(result, sort_keys=True))
    return 0 if result["status"] == "PASS" else 1


if __name__ == "__main__":
    raise SystemExit(main())
