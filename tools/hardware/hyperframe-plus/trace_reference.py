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
"""Bounded, intercepted reference routines; NEVER execute a whole DOS program.

Bindings are private JSON: {"j1": {"path": "...", "sha256": "..."}, ...}.
Paths are relative to that JSON. J1/D1 are MZ files; H1 is a COM file.
All traces and private identities go to a new directory outside the checkout.
BIOS/CRT/EMS substitutions are explicit and are NOT runtime validation.
"""
import argparse
from collections import Counter
import importlib.util
import json
from pathlib import Path
import struct

from verify_reference import (EXPECTED, ValidationError, mz_metadata, require,
                              sha256, verify_hash)

BASE = 0x10000
RETURN = 0xF000
RECORD = struct.Struct("<BHBH")  # direction (0=IN, 1=OUT), port, width, value


def op(port, value, width=1, direction=1):
    return RECORD.pack(direction, port, width, value)


def word(value):
    return struct.pack("<H", value & 0xFFFF)


def compare_trace(observed, expected):
    require(observed == expected, "HFRE_TRACE_MISMATCH")


def load_inputs(bindings_path):
    bindings = json.loads(bindings_path.read_text())
    require(set(bindings) == {"j1", "d1", "h1"}, "HFRE_BINDINGS")
    programs, identities = {}, {}
    for name, binding in bindings.items():
        path = (bindings_path.parent / binding["path"]).resolve()
        data = path.read_bytes()
        verify_hash(data, binding["sha256"])
        identities[name] = {"path": str(path), "sha256": sha256(data),
                            "bytes": len(data)}
        if name == "h1":
            require(len(data) == 3320, "HFRE_PROFILE_LAYOUT")
            programs[name] = {"code": bytes(256) + data, "ds": 0x1000,
                              "source_image_base": 0, "com_origin": 256}
        else:
            mz = mz_metadata(data)
            header, size = ((512, 50976) if name == "j1" else (5120, 101332))
            require(mz["header_bytes"] == header and len(data) == size,
                    "HFRE_PROFILE_LAYOUT")
            image = data[header:mz["declared_bytes"]]
            identities[name]["mz"] = mz
            if name == "j1":
                verify_hash(data, EXPECTED["jpgvhf.exe"])
                programs[name] = {"code": image, "ds": 0x1B09,
                                  "source_image_base": 0}
            else:
                # Only the graphics segment, not the MZ entry or DOS loader.
                programs[name] = {"code": image[0x12800:0x144B0], "ds": 0x4000,
                                  "source_image_base": 0x12800}
    return programs, identities


