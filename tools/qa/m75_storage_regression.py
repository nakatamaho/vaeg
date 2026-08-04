#!/usr/bin/env python3
"""Prepare and run the M75 SASI/HOSTFAT storage regression scenario."""

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

from __future__ import annotations

import argparse
import hashlib
import json
import os
import re
import struct
import subprocess
import sys
import tempfile
from pathlib import Path


SASI_HEADER_SIZE = 4096
SASI_CYLINDERS = 615
SASI_SURFACES = 8
SASI_SECTORS = 33
SASI_SECTOR_SIZE = 256
SASI_BLOCK_COUNT = SASI_CYLINDERS * SASI_SURFACES * SASI_SECTORS
SASI_DATA_SIZE = SASI_BLOCK_COUNT * SASI_SECTOR_SIZE
SASI_FILE_SIZE = SASI_HEADER_SIZE + SASI_DATA_SIZE
SASI_MARKER = b"M75 SASI REGRESSION\n"


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def create_sasi_image(path: Path) -> dict[str, int | str]:
    """Create the same 40MB HDI geometry used by newdisk_hdi()."""
    header = bytearray(SASI_HEADER_SIZE)
    struct.pack_into("<I", header, 8, SASI_HEADER_SIZE)
    struct.pack_into("<I", header, 16, SASI_SECTOR_SIZE)
    struct.pack_into("<I", header, 20, SASI_SECTORS)
    struct.pack_into("<I", header, 24, SASI_SURFACES)
    struct.pack_into("<I", header, 28, SASI_CYLINDERS)
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("wb") as stream:
        stream.write(header)
        stream.write(SASI_MARKER)
        stream.truncate(SASI_FILE_SIZE)
    return {
        "path": str(path),
        "header_bytes": SASI_HEADER_SIZE,
        "block_size": SASI_SECTOR_SIZE,
        "blocks": SASI_BLOCK_COUNT,
        "size": SASI_FILE_SIZE,
        "sha256": sha256(path),
    }


def create_hostfat_root(path: Path) -> dict[str, int | str]:
    path.mkdir(parents=True, exist_ok=True)
    (path / "REGRESS.TXT").write_text(
        "M75 STORAGE REGRESSION\n", encoding="ascii")
    (path / "SUBDIR").mkdir()
    (path / "SUBDIR" / "DATA.BIN").write_bytes(bytes(range(32)))
    files = sorted(item for item in path.rglob("*") if item.is_file())
    directories = sorted(item for item in path.rglob("*") if item.is_dir())
    return {
        "path": str(path),
        "files": len(files),
        "directories": len(directories),
        "source_bytes": sum(item.stat().st_size for item in files),
    }


def fixture_selftest() -> dict[str, object]:
    with tempfile.TemporaryDirectory(prefix="m75-storage-fixture-") as temporary:
        root = Path(temporary)
        sasi = create_sasi_image(root / "sasi.hdi")
        hostfat = create_hostfat_root(root / "hostfat")
        with (root / "sasi.hdi").open("rb") as stream:
            stream.seek(SASI_HEADER_SIZE)
            marker = stream.read(len(SASI_MARKER))
        if marker != SASI_MARKER:
            raise AssertionError("SASI marker was not written")
        if (root / "sasi.hdi").stat().st_size != SASI_FILE_SIZE:
            raise AssertionError("SASI image size is not deterministic")
        if hostfat != {
                "path": str(root / "hostfat"),
                "files": 2,
                "directories": 1,
                "source_bytes": 55,
        }:
            raise AssertionError(f"unexpected HOSTFAT fixture: {hostfat}")
        return {"sasi": sasi, "hostfat": hostfat}


