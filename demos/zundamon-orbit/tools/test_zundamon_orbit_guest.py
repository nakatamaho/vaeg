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

"""Focused fail-closed tests for the M98k capture oracle."""

from __future__ import annotations

import importlib.util
import json
import struct
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path
from typing import Callable


SCRIPT = Path(__file__).with_name("verify_zundamon_orbit_guest.py")
ASSEMBLY = Path(__file__).resolve().parents[1] / "256" / "zundamon_orbit_256.asm"
SPEC = importlib.util.spec_from_file_location("m98k_oracle", SCRIPT)
if SPEC is None or SPEC.loader is None:
    raise RuntimeError("cannot load M98k oracle")
ORACLE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(ORACLE)


def write_registers(path: Path, *, bx: str = "0140") -> None:
    values = {
        "schema": "vaeg-registers-v1",
        "sequence": "1",
        "ordinal": "1",
        "clock": "1000",
        "ax": "984b",
        "bx": bx,
        "cx": "00c8",
        "dx": "0808",
        "si": "0101",
        "di": "005c",
        "bp": "0098",
        "sp": "f000",
        "es": "3000",
        "cs": "3000",
        "ss": "3000",
        "ds": "3000",
        "ip": "0800",
        "flags": "0200",
        "es_base": "00030000",
        "cs_base": "00030000",
        "ss_base": "00030000",
        "ds_base": "00030000",
    }
    path.write_text("".join(f"{key}\t{value}\n" for key, value in values.items()),
                    encoding="utf-8")


def bmp24(nonblack: bool) -> bytes:
    width = 320
    height = 200
    row_size = ((width * 3 + 3) // 4) * 4
    pixels = bytearray(row_size * height)
    if nonblack:
        for y in range(height):
            for x in range(width):
                offset = y * row_size + x * 3
                pixels[offset:offset + 3] = bytes((0x40, 0x70, 0x20))
    file_size = 54 + len(pixels)
    header = bytearray(54)
    struct.pack_into("<2sIHHI", header, 0, b"BM", file_size, 0, 0, 54)
    struct.pack_into("<IiiHHIIiiII", header, 14, 40, width, height, 1, 24,
                     0, len(pixels), 2835, 2835, 0, 0)
    return bytes(header + pixels)


def passing_gvram() -> bytes:
    raw = bytearray(ORACLE.GVRAM_SIZE)
    raw[:ORACLE.G0_SIZE] = ORACLE.EXPECTED_G0
    start = ORACLE.G1_OFFSET
    raw[start:start + len(ORACLE.EXPECTED_G1)] = ORACLE.EXPECTED_G1
    return bytes(raw)


def write_fixture(directory: Path) -> None:
    raw = passing_gvram()
    screen = bmp24(True)
    for prefix in ORACLE.PREFIXES:
        write_registers(directory / f"{prefix}.registers.tsv")
        (directory / f"{prefix}.gvram.bin").write_bytes(raw)
        (directory / f"{prefix}.screen.bmp").write_bytes(screen)
    (directory / "events.tsv").write_text(
        "event\tframe\tid\tvalue\n"
        "initialized\t0\t-\t5000\n"
        "frame\t1200\t-\t1200\n"
        "input\t1200\t-\t3\n"
        "wait-pc\t1210\t-\t1\n"
        "pc\t1220\t-\t1\n"
        "capture\t1220\tm98k-settled-a\t13\n"
        "wait-pc\t1220\t-\t1\n"
        "pc\t1221\t-\t2\n"
        "capture\t1221\tm98k-settled-b\t13\n"
        "exit\t1221\t-\t0\n",
        encoding="utf-8",
    )


class M98kOracleTests(unittest.TestCase):
    def run_case(
        self,
        mutation: Callable[[Path], Path | None] | None = None,
    ) -> tuple[int, dict[str, object]]:
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            write_fixture(directory)
            source = None if mutation is None else mutation(directory)
            command = [sys.executable, str(SCRIPT), str(directory)]
            if source is not None:
                command.extend(("--source", str(source)))
            completed = subprocess.run(command, check=False, capture_output=True, text=True)
            return completed.returncode, json.loads(completed.stdout)

    def assert_error(
        self,
        expected: str,
        mutation: Callable[[Path], Path | None],
    ) -> None:
        returncode, result = self.run_case(mutation)
        self.assertEqual(returncode, 1)
        self.assertEqual(result["status"], "FAIL")
        self.assertEqual(result["errors"], [expected])

    def test_passing_fixture(self) -> None:
        returncode, result = self.run_case()
        self.assertEqual(returncode, 0)
        self.assertEqual(result["status"], "PASS")
        self.assertEqual(result["errors"], [])
        self.assertEqual(result["g1_nonzero_count"], 90)

    def test_mode_signature_mutation(self) -> None:
        def mutate(directory: Path) -> None:
            for prefix in ORACLE.PREFIXES:
                write_registers(directory / f"{prefix}.registers.tsv", bx="0280")
            return None
        self.assert_error("M98K_MODE_SIGNATURE", mutate)

    def test_marker_pixel_mutation(self) -> None:
        def mutate(directory: Path) -> None:
            for prefix in ORACLE.PREFIXES:
                path = directory / f"{prefix}.gvram.bin"
                raw = bytearray(path.read_bytes())
                raw[ORACLE.G1_OFFSET + ORACLE.MARKER_Y * ORACLE.G1_WIDTH +
                    ORACLE.MARKER_X + 5] ^= 1
                path.write_bytes(raw)
            return None
        self.assert_error("M98K_MARKER_LAYOUT", mutate)

    def test_second_marker_mutation(self) -> None:
        def mutate(directory: Path) -> None:
            for prefix in ORACLE.PREFIXES:
                path = directory / f"{prefix}.gvram.bin"
                raw = bytearray(path.read_bytes())
                for row, expected in enumerate(ORACLE.EXPECTED_MARKER):
                    start = ORACLE.G1_OFFSET + (20 + row) * ORACLE.G1_WIDTH + 20
                    raw[start:start + ORACLE.MARKER_WIDTH] = expected
                path.write_bytes(raw)
            return None
        self.assert_error("M98K_MARKER_MULTIPLE", mutate)

    def test_two_frame_instability_mutation(self) -> None:
        def mutate(directory: Path) -> None:
            path = directory / f"{ORACLE.PREFIXES[1]}.gvram.bin"
            raw = bytearray(path.read_bytes())
            raw[-1] ^= 1
            path.write_bytes(raw)
            return None
        self.assert_error("M98K_GVRAM_UNSTABLE", mutate)

    def test_black_screen_mutation(self) -> None:
        def mutate(directory: Path) -> None:
            black = bmp24(False)
            for prefix in ORACLE.PREFIXES:
                (directory / f"{prefix}.screen.bmp").write_bytes(black)
            return None
        self.assert_error("M98K_SCREEN_BLACK", mutate)

    def test_frame_limit_mutation(self) -> None:
        def mutate(directory: Path) -> None:
            path = directory / "events.tsv"
            path.write_text(path.read_text(encoding="utf-8") +
                            "frame-limit\t5000\t-\t5000\n", encoding="utf-8")
            return None
        self.assert_error("M98K_EVENTS_TIMEOUT", mutate)

    def test_forbidden_source_mutation(self) -> None:
        def mutate(directory: Path) -> Path:
            path = directory / "guest.asm"
            path.write_text(ASSEMBLY.read_text(encoding="utf-8") +
                            "\n%include \"external.inc\"\n", encoding="utf-8")
            return path
        self.assert_error("M98K_SOURCE_FORBIDDEN", mutate)


if __name__ == "__main__":
    unittest.main()
