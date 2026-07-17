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
# OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
# OF THE POSSIBILITY OF SUCH DAMAGE.
"""Verify and identify the pinned SingleStepTests V20 native corpus."""

from __future__ import annotations

import argparse
import csv
import gzip
import hashlib
import json
import pathlib
import re
import subprocess
import sys
import tempfile
from collections import Counter
from typing import Any, Iterable


UPSTREAM_URL = "https://github.com/SingleStepTests/v20"
UPSTREAM_COMMIT = "9efbd02b8ec1a3aad347c2b59672ad25f3bcdb21"
SUITE_PATH = "v1_native"
IDENTITY_POLICY = "vaeg-upd9002-ssts-v20-v1"
EXPECTED_METADATA_SHA256 = (
    "71c12e705960941a73981891852674649c3332539579634ea34d1dae40c1795a"
)
EXPECTED_LICENSE_SHA256 = (
    "cb3882ef501e91281c1948f71ed62bfca6562345a98f5fc1b69824f62b6e1557"
)
EXPECTED_README_VERSION = "1.0.3"
EXPECTED_METADATA_VERSION = "1.0.2"

ROOT_KEYS = {
    "author",
    "cpu",
    "cpu_detail",
    "date",
    "generator",
    "github",
    "opcodes",
    "syntax_version",
    "version",
}
OPCODE_KEYS = {"arch", "flags", "flags-mask", "reg", "status"}
REGISTER_KEYS = {"arch", "flags", "flags-mask", "percentage", "status"}
PRIMARY_STATUSES = {"normal", "prefix", "fpu", "extension", "undocumented"}
REGISTER_STATUSES = {"normal", "undefined", "alias", "undocumented"}
RECORD_KEYS = {"name", "bytes", "initial", "final", "cycles", "hash", "idx"}
STATE_KEYS = {"regs", "ram", "queue"}
REGISTER_KEYS_IN_RECORD = {
    "ax",
    "bx",
    "cx",
    "dx",
    "sp",
    "bp",
    "si",
    "di",
    "cs",
    "ss",
    "ds",
    "es",
    "ip",
    "flags",
}
SEGMENT_PREFIXES = {0x26, 0x2E, 0x36, 0x3E}
REPEAT_PREFIXES = {
    0x64: "repnc",
    0x65: "repc",
    0xF2: "repne",
    0xF3: "repe",
}
IGNORED_PREFIXES = {0xF0, 0xF1}
GROUP_FORMS = {
    "80",
    "81",
    "82",
    "83",
    "8F",
    "C0",
    "C1",
    "C6",
    "C7",
    "D0",
    "D1",
    "D2",
    "D3",
    "F6",
    "F7",
    "FE",
    "FF",
}
HEX40 = re.compile(r"^[0-9a-f]{40}$")


class CorpusError(RuntimeError):
    """An identity, schema, or corpus verification failure."""


def canonical_bytes(value: Any) -> bytes:
    """Return the M43 canonical JSON representation."""

    return json.dumps(
        value, ensure_ascii=True, separators=(",", ":"), sort_keys=True
    ).encode("utf-8")


def sha256_bytes(value: bytes) -> str:
    return hashlib.sha256(value).hexdigest()