def execute(program, entry, ranges, *, args=(), registers=None, memory=None,
            reads=(), stubs=None, budget=2000000):
    import unicorn as uc
    from unicorn import x86_const as x
    machine = uc.Uc(uc.UC_ARCH_X86, uc.UC_MODE_16)
    machine.mem_map(0, 0x100000)
    machine.mem_write(BASE, program["code"])
    ds = program["ds"]
    for reg, value in ((x.UC_X86_REG_CS, BASE >> 4), (x.UC_X86_REG_DS, ds),
                       (x.UC_X86_REG_ES, ds), (x.UC_X86_REG_SS, ds),
                       (x.UC_X86_REG_SP, 0xE000)):
        machine.reg_write(reg, value)
    machine.mem_write(ds * 16 + 0xE000, word(RETURN) + b"".join(map(word, args)))
    for name, value in (registers or {}).items():
        machine.reg_write(getattr(x, "UC_X86_REG_" + name), value)
    for offset, data in (memory or {}).items():
        machine.mem_write(ds * 16 + offset, data)
    trace = bytearray()
    pending = iter(reads)
    calls = []
    # This counter measures code-hook visits, including substituted boundaries.
    # It is neither a physical instruction/timing measurement nor an OS trace.
    state = {"returned": False, "instructions": 0}
    stubs = stubs or {}

    def code_hook(m, address, size, unused):
        pc = address - BASE
        if pc == RETURN:
            state["returned"] = True
            m.emu_stop()
            return
        state["instructions"] += 1
        if pc in stubs:
            calls.append(pc)
            value = stubs[pc]
            if value is not None:
                m.reg_write(x.UC_X86_REG_AX, value)
            sp = m.reg_read(x.UC_X86_REG_SP)
            dest = struct.unpack("<H", m.mem_read(ds * 16 + sp, 2))[0]
            m.reg_write(x.UC_X86_REG_SP, (sp + 2) & 0xFFFF)
            m.reg_write(x.UC_X86_REG_IP, dest)
            return
        require(any(start <= pc and pc + size <= end for start, end in ranges),
                "HFRE_EXECUTION_RANGE")

    def input_hook(m, port, width, unused):
        item = next(pending, None)
        if item is None or item[:2] != (port, width):
            # IN hooks must return an integer even when stopping. Raising here
            # makes ctypes additionally complain about converting None to int.
            state["error"] = "HFRE_READ_SCRIPT"
            m.emu_stop()
            return 0
        trace.extend(op(port, item[2], width, 0))
        return item[2]

    def output_hook(m, port, width, value, unused):
        trace.extend(op(port, value, width))

    def interrupt_hook(m, number, unused):
        raise ValidationError("HFRE_UNEXPECTED_INTERRUPT")

    machine.hook_add(uc.UC_HOOK_CODE, code_hook)
    machine.hook_add(uc.UC_HOOK_INSN, input_hook, None, 1, 0, x.UC_X86_INS_IN)
    machine.hook_add(uc.UC_HOOK_INSN, output_hook, None, 1, 0, x.UC_X86_INS_OUT)
    machine.hook_add(uc.UC_HOOK_INTR, interrupt_hook)
    machine.emu_start(BASE + entry, BASE + RETURN + 1, count=budget)
    if state.get("error"):
        raise ValidationError(state["error"])
    require(state["returned"], "HFRE_RETURN_MISSING")
    require(next(pending, None) is None, "HFRE_READ_SCRIPT")
    regs = {r: machine.reg_read(getattr(x, "UC_X86_REG_" + r))
            for r in ("AX", "BX", "CX", "DX", "SI", "DI", "SP")}
    return {"trace": bytes(trace), "registers": regs, "stub_calls": calls,
            "instructions": state["instructions"],
            "data": bytes(machine.mem_read(ds * 16, 0x10000))}


def special_zero(mode=0xA1):
    result = op(0xD6, 0x191, 2)
    for column in range(0x180, 0x188):
        result += op(0xD4, column, 2) + op(0xD8, 0)
    result += op(0xD0, mode) + op(0xD6, 0x193, 2)
    for column in range(0x80, 0xA0):
        result += op(0xD4, column, 2) + op(0xDA, 0)
    return result


def clear_pixels(reverse):
    result = bytearray()
    columns = range(639, -1, -1) if reverse else range(640)
    for row in range(400):
        result.extend(op(0xD6, row, 2))
        for column in columns:
            result.extend(op(0xD4, column, 2) + op(0xD8, 0))
    return bytes(result)


def expected_init(width, height, clear, hf3):
    result = op(0xEE, 0xC0) + op(0xD0, 0xA5) + op(0xD3, 0)
    if clear == 2 or (clear == 1 and (width < 640 or height < 400)):
        result += clear_pixels(True)
    result += special_zero() + op(0xD0, 0xE1)
    if not hf3 and width < 639:
        position = 640 - (640 - width) // 2
        for index, value in ((0x84, position & 255), (0x85, position >> 8),
                             (0x86, 0x8F), (0x87, 1)):
            result += op(0xD4, index, 2) + op(0xDA, value)
    return result


