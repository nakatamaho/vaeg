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

"""Compare fixed-target final-client PNG captures against a frozen policy.

Standard library only; supports noninterlaced 8-bit RGB/RGBA PNG. Private
captures, policies, metadata and outputs stay outside Git. Caller-supplied
capture metadata must come from a trusted capture adapter: this comparator
checks its binding but cannot independently attest a screenshot's origin.
"""

from __future__ import annotations

import argparse
import copy
import hashlib
import json
from pathlib import Path
import struct
import tempfile
import zlib

ROOT = Path(__file__).resolve().parents[2]
CONTRACT = "screen-compare-v1"
TARGET_KEYS = {"platform", "backend", "video_driver", "dpi_scale", "logical_size",
               "fullscreen", "status_visible", "panel_visible"}


class QaError(Exception):
    def __init__(self, code):
        super().__init__(code)
        self.code = code


def require(condition, code):
    if not condition:
        raise QaError(code)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def read_png(data):
    """Decode PNG pixels, verifying CRC, bounded inflation and row filters."""
    require(data[:8] == b"\x89PNG\r\n\x1a\n", "SCREEN_QA_PNG_INVALID")
    pos, header, compressed, ended = 8, None, bytearray(), False
    while pos < len(data):
        require(pos + 12 <= len(data), "SCREEN_QA_PNG_INVALID")
        length, tag = struct.unpack_from(">I4s", data, pos)
        end = pos + 8 + length
        require(end + 4 <= len(data), "SCREEN_QA_PNG_INVALID")
        body = data[pos + 8:end]
        require(zlib.crc32(tag + body) == struct.unpack_from(">I", data, end)[0],
                "SCREEN_QA_PNG_INVALID")
        require(header is not None or tag == b"IHDR", "SCREEN_QA_PNG_INVALID")
        if tag == b"IHDR":
            require(header is None and length == 13, "SCREEN_QA_PNG_INVALID")
            header = struct.unpack(">IIBBBBB", body)
        elif tag == b"IDAT":
            compressed.extend(body)
        elif tag == b"IEND":
            require(length == 0 and end + 4 == len(data), "SCREEN_QA_PNG_INVALID")
            ended = True
            break
        elif tag == b"tRNS" or tag[0] & 32 == 0:
            raise QaError("SCREEN_QA_PNG_UNSUPPORTED")
        pos = end + 4
    require(ended and header is not None, "SCREEN_QA_PNG_INVALID")
    w, h, depth, colour, compression, filtering, interlace = header
    require(depth == 8 and colour in (2, 6) and
            compression == filtering == interlace == 0, "SCREEN_QA_PNG_UNSUPPORTED")
    require(0 < w <= 8192 and 0 < h <= 8192 and w * h <= 32000000,
            "SCREEN_QA_IMAGE_LIMIT")
    channels = 3 if colour == 2 else 4
    stride = w * channels
    limit = (stride + 1) * h
    try:
        decoder = zlib.decompressobj()
        raw = decoder.decompress(bytes(compressed), limit + 1)
        require(len(raw) == limit and decoder.eof and not decoder.unused_data,
                "SCREEN_QA_PNG_INVALID")
    except zlib.error as error:
        raise QaError("SCREEN_QA_PNG_INVALID") from error
    pixels, previous = bytearray(), bytearray(stride)
    for y in range(h):
        start = y * (stride + 1)
        method = raw[start]
        require(method <= 4, "SCREEN_QA_PNG_INVALID")
        row = bytearray(raw[start + 1:start + 1 + stride])
        for x in range(stride):
            a = row[x - channels] if x >= channels else 0
            b = previous[x]
            c = previous[x - channels] if x >= channels else 0
            if method == 1:
                row[x] = (row[x] + a) & 255
            elif method == 2:
                row[x] = (row[x] + b) & 255
            elif method == 3:
                row[x] = (row[x] + (a + b) // 2) & 255
            elif method == 4:
                p = a + b - c
                pa, pb, pc = abs(p - a), abs(p - b), abs(p - c)
                predictor = a if pa <= pb and pa <= pc else b if pb <= pc else c
                row[x] = (row[x] + predictor) & 255
        if channels == 4:
            pixels.extend(row)
        else:
            for x in range(0, stride, 3):
                pixels.extend(row[x:x + 3])
                pixels.append(255)
        previous = row
    return w, h, bytes(pixels)


def colour_profile(data):
    """Called only after PNG validation; colour metadata is not a pixel mask."""
    pos, result = 8, []
    while pos < len(data):
        length, tag = struct.unpack_from(">I4s", data, pos)
        if tag in (b"gAMA", b"cHRM", b"sRGB", b"iCCP", b"cICP", b"mDCV", b"cLLI"):
            result.append((tag, data[pos + 8:pos + 8 + length]))
        pos += length + 12
    return sorted(result)


def png_bytes(image):
    w, h, pixels = image
    def chunk(tag, body):
        return struct.pack(">I", len(body)) + tag + body + struct.pack(">I", zlib.crc32(tag + body))
    raw = b"".join(b"\x00" + pixels[y * w * 4:(y + 1) * w * 4] for y in range(h))
    return (b"\x89PNG\r\n\x1a\n" +
            chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 6, 0, 0, 0)) +
            chunk(b"IDAT", zlib.compress(raw)) + chunk(b"IEND", b""))


