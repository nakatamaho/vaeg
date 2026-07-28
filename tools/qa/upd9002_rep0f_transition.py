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
"""Verify the exact M42-to-M48 uPD9002 REP+0F transition."""

from __future__ import annotations

import argparse
import csv
import hashlib
import importlib.util
import io
import json
import pathlib
import sys
from typing import Dict, Iterable, List, Sequence, Set, Tuple


sys.dont_write_bytecode = True


class TransitionError(RuntimeError):
    """A fail-closed transition verification error."""


APPROVED_EVIDENCE_SHA = "c683fe502647918d8ded2b4c2243da1b787c9d0a"
DATASET_ID = (
    "ssts-v20-9efbd02b8ec1a3aad347c2b59672ad25f3bcdb21-"
    "1d2e9c0e14101f05379d938245af68f3219c16f638fce019ad2a1946084930a4"
)

HISTORICAL = {
    "tools/qa/golden/upd9002_final_dispatch_graph.csv":
        "e8c8fded6e1f44ef2d74ace93ba1063d64464b217bdd4f9180bf50e86e19df3d",
    "tools/qa/golden/upd9002_dispatch_provenance_m42.csv":
        "605e9af6761e3069f6c5af0ad9647ae574ab98252f7373969586cfee892cd6ba",
    "tools/qa/golden/upd9002_support_map_m42.csv":
        "7b856f95d8c3d0356325483d188b9dde2d03a8e6cbe9b7fd0cacd004f1d09d13",
    "tests/upd9002/harness_manifest.csv":
        "14769ca59bf589921cbcf88dc0c357937f6da36e4fe0b78f9a46dc75b9098f2a",
    "tests/upd9002/state_fixtures_m42.txt":
        "c8ed4bcf1a7df2a88964d71d85b846a6d7881f60a9233d8c9b787d3d5076f4fb",
    "tests/ssts/baseline/upd9002_v20_known_gaps.json":
        "11ec1496a96d661c0656720b13939b1ade3448b693b923f8acf36576cd7048c1",
    "tests/ssts/baseline/v20_native_ci.json":
        "a5db6a6cc82ae794fd2f60306c3d4a70136d6030e17e1aec733523bece864e31",
    "tests/ssts/baseline/v20_native_full.json":
        "dd3247774afe5c5a19228d3a08f01ac6f614e6b67a3a6f454c1abc58f3dbf3d9",
}

M48_PATHS = {
    "graph": "tools/qa/golden/upd9002_final_dispatch_graph_m48.csv",
    "provenance": "tools/qa/golden/upd9002_dispatch_provenance_m48.csv",
    "support": "tools/qa/golden/upd9002_support_map_m48.csv",
    "manifest": "tools/qa/golden/upd9002_rep0f_transition_manifest_m48.json",
}

M48_IDENTITIES = {
    M48_PATHS["graph"]:
        "fe9df28ad3d51cc55235afc3979ada890e86158b294762c15fa33c20d8a800a6",
    M48_PATHS["provenance"]:
        "128698af06c4e4e98183e4ec0151b7025f427c4f812f95d9012f41417461027a",
    M48_PATHS["support"]:
        "21dd037c3eb11e1674805ad456ef03663f17804affbd7382c8db77291ab25279",
    M48_PATHS["manifest"]:
        "4f3fefe8cbfb20a03364a80a0b917e475d3d545cab8eda6bee8a22c66e2147ee",
}

M62_GRAPH_REMOVED = {
    ("v30op", "0x27", "handler", "_daa"),
    ("v30op", "0x2f", "handler", "_das"),
    ("v30op", "0x37", "handler", "_aaa"),
    ("v30op", "0x3f", "handler", "_aas"),
    ("v30ope0x0f_table", "0x28", "handler", "v30_reserved_0x0f"),
}

M62_GRAPH_ADDED = {
    ("v30op", "0x27", "handler", "v30_daa"),
    ("v30op", "0x2f", "handler", "v30_das"),
    ("v30op", "0x37", "handler", "v30_aaa"),
    ("v30op", "0x3f", "handler", "v30_aas"),
    ("v30ope0x0f_table", "0x28", "handler", "v30_rol4_ea8"),
}

