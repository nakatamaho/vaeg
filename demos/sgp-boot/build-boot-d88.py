#!/usr/bin/env python3
"""Build the self-booting SGP demo disk (2DD D88) from source.

The disk needs no operating system: the PC-88VA ROM loads ipl.asm from the
first sector, which loads loader.asm, which offers a menu of the SGP demos
and runs each .COM image with a minimal INT 21h of its own.  Every byte is
assembled here from the repository's sources.  The output is a local
validation artifact: it is written outside the repository and must not be
committed (AGENTS.md).
"""

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
import os
import shutil
import struct
import subprocess
import sys
import tempfile
from pathlib import Path

SECTOR = 512
SECTORS_PER_TRACK = 9
TRACKS = 160  # 80 cylinders x 2 heads
LOADER_SECTORS_MAX = 16
COM_MAX = 0xFE00  # the image must fit DEMO_SEG:0100-FEFF below the stack

# (menu label, build step, file produced in the work directory)
DEMOS = [
    ("pseudo-sprites, 16 colours, M7a (synchronous)", "sprite16", "16/SGPD_7A.COM"),
    ("pseudo-sprites, 16 colours, M7b (dirty rectangles)", "sprite16", "16/SGPD_7B.COM"),
    ("pseudo-sprites, 16 colours, M7c (pipelined)", "sprite16", "16/SGPD_7C.COM"),
    ("pseudo-sprites, 16 colours, M7d (templates)", "sprite16", "16/SGPD_7D.COM"),
    ("pseudo-sprites, 16 colours, scrolling background", "sprite16", "16/SGPD_7S.COM"),
    ("pseudo-sprites, 256 colours", "sprite256", "256/SGP256S.COM"),
    ("pseudo-sprites, 256 colours, scrolling background", "sprite256t", "256/SGP256T.COM"),
    ("pseudo-sprites, 65536 colours", "sprite65536", "65536/SGP655S.COM"),
    ("wireframe, 16 colours", "wire16", "w16/SGPWIRE.COM"),
    ("wireframe, 256 colours", "wire256", "w256/SGPWIRE.COM"),
    ("wireframe, 65536 colours", "wire65536", "w65536/SGPWIRE.COM"),
]


def build_demos(repo: Path, work: Path, env: dict[str, str]) -> None:
    demos = repo / "demos"
    steps = {
        "sprite16": [demos / "sgp-pseudo-sprite/16/build.sh", work / "16"],
        "sprite256": [demos / "sgp-pseudo-sprite/256/build.sh", work / "256/SGP256S.COM"],
        "sprite256t": [demos / "sgp-pseudo-sprite/256/build-scroll.sh", work / "256/SGP256T.COM"],
        "sprite65536": [demos / "sgp-pseudo-sprite/65536/build.sh", work / "65536/SGP655S.COM"],
        "wire16": [demos / "sgp-wireframe/16/build.sh", work / "w16/SGPWIRE.COM"],
        "wire256": [demos / "sgp-wireframe/256/build.sh", work / "w256/SGPWIRE.COM"],
        "wire65536": [demos / "sgp-wireframe/65536/build.sh", work / "w65536/SGPWIRE.COM"],
    }
    for name in sorted({step for _, step, _ in DEMOS}):
        script, target = steps[name]
        (target if target.suffix == "" else target.parent).mkdir(parents=True, exist_ok=True)
        subprocess.run(["sh", str(script), str(target)], check=True, env=env,
                       stdout=subprocess.DEVNULL)


def nasm(env: dict[str, str], args: list[str]) -> None:
    subprocess.run([env.get("NASM", "nasm"), "-f", "bin", *args], check=True)


def sectors(size: int) -> int:
    return (size + SECTOR - 1) // SECTOR


def asm_string(text: str) -> str:
    if not text.isascii() or "'" in text:
        raise ValueError(f"label must be ASCII without quotes: {text!r}")
    return f"'{text}', 0"


