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
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
# OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
# USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
# ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
# THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

"""Run the M101/M102 CP/M probe programs through one Z80-compatible role.

The programs are built from tracked sources with z80asm 1.8 and run by
`vaeg_cpm_runner --role <role>`, which applies that role's production core
configuration. Each manifest entry names a program, one output file, and how
to judge it:

- `exact`: the output equals a recorded file byte for byte;
- `prefix`: a recorded file that stopped early is a prefix of the output;
- `records`: every record (line) of a recorded file appears in the output,
  for a real run restarted part-way through the program;
- `zex_all_ok`: a ZEX-style output completed with every group OK.

For the uPD70008-compatible mode the recorded files are real PC-88VA2
outputs. The FDC uPD780C has no real-machine record; its manifest requires
the stock Zilog CRCs.
"""

from __future__ import annotations

import argparse
import concurrent.futures
import json
import os
import pathlib
import subprocess
import sys
import tempfile
from typing import Any, Iterable

ROOT = pathlib.Path(__file__).resolve().parents[2]
ZEX_TOOLS = ROOT / "tools" / "cpmva" / "zex"
sys.path.insert(0, str(ZEX_TOOLS))
import probe_results  # noqa: E402

MANIFEST_SCHEMA = "vaeg-z80-role-probes-v1"
ROLES = ("upd70008", "upd780")
MODES = ("exact", "prefix", "records", "zex_all_ok")
SKIP_RETURN_CODE = 77
MAX_CLOCKS = 200000000000

ERROR_MANIFEST = "Z80_ROLE_MANIFEST"
ERROR_RUN = "Z80_ROLE_RUN_FAILED"
ERROR_MISSING = "Z80_ROLE_OUTPUT_MISSING"
ERROR_EXACT = "Z80_ROLE_EXACT_MISMATCH"
ERROR_PREFIX = "Z80_ROLE_PREFIX_MISMATCH"
ERROR_RECORDS = "Z80_ROLE_RECORD_MISSING"
ERROR_ZEX = "Z80_ROLE_ZEX_GROUP"


class ProbeError(RuntimeError):
    """A build or file error that prevents the check."""


# ---- Content layer: pure judgement of outputs against a manifest.


def validate_manifest(manifest: Any) -> list[tuple[str, str]]:
    if not isinstance(manifest, dict) or manifest.get("schema") != MANIFEST_SCHEMA:
        return [(ERROR_MANIFEST, f"expected schema {MANIFEST_SCHEMA}")]
    findings = []
    if manifest.get("role") not in ROLES:
        findings.append((ERROR_MANIFEST, "unknown role"))
    programs = manifest.get("programs")
    checks = manifest.get("checks")
    if not isinstance(programs, dict) or not programs:
        return findings + [(ERROR_MANIFEST, "programs must be a non-empty object")]
    if not isinstance(checks, list) or not checks:
        return findings + [(ERROR_MANIFEST, "checks must be a non-empty array")]
    for name, program in programs.items():
        if not isinstance(program, dict) or set(program) - {"tail", "may_fail"}:
            findings.append((ERROR_MANIFEST, f"program {name}: unknown keys"))
    for index, check in enumerate(checks):
        where = f"check {index}"
        if not isinstance(check, dict) or check.get("mode") not in MODES:
            findings.append((ERROR_MANIFEST, f"{where}: unknown mode"))
            continue
        if check.get("program") not in programs or not check.get("output"):
            findings.append((ERROR_MANIFEST, f"{where}: unknown program or no output"))
        mode = check["mode"]
        if mode in ("exact", "prefix", "records") and set(check) != {"program", "output", "mode", "reference"}:
            findings.append((ERROR_MANIFEST, f"{where}: needs program, output, mode, reference"))
        if mode == "zex_all_ok" and (
            set(check) != {"program", "output", "mode", "groups"}
            or not isinstance(check.get("groups"), int)
            or check["groups"] < 1
        ):
            findings.append((ERROR_MANIFEST, f"{where}: needs a positive group count"))
    return findings