def sha256_file(path: pathlib.Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        while True:
            block = stream.read(1024 * 1024)
            if not block:
                break
            digest.update(block)
    return digest.hexdigest()


def require_keys(value: Any, expected: set[str], where: str) -> dict[str, Any]:
    if not isinstance(value, dict):
        raise CorpusError(f"{where}: expected object, got {type(value).__name__}")
    actual = set(value)
    if actual != expected:
        raise CorpusError(
            f"{where}: schema keys differ: expected={sorted(expected)!r} "
            f"actual={sorted(actual)!r}"
        )
    return value


def validate_metadata(metadata: Any) -> dict[str, Any]:
    root = require_keys(metadata, ROOT_KEYS, "metadata")
    if not isinstance(root["opcodes"], dict):
        raise CorpusError("metadata.opcodes: expected object")

    primary_status = Counter()
    primary_arch = Counter()
    register_status = Counter()
    register_arch = Counter()
    group_count = 0
    register_count = 0
    for opcode, raw_entry in sorted(root["opcodes"].items()):
        if not re.fullmatch(r"[0-9A-F]{2}(?:[0-9A-F]{2})?", opcode):
            raise CorpusError(f"metadata.opcodes.{opcode}: invalid opcode key")
        entry = require_keys(raw_entry, set(raw_entry) & OPCODE_KEYS,
                             f"metadata.opcodes.{opcode}")
        unknown = set(entry) - OPCODE_KEYS
        if unknown:
            raise CorpusError(
                f"metadata.opcodes.{opcode}: unknown keys {sorted(unknown)!r}"
            )
        status = entry.get("status")
        if status is None:
            primary_status["missing"] += 1
        elif status not in PRIMARY_STATUSES:
            raise CorpusError(
                f"metadata.opcodes.{opcode}: unknown status {status!r}"
            )
        else:
            primary_status[status] += 1
        primary_arch[str(entry.get("arch", "missing"))] += 1
        if "flags-mask" in entry and not isinstance(entry["flags-mask"], int):
            raise CorpusError(f"metadata.opcodes.{opcode}.flags-mask: expected int")
        if "flags" in entry and (
            not isinstance(entry["flags"], str)
            or not re.fullmatch(r"[.oszapc]{8}", entry["flags"])
        ):
            raise CorpusError(
                f"metadata.opcodes.{opcode}.flags: expected eight-character mask"
            )
        if "reg" not in entry:
            continue
        if status is not None:
            raise CorpusError(
                f"metadata.opcodes.{opcode}: group parent must have missing status"
            )
        registers = entry["reg"]
        if not isinstance(registers, dict):
            raise CorpusError(f"metadata.opcodes.{opcode}.reg: expected object")
        group_count += 1
        for register, raw_register in sorted(registers.items()):
            if register not in {str(index) for index in range(8)}:
                raise CorpusError(
                    f"metadata.opcodes.{opcode}.reg: invalid key {register!r}"
                )
            reg = require_keys(
                raw_register,
                set(raw_register) & REGISTER_KEYS,
                f"metadata.opcodes.{opcode}.reg.{register}",
            )
            unknown = set(reg) - REGISTER_KEYS
            if unknown:
                raise CorpusError(
                    f"metadata.opcodes.{opcode}.reg.{register}: unknown keys "
                    f"{sorted(unknown)!r}"
                )
            reg_status = reg.get("status", "normal")
            if reg_status not in REGISTER_STATUSES:
                raise CorpusError(
                    f"metadata.opcodes.{opcode}.reg.{register}: unknown status "
                    f"{reg_status!r}"
                )
            register_status[reg_status] += 1
            register_arch[str(reg.get("arch", "missing"))] += 1
            register_count += 1
            if "flags-mask" in reg and not isinstance(reg["flags-mask"], int):
                raise CorpusError(
                    f"metadata.opcodes.{opcode}.reg.{register}.flags-mask: "
                    "expected int"
                )
            if "flags" in reg and (
                not isinstance(reg["flags"], str)
                or not re.fullmatch(r"[.oszapc]{8}", reg["flags"])
            ):
                raise CorpusError(
                    f"metadata.opcodes.{opcode}.reg.{register}.flags: "
                    "expected eight-character mask"
                )

    observed = {
        "opcode_count": len(root["opcodes"]),
        "primary_status_counts": dict(sorted(primary_status.items())),
        "primary_arch_counts": dict(sorted(primary_arch.items())),
        "group_parent_count": group_count,
        "register_entry_count": register_count,
        "register_status_counts": dict(sorted(register_status.items())),
        "register_arch_counts": dict(sorted(register_arch.items())),
    }
    expected = {
        "opcode_count": 282,
        "primary_status_counts": {
            "extension": 1,
            "fpu": 10,
            "missing": 17,
            "normal": 243,
            "prefix": 10,
            "undocumented": 1,
        },
        "primary_arch_counts": {"186": 10, "86": 221, "missing": 19, "v30": 32},
        "group_parent_count": 17,
        "register_entry_count": 136,
        "register_status_counts": {
            "alias": 11,
            "normal": 92,
            "undefined": 27,
            "undocumented": 6,
        },
        "register_arch_counts": {"186": 19, "86": 116, "missing": 1},
    }
    if observed != expected:
        raise CorpusError(
            "metadata vocabulary/counts differ from the pinned schema: "
            f"expected={expected!r} actual={observed!r}"
        )
    return observed


def validate_record(record: Any, where: str) -> dict[str, Any]:
    value = require_keys(record, RECORD_KEYS, where)
    if not isinstance(value["bytes"], list) or not value["bytes"]:
        raise CorpusError(f"{where}.bytes: expected nonempty array")
    if any(not isinstance(byte, int) or byte < 0 or byte > 0xff
           for byte in value["bytes"]):
        raise CorpusError(f"{where}.bytes: byte outside 0..255")
    if not isinstance(value["idx"], int) or value["idx"] < 0:
        raise CorpusError(f"{where}.idx: expected nonnegative int")
    if not isinstance(value["hash"], str) or not HEX40.fullmatch(value["hash"]):
        raise CorpusError(f"{where}.hash: expected lowercase 40-hex string")
    if not isinstance(value["name"], str):
        raise CorpusError(f"{where}.name: expected string")
    if not isinstance(value["cycles"], list):
        raise CorpusError(f"{where}.cycles: expected array")
    for state_name in ("initial", "final"):
        state = require_keys(value[state_name], STATE_KEYS, f"{where}.{state_name}")
        regs = state["regs"]
        if not isinstance(regs, dict):
            raise CorpusError(f"{where}.{state_name}.regs: expected object")
        if state_name == "initial" and set(regs) != REGISTER_KEYS_IN_RECORD:
            raise CorpusError(
                f"{where}.{state_name}.regs: expected all architectural registers"
            )
        if state_name == "final" and not set(regs).issubset(
            REGISTER_KEYS_IN_RECORD
        ):
            raise CorpusError(
                f"{where}.{state_name}.regs: unknown register"
            )
        if any(not isinstance(reg, int) or reg < 0 or reg > 0xffff
               for reg in regs.values()):
            raise CorpusError(f"{where}.{state_name}.regs: invalid 16-bit value")
        if not isinstance(state["ram"], list):
            raise CorpusError(f"{where}.{state_name}.ram: expected array")
        for item in state["ram"]:
            if (not isinstance(item, list) or len(item) != 2 or
                    not isinstance(item[0], int) or not 0 <= item[0] <= 0xfffff or
                    not isinstance(item[1], int) or not 0 <= item[1] <= 0xff):
                raise CorpusError(f"{where}.{state_name}.ram: invalid address/value")
        if not isinstance(state["queue"], list) or any(
            not isinstance(byte, int) or not 0 <= byte <= 0xff
            for byte in state["queue"]
        ):
            raise CorpusError(f"{where}.{state_name}.queue: invalid queue")
    return value


def corpus_files(dataset_root: pathlib.Path) -> list[pathlib.Path]:
    suite = dataset_root / SUITE_PATH
    paths = sorted(suite.glob("*.json.gz"), key=lambda path: path.name)
    if not paths:
        raise CorpusError(f"no corpus shards found below {suite}")
    return paths


def load_support_map(path: pathlib.Path) -> dict[tuple[str, int, str], dict[str, str]]:
    try:
        with path.open("r", encoding="utf-8", newline="") as stream:
            rows = list(csv.DictReader(stream))
    except OSError as error:
        raise CorpusError(f"cannot read support map {path}: {error}") from error
    expected_fields = {
        "mode", "opcode", "subopcode", "target", "classification", "basis"
    }
    if not rows or set(rows[0]) != expected_fields:
        raise CorpusError(f"{path}: unsupported support-map schema")
    result = {}
    for row in rows:
        if row["classification"] not in {"implemented", "known_target_gap"}:
            raise CorpusError(
                f"{path}: unknown support classification {row['classification']!r}"
            )
        key = (row["mode"], int(row["opcode"], 16), row["subopcode"])
        if key in result:
            raise CorpusError(f"{path}: duplicate support-map key {key!r}")
        result[key] = row
    if len(result) != 1296:
        raise CorpusError(f"{path}: expected 1296 support rows, got {len(result)}")
    return result


def metadata_for_form(
    metadata: dict[str, Any], form: str
) -> tuple[dict[str, Any], dict[str, Any], str, str, int]:
    opcode_key = form.split(".", 1)[0]
    opcodes = metadata["opcodes"]
    if opcode_key not in opcodes:
        raise CorpusError(f"{form}: no metadata entry for {opcode_key}")
    parent = opcodes[opcode_key]
    entry = parent
    if "." in form:
        base, register = form.split(".", 1)
        if base not in GROUP_FORMS or register not in {str(i) for i in range(8)}:
            raise CorpusError(f"{form}: unsupported structural form")
        try:
            entry = parent["reg"][register]
        except (KeyError, TypeError) as error:
            raise CorpusError(f"{form}: no metadata register entry") from error
    status = entry.get("status", "normal")
    if status not in REGISTER_STATUSES | PRIMARY_STATUSES:
        raise CorpusError(f"{form}: unknown resolved metadata status {status!r}")
    arch = entry.get("arch", parent.get("arch"))
    if arch is None:
        arch = "missing"
    if arch not in {"86", "186", "v30", "missing"}:
        raise CorpusError(f"{form}: unknown resolved architecture {arch!r}")
    flags_mask = entry.get("flags-mask", parent.get("flags-mask", 0xffff))
    if not isinstance(flags_mask, int) or not 0 <= flags_mask <= 0xffff:
        raise CorpusError(f"{form}: invalid resolved flags mask")
    return parent, entry, status, arch, flags_mask


def resolve_dispatch(
    form: str,
    instruction: list[int],
    support: dict[tuple[str, int, str], dict[str, str]],
) -> dict[str, Any]:
    position = 0
    repeat = "none"
    segment_prefixes = []
    lock_prefixes = []
    while position < len(instruction):
        byte = instruction[position]
        if byte in SEGMENT_PREFIXES:
            segment_prefixes.append(byte)
        elif byte in REPEAT_PREFIXES:
            repeat = REPEAT_PREFIXES[byte]
        elif byte in IGNORED_PREFIXES:
            lock_prefixes.append(byte)
        else:
            break
        position += 1
    if position >= len(instruction):
        raise CorpusError(f"{form}: instruction contains prefixes only")
    opcode = instruction[position]
    position += 1
    second_byte = None
    modrm_reg = None
    if opcode == 0x0f:
        if position >= len(instruction):
            raise CorpusError(f"{form}: 0f instruction lacks a second byte")
        second_byte = instruction[position]
        position += 1
    if form.split(".", 1)[0] in GROUP_FORMS:
        if position >= len(instruction):
            raise CorpusError(f"{form}: group instruction lacks ModR/M")
        modrm_reg = (instruction[position] >> 3) & 7

    expected_key = form.split(".", 1)[0]
    if expected_key.startswith("0F"):
        if opcode != 0x0f or second_byte != int(expected_key[2:], 16):
            raise CorpusError(
                f"{form}: bytes do not match extended opcode identity"
            )
    elif opcode != int(expected_key, 16):
        raise CorpusError(
            f"{form}: final opcode {opcode:02x} does not match shard identity"
        )
    if "." in form and modrm_reg != int(form.split(".", 1)[1]):
        raise CorpusError(f"{form}: ModR/M reg does not match shard identity")

    if repeat == "repnc":
        support_key = ("v30op", 0x64, "-")
    else:
        mode = {
            "none": "v30op",
            "repc": "v30op_repc",
            "repe": "v30op_repe",
            "repne": "v30op_repne",
        }[repeat]
        if mode == "v30op" and opcode == 0x0f:
            support_key = ("v30op_0f", 0x0f, f"0x{second_byte:02x}")
        elif mode == "v30op" and opcode in {0xf6, 0xf7}:
            support_key = (
                f"v30ope0x{opcode:02x}_table", opcode, f"/{modrm_reg}"
            )
        else:
            support_key = (mode, opcode, "-")
    if support_key not in support:
        raise CorpusError(f"{form}: no M42 support-map row for {support_key!r}")
    row = support[support_key]
    return {
        "repeat_prefix": repeat,
        "segment_prefixes": [f"0x{byte:02x}" for byte in segment_prefixes],
        "lock_prefixes": [f"0x{byte:02x}" for byte in lock_prefixes],
        "opcode": f"0x{opcode:02x}",
        "second_byte": None if second_byte is None else f"0x{second_byte:02x}",
        "modrm_reg": modrm_reg,
        "support_mode": support_key[0],
        "support_subopcode": support_key[2],
        "support_target": row["target"],
        "support_classification": row["classification"],
    }


def classify_record(
    form: str,
    record: dict[str, Any],
    metadata: dict[str, Any],
    support: dict[tuple[str, int, str], dict[str, str]],
) -> dict[str, Any]:
    _, _, status, arch, flags_mask = metadata_for_form(metadata, form)
    if record["initial"]["queue"]:
        return {
            "classification": "unsupported_fixture",
            "reason": "prefetch queue injection is not available in the M42 harness",
            "metadata_status": status,
            "metadata_arch": arch,
            "flags_mask": flags_mask,
            "dispatch": None,
        }
    dispatch = resolve_dispatch(form, record["bytes"], support)
    if status in {"fpu", "undefined"}:
        classification = "upstream_nonblocking"
        reason = f"upstream metadata status {status} is outside blocking semantics"
    elif dispatch["support_classification"] == "known_target_gap":
        classification = "known_target_gap"
        reason = "the exact M42 final dispatch form is absent from the current target"
    else:
        classification = "applicable"
        reason = "implemented target form with a defined V20 final-state oracle"
    return {
        "classification": classification,
        "reason": reason,
        "metadata_status": status,
        "metadata_arch": arch,
        "flags_mask": flags_mask,
        "dispatch": dispatch,
    }


def gap_selector(form: str, resolved: dict[str, Any]) -> dict[str, Any]:
    dispatch = resolved["dispatch"]
    return {
        "metadata_form": form,
        "opcode": dispatch["opcode"],
        "second_byte": dispatch["second_byte"],
        "modrm_reg": dispatch["modrm_reg"],
        "repeat_prefix": dispatch["repeat_prefix"],
        "segment_prefix_constraint": "dispatch-neutral-any",
        "lock_prefix_constraint": "dispatch-neutral-any",
        "support_mode": dispatch["support_mode"],
        "support_subopcode": dispatch["support_subopcode"],
        "support_target": dispatch["support_target"],
    }


def profile_records(records: list[dict[str, Any]], profile: str) -> list[dict[str, Any]]:
    empty = [record for record in records if not record["initial"]["queue"]]
    empty.sort(key=lambda record: record["hash"])
    if profile == "ci":
        return empty[:500]
    if profile == "full":
        return empty
    raise CorpusError(f"unknown profile {profile!r}")


def classify_profile(
    dataset_root: pathlib.Path,
    manifest: dict[str, Any],
    support_map_path: pathlib.Path,
    profile: str,
) -> tuple[dict[str, Any], dict[str, Any]]:
    verify_fast(dataset_root, manifest)
    metadata = json.loads(
        (dataset_root / SUITE_PATH / "metadata.json").read_text(encoding="utf-8")
    )
    validate_metadata(metadata)
    support = load_support_map(support_map_path)
    classifications = Counter()
    all_classifications = Counter()
    per_form = []
    known_rules: dict[bytes, dict[str, Any]] = {}
    selection_testsets = []
    selected_total = 0
    for shard_number, path in enumerate(corpus_files(dataset_root), 1):
        relative = path.relative_to(dataset_root).as_posix()
        with gzip.open(path, "rt", encoding="utf-8") as stream:
            records = json.load(stream)
        form = path.name.removesuffix(".json.gz").upper()
        selected = profile_records(records, profile)
        selected_hashes = []
        form_counts = Counter()
        for index, raw_record in enumerate(records):
            record = validate_record(raw_record, f"{relative}[{index}]")
            resolved = classify_record(form, record, metadata, support)
            all_classifications[resolved["classification"]] += 1
        for record in selected:
            resolved = classify_record(form, record, metadata, support)
            classification = resolved["classification"]
            if classification == "unsupported_fixture":
                raise CorpusError(
                    f"{form}:{record['hash']}: empty-queue profile became unsupported"
                )
            classifications[classification] += 1
            form_counts[classification] += 1
            selected_hashes.append(record["hash"])
            if classification == "known_target_gap":
                selector = gap_selector(form, resolved)
                selector_bytes = canonical_bytes(selector)
                rule = known_rules.setdefault(
                    selector_bytes,
                    {
                        "selector": selector,
                        "reason": resolved["reason"],
                        "evidence": {
                            "support_map": support_map_path.as_posix(),
                            "support_classification": "known_target_gap",
                            "metadata_status": resolved["metadata_status"],
                            "metadata_arch": resolved["metadata_arch"],
                        },
                        "resolved_test_hashes": [],
                    },
                )
                rule["resolved_test_hashes"].append(record["hash"])
        selected_hashes.sort()
        selection_testsets.append(
            {
                "form": form,
                "selected_count": len(selected_hashes),
                "selected_test_hashes_sha256": sha256_bytes(
                    canonical_bytes(selected_hashes)
                ),
            }
        )
        per_form.append(
            {
                "form": form,
                "selected_count": len(selected),
                "classification_counts": dict(sorted(form_counts.items())),
            }
        )
        selected_total += len(selected)
        print(
            f"ssts-classify: {shard_number:03d}/{len(corpus_files(dataset_root)):03d} "
            f"{form} selected={len(selected)}",
            file=sys.stderr,
            flush=True,
        )
    selection_digest = sha256_bytes(canonical_bytes(selection_testsets))
    result = {
        "schema": "vaeg-upd9002-ssts-classification-v1",
        "dataset_id": manifest["dataset_id"],
        "dataset_digest": manifest["dataset_digest"],
        "profile": profile,
        "profile_definition": (
            "empty initial queue; stable upstream-hash order; maximum 500 per form"
            if profile == "ci"
            else "all records with an empty initial prefetch queue"
        ),
        "selection_digest": selection_digest,
        "selected_records": selected_total,
        "classification_counts": dict(sorted(classifications.items())),
        "all_corpus_classification_counts": dict(sorted(all_classifications.items())),
        "per_form": per_form,
    }
    rules = []
    for selector_bytes, rule in sorted(known_rules.items()):
        del selector_bytes
        hashes = sorted(rule["resolved_test_hashes"])
        rule["resolved_test_hashes"] = hashes
        rule["resolved_count"] = len(hashes)
        rule["resolved_test_hashes_sha256"] = sha256_bytes(canonical_bytes(hashes))
        rules.append(rule)
    known_gaps = {
        "schema": "vaeg-upd9002-ssts-known-gaps-v1",
        "dataset_id": manifest["dataset_id"],
        "profile": profile,
        "rule_count": len(rules),
        "resolved_record_count": sum(rule["resolved_count"] for rule in rules),
        "rules": rules,
    }
    return result, known_gaps


def create_manifest(dataset_root: pathlib.Path) -> dict[str, Any]:
    metadata_path = dataset_root / SUITE_PATH / "metadata.json"
    license_path = dataset_root / "LICENSE"
    readme_path = dataset_root / "README.md"
    for path in (metadata_path, license_path, readme_path):
        if not path.is_file():
            raise CorpusError(f"required upstream file is missing: {path}")
    metadata_digest = sha256_file(metadata_path)
    license_digest = sha256_file(license_path)
    if metadata_digest != EXPECTED_METADATA_SHA256:
        raise CorpusError(
            f"metadata digest mismatch: expected={EXPECTED_METADATA_SHA256} "
            f"actual={metadata_digest}"
        )
    if license_digest != EXPECTED_LICENSE_SHA256:
        raise CorpusError(
            f"license digest mismatch: expected={EXPECTED_LICENSE_SHA256} "
            f"actual={license_digest}"
        )
    metadata = json.loads(metadata_path.read_text(encoding="utf-8"))
    schema = validate_metadata(metadata)
    if metadata.get("version") != EXPECTED_METADATA_VERSION:
        raise CorpusError(
            f"metadata version changed: expected={EXPECTED_METADATA_VERSION!r} "
            f"actual={metadata.get('version')!r}"
        )
    readme_match = re.search(
        r"^### Current Version: ([^\s]+)$",
        readme_path.read_text(encoding="utf-8"),
        re.MULTILINE,
    )
    if readme_match is None or readme_match.group(1) != EXPECTED_README_VERSION:
        raise CorpusError(
            "README version changed: "
            f"expected={EXPECTED_README_VERSION!r} "
            f"actual={readme_match.group(1) if readme_match else None!r}"
        )

    files = []
    testsets = []
    total_records = 0
    empty_queue_records = 0
    prefetched_records = 0
    for shard_number, path in enumerate(corpus_files(dataset_root), 1):
        relative = path.relative_to(dataset_root).as_posix()
        compressed_digest = sha256_file(path)
        try:
            with gzip.open(path, "rt", encoding="utf-8") as stream:
                records = json.load(stream)
        except (OSError, UnicodeError, json.JSONDecodeError) as error:
            raise CorpusError(f"{relative}: cannot decode shard: {error}") from error
        if not isinstance(records, list) or not records:
            raise CorpusError(f"{relative}: expected nonempty record array")
        record_digests = []
        upstream_hashes = set()
        shard_empty = 0
        shard_prefetched = 0
        for index, raw_record in enumerate(records):
            record = validate_record(raw_record, f"{relative}[{index}]")
            if record["hash"] in upstream_hashes:
                raise CorpusError(f"{relative}: duplicate upstream hash {record['hash']}")
            upstream_hashes.add(record["hash"])
            record_digests.append(sha256_bytes(canonical_bytes(record)))
            if record["initial"]["queue"]:
                shard_prefetched += 1
            else:
                shard_empty += 1
        form = path.name.removesuffix(".json.gz").upper()
        testset_content = {
            "form": form,
            "record_digests": sorted(record_digests),
        }
        testset_digest = sha256_bytes(canonical_bytes(testset_content))
        files.append(
            {
                "path": relative,
                "size": path.stat().st_size,
                "sha256": compressed_digest,
            }
        )
        testsets.append(
            {
                "form": form,
                "path": relative,
                "record_count": len(records),
                "empty_queue_count": shard_empty,
                "prefetched_count": shard_prefetched,
                "opcode_testset_digest": testset_digest,
            }
        )
        total_records += len(records)
        empty_queue_records += shard_empty
        prefetched_records += shard_prefetched
        print(
            f"ssts-manifest: {shard_number:03d}/{len(corpus_files(dataset_root)):03d} "
            f"{form} records={len(records)}",
            file=sys.stderr,
            flush=True,
        )

    corpus_digest = sha256_bytes(canonical_bytes(files))
    dataset_content = {
        "identity_policy": IDENTITY_POLICY,
        "metadata_sha256": metadata_digest,
        "corpus_sha256": corpus_digest,
        "opcode_testsets": [
            {
                "form": item["form"],
                "record_count": item["record_count"],
                "opcode_testset_digest": item["opcode_testset_digest"],
            }
            for item in sorted(testsets, key=lambda item: item["form"])
        ],
    }
    dataset_digest = sha256_bytes(canonical_bytes(dataset_content))
    dataset_id = f"ssts-v20-{UPSTREAM_COMMIT}-{dataset_digest}"
    return {
        "schema": "vaeg-upd9002-ssts-manifest-v1",
        "upstream": {
            "repository": UPSTREAM_URL,
            "commit": UPSTREAM_COMMIT,
            "suite": SUITE_PATH,
            "license": "MIT",
            "license_sha256": license_digest,
            "readme_version_informational": EXPECTED_README_VERSION,
            "metadata_version_informational": EXPECTED_METADATA_VERSION,
        },
        "identity_policy": IDENTITY_POLICY,
        "metadata_sha256": metadata_digest,
        "metadata_schema": schema,
        "corpus_sha256": corpus_digest,
        "dataset_digest": dataset_digest,
        "dataset_id": dataset_id,
        "counts": {
            "shards": len(files),
            "records": total_records,
            "empty_queue_records": empty_queue_records,
            "prefetched_records": prefetched_records,
        },
        "files": files,
        "opcode_testsets": sorted(testsets, key=lambda item: item["form"]),
    }


def load_manifest(path: pathlib.Path) -> dict[str, Any]:
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, UnicodeError, json.JSONDecodeError) as error:
        raise CorpusError(f"cannot read manifest {path}: {error}") from error
    if not isinstance(value, dict) or value.get("schema") != (
        "vaeg-upd9002-ssts-manifest-v1"
    ):
        raise CorpusError(f"{path}: unsupported manifest schema")
    return value


