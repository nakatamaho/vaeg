# Copyright (c) 2026 Nakata Maho
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
# INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
# STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
# IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.

"""Build the self-booting PC-8801-17 Video Art Board sample disk.

The disk shows two pictures on the Video Art Board's 320x200 65536-colour
screen, in a PC-8801 or in a PC-88VA in V1/V2 mode: a ray-traced ring of
spheres (raytrace.py) and a Mandelbrot close-up of Elephant valley
(mandel.py).  They are this directory's own scenes, different from the
HyPER-FRAME+ pictures of demos/hyperframe+.  It is a 2D disk with no file system and no system
files: the IPL in track 0 sector 1 is this script's own Z80 program, and the
pictures follow it as raw board memory images.

Layout (sectors of 256 bytes, 16 per track, track = cylinder * 2 + head):
  linear sector 0           the IPL (loaded at C000h by the boot ROM)
  linear sectors 1-512      picture 1, 128 KiB = the board's four banks
  linear sectors 513-1024   picture 2
Each picture is 320x200 words, row by row, bits 15-10 green, 9-5 red, 4-0
blue (little endian), followed by zero padding to 128 KiB.

The IPL finds the board (card 1 of the extended-RAM interface, ports E2h/E3h,
B4h), turns its display on, loads picture 1 through the N88-BASIC ROM's
sector read routine (369Ah, as in X88000's Video Art Board sample), waits for
a key, loads picture 2, waits for a key, and starts again.  Without the board
it prints a message and stops.

The raw D88 is written to --output-dir outside the repository; the compressed
companion goes to demos/vab/.
"""

from __future__ import annotations

import argparse
import hashlib
import lzma
import struct
import subprocess
import sys
import tempfile
from pathlib import Path

DEMO_DIR = Path(__file__).resolve().parent
REPOSITORY_ROOT = DEMO_DIR.parents[1]
STEM = "vab-samples"
PICTURES = (("raytrace.py", "RING.RGB"), ("mandel.py", "ELEPHANT.RGB"))

WIDTH, HEIGHT = 320, 200
BOARD_BYTES = 128 * 1024
SECTOR_BYTES = 256
SECTORS_PER_TRACK = 16
TRACKS = 80                                  # 40 cylinders, two heads
PICTURE_SECTORS = BOARD_BYTES // SECTOR_BYTES
IPL_ORIGIN = 0xC000
NO_BOARD_TEXT = b"NO VIDEO ART BOARD (PC-8801-17)"

# 4x4 ordered dither, 0..15.
BAYER = ((0, 8, 2, 10), (12, 4, 14, 6), (3, 11, 1, 9), (15, 7, 13, 5))


class BuildError(Exception):
    """A deterministic input or layout error."""


# --------------------------------------------------------------------------
# A small two-pass Z80 assembler for the IPL: plain byte strings, labels,
# relative jumps and absolute 16-bit label references.

def assemble(program, origin):
    """program: list of bytes, ("label", name), ("jr", opcode, name) and
    ("abs", opcode_bytes, name).  Returns the machine code."""
    labels, address = {}, origin
    for item in program:
        if isinstance(item, bytes):
            address += len(item)
        elif item[0] == "label":
            if item[1] in labels:
                raise BuildError(f"duplicate label {item[1]}")
            labels[item[1]] = address
        elif item[0] == "jr":
            address += 2
        elif item[0] == "abs":
            address += len(item[1]) + 2
        else:
            raise BuildError(f"bad item {item!r}")
    code, address = bytearray(), origin
    for item in program:
        if isinstance(item, bytes):
            code += item
            address += len(item)
        elif item[0] == "jr":
            offset = labels[item[2]] - (address + 2)
            if not -128 <= offset <= 127:
                raise BuildError(f"relative jump to {item[2]} out of range")
            code += bytes((item[1], offset & 0xFF))
            address += 2
        elif item[0] == "abs":
            code += item[1] + struct.pack("<H", labels[item[2]])
            address += len(item[1]) + 2
    return bytes(code)


