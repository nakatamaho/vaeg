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

"""Focused source-transform/content tests, independent of Git or hardware."""
import copy
import importlib.util
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

spec = importlib.util.spec_from_file_location("tvram_vb_build", Path(__file__).with_name("build.py"))
build = importlib.util.module_from_spec(spec)
spec.loader.exec_module(build)
spec = importlib.util.spec_from_file_location("tvram_vb_trace", Path(__file__).with_name("trace.py"))
trace = importlib.util.module_from_spec(spec)
spec.loader.exec_module(trace)


def trace_fixture():
    lines = ["upd9002-trace-v1\n"]
    state = {"cx": 0, "di": 0}
    ordinal = 0

    def emit(ip, opcode, cost, cx=None, di=None, next_ip=None, vb=None):
        nonlocal ordinal
        if cx is not None:
            state["cx"] = cx
        if di is not None:
            state["di"] = di
        lines.append(f"begin step={ordinal:08x} cs=2000 ip={ip:04x} bytes={opcode:0<16}\n")
        if vb is not None:
            lines.append(f"event step={ordinal:08x} seq=00000000 origin=cpu kind=io-read "
                         f"address=00000142 value={vb:08x} width=01\n")
        lines.append(f"end step={ordinal:08x} cx={state['cx']:04x} di={state['di']:04x} "
                     f"ip={(next_ip if next_ip is not None else ip + 1):04x} "
                     f"flags=0000 consumed={cost:08x}\n")
        ordinal += 1

    emit(0x100, "ec", 10, vb=0)
    emit(0x100, "ec", 10, vb=0x40)
    emit(0x110, "31ff", 2, di=0)
    emit(0x112, "b90200", 2, cx=2, next_ip=0x200)
    emit(0x200, "f3a5", 33, cx=1, di=2, next_ip=0x200)
    emit(0x200, "f3a5", 33, cx=0, di=4, next_ip=0x202)
    emit(0x210, "ec", 10, vb=0x40)
    emit(0x110, "31ff", 2, di=0)
    emit(0x112, "b90200", 2, cx=2, next_ip=0x200)
    emit(0x200, "f3a5", 53, cx=0, di=4, next_ip=0x202)
    emit(0x210, "ec", 10, vb=0)
    return lines


class ProbeTrace(unittest.TestCase):
    def test_summary(self):
        result = trace.summarize(trace_fixture(), 2)
        self.assertEqual(result["steps"], 11)
        self.assertEqual(result["cost_histogram"], {53: 1, 66: 1})
        self.assertEqual(result["dispatch_histogram"], {1: 1, 2: 1})
        self.assertEqual([w["chunk_count"] for w in result["complete_windows"]], [2])
        self.assertEqual(result["complete_windows"][0]["entry_bracket_width"], 10)
        self.assertEqual(result["complete_window_cost_histogram"], {53: 1, 66: 1})
        self.assertFalse(result["trailing_partial_window"])
        self.assertEqual(result["status"], "SOFTWARE_TRACE_SUMMARY_NOT_GATE_PASS")

    def test_external_and_partial(self):
        passing = trace_fixture()
        baseline = trace.summarize(passing, 2)
        irq = ("event step=00000000 seq=00000000 origin=device kind=interrupt "
               "address=0000000a value=00000100 width=02\n")
        result = trace.summarize(passing[:1] + [irq] + passing[1:], 2)
        self.assertEqual(result["external_irq_count"], 1)
        self.assertEqual(result["instruction_cost_sum"], baseline["instruction_cost_sum"])
        self.assertEqual(result["irq_entry_cost"], "NOT_INCLUDED")
        # The final IN lacks its low read: explicitly partial, not a full window.
        changed = [line for line in passing if not line.startswith("event step=0000000a")]
        result = trace.summarize(changed, 2)
        self.assertEqual(result["complete_windows"], [])
        self.assertTrue(result["trailing_partial_window"])
        # Starting high is also not a fully bracketed window.
        changed = [line.replace("value=00000000", "value=00000040")
                   if line.startswith("event step=00000000") else line for line in passing]
        result = trace.summarize(changed, 2)
        self.assertEqual(result["complete_windows"], [])
        self.assertEqual(result["leading_partial_windows"], 1)
        self.assertEqual(len(result["partial_windows"][0]["copies"]), 2)

    def test_negatives(self):
        mutations = [
            ("HEADER", lambda s: s.replace("upd9002-trace-v1", "unknown", 1)),
            ("ORDER", lambda s: s.replace("begin step=00000000", "begin step=00000001", 1)),
            ("RECORD", lambda s: s.replace("cs=2000", "cs=nothex", 1)),
            ("SCOPE", lambda s: s.replace("cs=2000", "cs=2001", 1)),
            ("COST", lambda s: s.replace("consumed=0000000a", "consumed=ffffffff", 1)),
            ("CHUNK", lambda s: s.replace("end step=00000003 cx=0002", "end step=00000003 cx=0003", 1)),
            ("REP", lambda s: s.replace("end step=00000004 cx=0001", "end step=00000004 cx=0002", 1)),
            ("REP", lambda s: s.replace("end step=00000005 cx=0000 di=0004",
                                        "end step=00000005 cx=0000 di=0006", 1)),
            ("TRUNCATED", lambda s: s[:s.rfind("end step=")]),
            ("IRQ", lambda s: s.replace(
                "begin step=00000002",
                "event step=00000002 seq=00000000 origin=device kind=interrupt "
                "address=0000000a value=00000110 width=02\nbegin step=00000002", 1)),
            ("POLL_COUNT", lambda s: s.replace(
                "end step=00000006",
                "event step=00000006 seq=00000001 origin=cpu kind=io-read "
                "address=00000142 value=00000040 width=01\nend step=00000006", 1)),
        ]
        for code, mutate in mutations:
            with self.subTest(code=code):
                fixture = "".join(trace_fixture())
                trace.summarize(fixture.splitlines(keepends=True), 2)
                with self.assertRaises(trace.TraceError) as error:
                    trace.summarize(mutate(fixture).splitlines(keepends=True), 2)
                self.assertEqual(error.exception.code, "M142_TVVB_TRACE_" + code)
        trace.summarize(trace_fixture(), 2)
        with self.assertRaises(trace.TraceError) as error:
            trace.summarize(trace_fixture(), 0)
        self.assertEqual(error.exception.code, "M142_TVVB_TRACE_CHUNK")

    def test_cli(self):
        with tempfile.TemporaryDirectory() as name:
            fixture = Path(name) / "fixture.log"
            fixture.write_text("".join(trace_fixture()))
            command = [sys.executable, str(Path(__file__).with_name("trace.py")),
                       str(fixture), "--chunk-bytes", "4"]
            passing = subprocess.run(command, capture_output=True, text=True)
            self.assertEqual(passing.returncode, 0, passing.stderr)
            self.assertEqual(json.loads(passing.stdout)["complete_windows"][0]["chunk_count"], 2)
            command[-1] = "3"
            rejected = subprocess.run(command, capture_output=True, text=True)
            self.assertEqual(rejected.returncode, 1)
            self.assertEqual(json.loads(rejected.stderr)["error"], "M142_TVVB_TRACE_CHUNK")
            command[-1] = "4"
            self.assertEqual(subprocess.run(command, capture_output=True).returncode, 0)
            command[2] = str(Path(name) / "missing.log")
            missing = subprocess.run(command, capture_output=True, text=True)
            self.assertEqual(missing.returncode, 1)
            self.assertEqual(json.loads(missing.stderr)["error"], "M142_TVVB_TRACE_IO")