def strip_padding(data: bytes) -> bytes:
    """Drop the CP/M end-of-file and record padding of a short run."""

    return data.rstrip(b"\x1a\x00")


def records(data: bytes) -> list[bytes]:
    """Split a CP/M text output into its non-empty lines."""

    return [line for line in strip_padding(data).split(b"\r\n") if line.strip()]


def judge(
    manifest: dict[str, Any],
    outputs: dict[tuple[str, str], bytes],
    failed_runs: dict[str, str],
    references: dict[str, bytes],
) -> list[tuple[str, str]]:
    """Return (code, detail) findings; empty means every check passes.

    outputs maps (program, output name) to the produced bytes, failed_runs
    maps a program to its failure text, references maps a reference path to
    the recorded bytes.
    """

    findings = validate_manifest(manifest)
    if findings:
        return findings
    for program, reason in sorted(failed_runs.items()):
        if not manifest["programs"][program].get("may_fail", False):
            findings.append((ERROR_RUN, f"{program}: {reason}"))
    for check in manifest["checks"]:
        key = (check["program"], check["output"])
        label = f"{check['program']}:{check['output']}"
        if key not in outputs:
            findings.append((ERROR_MISSING, label))
            continue
        data = outputs[key]
        mode = check["mode"]
        if mode == "exact":
            if data != references[check["reference"]]:
                findings.append((ERROR_EXACT, f"{label} != {check['reference']}"))
        elif mode == "records":
            produced = set(records(data))
            recorded = records(references[check["reference"]])
            missing = [line for line in recorded if line not in produced]
            if not recorded or missing:
                findings.append(
                    (ERROR_RECORDS, f"{check['reference']}: {len(missing)} of {len(recorded)} "
                     f"records missing from {label}")
                )
        elif mode == "prefix":
            recorded = strip_padding(references[check["reference"]])
            if not recorded or not data.startswith(recorded):
                findings.append((ERROR_PREFIX, f"{check['reference']} is not a prefix of {label}"))
        else:
            try:
                groups = probe_results.parse_zex(data.decode("ascii", "replace"))
            except probe_results.ResultError as error:
                findings.append((ERROR_ZEX, f"{label}: {error}"))
                continue
            bad = [group.name for group in groups if not group.ok]
            if len(groups) != check["groups"] or bad:
                findings.append(
                    (ERROR_ZEX, f"{label}: groups={len(groups)}/{check['groups']} not OK={bad}")
                )
    return findings


# ---- Execution layer: build and run the programs.


def build_programs(assembler: str) -> dict[str, bytes]:
    sys.path.insert(0, str(ZEX_TOOLS))
    import zexbuild  # noqa: E402

    try:
        return zexbuild.build_programs(assembler)
    except zexbuild.BuildError as error:
        raise ProbeError(f"program build failed: {error}") from error


def run_program(
    runner: pathlib.Path, role: str, program: pathlib.Path, tail: str | None, directory: pathlib.Path
) -> tuple[str, str | None]:
    directory.mkdir(parents=True, exist_ok=True)
    command = [
        str(runner), "--role", role, "--dir", str(directory),
        "--console", str(directory / "CONSOLE.LOG"), "--max-clocks", str(MAX_CLOCKS),
    ]
    if tail:
        command += ["--tail", tail]
    completed = subprocess.run(
        command + [str(program)],
        stdout=subprocess.DEVNULL, stderr=subprocess.PIPE, text=True, check=False,
    )
    if completed.returncode != 0:
        return program.stem, completed.stderr.strip() or f"exit {completed.returncode}"
    return program.stem, None


