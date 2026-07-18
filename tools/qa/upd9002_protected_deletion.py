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
"""Verify the exact G49-approved M50 protected-mode deletions."""

from __future__ import annotations

import argparse
import csv
import hashlib
import importlib.util
import io
import pathlib
import re
import sys
from collections import Counter
from typing import Dict, Iterable, List, Mapping, Sequence, Set, Tuple


sys.dont_write_bytecode = True


class DeletionError(RuntimeError):
    """A fail-closed M50 deletion verification error."""


APPROVED_GROUP_COUNTS = {
    "M50-PM-ARPL": 4,
    "M50-PM-MOV-SEG-EA": 4,
    "M50-PM-CTS-SYSTEM": 44,
}

M49_INVENTORY = "tools/qa/golden/upd9002_286_reachability_m49.csv"
M49_INVENTORY_SHA256 = (
    "f3843cd57b57af8f5baa4a180a7a30c88d628d0b12865d6a4a451a306794c15b"
)
M50_MANIFEST = "tools/qa/golden/upd9002_286_deletion_manifest_m50.csv"
M50_PROVENANCE = "tools/qa/golden/upd9002_dispatch_provenance_m50.csv"

IMMUTABLE_FILES = {
    "cpu/upd9002/cpucore.h":
        "78456e2de3a5903289f23382fa83863d6f092b9d86dbee055b576a2e24775196",
    "cpu/upd9002/upd9002_state.c":
        "09b1ffe22bab6d2a411f15e89ab72f82ee42195606bcefeb6740fd5e4f677505",
    "cpu/upd9002/upd9002_state.h":
        "a3ef33e7a9171c4cd14dda9759d929fe943d6e85ba5e2a7f04d6631ab6db4d80",
    "cpu/upd9002/upd9002_core.c":
        "658408730cd4fe7cc102a21b1262788abca877ea9e10d1d40929f96ab0bc9892",
    "cpu/upd9002/upd9002_dispatch.c":
        "e7b5d743d0e99d46f0ef114383dda25d130b35b436244fbc6c275566672d2502",
    "cpu/upd9002/i286c_ea.c":
        "f63a82a595028b05d7bbbc485e53186618fd844649f31f8b16822fbeb88d63d2",
    "cpu/upd9002/i286c.mcr":
        "4a68fe820cfcfb5c2f68e2d8370bc5aeec85bc96264a857443f3f8a62e245e88",
    "tests/upd9002/rep0f_diagnostic_stop.c":
        "59baf7627310f73c6862b6f7e26d3704f2ed972c1f9e352dd1c04c571eee91e6",
    "tests/upd9002/state_fixtures_m42.txt":
        "c8ed4bcf1a7df2a88964d71d85b846a6d7881f60a9233d8c9b787d3d5076f4fb",
    "tools/qa/golden/upd9002_final_dispatch_graph_m48.csv":
        "fe9df28ad3d51cc55235afc3979ada890e86158b294762c15fa33c20d8a800a6",
    "tools/qa/golden/upd9002_dispatch_provenance_m48.csv":
        "128698af06c4e4e98183e4ec0151b7025f427c4f812f95d9012f41417461027a",
    "tools/qa/golden/upd9002_support_map_m48.csv":
        "21dd037c3eb11e1674805ad456ef03663f17804affbd7382c8db77291ab25279",
    "tools/qa/golden/upd9002_rep0f_transition_manifest_m48.json":
        "4f3fefe8cbfb20a03364a80a0b917e475d3d545cab8eda6bee8a22c66e2147ee",
}

M51_CANONICAL_REPLACEMENTS = {
    "cpu/upd9002/upd9002_core.c": (
        (b'#include\t"upd9002_dispatch.h"', b'#include\t"v30patch.h"', 1),
    ),
    "cpu/upd9002/upd9002_dispatch.c": (
        (b'#include\t"upd9002_dispatch.h"', b'#include\t"v30patch.h"', 1),
    ),
    "tests/upd9002/rep0f_diagnostic_stop.c": (
        (b'#include "upd9002_dispatch.h"', b'#include "v30patch.h"', 1),
    ),
}

