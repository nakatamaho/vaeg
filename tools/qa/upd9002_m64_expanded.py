#!/usr/bin/env python3
# Copyright (c) 2026 Nakata Maho
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
# 1. Redistributions of source code must retain the above copyright notice,
#    this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
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
"""Audit expanded M64 ownership and derive the exact G64 target policy."""

from __future__ import annotations

import argparse
import contextlib
import copy
import csv
import gzip
import hashlib
import json
import pathlib
import re
import sys
import tempfile
from collections import Counter
from collections.abc import Iterable, Iterator
from typing import Any

import upd9002_m60b_authority as m60b
import upd9002_m61_mov_imm as m61
import upd9002_m62_bundle as m62
import upd9002_semantics_evidence as m59
import upd9002_ssts as ssts
import upd9002_ssts_ratchet as ratchet


MILESTONE = "M64"
CANDIDATE_GATE = "G64"
APPROVED_PREDECESSOR_GATE = "G62"
APPROVED_PREDECESSOR_SHA = "70b8e94e96aef4cb79eed72c7813c4148c5c0dd8"
G62_EVALUATED_SHA = "2cdaed95072d74bbf7187ae854fb31d3886c995d"
G62_POLICY_ID = (
    "upd9002-g62-"
    "6961b0f295110d32d16799cb3799bedff7600b9b956bc4ad893eebc249140212"
)
G62_POLICY_SHA256 = (
    "6961b0f295110d32d16799cb3799bedff7600b9b956bc4ad893eebc249140212"
)
G62_MANIFEST_PATH = pathlib.Path("tests/ssts/evidence/g62/manifest.json")
G62_MANIFEST_SHA256 = (
    "b15fef00aa66a342734781d5b6e1b9c183de2fbda8fb8bf5ccf0ee2f5753d847"
)
G60B_AUTHORITY_PATH = pathlib.Path("tests/ssts/authority/g60b/manifest.json")
G60B_AUTHORITY_SHA256 = (
    "f14fa57e8aedb54c773e55c94d55572d3c99e00457c01c75df3507582c35f1ac"
)
DATASET_ID = m61.DATASET_ID
CONTRACTS = m61.CONTRACTS
SELECTED_HASH_SETS = m61.SELECTED_HASH_SETS
SUPPORT_MAP_PATH = pathlib.Path("tools/qa/golden/upd9002_support_map_m48.csv")
DATASET_MANIFEST_PATH = m61.DATASET_MANIFEST_PATH
TARGET_POLICY_PATH = pathlib.Path("tests/ssts/target_policy/g64.json")
TASK_PATH = pathlib.Path("docs/agents/tasks/M64_upd9002_div_idiv.md")
REPORT_PATH = pathlib.Path("docs/agents/reports/m64_upd9002_div_idiv.md")
HEX40 = re.compile(r"^[0-9a-f]{40}$")
HEX64 = re.compile(r"^[0-9a-f]{64}$")

PHASE_FORMS = {
    "div_idiv": ("F6.6", "F6.7", "F7.6", "F7.7"),
    "add4s_sub4s_cmp4s": ("0F20", "0F22", "0F26"),
    "bit_operations": (
        "0F10", "0F11", "0F18", "0F19",
        "0F12", "0F13", "0F1A", "0F1B",
        "0F14", "0F15", "0F1C", "0F1D",
        "0F16", "0F17", "0F1E", "0F1F",
    ),
    "brkem": ("0FFF",),
}
PHASE_ORDER = tuple(PHASE_FORMS)
ACTIVATIONS = {
    "0F13": "v30_clr1_ea16_cl",
    "0F15": "v30_set1_ea16_cl",
    "0F16": "v30_not1_ea8_cl",
    "0F17": "v30_not1_ea16_cl",
    "0F1E": "v30_not1_ea8_i3",
    "0F1F": "v30_not1_ea16_i4",
    "0F26": "v30_cmp4s",
}
ACTIVATION_COUNTS_FULL = {
    "0F13": 5000,
    "0F15": 5000,
    "0F16": 5000,
    "0F17": 5000,
    "0F1E": 5000,
    "0F1F": 5000,
    "0F26": 1000,
}
EXPECTED_DIV_FAILURES = {
    "F6.6": 2561,
    "F6.7": 3716,
    "F7.6": 2486,
    "F7.7": 3723,
}
ROM_RECORDS = {
    "ADD4S": (("ff", "20", "00"), ("ff", "20", "01")),
    "SUB4S": (("ff", "22", "00"), ("ff", "22", "01")),
    "CMP4S": (("ff", "26", "00"), ("ff", "26", "01")),
    "TEST1": (("f6", "10", "03"),),
    "NOT1": (("f6", "16", "03"),),
    "CLR1": (("f6", "12", "03"),),
    "SET1": (("f6", "14", "03"),),
    "BRKEM": (("ff", "ff", "04"),),
}
BRKEM_COVERAGE = {
    "compatibility_scope": "no_v20_sst_cases",
    "selected": 0,
    "executed": 0,
    "sst_contract_status": "not_yet_present",
    "silicon_mode_identity": "underdetermined",
}
EMPTY_HASH_SET_SHA256 = ratchet.hash_set_digest([])


