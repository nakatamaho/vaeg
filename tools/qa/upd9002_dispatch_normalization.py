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
# OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
# ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

import argparse
from pathlib import Path
import re
import subprocess
import sys


ROOTS = {
    "v30op": ("I286OP", 256),
    "v30op_repne": ("I286OP", 256),
    "v30op_repe": ("I286OP", 256),
    "v30op_repc": ("I286OP", 256),
    "v30ope0xf6_table": ("I286OPF6", 8),
    "v30ope0xf7_table": ("I286OPF6", 8),
}
SNAPSHOTS = {
    "v30op": "v30op_snapshot",
    "v30op_repne": "v30op_repne_snapshot",
    "v30op_repe": "v30op_repe_snapshot",
    "v30op_repc": "v30op_repc_snapshot",
    "v30ope0xf6_table": "v30ope0xf6_snapshot",
    "v30ope0xf7_table": "v30ope0xf7_snapshot",
}
CONSTRUCTION_OPERATIONS = (
    "CopyMemory(v30op, i286op, sizeof(v30op));",
    "V30PATCHING(v30op, v30patch_op);",
    "CopyMemory(v30op_repne, i286op_repne, sizeof(v30op_repne));",
    "V30PATCHING(v30op_repne, v30patch_repne);",
    "CopyMemory(v30op_repe, i286op_repe, sizeof(v30op_repe));",
    "V30PATCHING(v30op_repe, v30patch_repe);",
    "CopyMemory(v30ope0xf6_table, c_ope0xf6_table, sizeof(v30ope0xf6_table));",
    "v30ope0xf6_table[6] = v30_div_ea8;",
    "v30ope0xf6_table[7] = v30_idiv_ea8;",
    "CopyMemory(v30ope0xf7_table, c_ope0xf7_table, sizeof(v30ope0xf7_table));",
    "v30ope0xf7_table[6] = v30_div_ea16;",
    "v30ope0xf7_table[7] = v30_idiv_ea16;",
    "v30op_repc[i] = v30_reserved_repc;",
    "V30PATCHING(v30op_repc, v30patch_repc);",
)


class NormalizationError(Exception):
    pass


def require(condition, message):
    if not condition:
        raise NormalizationError(message)


def read_text(root, relative):
    path = root / relative
    require(path.is_file(), "missing required file: {}".format(relative))
    return path.read_text(encoding="utf-8")


def function_body(text, signature):
    start = text.find(signature)
    require(start >= 0, "missing function: {}".format(signature))
    brace = text.find("{", start + len(signature))
    require(brace >= 0, "missing function body: {}".format(signature))
    depth = 0
    for index in range(brace, len(text)):
        if text[index] == "{":
            depth += 1
        elif text[index] == "}":
            depth -= 1
            if depth == 0:
                return text[brace + 1:index]
    raise NormalizationError("unterminated function: {}".format(signature))


def compact(text):
    return re.sub(r"\s+", "", text)


