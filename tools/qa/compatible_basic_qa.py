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

"""Local-only compatible BASIC QA, with independent reusable content checks.

Private input paths and generated evidence belong outside the repository.
A successful worker exit or screenshot is never sufficient for PASS. Existing
TVRAM dumps can be checked without rerunning an unchanged expensive case.
This entry point has no protected-artifact or Git-topology contract: it does
not validate evidence placement in Git or approve a milestone gate.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess

from m103b_basic_boot import _fixture, screen_rows, textmem_from_dump

CONTRACT = "compatible-basic-local-qa-v1"
ROOT = Path(__file__).resolve().parents[2]
LOAD = 'LOAD "CAS:"'
PROBE = (
    '10 CLEAR ,&HAFFF', '20 CLS',
    '30 FOR I=0 TO 46:READ D:POKE &HB000+I,D:NEXT I',
    '40 A=&HB000:CALL A', '50 PRINT "IN6E=";HEX$(INP(&H6E))',
    '60 PRINT "SCORE=";PEEK(&HB100)+256*PEEK(&HB101)', '70 END',
    '100 DATA 245,197,213,229,243,33,0,0,6,8,219,64',
    '110 DATA 230,32,40,250,219,64,230,32,32,250,35,219',
    '120 DATA 64,230,32,40,249,35,219,64,230,32,32,249',
    '130 DATA 16,240,34,0,177,251,225,209,193,241,201',
)


class QaError(Exception):
    def __init__(self, code):
        super().__init__(code)
        self.code = code


def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def rows_from_dump(data):
    memory = textmem_from_dump(data)
    if memory is None:
        raise QaError("BASIC_QA_INVALID_DUMP")
    return screen_rows(memory)


def tape_result(rows):
    # A prior Ok or a typed command containing Found must not satisfy this.
    starts = [i for i, row in enumerate(rows) if row.strip() == LOAD]
    if not starts:
        raise QaError("BASIC_QA_LOAD_COMMAND_MISSING")
    tail = [row.strip() for row in rows[starts[-1] + 1:]]
    if any("error" in row.lower() for row in tail):
        raise QaError("BASIC_QA_GUEST_ERROR")
    found = next((i for i, row in enumerate(tail) if row.startswith("Found:")), None)
    if found is None:
        raise QaError("BASIC_QA_TAPE_HEADER_NOT_FOUND")
    if "Ok" not in tail[found + 1:]:
        raise QaError("BASIC_QA_TAPE_LOAD_INCOMPLETE")
    return {"load_complete": True}


def clock_result(rows, standard):
    flags = [re.fullmatch(r"IN6E=([0-9A-F]{2})", row.strip()) for row in rows]
    flags = [match.group(1) for match in flags if match]
    if flags != ["FF" if standard else "7F"]:
        raise QaError("BASIC_QA_CLOCK_IDENTITY")
    scores = [re.fullmatch(r"SCORE=\s*([0-9]+)", row.strip()) for row in rows]
    scores = [int(match.group(1)) for match in scores if match]
    if len(scores) != 1 or not 0 < scores[0] < 65536:
        raise QaError("BASIC_QA_CLOCK_SCORE")
    if not any(row.strip() == "Ok" for row in rows):
        raise QaError("BASIC_QA_CLOCK_RETURN")
    return {"score": scores[0], "standard": standard}


def compare_clocks(h_rows, s_rows):
    h = clock_result(h_rows, False)["score"]
    s = clock_result(s_rows, True)["score"]
    ratio = s / h
    if not 0.45 <= ratio <= 0.60:
        raise QaError("BASIC_QA_HS_RATIO")
    return {"h_score": h, "s_score": s, "s_to_h_ratio": ratio}


def selftest():
    tape = ["Ok", LOAD, "Found:QA", "Ok"]
    h = ["IN6E=7F", "SCORE=29802", "Ok"]
    s = ["IN6E=FF", "SCORE=15303", "Ok"]
    assert tape_result(tape)["load_complete"]
    assert compare_clocks(h, s)["s_score"] == 15303
    assert rows_from_dump(_fixture(h))[:3] == h
    cases = [
        (lambda: rows_from_dump(b"invalid"), "BASIC_QA_INVALID_DUMP"),
        (lambda: tape_result(["Ok", "Found:QA", "Ok"]), "BASIC_QA_LOAD_COMMAND_MISSING"),
        (lambda: tape_result(["Ok", LOAD, "Ok"]), "BASIC_QA_TAPE_HEADER_NOT_FOUND"),
        (lambda: tape_result(["Ok", LOAD, "Found:QA"]), "BASIC_QA_TAPE_LOAD_INCOMPLETE"),
        (lambda: tape_result(["Ok", LOAD, "Syntax error", "Found:QA", "Ok"]), "BASIC_QA_GUEST_ERROR"),
        (lambda: clock_result(["IN6E=7F", *s[1:]], True), "BASIC_QA_CLOCK_IDENTITY"),
        (lambda: clock_result([s[0], "SCORE=0", s[2]], True), "BASIC_QA_CLOCK_SCORE"),
        (lambda: clock_result(s[:-1], True), "BASIC_QA_CLOCK_RETURN"),
        (lambda: compare_clocks(h, [s[0], "SCORE=29802", s[2]]), "BASIC_QA_HS_RATIO"),
    ]
    for action, expected in cases:
        try:
            action()
        except QaError as error:
            assert error.code == expected, (expected, error.code)
        else:
            raise AssertionError(expected)
    print("PASS: content fixtures and nine exact-code controlled negatives")


def run(args):
    output = args.output_dir.resolve()
    if output == ROOT or ROOT in output.parents or output.exists():
        raise QaError("BASIC_QA_OUTPUT_MUST_BE_NEW_AND_OUT_OF_TREE")
    if not args.worker or not args.roms or not args.backup_seed:
        raise QaError("BASIC_QA_REQUIRED_INPUT_MISSING")
    if not args.evaluated_commit or not re.fullmatch(r"[0-9a-f]{40}", args.evaluated_commit):
        raise QaError("BASIC_QA_EVALUATED_COMMIT_REQUIRED")
    if args.evaluated_commit.encode("ascii") not in args.worker.read_bytes():
        raise QaError("BASIC_QA_WORKER_COMMIT_MISMATCH")
    if args.scenario == "tape-load" and not args.tape:
        raise QaError("BASIC_QA_TAPE_INPUT_MISSING")
    output.mkdir(parents=True)
    shutil.copyfile(args.backup_seed, output / "backup.bin")
    config = ("[NekoProjectII]\nMain_RAM=640\nMain_RAM_Auto=false\nCPU_Speed=100\n"
              "BootMode=1\nV1V2_Standard=" + ("true" if args.standard else "false") +
              "\nCMT_Fast=" + ("true" if args.fast else "false") + "\n")
    (output / "config.ini").write_text(config, encoding="utf-8")
    lines = [LOAD] if args.scenario == "tape-load" else [*PROBE, "RUN"]
    script = "@wait 5200\n@enter\n@wait 300\n" + "".join(
        "@text " + line + "\n@enter\n@wait 60\n" for line in lines)
    (output / "input.txt").write_text(script, encoding="ascii")
    command = [str(args.worker.resolve()), "--roms", str(args.roms.resolve()),
               "--cfg", str(output / "config.ini"), "--bkupmem", str(output / "backup.bin"),
               "--model", args.model, "--cpumult", "2", "--nowait", "--mute", "--windowed",
               "--headless-input-script", str(output / "input.txt"),
               "--screen-tvram-dump", str(output / "tvram.bin"),
               "--screenshot", f"{args.frames}:{output}/final.png"]
    for slot in ["fdd1", "fdd2", "sasi1", "sasi2", *(f"scsi{i}" for i in range(7))]:
        command.extend(["--" + slot, "none"])
    inputs = {"worker_sha256": digest(args.worker), "backup_seed_sha256": digest(args.backup_seed),
              "rom_files": {str(path.relative_to(args.roms)): digest(path)
                            for path in sorted(args.roms.rglob("*"))
                            if path.is_file() and path.suffix.lower() == ".rom"}}
    if args.tape:
        target = output / ("tape" + args.tape.suffix.lower())
        shutil.copyfile(args.tape, target)
        inputs["tape_sha256"] = digest(target)
        command.extend(["--tape", str(target)])
    metadata = {"contract": CONTRACT, "tool_sha256": digest(__file__), "argv": command,
                "evaluated_commit": args.evaluated_commit,
                "inputs": inputs, "scenario": args.scenario, "standard": args.standard,
                "fast": args.fast, "frames": args.frames, "timeout": args.timeout,
                "status": "INCOMPLETE", "headless": True,
                "scope": "guest result only; not desktop clipboard or presenter acceptance"}
    report = output / "result.json"
    report.write_text(json.dumps(metadata, indent=2) + "\n", encoding="utf-8")
    try:
        with (output / "stdout.log").open("wb") as out, (output / "stderr.log").open("wb") as err:
            result = subprocess.run(command, cwd=output, stdout=out, stderr=err,
                                    timeout=args.timeout, env={**os.environ, "SDL_AUDIODRIVER": "dummy"})
        metadata["worker_exit_status"] = result.returncode
        if result.returncode:
            raise QaError("BASIC_QA_WORKER_FAILED")
        if not (output / "tvram.bin").is_file():
            raise QaError("BASIC_QA_DUMP_MISSING")
        rows = rows_from_dump((output / "tvram.bin").read_bytes())
        metadata["content"] = tape_result(rows) if args.scenario == "tape-load" else clock_result(rows, args.standard)
        metadata["status"] = "PASS"
    except subprocess.TimeoutExpired:
        metadata.update(status="FAIL", error_code="BASIC_QA_TIMEOUT")
    except QaError as error:
        metadata.update(status="FAIL", error_code=error.code)
    finally:
        metadata["outputs"] = {file.name: digest(file) for file in output.iterdir()
                               if file.is_file() and file != report}
        report.write_text(json.dumps(metadata, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({k: metadata[k] for k in ["contract", "status", "scenario"]} |
                     ({"error_code": metadata["error_code"]} if "error_code" in metadata else {})))
    return 0 if metadata["status"] == "PASS" else 1


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--selftest", action="store_true")
    parser.add_argument("--scenario", choices=["tape-load", "hs-clock"], default="tape-load")
    parser.add_argument("--check-dump", type=Path)
    parser.add_argument("--compare-hs", type=Path, nargs=2, metavar=("H_DUMP", "S_DUMP"))
    parser.add_argument("--worker", type=Path)
    parser.add_argument("--evaluated-commit", help="full commit embedded in the evaluated worker")
    parser.add_argument("--roms", type=Path)
    parser.add_argument("--backup-seed", type=Path)
    parser.add_argument("--tape", type=Path)
    parser.add_argument("--output-dir", type=Path)
    parser.add_argument("--model", choices=["va", "va2"], default="va2")
    parser.add_argument("--standard", action="store_true", help="live S selection at reset")
    parser.add_argument("--fast", action="store_true")
    parser.add_argument("--frames", type=int, default=20000)
    parser.add_argument("--timeout", type=int, default=180)
    args = parser.parse_args()
    try:
        if args.selftest:
            selftest()
            return 0
        if args.compare_hs:
            result = compare_clocks(*(rows_from_dump(path.read_bytes()) for path in args.compare_hs))
        elif args.check_dump:
            rows = rows_from_dump(args.check_dump.read_bytes())
            result = tape_result(rows) if args.scenario == "tape-load" else clock_result(rows, args.standard)
        else:
            if not args.output_dir or args.frames <= 0 or args.timeout <= 0:
                raise QaError("BASIC_QA_RUN_ARGUMENTS")
            return run(args)
        print(json.dumps({"contract": CONTRACT, "status": "PASS", **result}))
        return 0
    except (QaError, OSError) as error:
        print(json.dumps({"contract": CONTRACT, "status": "FAIL",
                          "error_code": error.code if isinstance(error, QaError) else "BASIC_QA_IO"}))
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
