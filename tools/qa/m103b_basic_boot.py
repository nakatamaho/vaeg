#!/usr/bin/env python3
# Copyright (c) 2026 Nakata Maho
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
# 1. Redistributions of source code must retain the above copyright notice,
#    this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR IMPLIED
# WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
# EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
# DAMAGE.

"""M103b: check that V2 disk BASIC reaches its key-input wait loop.

The run mode boots maintainer-local VA2 ROMs and V2 BASIC media without any
mode override, skips the early compatible-mode trace, and classifies a late
trace window. Private ROMs and media are never copied into the tree; the media
is copied to a temporary directory before use.
"""

from __future__ import annotations

import argparse
import os
import pathlib
import re
import shutil
import subprocess
import sys
import tempfile
from typing import Iterable, List, Optional, Tuple

SKIP_RETURN_CODE = 77

# Saved native frame of BRKEM2 90h at VA2 ROM F000:13B1 (three-byte opcode).
ENTRY_FRAME_PREFIX = "13b4/f000/"
BASIC_SEGMENT = "1000"
# N88-BASIC key-input wait: DI, key-buffer head/tail compare at 45F8h, EI,
# branch back when empty, plus the cursor routine at 7780h. Observed as an
# identical 38-address set in windows after 3, 10 and 30 million compatible
# instructions (plan section 5 and the M103b task file).
KEY_WAIT_IPS = frozenset(
    int(value, 16)
    for value in (
        "3593 3596 3599 359a 359b 359e 35a1 35a2 35a8 35ab 35ac 35ad "
        "45f8 45fb 45fd 45fe 45ff 4600 4676 4679 467a 467b 467c 467d 467e "
        "467f 4680 4681 4682 4683 4684 4685 4687 468a 468b 7780 7783 7784"
    ).split()
)
KEY_WAIT_ANCHORS = frozenset((0x3593, 0x35A1, 0x45F8))

ERR_NO_WINDOW = "M103B_NO_TRACE_WINDOW"
ERR_ENTRY_FRAME = "M103B_ENTRY_FRAME_MISMATCH"
ERR_SEGMENT = "M103B_SEGMENT_MISMATCH"
ERR_NOT_KEY_WAIT = "M103B_NOT_KEY_WAIT"
ERR_WORKER = "M103B_WORKER_FAILED"

TRACE_RE = re.compile(
    r"m76-compat-trace event=before slot=\d+ cs=([0-9a-f]{4}) ip=([0-9a-f]{4}) "
    r".* stk=([0-9a-f]{4}/[0-9a-f]{4}/[0-9a-f]{4}) "
)

Record = Tuple[str, int, str]


def parse_window(lines: Iterable[str]) -> List[Record]:
    records: List[Record] = []
    for line in lines:
        match = TRACE_RE.search(line)
        if match:
            records.append((match.group(1), int(match.group(2), 16), match.group(3)))
    return records


def classify(records: List[Record]) -> Optional[str]:
    """Return None for a key-wait window, otherwise one stable error code."""
    if not records:
        return ERR_NO_WINDOW
    if any(not stk.startswith(ENTRY_FRAME_PREFIX) for _, _, stk in records):
        return ERR_ENTRY_FRAME
    if any(cs != BASIC_SEGMENT for cs, _, _ in records):
        return ERR_SEGMENT
    ips = {ip for _, ip, _ in records}
    if not ips <= KEY_WAIT_IPS or not KEY_WAIT_ANCHORS <= ips:
        return ERR_NOT_KEY_WAIT
    return None


def _fixture_line(slot: int, cs: str, ip: int, stk: str) -> str:
    return (
        f"m76-compat-trace event=before slot={slot} cs={cs} ip={ip:04x} op=00/00 "
        "af=0044 bc=0000 de=ef9b hl=e6ba ix=0f7c iy=0101 sp=e5f1 nsp=fffa ss=0000 "
        f"stk={stk} flags=0044 rem=100"
    )


def _passing_fixture() -> List[str]:
    return [
        _fixture_line(slot, BASIC_SEGMENT, ip, "13b4/f000/f044")
        for slot, ip in enumerate(sorted(KEY_WAIT_IPS))
    ]