M62_SUPPORT_REMOVED = {
    ("v30op", "0x27", "-", "_daa", "implemented", "final-root-target"),
    ("v30op", "0x2f", "-", "_das", "implemented", "final-root-target"),
    ("v30op", "0x37", "-", "_aaa", "implemented", "final-root-target"),
    ("v30op", "0x3f", "-", "_aas", "implemented", "final-root-target"),
    (
        "v30op_0f",
        "0x0f",
        "0x28",
        "v30_reserved_0x0f",
        "known_target_gap",
        "second-byte-resolved",
    ),
}

M62_SUPPORT_ADDED = {
    ("v30op", "0x27", "-", "v30_daa", "implemented", "final-root-target"),
    ("v30op", "0x2f", "-", "v30_das", "implemented", "final-root-target"),
    ("v30op", "0x37", "-", "v30_aaa", "implemented", "final-root-target"),
    ("v30op", "0x3f", "-", "v30_aas", "implemented", "final-root-target"),
    (
        "v30op_0f",
        "0x0f",
        "0x28",
        "v30_rol4_ea8",
        "implemented",
        "second-byte-resolved",
    ),
}

M64_GRAPH_REMOVED = M62_GRAPH_REMOVED | {
    ("v30ope0x0f_table", opcode, "handler", "v30_reserved_0x0f")
    for opcode in ("0x13", "0x15", "0x16", "0x17", "0x1e", "0x1f", "0x26")
}

M64_GRAPH_ADDED = M62_GRAPH_ADDED | {
    ("v30ope0x0f_table", "0x13", "handler", "v30_clr1_ea16_cl"),
    ("v30ope0x0f_table", "0x15", "handler", "v30_set1_ea16_cl"),
    ("v30ope0x0f_table", "0x16", "handler", "v30_not1_ea8_cl"),
    ("v30ope0x0f_table", "0x17", "handler", "v30_not1_ea16_cl"),
    ("v30ope0x0f_table", "0x1e", "handler", "v30_not1_ea8_i3"),
    ("v30ope0x0f_table", "0x1f", "handler", "v30_not1_ea16_i4"),
    ("v30ope0x0f_table", "0x26", "handler", "v30_cmp4s"),
}

M64_SUPPORT_REMOVED = M62_SUPPORT_REMOVED | {
    (
        "v30op_0f",
        "0x0f",
        opcode,
        "v30_reserved_0x0f",
        "known_target_gap",
        "second-byte-resolved",
    )
    for opcode in ("0x13", "0x15", "0x16", "0x17", "0x1e", "0x1f", "0x26")
}

M64_SUPPORT_ADDED = M62_SUPPORT_ADDED | {
    ("v30op_0f", "0x0f", "0x13", "v30_clr1_ea16_cl", "implemented",
     "second-byte-resolved"),
    ("v30op_0f", "0x0f", "0x15", "v30_set1_ea16_cl", "implemented",
     "second-byte-resolved"),
    ("v30op_0f", "0x0f", "0x16", "v30_not1_ea8_cl", "implemented",
     "second-byte-resolved"),
    ("v30op_0f", "0x0f", "0x17", "v30_not1_ea16_cl", "implemented",
     "second-byte-resolved"),
    ("v30op_0f", "0x0f", "0x1e", "v30_not1_ea8_i3", "implemented",
     "second-byte-resolved"),
    ("v30op_0f", "0x0f", "0x1f", "v30_not1_ea16_i4", "implemented",
     "second-byte-resolved"),
    ("v30op_0f", "0x0f", "0x26", "v30_cmp4s", "implemented",
     "second-byte-resolved"),
}

M65A_GRAPH_REMOVED = M64_GRAPH_REMOVED | {
    ("c_ope0xff_table", "0x07", "handler", "_pop_ea16"),
}

M65A_GRAPH_ADDED = M64_GRAPH_ADDED | {
    ("c_ope0xff_table", "0x07", "handler", "_push_ff7_ea16"),
}