DELETED_IDENTIFIERS = (
    "_arpl", "_mov_seg_ea", "i286c_cts", "cts0_table", "cts1_table",
    "_sldt", "_str", "_lldt", "_ltr", "_verr", "_verw", "_sgdt",
    "_sidt", "_lgdt", "_lidt", "_smsw", "_lmsw", "_loadall286",
    "I286_0F", "I286OP_0F", "I286_IDTR", "I286_LDTR", "I286_TR",
    "I286_TRC",
)

PLACEHOLDERS = (
    ("v30op", "i286op", 0x0F, "i286c_cts", "v30_ope0x0f"),
    ("v30op_repe", "i286op_repe", 0x0F, "i286c_cts",
     "v30_repe_0f_diagnostic_stop"),
    ("v30op_repne", "i286op_repne", 0x0F, "i286c_cts",
     "v30_repne_0f_diagnostic_stop"),
    ("v30op", "i286op", 0x63, "_arpl", "v30_reserved"),
    ("v30op_repe", "i286op_repe", 0x63, "_arpl", "v30_reserved"),
    ("v30op_repne", "i286op_repne", 0x63, "_arpl", "v30_reserved"),
    ("v30op", "i286op", 0x8E, "_mov_seg_ea", "v30mov_seg_ea"),
    ("v30op_repe", "i286op_repe", 0x8E, "_mov_seg_ea",
     "v30mov_seg_ea"),
    ("v30op_repne", "i286op_repne", 0x8E, "_mov_seg_ea",
     "v30mov_seg_ea"),
)

MANIFEST_COLUMNS = (
    "candidate_id", "symbol_or_field", "kind", "defining_file",
    "approved_group", "m50_action", "retained_replacement", "evidence",
)

Row = Tuple[str, ...]


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def read_bytes(root: pathlib.Path, relative: str) -> bytes:
    try:
        return (root / relative).read_bytes()
    except OSError as error:
        raise DeletionError("cannot read {}: {}".format(relative, error)) from error


def verify_immutable_files(root: pathlib.Path) -> None:
    for relative, expected in IMMUTABLE_FILES.items():
        data = read_bytes(root, relative)
        for current, accepted, expected_count in M51_CANONICAL_REPLACEMENTS.get(
                relative, ()):
            actual_count = data.count(current)
            if actual_count != expected_count:
                raise DeletionError(
                    "M51 rename count changed: {} token={!r} expected={} "
                    "actual={}".format(relative, current, expected_count,
                                       actual_count))
            data = data.replace(current, accepted)
        actual = sha256(data)
        if actual != expected:
            raise DeletionError(
                "retained artifact changed: {} expected={} actual={}".format(
                    relative, expected, actual))


def load_dispatch_module(root: pathlib.Path):
    path = root / "tools/qa/upd9002_dispatch.py"
    spec = importlib.util.spec_from_file_location("upd9002_dispatch_m50", path)
    if spec is None or spec.loader is None:
        raise DeletionError("cannot load upd9002_dispatch.py")
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


def csv_rows(text: str) -> Set[Row]:
    return {tuple(row) for row in csv.reader(io.StringIO(text))}


def load_approved_rows(root: pathlib.Path) -> List[Dict[str, str]]:
    data = read_bytes(root, M49_INVENTORY)
    if sha256(data) != M49_INVENTORY_SHA256:
        raise DeletionError("accepted M49 inventory identity changed")
    rows = list(csv.DictReader(io.StringIO(data.decode("utf-8"))))
    approved = [row for row in rows
                if row["proposed_deletion_group"] in APPROVED_GROUP_COUNTS]
    counts = Counter(row["proposed_deletion_group"] for row in approved)
    if dict(counts) != APPROVED_GROUP_COUNTS:
        raise DeletionError("approved M49 group closure changed: {}".format(counts))
    unexpected = sorted({row["proposed_deletion_group"] for row in rows
                         if row["proposed_deletion_group"] != "-"} -
                        set(APPROVED_GROUP_COUNTS))
    if unexpected:
        raise DeletionError("unapproved M49 groups appeared: {}".format(unexpected))
    return approved


def build_manifest(rows: Iterable[Mapping[str, str]]) -> List[Dict[str, str]]:
    output = []
    for row in rows:
        placeholder = row["kind"] == "constructor_base_entry"
        output.append({
            "candidate_id": row["candidate_id"],
            "symbol_or_field": row["symbol_or_field"],
            "kind": row["kind"],
            "defining_file": row["defining_file"],
            "approved_group": row["proposed_deletion_group"],
            "m50_action": ("replaced_with_reserved_placeholder"
                           if placeholder else "deleted"),
            "retained_replacement": "_reserved" if placeholder else "-",
            "evidence": ("final native patch retained; base address removed"
                         if placeholder else
                         "approved dependency-closed member absent from active source"),
        })
    output.sort(key=lambda row: row["candidate_id"])
    return output


