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

"""Focused fail-closed tests for the M98l capture oracle."""

from __future__ import annotations

import json
import shutil
import struct
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path
from typing import Callable


TOOLS = Path(__file__).resolve().parent
if str(TOOLS) not in sys.path:
    sys.path.insert(0, str(TOOLS))
import build_zundamon_orbit_pipeline as pipeline  # noqa: E402
import inspect_zundamon_orbit_atlas as atlas_format  # noqa: E402
import verify_zundamon_orbit_guest as oracle  # noqa: E402


SCRIPT = TOOLS / "verify_zundamon_orbit_guest.py"
ASSEMBLY = TOOLS.parent / "256" / "zundamon_orbit_256.asm"


def write_registers(path: Path, required: dict[str, str]) -> None:
    values = {
        "schema": "vaeg-registers-v1", "sequence": "1", "ordinal": "1",
        "clock": "1000", "ax": "0000", "bx": "0000", "cx": "0000",
        "dx": "0000", "si": "0000", "di": "0000", "bp": "0000",
        "sp": "f000", "es": "3000", "cs": "3000", "ss": "3000",
        "ds": "3000", "ip": "0000", "flags": "0200",
        "es_base": "00030000", "cs_base": "00030000",
        "ss_base": "00030000", "ds_base": "00030000",
    }
    values.update(required)
    path.write_text("".join(f"{key}\t{value}\n" for key, value in values.items()),
                    encoding="utf-8")