M66B_REP_RENAMES = {
    ("i286c_rep_insb", "upd9002_rep_insb"),
    ("i286c_rep_insw", "upd9002_rep_insw"),
    ("i286c_rep_outsb", "upd9002_rep_outsb"),
    ("i286c_rep_movsb", "upd9002_rep_movsb"),
    ("i286c_rep_movsw", "upd9002_rep_movsw"),
    ("i286c_repe_cmpsb", "upd9002_repe_cmpsb"),
    ("i286c_repe_cmpsw", "upd9002_repe_cmpsw"),
    ("i286c_rep_stosb", "upd9002_rep_stosb"),
    ("i286c_rep_stosw", "upd9002_rep_stosw"),
    ("i286c_rep_lodsb", "upd9002_rep_lodsb"),
    ("i286c_rep_lodsw", "upd9002_rep_lodsw"),
    ("i286c_repe_scasb", "upd9002_repe_scasb"),
    ("i286c_repe_scasw", "upd9002_repe_scasw"),
    ("i286c_repne_cmpsb", "upd9002_repne_cmpsb"),
    ("i286c_repne_cmpsw", "upd9002_repne_cmpsw"),
    ("i286c_repne_scasb", "upd9002_repne_scasb"),
    ("i286c_repne_scasw", "upd9002_repne_scasw"),
}

M66B_REP_ROWS = (
    ("v30op_repe", "0x6c", "i286c_rep_insb", "upd9002_rep_insb"),
    ("v30op_repe", "0x6d", "i286c_rep_insw", "upd9002_rep_insw"),
    ("v30op_repe", "0x6e", "i286c_rep_outsb", "upd9002_rep_outsb"),
    ("v30op_repe", "0x6f", "i286c_rep_outsb", "upd9002_rep_outsb"),
    ("v30op_repe", "0xa4", "i286c_rep_movsb", "upd9002_rep_movsb"),
    ("v30op_repe", "0xa5", "i286c_rep_movsw", "upd9002_rep_movsw"),
    ("v30op_repe", "0xa6", "i286c_repe_cmpsb", "upd9002_repe_cmpsb"),
    ("v30op_repe", "0xa7", "i286c_repe_cmpsw", "upd9002_repe_cmpsw"),
    ("v30op_repe", "0xaa", "i286c_rep_stosb", "upd9002_rep_stosb"),
    ("v30op_repe", "0xab", "i286c_rep_stosw", "upd9002_rep_stosw"),
    ("v30op_repe", "0xac", "i286c_rep_lodsb", "upd9002_rep_lodsb"),
    ("v30op_repe", "0xad", "i286c_rep_lodsw", "upd9002_rep_lodsw"),
    ("v30op_repe", "0xae", "i286c_repe_scasb", "upd9002_repe_scasb"),
    ("v30op_repe", "0xaf", "i286c_repe_scasw", "upd9002_repe_scasw"),
    ("v30op_repne", "0x6c", "i286c_rep_insb", "upd9002_rep_insb"),
    ("v30op_repne", "0x6d", "i286c_rep_insw", "upd9002_rep_insw"),
    ("v30op_repne", "0x6e", "i286c_rep_outsb", "upd9002_rep_outsb"),
    ("v30op_repne", "0x6f", "i286c_rep_outsb", "upd9002_rep_outsb"),
    ("v30op_repne", "0xa4", "i286c_rep_movsb", "upd9002_rep_movsb"),
    ("v30op_repne", "0xa5", "i286c_rep_movsw", "upd9002_rep_movsw"),
    ("v30op_repne", "0xa6", "i286c_repne_cmpsb", "upd9002_repne_cmpsb"),
    ("v30op_repne", "0xa7", "i286c_repne_cmpsw", "upd9002_repne_cmpsw"),
    ("v30op_repne", "0xaa", "i286c_rep_stosb", "upd9002_rep_stosb"),
    ("v30op_repne", "0xab", "i286c_rep_stosw", "upd9002_rep_stosw"),
    ("v30op_repne", "0xac", "i286c_rep_lodsb", "upd9002_rep_lodsb"),
    ("v30op_repne", "0xad", "i286c_rep_lodsw", "upd9002_rep_lodsw"),
    ("v30op_repne", "0xae", "i286c_repne_scasb", "upd9002_repne_scasb"),
    ("v30op_repne", "0xaf", "i286c_repne_scasw", "upd9002_repne_scasw"),
)

