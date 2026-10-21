#!/usr/bin/env python3
# Copyright (c) 2026 Nakata Maho
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
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
"""Small content/build-policy layers, no Git history or large evidence corpus."""
import importlib.util
import json
import os
from pathlib import Path
import re
import runpy
import shutil
import subprocess
import sys
import tempfile
import unittest

# Fixture imports must not generate bytecode inside the source checkout.
sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[3]
SPEC = importlib.util.spec_from_file_location("audit", ROOT / "tools/repo/check_mp4_codec_archive.py")
AUDIT = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(AUDIT)
CI_NAMES = ("CI", "GITHUB_ACTIONS", "GITLAB_CI", "TF_BUILD", "BUILD_NUMBER", "JENKINS_URL",
            "TEAMCITY_VERSION", "BUILDKITE", "CIRCLECI", "TRAVIS", "APPVEYOR", "VAEG_PUBLIC_PRESET")
BASELINE = {"VAEG_ENABLE_LOCAL_MP4_ENCODERS": "ON", "VAEG_ENABLE_RECORDING": "ON",
            "VAEG_STATIC_FFMPEG": "ON", "VAEG_PUBLIC_BUILD": "OFF", "APPLE": "FALSE", "WIN32": "FALSE",
            "VAEG_LOCAL_MP4_ACKNOWLEDGEMENT": "LOCAL_ONLY_NO_DISTRIBUTION"}
SYMBOLS = "00000000 D ff_ffv1_encoder\n00000001 D ff_pcm_s16le_encoder\n"


def clean_environment():
    # Only child processes simulate a local environment; never bypass production policy.
    return {k: v for k, v in os.environ.items() if k not in CI_NAMES}


def policy(values, environment=None):
    with tempfile.TemporaryDirectory(prefix="vaeg-mp4-policy-") as tmp:
        script = Path(tmp) / "policy.cmake"
        script.write_text("cmake_minimum_required(VERSION 3.20)\n" + "\n".join(f'set({k} "{v}" CACHE STRING "fixture" FORCE)' for k, v in values.items()) +
                          f'\ninclude("{ROOT}/cmake/mp4_policy.cmake")\n'
                          'message(STATUS "MP4_POLICY_OK ${VAEG_ENABLE_LOCAL_MP4_ENCODERS}")\n')
        env = clean_environment()
        env.update(environment or {})
        return subprocess.run(["cmake", "-P", str(script)], env=env, capture_output=True, text=True)


class BuildPolicy(unittest.TestCase):
    def mutate(self, code, values=None, environment=None):
        baseline = policy(BASELINE)
        self.assertEqual(baseline.returncode, 0, baseline.stderr)
        self.assertIn("MP4_POLICY_OK ON", baseline.stdout)
        updated = dict(BASELINE)
        updated.update(values or {})
        result = policy(updated, environment)
        self.assertNotEqual(result.returncode, 0)
        self.assertEqual(re.findall(r"\bMP4_LOCAL_[A-Z_]+\b", result.stderr), [code])

    def test_default_off(self):
        result = policy({})
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("MP4_POLICY_OK OFF", result.stdout)

    def test_ci(self):
        for name in CI_NAMES[:-1]:
            with self.subTest(name=name):
                self.mutate("MP4_LOCAL_CI_FORBIDDEN", environment={name: "true"})

    def test_public(self):
        self.mutate("MP4_LOCAL_PUBLIC_FORBIDDEN", {"VAEG_PUBLIC_BUILD": "ON"})

    def test_preset_even_with_public_override(self):
        self.mutate("MP4_LOCAL_PRESET_FORBIDDEN", environment={"VAEG_PUBLIC_PRESET": "linux-release"})

    def test_acknowledgement(self):
        self.mutate("MP4_LOCAL_ACK_REQUIRED", {"VAEG_LOCAL_MP4_ACKNOWLEDGEMENT": ""})

    def test_static(self):
        self.mutate("MP4_LOCAL_STATIC_RECORDING_REQUIRED", {"VAEG_STATIC_FFMPEG": "OFF"})

    def test_recording(self):
        self.mutate("MP4_LOCAL_STATIC_RECORDING_REQUIRED", {"VAEG_ENABLE_RECORDING": "OFF"})

    def test_native_preferred(self):
        self.mutate("MP4_LOCAL_LINUX_ONLY", {"APPLE": "TRUE"})

    def test_objective_cpp_format_scope(self):
        formatter = runpy.run_path(str(ROOT / "tools/repo/clang_format.py"))
        self.assertIn(".mm", formatter["SOURCE_SUFFIXES"])
        self.assertIn("sdl2/recording/recording_mp4_macos.mm", formatter["load_manifest"]())

    def test_codec_mutation_formats(self):
        mutate = runpy.run_path(str(ROOT / "tests/frontend/recording/test_mp4_export.py"))["wrong_video_codec"]
        direct = b"prefixV_FFV1suffix"
        self.assertEqual(mutate(direct), b"prefixV_VP80suffix")
        for extra in (0, 42):
            vfw = (b"V_MS/VFW/FOURCC" + (40 + extra).to_bytes(4, "little") + bytes(8) +
                   (1).to_bytes(2, "little") + (24).to_bytes(2, "little") + b"FFV1" + bytes(20 + extra))
            self.assertEqual(mutate(vfw), vfw.replace(b"FFV1", b"VP80"))

    def test_release_assertion_guard(self):
        source = (ROOT / "tests/frontend/recording/test_recording_ffmpeg_video_encoder.cpp").read_text()
        guard = "#ifdef NDEBUG" + source.split("#ifdef NDEBUG", 1)[1].split("#endif", 1)[0] + "#endif\n"
        with tempfile.TemporaryDirectory(prefix="vaeg-recording-assertions-") as tmp:
            fixture = Path(tmp) / "guard.c"
            fixture.write_text(guard + "int main(void) { return 0; }\n")
            command = ["cc", "-c", str(fixture), "-o", str(Path(tmp) / "guard.o")]
            baseline = subprocess.run(command, capture_output=True, text=True)
            self.assertEqual(baseline.returncode, 0, baseline.stderr)
            # Exactly one mutation: define NDEBUG; no SDK/history dependency.
            result = subprocess.run(command + ["-DNDEBUG"], capture_output=True, text=True)
            self.assertNotEqual(result.returncode, 0)
            self.assertEqual(set(re.findall(r"RECORDING_[A-Z_]+", result.stderr)),
                             {"RECORDING_TEST_ASSERTIONS_REQUIRED"})

    def test_public_presets(self):
        presets = {p["name"]: p for p in json.loads((ROOT / "CMakePresets.json").read_text())["configurePresets"]}
        def resolve(name):
            p = presets[name]
            inherits = p.get("inherits", [])
            if isinstance(inherits, str):
                inherits = [inherits]
            result = {"cacheVariables": {}, "environment": {}}
            for parent in reversed(inherits):
                inherited = resolve(parent)
                for key in result:
                    result[key].update(inherited[key])
            for key in result:
                result[key].update(p.get(key, {}))
            return result
        for name in presets:
            if "ci" in name.split("-") or name.endswith("-release") or name == "mingw-cross":
                with self.subTest(preset=name):
                    resolved = resolve(name)
                    self.assertEqual(resolved["cacheVariables"]["VAEG_PUBLIC_BUILD"], "ON")
                    self.assertEqual(resolved["cacheVariables"]["VAEG_ENABLE_LOCAL_MP4_ENCODERS"], "OFF")
                    self.assertEqual(resolved["environment"]["VAEG_PUBLIC_PRESET"], "${presetName}")