def bmp24(nonblack: bool) -> bytes:
    width, height = 320, 200
    row_size = ((width * 3 + 3) // 4) * 4
    pixels = bytearray(row_size * height)
    if nonblack:
        pixels[:] = bytes((0x40, 0x70, 0x20)) * (width * height)
    header = bytearray(54)
    struct.pack_into("<2sIHHI", header, 0, b"BM", 54 + len(pixels), 0, 0, 54)
    struct.pack_into("<IiiHHIIiiII", header, 14, 40, width, height, 1, 24,
                     0, len(pixels), 2835, 2835, 0, 0)
    return bytes(header + pixels)


def write_fixture(directory: Path) -> tuple[Path, atlas_format.Header,
                                              atlas_format.Descriptor]:
    generated = directory / "public"
    pipeline.write_public_fixture(generated)
    atlas_path = directory / "ZUNDORB.BIN"
    shutil.copyfile(generated / pipeline.ATLAS_NAME, atlas_path)
    atlas = atlas_path.read_bytes()
    header, descriptors = atlas_format.inspect_bytes(atlas)
    descriptor = descriptors[-1]
    expected = oracle.expected_registers(header, descriptor)
    for prefix in oracle.ALL_PREFIXES:
        write_registers(directory / f"{prefix}.registers.tsv", expected[prefix])

    surface, _, _ = oracle.expected_g1(atlas, descriptor)
    raw = bytearray(oracle.GVRAM_SIZE)
    raw[:oracle.G0_SIZE] = oracle.expected_g0()
    raw[oracle.G1_OFFSET:oracle.G1_OFFSET + len(surface)] = surface
    screen = bmp24(True)
    for prefix in oracle.SETTLED_PREFIXES:
        (directory / f"{prefix}.gvram.bin").write_bytes(raw)
        (directory / f"{prefix}.screen.bmp").write_bytes(screen)

    events = ["event\tframe\tid\tvalue", "initialized\t0\t-\t5000",
              "frame\t1200\t-\t1200", "input\t1200\t-\t3"]
    phase_frames = (1220, 1221, 1222, 1222, 1223)
    for ordinal, (prefix, frame) in enumerate(
            zip(oracle.ALL_PREFIXES, phase_frames), 1):
        events.extend((f"wait-pc\t{frame - 1}\t-\t1",
                       f"pc\t{frame}\t-\t{ordinal}",
                       f"capture\t{frame}\t{prefix}\t1"))
    events.append("exit\t1223\t-\t0")
    (directory / "events.tsv").write_text("\n".join(events) + "\n",
                                           encoding="utf-8")

    source = oracle.BMS_WINDOW_BASE + descriptor.bank_offset
    destination_x = (oracle.G1_WIDTH - descriptor.width) // 2
    destination_y = (200 - descriptor.height) // 2
    destination = oracle.G1_PAGE_BASE + destination_y * oracle.G1_WIDTH + destination_x
    (directory / "sgp-trace.log").write_text(
        f"SGP_SCAN: SET_SOURCE addr={source:06x} dot=0 mode=2 "
        f"width={descriptor.width} height={descriptor.height} fbw={descriptor.pitch}\n"
        f"SGP_SCAN: SET_DEST addr={destination:06x} dot=0 mode=2 "
        f"width={descriptor.width} height={descriptor.height} fbw=320\n",
        encoding="utf-8",
    )
    (directory / "ZUNDORB.LST").write_text(
        "  100                                  staging_buffer:\n"
        "  101 000033C0 00<rep 1000h>              times STAGING_BYTES db 0\n",
        encoding="utf-8",
    )
    return atlas_path, header, descriptor


class M98lOracleTests(unittest.TestCase):
    def run_case(
        self,
        mutation: Callable[[Path, atlas_format.Header,
                            atlas_format.Descriptor], Path | None] | None = None,
    ) -> tuple[int, dict[str, object]]:
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            atlas, header, descriptor = write_fixture(directory)
            source = None if mutation is None else mutation(directory, header, descriptor)
            command = [sys.executable, str(SCRIPT), str(directory),
                       "--atlas", str(atlas),
                       "--trace", str(directory / "sgp-trace.log")]
            if source is not None:
                command.extend(("--source", str(source)))
            completed = subprocess.run(command, check=False, capture_output=True, text=True)
            return completed.returncode, json.loads(completed.stdout)

    def assert_error(self, expected: str, mutation: Callable[..., Path | None]) -> None:
        returncode, result = self.run_case(mutation)
        self.assertEqual(returncode, 1)
        self.assertEqual(result["status"], "FAIL")
        self.assertEqual(result["errors"], [expected])

    def test_passing_fixture(self) -> None:
        returncode, result = self.run_case()
        self.assertEqual(returncode, 0)
        self.assertEqual(result["status"], "PASS")
        self.assertEqual(result["errors"], [])
        self.assertEqual(result["staging"]["chunk_count"], 2)
        self.assertEqual(result["sgp"]["trace"]["bms_source_count"], 1)

    def test_probe_signature_mutation(self) -> None:
        def mutate(directory: Path, header, descriptor) -> None:
            required = oracle.expected_registers(header, descriptor)["m98l-probe"]
            required = dict(required, bx="00ec")
            write_registers(directory / "m98l-probe.registers.tsv", required)
            return None
        self.assert_error("M98L_PROBE_SIGNATURE", mutate)

    def test_g1_pixel_mutation(self) -> None:
        def mutate(directory: Path, _header, _descriptor) -> None:
            for prefix in oracle.SETTLED_PREFIXES:
                path = directory / f"{prefix}.gvram.bin"
                raw = bytearray(path.read_bytes())
                raw[oracle.G1_OFFSET + 123] ^= 1
                path.write_bytes(raw)
            return None
        self.assert_error("M98L_G1_CONTENT", mutate)

    def test_direct_source_mutation(self) -> None:
        def mutate(directory: Path, _header, descriptor) -> None:
            wrong = oracle.BMS_WINDOW_BASE + descriptor.bank_offset + 16
            path = directory / "sgp-trace.log"
            text = path.read_text(encoding="utf-8")
            text = text.replace(
                f"addr={oracle.BMS_WINDOW_BASE + descriptor.bank_offset:06x}",
                f"addr={wrong:06x}", 1)
            path.write_text(text, encoding="utf-8")
            return None
        self.assert_error("M98L_TRACE_BMS_SOURCE", mutate)

    def test_second_bitblt_source_mutation(self) -> None:
        def mutate(directory: Path, _header, _descriptor) -> Path:
            path = directory / "guest.asm"
            path.write_text(ASSEMBLY.read_text(encoding="utf-8") +
                            "\nmov ax, SGP_COMMAND_BITBLT\n", encoding="utf-8")
            return path
        self.assert_error("M98L_SOURCE_BITBLT_COUNT", mutate)

    def test_staging_size_source_mutation(self) -> None:
        def mutate(directory: Path, _header, _descriptor) -> Path:
            path = directory / "guest.asm"
            source = ASSEMBLY.read_text(encoding="utf-8").replace(
                "%define STAGING_BYTES           4096",
                "%define STAGING_BYTES           8192")
            path.write_text(source, encoding="utf-8")
            return path
        self.assert_error("M98L_SOURCE_CONTRACT", mutate)

    def test_va2_near_condition_mutation(self) -> None:
        def mutate(directory: Path, _header, _descriptor) -> None:
            path = directory / "ZUNDORB.LST"
            path.write_text(path.read_text(encoding="utf-8") +
                            "  999 00000123 0F820000 jc distant\n",
                            encoding="utf-8")
            return None
        self.assert_error("M98L_VA2_INSTRUCTION_SET", mutate)

    def test_two_frame_instability_mutation(self) -> None:
        def mutate(directory: Path, _header, _descriptor) -> None:
            path = directory / "m98l-settled-b.gvram.bin"
            raw = bytearray(path.read_bytes())
            raw[-1] ^= 1
            path.write_bytes(raw)
            return None
        self.assert_error("M98L_GVRAM_UNSTABLE", mutate)

    def test_black_screen_mutation(self) -> None:
        def mutate(directory: Path, _header, _descriptor) -> None:
            for prefix in oracle.SETTLED_PREFIXES:
                (directory / f"{prefix}.screen.bmp").write_bytes(bmp24(False))
            return None
        self.assert_error("M98L_SCREEN_BLACK", mutate)

    def test_frame_limit_mutation(self) -> None:
        def mutate(directory: Path, _header, _descriptor) -> None:
            path = directory / "events.tsv"
            path.write_text(path.read_text(encoding="utf-8") +
                            "frame-limit\t5000\t-\t5000\n", encoding="utf-8")
            return None
        self.assert_error("M98L_EVENTS_TIMEOUT", mutate)


if __name__ == "__main__":
    unittest.main()