def ipl_program():
    """The IPL.  Register use in the main loop: B = track (cylinder * 2 +
    head), C = sector (1-16) of the next sector to read."""
    L = lambda name: ("label", name)          # noqa: E731
    out = lambda port: bytes((0xD3, port))    # OUT (port),A
    inp = lambda port: bytes((0xDB, port))    # IN A,(port)
    return [
        # Find the board: try each extended-RAM card position (E3h bits 3-2).
        # B4h reads back its latch only while the board's card is selected.
        b"\x26\x00",                # LD H,0
        L("find"),
        b"\x7C", out(0xE3),         # LD A,H / OUT (E3h),A
        b"\x3E\x05", out(0xB4),     # LD A,05h / OUT (B4h),A
        inp(0xB4), b"\xE6\x07",     # IN A,(B4h) / AND 07h
        b"\x4F",                    # LD C,A
        b"\xAF", out(0xB4),         # XOR A / OUT (B4h),A
        b"\x79", b"\xFE\x05",       # LD A,C / CP 05h
        ("jr", 0x20, "next_card"),  # JR NZ,next_card
        inp(0xB4), b"\xE6\x07",     # IN A,(B4h) / AND 07h
        ("jr", 0x28, "found"),      # JR Z,found
        L("next_card"),
        b"\x7C", b"\xC6\x04", b"\x67",  # LD A,H / ADD A,4 / LD H,A
        b"\xFE\x10",                # CP 10h
        ("jr", 0x38, "find"),       # JR C,find
        # No board: print the message on the text screen and stop.
        ("abs", b"\x21", "message"),    # LD HL,message
        b"\x11\x40\xF4",            # LD DE,F440h (text VRAM, row 1)
        bytes((0x01, len(NO_BOARD_TEXT), 0x00)),  # LD BC,length
        b"\xED\xB0",                # LDIR
        L("stop"),
        ("jr", 0x18, "stop"),       # JR stop
        L("found"),
        b"\x7C", ("abs", b"\x32", "card"),  # LD A,H / LD (card),A
        b"\x3E\x47", out(0xB4),     # LD A,47h / OUT (B4h),A: display on
        L("first"),
        b"\x01\x02\x00",            # LD BC,0002h: track 0, sector 2
        L("picture"),
        ("abs", b"\xCD", "load"),   # CALL load
        ("abs", b"\xCD", "key"),    # CALL key
        b"\x78", b"\xFE\x40",       # LD A,B / CP 64: after picture 2?
        ("jr", 0x38, "picture"),    # JR C,picture
        ("jr", 0x18, "first"),      # JR first

        # load: read 512 sectors from B,C into the board's banks 0-3.
        L("load"),
        ("abs", b"\x3A", "card"),   # LD A,(card)
        b"\x67",                    # LD H,A: card, bank 0
        L("bank"),
        b"\xAF", b"\x57", b"\x5F",  # XOR A / LD D,A / LD E,A: DE = 0
        L("sector"),
        b"\x7C", out(0xE3),         # LD A,H / OUT (E3h),A
        b"\xAF", b"\x32\x85\xEC",   # XOR A / LD (EC85h),A: drive 1
        b"\x3E\x03", b"\x32\x50\xEF",   # LD A,3 / LD (EF50h),A
        b"\xA7",                    # AND A
        b"\xE5\xC5\xD5",            # PUSH HL / PUSH BC / PUSH DE
        b"\x21\x00\xC1",            # LD HL,C100h: sector buffer
        b"\xCD\x9A\x36",            # CALL 369Ah: N88-BASIC sector read
        b"\xD1",                    # POP DE
        b"\x01\x00\x01",            # LD BC,0100h
        b"\xF3",                    # DI
        b"\x3E\x10", out(0xE2),     # LD A,10h / OUT (E2h),A: write to card
        b"\xED\xB0",                # LDIR
        b"\xAF", out(0xE2),         # XOR A / OUT (E2h),A
        b"\xFB",                    # EI
        b"\xC1\xE1",                # POP BC / POP HL
        b"\x0C", b"\x79", b"\xFE\x11",  # INC C / LD A,C / CP 17
        ("jr", 0x38, "same_track"), # JR C,same_track
        b"\x0E\x01", b"\x04",       # LD C,1 / INC B
        L("same_track"),
        b"\x7A", b"\xA7",           # LD A,D / AND A
        ("abs", b"\xF2", "sector"), # JP P,sector: DE < 8000h
        b"\x24", b"\x7C", b"\xE6\x03",  # INC H / LD A,H / AND 3
        ("jr", 0x20, "bank"),       # JR NZ,bank
        b"\xC9",                    # RET

        # key: wait until no key is down, then until one is.
        L("key"),
        b"\xC5",                    # PUSH BC
        L("release"),
        ("abs", b"\xCD", "scan"),   # CALL scan
        ("jr", 0x20, "release"),    # JR NZ,release
        L("press"),
        ("abs", b"\xCD", "scan"),   # CALL scan
        ("jr", 0x28, "press"),      # JR Z,press
        b"\xC1", b"\xC9",           # POP BC / RET
        # scan: NZ when a key of rows 00h-0Bh is down (active low).
        L("scan"),
        b"\x0E\x00", b"\x16\xFF",   # LD C,0 / LD D,FFh
        L("row"),
        b"\xED\x78", b"\xA2", b"\x57",  # IN A,(C) / AND D / LD D,A
        b"\x0C", b"\x79", b"\xFE\x0C",  # INC C / LD A,C / CP 0Ch
        ("jr", 0x38, "row"),        # JR C,row
        b"\x7A", b"\x2F", b"\xB7",  # LD A,D / CPL / OR A
        b"\xC9",                    # RET

        L("card"), b"\x00",
        L("message"), NO_BOARD_TEXT,
    ]


# --------------------------------------------------------------------------

