#!/usr/bin/env python3
"""Build the bootable MS-DOS 4.0 640x200 65536-colour picture disks.

The PC-88VA's own graphics show the pictures of demos/hyperframe+ (a
ray-traced still and a Mandelbrot close-up), rendered again at 640x200 for
Graphic 0 single-plane 16 bpp, G6 R5 B5 words, with a 4x4 ordered dither.
The system files come from the FreeDOS-88VA project's MIT-licensed MS-DOS 4.0
for the PC-88VA (release msdos4-va.2, msdos4-pc88va-2hd.d88), checked by its
published SHA-256, exactly as for demos/hyperframe+:
  va65536-tetra-msdos4.d88    TETRA.G16   (../hyperframe+/raytrace.py)
  va65536-mandel-msdos4.d88   MANDEL.G16  (../hyperframe+/mandel.py)
each with VASHOW.COM (vashow.asm, NASM) and VASHOW.TXT. Directory entries get
a fixed time stamp, so the same inputs give the same bytes. The raw D88s are
written to --output-dir outside the repository; the compressed companions go
to demos/native65536/.
"""

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

from __future__ import annotations

import argparse
import hashlib
import importlib.util
import lzma
import os
import subprocess
import sys
import tempfile
from pathlib import Path

DEMO_DIR = Path(__file__).resolve().parent
REPOSITORY_ROOT = DEMO_DIR.parents[1]
PICTURE_DIR = REPOSITORY_ROOT / "demos" / "hyperframe+"
HF_BUILDER = PICTURE_DIR / "build-d88.py"
MSDOS4_BUILDER = REPOSITORY_ROOT / "tools" / "pc88va" / "build-all-demos-msdos4-disk.py"
WIDTH, HEIGHT = 640, 200
# disk stem, picture generator, picture file name
DISKS = (
    ("va65536-tetra-msdos4", "raytrace.py", "TETRA.G16"),
    ("va65536-mandel-msdos4", "mandel.py", "MANDEL.G16"),
)
BAYER4 = (
    (0, 8, 2, 10),
    (12, 4, 14, 6),
    (3, 11, 1, 9),
    (15, 7, 13, 5),
)


class BuildError(Exception):
    """A deterministic input or layout error."""


