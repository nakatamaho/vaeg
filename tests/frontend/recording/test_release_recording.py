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
import ast
import importlib.util
import pathlib
import re
import textwrap
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

    def test_no_partial_publication(self):
        self.mutation("              --draft \\\n", "",
                      "RELEASE_RECORDING_NOT_DRAFT")

    def test_all_platforms_required(self):
        self.mutation("needs: [linux, windows, macos]", "needs: [linux]",
                      "RELEASE_RECORDING_PUBLISH_GATE")

    def test_cpp_dialect_not_explicit(self):
        self.mutation("-DCMAKE_CXX_STANDARD=17", "-DCMAKE_CXX_STANDARD=20",
                      "RELEASE_RECORDING_CXX_STANDARD")

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

    def test_macos_requires_modern_bash(self):
        self.mutation("brew install cmake ninja nasm pkgconf bash",
                      "brew install cmake ninja nasm pkgconf",
                      "RELEASE_RECORDING_MACOS_BASH")

    def test_macos_recording_disabled(self):
        start = self.workflow.index("  macos:\n")
        self.workflow = self.workflow[:start] + self.workflow[start:].replace(
            "-DVAEG_ENABLE_RECORDING=ON", "-DVAEG_ENABLE_RECORDING=OFF", 1)
        with self.assertRaises(CONTRACT.ContractError) as error:
            CONTRACT.validate_content(self.workflow, self.builder)
        self.assertEqual(str(error.exception), "RELEASE_RECORDING_DISABLED")


class FollowupEvidence(unittest.TestCase):
    def setUp(self):
        source = (ROOT / ".github/workflows/release-macos-followup.yml").read_text()
        self.functions = {}
        blocks = re.findall(r"          python3 - <<'PY'\n(.*?)          PY", source, re.S)
        blocks += re.findall(r"          @'\n(.*?)          '@ \| python -", source, re.S)
        definitions = {}
        for block in blocks:
            tree = ast.parse(textwrap.dedent(block))
            for node in tree.body:
                if isinstance(node, ast.FunctionDef):
                    fingerprint = ast.dump(node)
                    if node.name in definitions:
                        self.assertEqual(definitions[node.name], fingerprint)
                    definitions[node.name] = fingerprint
            # Load only independently callable content layers, never their CLI I/O.
            nodes = [node for node in tree.body
                     if isinstance(node, (ast.Import, ast.ImportFrom, ast.FunctionDef))]
            exec(compile(ast.Module(body=nodes, type_ignores=[]), "followup-content", "exec"),
                 self.functions)
        self.sha = "4fb3c44052fcccab7e47830ca1945d1498469bb2"
        self.run = {"head_sha": self.sha}
        self.jobs = {"jobs": [
            {"name": "linux gcc release", "conclusion": "success"},
            {"name": "windows mingw native static-runtime", "conclusion": "failure"},
            {"name": "macos fetch-sdl2 release", "conclusion": "failure"}]}
        self.release = {"isDraft": True}
        self.assets = {"assets": [{"name": "vaeg-rel20261025" + suffix} for suffix in (
            "-linux-x86_64.tar.gz", "-win-x86_64.zip", "-macos-arm64.tar.gz")]}
        self.retained_jobs = {'jobs': [
            {'name': 'macos tagged-source Bash follow-up', 'conclusion': 'success'}]}
        self.retained_release = {'isDraft': True, 'assets': [a.copy() for a in self.assets['assets']]}
        self.retained_release['assets'][-1]['digest'] = 'sha256:b08539689269f7c49f72e3599ce2693388373ab1d58280c1522c28dde834f02e'
        self.validate()

    def validate(self):
        self.functions['validate_evidence'](self.sha, self.run, self.jobs, self.release)
        self.functions['validate_assets']('rel-20261025', self.assets)
        self.functions['validate_retained_macos'](self.sha, self.retained_jobs, self.retained_release)
        self.functions['validate_assets']('rel-20261025', self.retained_release, True)

    def assert_code(self, code):
        with self.assertRaises(SystemExit) as error:
            self.validate()
        self.assertEqual(error.exception.code, code)

    def test_complete_followup_contract(self):
        self.validate()

    def test_source_mismatch(self):
        self.run['head_sha'] = 'b' * 40
        self.assert_code('RELEASE_FOLLOWUP_SOURCE_MISMATCH')

    def test_not_draft(self):
        self.release['isDraft'] = False
        self.assert_code('RELEASE_FOLLOWUP_NOT_DRAFT')

    def test_failed_retained_job(self):
        self.jobs['jobs'][0]['conclusion'] = 'failure'
        self.assert_code('RELEASE_FOLLOWUP_JOB_EVIDENCE')

    def test_missing_retained_job(self):
        self.jobs['jobs'].pop()
        self.assert_code('RELEASE_FOLLOWUP_JOB_EVIDENCE')

    def test_missing_package(self):
        self.assets['assets'].pop()
        self.assert_code('RELEASE_FOLLOWUP_ASSET_SET')

    def test_retained_source_scope(self):
        with self.assertRaises(SystemExit) as error:
            self.functions['validate_retained_macos']('a' * 40, self.retained_jobs, self.retained_release)
        self.assertEqual(error.exception.code, 'RELEASE_FOLLOWUP_RETAINED_SOURCE')

    def test_retained_job_failed(self):
        self.retained_jobs['jobs'][0]['conclusion'] = 'failure'
        self.assert_code('RELEASE_FOLLOWUP_RETAINED_JOB')

    def test_retained_asset_digest(self):
        self.retained_release['assets'][-1]['digest'] = 'sha256:' + '0' * 64
        self.assert_code('RELEASE_FOLLOWUP_RETAINED_ASSET')

    def test_retained_publish_digest(self):
        self.retained_release['assets'][-1]['digest'] = 'sha256:' + '0' * 64
        with self.assertRaises(SystemExit) as error:
            self.functions['validate_assets']('rel-20261025', self.retained_release, True)
        self.assertEqual(error.exception.code, 'RELEASE_FOLLOWUP_RETAINED_ASSET')


if __name__ == "__main__":
    unittest.main()
