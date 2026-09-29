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

"""Parsers for the M101 probe outputs, shared by host_reference and compare."""

from __future__ import annotations

import re
import zlib
from dataclasses import dataclass
from pathlib import Path

PROFILES = ("upd780", "upd9002")
DUMP_NAMES = ("DAA", "CPL", "SCF", "CCF")
DUMP_SIZE = 32768
F_BITS = (("S", 0x80), ("Z", 0x40), ("Y", 0x20), ("H", 0x10), ("X", 0x08),
          ("P/V", 0x04), ("N", 0x02), ("C", 0x01))
FI_TO_F = (0x80, 0x40, 0x10, 0x04, 0x02, 0x01)  # fi bit 5..0 -> F bit


class ResultError(Exception):
    def __init__(self, code: str, message: str) -> None:
        super().__init__(f"{code}: {message}")
        self.code = code


def find_file(directory: Path, name: str) -> Path:
    """Find a CP/M output file by case-insensitive name."""
    for path in directory.iterdir():
        if path.name.upper() == name.upper():
            return path
    raise ResultError("FILE_MISSING", f"{name} not found in {directory}")


def read_text(directory: Path, name: str) -> str:
    data = find_file(directory, name).read_bytes()
    end = data.find(b"\x1a")
    if end >= 0:
        data = data[:end]
    return data.decode("ascii").replace("\r\n", "\n")


# ---- FLAGPRB

_PROBE = re.compile(
    r"^(P\w+) IN A=(\w\w) F=(\w\w) BC=(\w{4}) DE=(\w{4}) HL=(\w{4}) "
    r"OUT A=(\w\w) F=(\w\w) BC=(\w{4}) DE=(\w{4}) HL=(\w{4})(?: MEM=(\w\w))?$"
)
_EXPECT = re.compile(r"^# (P\w+) .*: UPD780 (.*); UPD9002 (.*)$")


@dataclass
class Probe:
    ident: str
    inputs: dict
    outputs: dict


def parse_flagprb(text: str) -> tuple[list[Probe], dict]:
    probes = []
    expected = {}
    for line in text.splitlines():
        match = _PROBE.match(line)
        if match:
            g = match.groups()
            inputs = dict(A=g[1], F=g[2], BC=g[3], DE=g[4], HL=g[5])
            outputs = dict(A=g[6], F=g[7], BC=g[8], DE=g[9], HL=g[10])
            if g[11] is not None:
                outputs["MEM"] = g[11]
            probes.append(Probe(g[0], inputs, outputs))
            continue
        match = _EXPECT.match(line)
        if match:
            expected[match.group(1)] = {
                "upd780": dict(item.split("=") for item in match.group(2).split()),
                "upd9002": dict(item.split("=") for item in match.group(3).split()),
            }
            continue
        if line and not line.startswith("#"):
            raise ResultError("FLAGPRB_FORMAT", f"unexpected line: {line}")
    if not probes:
        raise ResultError("FLAGPRB_FORMAT", "no probe lines")
    return probes, expected


def expected_field(probe: Probe, field: str) -> str:
    """Map an expectation field to the output value it describes."""
    if field == "C":
        return probe.outputs["BC"][2:]
    return probe.outputs[field]


# ---- ZEX-style reports

_GROUP = re.compile(r"^(.{30})  (OK|ERROR \*\*\*\* crc expected:(\w{8}) found:(\w{8}))$")


@dataclass
class Group:
    name: str
    ok: bool
    expected: str | None
    found: str | None


def parse_zex(text: str) -> list[Group]:
    groups = []
    for line in text.splitlines():
        match = _GROUP.match(line)
        if match:
            groups.append(Group(match.group(1).rstrip("."), match.group(2) == "OK",
                                match.group(3), match.group(4)))
    if not groups:
        raise ResultError("ZEX_FORMAT", "no test group lines")
    if "Tests complete" not in text:
        raise ResultError("ZEX_INCOMPLETE", "the run did not complete")
    return groups


# Expected CRCs of the ZEX13S groups in table order: the two stock ZEXDOC
# controls, then the uPD780 host values recorded in zex13s.patch.
ZEX13S_EXPECTED = (
    "48799360", "9b4ba675",
    "a4611558", "3551c326", "a2215a02", "7e08aada",
    "01dfb77c", "835e229b", "08d20fe0", "8a539a07",
)


def found_crc(group: Group, expected: str) -> str:
    """The CRC a group computed: its expected CRC when it reported OK."""
    return expected if group.ok else group.found


def group_signature(group: Group) -> str:
    return "OK" if group.ok else f"ERROR {group.expected} {group.found}"


def load_expectation_file(path: Path) -> list[str]:
    result = []
    for line in path.read_text(encoding="utf-8").splitlines():
        line = line.split("#", 1)[0].strip()
        if line:
            number, _, value = line.partition(" ")
            if int(number) != len(result) + 1:
                raise ResultError("EXPECT_FORMAT", f"{path}: out-of-order group {number}")
            result.append(value)
    return result


# ---- DAADUMP

def dump_records(data: bytes) -> list[tuple[int, int, int, int]]:
    """Return (fi, a, A_out, F_out) per record."""
    if len(data) != DUMP_SIZE:
        raise ResultError("DUMP_SIZE", f"dump is {len(data)} bytes, expected {DUMP_SIZE}")
    return [(index >> 8, index & 0xFF, data[2 * index], data[2 * index + 1])
            for index in range(DUMP_SIZE // 2)]


def f_in(fi: int) -> int:
    value = 0
    for bit, mask in enumerate(reversed(FI_TO_F)):
        if fi >> bit & 1:
            value |= mask
    return value


def parse_daadump_txt(text: str) -> dict[str, tuple[int, str]]:
    result = {}
    for line in text.splitlines():
        match = re.match(r"^(\w+)\.BIN (\d+) ([0-9A-F]{8})$", line)
        if match:
            result[match.group(1)] = (int(match.group(2)), match.group(3))
    return result


def crc32_hex(data: bytes) -> str:
    return f"{zlib.crc32(data) & 0xFFFFFFFF:08X}"
