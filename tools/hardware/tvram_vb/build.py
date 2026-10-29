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

"""Build source-only TSPSPR chunk/entry diagnostics, optionally on MS-DOS 4.

The committed demo and its distribution are never changed. Every variant
uses a five-byte entry slot and fixed-width labels, so changing chunk size
or edge policy does not move instructions/data between variants. This is a
measurement kit, NOT an established hardware cause or a timing correction.
"""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[3]
DEMO = ROOT / "demos/tsp-sprites"
SIZES = (8, 32, 128, 256)


class ProbeError(ValueError):
    pass


def require(condition, code):
    if not condition:
        raise ProbeError(code)


def replace_once(source, old, new):
    require(source.count(old) == 1, "M142_TVVB_SOURCE_ANCHOR")
    return source.replace(old, new)


def variant(source, size, edge):
    require(size in SIZES and type(size) is int, "M142_TVVB_CHUNK")
    require(type(edge) is bool, "M142_TVVB_ENTRY")
    name = "V%03d%s" % (size, "E" if edge else "A")
    require(source.count("%macro VB_CHUNKS 1") == 1, "M142_TVVB_SOURCE_ANCHOR")
    start = source.index("%macro VB_CHUNKS 1")
    end = source.index("%endmacro", start)
    macro = source[start:end]
    macro = replace_once(macro, "mov\tcx,64", "mov\tcx,%d" % (size // 2))
    slot = ("%%fall:\t\tin\tal,dx\n\t\ttest\tal,40h\n\t\tjnz\tshort %%fall\n" if edge else
            "\t\ttimes 5 nop\n")
    macro = replace_once(macro, "%%wait:\t\tin\tal,dx", slot + "%%wait:\t\tin\tal,dx")
    source = source[:start] + macro + source[end:]
    for row, target, suffix in (("Q", "TV", "31   "), ("R", "G1", "     "), ("W", "TV", "S200 ")):
        old = "'%s %s VB 128B %s'" % (row, target, suffix)
        new = "'%s %s VB %03dB %s'" % (row, target, size, suffix)
        source = replace_once(source, old, new)
    source = replace_once(source, ".screens:\tcall\tposition",
                          "\t\tcall\ttvram_screen\n.screens:\tcall\tposition")
    source = replace_once(source, "'TSPSPR 5 MEASURE'", "'%s 5 MEASURE '" % name)
    source = replace_once(source, "'TSPSPR 5 DONE  SPACE: AGAIN FROM 1'",
                          "'%s 5 DONE  SPACE: AGAIN FROM 1'" % name.ljust(6))
    require(len(name + " 5 MEASURE ") == len("TSPSPR 5 MEASURE"), "M142_TVVB_LABEL_WIDTH")
    return name, source


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def validate_manifest(directory, data):
    require(data.get("schema") == "m142-tvram-vb-v1", "M142_TVVB_SCHEMA")
    expected = {"TSPSPR.COM"} | {"V%03d%s.COM" % (size, mode) for size in SIZES for mode in "AE"}
    require(set(data["programs"]) == expected, "M142_TVVB_PROGRAM_SET")
    for name, record in data["programs"].items():
        require(sha(directory / "payload/root" / name) == record["sha256"], "M142_TVVB_PROGRAM_HASH")


def main():
    parser = argparse.ArgumentParser(__doc__)
    parser.add_argument("--output", type=Path, required=True, help="new out-of-tree directory")
    parser.add_argument("--msdos4", type=Path, help="pinned msdos4-va.2 source disk (optional)")
    parser.add_argument("--nasm", default="nasm")
    args = parser.parse_args()
    out = args.output.resolve()
    require(out != ROOT and ROOT not in out.parents, "M142_TVVB_OUT_OF_TREE")
    require(not out.exists(), "M142_TVVB_OUTPUT_EXISTS")
    if args.msdos4:
        require(sha(args.msdos4) == "7c4b141d31e0034120e0b07eb93b9bea808e1c54822e48e15189cc7398385db0",
                "M142_TVVB_SYSTEM_HASH")
    (out / "payload/root").mkdir(parents=True)
    (out / "source").mkdir()
    subprocess.run([sys.executable, "-I", str(DEMO / "gen_data.py"), str(out / "source/data.inc")], check=True)
    original = (DEMO / "tspspr.asm").read_text(encoding="utf-8")
    sources = [("TSPSPR", original, None, None)]
    for size in SIZES:
        for edge in (False, True):
            name, text = variant(original, size, edge)
            sources.append((name, text, size, edge))
    data = {"schema": "m142-tvram-vb-v1", "hardware": "NOT_RUN",
            "source_sha256": sha(DEMO / "tspspr.asm"), "programs": {},
            "nasm": subprocess.check_output([args.nasm, "-v"], text=True).strip()}
    for name, text, size, edge in sources:
        source = out / "source" / (name.lower() + ".asm")
        program = out / "payload/root" / (name + ".COM")
        source.write_text(text, encoding="utf-8")
        subprocess.run([args.nasm, "-f", "bin", "-I" + str(out / "source") + "/",
                        str(source), "-o", str(program)], check=True)
        data["programs"][program.name] = {"sha256": sha(program), "size": program.stat().st_size,
                                          "chunk_bytes": size, "edge_aligned": edge}
    # Fixed-width variants must retain the exact same image layout/length.
    lengths = {r["size"] for n, r in data["programs"].items() if n != "TSPSPR.COM"}
    require(len(lengths) == 1, "M142_TVVB_LAYOUT")
    validate_manifest(out, data)
    help_text = ("TVRAM/VB diagnostic; MS-DOS 4.0; synthetic source-built data only.\r\n"
                 "TSPSPR 1 is the unchanged baseline. V008A/V032A/V128A/V256A 1\r\n"
                 "test current-VB entry; suffix E waits for a complete new VB.\r\n"
                 "V* starts screen 5 directly; baseline needs four SPACE presses.\r\n"
                 "Photograph after DONE; RESET to return to MS-DOS.\r\n"
                 "Each title identifies the variant and the ROM-reported machine.\r\n"
                 "Record every row, SYNC, machine, pass count. RESET between COMs.\r\n"
                 "A/E variants contain the same-length entry slot; only the\r\n"
                 "baseline has no added slot. Do not equate chunks with byte rate.\r\n"
                 "No hardware measurements or fitted emulator correction implied.\r\n")
    (out / "payload/root/TVVB.TXT").write_bytes(help_text.encode("ascii"))
    if args.msdos4:
        tool = ROOT / "tools/pc88va/pcengine_disk.py"
        image = out / "tvram-vb-msdos4.d88"
        subprocess.run([sys.executable, "-I", str(tool), "vanilla", "--source", str(args.msdos4),
                        "--output", str(image)], check=True)
        subprocess.run([sys.executable, "-I", str(tool), "install", "--image", str(image),
                        "--payload", str(out / "payload")], check=True)
        data["image_sha256"] = sha(image)
    (out / "manifest.json").write_text(json.dumps(data, indent=2) + "\n", encoding="utf-8")
    print(out / "manifest.json")


if __name__ == "__main__":
    try:
        main()
    except (ValueError, OSError, subprocess.CalledProcessError) as error:
        print(str(error), file=sys.stderr)
        sys.exit(1)
