#!/usr/bin/env python3
"""Build the self-booting SGP demo disk (2HD D88, or 2DD) from source.

The disk needs no operating system: the PC-88VA ROM loads ipl.asm from the
first sector, which loads loader.asm, which offers a menu of the SGP demos
and runs each .COM image with a minimal INT 21h of its own.  Every byte is
assembled here from the repository's sources.  The output is a local
validation artifact: it is written outside the repository and must not be
committed (AGENTS.md).

With --system-source, the same demos are instead installed on a copy of a
PC-Engine 1.05/1.1 or PC-88VA MS-DOS system disk (an ordinary bootable disk
of that system; --pcengine-source is the former name); the output holds
that system's files and is likewise never committed.
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
import lzma
import os
import shutil
import struct
import subprocess
import sys
import tempfile
from pathlib import Path

# name: (D88 media byte, tracks, sectors per track, N, INT 80h disk mode)
FORMATS = {
    "2hd": (0x20, 154, 8, 3, 0x23),  # 77 cylinders x 2 heads x 8 x 1024 bytes
    "2dd": (0x10, 160, 9, 2, 0x12),  # 80 cylinders x 2 heads x 9 x 512 bytes
}
LOADER_BYTES_MAX = 8192
COM_MAX = 0xFE00  # the image must fit DEMO_SEG:0100-FEFF below the stack

# (menu label, build step, file produced in the work directory[, command tail])
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
    # M109: display-timing and TSP tests (tools/pc88va/vtiming), with their
    # command tails.
    ("V480PAT A: 24.8 kHz display timing sweep", "vtiming", "vt/V480PAT.COM", " A"),
    ("TSPMODE: undocumented TSP screen modes", "vtiming", "vt/TSPMODE.COM"),
    ("TSPFILL A: known data under those modes", "vtiming", "vt/TSPFILL.COM", " A"),
    # M109a: SGP regression test (tools/pc88va/sgptest).
    ("SGPTEST: SGP regression test, CRCs against vaeg", "sgptest", "sg/SGPTEST.COM"),
]


def demo_tail(entry: tuple) -> str:
    return entry[3] if len(entry) > 3 else ""


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
    vtiming = repo / "tools/pc88va/vtiming"
    (work / "vt").mkdir(parents=True, exist_ok=True)
    for name in ("v480pat", "tspmode", "tspfill"):
        nasm(env, ["-o", str(work / "vt" / f"{name.upper()}.COM"), str(vtiming / f"{name}.asm")])
    sgptest = repo / "tools/pc88va/sgptest"
    (work / "sg").mkdir(parents=True, exist_ok=True)
    nasm(env, [f"-I{sgptest}/", "-o", str(work / "sg" / "SGPTEST.COM"), str(sgptest / "sgptest.asm")])
    for name in sorted({entry[1] for entry in DEMOS} - {"vtiming", "sgptest"}):
        script, target = steps[name]
        (target if target.suffix == "" else target.parent).mkdir(parents=True, exist_ok=True)
        subprocess.run(["sh", str(script), str(target)], check=True, env=env,
                       stdout=subprocess.DEVNULL)


def nasm(env: dict[str, str], args: list[str]) -> None:
    subprocess.run([env.get("NASM", "nasm"), "-f", "bin", *args], check=True)


class Geometry:
    def __init__(self, name: str) -> None:
        self.name = name
        self.media, self.tracks, self.per_track, self.n, self.mode = FORMATS[name]
        self.size = 128 << self.n

    def sectors(self, size: int) -> int:
        return (size + self.size - 1) // self.size

    def defines(self) -> list[str]:
        return [f"-DSECTORS={self.per_track}", f"-DSECTOR_N={self.n}",
                f"-DDISK_MODE={self.mode:#x}"]


def asm_string(text: str) -> str:
    if not text.isascii() or "'" in text:
        raise ValueError(f"label must be ASCII without quotes: {text!r}")
    return f"'{text}', 0"


def write_d88(path: Path, image: bytes, name: str, geo: Geometry) -> None:
    """A D88 of geo.tracks tracks of geo.per_track MFM sectors."""
    header_size = 0x2B0
    track_size = geo.per_track * (16 + geo.size)
    total = header_size + geo.tracks * track_size
    out = bytearray(header_size)
    out[0:len(name)] = name.encode("ascii")
    out[0x1B] = geo.media
    struct.pack_into("<I", out, 0x1C, total)
    for track in range(geo.tracks):
        struct.pack_into("<I", out, 0x20 + track * 4, header_size + track * track_size)
    for track in range(geo.tracks):
        for record in range(1, geo.per_track + 1):
            lba = track * geo.per_track + record - 1
            out += struct.pack("<BBBBHBBB5xH", track // 2, track % 2, record, geo.n,
                               geo.per_track, 0, 0, 0, geo.size)
            out += image[lba * geo.size:(lba + 1) * geo.size]
    path.write_bytes(bytes(out))


# System-disk directories (8.3 names); sprites and wireframes share them,
# as in the existing distribution disks.
PCENGINE_DIRS = {"16": "16", "256": "256", "65536": "65536",
                 "w16": "16", "w256": "256", "w65536": "65536", "vt": "VTIMING",
                 "sg": "SGPTEST"}


def build_on_system(repo: Path, env: dict[str, str], source: Path, output: Path) -> int:
    if not source.is_file():
        print(f"error: system disk does not exist: {source}", file=sys.stderr)
        return 1
    tool = repo / "tools/pc88va/pcengine_disk.py"
    with tempfile.TemporaryDirectory(prefix="sgp-system-") as tmp:
        work = Path(tmp) / "build"
        payload = Path(tmp) / "payload"
        build_demos(repo, work, env)
        for entry in DEMOS:
            folder, name = entry[2].split("/")
            rel = entry[2]
            target = payload / PCENGINE_DIRS[folder] / name
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(work / rel, target)
        subprocess.run([sys.executable, str(tool), "vanilla", "--source", str(source),
                        "--output", str(output)], check=True)
        subprocess.run([sys.executable, str(tool), "install", "--image", str(output),
                        "--payload", str(payload)], check=True)
    print(f"Created bootable SGP demo disk: {output}")
    for entry in DEMOS:
        folder, name = entry[2].split("/")
        print(f"  {PCENGINE_DIRS[folder]}\\{name}")
    print("  the disk holds the source disk's system files: keep it outside the repository")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--output", required=True, type=Path,
                        help="raw D88 to write, outside the repository")
    parser.add_argument("--format", choices=sorted(FORMATS), default="2hd",
                        help="disk format (default 2hd)")
    parser.add_argument("--compressed-output", type=Path,
                        help="the distributable .d88.xz (M109; default "
                             "demos/disks/sgp-boot.d88.xz for the 2HD disk; 'none' skips it)")
    parser.add_argument("--system-source", "--pcengine-source", type=Path,
                        help="install the demos on a copy of this PC-Engine 1.05/1.1 "
                             "or PC-88VA MS-DOS system disk instead")
    args = parser.parse_args()
    geo = Geometry(args.format)

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

    if args.system_source is not None:
        return build_on_system(repo, env, args.system_source, output)
    compressed_output = args.compressed_output
    if compressed_output is None and geo.name == "2hd":
        compressed_output = repo / "demos/disks/sgp-boot.d88.xz"
    if compressed_output is not None and str(compressed_output) == "none":
        compressed_output = None
    if compressed_output is not None and compressed_output.exists():
        print(f"error: refusing to overwrite {compressed_output}", file=sys.stderr)
        return 1

    with tempfile.TemporaryDirectory(prefix="sgp-boot-") as tmp:
        work = Path(tmp)
        build_demos(repo, work, env)
        payloads = [(entry[0], (work / entry[2]).read_bytes(), entry[2], demo_tail(entry))
                    for entry in DEMOS]

        # Layout: IPL at sector 0, the loader from sector 1, then each demo.
        loader_first = 1
        loader_sectors_max = geo.sectors(LOADER_BYTES_MAX)
        first = loader_first + loader_sectors_max
        lines = [f"catalog_count\tdw\t{len(payloads)}", "catalog:"]
        for index, (label, data, rel, tail) in enumerate(payloads):
            if len(data) > COM_MAX:
                print(f"error: {rel} is {len(data)} bytes, over {COM_MAX}", file=sys.stderr)
                return 1
            tail_ref = f"tail{index}" if tail else "0"
            lines.append(f"\t\tdw\t{first}, {geo.sectors(len(data))}, label{index}, {tail_ref}")
            first += geo.sectors(len(data))
        lines.append("\t\tdw\t0")
        for index, (label, _, _, tail) in enumerate(payloads):
            lines.append(f"label{index}\t\tdb\t{asm_string(label)}")
            if tail:
                lines.append(f"tail{index}\t\tdb\t{asm_string(tail)}")
        (work / "catalog.inc").write_text("\n".join(lines) + "\n", encoding="ascii")
        if first > geo.tracks * geo.per_track:
            print(f"error: the demos do not fit a {geo.name.upper()} disk", file=sys.stderr)
            return 1

        nasm(env, [*geo.defines(), "-I", f"{work}/", "-o", str(work / "LOADER.BIN"),
                   str(here / "loader.asm")])
        loader = (work / "LOADER.BIN").read_bytes()
        if len(loader) > LOADER_BYTES_MAX:
            print("error: the loader is too large", file=sys.stderr)
            return 1
        nasm(env, [*geo.defines(), f"-DLOADER_SECTORS={geo.sectors(len(loader))}",
                   "-o", str(work / "IPL.BIN"), str(here / "ipl.asm")])
        ipl = (work / "IPL.BIN").read_bytes()
        if len(ipl) > geo.size:
            print("error: the IPL is larger than one sector", file=sys.stderr)
            return 1

        size = geo.size
        image = bytearray(geo.tracks * geo.per_track * size)
        image[0:len(ipl)] = ipl
        image[loader_first * size:loader_first * size + len(loader)] = loader
        lba = loader_first + loader_sectors_max
        for _, data, _, _ in payloads:
            image[lba * size:lba * size + len(data)] = data
            lba += geo.sectors(len(data))
        write_d88(output, bytes(image), "SGPBOOT", geo)
        if compressed_output is not None:
            # M109: everything on the disk is built from this repository's
            # sources (IPL, loader, demos and tests; no system files), so it is
            # distributable (AGENTS.md); the .xz must round-trip to the D88.
            raw = output.read_bytes()
            packed = lzma.compress(raw, format=lzma.FORMAT_XZ, preset=lzma.PRESET_EXTREME | 9)
            if lzma.decompress(packed) != raw:
                print("error: the compressed image does not round-trip", file=sys.stderr)
                return 1
            compressed_output.write_bytes(packed)

    print(f"Created self-booting SGP demo disk ({geo.name.upper()}): {output}")
    for index, (label, data, rel, _) in enumerate(payloads):
        print(f"  {chr(ord('A') + index)}  {rel:20s} {len(data):6d} bytes  {label}")
    print(f"  loader {len(loader)} bytes")
    if compressed_output is not None:
        print(f"  distributable copy: {compressed_output}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