def run_guest(args: argparse.Namespace) -> dict[str, object]:
    if not Path(args.worker).is_file():
        raise FileNotFoundError(f"worker not found: {args.worker}")
    for label, value in (("support D88", args.support_d88),
                         ("ROM directory", args.roms)):
        if not Path(value).exists():
            raise FileNotFoundError(f"{label} not found: {value}")
    support_bytes = Path(args.support_d88).read_bytes()
    if b"HOSTFAT" not in support_bytes.upper():
        raise RuntimeError(
            "support D88 does not contain HOSTFAT.SYS; provide the "
            "PC-Engine support disk with DEVICE=HOSTFAT.SYS")

    harness = Path(__file__).with_name("m75_scsi_harness.py")
    output_root = (Path(args.output_dir).resolve()
                   if args.output_dir else
                   Path(tempfile.mkdtemp(prefix="m75-storage-regression-")))
    output_root.mkdir(parents=True, exist_ok=True)
    sasi_path = output_root / "sasi.hdi"
    hostfat_path = output_root / "hostfat"
    screen_path = output_root / "screen.bin"
    trace_path = output_root / "trace.log"
    sasi = create_sasi_image(sasi_path)
    hostfat = create_hostfat_root(hostfat_path)

    worker_args = [
        "--model", args.model,
        "--roms", str(Path(args.roms).resolve()),
        "--fdd1", str(Path(args.support_d88).resolve()),
        "--fdd2", "none",
        "--sasi1", str(sasi_path),
        "--sasi2", "none",
        "--hostfat-dir", str(hostfat_path),
        "--nowait", "--mute",
    ]
    if args.headless_input_script:
        worker_args.extend(["--headless-input-script",
                            str(Path(args.headless_input_script).resolve())])
    command = [
        sys.executable, str(harness),
        "--worker", str(Path(args.worker).resolve()),
        "--screen-out", str(screen_path),
        "--trace-out", str(trace_path),
        "--timeout", str(args.timeout),
        "--", *worker_args,
    ]
    environment = os.environ.copy()
    environment["VAEG_SCREEN_EXIT_MS"] = str(args.exit_ms)
    completed = subprocess.run(command, check=False, text=True,
                               capture_output=True, env=environment)
    if completed.stdout.strip():
        try:
            harness_result = json.loads(completed.stdout)
        except json.JSONDecodeError as error:
            raise RuntimeError(
                f"harness did not return JSON: {completed.stdout!r}") from error
    else:
        harness_result = {"stdout": completed.stdout}
    if not trace_path.exists():
        raise RuntimeError(
            f"harness produced no trace (exit {completed.returncode}): "
            f"{completed.stderr.strip()}")
    trace = trace_path.read_text(encoding="utf-8", errors="replace")
    if not re.search(r"INFO: SCSI mount path=.*sasi\.hdi .*block_size=256",
                     trace):
        raise AssertionError("SASI mount record is missing from the trace")
    if "HOSTFAT: read-only snapshot ready: 2 files, 1 directories" not in trace:
        raise AssertionError("HOSTFAT snapshot mount record is missing")
    if completed.returncode != 0:
        raise RuntimeError(
            f"storage scenario failed with exit {completed.returncode}: "
            f"{completed.stderr.strip()}")
    if not screen_path.exists():
        raise RuntimeError("storage scenario produced no screen capture")
    return {
        "command": command,
        "output_dir": str(output_root),
        "sasi": sasi,
        "hostfat": hostfat,
        "harness": harness_result,
        "trace_sha256": sha256(trace_path),
        "screen_sha256": sha256(screen_path),
    }


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(
        description="Run the disposable M75 SASI/HOSTFAT regression.")
    parser.add_argument("--selftest", action="store_true",
                        help="validate only the generated disposable fixtures")
    parser.add_argument("--worker")
    parser.add_argument("--support-d88")
    parser.add_argument("--roms")
    parser.add_argument("--model", default="va2", choices=("va", "va2"))
    parser.add_argument("--output-dir",
                        help="retain fixtures and same-run screen/trace here")
    parser.add_argument("--headless-input-script")
    parser.add_argument("--timeout", type=float, default=180.0)
    parser.add_argument("--exit-ms", type=int, default=30000,
                        help="guest screen-harness exit delay (default: 30000)")
    args = parser.parse_args(argv)
    if args.selftest:
        print(json.dumps(fixture_selftest(), indent=2, sort_keys=True))
        return 0
    missing = [name for name in ("--worker", "--support-d88", "--roms")
               if getattr(args, name[2:].replace("-", "_")) is None]
    if missing:
        parser.error("guest run requires " + ", ".join(missing))
    print(json.dumps(run_guest(args), ensure_ascii=False, indent=2,
                         sort_keys=True))
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (AssertionError, FileNotFoundError, RuntimeError) as error:
        print(f"M75_STORAGE_REGRESSION_FAIL: {error}", file=sys.stderr)
        raise SystemExit(1)
