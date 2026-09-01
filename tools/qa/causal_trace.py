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
"""Validate and compare generic VAEG causal trace projections."""

from __future__ import annotations

import argparse
import json
import pathlib
import sys
from collections import Counter
from typing import Any


class CausalTraceError(RuntimeError):
    """A causal trace contract failed."""


EVENT_CLASSES = {
    "cpu_step",
    "io_read",
    "io_write",
    "mem_read",
    "mem_write",
    "irq_assert",
    "irq_clear",
    "irq_accept",
    "device_schedule",
    "mailbox",
    "drive_state",
    "fdc_command",
    "fdc_position",
    "sector_transfer",
    "dma",
    "instruction_fetch_correlation",
    "stop",
}
PRIVATE_TOKENS = ("/Users/", "/home/", "\\Users\\", "private", "rom", "d88")


def load_trace(path: pathlib.Path) -> list[dict[str, Any]]:
    try:
        lines = path.read_text(encoding="utf-8").splitlines()
    except OSError as exc:
        raise CausalTraceError(f"cannot read causal trace: {path}") from exc
    if not lines:
        raise CausalTraceError("causal trace is empty")
    records: list[dict[str, Any]] = []
    for number, line in enumerate(lines, 1):
        try:
            record = json.loads(line)
        except json.JSONDecodeError as exc:
            raise CausalTraceError(f"invalid JSON at causal trace line {number}") from exc
        if not isinstance(record, dict):
            raise CausalTraceError(f"causal trace line {number} is not an object")
        records.append(record)
    if records[0] != {"schema": "vaeg-causal-trace-v1", "encoding": "jsonl"}:
        raise CausalTraceError("causal trace header is not canonical")
    previous = -1
    stop_count = 0
    for record in records[1:]:
        sequence = record.get("seq")
        if not isinstance(sequence, int) or sequence <= previous:
            raise CausalTraceError("causal trace sequence is not strictly increasing")
        previous = sequence
        event_class = record.get("class")
        if event_class not in EVENT_CLASSES:
            raise CausalTraceError(f"unknown causal event class: {event_class!r}")
        if event_class == "stop":
            stop_count += 1
    if stop_count != 1 or records[-1].get("class") != "stop":
        raise CausalTraceError("causal trace must end with one stop record")
    return records


def event_counts(records: list[dict[str, Any]]) -> dict[str, int]:
    return dict(sorted(Counter(record["class"] for record in records[1:]).items()))


def common_prefix(first: list[dict[str, Any]], second: list[dict[str, Any]]) -> int:
    count = 0
    for left, right in zip(first[1:], second[1:]):
        if left != right:
            break
        count += 1
    return count


def boundary_report(first: list[dict[str, Any]], second: list[dict[str, Any]]) -> dict[str, Any]:
    prefix = common_prefix(first, second)
    first_event = first[prefix + 1] if prefix + 1 < len(first) else None
    second_event = second[prefix + 1] if prefix + 1 < len(second) else None
    last_common = first[prefix] if prefix < len(first) else first[-1]
    return {
        "common_event_count": prefix,
        "last_common_class": last_common.get("class"),
        "first_divergent_first": first_event,
        "first_divergent_second": second_event,
    }


def wait_loops(records: list[dict[str, Any]], minimum_repetitions: int = 3) -> list[dict[str, Any]]:
    signatures: Counter[tuple[Any, ...]] = Counter()
    for record in records[1:]:
        if record.get("class") != "cpu_step":
            continue
        signature = (
            record.get("cs"),
            record.get("ip"),
            record.get("flags"),
            record.get("ax"),
            record.get("bx"),
            record.get("cx"),
            record.get("dx"),
        )
        signatures[signature] += 1
    return [
        {"repetitions": repetitions, "state_fields": list(signature)}
        for signature, repetitions in sorted(signatures.items(), key=lambda item: repr(item[0]))
        if repetitions >= minimum_repetitions
    ]


def causal_correlations(records: list[dict[str, Any]]) -> list[dict[str, Any]]:
    transfers = [record for record in records if record.get("class") == "sector_transfer"]
    fetches = [record for record in records if record.get("class") == "instruction_fetch_correlation"]
    correlations = []
    for fetch in fetches:
        correlations.append(
            {
                "producer": "sector_transfer",
                "consumer": "instruction_fetch_correlation",
                "producer_present": bool(transfers),
                "consumer_sequence": fetch["seq"],
            }
        )
    return correlations


def redact(record: Any, fields: set[str]) -> Any:
    if isinstance(record, dict):
        return {key: (None if key in fields else redact(value, fields))
                for key, value in sorted(record.items())}
    if isinstance(record, list):
        return [redact(item, fields) for item in record]
    return record


