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
    "state_transition",
    "mailbox_boundary",
    "stop",
}
PRIVATE_TOKENS = ("/Users/", "/home/", "\\Users\\", "private", "rom", "d88")
PUBLIC_LEAKAGE_TOKENS = (
    "/users/", "/home/", "\\users\\", "pc88va-private-docs",
    "private-results", ".rom", ".d88", "varom", "vasubsys",
)

EXPECTED_PROVENANCE = (
    ("REQUEST_CONSUMED", "subsystem_request_consumer", "MAILBOX_RESPONSE_NOT_DELIVERED"),
    ("MOTOR_SETTLE_COMPLETED", "motor_settle", "MOTOR_COMPLETION_NOT_PRODUCED"),
    ("DRIVE_READY_CHANGED", "drive_ready", "READY_STATE_NOT_PROPAGATED"),
    ("MEDIA_SENSE_COMPLETED", "media_sense", "MEDIA_SENSE_NOT_PRODUCED"),
    ("RESPONSE_STATUS_WRITTEN", "response_status", "RESPONSE_STATUS_NOT_WRITTEN"),
    ("MAILBOX_RESPONSE_WRITTEN", "response_mailbox", "MAILBOX_RESPONSE_NOT_DELIVERED"),
    ("IRQ_RESPONSE_ASSERTED", "response_irq", "IRQ_RESPONSE_NOT_ASSERTED"),
    ("COMMAND_QUEUE_INSERTED", "command_queue", "COMMAND_QUEUE_NOT_POPULATED"),
    ("FDC_COMMAND_ATTEMPTED", "fdc_attempt", "FDC_ISSUE_PATH_ABSENT"),
)

MAILBOX_BOUNDARIES = (
    ("REQUEST_ACCEPTED", "ROUTE_NOT_SELECTED"),
    ("ROUTE_SELECTED", "ROUTE_NOT_SELECTED"),
    ("MAILBOX_ENQUEUE_ATTEMPTED", "ENQUEUE_NOT_ATTEMPTED"),
    ("MAILBOX_ENQUEUE_COMMITTED", "MAILBOX_WRITE_NOT_COMMITTED"),
    ("MAILBOX_REQUEST_VISIBLE", "MAILBOX_REQUEST_NOT_VISIBLE"),
    ("SUBSYSTEM_DISPATCHED", "SUBSYSTEM_NOT_SCHEDULED"),
    ("MAILBOX_DEQUEUE_ATTEMPTED", "DEQUEUE_NOT_ATTEMPTED"),
    ("CONSUMER_CALLBACK_ENTERED", "CONSUMER_CALLBACK_NOT_INVOKED"),
    ("REQUEST_CONSUMED", "REQUEST_CONSUMED_STATE_NOT_WRITTEN"),
    ("RESPONSE_ELIGIBLE", "REQUEST_CONSUMED_STATE_NOT_WRITTEN"),
)


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
        if event_class == "state_transition":
            required = {
                "component", "field", "old", "new", "cause", "producer",
                "transition", "correlation", "predicate",
            }
            if not required.issubset(record):
                raise CausalTraceError("state transition lacks provenance fields")
            if not isinstance(record["correlation"], int) or record["correlation"] < 0:
                raise CausalTraceError("state transition correlation is invalid")
            if not isinstance(record["predicate"], int) or not -1 <= record["predicate"] <= 2:
                raise CausalTraceError("state transition predicate is invalid")
        if event_class == "mailbox_boundary":
            required = {
                "step", "boundary", "producer", "consumer", "channel",
                "predecessor", "correlation", "predicate", "reason",
            }
            if not required.issubset(record):
                raise CausalTraceError("mailbox boundary lacks provenance fields")
            for field in ("boundary", "producer", "consumer", "channel",
                          "predecessor", "reason"):
                if not isinstance(record[field], str) or not record[field]:
                    raise CausalTraceError(
                        f"mailbox boundary {field} is invalid")
            if not isinstance(record["correlation"], int) or record["correlation"] < 0:
                raise CausalTraceError("mailbox boundary correlation is invalid")
            if not isinstance(record["predicate"], int) or not -1 <= record["predicate"] <= 2:
                raise CausalTraceError("mailbox boundary predicate is invalid")
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