def validate_manifest(rows: Sequence[Mapping[str, str]]) -> None:
    identifiers = [row["candidate_id"] for row in rows]
    duplicates = sorted(name for name, count in Counter(identifiers).items()
                        if count != 1)
    if duplicates:
        raise DeletionError("duplicate manifest candidates: {}".format(duplicates))
    counts = Counter(row["approved_group"] for row in rows)
    if dict(counts) != APPROVED_GROUP_COUNTS:
        raise DeletionError("manifest group closure changed: {}".format(counts))
    for row in rows:
        if set(row) != set(MANIFEST_COLUMNS):
            raise DeletionError("manifest columns changed: {}".format(
                row.get("candidate_id")))
        placeholder = row["kind"] == "constructor_base_entry"
        expected_action = ("replaced_with_reserved_placeholder"
                           if placeholder else "deleted")
        expected_replacement = "_reserved" if placeholder else "-"
        if (row["m50_action"] != expected_action or
                row["retained_replacement"] != expected_replacement):
            raise DeletionError("invalid deletion action: {}".format(
                row["candidate_id"]))
        for value in row.values():
            if re.search(r"(?:^|[ ;])/(?:tmp|home|mnt)/|[A-Za-z]:\\", value):
                raise DeletionError("host-dependent manifest value: {}".format(
                    row["candidate_id"]))


def manifest_bytes(rows: Sequence[Mapping[str, str]]) -> bytes:
    output = io.StringIO(newline="")
    writer = csv.DictWriter(output, fieldnames=MANIFEST_COLUMNS,
                            lineterminator="\n")
    writer.writeheader()
    writer.writerows(rows)
    return output.getvalue().encode("utf-8")


def active_source_paths(root: pathlib.Path) -> List[pathlib.Path]:
    paths = [root / "CMakeLists.txt"]
    for base in (root / "cpu/upd9002", root / "tests/upd9002"):
        for path in sorted(base.iterdir()):
            if path.is_file() and path.suffix in {".c", ".h", ".mcr"}:
                paths.append(path)
    return paths


def verify_source_absence(root: pathlib.Path) -> None:
    if (root / "cpu/upd9002/i286c_0f.c").exists():
        raise DeletionError("approved CTS translation unit still exists")
    findings = []
    for path in active_source_paths(root):
        text = path.read_text(encoding="utf-8")
        relative = path.relative_to(root).as_posix()
        for symbol in DELETED_IDENTIFIERS:
            pattern = r"(?<![A-Za-z0-9_]){}(?![A-Za-z0-9_])".format(
                re.escape(symbol))
            if re.search(pattern, text):
                findings.append("{}:{}".format(relative, symbol))
    if findings:
        raise DeletionError("deleted active identifiers remain: {}".format(
            ", ".join(findings)))


