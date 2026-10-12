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

"""uPD9002 native-mode check against the SingleStepTests V20 corpus.

Every record with an empty initial prefetch queue in the selected profile is
executed by `vaeg --upd9002-ssts-worker`, except forms whose upstream
metadata status is `fpu` or `undefined`. A record passes when registers
(FLAGS under the upstream flags mask), watched RAM, I/O, and termination
match the corpus.

The committed baseline lists, per form, the upstream hashes of the records
that fail today. `check` fails on any record that passed in the baseline
and fails now (a regression), and on any baseline failure that now passes
(the baseline is stale; run `update` and commit it). No other history,
commit, or milestone identity is involved.
"""

from __future__ import annotations

import argparse
import copy
import gzip
import json
import os
import pathlib
import sys
from concurrent.futures import ProcessPoolExecutor
from typing import Any, Iterable

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import upd9002_ssts as ssts  # noqa: E402


BASELINE_SCHEMA = "vaeg-sst-v20-baseline-v1"
RESULT_SCHEMA = "vaeg-sst-v20-result-v1"
EXCLUDED_STATUSES = ("fpu", "undefined")
PROFILES = {
    "ci": "empty initial queue; ascending upstream hash; first 500 per form",
    "full": "empty initial queue; all records",
}
CI_RECORDS_PER_FORM = 500
SKIP_RETURN_CODE = 77
WORKER_TIMEOUT = 300.0

ERROR_SCHEMA = "SST_V20_BASELINE_SCHEMA"
ERROR_IDENTITY = "SST_V20_IDENTITY_MISMATCH"
ERROR_FORM_SET = "SST_V20_FORM_SET_MISMATCH"
ERROR_SELECTION = "SST_V20_SELECTION_MISMATCH"
ERROR_REGRESSION = "SST_V20_REGRESSION"
ERROR_STALE = "SST_V20_BASELINE_STALE"


class SstError(RuntimeError):
    """A corpus, worker, or file error that prevents a comparison."""


# ---- Content layer: pure comparison of a result with a baseline.


def validate_document(document: Any, schema: str) -> list[tuple[str, str]]:
    """Return schema findings for a baseline or result document."""

    findings = []
    if not isinstance(document, dict) or document.get("schema") != schema:
        return [(ERROR_SCHEMA, f"expected schema {schema}")]
    for key in ("dataset_id", "profile", "selection", "forms"):
        if key not in document:
            findings.append((ERROR_SCHEMA, f"missing key {key}"))
    if findings:
        return findings
    if document["profile"] not in PROFILES:
        findings.append((ERROR_SCHEMA, f"unknown profile {document['profile']!r}"))
    forms = document["forms"]
    if not isinstance(forms, dict) or not forms:
        return findings + [(ERROR_SCHEMA, "forms must be a non-empty object")]
    for form, entry in forms.items():
        where = f"form {form}"
        if not isinstance(entry, dict):
            findings.append((ERROR_SCHEMA, f"{where}: expected object"))
            continue
        selected = entry.get("selected")
        if not isinstance(selected, int) or selected < 0:
            findings.append((ERROR_SCHEMA, f"{where}: invalid selected count"))
            continue
        if entry.get("status") == "excluded":
            if set(entry) != {"status", "selected"}:
                findings.append((ERROR_SCHEMA, f"{where}: excluded form has extra keys"))
            continue
        if entry.get("status") != "executed" or set(entry) != {
            "status", "selected", "fail"
        }:
            findings.append((ERROR_SCHEMA, f"{where}: expected status, selected, fail"))
            continue
        fail = entry["fail"]
        if (
            not isinstance(fail, list)
            or any(not isinstance(item, str) for item in fail)
            or fail != sorted(set(fail))
            or len(fail) > selected
        ):
            findings.append((ERROR_SCHEMA, f"{where}: fail must be sorted unique hashes"))
    return findings