M66B_GRAPH_REMOVED = M65A_GRAPH_REMOVED | {
    (table, opcode, "handler", old)
    for table, opcode, old, _new in M66B_REP_ROWS
}

M66B_GRAPH_ADDED = M65A_GRAPH_ADDED | {
    (table, opcode, "handler", new)
    for table, opcode, _old, new in M66B_REP_ROWS
}

M66B_SUPPORT_REMOVED = M64_SUPPORT_REMOVED | {
    (table, opcode, "-", old, "implemented", "final-root-target")
    for table, opcode, old, _new in M66B_REP_ROWS
}

M66B_SUPPORT_ADDED = M64_SUPPORT_ADDED | {
    (table, opcode, "-", new, "implemented", "final-root-target")
    for table, opcode, _old, new in M66B_REP_ROWS
}

Row = Tuple[str, ...]


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def verify_historical(root: pathlib.Path) -> None:
    for relative, expected in HISTORICAL.items():
        try:
            actual = sha256((root / relative).read_bytes())
        except OSError as error:
            raise TransitionError("cannot read historical artifact {}: {}".format(
                relative, error)) from error
        if actual != expected:
            raise TransitionError(
                "historical artifact identity changed: {} expected={} actual={}".format(
                    relative, expected, actual))


def load_dispatch_module(root: pathlib.Path):
    path = root / "tools/qa/upd9002_dispatch.py"
    spec = importlib.util.spec_from_file_location("upd9002_dispatch_m48", path)
    if spec is None or spec.loader is None:
        raise TransitionError("cannot load upd9002_dispatch.py")
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


def rows(text: str) -> Set[Row]:
    return {tuple(row) for row in csv.reader(io.StringIO(text))}


def file_rows(root: pathlib.Path, relative: str) -> Set[Row]:
    return rows((root / relative).read_text(encoding="utf-8"))


def sorted_rows(values: Iterable[Row]) -> List[List[str]]:
    return [list(row) for row in sorted(values)]


def require_exact_difference(
        name: str, old: Set[Row], new: Set[Row],
        expected_removed: Set[Row], expected_added: Set[Row]) -> None:
    removed = old - new
    added = new - old
    if removed != expected_removed or added != expected_added:
        raise TransitionError(
            "{} transition drifted: removed={} added={}".format(
                name, sorted(removed), sorted(added)))


def expected_graph_removed() -> Set[Row]:
    rows_out = {
        ("v30op_repe", "0x0f", "handler", "i286c_cts"),
        ("v30op_repne", "0x0f", "handler", "i286c_cts"),
        ("handler:i286c_cts", "-", "secondary-table", "cts0_table"),
        ("handler:i286c_cts", "-", "secondary-table", "cts1_table"),
    }
    for slot, target in enumerate(
            ("_sldt", "_str", "_lldt", "_ltr", "_verr", "_verw",
             "_verr", "_verw")):
        rows_out.add(("cts0_table", "0x{:02x}".format(slot),
                      "handler", target))
    for slot, target in enumerate(
            ("_sgdt", "_sidt", "_lgdt", "_lidt", "_smsw", "_smsw",
             "_lmsw", "_lmsw")):
        rows_out.add(("cts1_table", "0x{:02x}".format(slot),
                      "handler", target))
    return rows_out


GRAPH_ADDED = {
    ("v30op_repe", "0x0f", "handler", "v30_repe_0f_diagnostic_stop"),
    ("v30op_repne", "0x0f", "handler", "v30_repne_0f_diagnostic_stop"),
}

PROVENANCE_REMOVED = {
    ("v30op_repe", "0x0f", "i286op_repe", "i286c_cts", "base", "i286c_cts"),
    ("v30op_repne", "0x0f", "i286op_repne", "i286c_cts", "base", "i286c_cts"),
}

