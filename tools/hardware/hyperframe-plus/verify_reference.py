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
"""Read-only input validation. Byte matches do NOT establish control flow."""
import argparse
import hashlib
import json
import shutil
import struct
import subprocess
from pathlib import Path

EXPECTED = {
    "jpgvhf1e.lzh": "527d0f162c80f5f8a963ee6f3c6da89168a62eec9436345683ac6ba49f3b2476",
    "jpgvhf.doc": "dc89dda016e6b687eef95593dc58cc225269a20fda3b96a1bd4227ae8f993324",
    "jpgvhf.exe": "f2f845543be983a9534eedea0b22a4647dcf685dfd5f0372e13a16acd11647ab",
}
# These exact bytes were supplied in the task, not discovered by this tool.
LANDMARKS = {0x563F: bytes.fromhex("BA EE 00 B0 C0 EE"),
             0x4056: bytes.fromhex("E7 D6")}
WINDOWS = (0x5645, 0x564B, 0x56AA, 0x56CF, 0x581D, 0x40A5, 0x54A7)


class ValidationError(ValueError):
    def __init__(self, code):
        self.code = code
        super().__init__(code)


def require(condition, code):
    if not condition:
        raise ValidationError(code)


def sha256(data):
    return hashlib.sha256(data).hexdigest()


def verify_hash(data, expected):
    require(sha256(data) == expected, "HFRE_HASH_MISMATCH")


def mz_metadata(data):
    require(len(data) >= 28, "HFRE_MZ_TRUNCATED")
    require(data[:2] == b"MZ", "HFRE_MZ_MAGIC")
    fields = struct.unpack_from("<13H", data, 2)
    last, pages, relocations, paragraphs, minimum, maximum, ss, sp, checksum, ip, cs, table, overlay = fields
    require(pages > 0 and last < 512, "HFRE_MZ_PAGE_COUNT")
    declared = (pages - 1) * 512 + (last or 512)
    header = paragraphs * 16
    require(28 <= header < declared <= len(data), "HFRE_MZ_FILE_SIZE")
    require(table >= 28 and table + relocations * 4 <= header,
            "HFRE_MZ_RELOCATION_RANGE")
    load = data[header:declared]
    require(cs * 16 + ip < len(load), "HFRE_MZ_ENTRY_RANGE")
    entries = []
    for index in range(relocations):
        offset, segment = struct.unpack_from("<HH", data, table + index * 4)
        target = segment * 16 + offset
        require(target + 2 <= len(load), "HFRE_MZ_RELOCATION_TARGET")
        entries.append({"segment": segment, "offset": offset, "image_offset": target})
    return {"header_bytes": header, "header_paragraphs": paragraphs,
            "declared_bytes": declared, "load_bytes": len(load),
            "trailing_bytes": len(data) - declared, "relocation_table": table,
            "relocations": entries, "entry_cs": cs, "entry_ip": ip,
            "initial_ss": ss, "initial_sp": sp, "minimum_extra_paragraphs": minimum,
            "maximum_extra_paragraphs": maximum, "checksum": checksum,
            "overlay": overlay}


def verify_landmarks(load, landmarks):
    for offset, expected in landmarks.items():
        require(load[offset:offset + len(expected)] == expected,
                "HFRE_LANDMARK_BYTES")
    return [{"image_offset": offset, "bytes": expected.hex(" "),
             "classification": "BYTE_MATCH_ONLY; CONTROL_FLOW_NOT_VERIFIED"}
            for offset, expected in landmarks.items()]


def analyze_executable(data, expected_hash, header_paragraphs, landmarks):
    """Compose independently testable provenance, structure and byte checks."""
    verify_hash(data, expected_hash)
    mz = mz_metadata(data)
    require(mz["header_paragraphs"] == header_paragraphs, "HFRE_HEADER_LAYOUT")
    load = data[mz["header_bytes"]:mz["declared_bytes"]]
    return {"mz": mz, "landmarks": verify_landmarks(load, landmarks)}, load


def collect(directory):
    files = {}
    if directory.is_dir():
        for path in directory.iterdir():
            if path.is_file() and path.name.lower() in EXPECTED:
                require(path.name.lower() not in files, "HFRE_AMBIGUOUS_INPUT")
                files[path.name.lower()] = path.read_bytes()
    for name, data in files.items():
        verify_hash(data, EXPECTED[name])
    if "jpgvhf1e.lzh" in files:
        require(files["jpgvhf1e.lzh"][2:7] == b"-lh5-", "HFRE_ARCHIVE_METHOD")
        tool = shutil.which("bsdtar")
        require(tool is not None, "HFRE_EXTRACTOR_MISSING")
        archive = next(p for p in directory.iterdir() if p.name.lower() == "jpgvhf1e.lzh")
        listing = subprocess.check_output([tool, "-tf", str(archive)], text=True).splitlines()
        for name in ("jpgvhf.exe", "jpgvhf.doc"):
            matches = [p for p in listing if p.lower() == name]
            require(len(matches) == 1, "HFRE_ARCHIVE_MEMBERS")
            data = subprocess.check_output([tool, "-xOf", str(archive), matches[0]])
            verify_hash(data, EXPECTED[name])
            require(name not in files or files[name] == data, "HFRE_INPUT_DISAGREEMENT")
            files[name] = data
    require("jpgvhf.exe" in files, "HFRE_INPUT_MISSING")
    return files


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--references", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True,
                        help="New directory outside the checkout for private evidence")
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[3]
    output = args.output.resolve()
    require(output != root and root not in output.parents, "HFRE_OUTPUT_IN_CHECKOUT")
    require(not output.exists(), "HFRE_OUTPUT_EXISTS")
    output.mkdir(parents=True)
    try:
        files = collect(args.references)
        report, load = analyze_executable(files["jpgvhf.exe"], EXPECTED["jpgvhf.exe"],
                                          0x20, LANDMARKS)
        report.update(status="BYTE_CHECK_PASS", control_flow="NOT_REVIEWED",
                      inputs={name: {"bytes": len(data), "sha256": sha256(data)}
                              for name, data in files.items()},
                      windows=[{"image_offset": o, "file_offset": o + 0x200,
                                "bytes": load[o:o + 32].hex(" ")} for o in WINDOWS])
        (output / "load.bin").write_bytes(load)
        if "jpgvhf.doc" in files:
            text = files["jpgvhf.doc"].decode("cp932")
            (output / "document.utf8.txt").write_text(text, encoding="utf-8")
        disassembler = shutil.which("ndisasm")
        if disassembler:
            with (output / "linear-disassembly.txt").open("wb") as stream:
                subprocess.run([disassembler, "-b", "16", str(output / "load.bin")],
                               stdout=stream, check=True)
        code = 0
    except ValidationError as error:
        report = {"status": "BLOCKED" if error.code == "HFRE_INPUT_MISSING" else "FAIL",
                  "code": error.code, "control_flow": "NOT_REVIEWED"}
        code = 2 if report["status"] == "BLOCKED" else 1
    (output / "report.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"status": report["status"], "code": report.get("code", "HFRE_BYTES_ONLY")}))
    return code


if __name__ == "__main__":
    raise SystemExit(main())