class ArchiveContent(unittest.TestCase):
    def test_public(self):
        self.assertEqual(set(AUDIT.validate_symbols(SYMBOLS + "00000010 T ff_init_range_encoder\n")), AUDIT.BASE)

    def test_forbidden(self):
        AUDIT.validate_symbols(SYMBOLS)
        with self.assertRaises(AUDIT.Invalid) as raised:
            AUDIT.validate_symbols(SYMBOLS + "00000010 D ff_aac_encoder\n")
        self.assertEqual(raised.exception.code, "MP4_STATIC_ENCODER_FORBIDDEN")

    def test_missing(self):
        AUDIT.validate_symbols(SYMBOLS)
        with self.assertRaises(AUDIT.Invalid) as raised:
            AUDIT.validate_symbols(SYMBOLS.replace("00000000 D ff_ffv1_encoder\n", ""))
        self.assertEqual(raised.exception.code, "MP4_STATIC_ENCODER_REQUIRED")

    def test_unreadable(self):
        AUDIT.validate_symbols(SYMBOLS)
        with self.assertRaises(AUDIT.Invalid) as raised:
            AUDIT.validate_symbols("")
        self.assertEqual(raised.exception.code, "MP4_STATIC_SYMBOLS_UNREADABLE")

    def test_macho_and_undefined(self):
        self.assertEqual(set(AUDIT.validate_symbols(SYMBOLS.replace("ff_", "_ff_") +
                         "                 U _ff_aac_encoder\n")), AUDIT.BASE)

    def test_local(self):
        valid = SYMBOLS + "00000002 D ff_aac_encoder\n00000003 D ff_libx264_encoder\n"
        self.assertEqual(set(AUDIT.validate_symbols(valid, True)), AUDIT.BASE | AUDIT.LOCAL)
        with self.assertRaises(AUDIT.Invalid) as raised:
            AUDIT.validate_symbols(valid + "00000004 D ff_other_encoder\n", True)
        self.assertEqual(raised.exception.code, "MP4_STATIC_ENCODER_FORBIDDEN")

    def test_archive_entry_integration(self):
        with tempfile.TemporaryDirectory(prefix="vaeg-mp4-archive-") as tmp:
            tmp = Path(tmp)
            def object_file(name, code):
                source, obj = tmp / (name + ".c"), tmp / (name + ".o")
                source.write_text(code)
                subprocess.run(["cc", "-c", str(source), "-o", str(obj)], check=True, capture_output=True)
                return obj
            archive = tmp / "libavcodec.a"
            baseline = object_file("base", "int ff_ffv1_encoder=1; int ff_pcm_s16le_encoder=1;\n")
            subprocess.run(["ar", "rcs", str(archive), str(baseline)], check=True, capture_output=True)
            command = [sys.executable, str(ROOT / "tools/repo/check_mp4_codec_archive.py"),
                       "--nm", shutil.which("nm"), "--archive", str(archive)]
            result = subprocess.run(command, capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            # One controlled archive mutation: append one forbidden encoder object.
            extra = object_file("extra", "int ff_aac_encoder=1;\n")
            subprocess.run(["ar", "r", str(archive), str(extra)], check=True, capture_output=True)
            result = subprocess.run(command, capture_output=True, text=True)
            self.assertNotEqual(result.returncode, 0)
            self.assertEqual(result.stdout.split(":", 1)[0], "MP4_STATIC_ENCODER_FORBIDDEN")


if __name__ == "__main__":
    unittest.main()
