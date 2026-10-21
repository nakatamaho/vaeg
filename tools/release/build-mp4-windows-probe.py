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

"""Build an OS-only synthetic probe; never modify production or existing outputs.

Two generated Windows adapters differ only in hardware-transform preference.
Both retain the production API sequence, with read-only finalization telemetry.
The NV12 conversion function is extracted verbatim, not reimplemented or mocked.
No FFmpeg, external encoder, SDL, ROM, existing MKV or private SDK is involved.
"""

import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[2]
ADAPTER = Path("sdl2/recording/recording_mp4_windows.cpp")
COMMON = Path("sdl2/recording/recording_mp4.cpp")
SINK = Path("sdl2/recording/recording_mp4_sink.hpp")
PROBE = Path("tests/frontend/recording/test_mp4_windows_probe.cpp")


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def unique_replace(text, old, new):
    if text.count(old) != 1:
        raise ValueError("MP4_PROBE_SOURCE_CONTRACT: expected one " + repr(old))
    return text.replace(old, new)


def generate(adapter, common):
    """Pure generation contract, separately testable without a compiler/history."""
    declaration = (
        "\nvoid mp4_probe_report(const char *, IMFSinkWriter *, DWORD, DWORD, HRESULT) noexcept;"
    )
    instrumented = unique_replace(
        adapter, "#include <mfreadwrite.h>", "#include <mfreadwrite.h>" + declaration
    )
    instrumented = unique_replace(
        instrumented,
        "check(writer_->Finalize());",
        'mp4_probe_report("before", writer_.p, video_stream_, audio_stream_, S_OK);\n'
        "\t\tconst HRESULT result = writer_->Finalize();\n"
        '\t\tmp4_probe_report("after", writer_.p, video_stream_, audio_stream_, result);\n'
        "\t\tcheck(result);",
    )
    software = unique_replace(
        instrumented,
        "MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, TRUE",
        "MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, FALSE",
    )
    start = "void mp4_bgra_to_nv12("
    end = "\n#if !defined(__APPLE__) && !defined(_WIN32)"
    if common.count(start) != 1 or common.count(end) != 1:
        raise ValueError("MP4_PROBE_PIXEL_CONTRACT")
    function = common[common.index(start) : common.index(end)]
    if not function.rstrip().endswith("}"):
        raise ValueError("MP4_PROBE_PIXEL_CONTRACT")
    license_header = common[: common.index("*/") + 2]
    pixels = (
        license_header
        + '\n#include "recording_mp4_sink.hpp"\n#include <algorithm>\n'
        + "namespace vaeg::recording {\n"
        + function
        + "\n}\n"
    )
    return instrumented, software, pixels


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output-dir", required=True, type=Path)
    parser.add_argument("--cxx", default="x86_64-w64-mingw32-g++")
    args = parser.parse_args()
    cxx = shutil.which(args.cxx)
    if not cxx:
        parser.error("MP4_PROBE_COMPILER_UNAVAILABLE")
    machine = subprocess.check_output([cxx, "-dumpmachine"], text=True).strip()
    if machine != "x86_64-w64-mingw32":
        parser.error("MP4_PROBE_COMPILER_MUST_TARGET_X64_MINGW")
    generated = generate((ROOT / ADAPTER).read_text(), (ROOT / COMMON).read_text())
    output = args.output_dir.resolve()
    if output == ROOT or ROOT not in output.parents:
        parser.error("MP4_PROBE_OUTPUT_MUST_BE_OUT_OF_SOURCE_UNDER_REPOSITORY")
    if not output.relative_to(ROOT).parts[0] == "build":
        parser.error("MP4_PROBE_OUTPUT_MUST_BE_UNDER_BUILD")
    output.mkdir(parents=True, exist_ok=False)
    manifest = {
        "schema": "MP4_WINDOWS_PROBE_BUILD_V1",
        "source_commit": subprocess.check_output(
            ["git", "rev-parse", "HEAD"], cwd=ROOT, text=True
        ).strip(),
        "inputs": {str(p): sha(ROOT / p) for p in [ADAPTER, COMMON, SINK, PROBE]},
        "builder_sha256": sha(Path(__file__)),
        "compiler": subprocess.check_output([cxx, "--version"], text=True).strip(),
        "compiler_target": machine,
        "commands": [],
        "generated": {},
        "native_execution": "NOT_RUN",
        "output": {},
    }
    try:
        flags = [
            "-std=c++17", "-O2", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
            "-I" + str(ROOT / SINK.parent),
        ]
        objects = []
        for name, content in zip(["hardware_on", "hardware_off", "pixels"], generated):
            source = output / (name + ".cpp")
            with source.open("w", encoding="utf-8", newline="\n") as stream:
                stream.write(content)
            manifest["generated"][source.name] = sha(source)
            obj = output / (name + ".o")
            extra = [] if name == "pixels" else [
                "-Dcreate_native_mp4_sink=mp4_probe_" + name
            ]
            command = [cxx, *flags, *extra, "-c", str(source), "-o", str(obj)]
            result = subprocess.run(command, capture_output=True, text=True)
            (output / (name + ".log")).write_text(result.stdout + result.stderr)
            manifest["commands"].append({"argv": command, "exit": result.returncode})
            if result.returncode:
                raise RuntimeError("MP4_PROBE_COMPILE_FAILED: " + name)
            objects.append(str(obj))
        exe = output / "mp4-windows-probe.exe"
        command = [
            cxx, *flags,
            '-DMP4_PROBE_SOURCE_SHA256="' + manifest["inputs"][str(ADAPTER)] + '"',
            str(ROOT / PROBE), *objects,
            "-static", "-static-libgcc", "-static-libstdc++",
            "-lole32", "-lmfuuid", "-luuid", "-o", str(exe),
        ]
        result = subprocess.run(command, capture_output=True, text=True)
        (output / "link.log").write_text(result.stdout + result.stderr)
        manifest["commands"].append({"argv": command, "exit": result.returncode})
        if result.returncode:
            raise RuntimeError("MP4_PROBE_LINK_FAILED")
        manifest["output"] = {"sha256": sha(exe), "bytes": exe.stat().st_size}
        print(exe)
        print(manifest["output"]["sha256"])
    finally:
        (output / "build-report.json").write_text(json.dumps(manifest, indent=2) + "\n")


if __name__ == "__main__":
    main()