def compare(result: dict[str, Any], baseline: dict[str, Any]) -> list[tuple[str, str]]:
    """Return (code, detail) findings; empty means the check passes."""

    findings = validate_document(baseline, BASELINE_SCHEMA)
    findings += validate_document(result, RESULT_SCHEMA)
    if findings:
        return findings
    for key in ("dataset_id", "profile", "selection"):
        if result[key] != baseline[key]:
            findings.append((ERROR_IDENTITY, f"{key}: {result[key]!r} != {baseline[key]!r}"))
    if findings:
        return findings
    result_forms = set(result["forms"])
    baseline_forms = set(baseline["forms"])
    for form in sorted(result_forms ^ baseline_forms):
        side = "result" if form in result_forms else "baseline"
        findings.append((ERROR_FORM_SET, f"form {form} only in {side}"))
    for form in sorted(result_forms & baseline_forms):
        now = result["forms"][form]
        then = baseline["forms"][form]
        if now["selected"] != then["selected"] or now["status"] != then["status"]:
            findings.append((ERROR_SELECTION, f"form {form}: selection or status differs"))
            continue
        if now["status"] == "excluded":
            continue
        now_fail = set(now["fail"])
        then_fail = set(then["fail"])
        for record in sorted(now_fail - then_fail):
            findings.append((ERROR_REGRESSION, f"form {form} record {record}"))
        for record in sorted(then_fail - now_fail):
            findings.append((ERROR_STALE, f"form {form} record {record} now passes"))
    return findings


def baseline_from_result(result: dict[str, Any]) -> dict[str, Any]:
    baseline = {
        "schema": BASELINE_SCHEMA,
        "dataset_id": result["dataset_id"],
        "profile": result["profile"],
        "selection": result["selection"],
        "excluded_statuses": list(EXCLUDED_STATUSES),
        "forms": {},
    }
    for form, entry in sorted(result["forms"].items()):
        if entry["status"] == "excluded":
            baseline["forms"][form] = {"status": "excluded", "selected": entry["selected"]}
        else:
            baseline["forms"][form] = {
                "status": "executed",
                "selected": entry["selected"],
                "fail": list(entry["fail"]),
            }
    return baseline


# ---- Execution layer: corpus selection and worker runs.


def select_records(records: list[dict[str, Any]], profile: str) -> list[dict[str, Any]]:
    selected = [record for record in records if not record["initial"]["queue"]]
    selected.sort(key=lambda record: record["hash"])
    if profile == "ci":
        return selected[:CI_RECORDS_PER_FORM]
    return selected


def run_form(arguments: tuple[str, str, str, dict[str, Any]]) -> dict[str, Any]:
    path_text, worker_text, profile, metadata = arguments
    path = pathlib.Path(path_text)
    form = path.name.removesuffix(".json.gz").upper()
    with gzip.open(path, "rt", encoding="utf-8") as stream:
        records = json.load(stream)
    selected = select_records(records, profile)
    _, _, status, _, flags_mask = ssts.metadata_for_form(metadata, form)
    if status in EXCLUDED_STATUSES:
        return {"form": form, "status": "excluded", "selected": len(selected)}
    for record in selected:
        ssts.validate_record(record, f"{path.name}:{record.get('idx', '?')}")
    outcomes = ssts.run_worker_contained(pathlib.Path(worker_text), selected, WORKER_TIMEOUT)
    if len(outcomes) != len(selected):
        raise SstError(f"{form}: worker result count mismatch")
    fail = []
    details = []
    resolved = {"classification": "applicable", "flags_mask": flags_mask}
    for record, (worker_status, actual) in zip(selected, outcomes):
        watch, expected_ram = ssts.expected_memory(record)
        context = {
            "record": record,
            "record_digest": ssts.sha256_bytes(ssts.canonical_bytes(record)),
            "watch": watch,
            "expected_ram": expected_ram,
        }
        outcome, failure = ssts.compare_result(
            "sst-v20", profile, form, resolved, context, worker_status, actual
        )
        if outcome != "pass":
            fail.append(record["hash"])
            details.append(
                {
                    "record": record["hash"],
                    "outcome": outcome,
                    "mismatch_kinds": failure["content"]["mismatch_kinds"],
                    "instruction_bytes": failure["content"]["instruction_bytes"],
                }
            )
    order = sorted(range(len(fail)), key=lambda index: fail[index])
    return {
        "form": form,
        "status": "executed",
        "selected": len(selected),
        "fail": [fail[index] for index in order],
        "details": [details[index] for index in order],
    }


