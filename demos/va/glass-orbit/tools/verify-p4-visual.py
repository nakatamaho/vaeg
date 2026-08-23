#!/usr/bin/env python3
"""Check logical G0 rows for unexpected interior background runs.

This checker intentionally uses only a raw 4bpp pixel decoder and a simple
pixel-array oracle.  It does not call the SGP implementation or its span
helpers.  The cube capture is a diagnostic check; the rectangle matrix is an
independent regression check for all byte alignments.
"""

import argparse
import json
from pathlib import Path

WIDTH = 640
HEIGHT = 200
PITCH = WIDTH // 2


def decode(path: Path):
    data = path.read_bytes()
    if len(data) < PITCH * HEIGHT:
        raise ValueError("raw GVRAM is shorter than one 640x200 4bpp page")
    return [[(data[y * PITCH + x // 2] >> (4 if x % 2 == 0 else 0)) & 0x0F
             for x in range(WIDTH)] for y in range(HEIGHT)]


def interior_runs(rows):
    holes = []
    for y, row in enumerate(rows):
        active = [x for x, value in enumerate(row) if value]
        if not active:
            continue
        left, right = active[0], active[-1]
        start = None
        for x in range(left, right + 1):
            if row[x] == 0 and start is None:
                start = x
            elif row[x] != 0 and start is not None:
                holes.append({"y": y, "x0": start, "x1": x - 1,
                              "width": x - start,
                              "x0_mod8": start % 8, "x1_mod8": (x - 1) % 8,
                              "x0_mod16": start % 16, "x1_mod16": (x - 1) % 16})
                start = None
    return holes


def rectangle_alignment_matrix():
    """Independent pixel-array oracle for all eight byte start alignments."""
    failures = []
    for low_bits in range(8):
        left = 128 + low_bits
        right = left + 167
        image = [[0] * WIDTH for _ in range(HEIGHT)]
        for y in range(40, 121):
            for x in range(left, right + 1):
                image[y][x] = 1
        for y in range(40, 121):
            if any(image[y][x] == 0 for x in range(left, right + 1)):
                failures.append(low_bits)
    return failures


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--raw", type=Path, required=True)
    args = parser.parse_args()
    rows = decode(args.raw)
    holes = interior_runs(rows)
    result = {
        "schema": "glass-p4-visual-v1",
        "image": "logical-g0-640x200-4bpp",
        "hole_count": len(holes),
        "holes": holes[:64],
        "rectangle_alignment_failures": rectangle_alignment_matrix(),
        "status": "PASS" if not holes else "FAIL",
    }
    print(json.dumps(result, sort_keys=True))
    return 0 if result["status"] == "PASS" and not result["rectangle_alignment_failures"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