PROVENANCE_ADDED = {
    ("v30op_repe", "0x0f", "i286op_repe", "i286c_cts", "patch",
     "v30_repe_0f_diagnostic_stop"),
    ("v30op_repne", "0x0f", "i286op_repne", "i286c_cts", "patch",
     "v30_repne_0f_diagnostic_stop"),
}

SUPPORT_REMOVED = {
    ("v30op_repe", "0x0f", "-", "i286c_cts", "implemented",
     "final-root-target"),
    ("v30op_repne", "0x0f", "-", "i286c_cts", "implemented",
     "final-root-target"),
}

SUPPORT_ADDED = {
    ("v30op_repe", "0x0f", "-", "v30_repe_0f_diagnostic_stop",
     "implemented", "final-root-target"),
    ("v30op_repne", "0x0f", "-", "v30_repne_0f_diagnostic_stop",
     "implemented", "final-root-target"),
}

HARNESS_ADDED = {
    ("patch-v30op_repe-0f", "v30op_repe", "0x0f",
     "v30_repe_0f_diagnostic_stop", "f30fc0000000000000", "1",
     "patched-root"),
    ("patch-v30op_repne-0f", "v30op_repne", "0x0f",
     "v30_repne_0f_diagnostic_stop", "f20fc0000000000000", "1",
     "patched-root"),
}

M62_HARNESS_REMOVED = {
    (
        "native-0f-28",
        "v30ope0x0f_table",
        "0x28",
        "v30_reserved_0x0f",
        "0f28c0000000000000",
        "1",
        "native-secondary",
    ),
}

M62_HARNESS_ADDED = {
    (
        "native-0f-28",
        "v30ope0x0f_table",
        "0x28",
        "v30_rol4_ea8",
        "0f28c0000000000000",
        "1",
        "native-secondary",
    ),
    (
        "patch-v30op-27",
        "v30op",
        "0x27",
        "v30_daa",
        "27c0000000000000",
        "1",
        "patched-root",
    ),
    (
        "patch-v30op-2f",
        "v30op",
        "0x2f",
        "v30_das",
        "2fc0000000000000",
        "1",
        "patched-root",
    ),
    (
        "patch-v30op-37",
        "v30op",
        "0x37",
        "v30_aaa",
        "37c0000000000000",
        "1",
        "patched-root",
    ),
    (
        "patch-v30op-3f",
        "v30op",
        "0x3f",
        "v30_aas",
        "3fc0000000000000",
        "1",
        "patched-root",
    ),
}

M64_NATIVE_HANDLERS = {
    "13": "v30_clr1_ea16_cl",
    "15": "v30_set1_ea16_cl",
    "16": "v30_not1_ea8_cl",
    "17": "v30_not1_ea16_cl",
    "1e": "v30_not1_ea8_i3",
    "1f": "v30_not1_ea16_i4",
    "26": "v30_cmp4s",
}

M64_HARNESS_REMOVED = M62_HARNESS_REMOVED | {
    (
        "native-0f-{}".format(opcode),
        "v30ope0x0f_table",
        "0x{}".format(opcode),
        "v30_reserved_0x0f",
        "0f{}c0000000000000".format(opcode),
        "1",
        "native-secondary",
    )
    for opcode in M64_NATIVE_HANDLERS
}

M64_HARNESS_ADDED = M62_HARNESS_ADDED | {
    (
        "native-0f-{}".format(opcode),
        "v30ope0x0f_table",
        "0x{}".format(opcode),
        handler,
        "0f{}c0000000000000".format(opcode),
        "1",
        "native-secondary",
    )
    for opcode, handler in M64_NATIVE_HANDLERS.items()
}


