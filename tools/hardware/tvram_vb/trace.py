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

"""Content-only reduction of TSPSPR VB_CHUNKS instruction traces, not a gate."""
import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
import re
import sys


class TraceError(ValueError):
    def __init__(self, suffix, detail):
        self.code = "M142_TVVB_TRACE_" + suffix
        self.detail = detail
        super().__init__(self.code)


def require(condition, suffix, detail):
    if not condition:
        raise TraceError(suffix, detail)


def integer(fields, key):
    value = fields.get(key, "")
    require(re.fullmatch(r"[0-9a-f]{1,8}", value), "RECORD", "invalid hex field: " + key)
    return int(value, 16)


def records(lines):
    """Yield complete dispatches or external events, preserving record order."""
    iterator = iter(lines)
    require(next(iterator, "").rstrip("\n") == "upd9002-trace-v1", "HEADER", "trace version")
    current = None
    ordinal = 0
    for line in iterator:
        parts = line.split()
        require(parts and parts[0] in ("begin", "event", "end"), "RECORD", "record kind")
        fields = {}
        for item in parts[1:]:
            pair = item.split("=", 1)
            require(len(pair) == 2 and pair[0] not in fields, "RECORD", "duplicate/invalid field")
            fields[pair[0]] = pair[1]
        require(integer(fields, "step") == ordinal, "ORDER", "noncontiguous dispatch ordinal")
        kind = parts[0]
        if kind == "begin":
            require(current is None, "ORDER", "nested dispatch")
            require(re.fullmatch(r"[0-9a-f]{16}", fields.get("bytes", "")), "RECORD", "opcode bytes")
            integer(fields, "cs")
            integer(fields, "ip")
            current = {"begin": fields, "events": []}
        elif kind == "event":
            require(fields.get("origin") in ("cpu", "device", "dma") and fields.get("kind"),
                    "RECORD", "event origin/kind")
            for key in ("seq", "address", "value", "width"):
                integer(fields, key)
            if current is None:
                yield {"external": fields}
            else:
                current["events"].append(fields)
        else:
            require(current is not None, "ORDER", "end without begin")
            for key in ("cx", "di", "ip", "flags", "consumed"):
                integer(fields, key)
            current["end"] = fields
            yield current
            current = None
            ordinal += 1
    require(current is None, "TRUNCATED", "unfinished dispatch")
    require(ordinal > 0, "RECORD", "empty trace")


def histogram(values):
    return dict(sorted(Counter(values).items()))