def tracked_production_sources(root):
    result = subprocess.run(
        ["git", "-C", str(root), "ls-files", "-z", "*.c", "*.h", "*.cpp"],
        check=False,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    require(result.returncode == 0, "git ls-files failed")
    excluded = ("docs/", "hlp/", "i286x/", "tests/", "win9x/")
    return [
        item.decode("utf-8") for item in result.stdout.split(b"\0")
        if item and not item.decode("utf-8").startswith(excluded)
    ]


def check_constructor_references(root, core, dispatch, header):
    references = {}
    for relative in tracked_production_sources(root):
        count = len(re.findall(r"\bv30cinit\s*\(", read_text(root, relative)))
        if count:
            references[relative] = count
    require(references == {
        "i286c/i286c.c": 1,
        "i286c/v30patch.c": 1,
        "i286c/v30patch.h": 1,
    }, "constructor reference map changed: {}".format(references))
    require(header.count("void v30cinit(void);") == 1,
            "constructor declaration is not unique")
    require(dispatch.count("void v30cinit(void)") == 1,
            "constructor definition is not unique")
    initialize = function_body(core, "void i286c_initialize(void)")
    require(initialize.count("v30cinit();") == 1,
            "production initialization does not invoke constructor once")


def check_roots_and_construction(dispatch):
    for root, (pointer_type, size) in ROOTS.items():
        pattern = (r"\bstatic\s+{}\s+{}\s*\[{}\]\s*;".format(
            pointer_type, re.escape(root), size))
        require(len(re.findall(pattern, dispatch)) == 1,
                "root definition changed: {}".format(root))

    body = function_body(dispatch, "void v30cinit(void)")
    body_compact = compact(body)
    cursor = 0
    for operation in CONSTRUCTION_OPERATIONS:
        normalized = compact(operation)
        position = body_compact.find(normalized, cursor)
        require(position >= cursor,
                "constructor operation missing or reordered: {}".format(operation))
        require(body_compact.count(normalized) == 1,
                "constructor operation is not unique: {}".format(operation))
        cursor = position + len(normalized)
    require(body_compact.find("if(v30_dispatch_initialized)") <
            body_compact.find(compact(CONSTRUCTION_OPERATIONS[0])),
            "constructor re-entry guard does not precede table writes")
    require(body_compact.find("v30_dispatch_snapshot();") > cursor,
            "snapshot is not immediately after construction operations")
    require(body_compact.find("v30_dispatch_initialized=TRUE;") >
            body_compact.find("v30_dispatch_snapshot();"),
            "constructor completion marker precedes the snapshot")

    direct_expected = {
        "v30op": 0,
        "v30op_repne": 0,
        "v30op_repe": 0,
        "v30op_repc": 1,
        "v30ope0xf6_table": 2,
        "v30ope0xf7_table": 2,
    }
    for root, expected in direct_expected.items():
        assignments = re.findall(
            r"\b{}\s*\[[^\]]+\]\s*=".format(re.escape(root)), dispatch)
        require(len(assignments) == expected,
                "unexpected direct live-table writes for {}: {}".format(
                    root, len(assignments)))
    patcher = function_body(dispatch,
                            "static void v30patching(I286OP *op, const V30PATCH *patch, int cnt)")
    require(compact(patcher).count("op[patch->opnum]=patch->v30opcode;") == 1,
            "patch helper write changed")
    require(dispatch.count("#define\tV30PATCHING(a, b)\t") == 1,
            "patch helper macro changed")
    require(len(re.findall(r"\bV30PATCHING\s*\(", dispatch)) == 5,
            "patch helper has an alternative call site")
    copy_destinations = re.findall(
        r"\bCopyMemory\s*\(\s*([A-Za-z_]\w*)", dispatch)
    live_copy_destinations = [name for name in copy_destinations if name in ROOTS]
    require(live_copy_destinations == [
        "v30op", "v30op_repne", "v30op_repe",
        "v30ope0xf6_table", "v30ope0xf7_table",
    ], "live root copy destinations changed: {}".format(live_copy_destinations))

    call_expected = {
        "v30op": 5,
        "v30op_repne": 5,
        "v30op_repe": 5,
        "v30op_repc": 5,
        "v30ope0xf6_table": 1,
        "v30ope0xf7_table": 1,
    }
    for root, expected in call_expected.items():
        calls = re.findall(
            r"\b{}\s*\[[^\]]+\]\s*\(".format(re.escape(root)), dispatch)
        require(len(calls) == expected,
                "function-pointer call count changed for {}: {}".format(
                    root, len(calls)))


def check_snapshot_and_lifecycle(root, core, dispatch, header):
    for live, snapshot in SNAPSHOTS.items():
        pointer_type, size = ROOTS[live]
        require(len(re.findall(
            r"\bstatic\s+{}\s+{}\s*\[{}\]\s*;".format(
                pointer_type, re.escape(snapshot), size), dispatch)) == 1,
            "snapshot definition changed: {}".format(snapshot))
        require(compact(dispatch).count(
            "{}[i]={}[i];".format(snapshot, live)) == 1,
            "snapshot assignment changed: {}".format(live))
    require("memcmp" not in dispatch,
            "function-pointer table code must not use memcmp")
    equal = function_body(
        dispatch,
        "static BOOL v30_dispatch_equal(const I286OP *live,")
    f6_equal = function_body(
        dispatch,
        "static BOOL v30_dispatch_f6_equal(const I286OPF6 *live,")
    require(compact(equal).count("live[i]==snapshot[i]") == 1,
            "I286OP snapshots are not compared element-wise with ==")
    require(compact(f6_equal).count("live[i]==snapshot[i]") == 1,
            "I286OPF6 snapshots are not compared element-wise with ==")
    verify = compact(function_body(
        dispatch, "int upd9002_dispatch_test_verify(void)"))
    for live, snapshot in SNAPSHOTS.items():
        require(live in verify and snapshot in verify,
                "snapshot verifier omits {}".format(live))
    require("VAEG_UPD9002_M46_TESTING" in header,
            "test seam declarations are not target-local")

    reset = function_body(core, "void i286c_reset(void)")
    require(reset.count("upd9002_dispatch_test_require_immutable();") == 1,
            "ordinary reset lacks immutability verification")
    selftest = read_text(root, "sdl2/selftest.c")
    require(selftest.count("upd9002_dispatch_normalization_verify_live()") == 2,
            "selftest/state-load verification count changed")
    dedicated = read_text(root, "tests/upd9002/dispatch_normalization.c")
    require(dedicated.count("i286c_initialize();") == 1,
            "dedicated QA initialization count changed")
    require(dedicated.count("i286c_reset();") == 1,
            "dedicated QA reset count changed")
    require(dedicated.count("v30cinit();") == 1,
            "dedicated QA lacks exactly one rejected re-entry attempt")
    require("upd9002_dispatch_test_construction_count() != 1" in dedicated,
            "dedicated QA does not enforce one construction")
    require("upd9002_dispatch_test_rejected_count() != 1" in dedicated,
            "dedicated QA does not enforce rejected re-entry")

    cmake = read_text(root, "CMakeLists.txt")
    require(cmake.count("VAEG_UPD9002_M46_TESTING=1") == 2,
            "M46 instrumentation is not target-local to both test targets")
    require(cmake.count("tests/upd9002/dispatch_normalization.c") == 1,
            "dedicated QA source registration changed")
    main = read_text(root, "sdl2/np2.c")
    require(main.count("--upd9002-m46-dispatch-qa") == 1,
            "dedicated QA entry point changed")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", type=Path, required=True)
    arguments = parser.parse_args()
    root = arguments.root.resolve()
    try:
        core = read_text(root, "i286c/i286c.c")
        dispatch = read_text(root, "i286c/v30patch.c")
        header = read_text(root, "i286c/v30patch.h")
        check_constructor_references(root, core, dispatch, header)
        check_roots_and_construction(dispatch)
        check_snapshot_and_lifecycle(root, core, dispatch, header)
    except (NormalizationError, OSError, UnicodeError, ValueError) as error:
        print("upd9002-dispatch-normalization-static: FAIL: {}".format(error),
              file=sys.stderr)
        return 1
    print("upd9002-dispatch-normalization-static: constructor=v30cinit "
          "production-calls=1 successful-constructions=1")
    print("upd9002-dispatch-normalization-static: roots="
          "v30op:256,v30op_repne:256,v30op_repe:256,v30op_repc:256,"
          "v30ope0xf6_table:8,v30ope0xf7_table:8")
    print("upd9002-dispatch-normalization-static: writes=constructor-only "
          "snapshot=element-wise-equality reset=selftest=state-load=checked")
    return 0


if __name__ == "__main__":
    sys.exit(main())
