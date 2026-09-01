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

"""Generate the public minimal-font FPS/ZUNDAMON G0 HUD tiles."""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path

FOREGROUND = 0xFF
BACKGROUND = 0x01
HUD_WIDTH = 66
HUD_HEIGHT = 16
CELL_WIDTH = 6
CELL_HEIGHT = 8
FPS_WIDTH = 18
FPS_HEIGHT = 8
FPS_FIELDS = ("60 ", "30 ", "20 ", "15 ", "12 ", "10 ", "8.6", "7.5")
COUNT_FIELDS = tuple(f"{count:>2}" for count in range(1, 17))
COUNT_WIDTH = 12

# Public, task-authored 5x7 glyphs.  No ROM or firmware font is consulted.
GLYPHS = {
    " ": (".....",) * 7,
    ".": (".....", ".....", ".....", ".....", ".....", ".##..", ".##.."),
    ":": (".....", ".##..", ".##..", ".....", ".##..", ".##..", "....."),
    "0": (".###.", "#...#", "#..##", "#.#.#", "##..#", "#...#", ".###."),
    "1": ("..#..", ".##..", "..#..", "..#..", "..#..", "..#..", ".###."),
    "2": (".###.", "#...#", "....#", "...#.", "..#..", ".#...", "#####"),
    "3": ("####.", "....#", "....#", ".###.", "....#", "....#", "####."),
    "4": ("...#.", "..##.", ".#.#.", "#..#.", "#####", "...#.", "...#."),
    "5": ("#####", "#....", "#....", "####.", "....#", "....#", "####."),
    "6": (".###.", "#....", "#....", "####.", "#...#", "#...#", ".###."),
    "7": ("#####", "....#", "...#.", "..#..", ".#...", ".#...", ".#..."),
    "8": (".###.", "#...#", "#...#", ".###.", "#...#", "#...#", ".###."),
    "9": (".###.", "#...#", "#...#", ".####", "....#", "....#", ".###."),
    "A": (".###.", "#...#", "#...#", "#####", "#...#", "#...#", "#...#"),
    "D": ("####.", "#...#", "#...#", "#...#", "#...#", "#...#", "####."),
    "F": ("#####", "#....", "#....", "####.", "#....", "#....", "#...."),
    "M": ("#...#", "##.##", "#.#.#", "#.#.#", "#...#", "#...#", "#...#"),
    "N": ("#...#", "##..#", "##..#", "#.#.#", "#..##", "#..##", "#...#"),
    "O": (".###.", "#...#", "#...#", "#...#", "#...#", "#...#", ".###."),
    "P": ("####.", "#...#", "#...#", "####.", "#....", "#....", "#...."),
    "S": (".####", "#....", "#....", ".###.", "....#", "....#", "####."),
    "U": ("#...#", "#...#", "#...#", "#...#", "#...#", "#...#", ".###."),
    "Z": ("#####", "....#", "...#.", "..#..", ".#...", "#....", "#####"),
}


def render_text(text: str) -> bytes:
    result = bytearray(len(text) * CELL_WIDTH * CELL_HEIGHT)
    row_width = len(text) * CELL_WIDTH
    for cell, character in enumerate(text):
        glyph = GLYPHS[character]
        for y in range(CELL_HEIGHT):
            pattern = glyph[y] if y < 7 else "....."
            for x in range(CELL_WIDTH):
                lit = x < 5 and pattern[x] == "#"
                result[y * row_width + cell * CELL_WIDTH + x] = (
                    FOREGROUND if lit else BACKGROUND)
    return bytes(result)


def render_full(field: str) -> bytes:
    line1 = render_text(f"FPS: {field}   ")
    line2 = render_text("ZUNDAMON: 1")
    if len(line1) != HUD_WIDTH * CELL_HEIGHT or len(line2) != len(line1):
        raise ValueError("M98T_HUD_LAYOUT")
    return line1 + line2


def emit_bytes(lines: list[str], label: str, data: bytes, width: int) -> None:
    lines.append(f"{label}:")
    for offset in range(0, len(data), width):
        row = data[offset:offset + width]
        lines.append("    db " + ",".join(f"0x{value:02x}" for value in row))


