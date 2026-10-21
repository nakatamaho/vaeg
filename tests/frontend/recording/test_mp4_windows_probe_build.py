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

"""Focused content/generation tests; no Git history, SDK, media or compiler."""
import importlib.util
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[3]
SPEC = importlib.util.spec_from_file_location(
    "probe_builder", ROOT / "tools/release/build-mp4-windows-probe.py"
)
BUILDER = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(BUILDER)


class ProbeGeneration(unittest.TestCase):
    def setUp(self):
        self.adapter = (ROOT / BUILDER.ADAPTER).read_text()
        self.common = (ROOT / BUILDER.COMMON).read_text()
        # Every negative begins with a passing fixture, independently of Git.
        BUILDER.generate(self.adapter, self.common)

    def assert_code(self, adapter, common, code):
        with self.assertRaises(ValueError) as caught:
            BUILDER.generate(adapter, common)
        self.assertEqual(str(caught.exception).split(":", 1)[0], code)

    def test_hardware_is_the_only_between_variant_mutation(self):
        on, off, pixels = BUILDER.generate(self.adapter, self.common)
        self.assertEqual(
            off,
            on.replace(
                "MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, TRUE",
                "MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, FALSE",
            ),
        )
        self.assertEqual(on.count("writer_->Finalize()"), 1)
        self.assertIn("check(result);", on)
        self.assertIn('mp4_probe_report("before"', on)
        self.assertIn('mp4_probe_report("after"', on)
        start = self.common.index("void mp4_bgra_to_nv12(")
        end = self.common.index("\n#if !defined(__APPLE__) && !defined(_WIN32)")
        self.assertIn(self.common[start:end], pixels)
        self.assertNotIn("libavcodec", pixels)

    def test_missing_finalize(self):
        changed = self.adapter.replace("check(writer_->Finalize());", "removed();")
        self.assert_code(changed, self.common, "MP4_PROBE_SOURCE_CONTRACT")

    def test_duplicate_finalize(self):
        changed = self.adapter + "\ncheck(writer_->Finalize());\n"
        self.assert_code(changed, self.common, "MP4_PROBE_SOURCE_CONTRACT")

    def test_missing_hardware_preference(self):
        changed = self.adapter.replace("MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, TRUE", "missing")
        self.assert_code(changed, self.common, "MP4_PROBE_SOURCE_CONTRACT")

    def test_missing_pixel_function(self):
        changed = self.common.replace("void mp4_bgra_to_nv12(", "void renamed(")
        self.assert_code(self.adapter, changed, "MP4_PROBE_PIXEL_CONTRACT")

    def test_missing_pixel_boundary(self):
        changed = self.common.replace(
            "\n#if !defined(__APPLE__) && !defined(_WIN32)", "\n#if 0"
        )
        self.assert_code(self.adapter, changed, "MP4_PROBE_PIXEL_CONTRACT")


if __name__ == "__main__":
    unittest.main()
