#!/usr/bin/env python3
# Copyright (c) 2026 Nakata Maho
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
# 1. Redistributions of source code must retain the above copyright notice,
#    this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR IMPLIED
# WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
# OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
# OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
# OF THE POSSIBILITY OF SUCH DAMAGE.
"""Original RGB332 dawn and perimeter-DDA model; no hardware proof."""
import argparse
import copy
import math
from pathlib import Path
import unittest

import generate as voyage

BOUNDARY = (tuple(range(9)) + tuple(range(17, 108, 9)) +
            tuple(range(106, 98, -1)) + tuple(range(90, 0, -9)))
WORD_LIMIT = 8192
# SET WORK/CLS/END margin, every primitive changing colour: deliberately loose.
MAX_WORDS = 20 + (voyage.MAX_LINES + 100 + 29 + 200 + 200) * 10


class SceneError(ValueError):
    def __init__(self, code):
        self.code = code
        super().__init__(code)


def blend(a, b, stage):
    value = 0
    for shift, mask in ((5, 7), (2, 7), (0, 3)):
        x, y = (a >> shift) & mask, (b >> shift) & mask
        value |= ((x * (7 - stage) + y * stage + 3) // 7) << shift
    return value


def make_scene():
    night = (3, 7, 0x23, 0x27, 0x43, 0x47, 0x23, 3)
    day = (0x23, 0x43, 0x63, 0x67, 0x83, 0x87, 0xa7, 0xc7)
    sky = [0,1,2,3,7, 1,3,7,11,15, 3,7,11,15,19, 7,11,15,23,27,
           11,15,0x33,0x37,0x3b, 15,0x33,0x37,0x3b,0x5b,
           0x33,0x37,0x5b,0x5f,0x7d, 0x63,0x67,0x8b,0x5f,0x7d]
    return {"version": 1, "boundary": list(BOUNDARY), "sky": sky,
            "sea": [blend(a, b, stage) for stage in range(8) for a, b in zip(night, day)],
            "sun": [math.isqrt(196 - dy * dy) for dy in range(-14, 15)],
            "word_limit": WORD_LIMIT}


def validate(data):
    if data.get("version") != 1:
        raise SceneError("M107C_SCHEMA")
    boundary = data.get("boundary", [])
    if len(boundary) != 38:
        raise SceneError("M107C_BOUNDARY_COUNT")
    if any(type(v) is not int or not 0 <= v < 108 for v in boundary):
        raise SceneError("M107C_BOUNDARY_RANGE")
    if tuple(boundary) != BOUNDARY:
        raise SceneError("M107C_BOUNDARY_ORDER")
    for name, count in (("sky", 40), ("sea", 64)):
        colours = data.get(name, [])
        if len(colours) != count:
            raise SceneError("M107C_COLOUR_COUNT")
        if any(type(v) is not int or not 0 <= v <= 255 for v in colours):
            raise SceneError("M107C_COLOUR_RANGE")
    if data.get("sun") != [math.isqrt(196 - dy * dy) for dy in range(-14, 15)]:
        raise SceneError("M107C_SUN_SHAPE")
    if data.get("word_limit") != WORD_LIMIT or MAX_WORDS > WORD_LIMIT:
        raise SceneError("M107C_COMMAND_CAPACITY")


def trunc_div(a, b):
    return (abs(a) // abs(b)) * (-1 if (a < 0) != (b < 0) else 1)


def edge_points(a, b):
    if a[1] > b[1]:
        a, b = b, a
    x, y = a
    dy = b[1] - y
    if not dy:
        return []
    dx = b[0] - x
    whole = trunc_div(dx, dy)
    fraction = trunc_div((dx - whole * dy) * 256, dy)
    remainder = 0
    result = []
    for row in range(y, b[1]):
        result.append((row, max(0, min(319, x))))
        x += whole
        remainder += fraction
        if remainder < 0:
            remainder += 256
            x -= 1
        elif remainder >= 256:
            remainder -= 256
            x += 1
    return result


def spans(points):
    left, right = [320] * 200, [-1] * 200
    boundary = BOUNDARY + (BOUNDARY[0],)
    for a, b in zip(boundary, boundary[1:]):
        for y, x in edge_points(points[a], points[b]):
            left[y] = min(left[y], x)
            right[y] = max(right[y], x)
    return list(zip(left, right))


def stage(tick, loops):
    return (tick % 900) * 8 // 900 if loops & 1 else 0


class Tests(unittest.TestCase):
    def setUp(self):
        self.data = make_scene()
        validate(self.data)

    def rejected(self, mutation, code):
        data = copy.deepcopy(self.data)
        mutation(data)
        with self.assertRaises(SceneError) as caught:
            validate(data)
        self.assertEqual(caught.exception.code, code)

    def test_boundary_count(self):
        self.rejected(lambda d: d["boundary"].pop(), "M107C_BOUNDARY_COUNT")

    def test_boundary_range(self):
        self.rejected(lambda d: d["boundary"].__setitem__(0, 108), "M107C_BOUNDARY_RANGE")

    def test_colour_range(self):
        self.rejected(lambda d: d["sea"].__setitem__(0, 256), "M107C_COLOUR_RANGE")

    def test_capacity(self):
        self.rejected(lambda d: d.update(word_limit=4096), "M107C_COMMAND_CAPACITY")

    def test_sun(self):
        self.rejected(lambda d: d["sun"].__setitem__(14, 13), "M107C_SUN_SHAPE")

    def test_vectors(self):
        self.assertEqual(edge_points((2, 1), (6, 5)), [(1,2),(2,3),(3,4),(4,5)])
        self.assertEqual(edge_points((6, 1), (2, 5)), [(1,6),(2,5),(3,4),(4,3)])
        self.assertEqual(edge_points((0, 1), (319, 1)), [])
        self.assertEqual(edge_points((0, 0), (319, 1)), [(0,0)])
        self.assertEqual(edge_points((319, 199), (0, 0)), edge_points((0,0),(319,199)))

    def test_all_poses(self):
        data = voyage.make_data()
        for tick in range(900):
            points = voyage.project(tick, data)
            surface = spans(points)
            self.assertTrue(all((l == 320 and r == -1) or 0 <= l <= r <= 319 for l, r in surface))
            self.assertTrue(any(l <= r for l, r in surface))
            for a, b in zip(BOUNDARY, BOUNDARY[1:] + BOUNDARY[:1]):
                p, q = points[a], points[b]
                if p[1] > q[1]:
                    p, q = q, p
                if p[1] == q[1]:
                    continue
                for y, x in edge_points(p, q):
                    exact = p[0] + (q[0]-p[0])*(y-p[1])/(q[1]-p[1])
                    self.assertLessEqual(abs(x - exact), 1.8)

    def test_staging(self):
        self.assertEqual([stage(899, i) for i in range(4)], [0,7,0,7])
        self.assertEqual({stage(t, 1) for t in range(900)}, set(range(8)))
        self.assertLess(MAX_WORDS, WORD_LIMIT)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    output = args.output.resolve()
    repo = Path(__file__).resolve().parents[2]
    if output == repo or repo in output.parents:
        raise SceneError("M107C_OUTPUT_IN_TREE")
    data = make_scene()
    validate(data)
    output.mkdir(parents=True, exist_ok=True)
    lines = ["; Source-generated original dawn tables."]
    for label, kind, values in (("dawn_boundary", "dw", [n*4 for n in BOUNDARY + BOUNDARY[:1]]),
                                ("dawn_sky_colours", "db", data["sky"]),
                                ("dawn_sea_colours", "db", data["sea"]),
                                ("dawn_sun_widths", "db", data["sun"])):
        lines += [label + ":", "    " + kind + " " + ",".join(str(v) for v in values)]
    (output / "dawn_tables.inc").write_text("\n".join(lines) + "\n", encoding="utf-8")
    print(f"M107C_SCENE_OK edges=38 poses=900 words_max={MAX_WORDS}/{WORD_LIMIT}")


if __name__ == "__main__":
    main()