def verify_fast(dataset_root: pathlib.Path, expected: dict[str, Any]) -> None:
    upstream = expected.get("upstream")
    if not isinstance(upstream, dict) or (
        upstream.get("repository") != UPSTREAM_URL
        or upstream.get("commit") != UPSTREAM_COMMIT
        or upstream.get("suite") != SUITE_PATH
    ):
        raise CorpusError("manifest upstream identity is not the approved pin")
    if expected.get("identity_policy") != IDENTITY_POLICY:
        raise CorpusError("manifest identity policy differs from the approved policy")
    expected_files = expected.get("files")
    if not isinstance(expected_files, list):
        raise CorpusError("manifest.files: expected array")
    actual_paths = [path.relative_to(dataset_root).as_posix()
                    for path in corpus_files(dataset_root)]
    listed_paths = [item.get("path") for item in expected_files]
    if actual_paths != listed_paths:
        raise CorpusError("corpus shard list differs from manifest")
    for item in expected_files:
        path = dataset_root / item["path"]
        actual_size = path.stat().st_size
        if actual_size != item["size"]:
            raise CorpusError(
                f"{item['path']}: size mismatch expected={item['size']} "
                f"actual={actual_size}"
            )
        actual_digest = sha256_file(path)
        if actual_digest != item["sha256"]:
            raise CorpusError(
                f"{item['path']}: digest mismatch expected={item['sha256']} "
                f"actual={actual_digest}"
            )
    metadata_digest = sha256_file(dataset_root / SUITE_PATH / "metadata.json")
    if metadata_digest != expected.get("metadata_sha256"):
        raise CorpusError("metadata digest differs from manifest")
    license_digest = sha256_file(dataset_root / "LICENSE")
    if license_digest != expected["upstream"].get("license_sha256"):
        raise CorpusError("license digest differs from manifest")


