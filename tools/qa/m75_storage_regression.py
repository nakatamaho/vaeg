#!/usr/bin/env python3
"""Run the disposable M75 SASI/HOSTFAT storage regressions."""

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
HOSTFAT_FILE = "REGRESS.TXT"
HOSTFAT_CONTENT = "M75 STORAGE REGRESSION\n"


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def directory_manifest(path: Path) -> dict[str, object]:
    entries = []
    for item in sorted(path.rglob("*")):
        relative = item.relative_to(path).as_posix()
        if item.is_dir():
            entries.append({"path": relative, "type": "directory"})
        elif item.is_file():
            entries.append({
                "path": relative,
                "type": "file",
                "size": item.stat().st_size,
                "sha256": sha256(item),
            })
        else:
            raise AssertionError(f"unexpected HOSTFAT fixture entry: {item}")
    encoded = json.dumps(entries, sort_keys=True, separators=(",", ":"))
    return {
        "entries": entries,
        "sha256": hashlib.sha256(encoded.encode("utf-8")).hexdigest(),
    }


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


def create_hostfat_root(path: Path) -> dict[str, int | str | object]:
    path.mkdir(parents=True, exist_ok=True)
    (path / HOSTFAT_FILE).write_text(HOSTFAT_CONTENT, encoding="ascii")
    (path / "SUBDIR").mkdir()
    (path / "SUBDIR" / "DATA.BIN").write_bytes(bytes(range(32)))
    files = sorted(item for item in path.rglob("*") if item.is_file())
    directories = sorted(item for item in path.rglob("*") if item.is_dir())
    manifest = directory_manifest(path)
    return {
        "path": str(path),
        "files": len(files),
        "directories": len(directories),
        "source_bytes": sum(item.stat().st_size for item in files),
        "manifest": manifest,
    }


def guest_input_lines(command: str, drive: str) -> list[str]:
    drive = drive.upper()
    if not re.fullmatch(r"[A-Z]", drive):
        raise ValueError(f"invalid HOSTFAT drive letter: {drive}")
    return [
        "@wait 600",
        f"{command} {drive}:\\{HOSTFAT_FILE}",
        "@wait 120",
    ]


def delete_input_lines(drive: str) -> list[str]:
    drive = drive.upper()
    if not re.fullmatch(r"[A-Z]", drive):
        raise ValueError(f"invalid HOSTFAT drive letter: {drive}")
    return [
        "@wait 600",
        f"DEL {drive}:\\{HOSTFAT_FILE}",
        "@wait 120",
        f"DIR {drive}:",
        "@wait 120",
    ]


def write_guest_input(path: Path, lines: list[str]) -> None:
    path.write_text("\n".join(lines) + "\n", encoding="ascii")


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
        if hostfat["files"] != 2 or hostfat["directories"] != 1:
            raise AssertionError(f"unexpected HOSTFAT fixture: {hostfat}")
        if guest_input_lines("TYPE", "D") != [
                "@wait 600", "TYPE D:\\REGRESS.TXT", "@wait 120"]:
            raise AssertionError("read input script is not deterministic")
        if delete_input_lines("D")[-2] != "DIR D:":
            raise AssertionError("delete input script lacks post-delete DIR")
        return {"sasi": sasi, "hostfat": hostfat}


def validate_guest_inputs(args: argparse.Namespace) -> None:
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
    if not re.fullmatch(r"[A-Za-z]", args.hostfat_drive):
        raise ValueError("--hostfat-drive must be one ASCII drive letter")