def summarize(lines, words, segment=0x2000):
    """Summarize Q/R/W loops; never infer physical timing or add IRQ clocks.

    Start tracing at a VB_CHUNKS function entry, before its first REP. A
    partially observed leading/trailing window is reported, not promoted to
    a fully bracketed window. Other trace kinds (e.g. S's pure poll loop) are
    outside this contract. This function has no filesystem/Git dependencies.
    """
    require(type(words) is int and 1 <= words <= 0xffff, "CHUNK", "16-bit positive word count")
    require(type(segment) is int and 0 <= segment <= 0xffff, "SCOPE", "16-bit segment")
    windows = []
    partials = []
    copies = []
    previous = None
    previous_poll = None
    active = None
    rep = None
    time = 0
    count = 0
    leading = 0
    irq_count = 0
    for step in records(lines):
        if "external" in step:
            if step["external"]["kind"] == "interrupt":
                irq_count += 1
                require(active is None, "IRQ", "IRQ inside polled-high copy window")
            continue
        begin, end = step["begin"], step["end"]
        require(integer(begin, "cs") == segment, "SCOPE", "unexpected code segment")
        cost = integer(end, "consumed")
        require(cost < 0x80000000, "COST", "negative/wrapped instruction cost")
        count += 1
        if begin["bytes"].startswith("f3a5"):
            require(active is not None and previous is not None, "REP", "REP before observed entry")
            require(not (integer(end, "flags") & 0x200), "IRQ", "copy must mask interrupts")
            before = integer(previous, "cx")
            after = integer(end, "cx")
            if rep is None:
                require(before == words, "CHUNK", "unexpected initial CX")
                rep = {"start": time, "cost": 0, "dispatches": 0, "ip": integer(begin, "ip")}
            require(integer(begin, "ip") == rep["ip"] and 0 <= after < before,
                    "REP", "REP continuation/CX progress")
            rep["cost"] += cost
            rep["dispatches"] += 1
            if after == 0:
                require(integer(end, "di") == 2 * words % 0x10000 and
                        integer(end, "ip") == (rep["ip"] + 2) % 0x10000,
                        "REP", "completed REP destination/return")
                rep["end"] = time + cost
                active["copies"].append(rep)
                copies.append(rep)
                rep = None
            else:
                require(integer(end, "ip") == rep["ip"], "REP", "partial REP must redispatch")
        else:
            require(rep is None, "REP", "untraced interruption of REP")
        for event in step["events"]:
            require(event["kind"] != "interrupt", "IRQ", "IRQ inside dispatch record")
            if event["kind"] != "io-read" or integer(event, "address") != 0x142:
                continue
            high = bool(integer(event, "value") & 0x40)
            if high:
                if active is None:
                    bracketed = previous_poll is not None and not previous_poll["high"]
                    active = {"first_high": time, "bracketed": bracketed,
                              "entry_bracket_width": time - previous_poll["time"] if bracketed else None,
                              "copies": [], "high_polls": []}
                active["high_polls"].append(time)
            elif active is not None:
                require(rep is None, "REP", "poll inside unfinished REP")
                if active["bracketed"]:
                    require(len(active["copies"]) == len(active["high_polls"]),
                            "POLL_COUNT", "one entry and one post-copy poll per counted chunk")
                    active["chunk_count"] = len(active["copies"])
                    active["postcopy_high_offsets"] = [p - active["first_high"]
                                                          for p in active["high_polls"][1:]]
                    active["first_low_offset"] = time - active["first_high"]
                    windows.append(active)
                else:
                    leading += 1
                    partials.append(active)
                active = None
            previous_poll = {"high": high, "time": time}
        time += cost
        previous = end
    if active is not None:
        partials.append(active)
    bracketed_copies = [copy for window in windows for copy in window["copies"]]
    return {"schema": "m142-tvram-vb-trace-summary-v1",
            "status": "SOFTWARE_TRACE_SUMMARY_NOT_GATE_PASS", "words": words,
            "steps": count, "instruction_cost_sum": time,
            "time_coordinate": "dispatched instruction costs only; polls at IN instruction begin",
            "irq_entry_cost": "NOT_INCLUDED", "external_irq_count": irq_count,
            "cost_histogram": histogram(c["cost"] for c in copies),
            "dispatch_histogram": histogram(c["dispatches"] for c in copies),
            "complete_window_cost_histogram": histogram(c["cost"] for c in bracketed_copies),
            "complete_window_dispatch_histogram": histogram(c["dispatches"] for c in bracketed_copies),
            "complete_windows": windows, "partial_windows": partials,
            "leading_partial_windows": leading,
            "trailing_partial_window": active is not None, "incomplete_rep": rep is not None}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("trace", type=Path)
    parser.add_argument("--chunk-bytes", type=int, required=True)
    args = parser.parse_args()
    try:
        require(args.chunk_bytes > 0 and args.chunk_bytes % 2 == 0, "CHUNK", "even positive bytes")
        with args.trace.open(encoding="ascii") as stream:
            result = summarize(stream, args.chunk_bytes // 2)
        result["trace_sha256"] = hashlib.sha256(args.trace.read_bytes()).hexdigest()
    except TraceError as error:
        print(json.dumps({"error": error.code, "detail": error.detail}), file=sys.stderr)
        return 1
    except (OSError, UnicodeError) as error:
        print(json.dumps({"error": "M142_TVVB_TRACE_IO", "detail": str(error)}), file=sys.stderr)
        return 1
    print(json.dumps(result, indent=2))
    return 0


if __name__ == "__main__":
    sys.exit(main())
