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
"""Bounded execution of actual compiled backend bytes; all I/O is intercepted."""
import argparse
import json
import re
import struct
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from verify_reference import mz_metadata, require, sha256


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", type=Path, required=True)
    parser.add_argument("--listing", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    try:
        import unicorn as u
        from unicorn import x86_const as x
    except ImportError:
        print("HF_BACKEND_DEPENDENCY_MISSING: Unicorn required; NOT_RUN")
        return 77
    root = Path(__file__).resolve().parents[4]
    output = args.output.resolve()
    require(output != root and root not in output.parents, "HF_BACKEND_OUTPUT_IN_CHECKOUT")
    require(not output.exists(), "HF_BACKEND_OUTPUT_EXISTS")
    listing_data = args.listing.read_bytes()
    listing = listing_data.decode("ascii")
    # Only the three leaf functions before hf_platform_io_. No relocated calls.
    prefix = listing.split("hf_platform_io_:", 1)[0]
    labels = dict((name, int(offset, 16)) for offset, name in re.findall(
        r"^([0-9A-F]+)\s+(dos_in8_|dos_out8_|dos_out16_):", prefix, re.M))
    require(len(labels) == 3, "HF_BACKEND_LABELS")
    code = bytearray()
    for line in prefix.splitlines():
        match = re.match(r"^([0-9A-F]+)\s+((?:[0-9A-F]{2} )*[0-9A-F]{2})\s+\w", line)
        if match:
            require(int(match[1], 16) == len(code), "HF_BACKEND_LISTING_GAP")
            code.extend(bytes.fromhex(match[2]))
    exe = args.exe.read_bytes()
    mz = mz_metadata(exe)
    image = exe[mz["header_bytes"]:mz["declared_bytes"]]
    require(code and image.count(code) == 1, "HF_BACKEND_EXE_BINDING")
    image_offset = image.find(code)
    results = []

    def case(name, port, value, expected, result=0, pointer=0x100):
        cpu = u.Uc(u.UC_ARCH_X86, u.UC_MODE_16)
        cpu.mem_map(0x10000, 0x30000)
        cpu.mem_write(0x10000, bytes(code))
        cpu.mem_write(0x3FFFC, struct.pack("<H", 0x800))
        for reg, val in ((x.UC_X86_REG_CS, 0x1000), (x.UC_X86_REG_DS, 0x2000),
                         (x.UC_X86_REG_SS, 0x3000), (x.UC_X86_REG_SP, 0xFFFC),
                         (x.UC_X86_REG_AX, 0), (x.UC_X86_REG_DX, port),
                         (x.UC_X86_REG_BX, pointer if name == "dos_in8_" else value)):
            cpu.reg_write(reg, val)
        events = []

        def input_hook(_cpu, io_port, size, _context):
            events.append(["IN", io_port, size, 0x5A])
            return 0x5A

        def output_hook(_cpu, io_port, size, io_value, _context):
            events.append(["OUT", io_port, size, io_value])

        def interrupt(_cpu, number, _context):
            raise AssertionError(("HF_BACKEND_UNEXPECTED_INTERRUPT", number))

        cpu.hook_add(u.UC_HOOK_INSN, input_hook, None, 1, 0, x.UC_X86_INS_IN)
        cpu.hook_add(u.UC_HOOK_INSN, output_hook, None, 1, 0, x.UC_X86_INS_OUT)
        cpu.hook_add(u.UC_HOOK_INTR, interrupt)
        cpu.emu_start(0x10000 + labels[name], 0x10800, count=256)
        require(cpu.reg_read(x.UC_X86_REG_IP) == 0x800, "HF_BACKEND_NOT_RETURNED")
        require(cpu.reg_read(x.UC_X86_REG_AX) == result, "HF_BACKEND_RETURN")
        require(events == expected, "HF_BACKEND_IO_TRACE")
        if name == "dos_in8_" and result == 0:
            require(bytes(cpu.mem_read(0x20100, 1)) == b"\x5A", "HF_BACKEND_READ_VALUE")
        results.append({"function": name, "port": port, "value": value,
                        "return": result, "trace": events})

    for port in (0xD8, 0xDA, 0xDC):
        case("dos_in8_", port, 0, [["IN", port, 1, 0x5A]])
    case("dos_in8_", 0xD7, 0, [], 0xFFFF)
    case("dos_in8_", 0xD8, 0, [], 0xFFFF, pointer=0)
    for port in (0xD0, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xDA, 0xDC, 0xEE):
        value = 0 if port in (0xD0, 0xEE) else 0xA5
        case("dos_out8_", port, value, [["OUT", port, 1, value]])
    for port in (0xD0, 0xEE, 0xEC, 0xD3, 0xD9, 0xDB):
        case("dos_out8_", port, 0xA5, [], 0xFFFF)
    for port, maximum in ((0xD4, 639), (0xD6, 399)):
        case("dos_out16_", port, maximum, [["OUT", port, 2, maximum]])
        case("dos_out16_", port, maximum + 1, [], 0xFFFF)
    case("dos_out16_", 0xD5, 639, [], 0xFFFF)
    report = {"status": "PASS", "target": "Unicorn isolated compiled leaf functions",
              "unicorn_version": u.__version__, "exe_sha256": sha256(exe),
              "listing_sha256": sha256(listing_data), "leaf_bytes_sha256": sha256(code),
              "leaf_image_offset": image_offset, "leaf_bytes": len(code),
              "cases": results, "hardware": "NOT_RUN"}
    args.output.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(f"PASS {len(results)} compiled DOS backend cases; hardware NOT_RUN")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
