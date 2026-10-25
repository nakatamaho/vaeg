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

"""Capture/prepare/compare the closed G132 virtual/physical X11 display matrix.

Reference preparation always creates DRAFT, never approval. All generated
artifacts, inputs and review records must stay outside Git. A draft pixel
match returns 2 (APPROVAL_REQUIRED), never gate PASS. The caller must freeze
and supply the previously reviewed reference manifest digest for comparison.
"""

from __future__ import annotations

import argparse
import hashlib
import html
import json
from pathlib import Path
import shutil
import subprocess
import sys

import screen_compare as screen
from x11_capture import default_cases, digest

CONTRACT = "g132-x11-matrix-v1"
ROOT = Path(__file__).resolve().parents[2]


def new_output(path):
    path = path.resolve()
    screen.require(path != ROOT and ROOT not in path.parents and not path.exists(), "M132_UI_MATRIX_OUTPUT")
    path.mkdir(parents=True)
    return path


def case_set(cases):
    expected = {case["id"] for case in default_cases()}
    screen.require(isinstance(cases, dict) and set(cases) == expected, "M132_UI_MATRIX_CASE_SET")


def capture(args):
    output = new_output(args.output_dir)
    results = {}
    for case in default_cases():
        case_path = output / (case["id"] + ".json")
        case_path.write_text(json.dumps(case, indent=2) + "\n")
        argv = [sys.executable, str(Path(__file__).with_name("x11_capture.py")),
                "--worker", str(args.worker.resolve()), "--evaluated-commit", args.evaluated_commit,
                "--roms", str(args.roms.resolve()), "--backup-seed", str(args.backup_seed.resolve()),
                "--case", str(case_path), "--output-dir", str(output / case["id"]),
                "--display", args.display, "--display-kind", args.display_kind]
        with (output / (case["id"] + ".log")).open("wb") as log:
            result = subprocess.run(argv, stdout=log, stderr=subprocess.STDOUT, timeout=100)
        report_path = output / case["id"] / "result.json"
        results[case["id"]] = {"exit_status": result.returncode,
                               "status": json.loads(report_path.read_text())["status"] if report_path.exists() else "FAIL",
                               "command": argv, "report_sha256": digest(report_path) if report_path.exists() else None}
        print(case["id"], results[case["id"]]["status"], flush=True)
    case_set(results)
    complete = all(row["exit_status"] == 0 and row["status"] == "CAPTURED_UNREVIEWED" for row in results.values())
    report = {"contract": CONTRACT, "status": "CAPTURED_UNREVIEWED" if complete else "FAIL",
              "gate_pass": False, "evaluated_commit": args.evaluated_commit, "worker_sha256": digest(args.worker),
              "tool_sha256": digest(__file__), "cases": results}
    (output / "matrix.json").write_text(json.dumps(report, indent=2) + "\n")
    return 0 if complete else 1


