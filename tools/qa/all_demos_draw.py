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
# OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
# OF THE POSSIBILITY OF SUCH DAMAGE.

"""Content-only all-demos drawing check; private captures stay outside Git.

Requires a pre-command screenshot and post-command completed guest frames.
Checks substantial, multicoloured guest drawing, not merely process exit,
console text or a saved PNG. This is NOT a full image/animation oracle, a
final-client/layout check, physical-GPU coverage or Windows execution.
Capture/binary/media provenance belongs to the caller's evidence adapter;
content tests have no Git-history, topology or protected-artifact dependencies.

The sphere24 profile covers the source's single 24x24 sprite on a 320x200
canvas displayed at 640x400. It must not inherit the larger scene-area bound.
Do not use the coloured-scene profiles for monochrome programs. The mandel
profile checks source-derived first-row colours and increasing plotted rows,
not completion of the full fractal.
"""
from __future__ import annotations

import argparse
import json
from pathlib import Path
import re
import struct

from screen_compare import QaError, read_png, require

CONTRACT = "all-demos-coloured-draw-v1"
LIMITS = {"scene": (4096, 1024), "sphere24": (256, 128)}


def classify(before, posts, profile="scene"):
    require(profile in LIMITS, "G139_DEMO_PROFILE")
    require(bool(posts), "G139_DEMO_CAPTURE_SET")
    w, h, original = before
    require(w == 640 and 400 <= h <= 512 and len(original) == w*h*4,
            "G139_DEMO_GEOMETRY")
    require(all(p[:2] == (w, h) and len(p[2]) == w*h*4 for p in posts),
            "G139_DEMO_GEOMETRY")
    metrics = []
    for _, _, pixels in posts:
        changed = coloured = 0
        colours = set()
        # The guest contract uses the first 400 rows, not the status strip.
        for i in range(0, w*400*4, 4):
            rgb = pixels[i:i+3]
            if rgb != b"\0\0\0":
                colours.add(rgb)
            if rgb != original[i:i+3]:
                changed += 1
                if max(rgb) >= 96 and max(rgb)-min(rgb) >= 64:
                    coloured += 1
        metrics.append({"changed_pixels": changed,
                        "coloured_changed_pixels": coloured,
                        "nonblack_colours": len(colours)})
    area, coloured_area = LIMITS[profile]
    require(any(m["changed_pixels"] >= area and
                m["coloured_changed_pixels"] >= coloured_area and
                m["nonblack_colours"] >= 3 for m in metrics),
            "G139_DEMO_DRAW_MISSING")
    return {"contract": CONTRACT, "status": "PASS_DRAW_CONTRACT",
            "profile": profile, "frames": metrics,
            "scope": "coloured guest drawing only; not full scene/animation or presenter acceptance"}


def classify_mandel(before, posts):
    """Unmodified mandelbrot.bas: MI32, x0/320/639 escape at IT1/3/2.

    PP=1+INT(IT*14/32) gives red/orange/red on the first plotted row.
    The leftmost32 pixels have CR<-2 and escape immediately on every row.
    Active400-line graphics are bottom-aligned in the native guest capture.
    """
    require(len(posts) == 2, "G139_DEMO_CAPTURE_SET")
    w, h, _ = before
    require(w == 640 and 400 <= h <= 512 and
            all(p[:2] == (w, h) and len(p[2]) == w*h*4 for p in [before, *posts]),
            "G139_DEMO_GEOMETRY")
    origin = h-400

    def rgb(image, x, y):
        i = ((y+origin)*w+x)*4
        return image[2][i:i+3]

    def red(c):
        return c[0] >= 192 and c[1] <= 32 and c[2] <= 32

    require(not red(rgb(before, 0, 0)), "G139_MANDEL_PHASE")
    counts = []
    for image in posts:
        middle = rgb(image, 320, 0)
        require(red(rgb(image, 0, 0)) and red(rgb(image, 639, 0)) and
                middle[0] >= 192 and 32 <= middle[1] <= 192 and middle[2] <= 32,
                "G139_MANDEL_POINTS")
        counts.append(sum(sum(red(rgb(image, x, y)) for x in range(32)) >= 28
                          for y in range(400)))
    require(2 <= counts[0] < counts[1] <= 400, "G139_MANDEL_NO_PROGRESS")
    return {"contract": "mandelbrot-basic-progress-v1", "status": "PASS_PROGRESS_CONTRACT",
            "first_row_escape_iterations": [1, 3, 2], "red_prefix_rows": counts,
            "scope": "unmodified BASIC source first-point colours and drawing progress; not full fractal completion"}