def run(
    manifest_path: pathlib.Path,
    runner: pathlib.Path,
    assembler: str,
    jobs: int,
    work: pathlib.Path,
) -> list[tuple[str, str]]:
    manifest = read_json(manifest_path)
    findings = validate_manifest(manifest)
    if findings:
        return findings
    images = build_programs(assembler)
    program_dir = work / "programs"
    program_dir.mkdir(parents=True, exist_ok=True)
    for name in manifest["programs"]:
        image = images.get(f"{name}.COM")
        if image is None:
            raise ProbeError(f"no built program {name}.COM")
        (program_dir / f"{name}.COM").write_bytes(image)
    failed_runs = {}
    with concurrent.futures.ThreadPoolExecutor(max_workers=jobs) as pool:
        futures = [
            pool.submit(
                run_program, runner, manifest["role"], program_dir / f"{name}.COM",
                program.get("tail"), work / "run" / name,
            )
            for name, program in manifest["programs"].items()
        ]
        for future in concurrent.futures.as_completed(futures):
            name, reason = future.result()
            print(f"z80-role-probes: {manifest['role']} {name} "
                  + ("ok" if reason is None else f"failed: {reason}"), file=sys.stderr)
            if reason is not None:
                failed_runs[name] = reason
    outputs = {}
    references = {}
    for check in manifest["checks"]:
        path = work / "run" / check["program"] / check["output"]
        if path.is_file():
            outputs[(check["program"], check["output"])] = path.read_bytes()
        if "reference" in check:
            references[check["reference"]] = (ROOT / check["reference"]).read_bytes()
    return judge(manifest, outputs, failed_runs, references)


# ---- Self-test of the content layer.


ZEX_OK = (
    "Z80 instruction exerciser\r\n"
    "aluop a,nn....................  OK\r\n"
    "<daa,cpl,scf,ccf>.............  OK\r\n"
    "Tests complete\r\n"
).encode("ascii")


def synthetic() -> tuple[dict[str, Any], dict[tuple[str, str], bytes], dict[str, str], dict[str, bytes]]:
    manifest = {
        "schema": MANIFEST_SCHEMA,
        "role": "upd70008",
        "programs": {"A": {}, "B": {"may_fail": True}, "Z": {}, "R": {}},
        "checks": [
            {"program": "A", "output": "A.BIN", "mode": "exact", "reference": "ref/a.bin"},
            {"program": "B", "output": "B.TXT", "mode": "prefix", "reference": "ref/b.txt"},
            {"program": "Z", "output": "Z.TXT", "mode": "zex_all_ok", "groups": 2},
            {"program": "R", "output": "R.TXT", "mode": "records", "reference": "ref/r.txt"},
        ],
    }
    outputs = {("A", "A.BIN"): b"\x01\x02", ("B", "B.TXT"): b"line1\r\nline2\r\n",
               ("Z", "Z.TXT"): ZEX_OK, ("R", "R.TXT"): b"r1\r\nr2\r\nr3\r\n"}
    references = {"ref/a.bin": b"\x01\x02", "ref/b.txt": b"line1\r\n\x1a\x1a",
                  "ref/r.txt": b"r3\r\nr1\r\n\x1a"}
    return manifest, outputs, {"B": "stopped early"}, references


