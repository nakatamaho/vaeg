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
# EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
# OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
# OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
# OF THE POSSIBILITY OF SUCH DAMAGE.

"""Prove that fetched ZEX inputs are absent from an archive."""

import argparse
import hashlib
from pathlib import Path, PurePosixPath
import tarfile
from typing import List, Tuple
import zipfile


PROHIBITED_NAMES = {
    "zexdoc.cim",
    "zexall.cim",
    "zexdoc.src",
    "zexall.src",
    "zex-license.txt",
}
PROHIBITED_HASHES = {
    "b3015112a99bb72273e0cacde7c7549eb9840ba996af76f7bf7992ef7d6e2f90",
    "fbb1bb5d46f61c33ea6841a71f2b23c49b9b62410ce6ed4e57b7d9b2e7b437e0",
    "0e2e7d05e5dd27c932de64d4c3711351f53388ed02d2e99e2e706ef6216ca9b3",
    "a263efc67ed6f890268c6f9e00f7911d9376a6bc6ddaec5ce04e33a5f483733c",
    "e57f1c320b8cf8798a7d2ff83a6f9e06a33a03585f6e065fea97f1d86db84052",
}


def inspect_member(name: str, data: bytes, violations: List[str]) -> None:
    basename = PurePosixPath(name.replace("\\", "/")).name.lower()
    digest = hashlib.sha256(data).hexdigest()
    if basename in PROHIBITED_NAMES:
        violations.append(f"prohibited ZEX artifact name: {name}")
    if digest in PROHIBITED_HASHES:
        violations.append(f"prohibited ZEX artifact content: {name} ({digest})")


def inspect_tar(path: Path) -> Tuple[int, List[str]]:
    count = 0
    violations: List[str] = []
    with tarfile.open(path, "r:*") as archive:
        for member in archive.getmembers():
            if not member.isfile():
                continue
            extracted = archive.extractfile(member)
            if extracted is None:
                continue
            inspect_member(member.name, extracted.read(), violations)
            count += 1
    return count, violations


def inspect_zip(path: Path) -> Tuple[int, List[str]]:
    count = 0
    violations: List[str] = []
    with zipfile.ZipFile(path) as archive:
        for member in archive.infolist():
            if member.is_dir():
                continue
            inspect_member(member.filename, archive.read(member), violations)
            count += 1
    return count, violations


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Reject fetched ZEX inputs from source/release archives"
    )
    parser.add_argument("archives", type=Path, nargs="+")
    args = parser.parse_args()

    failed = False
    for path in args.archives:
        try:
            if zipfile.is_zipfile(path):
                count, violations = inspect_zip(path)
            elif tarfile.is_tarfile(path):
                count, violations = inspect_tar(path)
            else:
                raise ValueError("not a supported tar or zip archive")
        except (OSError, ValueError, tarfile.TarError, zipfile.BadZipFile) as error:
            print(f"FAIL {path}: {error}")
            failed = True
            continue
        if violations:
            print(f"FAIL {path}:")
            for violation in violations:
                print(f"  {violation}")
            failed = True
        else:
            print(f"PASS {path}: checked {count} files; no ZEX artifacts")
    return 1 if failed else 0


if __name__ == "__main__":
    raise SystemExit(main())