def boot_rows(data):
    """Decode the live first native TSP text frame, not arbitrary RAM strings."""
    require(len(data) >= 44 and data[:8] == b"VAEGSCN1", "G139_BOOT_DUMP")
    version, id_len = struct.unpack_from("<II", data, 8)
    start = 16+id_len
    require(version == 1 and id_len <= 4096 and len(data) >= start+28, "G139_BOOT_DUMP")
    table, _, lineheight, _, _, _, size = struct.unpack_from("<7I", data, start)
    mem = data[start+28:]
    require(size == len(mem) and 0 < size <= 0x40000 and table+32 <= size and
            1 <= lineheight <= 32, "G139_BOOT_DUMP")
    stride, mode = struct.unpack_from("<HH", mem, table+8)
    rsa = struct.unpack_from("<H", mem, table+16)[0]
    height = struct.unpack_from("<H", mem, table+20)[0] & 0x1fe
    require(mode & 0x1f == 1 and stride == 160, "G139_BOOT_TEXT_MODE")
    count = min(25, height//lineheight)
    require(count >= 18 and rsa+count*stride <= size, "G139_BOOT_DUMP")
    # Unwritten zero character cells render blank; do not treat them as NUL
    # separators in directory names/columns or as nonempty trailing rows.
    return [bytes(c if c >= 32 else 32 for c in
                  mem[rsa+y*stride:rsa+(y+1)*stride:2]).decode("ascii", errors="replace").rstrip()
            for y in range(count)]


def classify_boot(data, kind):
    require(kind in ("fdd", "sasi", "sgp"), "G139_BOOT_PROFILE")
    rows = boot_rows(data)
    text = "\n".join(rows)
    if kind == "sgp":
        require(all(s in text for s in ("pseudo-sprites", "wireframe", "SGPTEST",
                                       "SGPDUMP", "Press a letter to run a demo.")),
                "G139_SGP_BOOT_MENU")
    else:
        require(rows[0].startswith("A:"), "G139_PCENGINE_BOOT_VOLUME")
        visible = [r.strip() for r in rows if r.strip()]
        require(visible[-1] == "Ready", "G139_PCENGINE_PROMPT")
        directories = set(re.findall(r"^([A-Z0-9]+)\s+<DIR>", text, re.M))
        expected = ({"ARCHIVE", "BIN", "DOC", "SYS", "TMP"} if kind == "fdd" else
                    {"BASIC", "BIN", "CPM", "DOC", "LSIC86", "SRC", "SYS", "UNIX", "VZ"})
        require(expected <= directories and (kind != "fdd" or
                ("PCENGINE.COM" in text and re.search(r"\bCONFIG\s*\.SYS\b", text))),
                "G139_BOOT_DIRECTORY")
    return {"contract": "utility-native-boot-content-v1", "status": "PASS_BOOT_CONTRACT",
            "kind": kind, "scope": "live guest menu or filesystem directory and prompt; not all utilities or presenter coverage"}


def boot_selftest():
    for kind in ("fdd", "sasi", "sgp"):
        rows = ["A:", "PCENGINE.COM", "CONFIG  .SYS"]
        if kind == "sgp":
            rows = ["pseudo-sprites", "wireframe", "SGPTEST", "SGPDUMP",
                    "Press a letter to run a demo."]
        else:
            rows += [n+" <DIR>" for n in ("ARCHIVE", "BASIC", "BIN", "CPM", "DOC", "LSIC86",
                                          "SRC", "SYS", "TMP", "UNIX", "VZ")]+["Ready"]
        mem = bytearray(0x40000)
        for y, row in enumerate(rows):
            for x, char in enumerate(row):
                mem[y*160+x*2] = ord(char)
        struct.pack_into("<HH", mem, 0x7008, 160, 1)
        struct.pack_into("<H", mem, 0x7014, 400)
        data = b"VAEGSCN1"+struct.pack("<9I", 1, 0, 0x7000, 0x8000, 20, 31, 0, 1, len(mem))+mem
        assert classify_boot(data, kind)["status"] == "PASS_BOOT_CONTRACT"
        wrong = bytearray(data)
        y = 4 if kind == "sgp" else len(rows)-1
        wrong[44+y*160] = ord("X")
        cases = [(bytes(wrong), kind, "G139_SGP_BOOT_MENU" if kind == "sgp" else "G139_PCENGINE_PROMPT"),
                 (b"X"+data[1:], kind, "G139_BOOT_DUMP"),
                 (data, "unknown", "G139_BOOT_PROFILE")]
        if kind != "sgp":
            missing = bytearray(data)
            offset = 44+2*160+9*2 if kind == "fdd" else 44+10*160
            missing[offset] = ord("X")
            cases.append((bytes(missing), kind, "G139_BOOT_DIRECTORY"))
        for mutated, selected, code in cases:
            try:
                classify_boot(mutated, selected)
            except QaError as error:
                assert error.code == code, (error.code, code)
            else:
                raise AssertionError(code)
    return 11


def selftest():
    width, height = 640, 400
    blank = (width, height, bytes([0, 0, 0, 255])*(width*height))
    for profile, side in (("scene", 64), ("sphere24", 16)):
        pixels = bytearray(blank[2])
        for y in range(side):
            for x in range(side):
                i = (y*width+x)*4
                pixels[i:i+3] = bytes((255, 0, 0))
        pixels[:3] = bytes((0, 255, 0))
        pixels[4:7] = bytes((0, 0, 255))
        passing = (width, height, bytes(pixels))
        assert classify(blank, [passing], profile)["status"] == "PASS_DRAW_CONTRACT"
        # Each negative starts with the passing fixture and changes one field.
        mutations = [([blank], profile, "G139_DEMO_DRAW_MISSING"),
                     ([(639, height, passing[2])], profile, "G139_DEMO_GEOMETRY"),
                     ([passing], "unknown", "G139_DEMO_PROFILE"),
                     ([], profile, "G139_DEMO_CAPTURE_SET")]
        white = bytearray(passing[2])
        for i in range(0, len(white), 4):
            if white[i:i+3] != b"\0\0\0":
                white[i:i+3] = b"\xff\xff\xff"
        mutations.append(([(width, height, bytes(white))], profile,
                          "G139_DEMO_DRAW_MISSING"))
        for posts, selected, code in mutations:
            try:
                classify(blank, posts, selected)
            except QaError as error:
                assert error.code == code, (error.code, code)
            else:
                raise AssertionError(code)
    def plot(rows):
        pixels = bytearray(blank[2])
        for y in range(rows):
            for x in [*range(32), 639]:
                i = (y*width+x)*4
                pixels[i:i+3] = bytes((255, 0, 0))
        pixels[320*4:320*4+3] = bytes((255, 109, 0))
        return (width, height, bytes(pixels))

    first, second = plot(4), plot(8)
    assert classify_mandel(blank, [first, second])["red_prefix_rows"] == [4, 8]
    wrong = bytearray(first[2])
    wrong[320*4:320*4+3] = bytes((255, 0, 0))
    for before, posts, code in [
            (blank, [first, first], "G139_MANDEL_NO_PROGRESS"),
            (blank, [(width, height, bytes(wrong)), second], "G139_MANDEL_POINTS"),
            (first, [first, second], "G139_MANDEL_PHASE"),
            (blank, [first], "G139_DEMO_CAPTURE_SET")]:
        try:
            classify_mandel(before, posts)
        except QaError as error:
            assert error.code == code, (error.code, code)
        else:
            raise AssertionError(code)
    negatives = 14+boot_selftest()
    return {"status": "PASS", "profiles": 6, "single_mutation_negatives": negatives}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--selftest", action="store_true")
    parser.add_argument("--boot-dump", type=Path)
    parser.add_argument("--boot-kind", choices=("fdd", "sasi", "sgp"))
    parser.add_argument("--before", type=Path)
    parser.add_argument("--after", type=Path, action="append")
    parser.add_argument("--profile", choices=sorted([*LIMITS, "mandel"]), default="scene")
    args = parser.parse_args()
    try:
        if args.selftest:
            result = selftest()
        elif args.boot_dump:
            result = classify_boot(args.boot_dump.read_bytes(), args.boot_kind)
        else:
            require(args.before is not None and bool(args.after), "G139_DEMO_CAPTURE_SET")
            before = read_png(args.before.read_bytes())
            posts = [read_png(p.read_bytes()) for p in args.after]
            result = (classify_mandel(before, posts) if args.profile == "mandel" else
                      classify(before, posts, args.profile))
        print(json.dumps(result))
        return 0
    except QaError as error:
        print(json.dumps({"contract": CONTRACT, "status": "FAIL", "code": error.code}))
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