def selftest() -> None:
    import copy

    if judge(*synthetic()):
        raise AssertionError("synthetic fixture must pass")
    cases = [
        ("exact-byte", ERROR_EXACT, lambda m, o, f, r: o.__setitem__(("A", "A.BIN"), b"\x01\x03")),
        ("prefix-byte", ERROR_PREFIX, lambda m, o, f, r: o.__setitem__(("B", "B.TXT"), b"line9\r\n")),
        ("record-missing", ERROR_RECORDS,
         lambda m, o, f, r: o.__setitem__(("R", "R.TXT"), b"r1\r\nr2\r\n")),
        ("missing-output", ERROR_MISSING, lambda m, o, f, r: o.pop(("A", "A.BIN"))),
        ("run-failed", ERROR_RUN, lambda m, o, f, r: f.__setitem__("A", "crash")),
        ("zex-error", ERROR_ZEX, lambda m, o, f, r: o.__setitem__(
            ("Z", "Z.TXT"), ZEX_OK.replace(b"nn....................  OK",
                                           b"nn.... ERROR **** crc expected:12345678 found:87654321"))),
        ("zex-count", ERROR_ZEX, lambda m, o, f, r: m["checks"][2].__setitem__("groups", 3)),
        ("zex-incomplete", ERROR_ZEX, lambda m, o, f, r: o.__setitem__(
            ("Z", "Z.TXT"), ZEX_OK.replace(b"Tests complete", b""))),
        ("manifest-mode", ERROR_MANIFEST, lambda m, o, f, r: m["checks"][0].__setitem__("mode", "x")),
        ("manifest-role", ERROR_MANIFEST, lambda m, o, f, r: m.__setitem__("role", "z80")),
    ]
    for name, expected, mutate in cases:
        fixture = copy.deepcopy(synthetic())
        mutate(*fixture)
        codes = {code for code, _ in judge(*fixture)}
        if codes != {expected}:
            raise AssertionError(f"{name}: expected {{{expected}}}, got {sorted(codes)}")
    print(f"z80-role-probes-selftest: {len(cases) + 1} checks passed")


# ---- Command line.


def read_json(path: pathlib.Path) -> Any:
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except (OSError, UnicodeError, json.JSONDecodeError) as error:
        raise ProbeError(f"cannot read {path}: {error}") from error


def parse_args(argv: Iterable[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    subparsers = parser.add_subparsers(dest="command", required=True)
    run_parser = subparsers.add_parser("run", help="build, run, and judge one role")
    run_parser.add_argument("--manifest", type=pathlib.Path, required=True)
    run_parser.add_argument("--runner", type=pathlib.Path, required=True)
    run_parser.add_argument("--assembler", default="z80asm")
    run_parser.add_argument("--jobs", type=int, default=max(1, min(8, os.cpu_count() or 1)))
    run_parser.add_argument("--work", type=pathlib.Path)
    validate_parser = subparsers.add_parser("validate", help="check a manifest")
    validate_parser.add_argument("--manifest", type=pathlib.Path, required=True)
    subparsers.add_parser("selftest", help="run the judgement self-test")
    skip_parser = subparsers.add_parser("skip", help="report a skipped check (exit 77)")
    skip_parser.add_argument("--reason", required=True)
    return parser.parse_args(list(argv))


def report(findings: list[tuple[str, str]]) -> int:
    for code, detail in findings:
        print(f"{code} {detail}", file=sys.stderr)
    if findings:
        print(f"z80-role-probes: FAIL codes={','.join(sorted({c for c, _ in findings}))}",
              file=sys.stderr)
        return 1
    return 0


def main(argv: Iterable[str] | None = None) -> int:
    arguments = parse_args(sys.argv[1:] if argv is None else argv)
    try:
        if arguments.command == "selftest":
            selftest()
            return 0
        if arguments.command == "skip":
            print(f"z80-role-probes: SKIP {arguments.reason}")
            return SKIP_RETURN_CODE
        if arguments.command == "validate":
            status = report(validate_manifest(read_json(arguments.manifest)))
            if status == 0:
                print(f"z80-role-probes: manifest valid {arguments.manifest}")
            return status
        if arguments.work is not None:
            findings = run(arguments.manifest, arguments.runner.resolve(), arguments.assembler,
                           max(1, arguments.jobs), arguments.work.resolve())
        else:
            with tempfile.TemporaryDirectory(prefix="vaeg-z80-role-") as work:
                findings = run(arguments.manifest, arguments.runner.resolve(),
                               arguments.assembler, max(1, arguments.jobs), pathlib.Path(work))
        status = report(findings)
        if status == 0:
            print(f"z80-role-probes: PASS {arguments.manifest.name}")
        return status
    except ProbeError as error:
        print(f"z80-role-probes-error: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