def verify_dispatch(root: pathlib.Path, module, write: bool) -> Tuple[str, str]:
    sources = module.load_sources(root)
    arrays = module.parse_arrays(sources)
    roots, _provenance_rows = module.construct_roots(
        arrays, sources["upd9002_dispatch.c"])
    for root_name, base_name, slot, _old_target, final_target in PLACEHOLDERS:
        if arrays[base_name][slot] != "_reserved":
            raise DeletionError("base placeholder changed: {}[{:#04x}]".format(
                base_name, slot))
        if roots[root_name][slot] != final_target:
            raise DeletionError("final dispatch changed: {}[{:#04x}]".format(
                root_name, slot))

    graph, provenance, harness, support = module.generate(root)
    expected_graph = read_bytes(
        root, "tools/qa/golden/upd9002_final_dispatch_graph_m48.csv")
    expected_support = read_bytes(
        root, "tools/qa/golden/upd9002_support_map_m48.csv")
    if graph.encode("utf-8") != expected_graph:
        raise DeletionError("M48 final dispatch graph changed")
    if support.encode("utf-8") != expected_support:
        raise DeletionError("M48 support map changed")

    old_provenance_text = read_bytes(
        root, "tools/qa/golden/upd9002_dispatch_provenance_m48.csv"
    ).decode("utf-8")
    old_rows = csv_rows(old_provenance_text)
    new_rows = csv_rows(provenance)
    removed = set()
    added = set()
    for root_name, base_name, slot, old_target, final_target in PLACEHOLDERS:
        slot_text = "0x{:02x}".format(slot)
        removed.add((root_name, slot_text, base_name, old_target,
                     "patch", final_target))
        added.add((root_name, slot_text, base_name, "_reserved",
                   "patch", final_target))
    if old_rows - new_rows != removed or new_rows - old_rows != added:
        raise DeletionError("M50 provenance transition exceeded nine placeholders")

    old_harness = csv_rows(read_bytes(
        root, "tests/upd9002/harness_manifest.csv").decode("utf-8"))
    live_harness = csv_rows(harness)
    harness_added = {
        ("patch-v30op_repe-0f", "v30op_repe", "0x0f",
         "v30_repe_0f_diagnostic_stop", "f30fc0000000000000", "1",
         "patched-root"),
        ("patch-v30op_repne-0f", "v30op_repne", "0x0f",
         "v30_repne_0f_diagnostic_stop", "f20fc0000000000000", "1",
         "patched-root"),
    }
    if old_harness - live_harness or live_harness - old_harness != harness_added:
        raise DeletionError("direct harness transition changed")

    compare_or_write(root / M50_PROVENANCE, provenance.encode("utf-8"), write)
    return sha256(provenance.encode("utf-8")), sha256(graph.encode("utf-8"))


def compare_or_write(path: pathlib.Path, data: bytes, write: bool) -> None:
    if write:
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(data)
        return
    try:
        existing = path.read_bytes()
    except OSError as error:
        raise DeletionError("cannot read generated artifact {}: {}".format(
            path, error)) from error
    if existing != data:
        raise DeletionError("generated artifact differs: {}".format(path))


def internal_selftest(rows: Sequence[Mapping[str, str]]) -> None:
    duplicate = [dict(row) for row in rows]
    duplicate.append(dict(rows[0]))
    try:
        validate_manifest(duplicate)
    except DeletionError:
        pass
    else:
        raise DeletionError("duplicate manifest candidate was accepted")

    invalid = [dict(row) for row in rows]
    invalid[0]["retained_replacement"] = "-"
    try:
        validate_manifest(invalid)
    except DeletionError:
        pass
    else:
        raise DeletionError("invalid replacement action was accepted")

    host_dependent = [dict(row) for row in rows]
    host_dependent[0]["evidence"] = "/tmp/address-dependent"
    try:
        validate_manifest(host_dependent)
    except DeletionError:
        pass
    else:
        raise DeletionError("host-dependent manifest value was accepted")


def verify(root: pathlib.Path, write: bool, selftest: bool) -> Tuple[int, str, str]:
    verify_immutable_files(root)
    approved = load_approved_rows(root)
    rows = build_manifest(approved)
    validate_manifest(rows)
    verify_source_absence(root)
    module = load_dispatch_module(root)
    provenance_digest, graph_digest = verify_dispatch(root, module, write)
    data = manifest_bytes(rows)
    compare_or_write(root / M50_MANIFEST, data, write)
    if selftest:
        internal_selftest(rows)
        module.internal_selftest()
    return len(rows), sha256(data), provenance_digest + ":" + graph_digest


def parse_args(argv: Sequence[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=pathlib.Path, default=pathlib.Path("."))
    parser.add_argument("--write", action="store_true")
    parser.add_argument("--selftest", action="store_true")
    return parser.parse_args(argv)


def main(argv: Sequence[str] = None) -> int:
    arguments = parse_args(sys.argv[1:] if argv is None else argv)
    try:
        count, manifest_digest, dispatch_digests = verify(
            arguments.root.resolve(), arguments.write, arguments.selftest)
    except (DeletionError, OSError, UnicodeError, ValueError, KeyError,
            TypeError) as error:
        print("upd9002-protected-deletion: FAIL: {}".format(error),
              file=sys.stderr)
        return 1
    print("upd9002-protected-deletion: PASS candidates={} manifest_sha256={}".format(
        count, manifest_digest))
    print("upd9002-protected-deletion: provenance:graph_sha256={}".format(
        dispatch_digests))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