def verify_source_policy(root: pathlib.Path) -> None:
    dispatch = (root / "cpu/upd9002/upd9002_dispatch.c").read_text(
        encoding="utf-8")
    state = (root / "cpu/upd9002/upd9002_state.c").read_text(
        encoding="utf-8")
    pccore = (root / "pccore.c").read_text(encoding="utf-8")
    test = (root / "tests/upd9002/rep0f_diagnostic_stop.c").read_text(
        encoding="utf-8")
    requirements = {
        "REPNE patch": dispatch.count(
            "{0x0f, v30_repne_0f_diagnostic_stop}") == 1,
        "REPE patch": dispatch.count(
            "{0x0f, v30_repe_0f_diagnostic_stop}") == 1,
        "complete state restore": dispatch.count("upd9002_core_context.s = state_before;") == 1,
        "DMA bypass": "upd9002_diagnostic_pending()" in dispatch,
        "scheduler stop": pccore.count("upd9002_diagnostic_pending()") == 2,
        "MSW.PE preflight": state.count("state.MSW & MSW_PE") == 1,
        "512-case loop": "second < 256" in test,
        "state atomic test": "memcmp(&state_before, &upd9002_core_context.s" in test,
        "memory atomic test": "hash_before != memory_hash()" in test,
    }
    missing = [name for name, present in requirements.items() if not present]
    if missing:
        raise TransitionError("source policy evidence missing: " + ", ".join(missing))


def build_manifest(
        root: pathlib.Path, graph: str, provenance: str, support: str,
        harness: str) -> bytes:
    old_graph = file_rows(root, "tools/qa/golden/upd9002_final_dispatch_graph.csv")
    old_provenance = file_rows(
        root, "tools/qa/golden/upd9002_dispatch_provenance_m42.csv")
    old_support = file_rows(root, "tools/qa/golden/upd9002_support_map_m42.csv")
    old_harness = file_rows(root, "tests/upd9002/harness_manifest.csv")
    new_graph = rows(graph)
    new_provenance = rows(provenance)
    new_support = rows(support)
    new_harness = old_harness | HARNESS_ADDED

    graph_removed = expected_graph_removed()
    require_exact_difference(
        "final graph", old_graph, new_graph, graph_removed, GRAPH_ADDED)
    require_exact_difference(
        "provenance", old_provenance, new_provenance,
        PROVENANCE_REMOVED, PROVENANCE_ADDED)
    require_exact_difference(
        "support", old_support, new_support, SUPPORT_REMOVED, SUPPORT_ADDED)
    require_exact_difference(
        "direct harness", old_harness, new_harness, set(), HARNESS_ADDED)

    corpus = json.loads((root /
        "tests/ssts/baseline/upd9002_rep0f_corpus_m47.json").read_text(
            encoding="utf-8"))
    if corpus.get("dataset_id") != DATASET_ID:
        raise TransitionError("M43 dataset identity changed")
    if corpus.get("decoded_rep0f_record_count") != 0:
        raise TransitionError("M43 now contains decoded REP+0F records")

    value = {
        "schema": "vaeg-upd9002-rep0f-transition-manifest-m48-v1",
        "approved_evidence_sha": APPROVED_EVIDENCE_SHA,
        "policy": {
            "rep0f": (
                "emulator-level diagnostic stop before architectural state "
                "mutation; no architectural uPD9002/V52 claim"
            ),
            "protected_state_import": (
                "transactionally reject CPU286 payloads with MSW.PE set"
            ),
            "dormant_descriptor_residue": (
                "accepted and preserved when MSW.PE is clear"
            ),
        },
        "historical_artifact_sha256": HISTORICAL,
        "m48_artifact_sha256": {
            M48_PATHS["graph"]: sha256(graph.encode("utf-8")),
            M48_PATHS["provenance"]: sha256(provenance.encode("utf-8")),
            M48_PATHS["support"]: sha256(support.encode("utf-8")),
        },
        "final_dispatch_transition": {
            "removed": sorted_rows(graph_removed),
            "added": sorted_rows(GRAPH_ADDED),
            "removed_count": len(graph_removed),
            "added_count": len(GRAPH_ADDED),
            "note": (
                "18 removed rows are the derived CTS0/CTS1 reachability "
                "closure; no protected-mode source is deleted in M48"
            ),
        },
        "provenance_transition": {
            "removed": sorted_rows(PROVENANCE_REMOVED),
            "added": sorted_rows(PROVENANCE_ADDED),
        },
        "support_transition": {
            "removed": sorted_rows(SUPPORT_REMOVED),
            "added": sorted_rows(SUPPORT_ADDED),
        },
        "direct_harness": {
            "historical_case_count": len(old_harness) - 1,
            "historical_manifest_unchanged": True,
            "live_generated_transition_rows": sorted_rows(HARNESS_ADDED),
            "replacement_coverage": (
                "dedicated 522-case diagnostic-stop regression"
            ),
        },
        "instruction_matrix_case_count": 512,
        "m43_transition": {
            "dataset_id": DATASET_ID,
            "applicable_record_hashes": [],
            "known_gap_record_hashes": [],
            "semantic_failure_record_hashes": [],
            "failure_signatures": [],
            "reason": "the pinned dataset has zero decoded F2/F3+0F records",
        },
        "state_transition": {
            "rejected_field": "Cpu286StateCompat.MSW",
            "serialized_offset": 70,
            "serialized_size": 2,
            "rejected_mask": "0x0001 (MSW_PE)",
            "state_version_changed": False,
            "payload_layout_changed": False,
            "accepted_fixture_changes": [],
            "cpu_shut_changes": [],
        },
    }
    return (json.dumps(value, indent=2, sort_keys=True) + "\n").encode("utf-8")