def write_d88(path: Path, image: bytes, name: str) -> None:
    """A 2DD D88: 160 tracks of 9 MFM sectors of 512 bytes (N = 2)."""
    header_size = 0x2B0
    track_size = SECTORS_PER_TRACK * (16 + SECTOR)
    total = header_size + TRACKS * track_size
    out = bytearray(header_size)
    out[0:len(name)] = name.encode("ascii")
    out[0x1B] = 0x10  # 2DD
    struct.pack_into("<I", out, 0x1C, total)
    for track in range(TRACKS):
        struct.pack_into("<I", out, 0x20 + track * 4, header_size + track * track_size)
    for track in range(TRACKS):
        for record in range(1, SECTORS_PER_TRACK + 1):
            lba = track * SECTORS_PER_TRACK + record - 1
            out += struct.pack("<BBBBHBBB5xH", track // 2, track % 2, record, 2,
                               SECTORS_PER_TRACK, 0, 0, 0, SECTOR)
            out += image[lba * SECTOR:(lba + 1) * SECTOR]
    path.write_bytes(bytes(out))


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--output", required=True, type=Path,
                        help="raw D88 to write, outside the repository")
    args = parser.parse_args()

    here = Path(__file__).resolve().parent
    repo = here.parent.parent
    output = args.output.resolve()
    if output.exists():
        print(f"error: refusing to overwrite {output}", file=sys.stderr)
        return 1
    if output == repo or repo in output.parents:
        print("error: the bootable disk must be written outside the repository",
              file=sys.stderr)
        return 1
    if not output.parent.is_dir():
        print(f"error: output directory does not exist: {output.parent}", file=sys.stderr)
        return 1
    env = dict(os.environ)
    if shutil.which(env.get("NASM", "nasm")) is None:
        print("error: nasm is required (set NASM to its path)", file=sys.stderr)
        return 1

    with tempfile.TemporaryDirectory(prefix="sgp-boot-") as tmp:
        work = Path(tmp)
        build_demos(repo, work, env)
        payloads = [(label, (work / rel).read_bytes(), rel) for label, _, rel in DEMOS]

        # Layout: IPL at sector 0, the loader from sector 1, then each demo.
        loader_first = 1
        first = loader_first + LOADER_SECTORS_MAX
        lines = [f"catalog_count\tdw\t{len(payloads)}", "catalog:"]
        for index, (label, data, rel) in enumerate(payloads):
            if len(data) > COM_MAX:
                print(f"error: {rel} is {len(data)} bytes, over {COM_MAX}", file=sys.stderr)
                return 1
            lines.append(f"\t\tdw\t{first}, {sectors(len(data))}, label{index}")
            first += sectors(len(data))
        lines.append("\t\tdw\t0")
        for index, (label, _, _) in enumerate(payloads):
            lines.append(f"label{index}\t\tdb\t{asm_string(label)}")
        (work / "catalog.inc").write_text("\n".join(lines) + "\n", encoding="ascii")
        if first > TRACKS * SECTORS_PER_TRACK:
            print("error: the demos do not fit a 2DD disk", file=sys.stderr)
            return 1

        nasm(env, ["-I", f"{work}/", "-o", str(work / "LOADER.BIN"), str(here / "loader.asm")])
        loader = (work / "LOADER.BIN").read_bytes()
        if sectors(len(loader)) > LOADER_SECTORS_MAX:
            print("error: the loader is too large", file=sys.stderr)
            return 1
        nasm(env, [f"-DLOADER_SECTORS={sectors(len(loader))}", "-o", str(work / "IPL.BIN"),
                   str(here / "ipl.asm")])
        ipl = (work / "IPL.BIN").read_bytes()
        if len(ipl) != SECTOR:
            print("error: the IPL is not one sector", file=sys.stderr)
            return 1

        image = bytearray(TRACKS * SECTORS_PER_TRACK * SECTOR)
        image[0:SECTOR] = ipl
        image[loader_first * SECTOR:loader_first * SECTOR + len(loader)] = loader
        lba = loader_first + LOADER_SECTORS_MAX
        for _, data, _ in payloads:
            image[lba * SECTOR:lba * SECTOR + len(data)] = data
            lba += sectors(len(data))
        write_d88(output, bytes(image), "SGPBOOT")

    print(f"Created self-booting SGP demo disk (2DD): {output}")
    for index, (label, data, rel) in enumerate(payloads):
        print(f"  {chr(ord('A') + index)}  {rel:20s} {len(data):6d} bytes  {label}")
    print(f"  loader {len(loader)} bytes; the disk must stay outside the repository")
    return 0


if __name__ == "__main__":
    sys.exit(main())
