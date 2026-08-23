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

"""Validate the captured GLASS ORBIT GA-1 bare-payload register contract."""

import argparse
import json
from pathlib import Path


EXPECTED = {
    "schema": "vaeg-registers-v1",
    "ax": "4741",
    "cs": "2000",
    "ds": "2000",
    "es": "2000",
    "ss": "2000",
    "sp": "f000",
    "ip": "0100",
}


def read_tsv(path: Path) -> dict[str, str]:
    values = {}
    for line in path.read_text(encoding="utf-8").splitlines():
        key, separator, value = line.partition("\t")
        if not separator or key in values:
            raise ValueError("GA1_CAPTURE_SCHEMA")
        values[key] = value
    return values


def validate(capture_dir: Path) -> dict[str, object]:
    errors = []
    registers_path = capture_dir / "ga1-idle.registers.tsv"
    screen_path = capture_dir / "ga1-idle.screen.bmp"
    events_path = capture_dir / "events.tsv"
    if not registers_path.is_file():
        errors.append("GA1_REGISTERS_MISSING")
        values = {}
    else:
        try:
            values = read_tsv(registers_path)
        except (OSError, UnicodeError, ValueError):
            values = {}
            errors.append("GA1_CAPTURE_SCHEMA")
    for key, expected in EXPECTED.items():
        if values.get(key) != expected:
            errors.append(f"GA1_{key.upper()}_MISMATCH")
    try:
        flags = int(values.get("flags", ""), 16)
    except ValueError:
        errors.append("GA1_FLAGS_INVALID")
    else:
        if flags & 0x0400:
            errors.append("GA1_DIRECTION_FLAG_SET")
        if not flags & 0x0200:
            errors.append("GA1_INTERRUPTS_DISABLED")
    if not screen_path.is_file() or screen_path.stat().st_size == 0:
        errors.append("GA1_SCREEN_MISSING")
    if not events_path.is_file():
        errors.append("GA1_EVENTS_MISSING")
    elif not any(line.startswith("pc\t") for line in events_path.read_text(encoding="utf-8").splitlines()):
        errors.append("GA1_IDLE_NOT_REACHED")
    return {
        "schema": "glass-orbit-ga1-v1",
        "status": "PASS" if not errors else "FAIL",
        "errors": errors,
        "registers": {key: values.get(key) for key in EXPECTED},
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("capture_dir", type=Path)
    args = parser.parse_args()
    try:
        result = validate(args.capture_dir)
    except OSError as error:
        result = {"schema": "glass-orbit-ga1-v1", "status": "FAIL", "errors": [str(error)]}
    print(json.dumps(result, sort_keys=True))
    return 0 if result["status"] == "PASS" else 1


if __name__ == "__main__":
    raise SystemExit(main())
