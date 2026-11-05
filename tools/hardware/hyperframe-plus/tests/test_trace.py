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
"""Small passing fixtures and single mutations; no private inputs or Git."""
import importlib.util
from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from trace_reference import compare_trace, execute, op
from verify_reference import ValidationError


class ContentTests(unittest.TestCase):
    def setUp(self):
        self.fixture = op(0xD4, 639, 2) + op(0xD8, 0x12)
        compare_trace(self.fixture, self.fixture)

    def mismatch(self, changed):
        with self.assertRaises(ValidationError) as caught:
            compare_trace(changed, self.fixture)
        self.assertEqual(caught.exception.code, "HFRE_TRACE_MISMATCH")

    def test_direction(self):
        self.mismatch(op(0xD4, 639, 2, 0) + op(0xD8, 0x12))

    def test_port(self):
        self.mismatch(op(0xD6, 639, 2) + op(0xD8, 0x12))

    def test_width(self):
        self.mismatch(op(0xD4, 639, 1) + op(0xD8, 0x12))

    def test_value(self):
        self.mismatch(op(0xD4, 638, 2) + op(0xD8, 0x12))

    def test_order(self):
        self.mismatch(op(0xD8, 0x12) + op(0xD4, 639, 2))

    def test_missing(self):
        self.mismatch(op(0xD4, 639, 2))


@unittest.skipUnless(importlib.util.find_spec("unicorn"), "Unicorn not installed: NOT_RUN")
class ExecutionTests(unittest.TestCase):
    def setUp(self):
        # Independently authored fixture: MOV DX,D8; IN AL,DX; MOV DX,DA;
        # MOV AL,12; OUT DX,AL; NOP; RET. No original program bytes are embedded.
        self.code = bytes.fromhex("BA D8 00 EC BA DA 00 B0 12 EE 90 C3")
        self.program = {"code": self.code, "ds": 0x4000}
        self.ranges = [(0, len(self.code))]
        self.reads = [(0xD8, 1, 0x34)]
        self.expected = op(0xD8, 0x34, direction=0) + op(0xDA, 0x12)
        result = execute(self.program, 0, self.ranges, reads=self.reads)
        compare_trace(result["trace"], self.expected)
        self.assertEqual(result["registers"]["SP"], 0xE002)

    def rejection(self, code, **changes):
        kwargs = {"program": self.program, "entry": 0,
                  "ranges": self.ranges, "reads": self.reads}
        kwargs.update(changes)
        with self.assertRaises(ValidationError) as caught:
            execute(**kwargs)
        self.assertEqual(caught.exception.code, code)

    def test_input_port(self):
        changed = self.code[:1] + bytes((0xD9,)) + self.code[2:]
        self.rejection("HFRE_READ_SCRIPT", program={"code": changed, "ds": 0x4000})

    def test_unused_input(self):
        self.rejection("HFRE_READ_SCRIPT", reads=self.reads + [(0xDA, 1, 0)])

    def test_code_range(self):
        self.rejection("HFRE_EXECUTION_RANGE", ranges=[(0, len(self.code) - 1)])

    def test_instruction_budget(self):
        self.rejection("HFRE_RETURN_MISSING", budget=1)

    def test_interrupt(self):
        changed = self.code[:-2] + bytes.fromhex("CD 21")
        self.rejection("HFRE_UNEXPECTED_INTERRUPT", program={"code": changed, "ds": 0x4000})

    def test_composed_worker_and_content(self):
        observed = execute(self.program, 0, self.ranges, reads=self.reads)
        compare_trace(observed["trace"], self.expected)
        # One expected-value mutation must be caught by content, not the worker.
        with self.assertRaises(ValidationError) as caught:
            compare_trace(observed["trace"], self.expected[:-2] + bytes((0x13, 0)))
        self.assertEqual(caught.exception.code, "HFRE_TRACE_MISMATCH")


if __name__ == "__main__":
    unittest.main(verbosity=2)