def run_git(arguments: list[str], cwd: pathlib.Path | None = None) -> str:
    try:
        result = subprocess.run(
            ["git", *arguments],
            cwd=cwd,
            check=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )
    except (OSError, subprocess.CalledProcessError) as error:
        detail = getattr(error, "stderr", "") or str(error)
        raise CorpusError(f"git {' '.join(arguments)} failed: {detail.strip()}") from error
    return result.stdout.strip()


def acquire(cache_root: pathlib.Path) -> pathlib.Path:
    cache_root.mkdir(parents=True, exist_ok=True)
    destination = cache_root / f"singlesteptests-v20-{UPSTREAM_COMMIT}"
    if not destination.exists():
        run_git(["clone", "--filter=blob:none", "--no-checkout", UPSTREAM_URL,
                 str(destination)])
    if not (destination / ".git").exists():
        raise CorpusError(f"cache destination is not a Git checkout: {destination}")
    run_git(["fetch", "--depth", "1", "origin", UPSTREAM_COMMIT], destination)
    run_git(["checkout", "--detach", UPSTREAM_COMMIT], destination)
    actual = run_git(["rev-parse", "HEAD"], destination)
    if actual != UPSTREAM_COMMIT:
        raise CorpusError(
            f"checkout identity mismatch: expected={UPSTREAM_COMMIT} actual={actual}"
        )
    if run_git(["status", "--short"], destination):
        raise CorpusError(f"upstream checkout is not clean: {destination}")
    print(f"ssts-acquire: checkout={destination} commit={actual}")
    return destination