def selftest() -> int:
    base = _passing_fixture()
    failures = []
    if classify(parse_window(base)) is not None:
        failures.append("passing fixture rejected")
    # Each case applies exactly one controlled mutation to the passing fixture.
    cases = [
        ("empty window", [], ERR_NO_WINDOW),
        ("unrelated lines only", ["upd780trace core=x event=in"], ERR_NO_WINDOW),
        (
            "different entry frame",
            base[:-1] + [base[-1].replace("stk=13b4/f000/", "stk=13b5/f000/")],
            ERR_ENTRY_FRAME,
        ),
        (
            "different segment",
            base[:-1] + [base[-1].replace(f"cs={BASIC_SEGMENT}", "cs=1001")],
            ERR_SEGMENT,
        ),
        (
            "address outside the loop",
            base + [_fixture_line(999, BASIC_SEGMENT, 0xC244, "13b4/f000/f044")],
            ERR_NOT_KEY_WAIT,
        ),
        (
            "missing anchor",
            [line for line in base if " ip=45f8 " not in line],
            ERR_NOT_KEY_WAIT,
        ),
    ]
    for name, lines, expected in cases:
        actual = classify(parse_window(lines))
        if actual != expected:
            failures.append(f"{name}: expected {expected}, got {actual}")
    for failure in failures:
        print(f"m103b-basic-boot selftest: {failure}", file=sys.stderr)
    if failures:
        return 1
    print("m103b-basic-boot selftest: all classifications passed")
    return 0


def run(worker: str, roms: str, media: str, frames: int, skip: int, window: int,
        timeout: int) -> int:
    # The worker runs in a temporary directory, so resolve every input first.
    worker = os.path.abspath(worker)
    roms = os.path.abspath(roms)
    media = os.path.abspath(media)
    with tempfile.TemporaryDirectory(prefix="vaeg-m103b-") as work:
        disk = pathlib.Path(work) / "boot.d88"
        shutil.copyfile(media, disk)
        env = dict(os.environ)
        env.update(
            {
                "SDL_VIDEODRIVER": "dummy",
                "SDL_AUDIODRIVER": "dummy",
                "VAEG_UPD70008_TRACE": str(window),
                "VAEG_UPD70008_TRACE_SKIP": str(skip),
            }
        )
        command = [
            worker,
            "--nowait",
            "--no-cfg",
            "--no-bkupmem",
            "--roms",
            roms,
            "--model",
            "VA2",
            "--fdd1",
            str(disk),
            "--screenshot",
            f"{frames}:{pathlib.Path(work) / 'final.png'}",
        ]
        completed = subprocess.run(
            command,
            cwd=work,
            env=env,
            capture_output=True,
            text=True,
            errors="replace",
            timeout=timeout,
            check=False,
        )
    if completed.returncode != 0:
        print(f"{ERR_WORKER}: worker exit status {completed.returncode}", file=sys.stderr)
        return 1
    records = parse_window(completed.stderr.splitlines() + completed.stdout.splitlines())
    error = classify(records)
    if error is not None:
        print(f"{error}: {len(records)} compatible trace records", file=sys.stderr)
        return 1
    print(
        f"m103b-basic-boot: key-input wait reached ({len(records)} records, "
        f"{len({ip for _, ip, _ in records})} addresses)"
    )
    return 0


def main(argv: Optional[List[str]] = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="mode", required=True)
    sub.add_parser("selftest")
    skip_parser = sub.add_parser("skip")
    skip_parser.add_argument("--reason", required=True)
    run_parser = sub.add_parser("run")
    run_parser.add_argument("--worker", required=True)
    run_parser.add_argument("--roms", required=True)
    run_parser.add_argument("--media", required=True)
    run_parser.add_argument("--frames", type=int, default=6000)
    run_parser.add_argument("--skip", type=int, default=30000000)
    run_parser.add_argument("--window", type=int, default=20000)
    run_parser.add_argument("--timeout", type=int, default=600)
    args = parser.parse_args(argv)
    if args.mode == "selftest":
        return selftest()
    if args.mode == "skip":
        print(f"m103b-basic-boot: skipped: {args.reason}")
        return SKIP_RETURN_CODE
    return run(args.worker, args.roms, args.media, args.frames, args.skip, args.window,
               args.timeout)


if __name__ == "__main__":
    sys.exit(main())