def run(
    dataset_root: pathlib.Path,
    manifest_path: pathlib.Path,
    worker: pathlib.Path,
    profile: str,
    jobs: int,
) -> dict[str, Any]:
    manifest = ssts.load_manifest(manifest_path)
    ssts.verify_fast(dataset_root, manifest)
    if not worker.is_file():
        raise SstError(f"worker executable is missing: {worker}")
    metadata = json.loads(
        (dataset_root / ssts.SUITE_PATH / "metadata.json").read_text(encoding="utf-8")
    )
    ssts.validate_metadata(metadata)
    tasks = [
        (str(path), str(worker), profile, metadata)
        for path in ssts.corpus_files(dataset_root)
    ]
    forms = {}
    details = {}
    with ProcessPoolExecutor(max_workers=jobs) as executor:
        for entry in executor.map(run_form, tasks):
            form = entry.pop("form")
            if "details" in entry:
                if entry["details"]:
                    details[form] = entry["details"]
                del entry["details"]
            forms[form] = entry
            print(
                f"sst-v20: {form} {entry['status']} selected={entry['selected']}"
                + (f" fail={len(entry['fail'])}" if entry["status"] == "executed" else ""),
                file=sys.stderr,
                flush=True,
            )
    return {
        "schema": RESULT_SCHEMA,
        "dataset_id": manifest["dataset_id"],
        "profile": profile,
        "selection": PROFILES[profile],
        "forms": dict(sorted(forms.items())),
        "failure_details": dict(sorted(details.items())),
    }


def summary(document: dict[str, Any]) -> str:
    executed = [entry for entry in document["forms"].values() if entry["status"] == "executed"]
    selected = sum(entry["selected"] for entry in executed)
    failed = sum(len(entry["fail"]) for entry in executed)
    excluded = sum(
        entry["selected"] for entry in document["forms"].values()
        if entry["status"] == "excluded"
    )
    failing_forms = sum(1 for entry in executed if entry["fail"])
    return (
        f"forms={len(document['forms'])} executed={selected} pass={selected - failed} "
        f"fail={failed} failing_forms={failing_forms} excluded={excluded}"
    )


# ---- Files and command line.


def read_json(path: pathlib.Path) -> Any:
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except (OSError, UnicodeError, json.JSONDecodeError) as error:
        raise SstError(f"cannot read {path}: {error}") from error


def write_json(path: pathlib.Path, value: Any) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(value, indent=1, sort_keys=False) + "\n", encoding="utf-8")


def report(findings: list[tuple[str, str]], limit: int = 50) -> int:
    if not findings:
        return 0
    for code, detail in findings[:limit]:
        print(f"{code} {detail}", file=sys.stderr)
    if len(findings) > limit:
        print(f"... {len(findings) - limit} more findings", file=sys.stderr)
    codes = sorted({code for code, _ in findings})
    print(f"sst-v20: FAIL codes={','.join(codes)}", file=sys.stderr)
    if ERROR_STALE in codes:
        print(
            "sst-v20: records improved; regenerate the baseline with "
            "`tools/qa/sst_v20.py update` and commit it",
            file=sys.stderr,
        )
    return 1


def synthetic_pair() -> tuple[dict[str, Any], dict[str, Any]]:
    result = {
        "schema": RESULT_SCHEMA,
        "dataset_id": "dataset",
        "profile": "ci",
        "selection": PROFILES["ci"],
        "forms": {
            "00": {"status": "executed", "selected": 3, "fail": []},
            "6C": {"status": "executed", "selected": 3, "fail": ["aa", "bb"]},
            "D8": {"status": "excluded", "selected": 3},
        },
        "failure_details": {},
    }
    return result, baseline_from_result(result)


def selftest() -> None:
    result, baseline = synthetic_pair()
    if compare(result, baseline):
        raise AssertionError("synthetic pair must pass")

    def on_result(mutation):
        def apply(pair):
            mutation(pair[0])
        return apply

    def on_baseline(mutation):
        def apply(pair):
            mutation(pair[1])
        return apply

    cases = [
        ("regression", ERROR_REGRESSION,
         on_result(lambda r: r["forms"]["00"]["fail"].append("cc"))),
        ("improvement", ERROR_STALE,
         on_result(lambda r: r["forms"]["6C"]["fail"].remove("bb"))),
        ("dataset", ERROR_IDENTITY,
         on_result(lambda r: r.__setitem__("dataset_id", "other"))),
        ("profile", ERROR_IDENTITY,
         on_baseline(lambda b: b.__setitem__("profile", "full"))),
        ("form-added", ERROR_FORM_SET,
         on_result(lambda r: r["forms"].__setitem__(
             "01", {"status": "executed", "selected": 1, "fail": []}))),
        ("form-removed", ERROR_FORM_SET,
         on_baseline(lambda b: b["forms"].pop("D8"))),
        ("selected-count", ERROR_SELECTION,
         on_result(lambda r: r["forms"]["00"].__setitem__("selected", 4))),
        ("status-flip", ERROR_SELECTION,
         on_baseline(lambda b: b["forms"].__setitem__(
             "D8", {"status": "executed", "selected": 3, "fail": []}))),
        ("unsorted-fail", ERROR_SCHEMA,
         on_baseline(lambda b: b["forms"]["6C"].__setitem__("fail", ["bb", "aa"]))),
        ("schema", ERROR_SCHEMA,
         on_baseline(lambda b: b.__setitem__("schema", "other"))),
    ]
    for name, expected, mutate in cases:
        pair = copy.deepcopy(synthetic_pair())
        mutate(pair)
        codes = {code for code, _ in compare(pair[0], pair[1])}
        if codes != {expected}:
            raise AssertionError(f"{name}: expected {{{expected}}}, got {sorted(codes)}")
    print(f"sst-v20-selftest: {len(cases) + 1} checks passed")