def board_picture(rgb):
    """320x200 RGB bytes -> 128 KiB board image (ordered dither to 6/5/5)."""
    if len(rgb) != WIDTH * HEIGHT * 3:
        raise BuildError("a picture is not 320x200x3 bytes")
    out = bytearray(BOARD_BYTES)
    for y in range(HEIGHT):
        for x in range(WIDTH):
            i = 3 * (y * WIDTH + x)
            threshold = (BAYER[y & 3][x & 3] + 0.5) / 16.0
            r, g, b = (min(levels, int(value * levels / 255.0 + threshold))
                       for value, levels in zip(rgb[i:i + 3], (31, 63, 31)))
            struct.pack_into("<H", out, 2 * (y * WIDTH + x), (g << 10) | (r << 5) | b)
    return bytes(out)


def d88_image(sectors):
    """sectors: list of 256-byte linear sectors.  An unprotected 2D D88."""
    header_size = 0x2B0
    tracks = bytearray()
    offsets = []
    for track in range(TRACKS):
        offsets.append(header_size + len(tracks))
        cylinder, head = divmod(track, 2)
        for record in range(1, SECTORS_PER_TRACK + 1):
            linear = track * SECTORS_PER_TRACK + record - 1
            data = sectors[linear] if linear < len(sectors) else bytes(SECTOR_BYTES)
            tracks += struct.pack("<4BHBBB5xH", cylinder, head, record, 1,
                                  SECTORS_PER_TRACK, 0x00, 0x00, 0x00, SECTOR_BYTES)
            tracks += data
    header = bytearray(header_size)
    header[0:16] = b"VAB SAMPLES".ljust(16, b"\x00")
    header[0x1A] = 0x00           # not write protected
    header[0x1B] = 0x00           # 2D
    struct.pack_into("<I", header, 0x1C, header_size + len(tracks))
    for index, offset in enumerate(offsets):
        struct.pack_into("<I", header, 0x20 + 4 * index, offset)
    return bytes(header + tracks)


def build(samples, work):
    ipl = assemble(ipl_program(), IPL_ORIGIN)
    if len(ipl) > SECTOR_BYTES:
        raise BuildError(f"the IPL is {len(ipl)} bytes, more than one sector")
    data = ipl.ljust(SECTOR_BYTES, b"\x00")
    for generator, name in PICTURES:
        path = work / name
        subprocess.run([sys.executable, "-I", str(DEMO_DIR / generator), str(path),
                        "--samples", str(samples)], check=True)
        data += board_picture(path.read_bytes())
    sectors = [data[i:i + SECTOR_BYTES] for i in range(0, len(data), SECTOR_BYTES)]
    if len(sectors) != 1 + 2 * PICTURE_SECTORS:
        raise BuildError("unexpected disk layout")
    return d88_image(sectors), ipl


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--output-dir", required=True, type=Path,
                        help="directory for the raw D88, outside the repository")
    parser.add_argument("--compressed-dir", type=Path, default=DEMO_DIR,
                        help="directory for the .d88.xz companion (default demos/vab)")
    parser.add_argument("--check", action="store_true",
                        help="build, and compare with the committed .d88.xz instead of "
                             "writing it")
    parser.add_argument("--samples", type=int, default=4)
    args = parser.parse_args(argv)
    output_dir = args.output_dir.resolve()
    xz = args.compressed_dir.resolve() / f"{STEM}.d88.xz"
    raw = output_dir / f"{STEM}.d88"
    try:
        if output_dir == REPOSITORY_ROOT or REPOSITORY_ROOT in output_dir.parents:
            raise BuildError("the raw D88 must be written outside the repository")
        output_dir.mkdir(parents=True, exist_ok=True)
        if raw.exists():
            raise BuildError(f"output already exists: {raw}")
        if not args.check and xz.exists():
            raise BuildError(f"output already exists: {xz}")
        with tempfile.TemporaryDirectory(prefix="vaeg-vab-") as temporary:
            image, ipl = build(args.samples, Path(temporary))
        compressed = lzma.compress(image, format=lzma.FORMAT_XZ, preset=lzma.PRESET_EXTREME | 9)
        if lzma.decompress(compressed) != image:
            raise BuildError("M160_VAB_XZ_ROUND_TRIP")
        if args.check and lzma.decompress(xz.read_bytes()) != image:
            raise BuildError("M160_VAB_COMMITTED_IMAGE_DIFFERS")
        raw.write_bytes(image)
        if not args.check:
            xz.write_bytes(compressed)
    except (BuildError, OSError, subprocess.CalledProcessError) as error:
        parser.exit(1, f"error: {error}\n")
    print(f"IPL {len(ipl)} bytes")
    print(f"{STEM}.d88 SHA-256 {hashlib.sha256(image).hexdigest()}")
    if args.check:
        print(f"  matches {xz}")
    else:
        print(f"  {STEM}.d88.xz SHA-256 {hashlib.sha256(compressed).hexdigest()}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