def run_guest(args: argparse.Namespace, output_root: Path,
              input_lines: list[str] | None = None,
              expected_screen: tuple[str, ...] = ()) -> dict[str, object]:
    output_root.mkdir(parents=True, exist_ok=True)
    harness = Path(__file__).with_name("m75_scsi_harness.py")
    sasi_path = output_root / "sasi.hdi"
    hostfat_path = output_root / "hostfat"
    screen_path = output_root / "screen.bin"
    trace_path = output_root / "trace.log"
    input_path = output_root / "headless-input.txt"
    sasi = create_sasi_image(sasi_path)
    hostfat = create_hostfat_root(hostfat_path)
    sasi_before = sasi["sha256"]
    hostfat_before = hostfat["manifest"]["sha256"]
    if input_lines is not None:
        write_guest_input(input_path, input_lines)

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
    if input_lines is not None:
        worker_args.extend(["--headless-input-script", str(input_path)])
    elif args.headless_input_script:
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
    screen = harness_result.get("screen", {})
    screen_lines = screen.get("lines", [])
    run_id = harness_result.get("run_id")
    if not run_id or screen.get("run_id") != run_id or run_id not in trace:
        raise AssertionError("screen and trace are not from the same run")
    if harness_result.get("termination") != "process-exit":
        raise AssertionError("scenario did not end by process exit")
    for expected in expected_screen:
        if not any(expected in line for line in screen_lines):
            raise AssertionError(
                f"screen is missing expected text {expected!r}: {screen_lines}")
    sasi_after = sha256(sasi_path)
    hostfat_after = directory_manifest(hostfat_path)["sha256"]
    if sasi_after != sasi_before:
        raise AssertionError("SASI image changed during read-only scenario")
    if hostfat_after != hostfat_before:
        raise AssertionError("HOSTFAT source directory changed during scenario")
    result = {
        "command": command,
        "output_dir": str(output_root),
        "input_script": str(input_path) if input_lines is not None else None,
        "input_lines": input_lines,
        "sasi": {**sasi, "sha256_before": sasi_before,
                 "sha256_after": sasi_after},
        "hostfat": {**hostfat, "manifest_sha256_before": hostfat_before,
                    "manifest_sha256_after": hostfat_after},
        "screen_lines": screen_lines,
        "harness": harness_result,
        "trace_sha256": sha256(trace_path),
        "screen_sha256": sha256(screen_path),
    }
    return result


def run_guest_io(args: argparse.Namespace) -> dict[str, object]:
    base = (Path(args.output_dir).resolve()
            if args.output_dir else
            Path(tempfile.mkdtemp(prefix="m75-storage-guest-io-")))
    base.mkdir(parents=True, exist_ok=True)
    read_root = base / "read"
    delete_root = base / "delete-rejected"
    if read_root.exists() or delete_root.exists():
        raise FileExistsError(
            f"guest-io output already contains read/delete directories: {base}")
    drive = args.hostfat_drive.upper()
    read = run_guest(
        args, read_root, guest_input_lines("TYPE", drive),
        (f"TYPE {drive}:\\{HOSTFAT_FILE}", HOSTFAT_CONTENT.rstrip("\n")))
    delete = run_guest(
        args, delete_root, delete_input_lines(drive),
        (f"DEL {drive}:\\{HOSTFAT_FILE}", "削除できません", HOSTFAT_FILE))
    return {
        "mode": "guest-io",
        "output_dir": str(base),
        "hostfat_drive": drive,
        "read": read,
        "delete_rejected": delete,
    }


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(
        description="Run the disposable M75 SASI/HOSTFAT storage regressions.")
    parser.add_argument("--selftest", action="store_true",
                        help="validate generated fixtures and input scripts")
    parser.add_argument("--guest-io", action="store_true",
                        help="run TYPE success and DEL rejection guest scenarios")
    parser.add_argument("--worker")
    parser.add_argument("--support-d88")
    parser.add_argument("--roms")
    parser.add_argument("--model", default="va2", choices=("va", "va2"))
    parser.add_argument("--output-dir",
                        help="retain fixtures and same-run screen/trace pairs")
    parser.add_argument("--headless-input-script")
    parser.add_argument("--hostfat-drive", default="D",
                        help="DOS drive letter assigned to HOSTFAT (default: D)")
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
    validate_guest_inputs(args)
    if args.guest_io:
        if args.headless_input_script:
            parser.error("--guest-io generates its own headless input scripts")
        result = run_guest_io(args)
    else:
        output_root = (Path(args.output_dir).resolve()
                       if args.output_dir else
                       Path(tempfile.mkdtemp(prefix="m75-storage-regression-")))
        result = run_guest(args, output_root)
    print(json.dumps(result, ensure_ascii=False, indent=2, sort_keys=True))
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (AssertionError, FileExistsError, FileNotFoundError,
            RuntimeError, ValueError) as error:
        print(f"M75_STORAGE_REGRESSION_FAIL: {error}", file=sys.stderr)
        raise SystemExit(1)