def provenance_diagnostic(records: list[dict[str, Any]]) -> dict[str, Any]:
    """Attribute the first missing producer in the generic response path."""
    transitions = [
        record for record in records if record.get("class") == "state_transition"
    ]
    last = transitions[-1] if transitions else None
    names = {record.get("transition") for record in transitions}
    first_absent = None
    first_absent_site = None
    first_unmet = None
    predicate_state = "not_observable"
    classification = "TELEMETRY_DEFECT"
    for transition, producer, candidate in EXPECTED_PROVENANCE:
        observed = [
            record for record in transitions
            if record.get("transition") == transition
        ]
        if observed:
            state = observed[-1].get("predicate")
            if state != 1:
                first_unmet = transition
                first_absent_site = None
                predicate_state = {
                    0: "false", 2: "not_produced", -1: "not_observable",
                }.get(state, "not_observable")
                classification = candidate
                break
            continue
        if transition not in names:
            first_absent = transition
            first_absent_site = producer
            first_unmet = transition
            predicate_state = "not_produced"
            classification = candidate
            break
    if first_unmet is None:
        if any(record.get("transition") == "FDC_COMMAND_REJECTED" for record in transitions):
            classification = "TELEMETRY_DEFECT"
        else:
            classification = "FDC_ISSUE_PATH_ABSENT"
    last_writer: dict[str, str] = {}
    for record in transitions:
        field = record.get("field")
        producer = record.get("producer")
        if isinstance(field, str) and isinstance(producer, str):
            last_writer[field] = producer
    return {
        "correlation_ids": sorted(
            {record.get("correlation") for record in transitions
             if isinstance(record.get("correlation"), int)}
        ),
        "last_reached_transition": last.get("transition") if last else None,
        "current_abstract_phase": last.get("field") if last else None,
        "last_writer_by_field": dict(sorted(last_writer.items())),
        "first_absent_producer": first_absent_site,
        "first_absent_transition": first_absent,
        "first_absent_producer_site": first_absent_site,
        "first_unmet_predicate": first_unmet if first_unmet else "none",
        "predicate_state": predicate_state,
        "expected_producer_path": [
            {"transition": item[0], "producer": item[1]}
            for item in EXPECTED_PROVENANCE
        ],
        "pending_prerequisites": [
            item[0] for item in EXPECTED_PROVENANCE[
                next((index for index, item in enumerate(EXPECTED_PROVENANCE)
                      if item[0] == first_unmet), len(EXPECTED_PROVENANCE)):
            ]
        ],
        "classification": classification,
        "observed_producer_sites": sorted(
            {record.get("producer") for record in transitions
             if isinstance(record.get("producer"), str)}
        ),
    }


