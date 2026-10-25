#!/usr/bin/env python3
# Copyright (c) 2026 Nakata Maho
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
# 1. Redistributions of source code must retain the above copyright notice,
#    this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
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

"""M126 content tests; the public-cache integration never downloads assets."""

import hashlib
import importlib.util
import os
from pathlib import Path
import shutil
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
TOOLS = ROOT / "tools/pc88va"


def load(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


STAGE = load("softlib_stage", TOOLS / "stage-softlib-additions.py")
MEDIA = load("softlib_media", TOOLS / "test_vtiming_media.py")
CACHE = Path(os.environ.get("VAEG_CACHE_DIR") or Path.home() / ".cache/vaeg") / "auto-generated-pc88va-utility-media"


class ContentTests(unittest.TestCase):
    def test_basic_sources_are_unchanged_except_crlf(self):
        files = STAGE.basic_files()
        for source, target in (("fpu-test/fputest.bas", "FPUTEST.BAS"),
                               ("mandelbrot/mandelbrot.bas", "MANDEL.BAS")):
            actual = files[f"BASIC/{target}"]
            self.assertIn(b"\r\n", actual)
            self.assertNotIn(b"\n", actual.replace(b"\r\n", b""))
            self.assertEqual(actual.replace(b"\r\n", b"\n"),
                             (ROOT / "demos" / source).read_bytes())

    def test_hash_negative_has_one_mutation_and_exact_code(self):
        with tempfile.TemporaryDirectory() as temp:
            path = Path(temp) / "fixture"
            path.write_bytes(b"valid fixture")
            expected = hashlib.sha256(path.read_bytes()).hexdigest()
            self.assertEqual(STAGE.verified(path, expected, "M126_INPUT_HASH"),
                             b"valid fixture")
            path.write_bytes(b"Valid fixture")
            with self.assertRaises(STAGE.PackageError) as caught:
                STAGE.verified(path, expected, "M126_INPUT_HASH")
            self.assertEqual(str(caught.exception).split(":", 1)[0], "M126_INPUT_HASH")

    def test_additions_are_not_wired_into_fdd(self):
        hdd = (TOOLS / "build-sasi-utility-disks.sh").read_text()
        fdd = (TOOLS / "build-utility-disk.sh").read_text()
        common = (TOOLS / "stage-development-tools.sh").read_text()
        self.assertIn('"$script_dir/stage-softlib-additions.py"', hdd)
        self.assertNotIn("stage-softlib-additions.py", fdd + common)
        self.assertEqual(hdd.count('--supplemental-tree "$supplemental_tree"'), 2)


@unittest.skipUnless(all((CACHE / name).is_file() for name in STAGE.INPUTS)
                     and shutil.which("dosbox") and shutil.which("lha"),
                     "requires cached public packages, DOSBox and Lhasa")
class PackagingIntegrationTests(unittest.TestCase):
    def test_reproducible_build_and_synthetic_hdd_readback(self):
        with tempfile.TemporaryDirectory() as temp:
            work = Path(temp)
            first = STAGE.stage(CACHE, work / "first")
            second = STAGE.stage(CACHE, work / "second")
            self.assertEqual(first, second)
            for name, expected in STAGE.OUTPUTS.items():
                self.assertEqual(hashlib.sha256(first[f"BIN/{name}"]).hexdigest(), expected)
            self.assertEqual(first["ARCHIVE/VA3DDEMO.ASM"],
                             (CACHE / "VA3DDEMO.ASM").read_bytes())
            self.assertEqual(first["ARCHIVE/PCODESRC.LZH"],
                             (CACHE / "PCODESRC.LZH").read_bytes())
            for name in STAGE.TENNIS_FILES:
                self.assertGreater(len(first[f"TENIM3/{name}"]), 0)
            # All media and system files here are synthetic; no private input.
            source = work / "source.d88"
            source.write_bytes(MEDIA.FIXTURE.make_pcengine_fixture())
            manifest = work / "manifest.tsv"
            manifest.write_text("".join(
                f"sasi\t{name}\t{hashlib.sha256(data).hexdigest()}\t{len(data)}\n"
                for name, data in first.items()), encoding="ascii")
            # The existing synthetic system fixture models PC-Engine 1.1.
            for variant in ("va2",):
                output = work / f"{variant}.hdi"
                MEDIA.SASI.build(source, output, variant,
                                 supplemental_tree=work / "first",
                                 supplemental_manifest=manifest)
                image = output.read_bytes()
                for name, contents in first.items():
                    self.assertEqual(MEDIA.hdi_file(image, name), contents, name)


if __name__ == "__main__":
    unittest.main()
