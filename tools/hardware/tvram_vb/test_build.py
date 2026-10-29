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

"""Focused source-transform/content tests, independent of Git or hardware."""
import copy
import importlib.util
from pathlib import Path
import tempfile
import unittest

spec = importlib.util.spec_from_file_location("tvram_vb_build", Path(__file__).with_name("build.py"))
build = importlib.util.module_from_spec(spec)
spec.loader.exec_module(build)


class ProbeBuild(unittest.TestCase):
    def assert_code(self, code, function, *args):
        with self.assertRaises(build.ProbeError) as error:
            function(*args)
        self.assertEqual(str(error.exception), code)

    def test_source(self):
        source = (build.DEMO / "tspspr.asm").read_text()
        for size in build.SIZES:
            for edge in (False, True):
                name, changed = build.variant(source, size, edge)
                self.assertIn("'%s 5 MEASURE '" % name, changed)
                self.assertIn("mov\tcx,%d" % (size // 2), changed)
                self.assertEqual(changed.count("times 5 nop"), 0 if edge else 1)
                self.assertEqual(changed.count("jnz\tshort %%fall"), 1 if edge else 0)
                self.assertEqual(changed.count("VB %03dB" % size), 3)
        build.variant(source, 128, False)  # passing fixture before each mutation
        self.assert_code("M142_TVVB_CHUNK", build.variant, source, 0, False)
        self.assert_code("M142_TVVB_ENTRY", build.variant, source, 128, "E")
        altered = source.replace("%macro VB_CHUNKS 1", "%macro VB_CHUNKS 2")
        self.assert_code("M142_TVVB_SOURCE_ANCHOR", build.variant, altered, 128, False)
        altered = source.replace("'Q TV VB 128B 31   '", "'Q TV VB 128B 30   '")
        self.assert_code("M142_TVVB_SOURCE_ANCHOR", build.variant, altered, 128, False)

    def test_manifest(self):
        with tempfile.TemporaryDirectory() as name:
            root = Path(name)
            programs = root / "payload/root"
            programs.mkdir(parents=True)
            names = ["TSPSPR.COM"] + ["V%03d%s.COM" % (n, mode) for n in build.SIZES for mode in "AE"]
            for file in names:
                (programs / file).write_bytes(b"independent fake fixture")
            passing = {"schema": "m142-tvram-vb-v1", "programs": {
                name: {"sha256": build.sha(programs / name)} for name in names}}
            build.validate_manifest(root, passing)
            data = copy.deepcopy(passing)
            data["schema"] = "unknown"
            self.assert_code("M142_TVVB_SCHEMA", build.validate_manifest, root, data)
            data = copy.deepcopy(passing)
            del data["programs"][names[0]]
            self.assert_code("M142_TVVB_PROGRAM_SET", build.validate_manifest, root, data)
            data = copy.deepcopy(passing)
            data["programs"][names[0]]["sha256"] = "wrong"
            self.assert_code("M142_TVVB_PROGRAM_HASH", build.validate_manifest, root, data)


if __name__ == "__main__":
    unittest.main()
