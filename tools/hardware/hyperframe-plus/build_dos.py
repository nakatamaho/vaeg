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
"""Build real-mode DOS code with the pinned SDK's DOS compiler in isolated DOSBox.

No reference viewer or board-changing command is executed. No containers or
system package installation. Output must be new and outside the checkout.
"""
import argparse
import json
import os
import re
import subprocess
import zipfile
from pathlib import Path, PurePosixPath
from verify_reference import mz_metadata, require, sha256

ROOT = Path(__file__).resolve().parent
REPO = ROOT.parents[2]
SOURCES = ("src/hf_io.h", "src/hf_io.c", "src/hf_dos.c", "src/hf_test.c", "build.bat")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--archive", type=Path, required=True)
    parser.add_argument("--dosbox", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--build-id", required=True, help="Eight lowercase hexadecimal characters")
    args = parser.parse_args()
    require(re.fullmatch(r"[0-9a-f]{8}", args.build_id) is not None, "HFBUILD_ID")
    output = args.output.resolve()
    require(output != REPO and REPO not in output.parents, "HFBUILD_OUTPUT_IN_CHECKOUT")
    require(not output.exists(), "HFBUILD_OUTPUT_EXISTS")
    require(args.dosbox.is_file(), "HFBUILD_DOSBOX_MISSING")
    policy = (REPO / "tools/openwatcom/containerfile").read_text(encoding="utf-8")
    expected = re.search(r"^ARG OW_SHA256=([0-9a-f]{64})$", policy, re.M).group(1)
    require(sha256(args.archive.read_bytes()) == expected, "HFBUILD_SDK_HASH")
    output.mkdir(parents=True)
    sdk, stage, run = (output / name for name in ("sdk", "stage", "run"))
    for directory in (sdk, stage, run):
        directory.mkdir()
    with zipfile.ZipFile(args.archive) as archive:
        for member in archive.infolist():
            if not member.filename.startswith(("binw/", "h/", "lib286/")):
                continue
            path = PurePosixPath(member.filename)
            require(not path.is_absolute() and ".." not in path.parts and
                    "\\" not in member.filename and
                    (member.external_attr >> 16) & 0o170000 != 0o120000,
                    "HFBUILD_SDK_MEMBER")
            archive.extract(member, sdk)
    for tool in ("wcc.exe", "wlink.exe", "wdis.exe", "dos4gw.exe"):
        require((sdk / "binw" / tool).is_file(), "HFBUILD_SDK_TOOL")
    source_hashes = {}
    for name in SOURCES:
        data = (ROOT / name).read_bytes()
        require(all(byte < 128 for byte in data), "HFBUILD_ASCII_SOURCE")
        source_hashes[name] = sha256(data)
        target = stage / name
        target.parent.mkdir(exist_ok=True)
        target.write_bytes(data.replace(b"\n", b"\r\n") if name.endswith(".bat") else data)
    (run / "cancel.txt").write_bytes(b"\r\n")
    batch = ["@echo off", f"call S:\\build.bat S:\\src {args.build_id} > build.log",
             "if not exist build.ok goto failed"]
    smoke = [("", "default.txt", 0, "HF_OK"),
             ("info", "info.txt", 0, "HF_OK"),
             ("init", "init.txt", 2, "HF_UNSUPPORTED"),
             ("banktest", "bank.txt", 2, "HF_UNSUPPORTED"),
             ("off < cancel.txt", "cancel.out", 2, "HF_ABORTED"),
             ("unknown", "usage.txt", 3, "HF_ARGUMENT")]
    for command, name, code, _ in smoke:
        batch += [f"hftest {command} > {name}",
                  f"if errorlevel {code + 1} goto failed"]
        if code:
            batch.append(f"if not errorlevel {code} goto failed")
    batch += ["echo HFSMOKE_OK>smoke.ok", "goto end", ":failed",
              "echo HFSMOKE_FAILED>smoke.err", ":end"]
    (run / "run.bat").write_bytes(("\r\n".join(batch) + "\r\n").encode("ascii"))
    config = """[sdl]
output=surface
[dosbox]
memsize=16
[cpu]
core=normal
cycles=max
[mixer]
nosound=true
[midi]
mpu401=none
mididevice=none
[sblaster]
sbtype=none
[speaker]
pcspeaker=false
[autoexec]
"""
    config += "\n".join([f'mount c "{run}"', f'mount d "{sdk}"', f'mount s "{stage}"',
                          "c:", "set PATH=D:\\BINW", "set WATCOM=D:\\",
                          "set INCLUDE=D:\\H", "set LIB=D:\\LIB286;D:\\LIB286\\DOS",
                          "set TEMP=C:\\", "set TMP=C:\\", "call run.bat", "exit", ""])
    config_path = output / "dosbox.conf"
    config_path.write_text(config, encoding="ascii")
    env = os.environ.copy()
    env.update(SDL_VIDEODRIVER="dummy", SDL_AUDIODRIVER="dummy")
    command = [str(args.dosbox.resolve()), "-conf", str(config_path)]
    with (output / "dosbox.log").open("wb") as log:
        process = subprocess.run(command, stdout=log, stderr=subprocess.STDOUT,
                                 env=env, cwd=run, timeout=180)
    require(process.returncode == 0, "HFBUILD_WORKER_EXIT")
    require((run / "build.ok").is_file() and not (run / "build.err").exists(),
            "HFBUILD_COMPILER_FAILED")
    exe = (run / "hftest.exe").read_bytes()
    mz = mz_metadata(exe)
    require(b"HFTEST build %s" in exe and args.build_id.encode() in exe, "HFBUILD_ID_EMBED")
    listing = (run / "hf_dos.lst").read_text(encoding="ascii")
    forms = {}
    for name, pattern, opcode in (
        ("in8", r"\bin\s+al\s*,\s*dx\b", "EC"),
        ("out8", r"\bout\s+dx\s*,\s*al\b", "EE"),
        ("out16", r"\bout\s+dx\s*,\s*ax\b", "EF")):
        lines = [line.strip() for line in listing.splitlines()
                 if re.search(pattern, line, re.I)]
        require(len(lines) == 1 and opcode in lines[0].split(), "HFBUILD_IO_INSTRUCTION_FORM")
        forms[name] = lines[0]
    require((run / "smoke.ok").is_file() and not (run / "smoke.err").exists(),
            "HFBUILD_SMOKE_EXIT")
    smoke_results = []
    for cmd, name, code, expected_result in smoke:
        data = (run / name).read_bytes()
        text = data.decode("ascii")
        require(re.findall(r"^HF_RESULT=(\w+)\r?$", text, re.M) == [expected_result],
                "HFBUILD_SMOKE_RESULT")
        require(f"HFTEST build {args.build_id};" in text, "HFBUILD_SMOKE_ID")
        smoke_results.append({"command": "hftest " + cmd, "exit": code,
                              "result": expected_result, "sha256": sha256(data)})
    report = {"status": "BUILT", "build_id": args.build_id,
              "compiler": "Open Watcom v2, pinned 2026-08-01, DOS-hosted wcc",
              "compiler_archive_sha256": expected,
              "source_files_sha256": source_hashes,
              "compiler_options": "-bt=dos -1 -ms -za -w4 -we -s; hf_dos.c additionally -oi",
              "source_head": subprocess.check_output(["git", "rev-parse", "HEAD"],
                                                       cwd=REPO, text=True).strip(),
              "source_status": subprocess.check_output(["git", "status", "--short", "--",
                                                          str(ROOT)], cwd=REPO, text=True),
              "dosbox_command": command, "worker_exit": process.returncode,
              "exe_bytes": len(exe), "exe_sha256": sha256(exe), "mz": mz,
              "io_instruction_forms": forms, "dos_smoke": smoke_results,
              "target": "DOSBox software DOS environment; no board I/O commands authorized",
              "pc9801_runtime": "NOT_RUN", "hardware": "NOT_RUN"}
    (output / "build.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"status": "BUILT", "exe": str(run / "hftest.exe"),
                      "sha256": sha256(exe), "smoke_cases": len(smoke_results),
                      "hardware": "NOT_RUN"}))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
