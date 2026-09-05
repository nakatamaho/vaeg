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

"""Static preset and reference-coordinate checks, not a GPU rendering test.

Optionally compile both stages with --glslang /path/to/glslangValidator.
"""
import argparse
from fractions import Fraction
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[3]
CRT = ROOT / "assets/shaders/crt"
SOURCE = (CRT / "shaders/vaeg-screen-size.slang").read_text()


class ScreenSizeTests(unittest.TestCase):
    def test_preset_order_and_extent(self):
        preset = dict(line.strip().split(" = ", 1)
                      for line in (CRT / "vaeg_crt_default.slangp").read_text().splitlines()
                      if " = " in line)
        self.assertEqual(preset["shaders"], "2")
        self.assertEqual(preset["shader0"], "shaders/vaeg-screen-size.slang")
        self.assertEqual(preset["shader1"], "shaders/crt-lottes-fast.slang")
        self.assertEqual(preset["scale_type0"], "source")
        self.assertEqual(preset["scale0"], "1.0")
        self.assertEqual(preset["scale_type1"], "viewport")

    def test_shader_contract(self):
        self.assertIn('"Screen size (%)" 100.0 80.0 120.0 1.0', SOURCE)
        self.assertIn("(vTexCoord - vec2(0.5)) / scale + vec2(0.5)", SOURCE)
        self.assertIn("lessThan(uv, vec2(0.0))", SOURCE)
        self.assertIn("greaterThanEqual(uv, vec2(1.0))", SOURCE)
        self.assertIn("vec4(0.0, 0.0, 0.0, 1.0)", SOURCE)
        self.assertNotIn("#include", SOURCE)
        self.assertLess(SOURCE.index("*/"), SOURCE.index("#version"))

    def test_reference_pixel_centers(self):
        # Exact rational oracle for the shader equation before CRT distortion.
        for extent in (640, 400, 1920, 1080):
            for percent in (80, 100, 120):
                scale = Fraction(percent, 100)
                mapped = [((Fraction(2 * x + 1, 2 * extent) - Fraction(1, 2))
                           / scale + Fraction(1, 2)) for x in range(extent)]
                visible = [x for x, uv in enumerate(mapped) if 0 <= uv < 1]
                self.assertEqual(len(visible), extent * min(percent, 100) // 100)
                self.assertEqual(visible[0], extent - 1 - visible[-1])
                if percent == 100:
                    self.assertEqual(mapped[0], Fraction(1, 2 * extent))
                if percent == 80:
                    self.assertEqual(visible[0], extent // 10)


def compile_stages(compiler):
    common, stages = SOURCE.split("#pragma stage vertex", 1)
    vertex, fragment = stages.split("#pragma stage fragment", 1)
    common = "\n".join(line for line in common.splitlines()
                       if not line.startswith("#pragma parameter")) + "\n"
    with tempfile.TemporaryDirectory(prefix="vaeg-screen-size-") as directory:
        for suffix, stage in (("vert", vertex), ("frag", fragment)):
            source = Path(directory) / ("screen." + suffix)
            source.write_text(common + stage)
            subprocess.run([compiler, "-V", str(source), "-o",
                            str(Path(directory) / (suffix + ".spv"))], check=True)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--glslang")
    args = parser.parse_args()
    if args.glslang:
        compile_stages(args.glslang)
    unittest.main(argv=[__file__])