def load_module(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    if spec is None or spec.loader is None:
        raise BuildError(f"could not load {path}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def quantise(value, bits, threshold):
    """8-bit value to `bits` bits, with an ordered-dither threshold 0..15
    (integer arithmetic only)."""
    top = (1 << bits) - 1
    # value * top / 255 plus a dither offset of (threshold + 0.5) / 16 - 0.5
    scaled = value * top * 32 + (threshold * 2 + 1 - 16) * 255
    level = (scaled + 255 * 16) // (255 * 32)
    return max(0, min(top, level))


def rgb_to_g16(rgb):
    """640x200 R G B bytes to G6 R5 B5 little-endian words."""
    if len(rgb) != WIDTH * HEIGHT * 3:
        raise BuildError("the rendered picture is not 640x200x3 bytes")
    out = bytearray(WIDTH * HEIGHT * 2)
    for y in range(HEIGHT):
        row = BAYER4[y & 3]
        for x in range(WIDTH):
            i = (y * WIDTH + x) * 3
            t = row[x & 3]
            r = quantise(rgb[i], 5, t)
            g = quantise(rgb[i + 1], 6, t)
            b = quantise(rgb[i + 2], 5, t)
            word = (g << 10) | (r << 5) | b
            o = (y * WIDTH + x) * 2
            out[o] = word & 0xff
            out[o + 1] = word >> 8
    return bytes(out)


def build_payload(root, nasm, samples, generator, picture):
    """Write VASHOW.COM, VASHOW.TXT and the picture into root."""
    root.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="vaeg-va65536-render-") as temporary:
        rendered = Path(temporary) / "picture.rgb"
        subprocess.run([sys.executable, str(PICTURE_DIR / generator), str(rendered),
                        "--samples", str(samples), "--height", str(HEIGHT)], check=True)
        (root / picture).write_bytes(rgb_to_g16(rendered.read_bytes()))
    subprocess.run([nasm, "-f", "bin", "-o", str(root / "VASHOW.COM"),
                    str(DEMO_DIR / "vashow.asm")], check=True)
    text = (DEMO_DIR / "vashow.txt").read_bytes().replace(b"\r\n", b"\n")
    (root / "VASHOW.TXT").write_bytes(text.replace(b"\n", b"\r\n"))


def main(argv=None):
    msdos4 = load_module("pc88va_all_demos_msdos4_builder", MSDOS4_BUILDER)
    hf = load_module("vaeg_hyperframe_plus_builder", HF_BUILDER)
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--source", type=Path,
                        help=f"msdos4-pc88va-2hd.d88 of {msdos4.MSDOS4_RELEASE} (default: "
                             f"the shared cache, downloaded when missing)")
    parser.add_argument("--output-dir", required=True, type=Path,
                        help="directory for the raw D88s, outside the repository")
    parser.add_argument("--compressed-dir", type=Path, default=DEMO_DIR,
                        help="directory for the .d88.xz companions (default demos/native65536)")
    parser.add_argument("--check", action="store_true",
                        help="build, and compare with the committed .d88.xz instead of "
                             "writing them")
    parser.add_argument("--samples", type=int, default=3)
    args = parser.parse_args(argv)
    nasm = os.environ.get("NASM", "nasm")
    output_dir = args.output_dir.resolve()
    compressed_dir = args.compressed_dir.resolve()
    results = []
    try:
        if output_dir == REPOSITORY_ROOT or REPOSITORY_ROOT in output_dir.parents:
            raise BuildError("the raw D88s must be written outside the repository")
        output_dir.mkdir(parents=True, exist_ok=True)
        for stem, _, _ in DISKS:
            if (output_dir / f"{stem}.d88").exists():
                raise BuildError(f"output already exists: {output_dir / f'{stem}.d88'}")
            if not args.check and (compressed_dir / f"{stem}.d88.xz").exists():
                raise BuildError(f"output already exists: {compressed_dir / f'{stem}.d88.xz'}")
        source = args.source if args.source is not None else msdos4.cached_source()
        for stem, generator, picture in DISKS:
            names = ("VASHOW.COM", "VASHOW.TXT", picture)
            with tempfile.TemporaryDirectory(prefix="vaeg-va65536-payload-") as temporary:
                payload_root = Path(temporary) / "root"
                build_payload(payload_root, nasm, args.samples, generator, picture)
                image = hf.build_image(source.resolve(), payload_root, msdos4)
                errors = [code.replace("M149_HFSHOW_", "M149_VASHOW_")
                          for code in hf.validate_image(image, payload_root, msdos4, names)]
            if errors:
                raise BuildError(f"{stem}: " + " ".join(errors))
            compressed = lzma.compress(image, format=lzma.FORMAT_XZ,
                                       preset=lzma.PRESET_EXTREME | 9)
            if lzma.decompress(compressed) != image:
                raise BuildError(f"{stem}: the compressed image does not round-trip")
            xz = compressed_dir / f"{stem}.d88.xz"
            if args.check:
                if lzma.decompress(xz.read_bytes()) != image:
                    raise BuildError(f"{stem}: M149_VASHOW_COMMITTED_IMAGE_DIFFERS")
            (output_dir / f"{stem}.d88").write_bytes(image)
            if not args.check:
                xz.write_bytes(compressed)
            results.append((stem, hashlib.sha256(image).hexdigest(),
                            hashlib.sha256(compressed).hexdigest()))
    except (hf.BuildError, BuildError, OSError, subprocess.CalledProcessError) as error:
        parser.exit(1, f"error: {error}\n")
    for stem, image_sha, xz_sha in results:
        print(f"{stem}.d88 SHA-256 {image_sha}")
        if args.check:
            print(f"  matches {compressed_dir / (stem + '.d88.xz')}")
        else:
            print(f"  {stem}.d88.xz SHA-256 {xz_sha}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