def rectangle(rect, w, h):
    require(isinstance(rect, list) and len(rect) == 4 and
            all(type(n) is int for n in rect), "SCREEN_QA_POLICY")
    x, y, rw, rh = rect
    require(x >= 0 and y >= 0 and rw > 0 and rh > 0 and
            x + rw <= w and y + rh <= h, "SCREEN_QA_POLICY")
    return rect


def contains(rect, x, y):
    rx, ry, rw, rh = rect
    return rx <= x < rx + rw and ry <= y < ry + rh


def validate_policy(policy):
    require(isinstance(policy, dict) and policy.get("contract") == CONTRACT, "SCREEN_QA_POLICY")
    w, h = policy.get("width"), policy.get("height")
    require(type(w) is int and type(h) is int and w > 0 and h > 0 and
            w <= 8192 and h <= 8192 and w * h <= 32000000, "SCREEN_QA_POLICY")
    target = policy.get("target")
    require(isinstance(target, dict) and set(target) == TARGET_KEYS, "SCREEN_QA_POLICY")
    require(all(type(target[k]) is bool for k in ("fullscreen", "status_visible", "panel_visible")),
            "SCREEN_QA_POLICY")
    require(type(target["dpi_scale"]) in (int, float) and 0 < target["dpi_scale"] <= 8,
            "SCREEN_QA_POLICY")
    require(isinstance(target["logical_size"], list) and len(target["logical_size"]) == 2 and
            all(type(n) is int and n > 0 for n in target["logical_size"]), "SCREEN_QA_POLICY")
    require(all(isinstance(target[k], str) and target[k] for k in
                ("platform", "backend", "video_driver")), "SCREEN_QA_POLICY")
    require(target["video_driver"] in ("cocoa", "windows", "x11", "wayland"),
            "SCREEN_QA_NOT_REAL_DISPLAY")
    require(isinstance(policy.get("reference_sha256"), str) and
            len(policy["reference_sha256"]) == 64 and
            all(c in "0123456789abcdef" for c in policy["reference_sha256"]), "SCREEN_QA_POLICY")
    masks = policy.get("masks", [])
    require(isinstance(masks, list), "SCREEN_QA_POLICY")
    for mask in masks:
        require(isinstance(mask, dict) and isinstance(mask.get("name"), str) and mask["name"],
                "SCREEN_QA_POLICY")
        rectangle(mask.get("rect"), w, h)
    regions = policy.get("regions")
    require(isinstance(regions, list) and regions, "SCREEN_QA_POLICY")
    names = set()
    for region in regions:
        require(isinstance(region, dict) and isinstance(region.get("name"), str) and
                region["name"] not in names, "SCREEN_QA_POLICY")
        names.add(region["name"])
        x, y, rw, rh = rectangle(region.get("rect"), w, h)
        require(type(region.get("max_delta")) is int and 0 <= region["max_delta"] <= 16,
                "SCREEN_QA_POLICY")
        fraction = region.get("max_changed_fraction")
        require(type(fraction) in (float, int) and 0 <= fraction <= 0.01, "SCREEN_QA_POLICY")
        # Conservative sum of rectangle intersections; overlapping masks are
        # counted twice, never used to understate how much evidence is hidden.
        covered = sum(max(0, min(x + rw, mx + mw) - max(x, mx)) *
                      max(0, min(y + rh, my + mh) - max(y, my))
                      for mx, my, mw, mh in (mask["rect"] for mask in masks))
        require(covered / (rw * rh) <= 0.05, "SCREEN_QA_MASK_TOO_BROAD")
        if region["name"] == "full-client":
            require(region["rect"] == [0, 0, w, h], "SCREEN_QA_POLICY")
    required = {"full-client", "guest"}
    if target["status_visible"]:
        required.add("status")
    if target["panel_visible"]:
        required.add("panel")
    require(required <= names, "SCREEN_QA_REQUIRED_REGION")
    layout = [region["rect"] for region in regions if region["name"] in ("guest", "status", "panel")]
    for i, (x, y, rw, rh) in enumerate(layout):
        for mx, my, mw, mh in layout[i + 1:]:
            require(min(x + rw, mx + mw) <= max(x, mx) or
                    min(y + rh, my + mh) <= max(y, my), "SCREEN_QA_LAYOUT_OVERLAP")
    return policy


