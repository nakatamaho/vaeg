#
# Copyright (c) 2026 Nakata Maho
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
# OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
# USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
# ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
# THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

"""Exercise static archive resolution independently of host packages/history."""
import hashlib
import pathlib
import re
import shutil
import subprocess
import tempfile
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[3]


class StaticFFmpeg(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix="vaeg-static-ffmpeg-")
        self.addCleanup(self.temp.cleanup)
        self.root = pathlib.Path(self.temp.name)
        for name in ("avformat", "avcodec", "avutil"):
            for suffix in ("a", "dylib", "so"):
                (self.root / ("lib" + name + "." + suffix)).touch()
        self.flags = "-lavformat;-lavcodec;-lavutil;-pthread;-lm;-framework;CoreVideo"

    def configure(self):
        (self.root / "CMakeLists.txt").write_text(f'''cmake_minimum_required(VERSION 3.20)
project(static_fixture C)
set(CMAKE_STATIC_LIBRARY_SUFFIX ".a")
set(P_STATIC_LIBRARY_DIRS "{self.root.as_posix()}")
set(P_STATIC_LDFLAGS "{self.flags}")
include("{(ROOT / 'cmake/ffmpeg_static.cmake').as_posix()}")
vaeg_ffmpeg_static_target(fixture P)
get_target_property(libs fixture INTERFACE_LINK_LIBRARIES)
file(WRITE "${{CMAKE_BINARY_DIR}}/resolved.txt" "${{libs}}")
''')
        return subprocess.run(["cmake", "-S", str(self.root), "-B",
                               str(self.root / "build")], capture_output=True, text=True)

    def valid(self):
        result = self.configure()
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        resolved = (self.root / "build/resolved.txt").read_text().split(";")
        self.assertEqual(resolved, [str(self.root / ("lib" + n + ".a"))
                                   for n in ("avformat", "avcodec", "avutil")]
                         + ["-pthread", "-lm", "-framework CoreVideo"])

    def assert_code(self, result, expected):
        self.assertNotEqual(result.returncode, 0)
        codes = re.findall(r"(VAEG_FFMPEG_[A-Z_]+):", result.stdout + result.stderr)
        self.assertEqual(codes, [expected], result.stdout + result.stderr)

    def test_desktop_dependency_defaults(self):
        source = (ROOT / "CMakeLists.txt").read_text()
        options = source.split("set(VAEG_DESKTOP_STATIC_DEFAULT OFF)", 1)[1]
        options = "set(VAEG_DESKTOP_STATIC_DEFAULT OFF)" + options.split(
            "option(VAEG_Z80_COMPAT_INTEGRATION_TRACE", 1)[0]
        for platform in ("windows", "macos", "linux"):
            with self.subTest(platform=platform):
                script = self.root / (platform + ".cmake")
                settings = (f"set(WIN32 {'ON' if platform == 'windows' else 'OFF'})\n"
                            f"set(APPLE {'ON' if platform == 'macos' else 'OFF'})\n")
                expected = "OFF" if platform == "linux" else "ON"
                checks = ""
                for name in ("FETCH_SDL2", "STATIC_SDL2", "FETCH_LIBARCHIVE",
                             "STATIC_FFMPEG", "STATIC_LIBRASHADER"):
                    checks += (f'if(NOT "${{VAEG_{name}}}" STREQUAL "{expected}")\n'
                               f' message(FATAL_ERROR "DEFAULT_MISMATCH: {name}")\n'
                               'endif()\n')
                script.write_text(settings + options + checks)
                result = subprocess.run(["cmake", "-P", str(script)],
                                        capture_output=True, text=True)
                self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    def test_exact_archive_and_private_link_order(self):
        self.valid()

    def test_missing_archive_does_not_use_shared(self):
        self.valid()
        (self.root / "libavcodec.a").unlink()
        self.assert_code(self.configure(), "VAEG_FFMPEG_STATIC_ARCHIVE_MISSING")

    def test_incomplete_framework_metadata_fails_closed(self):
        self.valid()
        self.flags += ";-framework"
        self.assert_code(self.configure(), "VAEG_FFMPEG_STATIC_METADATA_MISSING")

    def test_empty_metadata_fails_closed(self):
        self.valid()
        self.flags = ""
        self.assert_code(self.configure(), "VAEG_FFMPEG_STATIC_METADATA_MISSING")


@unittest.skipUnless(shutil.which("bash") and shutil.which("sha256sum"),
                     "requires Bash and sha256sum for the container recipe")
class MinGWRecordingRecipe(unittest.TestCase):
    def test_recording_default_and_explicit_opt_out(self):
        driver = (ROOT / "tools/release/build-mingw-static-container.sh").read_text()
        # Execute the real option parser, stopping before Git/Docker/build work.
        parser = driver.split('commit="$(git -C ', 1)[0]
        self.assertNotEqual(parser, driver)
        with tempfile.TemporaryDirectory() as temp:
            path = pathlib.Path(temp) / "parser.sh"
            path.write_text(parser + '\nprintf "%s" "$recording"\n')
            for arguments, expected in (([], "ON"), (["--no-recording"], "OFF")):
                result = subprocess.run(["bash", path.as_posix(), *arguments],
                                        text=True, capture_output=True)
                self.assertEqual(result.returncode, 0, result.stderr)
                self.assertEqual(result.stdout, expected)
        self.assertIn('-DVAEG_ENABLE_RECORDING="${ENABLE_RECORDING}"', driver)
        self.assertIn('-DVAEG_STATIC_FFMPEG=ON', driver)
        self.assertIn('ff_ffv1_encoder ff_pcm_s16le_encoder ff_matroska_muxer', driver)

    def test_cache_content_validation_before_single_mutation(self):
        helper = ROOT / "tools/release/build-mingw-static-ffmpeg.sh"
        with tempfile.TemporaryDirectory() as temp:
            root = pathlib.Path(temp)
            payload = root / "libavcodec.a"
            payload.write_bytes(b"valid static fixture")
            digest = hashlib.sha256(payload.read_bytes()).hexdigest()
            (root / "manifest.sha256").write_text(f"{digest}  libavcodec.a\n")

            def validate():
                return subprocess.run([
                    "bash", "-c", 'source "$1"; ffmpeg_verify_cache "$2"',
                    "cache-test", helper.as_posix(), root.as_posix()], text=True, capture_output=True)

            result = validate()
            self.assertEqual(result.returncode, 0, result.stderr)
            payload.write_bytes(b"Valid static fixture")
            result = validate()
            self.assertNotEqual(result.returncode, 0)
            codes = re.findall(r"^VAEG_FFMPEG_[A-Z_]+$", result.stderr, re.MULTILINE)
            self.assertEqual(codes, ["VAEG_FFMPEG_CACHE_HASH_MISMATCH"], result.stderr)


if __name__ == "__main__":
    unittest.main()