def policy_for(case, metadata):
    """Expected layout at pinned 16px/100% or 32px/200% GUI font metrics.

    Do not infer footer heights from candidate pixels. GUI scale is NOT the
    drawable/logical DPI ratio: current X11 targets explicitly measure ratio 1.
    """
    w, h = metadata["target"]["logical_size"]
    scale = case["ui_scale"] // 100
    status = 24 * scale if case["status"] else 0
    panel = 14 * max(1, (w + 320) // 640) if case["panel"] and not case["fullscreen"] else 0
    menu = 0 if case["fullscreen"] else 22 * scale
    if case["fullscreen"]:
        screen.require([w, h] == metadata["screen_size"], "M132_UI_FOOTER_GEOMETRY")
    else:
        screen.require(w == case["width"] and h == case["height"] + status + panel,
                       "M132_UI_FOOTER_GEOMETRY")
    guest_bottom = h - status - panel
    screen.require(guest_bottom > menu, "M132_UI_FOOTER_GEOMETRY")
    rectangles = [("full-client", [0, 0, w, h]), ("guest", [0, menu, w, guest_bottom - menu])]
    if menu:
        rectangles.append(("menu", [0, 0, w, menu]))
    if status:
        rectangles.append(("status", [0, guest_bottom, w, status]))
    if panel:
        rectangles.append(("panel", [0, h - panel, w, panel]))
    return screen.validate_policy({"contract": screen.CONTRACT, "width": w, "height": h,
                                   "reference_sha256": metadata["image_sha256"], "target": metadata["target"],
                                   "masks": [], "regions": [{"name": name, "rect": rect, "max_delta": 0,
                                                              "max_changed_fraction": 0} for name, rect in rectangles]})


def panel_visibility(image, visible):
    """Independent static-theme oracle at the unused bottom-centre panel row."""
    w, h, pixels = image
    screen.require(w > 0 and h > 0 and len(pixels) == w * h * 4, "M132_UI_PANEL_VISIBILITY")
    positions = range(max(0, w // 2 - 5), min(w, w // 2 + 5))
    has_panel_background = all(pixels[((h - 1) * w + x) * 4:((h - 1) * w + x) * 4 + 4] ==
                               bytes((34, 34, 36, 255)) for x in positions)
    screen.require(has_panel_background == visible, "M132_UI_PANEL_VISIBILITY")


def worker_binding(report, matrix):
    for record in (report, matrix):
        screen.require(isinstance(record, dict), "M132_UI_MATRIX_WORKER_BINDING")
        for key, length in (("worker_sha256", 64), ("evaluated_commit", 40)):
            value = record.get(key)
            screen.require(isinstance(value, str) and len(value) == length and
                           all(c in "0123456789abcdef" for c in value), "M132_UI_MATRIX_WORKER_BINDING")
    screen.require(report.get("worker_sha256") == matrix.get("worker_sha256") and
                   report.get("evaluated_commit") == matrix.get("evaluated_commit"),
                   "M132_UI_MATRIX_WORKER_BINDING")


def reference_approval(manifest):
    approval = manifest.get("approval", {})
    screen.require(isinstance(approval, dict), "M132_UI_MATRIX_APPROVAL")
    screen.require(approval.get("status") in ("DRAFT", "APPROVED"), "M132_UI_MATRIX_APPROVAL")
    approved = approval["status"] == "APPROVED"
    screen.require(not approved or isinstance(approval.get("record"), str) and approval["record"].strip(),
                   "M132_UI_MATRIX_APPROVAL")
    return approved


def comparison_status(approved, matched):
    return "PASS" if approved and matched else "APPROVAL_REQUIRED" if matched else "FAIL"


def prepare(args):
    source = args.capture_dir.resolve()
    matrix = json.loads((source / "matrix.json").read_text())
    case_set(matrix.get("cases"))
    screen.require(matrix["status"] == "CAPTURED_UNREVIEWED", "M132_UI_MATRIX_INCOMPLETE")
    output = new_output(args.output_dir)
    entries, cards = {}, []
    for case in default_cases():
        name = case["id"]
        root, target = source / name, output / name
        report_data = (root / "result.json").read_bytes()
        screen.require(screen.sha(report_data) == matrix["cases"][name]["report_sha256"], "M132_UI_MATRIX_CAPTURE_BINDING")
        report = json.loads(report_data)
        worker_binding(report, matrix)
        screen.require(report["status"] == "CAPTURED_UNREVIEWED" and report["case"] == case,
                       "M132_UI_MATRIX_INCOMPLETE")
        for filename, expected in report["output_digests"].items():
            screen.require(digest(root / filename) == expected, "M132_UI_MATRIX_CAPTURE_BINDING")
        metadata = json.loads((root / "capture.json").read_text())
        policy = policy_for(case, metadata)
        panel_visibility(screen.read_png((root / "client.png").read_bytes()), metadata["target"]["panel_visible"])
        target.mkdir()
        shutil.copyfile(root / "client.png", target / "reference.png")
        shutil.copyfile(root / "capture.json", target / "reference-capture.json")
        (target / "policy.json").write_text(json.dumps(policy, indent=2) + "\n")
        entries[name] = {"reference_sha256": digest(target / "reference.png"),
                         "policy_sha256": digest(target / "policy.json"), "capture_report_sha256": screen.sha(report_data)}
        cards.append(f'<article><h2>{html.escape(name)}</h2><a href="{name}/reference.png">'
                     f'<img src="{name}/reference.png" width="640"></a></article>')
    manifest = {"contract": CONTRACT, "approval": {"status": "DRAFT", "record": None},
                "baseline_commit": matrix["evaluated_commit"], "baseline_worker_sha256": matrix["worker_sha256"],
                "capture_matrix_sha256": digest(source / "matrix.json"), "cases": entries}
    (output / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    (output / "index.html").write_text('<!doctype html><meta charset="utf-8"><title>G132 reference DRAFT</title>'
                                       '<h1>G132 X11 reference set — DRAFT, NOT APPROVED</h1>' + "\n".join(cards))
    print(json.dumps({"status": "DRAFT_REFERENCE_PREPARED", "count": len(entries), "gate_pass": False}))
    return 0


def compare(args):
    reference, actual = args.reference_dir.resolve(), args.capture_dir.resolve()
    data = (reference / "manifest.json").read_bytes()
    screen.require(screen.sha(data) == args.manifest_sha256, "M132_UI_MATRIX_MANIFEST_BINDING")
    manifest = json.loads(data)
    case_set(manifest.get("cases"))
    approved = reference_approval(manifest)
    matrix = json.loads((actual / "matrix.json").read_text())
    case_set(matrix.get("cases"))
    screen.require(matrix["status"] == "CAPTURED_UNREVIEWED", "M132_UI_MATRIX_INCOMPLETE")
    output = new_output(args.output_dir)
    results = {}
    for case in default_cases():
        name = case["id"]
        root, candidate = reference / name, actual / name
        entry = manifest["cases"][name]
        policy = screen.load_policy((root / "policy.json").read_bytes(), entry["policy_sha256"])
        screen.require(digest(root / "reference.png") == entry["reference_sha256"], "M132_UI_MATRIX_REFERENCE_BINDING")
        screen.require(digest(candidate / "result.json") == matrix["cases"][name]["report_sha256"],
                       "M132_UI_MATRIX_CAPTURE_BINDING")
        report = json.loads((candidate / "result.json").read_text())
        worker_binding(report, matrix)
        screen.require(report["case"] == case, "M132_UI_MATRIX_CASE_SET")
        for filename, expected in report["output_digests"].items():
            screen.require(digest(candidate / filename) == expected, "M132_UI_MATRIX_CAPTURE_BINDING")
        metadata = json.loads((candidate / "capture.json").read_text())
        screen.require(policy_for(case, metadata) == {**policy, "reference_sha256": metadata["image_sha256"]},
                       "M132_UI_MATRIX_TARGET")
        panel_visibility(screen.read_png((candidate / "client.png").read_bytes()), metadata["target"]["panel_visible"])
        comparison, diff = screen.evaluate((root / "reference.png").read_bytes(),
                                          (candidate / "client.png").read_bytes(), policy, metadata)
        folder = output / name
        folder.mkdir()
        (folder / "diff.png").write_bytes(screen.png_bytes(diff))
        (folder / "result.json").write_text(json.dumps(comparison, indent=2) + "\n")
        results[name] = comparison
    matched = all(result["status"] == "PASS" for result in results.values())
    status = comparison_status(approved, matched)
    summary = {"contract": CONTRACT, "status": status, "gate_pass": False,
               "note": "suite PASS is not whole G132 acceptance; physical/macOS/HiDPI/native-CRT remain separate",
               "manifest_sha256": args.manifest_sha256, "candidate_matrix_sha256": digest(actual / "matrix.json"),
               "tool_sha256": digest(__file__), "cases": results}
    (output / "summary.json").write_text(json.dumps(summary, indent=2) + "\n")
    print(json.dumps({"status": status, "matched": sum(result["status"] == "PASS" for result in results.values()),
                      "total": len(results), "gate_pass": False}))
    return 0 if status == "PASS" else 2 if status == "APPROVAL_REQUIRED" else 1


def selftest():
    cases = default_cases()
    valid = {case["id"]: {} for case in cases}
    case_set(valid)
    try:
        case_set({key: value for key, value in valid.items() if key != cases[0]["id"]})
    except screen.QaError as error:
        screen.require(error.code == "M132_UI_MATRIX_CASE_SET", "M132_UI_SELFTEST")
    else:
        raise screen.QaError("M132_UI_SELFTEST")
    case = next(case for case in cases if case["id"] == "software-normal-s1p1")
    metadata = {"image_sha256": "0" * 64, "screen_size": [1280, 960],
                "target": {"platform": "fixture", "backend": "sdl-software", "video_driver": "x11",
                           "dpi_scale": 1, "logical_size": [640, 460], "fullscreen": False,
                           "status_visible": True, "panel_visible": True}}
    policy = policy_for(case, metadata)
    screen.require(policy["regions"][-1]["rect"] == [0, 446, 640, 14], "M132_UI_SELFTEST")
    try:
        policy_for(case, {**metadata, "target": {**metadata["target"], "logical_size": [640, 459]}})
    except screen.QaError as error:
        screen.require(error.code == "M132_UI_FOOTER_GEOMETRY", "M132_UI_SELFTEST")
    else:
        raise screen.QaError("M132_UI_SELFTEST")
    def rejects(action, expected):
        try:
            action()
        except screen.QaError as error:
            screen.require(error.code == expected, "M132_UI_SELFTEST")
        else:
            raise screen.QaError("M132_UI_SELFTEST")
    panel_visibility((1, 1, bytes((34, 34, 36, 255))), True)
    panel_visibility((1, 1, bytes((0, 0, 0, 255))), False)
    rejects(lambda: panel_visibility((1, 1, bytes((0, 0, 0, 255))), True), "M132_UI_PANEL_VISIBILITY")
    identity = {"worker_sha256": "0" * 64, "evaluated_commit": "0" * 40}
    worker_binding(identity, identity)
    rejects(lambda: worker_binding({**identity, "worker_sha256": "1" * 64}, identity),
            "M132_UI_MATRIX_WORKER_BINDING")
    draft = {"approval": {"status": "DRAFT", "record": None}}
    approved = {"approval": {"status": "APPROVED", "record": "synthetic fixture approval"}}
    screen.require(not reference_approval(draft) and reference_approval(approved), "M132_UI_SELFTEST")
    rejects(lambda: reference_approval({"approval": {**approved["approval"], "record": None}}),
            "M132_UI_MATRIX_APPROVAL")
    rejects(lambda: reference_approval({"approval": {**draft["approval"], "status": "SELF_APPROVED"}}),
            "M132_UI_MATRIX_APPROVAL")
    screen.require(comparison_status(False, True) == "APPROVAL_REQUIRED" and
                   comparison_status(False, False) == "FAIL" and comparison_status(True, True) == "PASS",
                   "M132_UI_SELFTEST")
    print("PASS: matrix case-set, geometry, worker binding and fail-closed reference approval")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("action", choices=("capture", "prepare", "compare", "selftest"))
    for name in ("worker", "roms", "backup-seed", "capture-dir", "reference-dir", "output-dir"):
        parser.add_argument("--" + name, type=Path)
    parser.add_argument("--evaluated-commit")
    parser.add_argument("--display")
    parser.add_argument("--display-kind", choices=("virtual-x11", "physical-x11"))
    parser.add_argument("--manifest-sha256")
    args = parser.parse_args()
    try:
        if args.action == "selftest":
            selftest()
            return 0
        screen.require(args.output_dir is not None, "M132_UI_MATRIX_ARGUMENTS")
        if args.action == "capture":
            screen.require(all((args.worker, args.roms, args.backup_seed, args.evaluated_commit,
                                args.display, args.display_kind)), "M132_UI_MATRIX_ARGUMENTS")
            return capture(args)
        screen.require(args.capture_dir is not None, "M132_UI_MATRIX_ARGUMENTS")
        if args.action == "prepare":
            return prepare(args)
        screen.require(args.reference_dir and args.manifest_sha256, "M132_UI_MATRIX_ARGUMENTS")
        return compare(args)
    except (screen.QaError, OSError, ValueError, KeyError, TypeError, subprocess.SubprocessError) as error:
        print(json.dumps({"status": "FAIL", "error_code": error.code if isinstance(error, screen.QaError)
                          else "M132_UI_MATRIX_EXECUTION"}))
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