def compare_or_write(path: pathlib.Path, data: bytes, write: bool) -> None:
    if write:
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(data)
        print("rep0f-transition: wrote {} sha256={}".format(path, sha256(data)))
        return
    try:
        existing = path.read_bytes()
    except OSError as error:
        raise TransitionError("cannot read M48 artifact {}: {}".format(
            path, error)) from error
    if existing != data:
        raise TransitionError("M48 artifact differs from regeneration: {}".format(path))
    print("rep0f-transition: checked {} sha256={}".format(path, sha256(data)))


def verify(root: pathlib.Path, write: bool, selftest: bool) -> None:
    verify_historical(root)
    verify_source_policy(root)
    for relative, expected in M48_IDENTITIES.items():
        actual = sha256((root / relative).read_bytes())
        if actual != expected:
            raise TransitionError(
                "accepted M48 artifact identity changed: {} expected={} actual={}".format(
                    relative, expected, actual))
    module = load_dispatch_module(root)
    if selftest:
        module.internal_selftest()
    live_graph, _live_provenance, harness, live_support = module.generate(root)
    graph = (root / M48_PATHS["graph"]).read_text(encoding="utf-8")
    provenance = (root / M48_PATHS["provenance"]).read_text(encoding="utf-8")
    support = (root / M48_PATHS["support"]).read_text(encoding="utf-8")
    require_exact_difference(
        "post-M48 governed graph",
        rows(graph),
        rows(live_graph),
        M66B_GRAPH_REMOVED,
        M66B_GRAPH_ADDED,
    )
    require_exact_difference(
        "post-M48 governed support",
        rows(support),
        rows(live_support),
        M66B_SUPPORT_REMOVED,
        M66B_SUPPORT_ADDED,
    )
    accepted_harness = (
        file_rows(root, "tests/upd9002/harness_manifest.csv") | HARNESS_ADDED
    )
    require_exact_difference(
        "post-M48 governed harness",
        accepted_harness,
        rows(harness),
        M64_HARNESS_REMOVED,
        M64_HARNESS_ADDED,
    )
    for name, content in (("graph", graph), ("provenance", provenance),
                          ("support", support)):
        compare_or_write(root / M48_PATHS[name], content.encode("utf-8"), False)
    manifest = build_manifest(root, graph, provenance, support, harness)
    compare_or_write(root / M48_PATHS["manifest"], manifest, write)


def parse_args(argv: Sequence[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=pathlib.Path, default=pathlib.Path("."))
    parser.add_argument("--write", action="store_true")
    parser.add_argument("--selftest", action="store_true")
    return parser.parse_args(argv)


def main(argv: Sequence[str] = None) -> int:
    arguments = parse_args(sys.argv[1:] if argv is None else argv)
    try:
        verify(arguments.root.resolve(), arguments.write, arguments.selftest)
    except (TransitionError, OSError, ValueError, KeyError, TypeError) as error:
        print("rep0f-transition: FAIL: {}".format(error), file=sys.stderr)
        return 1
    print("rep0f-transition: PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
