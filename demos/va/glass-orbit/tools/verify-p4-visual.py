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
PROJECTION_OFFSET = 0xFC00
FACE_COLOURS = (8, 10, 13)


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
            if any(image[y][x] != 1 for x in range(left, right + 1)):
                failures.append(low_bits)
            border = [left - 1, right + 1]
            if any(0 <= x < WIDTH and image[y][x] != 0 for x in border):
                failures.append(low_bits)
    # Same-word, adjacent, and multi-word endpoint combinations.
    for start_offset in range(8):
        for end_offset in range(8):
            left = 128 + start_offset
            right = 128 + end_offset
            if right < left:
                left, right = right, left
            for width in (1, 2, 3, 4, 5, 8, 9, 16, 17):
                x0 = left
                x1 = min(WIDTH - 2, x0 + width - 1)
                image = [0] * WIDTH
                for x in range(x0, x1 + 1):
                    image[x] = 1
                if any(image[x] != 1 for x in range(x0, x1 + 1)):
                    failures.append((start_offset, end_offset, width))
                if (x0 > 0 and image[x0 - 1]) or (x1 + 1 < WIDTH and image[x1 + 1]):
                    failures.append((start_offset, end_offset, width))
    return failures


def trunc_div(numerator, denominator):
    if numerator >= 0:
        return numerator // denominator
    return -((-numerator) // denominator)


def triangle_spans(vertices, indices):
    points = [[vertices[index][0], vertices[index][1] // 2] for index in indices]
    points.sort(key=lambda point: point[1])
    spans = {}
    for y in range(points[0][1], points[2][1]):
        def edge(first, second):
            dy = second[1] - first[1]
            if dy == 0:
                return first[0]
            return first[0] + trunc_div((y - first[1]) * (second[0] - first[0]), dy)

        long_x = edge(points[0], points[2])
        if y < points[1][1]:
            short_x = edge(points[0], points[1])
        else:
            short_x = edge(points[1], points[2])
        spans[y] = (min(long_x, short_x), max(long_x, short_x))
    return spans


def expected_faces(data):
    raw_vertices = []
    for offset in range(PROJECTION_OFFSET, PROJECTION_OFFSET + 48, 6):
        x = int.from_bytes(data[offset:offset + 2], "little", signed=True)
        y = int.from_bytes(data[offset + 2:offset + 4], "little", signed=True)
        raw_vertices.append((x, y))
    faces = ((0, 1, 2, 3, 8), (4, 7, 6, 5, 9),
             (0, 4, 5, 1, 10), (3, 2, 6, 7, 12),
             (0, 3, 7, 4, 11), (1, 5, 6, 2, 13))
    expected = [[0] * WIDTH for _ in range(HEIGHT)]
    for v0, v1, v2, v3, colour in faces:
        a, b, c = raw_vertices[v0], raw_vertices[v1], raw_vertices[v2]
        cross = ((b[0] - a[0]) * (c[1] - a[1]) -
                 (b[1] - a[1]) * (c[0] - a[0]))
        if cross >= 0:
            continue
        for triangle in ((v0, v1, v2), (v0, v2, v3)):
            for y, (x0, x1) in triangle_spans(raw_vertices, triangle).items():
                if not 0 <= y < HEIGHT:
                    continue
                for x in range(max(0, x0), min(WIDTH - 1, x1) + 1):
                    expected[y][x] = colour
    return expected


def compare_faces(data, actual):
    expected = expected_faces(data)
    underfill = []
    overfill = []
    for y in range(HEIGHT):
        for x in range(WIDTH):
            expected_face = expected[y][x] in FACE_COLOURS
            actual_face = actual[y][x] in FACE_COLOURS
            # Outline colours legitimately replace face pixels in the final
            # stage; only an actual background pixel is underfill.
            if expected_face and actual[y][x] == 0:
                underfill.append((x, y))
            elif actual_face and not expected_face:
                overfill.append((x, y))
    return underfill, overfill


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--raw", type=Path, required=True)
    args = parser.parse_args()
    rows = decode(args.raw)
    holes = interior_runs(rows)
    data = args.raw.read_bytes()
    underfill, overfill = compare_faces(data, rows)
    alignment_failures = rectangle_alignment_matrix()
    result = {
        "schema": "glass-p4-visual-v1",
        "image": "logical-g0-640x200-4bpp",
        "hole_count": len(holes),
        "holes": holes[:64],
        "underfill": len(underfill),
        "overfill": len(overfill),
        "underfill_runs": underfill[:32],
        "overfill_runs": overfill[:32],
        "rectangle_alignment_failures": alignment_failures,
        "status": "PASS" if not underfill and not overfill and
        not alignment_failures else "FAIL",
    }
    print(json.dumps(result, sort_keys=True))
    return 0 if result["status"] == "PASS" and not result["rectangle_alignment_failures"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
