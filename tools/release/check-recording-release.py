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

"""Closed release-job content contract, independent of Git history or assets.

This is not a YAML/shell parser or an archive audit. Keep the actual CMake
configure/prebuild and SDK/staging archive audits as the binary-level checks.
"""
import pathlib
import re
import sys


class ContractError(ValueError):
    pass


def validate_content(workflow, builder):
    if 'gh release create "${RELEASE_TAG}" \\\n              --draft \\' not in workflow:
        raise ContractError("RELEASE_RECORDING_NOT_DRAFT")
    publish = workflow.split("  publish:\n", 1)
    if (len(publish) != 2 or "needs: [linux, windows, macos]" not in publish[1]
            or '--draft=false --latest' not in publish[1]):
        raise ContractError("RELEASE_RECORDING_PUBLISH_GATE")
    for platform in ("linux", "windows", "macos"):
        match = re.search(r"^  " + platform + r":\n(.*?)(?=^  \w+:|\Z)",
                          workflow, re.MULTILINE | re.DOTALL)
        if match is None:
            raise ContractError("RELEASE_RECORDING_JOB")
        job = match.group(1)
        match = re.search(r"      - name: Configure\n(.*?)(?=      - |\Z)",
                          job, re.DOTALL)
        if match is None:
            raise ContractError("RELEASE_RECORDING_CONFIGURE")
        configure = match.group(1)
        if (re.findall(r"-DCMAKE_CXX_STANDARD=([^\s]+)", configure) != ['17'] or
                re.findall(r"-DCMAKE_CXX_STANDARD_REQUIRED=([^\s]+)", configure) != ['ON']):
            raise ContractError("RELEASE_RECORDING_CXX_STANDARD")
        for option, expected, code in (
            ("ENABLE_RECORDING", "ON", "RELEASE_RECORDING_DISABLED"),
            ("STATIC_FFMPEG", "ON", "RELEASE_RECORDING_NOT_STATIC"),
            ("PUBLIC_BUILD", "ON", "RELEASE_RECORDING_NOT_PUBLIC"),
            ("ENABLE_LOCAL_MP4_ENCODERS", "OFF", "RELEASE_RECORDING_LOCAL_ENCODERS"),
        ):
            if re.findall(r"-DVAEG_" + option + r"=([^\s]+)", configure) != [expected]:
                raise ContractError(code)
        sdk = "bash tools/release/build-recording-ffmpeg.sh build/recording-sdk"
        if platform == "macos":
            if "brew install cmake ninja nasm pkgconf bash" not in job:
                raise ContractError("RELEASE_RECORDING_MACOS_BASH")
            sdk = '\'"$(brew --prefix)/bin/bash" tools/release/build-recording-ffmpeg.sh build/recording-sdk\''
        if sdk not in job.split("      - name: Configure", 1)[0]:
            raise ContractError("RELEASE_RECORDING_SDK")
        if 'PKG_CONFIG_PATH="$PWD/build/recording-sdk/stage/lib/pkgconfig"' not in configure:
            raise ContractError("RELEASE_RECORDING_PREFIX")
        if 'bash tools/release/stage-recording-assets.sh build/recording-sdk "dist-package/${package}"' not in job:
            raise ContractError("RELEASE_RECORDING_SOURCES")
    for option, expected in (("encoder", "ffv1,pcm_s16le"),
                             ("decoder", "ffv1,pcm_s16le"),
                             ("muxer", "matroska"), ("demuxer", "matroska"),
                             ("protocol", "file")):
        if re.findall(r"--enable-" + option + r"=([^\s]+)", builder) != [expected]:
            raise ContractError("RELEASE_RECORDING_CODEC_SET")
    for flag in ("--disable-gpl", "--disable-nonfree", "--disable-version3",
                 "--disable-autodetect", "--disable-network", "--disable-everything"):
        if flag not in builder:
            raise ContractError("RELEASE_RECORDING_SDK_POLICY")
    if "check_mp4_codec_archive.py" not in builder or "--local-only" in builder:
        raise ContractError("RELEASE_RECORDING_ARCHIVE_AUDIT")


def main():
    root = pathlib.Path(__file__).resolve().parents[2]
    try:
        validate_content((root / ".github/workflows/release.yml").read_text(),
                         (root / "tools/release/build-recording-ffmpeg.sh").read_text())
    except ContractError as error:
        print(str(error), file=sys.stderr)
        return 1
    print("release-recording-content: PASS (three public static recording jobs)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