class ProbeBuild(unittest.TestCase):
    def assert_code(self, code, function, *args):
        with self.assertRaises(build.ProbeError) as error:
            function(*args)
        self.assertEqual(str(error.exception), code)

    def test_source(self):
        source = (build.DEMO / "tspspr.asm").read_text()
        for size in build.SIZES:
            for edge in (False, True):
                name, changed = build.variant(source, size, edge)
                self.assertIn("'%s 5 MEASURE '" % name, changed)
                self.assertIn("mov\tcx,%d" % (size // 2), changed)
                self.assertEqual(changed.count("times 5 nop"), 0 if edge else 1)
                self.assertEqual(changed.count("jnz\tshort %%fall"), 1 if edge else 0)
                self.assertEqual(changed.count("VB %03dB" % size), 3)
        build.variant(source, 128, False)  # passing fixture before each mutation
        self.assert_code("M142_TVVB_CHUNK", build.variant, source, 0, False)
        self.assert_code("M142_TVVB_ENTRY", build.variant, source, 128, "E")
        altered = source.replace("%macro VB_CHUNKS 1", "%macro VB_CHUNKS 2")
        self.assert_code("M142_TVVB_SOURCE_ANCHOR", build.variant, altered, 128, False)
        altered = source.replace("'Q TV VB 128B 31   '", "'Q TV VB 128B 30   '")
        self.assert_code("M142_TVVB_SOURCE_ANCHOR", build.variant, altered, 128, False)

    def test_manifest(self):
        with tempfile.TemporaryDirectory() as name:
            root = Path(name)
            programs = root / "payload/root"
            programs.mkdir(parents=True)
            names = ["TSPSPR.COM"] + ["V%03d%s.COM" % (n, mode) for n in build.SIZES for mode in "AE"]
            for file in names:
                (programs / file).write_bytes(b"independent fake fixture")
            passing = {"schema": "m142-tvram-vb-v1", "programs": {
                name: {"sha256": build.sha(programs / name)} for name in names}}
            build.validate_manifest(root, passing)
            data = copy.deepcopy(passing)
            data["schema"] = "unknown"
            self.assert_code("M142_TVVB_SCHEMA", build.validate_manifest, root, data)
            data = copy.deepcopy(passing)
            del data["programs"][names[0]]
            self.assert_code("M142_TVVB_PROGRAM_SET", build.validate_manifest, root, data)
            data = copy.deepcopy(passing)
            data["programs"][names[0]]["sha256"] = "wrong"
            self.assert_code("M142_TVVB_PROGRAM_HASH", build.validate_manifest, root, data)


if __name__ == "__main__":
    unittest.main()