def write_json(path: pathlib.Path, value: Any) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(
        json.dumps(value, ensure_ascii=True, indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
        newline="\n",
    )


def selftest() -> None:
    record = {
        "name": "nop",
        "bytes": [0x90],
        "initial": {
            "regs": {name: 0 for name in REGISTER_KEYS_IN_RECORD},
            "ram": [[0, 0x90]],
            "queue": [],
        },
        "final": {
            "regs": {name: 0 for name in REGISTER_KEYS_IN_RECORD},
            "ram": [[0, 0x90]],
            "queue": [],
        },
        "cycles": [],
        "hash": "0" * 40,
        "idx": 0,
    }
    validate_record(record, "synthetic")
    first = sha256_bytes(canonical_bytes(record))
    second = sha256_bytes(canonical_bytes(dict(reversed(list(record.items())))))
    if first != second:
        raise CorpusError("canonical record digest depends on object key order")
    bad = json.loads(json.dumps(record))
    bad["ignored"] = True
    try:
        validate_record(bad, "synthetic-bad")
    except CorpusError:
        pass
    else:
        raise CorpusError("unknown record field was silently accepted")

    unknown_metadata = {key: "synthetic" for key in ROOT_KEYS}
    unknown_metadata["opcodes"] = {
        "00": {"status": "invalid", "arch": "86"}
    }
    try:
        validate_metadata(unknown_metadata)
    except CorpusError as error:
        if "unknown status" not in str(error):
            raise
    else:
        raise CorpusError("unknown metadata status was silently accepted")

    unclassified_record = json.loads(json.dumps(record))
    unclassified_record["bytes"] = [0x90]
    try:
        classify_record(
            "90",
            unclassified_record,
            {"opcodes": {"90": {"status": "normal", "arch": "86"}}},
            {},
        )
    except CorpusError as error:
        if "no M42 support-map row" not in str(error):
            raise
    else:
        raise CorpusError("unclassified dispatch form was silently accepted")

    with tempfile.TemporaryDirectory(prefix="vaeg-ssts-selftest-") as temporary:
        root = pathlib.Path(temporary)
        suite = root / SUITE_PATH
        suite.mkdir()
        shard = suite / "00.json.gz"
        shard.write_bytes(b"not-a-gzip-stream")
        (suite / "metadata.json").write_text("{}\n", encoding="utf-8")
        (root / "LICENSE").write_text("synthetic\n", encoding="utf-8")
        synthetic_manifest = {
            "upstream": {
                "repository": UPSTREAM_URL,
                "commit": UPSTREAM_COMMIT,
                "suite": SUITE_PATH,
                "license_sha256": sha256_file(root / "LICENSE"),
            },
            "identity_policy": IDENTITY_POLICY,
            "metadata_sha256": sha256_file(suite / "metadata.json"),
            "files": [
                {
                    "path": f"{SUITE_PATH}/00.json.gz",
                    "size": shard.stat().st_size,
                    "sha256": "0" * 64,
                }
            ],
        }
        try:
            verify_fast(root, synthetic_manifest)
        except CorpusError as error:
            if "digest mismatch" not in str(error):
                raise
        else:
            raise CorpusError("corpus digest mismatch was silently accepted")
    print(
        "ssts-selftest: canonical digest, fail-closed schema/status/form, and "
        "digest rejection passed"
    )


def parse_args(argv: Iterable[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    subparsers = parser.add_subparsers(dest="command", required=True)
    acquire_parser = subparsers.add_parser(
        "acquire", help="acquire the exact pinned upstream commit"
    )
    acquire_parser.add_argument("--cache-root", required=True, type=pathlib.Path)
    manifest = subparsers.add_parser("manifest", help="create a full manifest")
    manifest.add_argument("--dataset-root", required=True, type=pathlib.Path)
    manifest.add_argument("--output", required=True, type=pathlib.Path)
    verify = subparsers.add_parser("verify", help="verify pinned compressed bytes")
    verify.add_argument("--dataset-root", required=True, type=pathlib.Path)
    verify.add_argument("--manifest", required=True, type=pathlib.Path)
    classify = subparsers.add_parser(
        "classify", help="classify a deterministic comparison profile"
    )
    classify.add_argument("--dataset-root", required=True, type=pathlib.Path)
    classify.add_argument("--manifest", required=True, type=pathlib.Path)
    classify.add_argument("--support-map", required=True, type=pathlib.Path)
    classify.add_argument("--profile", required=True, choices=("ci", "full"))
    classify.add_argument("--output", required=True, type=pathlib.Path)
    classify.add_argument("--known-gaps", required=True, type=pathlib.Path)
    subparsers.add_parser("selftest", help="run synthetic parser tests")
    return parser.parse_args(argv)


def main(argv: Iterable[str] | None = None) -> int:
    arguments = parse_args(sys.argv[1:] if argv is None else argv)
    try:
        if arguments.command == "manifest":
            value = create_manifest(arguments.dataset_root.resolve())
            write_json(arguments.output, value)
            print(
                "ssts-manifest: "
                f"dataset_id={value['dataset_id']} "
                f"records={value['counts']['records']}"
            )
        elif arguments.command == "acquire":
            acquire(arguments.cache_root.resolve())
        elif arguments.command == "verify":
            value = load_manifest(arguments.manifest)
            verify_fast(arguments.dataset_root.resolve(), value)
            print(
                "ssts-verify: verified "
                f"dataset_id={value['dataset_id']} files={len(value['files'])}"
            )
        elif arguments.command == "classify":
            manifest = load_manifest(arguments.manifest)
            result, known_gaps = classify_profile(
                arguments.dataset_root.resolve(),
                manifest,
                arguments.support_map.resolve(),
                arguments.profile,
            )
            write_json(arguments.output, result)
            write_json(arguments.known_gaps, known_gaps)
            print(
                "ssts-classify: "
                f"dataset_id={result['dataset_id']} profile={result['profile']} "
                f"selected={result['selected_records']} "
                f"categories={canonical_bytes(result['classification_counts']).decode()}"
            )
        elif arguments.command == "selftest":
            selftest()
        else:
            raise AssertionError(arguments.command)
    except (CorpusError, OSError) as error:
        print(f"ssts-error: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