def cases():
    # Each case states exact entry, closed code ranges, substituted calls and I/O.
    for label, mode, w, h, c, hf3, info, ew, eh, em in (
            ("c0-full", 0, 640, 400, 0, 0, 0, 640, 400, 0),
            ("c1-equal", 0, 640, 400, 1, 0, 0, 640, 400, 0),
            ("c2-full", 0, 640, 400, 2, 0, 0, 640, 400, 0),
            ("c1-narrow-tall", 0, 639, 401, 1, 0, 0, 639, 401, 0),
            ("c1-wide-short", 0, 641, 399, 1, 0, 0, 641, 399, 0),
            ("c1-large", 0, 641, 401, 1, 0, 0, 641, 401, 0),
            ("width638", 0, 638, 400, 0, 0, 0, 638, 400, 0),
            ("hf3-width638", 0, 638, 400, 0, 1, 0, 638, 400, 0),
            ("mode1", 1, 640, 400, 0, 0, 0, 640, 400, 1),
            ("mode2", 2, 640, 400, 0, 0, 0, 320, 200, 2),
            ("mode3", 3, 640, 400, 0, 0, 0, 640, 400, 3),
            ("mode5", 5, 1280, 400, 0, 0, 0, 640, 200, 5),
            ("mode6", 6, 1280, 800, 0, 0, 0, 640, 400, 6),
            ("mode6-fallback", 6, 320, 200, 0, 0, 0, 320, 200, 0),
            ("mode8", 8, 641, 401, 0, 0, 0, 81, 51, 8),
            ("ems-call", 0, 640, 401, 0, 0, 1, 640, 401, 0),
            ("hf3-no-ems", 0, 640, 401, 0, 1, 1, 640, 401, 0)):
        memory = {a: word(v) for a, v in ((0x1490, w), (0x1492, h),
                  (0x14BE, mode), (0x14C2, c), (0x14C4, hf3), (0x14C0, info))}
        yield {"id": "j1-init-" + label, "program": "j1", "entry": 0x54CF,
               "ranges": [(0x54CF, 0x57D4), (0xA400, 0xAA00)], "memory": memory,
               "stubs": {0x5866: None, 0x57E6: None, 0x867A: 0, 0x7C01: 0xFFFF},
               "expected": expected_init(ew, eh, c, hf3),
               "data_words": {0x1494: min(ew, 640), 0x1496: eh, 0x14BE: em},
               "expected_calls": ([0x5866] + ([0x867A] if em in (2, 5, 6) else [])
                                  + [0x57E6] + ([0x7C01] if info > 0 and eh > 400
                                               and not hf3 else []))}
    for width in (638, 639):
        for hf3 in (0, 1):
            for line in (399, 400, 655):
                value = (line + (width >= 639)) % 400
                expected = (op(0xD6, 0x193, 2) + op(0xD4, 0x86, 2)
                            + op(0xDA, value & 255) + op(0xD4, 0x87, 2)
                            + op(0xDA, value >> 8)) if not hf3 else b""
                yield {"id": f"j1-scroll-{width}-{hf3}-{line}", "program": "j1",
                       "entry": 0x5474, "ranges": [(0x5474, 0x54CF)],
                       "args": (line,), "memory": {0x1494: word(width),
                                                   0x14C4: word(hf3)},
                       "stubs": {0x7E0A: 0}, "expected": expected,
                       "expected_calls": [0x7E0A] if not hf3 else []}
    for width in (638, 639):
        value = (655 + 400 + (width >= 639)) % 400
        yield {"id": f"j1-byte-scroll-{width}", "program": "j1", "entry": 0x82B6,
               "ranges": [(0x82B6, 0x831C)], "args": (655,),
               "memory": {0x1494: word(width), 0x14C4: word(0)},
               "expected": (op(0xD7, 1) + op(0xD6, 0x93) + op(0xD5, 0)
                            + op(0xD4, 0x86) + op(0xDA, value & 255)
                            + op(0xD4, 0x87) + op(0xDA, value >> 8))}
    yield {"id": "j1-off", "program": "j1", "entry": 0x581D,
           "ranges": [(0x581D, 0x582E)], "expected": op(0xD0, 0) + op(0xEE, 0)}
    for key in (0x1B, 0x20, 0x0D):
        yield {"id": f"j1-exit-key-{key:02x}", "program": "j1", "entry": 0x4D4,
               "ranges": [(0x4D4, 0x501), (0x581D, 0x582E)],
               "stubs": {0x584A: None, 0x90EF: 0, 0xAEF3: 0, 0x8107: key,
                         0x582E: None},
               "expected_calls": [0x584A, 0x90EF, 0xAEF3, 0x8107, 0x582E],
               "expected": b"" if key == 0x1B else op(0xD0, 0) + op(0xEE, 0)}
    # One real grayscale row; component samples are synthetic and not copyrighted.
    yield {"id": "j1-gray-row", "program": "j1", "entry": 0x4238,
           "ranges": [(0x4238, 0x4395)], "args": (0,),
           "memory": {0x1490: word(2), 0x1492: word(1), 0x14A4: word(1),
                      0x14BC: word(0), 0x5D6E: word(0x6000) + word(0x1B09),
                      0x6000: bytes((0x12, 0xA9))},
           "expected": (op(0xD6, 0, 2) + op(0xD0, 0xE5) + op(0xD4, 0, 2)
                        + op(0xD8, 0x12) + op(0xD4, 1, 2) + op(0xD8, 0xA9)
                        + op(0xD0, 0xE1))}
    yield {"id": "j1-rgb-row", "program": "j1", "entry": 0x3FE9,
           "ranges": [(0x3FE9, 0x4238)], "args": (0,),
           "memory": {0x1490: word(2), 0x1492: word(1), 0x14A4: word(1),
                      0x14BC: word(0), 0x5D6E: word(0x6000) + word(0x1B09),
                      0x5DEE: word(0x6010) + word(0x1B09),
                      0x5E6E: word(0x6020) + word(0x1B09),
                      0x6000: bytes((0x12, 0xA9)), 0x6010: bytes((0x34, 0xCB)),
                      0x6020: bytes((0x56, 0xED))},
           "expected": (op(0xD6, 0, 2) + op(0xD4, 0, 2) + op(0xD8, 0x12)
                        + op(0xDA, 0x34) + op(0xDC, 0x56) + op(0xD4, 1, 2)
                        + op(0xD8, 0xA9) + op(0xDA, 0xCB) + op(0xDC, 0xED))}
    for old, ack, pixel_ack in ((0, 0x81, 0x11), (0, 0, 0x11),
                                 (0, 0x81, 0xFF), (0xFF, 0xFF, 0xFF)):
        reads = [(0xD0, 1, old), (0xD0, 1, ack)]
        expected = (op(0xEE, 0xC0) + op(0xD0, old, direction=0)
                    + op(0xD0, old | 0x81) + op(0xD0, ack, direction=0))
        matched = ack == (old | 0x81)
        if matched:
            expected += op(0xD0, ack | 4)
            expected += special_zero(ack) + op(0xD4, 0, 2) + op(0xD6, 0, 2)
            reads += [(0xD8, 1, 0x5A), (0xD8, 1, pixel_ack)]
            expected += (op(0xD8, 0x5A, direction=0) + op(0xD8, 0x11)
                         + op(0xD8, pixel_ack, direction=0))
            if pixel_ack == 0x11:
                expected += op(0xD8, 0x5A)
        yield {"id": f"d1-detect-{old:02x}-{ack:02x}-{pixel_ack:02x}",
               "program": "d1", "entry": 0x8C7,
               "ranges": [(0x8C7, 0x914), (0xB16, 0xB3C)], "reads": reads,
               "expected": expected,
               "expected_registers": {"AX": 0 if matched and pixel_ack == 0x11
                                       else 0xFFFF}}
    for name, entry, value, d3 in (("on", 0x914, 0xD1, 0),
                                  ("off", 0x923, 0xC1, 0x80)):
        yield {"id": "d1-" + name, "program": "d1", "entry": entry,
               "ranges": [(entry, 0x930)], "reads": [(0xD0, 1, 0xE1)],
               "expected": op(0xD0, 0xE1, direction=0) + op(0xD0, value)
                           + op(0xD3, d3)}
    coords = op(0xD4, 639, 2) + op(0xD6, 399, 2)
    yield {"id": "d1-put", "program": "d1", "entry": 0x95C,
           "ranges": [(0x95C, 0x971)],
           "registers": {"AX": 639, "BX": 399, "CX": 0x1256, "DX": 0x34},
           "expected": coords + op(0xD8, 0x12) + op(0xDA, 0x34) + op(0xDC, 0x56)}
    yield {"id": "d1-get", "program": "d1", "entry": 0x971,
           "ranges": [(0x971, 0x984)], "registers": {"AX": 639, "BX": 399},
           "reads": [(0xD8, 1, 0x12), (0xDA, 1, 0x34), (0xDC, 1, 0x56)],
           "expected": coords + op(0xD8, 0x12, direction=0)
                       + op(0xDA, 0x34, direction=0) + op(0xDC, 0x56, direction=0),
           "expected_registers": {"CX": 0x1256, "DX": 0x34}}
    yield {"id": "d1-get-row-layout", "program": "d1", "entry": 0x9D7,
           "ranges": [(0x9D7, 0x9EE)],
           "registers": {"AX": 639, "BX": 399, "CX": 1, "DI": 0x5000},
           "reads": [(0xD8, 1, 0x12), (0xDA, 1, 0x34), (0xDC, 1, 0x56)],
           "expected": op(0xD6, 399, 2) + op(0xD4, 639, 2)
                       + op(0xD8, 0x12, direction=0) + op(0xDA, 0x34, direction=0)
                       + op(0xDC, 0x56, direction=0),
           "data_bytes": {0x5000: bytes((0x34, 0x12, 0x56))}}
    yield {"id": "d1-scroll", "program": "d1", "entry": 0x9EE,
           "ranges": [(0x9EE, 0xA36)], "registers": {"AX": 0x123, "BX": 401},
           "reads": [(0xD0, 1, 0x81)],
           "expected": (op(0xD0, 0x81, direction=0) + op(0xD0, 0xC1)
                        + op(0xD6, 0x193, 2) + op(0xD4, 0x84, 2)
                        + op(0xDA, 0x23) + op(0xD4, 0x85, 2) + op(0xDA, 1)
                        + op(0xD4, 0x86, 2) + op(0xDA, 1)
                        + op(0xD4, 0x87, 2) + op(0xDA, 0))}
    yield {"id": "d1-clear", "program": "d1", "entry": 0x930,
           "ranges": [(0x930, 0x95C)], "reads": [(0xD0, 1, 0x81)],
           "expected": op(0xD0, 0x81, direction=0) + op(0xD0, 0x85)
                       + clear_pixels(False) + op(0xD0, 0x81)}
    # A single sample prefix, ending after POP BX, before dither/VRAM output.
    yield {"id": "h1-sample-zoom8", "program": "h1", "entry": 0x337,
           "ranges": [(0x337, 0x372)], "stubs": {0x372: None},
           "registers": {"DI": 399, "CX": 3},
           "memory": {0xCBC: word(100), 0xCBE: word(10), 0xCF2: bytes(range(256))},
           "reads": [(0xD8, 1, 0x12), (0xDA, 1, 0x34), (0xDC, 1, 0x56)],
           "expected": (op(0xD6, 59, 2) + op(0xD4, 100, 2)
                        + op(0xD8, 0x12, direction=0) + op(0xDA, 0x34, direction=0)
                        + op(0xDC, 0x56, direction=0)),
           "data_bytes": {0xCCC: bytes((0x12, 0x34, 0x56))}}
    # Stop before the first PC-98 INT 18h, without faking a full BIOS.
    for old, d3 in ((0xE1, 0x12), (0xFF, 0xFF)):
        yield {"id": f"h1-acquire-{old:02x}", "program": "h1", "entry": 0x207,
               "ranges": [(0x207, 0x21C)], "stubs": {0x21C: None},
               "reads": [(0xD0, 1, old), (0xD3, 1, d3)],
               "expected": (op(0xEE, 0xC0) + op(0xD0, old, direction=0)
                            + op(0xD3, d3, direction=0) + op(0xD3, d3 | 0x80)
                            + op(0xD0, 0x81)), "data_words": {0xCD0: old * 256 + d3}}
        yield {"id": f"h1-writeback-{old:02x}", "program": "h1", "entry": 0x282,
               "ranges": [(0x282, 0x28C)], "memory": {0xCD0: word(old * 256 + d3)},
               "expected": op(0xD3, d3) + op(0xD0, old)}


