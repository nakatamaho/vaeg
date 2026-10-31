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

"""Cross-check CMake generation, small content negatives and packaging CLI."""
import copy
import importlib.util
import json
import re
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
HELPER = ROOT / "tools/release/package-identity.py"
spec = importlib.util.spec_from_file_location("package_identity", HELPER)
identity = importlib.util.module_from_spec(spec)
spec.loader.exec_module(identity)
SHA = "2e662105" + "0" * 32


def fixture(milestone):
    dev = milestone.endswith("-dev")
    return {"release": "20261029", "milestone": milestone, "commit": SHA,
            "short_commit": SHA[:8],
            "display": "Rel.20261029 (" + milestone + (", 2e662105" if dev else "") + ")",
            "package_stem": "vaeg-20261029-" + milestone.lower() + ("-g2e662105" if dev else "")}


class ReleaseIdentity(unittest.TestCase):
    def test_cmake_and_cli(self):
        with tempfile.TemporaryDirectory() as name:
            root = Path(name)
            for milestone in ("M141", "M142-dev", "M62b1-dev"):
                with self.subTest(milestone=milestone):
                    script = root / "fixture.cmake"
                    result = root / "result.txt"
                    script.write_text('include("' + (ROOT / "cmake/release_identity.cmake").as_posix() + '")\n'
                                      'vaeg_release_identity("20261029" "' + milestone + '" "' + SHA + '")\n'
                                      'file(WRITE "' + result.as_posix() + '" '
                                      '"${VAEG_BUILD_DISPLAY}\\n${VAEG_PACKAGE_STEM}\\n")\n')
                    subprocess.run(["cmake", "-P", str(script)], check=True, capture_output=True)
                    data = fixture(milestone)
                    self.assertEqual(result.read_text().splitlines(), [data["display"], data["package_stem"]])
                    identity.validate(data)
                    manifest = root / "identity.json"
                    manifest.write_text(json.dumps(data))
                    command = [sys.executable, str(HELPER), "--identity", str(manifest),
                               "--field", "package", "--platform", "windows-x64"]
                    cli = subprocess.run(command, capture_output=True, text=True)
                    self.assertEqual(cli.returncode, 0, cli.stderr)
                    self.assertEqual(cli.stdout.strip(), data["package_stem"] + "-windows-x64")
                    official = subprocess.run(command + ["--tag", "rel-20261029"], capture_output=True, text=True)
                    if milestone.endswith("-dev"):
                        self.assertEqual(official.returncode, 1)
                        self.assertEqual(official.stderr.strip(), "RELEASE_ID_DEVELOPMENT")
                    else:
                        self.assertEqual(official.returncode, 0, official.stderr)

    def test_canonical_cmake_milestones(self):
        with tempfile.TemporaryDirectory() as name:
            script = Path(name) / "canonical.cmake"
            prefix = 'include("' + (ROOT / "cmake/release_identity.cmake").as_posix() + '")\n'
            def evaluate(milestone):
                script.write_text(prefix + 'vaeg_release_identity("20261029" "' + milestone + '" "' + SHA + '")\n')
                return subprocess.run(["cmake", "-P", str(script)], capture_output=True, text=True)
            self.assertEqual(evaluate("M142-dev").returncode, 0)
            for invalid in ("M0142-dev", "M0a", "M62b0", "M62b01", "M62B", "M142-accepted"):
                with self.subTest(milestone=invalid):
                    result = evaluate(invalid)
                    self.assertNotEqual(result.returncode, 0)
                    self.assertEqual(re.findall(r"RELEASE_ID_[A-Z_]+", result.stderr), ["RELEASE_ID_MILESTONE"])
                    data = fixture("M141")
                    identity.validate(data)
                    data["milestone"] = invalid
                    with self.assertRaises(identity.IdentityError) as error:
                        identity.validate(data)
                    self.assertEqual(str(error.exception), "RELEASE_ID_MILESTONE")

    def test_single_mutation_negatives(self):
        passing = fixture("M141")
        identity.validate(passing, "rel-20261029")
        mutations = (("release", "../escape", "RELEASE_ID_DATE"),
                     ("milestone", "M142-accepted", "RELEASE_ID_MILESTONE"),
                     ("commit", "bad", "RELEASE_ID_COMMIT"),
                     ("short_commit", "ffffffff", "RELEASE_ID_SHORT_COMMIT"),
                     ("display", "Rel.20261029 (M140)", "RELEASE_ID_FORMAT"),
                     ("package_stem", "../escape", "RELEASE_ID_FORMAT"))
        for field, value, code in mutations:
            with self.subTest(field=field):
                data = copy.deepcopy(passing)
                data[field] = value
                with self.assertRaises(identity.IdentityError) as error:
                    identity.validate(data, "rel-20261029")
                self.assertEqual(str(error.exception), code)
        with self.assertRaises(identity.IdentityError) as error:
            identity.validate(passing, "rel-20261030")
        self.assertEqual(str(error.exception), "RELEASE_ID_TAG")
        data = copy.deepcopy(passing)
        data["extra"] = "one unexpected field"
        with self.assertRaises(identity.IdentityError) as error:
            identity.validate(data)
        self.assertEqual(str(error.exception), "RELEASE_ID_SCHEMA")

    def test_legacy_record_identity(self):
        # Fixed 16-byte operation-record versions must not receive long UI text.
        header = (ROOT / "np2ver.h").read_text()
        self.assertIn('#define VAEGREL_CORE "Rel." VAEG_BUILD_RELEASE', header)
        self.assertIn('#define VAEGREL_DISPLAY VAEG_BUILD_DISPLAY', header)
        source = (ROOT / "oprecord.c").read_text()
        self.assertIn('char\tvaegrel[16];', source)
        self.assertIn('VAEGREL_CORE', source)
        self.assertNotIn('VAEGREL_DISPLAY', source)


if __name__ == "__main__":
    unittest.main()