class M64Error(RuntimeError):
    """Raised when M64 evidence fails closed."""


def reject(code: str, detail: str) -> None:
    raise M64Error(f"{code}: {detail}")


def require(condition: bool, code: str, detail: str) -> None:
    if not condition:
        reject(code, detail)


def canonical_bytes(value: Any) -> bytes:
    return json.dumps(
        value, ensure_ascii=True, sort_keys=True, separators=(",", ":")
    ).encode("utf-8")


def sha256_bytes(value: bytes) -> str:
    return hashlib.sha256(value).hexdigest()


def sha256_file(path: pathlib.Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def read_json(path: pathlib.Path) -> Any:
    return json.loads(path.read_text(encoding="utf-8"))


def write_json(path: pathlib.Path, value: Any) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(canonical_bytes(value) + b"\n")


def verify_predecessor(root: pathlib.Path) -> None:
    require(
        sha256_file(root / G62_MANIFEST_PATH) == G62_MANIFEST_SHA256,
        "protected-g62-mutation",
        str(G62_MANIFEST_PATH),
    )
    require(
        sha256_file(root / G60B_AUTHORITY_PATH) == G60B_AUTHORITY_SHA256,
        "rom-authority-mutation",
        str(G60B_AUTHORITY_PATH),
    )
    policy = read_json(root / m62.TARGET_POLICY_PATH)
    require(
        policy.get("target_policy_id") == G62_POLICY_ID
        and policy.get("target_policy_sha256") == G62_POLICY_SHA256,
        "wrong-g62-policy",
        repr(policy.get("target_policy_id")),
    )


def expand_mask(mask: int, value: int) -> list[int]:
    require((value & mask) == value, "malformed-rom-record", f"{mask:02x}:{value:02x}")
    return [opcode for opcode in range(256) if (opcode & mask) == value]


def verify_rom_contract() -> None:
    require(
        expand_mask(0xF6, 0x10) == [0x10, 0x11, 0x18, 0x19],
        "f6-mask-expansion",
        "TEST1",
    )
    require(
        expand_mask(0xF6, 0x12) == [0x12, 0x13, 0x1A, 0x1B],
        "f6-mask-expansion",
        "CLR1",
    )
    require(
        expand_mask(0xF6, 0x14) == [0x14, 0x15, 0x1C, 0x1D],
        "f6-mask-expansion",
        "SET1",
    )
    require(
        expand_mask(0xF6, 0x16) == [0x16, 0x17, 0x1E, 0x1F],
        "f6-mask-expansion",
        "NOT1",
    )
    require(
        all(len(record) == 3 for records in ROM_RECORDS.values() for record in records),
        "raw-record-width",
        "ROM dispatch records are not three bytes wide",
    )
    require(
        ("ff", "fe", "04") not in {
            record for records in ROM_RECORDS.values() for record in records
        },
        "brkfem-included",
        "0FFE is outside M64",
    )


def g64_support_rows(rows: list[dict[str, str]]) -> list[dict[str, str]]:
    result = m62.g62_support_rows(rows)
    changed: set[str] = set()
    for row in result:
        if row["mode"] != "v30op_0f" or row["opcode"] != "0x0f":
            continue
        form = f"0F{int(row['subopcode'], 16):02X}"
        if form not in ACTIVATIONS:
            continue
        require(
            row["classification"] == "known_target_gap"
            and row["target"] == "v30_reserved_0x0f",
            "live-policy-conflict",
            repr(row),
        )
        row["classification"] = "implemented"
        row["target"] = ACTIVATIONS[form]
        row["basis"] = "g60b-rom-authority+m64-sst-contract"
        changed.add(form)
    require(changed == set(ACTIVATIONS), "partial-activation", repr(sorted(changed)))
    return result


@contextlib.contextmanager
def support_map(root: pathlib.Path, epoch: str) -> Iterator[pathlib.Path]:
    require(epoch in {"g62", "g64"}, "unknown-policy-epoch", epoch)
    fields, rows = m60b.read_support_rows(root / SUPPORT_MAP_PATH)
    candidate = m62.g62_support_rows(rows)
    if epoch == "g64":
        candidate = g64_support_rows(rows)
    with tempfile.TemporaryDirectory(prefix=f"vaeg-m64-{epoch}-support-") as name:
        path = pathlib.Path(name) / f"upd9002_support_map_{epoch}.csv"
        with path.open("w", encoding="utf-8", newline="") as stream:
            writer = csv.DictWriter(stream, fieldnames=fields, lineterminator="\n")
            writer.writeheader()
            writer.writerows(candidate)
        ssts.load_support_map(path)
        yield path


def policy_enumerations(
    root: pathlib.Path, dataset_root: pathlib.Path
) -> dict[str, dict[str, Any]]:
    manifest = ssts.load_manifest(root / DATASET_MANIFEST_PATH)
    require(manifest["dataset_id"] == DATASET_ID, "dataset-drift", "manifest")
    ssts.verify_fast(dataset_root, manifest)
    metadata = read_json(dataset_root / ssts.SUITE_PATH / "metadata.json")
    ssts.validate_metadata(metadata)
    predecessor = read_json(root / m62.TARGET_POLICY_PATH)
    values: dict[str, dict[str, Any]] = {}
    with support_map(root, "g62") as before, support_map(root, "g64") as after:
        before_support = ssts.load_support_map(before)
        after_support = ssts.load_support_map(after)
        work = {
            scope: {
                "after_applicable": [],
                "after_form_counts": {},
                "changes": [],
                "selected_count": 0,
            }
            for scope in ("ci", "full")
        }
        for path in ssts.corpus_files(dataset_root):
            form = path.name.removesuffix(".json.gz").upper()
            with gzip.open(path, "rt", encoding="utf-8") as stream:
                records = ssts.profile_records(json.load(stream), "full")
            counts = {"ci": Counter(), "full": Counter()}
            for record_index, raw_record in enumerate(records):
                scopes = ("ci", "full") if record_index < 500 else ("full",)
                record = ssts.validate_record(
                    raw_record, f"{path.name}:{raw_record.get('idx')}"
                )
                before_classification = ssts.classify_record(
                    form, record, metadata, before_support
                )["classification"]
                after_classification = ssts.classify_record(
                    form, record, metadata, after_support
                )["classification"]
                record_hash = sha256_bytes(canonical_bytes(record))
                for scope in scopes:
                    work[scope]["selected_count"] += 1
                    counts[scope][after_classification] += 1
                    if after_classification == "applicable":
                        work[scope]["after_applicable"].append(record_hash)
                    if before_classification != after_classification:
                        work[scope]["changes"].append(
                            {
                                "after": after_classification,
                                "before": before_classification,
                                "form": form,
                                "record_hash": record_hash,
                                "upstream_test_hash": record["hash"],
                            }
                        )
            for scope in ("ci", "full"):
                work[scope]["after_form_counts"][form] = counts[scope]
        for scope in ("ci", "full"):
            after_applicable = work[scope]["after_applicable"]
            changes = work[scope]["changes"]
            after_form_counts = work[scope]["after_form_counts"]
            selected_count = work[scope]["selected_count"]
            changes.sort(key=lambda item: item["record_hash"])
            require(
                all(
                    item["form"] in ACTIVATIONS
                    and item["before"] == "known_target_gap"
                    and item["after"] == "applicable"
                    for item in changes
                ),
                "unauthorized-classification-change",
                scope,
            )
            predecessor_sets = predecessor["applicable_hash_sets"][scope]
            after_digest = ratchet.hash_set_digest(after_applicable)
            value = {
                "after_form_counts": after_form_counts,
                "after_set_digests": {"applicable": after_digest},
                "after_sets": {"applicable": after_applicable},
                "before_set_digests": {
                    "applicable": predecessor_sets["after_sha256"]
                },
                "before_sets": {
                    "applicable": [None] * predecessor_sets["after_count"]
                },
                "classification_changes": changes,
                "selected_count": selected_count,
                "selected_hash_set_sha256": SELECTED_HASH_SETS[scope],
            }
            require(
                value["selected_hash_set_sha256"] == SELECTED_HASH_SETS[scope],
                "selected-set-drift",
                scope,
            )
            values[scope] = value
    full = Counter(item["form"] for item in values["full"]["classification_changes"])
    require(dict(full) == ACTIVATION_COUNTS_FULL, "partial-activation", repr(full))
    return values


def gap_selector_metadata(root: pathlib.Path) -> dict[str, dict[str, Any]]:
    known = read_json(root / "tests/ssts/baseline/upd9002_v20_known_gaps.json")
    taxonomy = read_json(root / "tests/ssts/gap_taxonomy.json")
    annotations = {entry["selector_sha256"]: entry for entry in taxonomy["annotations"]}
    result: dict[str, dict[str, Any]] = {}
    for rule in known["rules"]:
        form = rule["selector"]["metadata_form"]
        if form not in ACTIVATIONS:
            continue
        selector_sha = sha256_bytes(canonical_bytes(rule["selector"]))
        annotation = annotations.get(selector_sha)
        require(annotation is not None, "missing-gap-taxonomy", form)
        require(
            annotation["gap_kind"] == "implementation_missing",
            "unauthorized-gap-kind",
            form,
        )
        result[form] = {
            "before_gap_kind": "implementation_missing",
            "after_gap_kind": None,
            "form": form,
            "resolved_count": rule["resolved_count"],
            "resolved_test_hashes_sha256": annotation[
                "resolved_test_hashes_sha256"
            ],
            "selector_sha256": selector_sha,
        }
    require(set(result) == set(ACTIVATIONS), "missing-gap-selector", repr(result))
    return result


def generate_target_policy(
    root: pathlib.Path, dataset_root: pathlib.Path, evaluated_sha: str
) -> dict[str, Any]:
    require(HEX40.fullmatch(evaluated_sha) is not None, "evaluated-sha", evaluated_sha)
    enumerations = policy_enumerations(root, dataset_root)
    predecessor = read_json(root / m62.TARGET_POLICY_PATH)
    gaps = gap_selector_metadata(root)
    body = {
        "applicable_hash_sets": {
            scope: {
                "before_count": len(enumerations[scope]["before_sets"]["applicable"]),
                "before_sha256": enumerations[scope]["before_set_digests"]["applicable"],
                "after_count": len(enumerations[scope]["after_sets"]["applicable"]),
                "after_sha256": enumerations[scope]["after_set_digests"]["applicable"],
            }
            for scope in ("ci", "full")
        },
        "approved_predecessor_gate": APPROVED_PREDECESSOR_GATE,
        "approved_predecessor_sha": APPROVED_PREDECESSOR_SHA,
        "authority_manifest_sha256": G60B_AUTHORITY_SHA256,
        "brkem_coverage": BRKEM_COVERAGE,
        "candidate_gate": CANDIDATE_GATE,
        "classification_changes": {
            scope: {
                "count": len(enumerations[scope]["classification_changes"]),
                "forms": dict(
                    sorted(
                        Counter(
                            item["form"]
                            for item in enumerations[scope]["classification_changes"]
                        ).items()
                    )
                ),
                "record_hash_set_sha256": ratchet.hash_set_digest(
                    item["record_hash"]
                    for item in enumerations[scope]["classification_changes"]
                ),
                "upstream_hash_set_sha256": ratchet.upstream_hash_set_digest(
                    item["upstream_test_hash"]
                    for item in enumerations[scope]["classification_changes"]
                ),
            }
            for scope in ("ci", "full")
        },
        "comparison_contracts": CONTRACTS,
        "dataset_id": DATASET_ID,
        "epoch_gate": CANDIDATE_GATE,
        "evaluated_sha": evaluated_sha,
        "gap_kind_changes": [gaps[form] for form in sorted(gaps)],
        "milestone": MILESTONE,
        "predecessor_policy": {
            "target_policy_id": predecessor["target_policy_id"],
            "target_policy_sha256": predecessor["target_policy_sha256"],
        },
        "schema": "vaeg-upd9002-target-policy-v3",
        "schema_version": 3,
        "selected_hash_sets": {
            scope: enumerations[scope]["selected_hash_set_sha256"]
            for scope in ("ci", "full")
        },
        "taxonomy_counts": {
            "before": predecessor["taxonomy_counts"]["after"],
            "after": {
                "documented_silicon_absent": 32000,
                "implementation_missing": 5908,
                "target_support_unverified": 0,
            },
        },
        "transition_kind": "monitor_authority_activation",
    }
    digest = sha256_bytes(canonical_bytes(body))
    value = {
        **body,
        "target_policy_id": f"upd9002-g64-{digest}",
        "target_policy_sha256": digest,
    }
    validate_target_policy(value)
    return value


def validate_target_policy(value: Any) -> None:
    require(isinstance(value, dict), "target-policy-schema", "not an object")
    digest = value.get("target_policy_sha256")
    require(
        isinstance(digest, str) and HEX64.fullmatch(digest) is not None,
        "target-policy-digest",
        repr(digest),
    )
    body = {
        key: item
        for key, item in value.items()
        if key not in {"target_policy_id", "target_policy_sha256"}
    }
    require(
        sha256_bytes(canonical_bytes(body)) == digest
        and value.get("target_policy_id") == f"upd9002-g64-{digest}",
        "target-policy-digest",
        "content identity differs",
    )
    require(
        value.get("approved_predecessor_sha") == APPROVED_PREDECESSOR_SHA,
        "wrong-predecessor",
        "G62 SHA differs",
    )
    require(value.get("dataset_id") == DATASET_ID, "dataset-drift", "policy")
    require(value.get("comparison_contracts") == CONTRACTS, "contract-drift", "policy")
    require(
        value.get("selected_hash_sets") == SELECTED_HASH_SETS,
        "selected-set-drift",
        "policy",
    )
    require(
        value.get("authority_manifest_sha256") == G60B_AUTHORITY_SHA256,
        "rom-authority-missing",
        "policy",
    )
    changes = value.get("classification_changes", {}).get("full", {})
    require(
        changes.get("count") == 31000
        and changes.get("forms") == ACTIVATION_COUNTS_FULL,
        "partial-activation",
        repr(changes),
    )
    require(
        value.get("brkem_coverage") == BRKEM_COVERAGE,
        "fabricated-brkem-coverage",
        repr(value.get("brkem_coverage")),
    )
    require(
        {item.get("form") for item in value.get("gap_kind_changes", [])}
        == set(ACTIVATIONS),
        "unauthorized-gap-kind-change",
        repr(value.get("gap_kind_changes")),
    )


def load_selected_records(dataset_root: pathlib.Path, form: str) -> list[dict[str, Any]]:
    path = dataset_root / ssts.SUITE_PATH / f"{form}.json.gz"
    if form == "0FFF":
        require(not path.exists(), "unexpected-brkem-sst-shard", str(path))
        return []
    require(path.is_file(), "missing-sst-shard", form)
    with gzip.open(path, "rt", encoding="utf-8") as stream:
        return ssts.profile_records(json.load(stream), "full")


def case_row(
    form: str,
    record: dict[str, Any],
    resolved: dict[str, Any],
    actual: dict[str, Any],
) -> dict[str, Any]:
    expected_regs = ssts.expected_registers(record)
    watch, expected_ram = ssts.expected_memory(record)
    context = {
        "record": record,
        "record_digest": sha256_bytes(canonical_bytes(record)),
        "watch": watch,
        "expected_ram": expected_ram,
    }
    architectural = ssts.make_failure(
        DATASET_ID,
        "full",
        form,
        resolved["classification"],
        resolved["flags_mask"],
        context,
        "ok",
        actual,
    )["content"]
    fingerprint = ssts.make_failure(
        DATASET_ID,
        "full",
        form,
        resolved["classification"],
        0xFFFF,
        context,
        "ok",
        actual,
    )["content"]
    initial = record["initial"]["regs"]
    return {
        "actual_final_state": {
            "ram": m59.ram_entries(actual["ram"]),
            "registers": {
                key: f"{value:04x}" for key, value in sorted(actual["registers"].items())
            },
            "termination": actual["termination"],
        },
        "architectural_mismatch_kinds": architectural["mismatch_kinds"],
        "architectural_outcome": (
            "pass" if not architectural["mismatch_kinds"] else "fail"
        ),
        "case_hash": context["record_digest"],
        "complete_instruction_bytes": "".join(f"{byte:02x}" for byte in record["bytes"]),
        "conclusion_status": "proven",
        "evidence_notes": "Expected and actual worker states are recorded side by side.",
        "expected_final_state": {
            "ram": m59.ram_entries(expected_ram),
            "registers": {
                key: f"{value:04x}" for key, value in sorted(expected_regs.items())
            },
            "termination": ssts.expected_termination(form, record),
        },
        "fingerprint_mismatch_kinds": fingerprint["mismatch_kinds"],
        "fingerprint_outcome": (
            "pass" if not fingerprint["mismatch_kinds"] else "fail"
        ),
        "form": form,
        "initial_state": {
            "ram": m59.ram_entries({address: value for address, value in record["initial"]["ram"]}),
            "registers": {
                key: f"{value:04x}" for key, value in sorted(initial.items())
            },
        },
        "selected": True,
        "structural_partition": {
            "modrm_mod": (
                None if len(record["bytes"]) < 2 else (record["bytes"][-1] >> 6)
            ),
            "prefix_sequence": [
                f"{byte:02x}" for byte in record["bytes"][:-1]
            ],
        },
        "top_level_classification": resolved["classification"],
        "upstream_case_hash": record["hash"],
    }


def run_phase_audit(
    root: pathlib.Path,
    dataset_root: pathlib.Path,
    worker: pathlib.Path,
    phase: str,
    epoch: str,
) -> dict[str, Any]:
    require(phase in PHASE_FORMS, "unknown-phase", phase)
    manifest = ssts.load_manifest(root / DATASET_MANIFEST_PATH)
    require(manifest["dataset_id"] == DATASET_ID, "dataset-drift", phase)
    ssts.verify_fast(dataset_root, manifest)
    metadata = read_json(dataset_root / ssts.SUITE_PATH / "metadata.json")
    ssts.validate_metadata(metadata)
    rows_by_form: dict[str, list[dict[str, Any]]] = {}
    summaries: dict[str, dict[str, Any]] = {}
    with support_map(root, epoch) as support_path:
        support = ssts.load_support_map(support_path)
        for form in PHASE_FORMS[phase]:
            selected = load_selected_records(dataset_root, form)
            if form == "0FFF":
                rows_by_form[form] = []
                summaries[form] = {
                    **BRKEM_COVERAGE,
                    "record_hash_set_sha256": EMPTY_HASH_SET_SHA256,
                }
                continue
            resolved = [
                ssts.classify_record(form, record, metadata, support)
                for record in selected
            ]
            results = ssts.run_worker_contained(worker, selected, 120.0)
            require(
                len(results) == len(selected)
                and all(status == "ok" for status, _ in results),
                "missing-executed-record",
                form,
            )
            rows = [
                case_row(form, record, classification, actual)
                for record, classification, (_, actual) in zip(
                    selected, resolved, results
                )
            ]
            rows.sort(key=lambda item: item["case_hash"])
            require(
                len(rows) == len({item["case_hash"] for item in rows}),
                "duplicate-hash",
                form,
            )
            passing = [
                item["case_hash"]
                for item in rows
                if item["architectural_outcome"] == "pass"
            ]
            failing = [
                item["case_hash"]
                for item in rows
                if item["architectural_outcome"] == "fail"
            ]
            classifications = Counter(item["top_level_classification"] for item in rows)
            summaries[form] = {
                "architectural_failure_count": len(failing),
                "architectural_failure_hash_set_sha256": ratchet.hash_set_digest(failing),
                "architectural_pass_count": len(passing),
                "architectural_pass_hash_set_sha256": ratchet.hash_set_digest(passing),
                "classification_counts": dict(sorted(classifications.items())),
                "executed_by_worker": len(rows),
                "record_hash_set_sha256": ratchet.hash_set_digest(
                    item["case_hash"] for item in rows
                ),
                "selected": len(rows),
            }
            rows_by_form[form] = rows
    return {
        "epoch": epoch,
        "forms": list(PHASE_FORMS[phase]),
        "phase": phase,
        "rows": rows_by_form,
        "schema": "vaeg-upd9002-m64-phase-audit-v1",
        "schema_version": 1,
        "summaries": summaries,
        "worker_sha256": sha256_file(worker),
    }


def write_phase_audit(output: pathlib.Path, value: dict[str, Any], source_sha: str) -> None:
    require(HEX40.fullmatch(source_sha) is not None, "source-sha", source_sha)
    output.mkdir(parents=True, exist_ok=True)
    for form in value["forms"]:
        ratchet.write_deterministic_gzip(
            output / f"{form.lower().replace('.', '_')}_cases.json.gz",
            {
                "form": form,
                "row_count": len(value["rows"][form]),
                "rows": value["rows"][form],
                "schema": "vaeg-upd9002-m64-cases-v1",
                "schema_version": 1,
            },
        )
    write_json(
        output / "phase_summary.json",
        {
            key: item for key, item in value.items() if key != "rows"
        }
        | {"source_sha": source_sha},
    )


def verify_static(root: pathlib.Path) -> None:
    verify_predecessor(root)
    verify_rom_contract()
    task = (root / TASK_PATH).read_text(encoding="utf-8")
    require(
        "topic/m64-upd9002-div-idiv" in task
        and "compatibility_scope = no_v20_sst_cases" in task
        and "contains no `0FFF.json.gz` shard" in task,
        "scope-expansion-documentation",
        str(TASK_PATH),
    )
    manifest = read_json(root / DATASET_MANIFEST_PATH)
    paths = {entry["path"] for entry in manifest["files"]}
    require(
        "v1_native/0FFF.json.gz" not in paths,
        "unexpected-brkem-sst-shard",
        "dataset manifest",
    )
    require(
        not (root / "tests/ssts/v1_native/0FFF.json.gz").exists(),
        "unexpected-brkem-sst-shard",
        "repository",
    )
    print(
        "m64-static: G62/G60b identities protected; requested ROM forms are "
        "bounded; BRKEM has exact zero SST-v20 coverage"
    )


def selftest() -> None:
    verify_rom_contract()
    good = {
        "approved_predecessor_sha": APPROVED_PREDECESSOR_SHA,
        "authority_manifest_sha256": G60B_AUTHORITY_SHA256,
        "brkem_coverage": BRKEM_COVERAGE,
        "classification_changes": {
            "full": {"count": 31000, "forms": ACTIVATION_COUNTS_FULL}
        },
        "comparison_contracts": CONTRACTS,
        "dataset_id": DATASET_ID,
        "gap_kind_changes": [{"form": form} for form in sorted(ACTIVATIONS)],
        "selected_hash_sets": SELECTED_HASH_SETS,
    }

    def validate_body(value: dict[str, Any]) -> None:
        require(
            value["approved_predecessor_sha"] == APPROVED_PREDECESSOR_SHA,
            "wrong-predecessor",
            "selftest",
        )
        require(value["dataset_id"] == DATASET_ID, "dataset-drift", "selftest")
        require(value["comparison_contracts"] == CONTRACTS, "contract-drift", "selftest")
        require(value["selected_hash_sets"] == SELECTED_HASH_SETS, "selected-drift", "selftest")
        require(
            value["authority_manifest_sha256"] == G60B_AUTHORITY_SHA256,
            "authority-drift",
            "selftest",
        )
        require(value["brkem_coverage"] == BRKEM_COVERAGE, "fabricated-brkem", "selftest")
        require(
            value["classification_changes"]["full"]
            == {"count": 31000, "forms": ACTIVATION_COUNTS_FULL},
            "partial-activation",
            "selftest",
        )
        require(
            {item["form"] for item in value["gap_kind_changes"]} == set(ACTIVATIONS),
            "unauthorized-gap-kind",
            "selftest",
        )

    mutations = {
        "wrong-predecessor": lambda value: value.update(
            approved_predecessor_sha="0" * 40
        ),
        "dataset-drift": lambda value: value.update(dataset_id="wrong"),
        "contract-drift": lambda value: value.update(comparison_contracts={}),
        "selected-drift": lambda value: value.update(selected_hash_sets={}),
        "authority-drift": lambda value: value.update(authority_manifest_sha256="0" * 64),
        "fabricated-brkem": lambda value: value["brkem_coverage"].update(selected=1),
        "partial-activation": lambda value: value["classification_changes"]["full"].update(count=30999),
        "brkfem-included": lambda value: value["gap_kind_changes"].append({"form": "0FFE"}),
    }
    validate_body(copy.deepcopy(good))
    rejected = 0
    for code, mutation in mutations.items():
        value = copy.deepcopy(good)
        mutation(value)
        try:
            validate_body(value)
        except M64Error:
            rejected += 1
        else:
            reject("selftest-did-not-reject", code)
    sample = {"rows": [{"case_hash": "1" * 64}], "schema_version": 1}
    require(canonical_bytes(sample) == canonical_bytes(copy.deepcopy(sample)), "json", "nondeterministic")
    require(
        ratchet.deterministic_gzip_bytes(sample)
        == ratchet.deterministic_gzip_bytes(copy.deepcopy(sample)),
        "gzip",
        "nondeterministic",
    )
    print(f"m64-selftest: {rejected} fail-closed mutations rejected")


def parse_args(argv: Iterable[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    subparsers = parser.add_subparsers(dest="command", required=True)
    subparsers.add_parser("selftest")
    static = subparsers.add_parser("verify-static")
    static.add_argument("--root", type=pathlib.Path, default=pathlib.Path.cwd())
    support = subparsers.add_parser("write-support-map")
    support.add_argument("--root", type=pathlib.Path, required=True)
    support.add_argument("--epoch", choices=("g62", "g64"), required=True)
    support.add_argument("--output", type=pathlib.Path, required=True)
    policy = subparsers.add_parser("target-policy")
    policy.add_argument("--root", type=pathlib.Path, required=True)
    policy.add_argument("--dataset-root", type=pathlib.Path, required=True)
    policy.add_argument("--evaluated-sha", required=True)
    policy.add_argument("--output", type=pathlib.Path, required=True)
    audit = subparsers.add_parser("audit")
    audit.add_argument("--root", type=pathlib.Path, required=True)
    audit.add_argument("--dataset-root", type=pathlib.Path, required=True)
    audit.add_argument("--worker", type=pathlib.Path, required=True)
    audit.add_argument("--phase", choices=PHASE_ORDER, required=True)
    audit.add_argument("--epoch", choices=("g62", "g64"), required=True)
    audit.add_argument("--source-sha", required=True)
    audit.add_argument("--output", type=pathlib.Path, required=True)
    return parser.parse_args(list(argv))


def main(argv: Iterable[str] | None = None) -> int:
    arguments = parse_args(sys.argv[1:] if argv is None else argv)
    try:
        if arguments.command == "selftest":
            selftest()
        elif arguments.command == "verify-static":
            verify_static(arguments.root.resolve())
        elif arguments.command == "write-support-map":
            with support_map(arguments.root.resolve(), arguments.epoch) as source:
                arguments.output.parent.mkdir(parents=True, exist_ok=True)
                arguments.output.write_bytes(source.read_bytes())
            print(f"m64-support-map: epoch={arguments.epoch}")
        elif arguments.command == "target-policy":
            value = generate_target_policy(
                arguments.root.resolve(),
                arguments.dataset_root.resolve(),
                arguments.evaluated_sha,
            )
            write_json(arguments.output.resolve(), value)
            print(
                "m64-target-policy: "
                f"id={value['target_policy_id']} "
                f"full_applicable={value['applicable_hash_sets']['full']['after_count']}"
            )
        elif arguments.command == "audit":
            value = run_phase_audit(
                arguments.root.resolve(),
                arguments.dataset_root.resolve(),
                arguments.worker.resolve(),
                arguments.phase,
                arguments.epoch,
            )
            write_phase_audit(arguments.output.resolve(), value, arguments.source_sha)
            print(
                f"m64-audit: phase={arguments.phase} epoch={arguments.epoch} "
                f"forms={len(value['forms'])}"
            )
    except (
        M64Error,
        m62.M62Error,
        m59.EvidenceError,
        ssts.CorpusError,
        OSError,
        ValueError,
        KeyError,
        TypeError,
    ) as error:
        print(f"m64-error: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
