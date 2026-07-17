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
        "ssts-selftest: canonical digest, fail-closed schema/status, and "
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