def load_policy(data, approved_digest):
    require(sha(data) == approved_digest, "SCREEN_QA_POLICY_BINDING")
    return validate_policy(json.loads(data))


def validate_capture(metadata, policy, actual_data):
    require(isinstance(metadata, dict) and metadata.get("capture_kind") == "final-client",
            "SCREEN_QA_CAPTURE_KIND")
    require(json.dumps(metadata.get("target"), sort_keys=True) ==
            json.dumps(policy["target"], sort_keys=True), "SCREEN_QA_TARGET_MISMATCH")
    require(metadata.get("image_sha256") == sha(actual_data), "SCREEN_QA_CAPTURE_BINDING")


def compare_pixels(reference, actual, policy):
    """Independent content layer; no capture-origin or Git-history dependency."""
    validate_policy(policy)
    w, h = policy["width"], policy["height"]
    require(reference[:2] == actual[:2] == (w, h), "SCREEN_QA_SIZE_MISMATCH")
    require(len(reference[2]) == len(actual[2]) == w * h * 4, "SCREEN_QA_PIXEL_BUFFER")
    masks, results = policy.get("masks", []), []
    diff = bytearray(w * h * 4)
    for i in range(0, len(diff), 4):
        changed = reference[2][i:i + 4] != actual[2][i:i + 4]
        diff[i:i + 4] = bytes((255 if changed else 0, 0, 0, 255))
    for region in policy["regions"]:
        x, y, rw, rh = region["rect"]
        changed, tested, peak = 0, 0, 0
        for py in range(y, y + rh):
            for px in range(x, x + rw):
                if any(contains(mask["rect"], px, py) for mask in masks):
                    continue
                i = (py * w + px) * 4
                delta = max(abs(a - b) for a, b in zip(reference[2][i:i + 4], actual[2][i:i + 4]))
                changed += delta > region["max_delta"]
                tested += 1
                peak = max(peak, delta)
        results.append({"name": region["name"], "tested_pixels": tested,
                        "changed_pixels": changed, "peak_delta": peak,
                        "changed_fraction": changed / tested,
                        "pass": changed / tested <= region["max_changed_fraction"]})
    return results, (w, h, bytes(diff))


def compare_images(reference_data, actual_data, policy):
    reference, actual = read_png(reference_data), read_png(actual_data)
    require(colour_profile(reference_data) == colour_profile(actual_data), "SCREEN_QA_COLOUR_PROFILE")
    return compare_pixels(reference, actual, policy)


def evaluate(reference_data, actual_data, policy, metadata):
    validate_policy(policy)
    require(sha(reference_data) == policy["reference_sha256"], "SCREEN_QA_REFERENCE_BINDING")
    validate_capture(metadata, policy, actual_data)
    metrics, diff = compare_images(reference_data, actual_data, policy)
    return {"contract": CONTRACT, "status": "PASS" if all(r["pass"] for r in metrics) else "FAIL",
            "error_code": None if all(r["pass"] for r in metrics) else "SCREEN_QA_PIXEL_MISMATCH",
            "regions": metrics}, diff