def mailbox_consumer_diagnostic(records: list[dict[str, Any]]) -> dict[str, Any]:
    """Analyze the source boundaries of one correlated mailbox request."""
    boundaries = [
        record for record in records if record.get("class") == "mailbox_boundary"
    ]
    request_records: dict[int, list[dict[str, Any]]] = {}
    for record in boundaries:
        correlation = record.get("correlation")
        if isinstance(correlation, int) and correlation != 0:
            request_records.setdefault(correlation, []).append(record)
    correlations = sorted(request_records)
    zero_correlation_records = [
        record for record in boundaries if record.get("correlation") == 0
    ]
    selected_correlation = None
    if request_records:
        selected_correlation = min(
            request_records,
            key=lambda correlation: (-len(request_records[correlation]), correlation),
        )
    selected = request_records.get(selected_correlation, [])
    by_name: dict[str, list[dict[str, Any]]] = {}
    for record in selected:
        by_name.setdefault(record["boundary"], []).append(record)
    mandatory = MAILBOX_BOUNDARIES[:9]
    mandatory_names = {name for name, _ in mandatory}
    mandatory_records = [record for record in selected
                         if record["boundary"] in mandatory_names]
    continuity = bool(selected) and all(
        isinstance(record.get("correlation"), int) and
        record.get("correlation") == selected_correlation
        for record in mandatory_records
    )
    reached: dict[str, bool] = {}
    first_absent: str | None = None
    first_unmet: str | None = None
    classification = "TELEMETRY_SEMANTICS_MISMATCH"
    for name, candidate in MAILBOX_BOUNDARIES:
        observed = by_name.get(name, [])
        good = any(record.get("predicate") == 1 for record in observed)
        reached[name] = good
        if not good and first_absent is None:
            first_absent = name
            first_unmet = name
            classification = candidate
    if first_absent is None and not selected:
        first_absent = MAILBOX_BOUNDARIES[0][0]
        first_unmet = first_absent
        classification = MAILBOX_BOUNDARIES[0][1]
    if zero_correlation_records or (not continuity and mandatory_records):
        classification = "CORRELATION_PROPAGATION_DEFECT"
    if all(reached.get(name, False) for name, _ in mandatory):
        classification = "REQUEST_CONSUMER_ESTABLISHED"
    last_reached = None
    for name, _ in MAILBOX_BOUNDARIES:
        if reached.get(name, False):
            last_reached = name
        else:
            break
    return {
        "reached": reached,
        "last_reached_boundary": last_reached,
        "first_absent_boundary": first_absent,
        "first_unmet_predicate": first_unmet,
        "correlation_ids": correlations,
        "selected_correlation": selected_correlation,
        "correlation_continuity": continuity,
        "classification": classification,
        "boundary_count": len(selected),
        "request_count": len(correlations),
        "per_request_boundary_counts": {
            str(correlation): len(request_records[correlation])
            for correlation in correlations
        },
    }


def verify_public_content(label: str, text: str) -> None:
    """Reject private input identities and paths in public text."""
    lowered = text.casefold()
    for token in PUBLIC_LEAKAGE_TOKENS:
        if token in lowered:
            raise CausalTraceError(
                f"public file {label} contains a forbidden private token: {token}"
            )


def verify_public_text(path: pathlib.Path) -> None:
    """Reject private input identities and paths in a public source file."""
    try:
        text = path.read_text(encoding="utf-8")
    except OSError as exc:
        raise CausalTraceError(f"cannot read public file: {path}") from exc
    verify_public_content(str(path), text)


def verify_public_paths(paths: list[pathlib.Path]) -> None:
    if not paths:
        raise CausalTraceError("public leakage check requires at least one file")
    for path in paths:
        if not path.is_file():
            raise CausalTraceError(f"public leakage check requires a regular file: {path}")
        verify_public_text(path)


def redact(record: Any, fields: set[str]) -> Any:
    if isinstance(record, dict):
        return {key: (None if key in fields else redact(value, fields))
                for key, value in sorted(record.items())}
    if isinstance(record, list):
        return [redact(item, fields) for item in record]
    return record