def encode_include() -> tuple[bytes, tuple[bytes, ...], tuple[bytes, ...]]:
    full_tiles = tuple(render_full(field) for field in FPS_FIELDS)
    fps_tiles = tuple(render_text(field) for field in FPS_FIELDS)
    count_tiles = tuple(render_text(field) for field in COUNT_FIELDS)
    lines = [
        "; Copyright (c) 2026 Nakata Maho",
        ";",
        "; Redistribution and use in source and binary forms, with or without",
        "; modification, are permitted provided that the following conditions are met:",
        "; 1. Redistributions of source code must retain the above copyright notice,",
        ";    this list of conditions and the following disclaimer.",
        "; 2. Redistributions in binary form must reproduce the above copyright notice,",
        ";    this list of conditions and the following disclaimer in the documentation",
        ";    and/or other materials provided with the distribution.",
        ";",
        "; THIS SOFTWARE IS PROVIDED BY THE AUTHOR \"AS IS\" AND ANY EXPRESS OR IMPLIED",
        "; WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF",
        "; MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO",
        "; EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,",
        "; SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,",
        "; PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;",
        "; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,",
        "; WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR",
        "; OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF",
        "; ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.",
        ";",
        "; Generated by generate_zundamon_orbit_hud.py.  Do not edit by hand.",
        "%define HUD_TILE_COUNT 8",
        f"%define HUD_FULL_TILE_BYTES {HUD_WIDTH * HUD_HEIGHT}",
        f"%define HUD_FPS_TILE_BYTES {FPS_WIDTH * FPS_HEIGHT}",
        f"%define HUD_COUNT_TILE_COUNT {len(COUNT_FIELDS)}",
        f"%define HUD_COUNT_TILE_BYTES {COUNT_WIDTH * CELL_HEIGHT}",
        f"%define HUD_FOREGROUND 0x{FOREGROUND:02x}",
        f"%define HUD_BACKGROUND 0x{BACKGROUND:02x}",
    ]
    for index, data in enumerate(full_tiles, 1):
        emit_bytes(lines, f"hud_full_tile_v{index}", data, HUD_WIDTH)
    lines.append("hud_full_tile_pointers:")
    lines.append("    dw " + ",".join(f"hud_full_tile_v{index}" for index in range(1, 9)))
    for index, data in enumerate(fps_tiles, 1):
        emit_bytes(lines, f"hud_fps_tile_v{index}", data, FPS_WIDTH)
    lines.append("hud_fps_tile_pointers:")
    lines.append("    dw " + ",".join(f"hud_fps_tile_v{index}" for index in range(1, 9)))
    for field, data in zip(COUNT_FIELDS, count_tiles):
        label = field.strip()
        emit_bytes(lines, f"hud_count_tile_{label}", data, COUNT_WIDTH)
    lines.append("hud_count_tile_pointers:")
    lines.append("    dw " + ",".join(
        f"hud_count_tile_{count}" for count in range(1, 17)))
    return ("\n".join(lines) + "\n").encode("ascii"), full_tiles, fps_tiles


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--metadata", type=Path)
    args = parser.parse_args()
    if args.output.exists() or (args.metadata is not None and args.metadata.exists()):
        parser.error("refusing to overwrite generated output")
    encoded, full_tiles, fps_tiles = encode_include()
    count_tiles = tuple(render_text(field) for field in COUNT_FIELDS)
    args.output.write_bytes(encoded)
    digest = hashlib.sha256(encoded).hexdigest()
    if args.metadata is not None:
        args.metadata.write_text(json.dumps({
            "schema": "zundamon-orbit-m98x-hud-v1",
            "font_provenance": "task-authored-public-5x7",
            "glyphs": sorted(GLYPHS),
            "foreground": FOREGROUND,
            "background": BACKGROUND,
            "hud_rect": [4, 4, 70, 20],
            "fps_value_rect": [34, 4, 52, 12],
            "fps_fields": list(FPS_FIELDS),
            "count_fields": list(COUNT_FIELDS),
            "count_value_rect": [58, 12, 70, 20],
            "full_tile_sha256": [hashlib.sha256(tile).hexdigest()
                                 for tile in full_tiles],
            "fps_tile_sha256": [hashlib.sha256(tile).hexdigest()
                                for tile in fps_tiles],
            "count_tile_sha256": [hashlib.sha256(tile).hexdigest()
                                  for tile in count_tiles],
            "include_sha256": digest,
        }, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(f"M98X_HUD_GENERATION_PASS fps_fields={len(FPS_FIELDS)} "
          f"count_fields={len(COUNT_FIELDS)} "
          f"full_bytes={len(full_tiles[0])} fps_bytes={len(fps_tiles[0])} "
          f"sha256={digest}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
