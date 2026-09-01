#!/usr/bin/env python3
# Copyright (c) 2026 Nakata Maho
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
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
# OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
# OF THE POSSIBILITY OF SUCH DAMAGE.
"""Verify the ROM-free production-memory CPU trace contract."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import pathlib
import re
import shlex
import subprocess
import sys
from dataclasses import dataclass


TRACE_DEFINE = "VAEG_Z80_COMPAT_INTEGRATION_TRACE"
MEMORY_TEST_DEFINE = "VAEG_UPD9002_SSTS_TESTING"
SUBSYSTEM_TEST_DEFINE = "VAEG_UPD780_INTEGRATION_TESTING"
CHECKPOINT_PREFIX = "selftest: production trace checkpoint "
CAPABILITY = (
    "vaeg-production-trace-v1 trace=enabled tests=disabled memory=production "
    "test-flat=absent bounded-stop=enabled file-output=enabled"
)


class TraceQaError(RuntimeError):
    """A production-trace invariant failed."""


@dataclass(frozen=True)
class ModeExpectation:
    trace: bool
    tests: bool


MODES = {
    "p0": ModeExpectation(trace=False, tests=False),
    "p1": ModeExpectation(trace=True, tests=False),
    "t0": ModeExpectation(trace=False, tests=True),
    "t1": ModeExpectation(trace=True, tests=True),
}


def sha256(path: pathlib.Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def command_tokens(record: dict[str, object]) -> list[str]:
    arguments = record.get("arguments")
    if isinstance(arguments, list) and all(isinstance(item, str) for item in arguments):
        return list(arguments)
    command = record.get("command")
    if isinstance(command, str):
        return shlex.split(command)
    raise TraceQaError("compile command has neither arguments nor command")


def compile_defines(record: dict[str, object]) -> set[str]:
    defines: set[str] = set()
    tokens = command_tokens(record)
    position = 0
    while position < len(tokens):
        token = tokens[position]
        if token == "-D" and position + 1 < len(tokens):
            position += 1
            defines.add(tokens[position].split("=", 1)[0])
        elif token.startswith("-D") and len(token) > 2:
            defines.add(token[2:].split("=", 1)[0])
        position += 1
    return defines


def load_compile_commands(path: pathlib.Path) -> list[dict[str, object]]:
    try:
        data = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise TraceQaError(f"cannot read compile commands: {path}") from exc
    if not isinstance(data, list) or not all(isinstance(item, dict) for item in data):
        raise TraceQaError(f"compile commands are not a JSON object array: {path}")
    return data


def source_suffix(record: dict[str, object]) -> str:
    source = record.get("file")
    if not isinstance(source, str):
        raise TraceQaError("compile command lacks a source file")
    return pathlib.PurePath(source).as_posix()


def unique_source(
    records: list[dict[str, object]], suffix: str
) -> dict[str, object]:
    matches = [record for record in records if source_suffix(record).endswith(suffix)]
    if len(matches) != 1:
        raise TraceQaError(f"expected one {suffix} compile command, found {len(matches)}")
    return matches[0]


def verify_mode(path: pathlib.Path, mode: str) -> None:
    expectation = MODES[mode]
    records = load_compile_commands(path)
    memory = unique_source(records, "/cpu/upd9002/memory.c")
    subsystem = unique_source(records, "/io/subsystem.cpp")
    for name, record, test_define in (
        ("memory", memory, MEMORY_TEST_DEFINE),
        ("subsystem", subsystem, SUBSYSTEM_TEST_DEFINE),
    ):
        defines = compile_defines(record)
        if (TRACE_DEFINE in defines) != expectation.trace:
            raise TraceQaError(f"{mode} {name} trace definition does not match the matrix")
        if (test_define in defines) != expectation.tests:
            raise TraceQaError(f"{mode} {name} test-memory definition does not match the matrix")
    if mode == "p1":
        test_sources = [
            source_suffix(record)
            for record in records
            if "/tests/" in source_suffix(record)
        ]
        if test_sources:
            raise TraceQaError("P1 compiled test sources: " + ", ".join(test_sources[:5]))


def exported_symbols(binary: pathlib.Path) -> set[str]:
    try:
        process = subprocess.run(
            ["nm", "-g", str(binary)],
            check=False,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )
    except OSError as exc:
        raise TraceQaError("nm is required for structural link verification") from exc
    if process.returncode != 0:
        raise TraceQaError("nm could not inspect the P1 executable")
    symbols: set[str] = set()
    for line in process.stdout.splitlines():
        fields = line.split()
        if not fields:
            continue
        symbols.add(fields[-1].lstrip("_"))
    return symbols


def verify_p1_link(binary: pathlib.Path) -> None:
    symbols = exported_symbols(binary)
    required = {
        "upd9002_memoryread",
        "upd9002_memoryread_va",
        "upd9002_trace_start_bounded",
    }
    missing = sorted(required - symbols)
    if missing:
        raise TraceQaError("P1 lacks production trace symbols: " + ", ".join(missing))
    if "upd9002_test_flat_memory_set" in symbols:
        raise TraceQaError("P1 links the flat test-memory seam")


def verify_no_extra_fetch_read(source_root: pathlib.Path) -> None:
    core_path = source_root / "cpu" / "upd9002" / "upd9002_core.c"
    trace_path = source_root / "cpu" / "upd9002" / "upd9002_trace.c"
    try:
        core = core_path.read_text(encoding="utf-8")
        trace = trace_path.read_text(encoding="utf-8")
    except OSError as exc:
        raise TraceQaError("cannot inspect the CPU fetch boundary") from exc
    fetch = core.find("opcode = upd9002_memoryread(")
    trace_call = core.find("upd9002_trace_step_begin", fetch)
    fetch_end = fetch + len("opcode = upd9002_memoryread(") if fetch >= 0 else -1
    if fetch < 0 or trace_call < 0 or "upd9002_memoryread(" in core[fetch_end:trace_call]:
        raise TraceQaError("causal trace is not attached to the existing fetch boundary")
    causal_call = trace.find("vaeg_causal_trace_cpu_step")
    if causal_call < 0 or "memoryread" in trace[causal_call : causal_call + 320]:
        raise TraceQaError("CPU causal tracing performs an extra memory read")


def run_process(arguments: list[str], environment: dict[str, str]) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        arguments,
        check=False,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        env=environment,
    )


def checkpoint(stderr: str) -> str:
    matches = [line for line in stderr.splitlines() if line.startswith(CHECKPOINT_PREFIX)]
    if len(matches) != 1:
        raise TraceQaError(f"expected one production checkpoint, found {len(matches)}")
    return matches[0]


def prepare_output(path: pathlib.Path) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    if path.exists():
        path.unlink()


def causal_records(path: pathlib.Path) -> list[dict[str, object]]:
    try:
        lines = path.read_text(encoding="utf-8").splitlines()
        records = [json.loads(line) for line in lines]
    except (OSError, UnicodeDecodeError, json.JSONDecodeError) as exc:
        raise TraceQaError(f"causal trace is not canonical JSONL: {path}") from exc
    if not records or records[0] != {
        "schema": "vaeg-causal-trace-v1",
        "encoding": "jsonl",
    }:
        raise TraceQaError("causal trace header is not canonical")
    if len(records) < 2 or not all(isinstance(record, dict) for record in records):
        raise TraceQaError("causal trace records are not objects")
    sequences = [record.get("seq") for record in records[1:]]
    if sequences != list(range(len(sequences))):
        raise TraceQaError("causal trace sequence is not canonical")
    if records[-1].get("class") != "stop":
        raise TraceQaError("causal trace has no final stop record")
    if not isinstance(records[-1].get("reason"), str):
        raise TraceQaError("causal trace stop has no reason")
    return records


def causal_selftest(
    binary: pathlib.Path,
    first_path: pathlib.Path,
    second_path: pathlib.Path,
    first_manifest: pathlib.Path,
    second_manifest: pathlib.Path,
    environment: dict[str, str],
) -> None:
    for output, manifest in ((first_path, first_manifest), (second_path, second_manifest)):
        prepare_output(output)
        prepare_output(manifest)
        process = run_process(
            [
                str(binary),
                "--selftest",
                "--causal-trace-output",
                str(output),
                "--causal-trace-limit",
                "200",
                "--causal-trace-ring",
                "0",
                "--causal-trace-manifest",
                str(manifest),
            ],
            environment,
        )
        if process.returncode != 0:
            raise TraceQaError(
                "causal production-trace selftest failed: "
                + process.stderr.strip().replace("\n", " ")
            )
        if not output.is_file() or not manifest.is_file():
            raise TraceQaError("causal selftest did not produce its outputs")
        records = causal_records(output)
        if not any(record.get("class") == "device_schedule" for record in records):
            raise TraceQaError("causal selftest lacks a production device event")
        if b"/Users/" in output.read_bytes() or b"/home/" in output.read_bytes():
            raise TraceQaError("causal trace contains a host path")
    if first_path.read_bytes() != second_path.read_bytes():
        raise TraceQaError("two causal selftests produced different trace output")
    if first_manifest.read_bytes() != second_manifest.read_bytes():
        raise TraceQaError("two causal selftests produced different manifests")

    invalid = run_process(
        [
            str(binary),
            "--selftest",
            "--causal-trace-output",
            str(first_path.with_name("causal-invalid.jsonl")),
            "--causal-trace-limit",
            "10",
            "--causal-trace-io",
            "malformed-range",
        ],
        environment,
    )
    if invalid.returncode == 0 or "invalid causal trace configuration" not in invalid.stderr:
        raise TraceQaError("malformed causal range did not fail closed")


def traced_selftest(
    binary: pathlib.Path, output: pathlib.Path, environment: dict[str, str]
) -> subprocess.CompletedProcess[str]:
    prepare_output(output)
    process = run_process(
        [
            str(binary),
            "--selftest",
            "--trace-cpu",
            "1",
            "--trace-cpu-output",
            str(output),
            "--trace-cpu-stop",
        ],
        environment,
    )
    if process.returncode != 0:
        raise TraceQaError("bounded production-trace selftest failed")
    if not output.is_file():
        raise TraceQaError("bounded production-trace selftest produced no trace file")
    return process


def verify_trace_text(text: str) -> None:
    lines = text.splitlines()
    if len(lines) != 7 or lines[0] != "upd9002-trace-v1":
        raise TraceQaError("trace has an unexpected record count or header")
    required_begin = (
        "begin step=00000000 clock=00001000 model=va memory=production "
        "cs=0000 ip=2000 physical=00002000 bytes=90 "
        "ax=1111 bx=2222 cx=3333 dx=4444 si=5555 di=6666 bp=7777 sp=8888 "
        "es=9999 ss=aaaa ds=bbbb flags=0202 if=1"
    )
    if lines[1] != required_begin:
        raise TraceQaError("first production trace event does not match the synthetic state")
    if "origin=cpu kind=fetch address=00002000 value=00000090" not in lines[2]:
        raise TraceQaError("trace lacks the already-fetched production opcode")
    if not lines[5].startswith("end step=00000000 ") or " ip=2001 " not in lines[5]:
        raise TraceQaError("trace lacks the expected post-instruction state")
    if lines[6] != "stop reason=trace-limit step=00000001 memory=production":
        raise TraceQaError("trace limit lacks an explicit production-path stop")
    forbidden = (
        "/Users/",
        "/home/",
        "\\Users\\",
        "path=",
    )
    if any(token in text for token in forbidden):
        raise TraceQaError("canonical trace contains a host path")
    if re.search(r"\b20[0-9]{2}-[0-9]{2}-[0-9]{2}[T ]", text):
        raise TraceQaError("canonical trace contains a wall-clock timestamp")


def verify_runtime(binary: pathlib.Path, work: pathlib.Path) -> None:
    if not binary.is_file():
        raise TraceQaError(f"P1 executable is absent: {binary}")
    work.mkdir(parents=True, exist_ok=True)
    environment = os.environ.copy()
    environment.update({"SDL_VIDEODRIVER": "dummy", "SDL_AUDIODRIVER": "dummy"})

    capability = run_process([str(binary), "--production-trace-capability"], environment)
    if capability.returncode != 0 or capability.stdout.strip() != CAPABILITY:
        raise TraceQaError("P1 capability record is absent or incorrect")

    untraced = run_process([str(binary), "--selftest"], environment)
    if untraced.returncode != 0:
        raise TraceQaError("trace-compiled, runtime-disabled selftest failed")

    first_path = work / "trace-1.txt"
    second_path = work / "trace-2.txt"
    first = traced_selftest(binary, first_path, environment)
    second = traced_selftest(binary, second_path, environment)
    first_bytes = first_path.read_bytes()
    second_bytes = second_path.read_bytes()
    if first_bytes != second_bytes:
        raise TraceQaError("two identical P1 runs produced different trace projections")
    if checkpoint(first.stderr) != checkpoint(second.stderr):
        raise TraceQaError("two traced runs reached different architectural checkpoints")
    if checkpoint(untraced.stderr) != checkpoint(first.stderr):
        raise TraceQaError("trace enabled and disabled changed the architectural checkpoint")
    verify_trace_text(first_bytes.decode("utf-8"))

    causal_selftest(
        binary,
        work / "causal-trace-1.jsonl",
        work / "causal-trace-2.jsonl",
        work / "causal-manifest-1.json",
        work / "causal-manifest-2.json",
        environment,
    )

    missing_parent = work / "missing-parent" / "trace.txt"
    failed_output = run_process(
        [
            str(binary),
            "--selftest",
            "--trace-cpu",
            "1",
            "--trace-cpu-output",
            str(missing_parent),
        ],
        environment,
    )
    if failed_output.returncode == 0 or "cannot open --trace-cpu-output" not in failed_output.stderr:
        raise TraceQaError("missing trace-output parent did not fail closed")

    missing_count = run_process(
        [str(binary), "--trace-cpu-output", str(work / "unused.txt")], environment
    )
    if missing_count.returncode == 0 or "require --trace-cpu" not in missing_count.stderr:
        raise TraceQaError("trace output without a bound was accepted")

    print(
        "production trace runtime passed: "
        f"binary_sha256={sha256(binary)} trace_sha256={sha256(first_path)}"
    )


def compare_binaries(first: pathlib.Path, second: pathlib.Path) -> None:
    if not first.is_file() or not second.is_file():
        raise TraceQaError("P1 comparison executable is absent")
    if first.read_bytes() != second.read_bytes():
        raise TraceQaError(
            "two clean P1 builds differ: "
            f"first={sha256(first)} second={sha256(second)}"
        )
    print(f"two clean P1 executables are byte-identical: sha256={sha256(first)}")


def matrix(arguments: argparse.Namespace) -> None:
    paths = {
        "p0": pathlib.Path(arguments.p0_compile_commands),
        "p1": pathlib.Path(arguments.p1_compile_commands),
        "t0": pathlib.Path(arguments.t0_compile_commands),
        "t1": pathlib.Path(arguments.t1_compile_commands),
    }
    for mode, path in paths.items():
        verify_mode(path, mode)
    verify_no_extra_fetch_read(pathlib.Path(__file__).resolve().parents[2])
    verify_p1_link(pathlib.Path(arguments.p1_binary))
    print("P0/P1/T0/T1 compile definitions and P1 production-memory linkage passed")


def selftest() -> None:
    fake = {
        "file": "/source/cpu/upd9002/memory.c",
        "arguments": ["cc", "-DVAEG_Z80_COMPAT_INTEGRATION_TRACE=1", "-c"],
    }
    assert compile_defines(fake) == {TRACE_DEFINE}
    try:
        unique_source([fake, fake], "/cpu/upd9002/memory.c")
    except TraceQaError:
        pass
    else:
        raise AssertionError("duplicate compile command was accepted")
    assert hashlib.sha256(b"production-trace-selftest").hexdigest() == (
        "0f3f20b3970e089886ab36003658b6ccb6d4a6db06bc0e300a195d137b90a85e"
    )
    print("production trace QA selftest passed")


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    subparsers = parser.add_subparsers(dest="command", required=True)

    runtime = subparsers.add_parser("verify-runtime")
    runtime.add_argument("--binary", required=True)
    runtime.add_argument("--work", required=True)

    binary_compare = subparsers.add_parser("compare-binaries")
    binary_compare.add_argument("--first", required=True)
    binary_compare.add_argument("--second", required=True)

    build_matrix = subparsers.add_parser("verify-matrix")
    for mode in MODES:
        build_matrix.add_argument(f"--{mode}-compile-commands", required=True)
    build_matrix.add_argument("--p1-binary", required=True)

    subparsers.add_parser("selftest")
    return parser.parse_args()


def main() -> int:
    arguments = parse_arguments()
    try:
        if arguments.command == "verify-runtime":
            verify_runtime(pathlib.Path(arguments.binary), pathlib.Path(arguments.work))
        elif arguments.command == "compare-binaries":
            compare_binaries(pathlib.Path(arguments.first), pathlib.Path(arguments.second))
        elif arguments.command == "verify-matrix":
            matrix(arguments)
        else:
            selftest()
    except TraceQaError as exc:
        print(f"production trace QA failed: {exc}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