def default_jobs() -> int:
    return max(1, min(8, os.cpu_count() or 1))


def parse_args(argv: Iterable[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    subparsers = parser.add_subparsers(dest="command", required=True)

    def add_run_arguments(command: argparse.ArgumentParser) -> None:
        command.add_argument("--dataset-root", type=pathlib.Path, required=True)
        command.add_argument("--manifest", type=pathlib.Path,
                             default=pathlib.Path("tests/ssts/v20_dataset_manifest.json"))
        command.add_argument("--worker", type=pathlib.Path, required=True)
        command.add_argument("--profile", choices=sorted(PROFILES), default="ci")
        command.add_argument("--jobs", type=int, default=default_jobs())
        command.add_argument("--output", type=pathlib.Path)

    run_parser = subparsers.add_parser("run", help="execute a profile and write a result")
    add_run_arguments(run_parser)
    ci_parser = subparsers.add_parser("ci", help="execute a profile and check it")
    add_run_arguments(ci_parser)
    ci_parser.add_argument("--baseline", type=pathlib.Path, required=True)
    check_parser = subparsers.add_parser("check", help="compare a result with a baseline")
    check_parser.add_argument("--result", type=pathlib.Path, required=True)
    check_parser.add_argument("--baseline", type=pathlib.Path, required=True)
    update_parser = subparsers.add_parser("update", help="write a baseline from a result")
    update_parser.add_argument("--result", type=pathlib.Path, required=True)
    update_parser.add_argument("--baseline", type=pathlib.Path, required=True)
    validate_parser = subparsers.add_parser("validate", help="check a baseline's schema")
    validate_parser.add_argument("--baseline", type=pathlib.Path, required=True)
    subparsers.add_parser("selftest", help="run the comparison self-test")
    skip_parser = subparsers.add_parser("skip", help="report a skipped check (exit 77)")
    skip_parser.add_argument("--reason", required=True)
    return parser.parse_args(list(argv))


def main(argv: Iterable[str] | None = None) -> int:
    arguments = parse_args(sys.argv[1:] if argv is None else argv)
    try:
        if arguments.command == "selftest":
            selftest()
            return 0
        if arguments.command == "skip":
            print(f"sst-v20: SKIP {arguments.reason}")
            return SKIP_RETURN_CODE
        if arguments.command == "validate":
            baseline = read_json(arguments.baseline)
            status = report(validate_document(baseline, BASELINE_SCHEMA))
            if status == 0:
                print(f"sst-v20: baseline valid {summary(baseline)}")
            return status
        if arguments.command in {"run", "ci"}:
            result = run(
                arguments.dataset_root.resolve(),
                arguments.manifest.resolve(),
                arguments.worker.resolve(),
                arguments.profile,
                max(1, arguments.jobs),
            )
            if arguments.output is not None:
                write_json(arguments.output, result)
            print(f"sst-v20: result {summary(result)}")
            if arguments.command == "run":
                return 0
            status = report(compare(result, read_json(arguments.baseline)))
            if status == 0:
                print("sst-v20: PASS no regressions, baseline current")
            return status
        result = read_json(arguments.result)
        if arguments.command == "update":
            findings = validate_document(result, RESULT_SCHEMA)
            if findings:
                return report(findings)
            write_json(arguments.baseline, baseline_from_result(result))
            print(f"sst-v20: wrote {arguments.baseline} {summary(result)}")
            return 0
        status = report(compare(result, read_json(arguments.baseline)))
        if status == 0:
            print("sst-v20: PASS no regressions, baseline current")
        return status
    except (SstError, ssts.CorpusError, OSError) as error:
        print(f"sst-v20-error: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