def public_projection(records: list[dict[str, Any]]) -> list[dict[str, Any]]:
    projection = redact(records, {"address", "value", "physical", "opcode", "cs", "ip", "ax", "bx", "cx", "dx", "si", "di", "bp", "sp", "es", "ss", "ds", "flags", "if", "reason", "step"})
    encoded = json.dumps(projection, sort_keys=True, separators=(",", ":"), ensure_ascii=True)
    if any(token.lower() in encoded.lower() for token in PRIVATE_TOKENS):
        raise CausalTraceError("public causal projection contains a private token")
    return projection


def summarize(first_path: pathlib.Path, second_path: pathlib.Path | None) -> dict[str, Any]:
    first = load_trace(first_path)
    result: dict[str, Any] = {
        "event_counts": event_counts(first),
        "wait_loops": wait_loops(first),
        "correlations": causal_correlations(first),
        "public_projection": public_projection(first),
    }
    if second_path is not None:
        second = load_trace(second_path)
        result["comparison"] = boundary_report(first, second)
        result["byte_identical"] = first_path.read_bytes() == second_path.read_bytes()
    return result


def selftest() -> None:
    synthetic = [
        {"schema": "vaeg-causal-trace-v1", "encoding": "jsonl"},
        {"seq": 0, "class": "io_write", "actor": "main-cpu", "device": "io", "phase": "write", "address": 1, "value": 2, "width": 1},
        {"seq": 1, "class": "mailbox", "actor": "main-cpu", "device": "fd-subsystem", "phase": "write", "address": 2, "value": 3, "width": 1},
        {"seq": 2, "class": "stop", "step": 0, "reason": "synthetic", "events": 2},
    ]
    other = json.loads(json.dumps(synthetic))
    other[1]["value"] = 4
    if common_prefix(synthetic, other) != 0:
        raise AssertionError("causal prefix selftest failed")
    if public_projection(synthetic)[1].get("value") is not None:
        raise AssertionError("causal redaction selftest failed")
    if event_counts(synthetic) != {"io_write": 1, "mailbox": 1, "stop": 1}:
        raise AssertionError("causal count selftest failed")
    repeated = synthetic[:1] + [
        {
            "seq": 0,
            "class": "cpu_step",
            "cs": 1,
            "ip": 2,
            "flags": 0,
            "ax": 3,
            "bx": 4,
            "cx": 5,
            "dx": 6,
        },
        {
            "seq": 1,
            "class": "cpu_step",
            "cs": 1,
            "ip": 2,
            "flags": 0,
            "ax": 3,
            "bx": 4,
            "cx": 5,
            "dx": 6,
        },
        {
            "seq": 2,
            "class": "cpu_step",
            "cs": 1,
            "ip": 2,
            "flags": 0,
            "ax": 3,
            "bx": 4,
            "cx": 5,
            "dx": 6,
        },
        {"seq": 3, "class": "stop", "reason": "bound", "events": 3},
    ]
    if len(wait_loops(repeated)) != 1 or wait_loops(repeated)[0]["repetitions"] != 3:
        raise AssertionError("wait-loop selftest failed")
    correlated = repeated[:1] + [
        {"seq": 0, "class": "sector_transfer"},
        {"seq": 1, "class": "instruction_fetch_correlation"},
        {"seq": 2, "class": "stop", "reason": "bound", "events": 2},
    ]
    if causal_correlations(correlated) != [
        {
            "producer": "sector_transfer",
            "consumer": "instruction_fetch_correlation",
            "producer_present": True,
            "consumer_sequence": 1,
        }
    ]:
        raise AssertionError("transfer correlation selftest failed")
    if public_projection(synthetic)[1]["address"] is not None:
        raise AssertionError("private address was not redacted")
    print("causal trace analyzer selftest passed")


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    subparsers = parser.add_subparsers(dest="command", required=True)
    validate = subparsers.add_parser("validate")
    validate.add_argument("trace")
    compare = subparsers.add_parser("compare")
    compare.add_argument("first")
    compare.add_argument("second")
    summary = subparsers.add_parser("summary")
    summary.add_argument("trace")
    summary.add_argument("--compare")
    subparsers.add_parser("selftest")
    return parser.parse_args()


def main() -> int:
    arguments = parse_arguments()
    try:
        if arguments.command == "selftest":
            selftest()
        elif arguments.command == "validate":
            records = load_trace(pathlib.Path(arguments.trace))
            print(f"causal trace valid: events={len(records) - 2}")
        elif arguments.command == "compare":
            first = load_trace(pathlib.Path(arguments.first))
            second = load_trace(pathlib.Path(arguments.second))
            print(json.dumps(boundary_report(first, second), sort_keys=True, separators=(",", ":")))
            if pathlib.Path(arguments.first).read_bytes() != pathlib.Path(arguments.second).read_bytes():
                return 1
        else:
            result = summarize(pathlib.Path(arguments.trace), pathlib.Path(arguments.compare) if arguments.compare else None)
            print(json.dumps(result, sort_keys=True, separators=(",", ":"), ensure_ascii=True))
    except (CausalTraceError, OSError, json.JSONDecodeError) as exc:
        print(f"causal trace QA failed: {exc}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
