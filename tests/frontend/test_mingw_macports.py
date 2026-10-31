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

"""Small, host-independent SDK identity fixtures; never build codecs in CI."""
import copy
import importlib.util
from pathlib import Path
import tempfile
import unittest

spec = importlib.util.spec_from_file_location("mingw_macports", Path(__file__).resolve().parents[2] /
                                             "tools/release/build-mingw-macports.py")
build = importlib.util.module_from_spec(spec)
spec.loader.exec_module(build)


class SdkIdentity(unittest.TestCase):
    def assert_code(self, code, function, *args):
        with self.assertRaises(RuntimeError) as error:
            function(*args)
        self.assertEqual(str(error.exception).split(":", 1)[0], code)

    def test_shader(self):
        with tempfile.TemporaryDirectory() as name:
            root = Path(name)
            archive, notices = root / "shader.a", root / "notices.txt"
            archive.write_bytes(b"fixture, not an executable library")
            notices.write_text("fixture license")
            passing = {"source_revision": build.SHADER_PIN, "target": build.RUST_TARGET,
                       "feature": "runtime-d3d11", "archive_sha256": build.digest(archive),
                       "notices_sha256": build.digest(notices), "unresolved_notices": []}
            build.validate_shader(passing, archive, notices)
            for field, value in (("source_revision", "wrong"), ("target", "wrong"),
                                 ("feature", "runtime-metal"), ("archive_sha256", "wrong"),
                                 ("notices_sha256", "wrong"), ("unresolved_notices", ["missing"])):
                with self.subTest(field=field):
                    data = copy.deepcopy(passing)
                    data[field] = value
                    self.assert_code("MACPORTS_SHADER_IDENTITY", build.validate_shader,
                                     data, archive, notices)

    def test_ffmpeg(self):
        with tempfile.TemporaryDirectory() as name:
            root = Path(name)
            (root / "lib").mkdir()
            paths = ["lib/lib" + component + ".a" for component in
                     ("avcodec", "avformat", "avutil", "swresample")]
            for path in paths:
                (root / path).write_bytes(b"fixture only")
            identity = {"source_sha256": build.FFMPEG_HASH, "compiler": "fixture compiler"}
            passing = {"identity": identity, "archives": {p: build.digest(root / p) for p in paths}}
            build.validate_ffmpeg(passing, identity, root)
            for mutation in ("compiler", "hash", "missing", "empty", "traversal"):
                with self.subTest(mutation=mutation):
                    data = copy.deepcopy(passing)
                    if mutation == "compiler":
                        data["identity"]["compiler"] = "different compiler"
                    elif mutation == "hash":
                        data["archives"][paths[0]] = "wrong"
                    elif mutation == "missing":
                        del data["archives"][paths[0]]
                    elif mutation == "empty":
                        data["archives"] = {}
                    else:
                        data["archives"]["lib/../../escape.a"] = "wrong"
                    self.assert_code("MACPORTS_FFMPEG_IDENTITY", build.validate_ffmpeg,
                                     data, identity, root)

    def test_download(self):
        with tempfile.TemporaryDirectory() as name:
            archive = Path(name) / "archive"
            archive.write_bytes(b"cached fixture")
            build.download("https://invalid.invalid/not-used", archive, build.digest(archive))
            self.assert_code("MACPORTS_DOWNLOAD_HASH", build.download,
                             "https://invalid.invalid/not-used", archive, "wrong")


if __name__ == "__main__":
    unittest.main()