def selftest():
    image = (20, 20, bytes((10, 20, 30, 255)) * 400)
    reference = png_bytes(image)
    policy = {"contract": CONTRACT, "width": 20, "height": 20,
              "reference_sha256": sha(reference),
              "target": {"platform": "fixture", "backend": "fixture", "video_driver": "cocoa",
                         "dpi_scale": 1, "logical_size": [20, 20], "fullscreen": False,
                         "status_visible": True, "panel_visible": False},
              "masks": [{"name": "cursor", "rect": [0, 0, 1, 1]}],
              "regions": [{"name": name, "rect": rect, "max_delta": 0, "max_changed_fraction": 0}
                          for name, rect in [("full-client", [0, 0, 20, 20]),
                                             ("guest", [0, 0, 20, 10]), ("status", [0, 10, 20, 10])]]}
    metadata = {"capture_kind": "final-client", "target": copy.deepcopy(policy["target"]),
                "image_sha256": sha(reference)}
    require(read_png(reference) == image, "SCREEN_QA_SELFTEST")
    require(evaluate(reference, reference, policy, metadata)[0]["status"] == "PASS", "SCREEN_QA_SELFTEST")
    def rejects(action, expected):
        try:
            action()
        except QaError as error:
            require(error.code == expected, "SCREEN_QA_SELFTEST")
        else:
            raise QaError("SCREEN_QA_SELFTEST")
    policy_data = json.dumps(policy).encode()
    require(load_policy(policy_data, sha(policy_data)) == policy, "SCREEN_QA_SELFTEST")
    rejects(lambda: load_policy(policy_data, "0" * 64), "SCREEN_QA_POLICY_BINDING")
    rejects(lambda: read_png(reference[:-1]), "SCREEN_QA_PNG_INVALID")
    damaged = bytearray(reference)
    damaged[-1] ^= 1
    rejects(lambda: read_png(bytes(damaged)), "SCREEN_QA_PNG_INVALID")
    dummy_policy = copy.deepcopy(policy)
    dummy_policy["target"]["video_driver"] = "dummy"
    rejects(lambda: validate_policy(dummy_policy), "SCREEN_QA_NOT_REAL_DISPLAY")
    overlap_policy = copy.deepcopy(policy)
    overlap_policy["regions"][1]["rect"] = [0, 0, 20, 11]
    rejects(lambda: validate_policy(overlap_policy), "SCREEN_QA_LAYOUT_OVERLAP")
    broad_tolerance = copy.deepcopy(policy)
    broad_tolerance["regions"][0]["max_changed_fraction"] = 1
    rejects(lambda: validate_policy(broad_tolerance), "SCREEN_QA_POLICY")
    # Fixed, hand-computed RGB fixtures exercise every PNG row filter.
    expected_rgb = (2, 2, bytes((10, 20, 30, 255, 40, 50, 60, 255,
                                 15, 25, 35, 255, 45, 55, 65, 255)))
    filtered = [
        ([10, 20, 30, 40, 50, 60], [15, 25, 35, 45, 55, 65]),
        ([10, 20, 30, 30, 30, 30], [15, 25, 35, 30, 30, 30]),
        ([10, 20, 30, 40, 50, 60], [5, 5, 5, 5, 5, 5]),
        ([10, 20, 30, 35, 40, 45], [10, 15, 20, 18, 18, 18]),
        ([10, 20, 30, 30, 30, 30], [5, 5, 5, 5, 5, 5]),
    ]
    def chunk(tag, body):
        return struct.pack(">I", len(body)) + tag + body + struct.pack(">I", zlib.crc32(tag + body))
    for method, (first, second) in enumerate(filtered):
        raw = bytes([method, *first, method, *second])
        encoded = (b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", 2, 2, 8, 2, 0, 0, 0)) +
                   chunk(b"IDAT", zlib.compress(raw)) + chunk(b"IEND", b""))
        require(read_png(encoded) == expected_rgb, "SCREEN_QA_SELFTEST")
    gamma_changed = reference[:33] + chunk(b"gAMA", struct.pack(">I", 100000)) + reference[33:]
    rejects(lambda: compare_images(reference, gamma_changed, policy), "SCREEN_QA_COLOUR_PROFILE")
    negatives = [
        ("reference_sha256", "0" * 64, "SCREEN_QA_REFERENCE_BINDING"),
        ("width", 21, "SCREEN_QA_POLICY"),
        ("masks", [{"name": "cursor", "rect": [0, 0, 20, 20]}], "SCREEN_QA_MASK_TOO_BROAD"),
        ("regions", policy["regions"][:-1], "SCREEN_QA_REQUIRED_REGION"),
    ]
    for key, value, expected in negatives:
        # Each mutation starts from a fixture passing all composed checks.
        mutated = copy.deepcopy(policy)
        mutated[key] = value
        try:
            evaluate(reference, reference, mutated, metadata)
        except QaError as error:
            require(error.code == expected, "SCREEN_QA_SELFTEST")
        else:
            raise QaError("SCREEN_QA_SELFTEST")
    for key, value, expected in [
        ("capture_kind", "guest-only", "SCREEN_QA_CAPTURE_KIND"),
        ("target", {**metadata["target"], "video_driver": "dummy"}, "SCREEN_QA_TARGET_MISMATCH"),
        ("target", {**metadata["target"], "fullscreen": 0}, "SCREEN_QA_TARGET_MISMATCH"),
        ("image_sha256", "0" * 64, "SCREEN_QA_CAPTURE_BINDING"),
    ]:
        mutated = copy.deepcopy(metadata)
        mutated[key] = value
        try:
            validate_capture(mutated, policy, reference)
        except QaError as error:
            require(error.code == expected, "SCREEN_QA_SELFTEST")
        else:
            raise QaError("SCREEN_QA_SELFTEST")
    pixels = bytearray(image[2])
    pixels[0] = 255
    require(all(r["pass"] for r in compare_pixels(image, (20, 20, bytes(pixels)), policy)[0]),
            "SCREEN_QA_SELFTEST")
    pixels = bytearray(image[2])
    pixels[(15 * 20 + 5) * 4] = 255
    require(not all(r["pass"] for r in compare_pixels(image, (20, 20, bytes(pixels)), policy)[0]),
            "SCREEN_QA_SELFTEST")
    try:
        compare_pixels(image, (21, 20, bytes((10, 20, 30, 255)) * 420), policy)
    except QaError as error:
        require(error.code == "SCREEN_QA_SIZE_MISMATCH", "SCREEN_QA_SELFTEST")
    else:
        raise QaError("SCREEN_QA_SELFTEST")
    # Final integration: compose PNG, policy binding, provenance binding and pixels.
    actual = png_bytes((20, 20, bytes(pixels)))
    report, diff = evaluate(reference, actual, policy, {**metadata, "image_sha256": sha(actual)})
    require(report["status"] == "FAIL" and report["error_code"] == "SCREEN_QA_PIXEL_MISMATCH",
            "SCREEN_QA_SELFTEST")
    require(read_png(png_bytes(diff)) == diff, "SCREEN_QA_SELFTEST")
    # File/CLI integration uses only synthetic images and an isolated directory.
    with tempfile.TemporaryDirectory(prefix="vaeg-screen-qa-") as directory:
        root = Path(directory)
        files = {"reference": reference, "actual": reference,
                 "policy": json.dumps(policy).encode(),
                 "capture-metadata": json.dumps(metadata).encode()}
        arguments = []
        for name, data in files.items():
            path = root / name
            path.write_bytes(data)
            arguments.extend(["--" + name, str(path)])
        arguments.extend(["--policy-sha256", sha(files["policy"]),
                          "--output-dir", str(root / "result")])
        require(main(arguments) == 0, "SCREEN_QA_SELFTEST")
        saved = json.loads((root / "result/result.json").read_text())
        require(saved["status"] == "PASS" and saved["diff_sha256"] ==
                sha((root / "result/diff.png").read_bytes()), "SCREEN_QA_SELFTEST")
    print("PASS: synthetic pixel, mask, dimension, binding and composed mismatch tests")


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--selftest", action="store_true")
    for name in ("reference", "actual", "policy", "capture-metadata", "output-dir"):
        parser.add_argument("--" + name, type=Path)
    parser.add_argument("--policy-sha256", help="previously reviewed/frozen private policy digest")
    args = parser.parse_args(argv)
    try:
        if args.selftest:
            selftest()
            return 0
        require(all((args.reference, args.actual, args.policy, args.capture_metadata,
                     args.output_dir, args.policy_sha256)), "SCREEN_QA_ARGUMENTS")
        output = args.output_dir.resolve()
        require(output != ROOT and ROOT not in output.parents and not output.exists(),
                "SCREEN_QA_OUTPUT_MUST_BE_NEW_AND_OUT_OF_TREE")
        policy_data = args.policy.read_bytes()
        policy = load_policy(policy_data, args.policy_sha256)
        reference, actual = args.reference.read_bytes(), args.actual.read_bytes()
        metadata_data = args.capture_metadata.read_bytes()
        report, diff = evaluate(reference, actual, policy, json.loads(metadata_data))
        output.mkdir(parents=True)
        diff_data = png_bytes(diff)
        (output / "diff.png").write_bytes(diff_data)
        report.update(tool_sha256=sha(Path(__file__).read_bytes()),
                      policy_sha256=sha(policy_data), reference_sha256=sha(reference),
                      actual_sha256=sha(actual), metadata_sha256=sha(metadata_data),
                      diff_sha256=sha(diff_data),
                      provenance="caller metadata binding; origin requires a trusted capture adapter")
        (output / "result.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
        print(json.dumps({"status": report["status"], "error_code": report["error_code"]}))
        return 0 if report["status"] == "PASS" else 1
    except (QaError, OSError, ValueError, TypeError) as error:
        print(json.dumps({"status": "FAIL", "error_code": error.code if isinstance(error, QaError)
                          else "SCREEN_QA_INPUT_INVALID"}))
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
