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
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
# INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
# STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
# IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.

"""Single-mutation release-content tests; no current Git topology dependency."""
import importlib.util
import pathlib
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[3]
SPEC = importlib.util.spec_from_file_location(
    "release_contract", ROOT / "tools/release/check-recording-release.py")
CONTRACT = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(CONTRACT)


class ReleaseRecording(unittest.TestCase):
    def setUp(self):
        self.workflow = (ROOT / ".github/workflows/release.yml").read_text()
        self.builder = (ROOT / "tools/release/build-recording-ffmpeg.sh").read_text()
        CONTRACT.validate_content(self.workflow, self.builder)

    def mutation(self, old, new, code, builder=False):
        original = self.builder if builder else self.workflow
        self.assertIn(old, original)
        changed = original.replace(old, new, 1)
        with self.assertRaises(CONTRACT.ContractError) as error:
            CONTRACT.validate_content(self.workflow if builder else changed,
                                      changed if builder else self.builder)
        self.assertEqual(str(error.exception), code)

    def test_complete_release_contract(self):
        CONTRACT.validate_content(self.workflow, self.builder)

    def test_recording_disabled(self):
        self.mutation("-DVAEG_ENABLE_RECORDING=ON", "-DVAEG_ENABLE_RECORDING=OFF",
                      "RELEASE_RECORDING_DISABLED")

    def test_shared_ffmpeg(self):
        self.mutation("-DVAEG_STATIC_FFMPEG=ON", "-DVAEG_STATIC_FFMPEG=OFF",
                      "RELEASE_RECORDING_NOT_STATIC")

    def test_public_marker_missing(self):
        self.mutation("-DVAEG_PUBLIC_BUILD=ON", "", "RELEASE_RECORDING_NOT_PUBLIC")

    def test_local_encoders_enabled(self):
        self.mutation("-DVAEG_ENABLE_LOCAL_MP4_ENCODERS=OFF",
                      "-DVAEG_ENABLE_LOCAL_MP4_ENCODERS=ON",
                      "RELEASE_RECORDING_LOCAL_ENCODERS")

    def test_sdk_not_built(self):
        self.mutation("bash tools/release/build-recording-ffmpeg.sh build/recording-sdk",
                      "true", "RELEASE_RECORDING_SDK")

    def test_wrong_pkgconfig_prefix(self):
        self.mutation('PKG_CONFIG_PATH="$PWD/build/recording-sdk/stage/lib/pkgconfig"',
                      "", "RELEASE_RECORDING_PREFIX")

    def test_sources_not_staged(self):
        self.mutation('bash tools/release/stage-recording-assets.sh build/recording-sdk "dist-package/${package}"',
                      "true", "RELEASE_RECORDING_SOURCES")

    def test_extra_encoder(self):
        self.mutation("--enable-encoder=ffv1,pcm_s16le", "--enable-encoder=ffv1,pcm_s16le,aac",
                      "RELEASE_RECORDING_CODEC_SET", builder=True)

    def test_autodetect_enabled(self):
        self.mutation("--disable-autodetect", "--enable-autodetect",
                      "RELEASE_RECORDING_SDK_POLICY", builder=True)

    def test_archive_audit_removed(self):
        self.mutation("check_mp4_codec_archive.py", "missing_audit.py",
                      "RELEASE_RECORDING_ARCHIVE_AUDIT", builder=True)

    def test_windows_recording_disabled(self):
        start = self.workflow.index("  windows:\n")
        prefix, job = self.workflow[:start], self.workflow[start:]
        self.workflow = prefix + job.replace("-DVAEG_ENABLE_RECORDING=ON",
                                            "-DVAEG_ENABLE_RECORDING=OFF", 1)
        with self.assertRaises(CONTRACT.ContractError) as error:
            CONTRACT.validate_content(self.workflow, self.builder)
        self.assertEqual(str(error.exception), "RELEASE_RECORDING_DISABLED")

    def test_macos_recording_disabled(self):
        start = self.workflow.index("  macos:\n")
        self.workflow = self.workflow[:start] + self.workflow[start:].replace(
            "-DVAEG_ENABLE_RECORDING=ON", "-DVAEG_ENABLE_RECORDING=OFF", 1)
        with self.assertRaises(CONTRACT.ContractError) as error:
            CONTRACT.validate_content(self.workflow, self.builder)
        self.assertEqual(str(error.exception), "RELEASE_RECORDING_DISABLED")


if __name__ == "__main__":
    unittest.main()