def campaign(programs, output):
    results = []
    for case in cases():
        name = case["id"]
        kwargs = {k: case[k] for k in ("args", "registers", "memory", "reads", "stubs")
                  if k in case}
        observed = execute(programs[case["program"]], case["entry"],
                           case["ranges"], **kwargs)
        (output / (name + ".trace")).write_bytes(observed["trace"])
        compare_trace(observed["trace"], case["expected"])
        for reg, value in case.get("expected_registers", {}).items():
            require(observed["registers"][reg] == value, "HFRE_REGISTER_MISMATCH")
        for offset, value in case.get("data_words", {}).items():
            require(observed["data"][offset:offset + 2] == word(value),
                    "HFRE_DATA_MISMATCH")
        for offset, value in case.get("data_bytes", {}).items():
            require(observed["data"][offset:offset + len(value)] == value,
                    "HFRE_DATA_MISMATCH")
        if "expected_calls" in case:
            require(observed["stub_calls"] == case["expected_calls"],
                    "HFRE_STUB_CALL_MISMATCH")
        summary = {"id": name, "status": "PASS", "program": case["program"],
                   "entry": case["entry"], "ranges": case["ranges"],
                   "stub_calls": observed["stub_calls"],
                   "instructions": observed["instructions"],
                   "operations": len(observed["trace"]) // RECORD.size,
                   "trace_sha256": sha256(observed["trace"]),
                   "counts": dict(Counter(f"{d}:{p:04x}:{w}"
                       for d, p, w, v in RECORD.iter_unpack(observed["trace"])))}
        results.append(summary)
        (output / "cases.json").write_text(json.dumps(results, indent=2) + "\n")
        print(json.dumps({"id": name, "status": "PASS"}), flush=True)
    return results


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--bindings", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    try:
        require(importlib.util.find_spec("unicorn") is not None, "HFRE_UNICORN_MISSING")
        root = Path(__file__).resolve().parents[3]
        output = args.output.resolve()
        require(not output.is_relative_to(root), "HFRE_OUTPUT_IN_CHECKOUT")
        require(not output.exists(), "HFRE_OUTPUT_EXISTS")
        programs, identities = load_inputs(args.bindings.resolve())
        output.mkdir(parents=True)
        import unicorn
        libraries = {p.name: sha256(p.read_bytes()) for p in
                     (Path(unicorn.__file__).parent / "lib").glob("*unicorn*") if p.is_file()}
        report = {"status": "IN_PROGRESS", "hardware": "NOT_RUN",
                  "whole_program": "NOT_RUN", "inputs": identities,
                  "worker": {"unicorn": unicorn.__version__, "libraries": libraries},
                  "runner_sha256": sha256(Path(__file__).read_bytes()),
                  "validator_sha256": sha256(Path(__file__).with_name("verify_reference.py").read_bytes()),
                  "record_format": "<BHBH; 0=IN, 1=OUT; no timestamps"}
        (output / "report.json").write_text(json.dumps(report, indent=2) + "\n")
        results = campaign(programs, output)
        report.update(status="SOFTWARE_TRACE_PASS", cases=len(results))
        (output / "report.json").write_text(json.dumps(report, indent=2) + "\n")
        return 0
    except (ValidationError, OSError, ValueError, KeyError) as error:
        code = getattr(error, "code", "HFRE_INPUT_OR_RUNTIME_ERROR")
        print(json.dumps({"status": "NOT_RUN" if code == "HFRE_UNICORN_MISSING" else "FAIL",
                          "code": code}))
        return 77 if code == "HFRE_UNICORN_MISSING" else 1


if __name__ == "__main__":
    raise SystemExit(main())