def public_projection(records: list[dict[str, Any]]) -> list[dict[str, Any]]:
    projection = redact(records, {"address", "value", "physical", "opcode", "cs", "ip", "ax", "bx", "cx", "dx", "si", "di", "bp", "sp", "es", "ss", "ds", "flags", "if", "reason", "step", "old", "new"})
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
        "provenance": provenance_diagnostic(first),
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
    provenance = synthetic[:1] + [
        {
            "seq": 0,
            "class": "state_transition",
            "component": "fd-subsystem",
            "field": "handshake_phase",
            "old": 1,
            "new": 2,
            "cause": "handshake",
            "producer": "subsystem_request_consumer",
            "transition": "REQUEST_CONSUMED",
            "correlation": 1,
            "predicate": 1,
        },
        {
            "seq": 1,
            "class": "state_transition",
            "component": "drive",
            "field": "motor_state",
            "old": 1,
            "new": 2,
            "cause": "timer",
            "producer": "motor_settle",
            "transition": "MOTOR_SETTLE_COMPLETED",
            "correlation": 1,
            "predicate": 1,
        },
        {
            "seq": 2,
            "class": "state_transition",
            "component": "drive",
            "field": "drive_ready",
            "old": 0,
            "new": 1,
            "cause": "drive",
            "producer": "drive_ready",
            "transition": "DRIVE_READY_CHANGED",
            "correlation": 1,
            "predicate": 1,
        },
        {
            "seq": 3,
            "class": "state_transition",
            "component": "drive",
            "field": "media_sense",
            "old": 0,
            "new": 1,
            "cause": "media",
            "producer": "media_sense",
            "transition": "MEDIA_SENSE_COMPLETED",
            "correlation": 1,
            "predicate": 1,
        },
        {
            "seq": 4,
            "class": "state_transition",
            "component": "fd-subsystem",
            "field": "response_status",
            "old": 0,
            "new": 1,
            "cause": "fdc-result",
            "producer": "response_status",
            "transition": "RESPONSE_STATUS_WRITTEN",
            "correlation": 1,
            "predicate": 1,
        },
        {
            "seq": 5,
            "class": "state_transition",
            "component": "mailbox",
            "field": "response_mailbox",
            "old": 0,
            "new": 1,
            "cause": "handshake",
            "producer": "response_mailbox",
            "transition": "MAILBOX_RESPONSE_WRITTEN",
            "correlation": 1,
            "predicate": 1,
        },
        {
            "seq": 6,
            "class": "state_transition",
            "component": "mailbox",
            "field": "response_irq",
            "old": 0,
            "new": 1,
            "cause": "handshake",
            "producer": "response_irq",
            "transition": "IRQ_RESPONSE_ASSERTED",
            "correlation": 1,
            "predicate": 1,
        },
        {"seq": 7, "class": "stop", "reason": "bound", "events": 7},
    ]
    diagnostic = provenance_diagnostic(provenance)
    if diagnostic["classification"] != "COMMAND_QUEUE_NOT_POPULATED":
        raise AssertionError("provenance first-absent selftest failed")
    if diagnostic["correlation_ids"] != [1]:
        raise AssertionError("provenance correlation selftest failed")
    false_ready = provenance[:4] + [{
        "seq": 4,
        "class": "state_transition",
        "component": "drive",
        "field": "drive_ready",
        "old": 1,
        "new": 0,
        "cause": "drive",
        "producer": "drive_ready",
        "transition": "DRIVE_READY_CHANGED",
        "correlation": 1,
        "predicate": 0,
    }, {"seq": 5, "class": "stop", "reason": "bound", "events": 5}]
    false_diagnostic = provenance_diagnostic(false_ready)
    if false_diagnostic["classification"] != "READY_STATE_NOT_PROPAGATED":
        raise AssertionError("false predicate classification selftest failed")
    if false_diagnostic["first_absent_producer"] is not None:
        raise AssertionError("observed false predicate was called absent")
    if false_diagnostic["predicate_state"] != "false":
        raise AssertionError("false predicate state selftest failed")
    multi_request = provenance[:3] + [
        dict(provenance[1], seq=3, correlation=2),
        dict(provenance[2], seq=4, correlation=2),
        {"seq": 5, "class": "stop", "reason": "bound", "events": 5},
    ]
    if provenance_diagnostic(multi_request)["correlation_ids"] != [1, 2]:
        raise AssertionError("multiple request correlation selftest failed")
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
    try:
        verify_public_content("safe", "stable enum producer contract")
    except CausalTraceError:
        raise AssertionError("safe public text selftest failed")
    try:
        verify_public_content("unsafe", "source=.d88")
    except CausalTraceError:
        pass
    else:
        raise AssertionError("private leakage selftest failed")
    mailbox = [synthetic[0]] + [
        {
            "seq": index,
            "class": "mailbox_boundary",
            "step": index,
            "boundary": name,
            "producer": "stable_producer",
            "consumer": "stable_consumer",
            "channel": "stable-channel",
            "predecessor": "none" if index == 0 else MAILBOX_BOUNDARIES[index - 1][0],
            "correlation": 1,
            "predicate": 1,
            "reason": "synthetic",
        }
        for index, (name, _) in enumerate(MAILBOX_BOUNDARIES)
    ] + [{"seq": 10, "class": "stop", "reason": "bound", "events": 10}]
    mailbox_diagnostic = mailbox_consumer_diagnostic(mailbox)
    if (mailbox_diagnostic["classification"] != "REQUEST_CONSUMER_ESTABLISHED" or
            not mailbox_diagnostic["correlation_continuity"]):
        raise AssertionError("mailbox boundary selftest failed")
    incomplete = mailbox[:1] + mailbox[1:5] + [
        {"seq": 5, "class": "stop", "reason": "bound", "events": 4}
    ]
    incomplete_diagnostic = mailbox_consumer_diagnostic(incomplete)
    if incomplete_diagnostic["classification"] != "MAILBOX_REQUEST_NOT_VISIBLE":
        raise AssertionError("mailbox first-absent selftest failed")
    multi_mailbox = [mailbox[0]] + mailbox[1:-1] + [
        dict(mailbox[index], seq=index + 10, correlation=2)
        for index in range(1, 11)
    ] + [{"seq": 20, "class": "stop", "reason": "bound", "events": 20}]
    multi_mailbox_diagnostic = mailbox_consumer_diagnostic(multi_mailbox)
    if (multi_mailbox_diagnostic["classification"] != "REQUEST_CONSUMER_ESTABLISHED" or
            multi_mailbox_diagnostic["request_count"] != 2 or
            not multi_mailbox_diagnostic["correlation_continuity"]):
        raise AssertionError("multiple mailbox correlation selftest failed")
    zero_mailbox = mailbox[:1] + [
        dict(mailbox[1], correlation=0),
        mailbox[2],
        {"seq": 2, "class": "stop", "reason": "bound", "events": 2},
    ]
    if mailbox_consumer_diagnostic(zero_mailbox)["classification"] != "CORRELATION_PROPAGATION_DEFECT":
        raise AssertionError("zero mailbox correlation selftest failed")
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
    provenance = subparsers.add_parser("provenance")
    provenance.add_argument("trace")
    mailbox = subparsers.add_parser("mailbox")
    mailbox.add_argument("trace")
    privacy = subparsers.add_parser("privacy-check")
    privacy.add_argument("paths", nargs="+")
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
        elif arguments.command == "provenance":
            records = load_trace(pathlib.Path(arguments.trace))
            print(json.dumps(provenance_diagnostic(records), sort_keys=True,
                             separators=(",", ":"), ensure_ascii=True))
        elif arguments.command == "mailbox":
            records = load_trace(pathlib.Path(arguments.trace))
            print(json.dumps(mailbox_consumer_diagnostic(records), sort_keys=True,
                             separators=(",", ":"), ensure_ascii=True))
        elif arguments.command == "privacy-check":
            verify_public_paths([pathlib.Path(path) for path in arguments.paths])
            print("causal trace public leakage check passed")
        else:
            result = summarize(pathlib.Path(arguments.trace), pathlib.Path(arguments.compare) if arguments.compare else None)
            print(json.dumps(result, sort_keys=True, separators=(",", ":"), ensure_ascii=True))
    except (CausalTraceError, OSError, json.JSONDecodeError) as exc:
        print(f"causal trace QA failed: {exc}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
