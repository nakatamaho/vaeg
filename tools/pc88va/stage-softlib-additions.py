#!/usr/bin/env python3
# Copyright (c) 2026 Nakata Maho
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
# 1. Redistributions of source code must retain the above copyright notice,
#    this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
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

"""Stage the M126 SASI-only packages; public inputs and DOS outputs are pinned."""

import argparse
import hashlib
import os
from pathlib import Path
import subprocess
import tempfile
import zipfile


INPUTS = {
    "TENIM3.COM": "f7b89107dc0a6a7c4ce53547418400f68a62bc4d2206a97f02489dfb1d556eb8",
    "TENIM3.DOC": "e369fd63be567e19080fc89be41e43787bb32d9742a8be2f3995826f8d731fb6",
    "PCODESRC.LZH": "68e67920e507561b940ee88ac5a3c41afd17342c4b82c8f2124778d0aef93d31",
    "VA3DDEMO.ASM": "ad8f142d7b774b536bcd6b76c25fe6276b5e25fcb387236169c775dbacc33d2a",
    "VA3DDEMO.DOC": "ff416a82b52062a9d818f46e28fbaf7e73923728a5a61387736acdb9bedaaed5",
    "JWasm_v220_dos.zip": "e4cab76e0cdc038e4bc284be136cbd0e5116b02a0a2a76fc4a12cad326224723",
}
OUTPUTS = {
    "VA3DDEMO.EXE": "68ee8485848b0a38d61987be707eb8927738ac6de1db237b916768be8db7559d",
    "PCODE.COM": "c75b878c26cecfc4a4f063f9dd2e43763f00f34d571af659c19e52699a16b229",
}
TENNIS_FILES = (
    "HAIKEI.IMG", "LOADBACK.ASM", "LOADBACK.BIN", "MANEKIR1.PAT",
    "MANEKIR2.PAT", "MANEKIR3.PAT", "MANEKIR4.PAT", "MANEKIR5.PAT",
    "NEC_MAIL.DOC", "TE.PAT", "TENIM3.BAS",
)


class PackageError(Exception):
    """An explicit packaging invariant failed."""


def verified(path, expected, code):
    contents = path.read_bytes()
    if hashlib.sha256(contents).hexdigest() != expected:
        raise PackageError(f"{code}: {path.name}")
    return contents


def basic_files():
    """8.3 guest names, unchanged ASCII BASIC apart from DOS line endings."""
    root = Path(__file__).resolve().parents[2]
    files = {}
    for source, target in (("fpu-test/fputest.bas", "FPUTEST.BAS"),
                           ("mandelbrot/mandelbrot.bas", "MANDEL.BAS")):
        text = (root / "demos" / source).read_text(encoding="ascii")
        files[f"BASIC/{target}"] = text.replace("\r\n", "\n").replace("\n", "\r\n").encode("ascii")
    instructions = (
        "BASIC samples from vaeg (BSD-2-Clause; notices in each .BAS)\n"
        "CD \\BASIC\nBASIC /87\n"
        'LOAD "FPUTEST.BAS"\nRUN\n'
        "Expected: failed: 0. Enable the 8087 to test it with /87.\n"
        'LOAD "MANDEL.BAS",A\nRUN\n'
        "Mandelbrot also runs without /87; the full image takes time.\n"
        "TENIM3: CD \\TENIM3, then BASIC TENIM3.BAS (mouse required).\n"
        "VA3DDEMO: VA3DDEMO from the DOS prompt; see DOC\\VA3DDEMO.DOC.\n"
        "Known issue: VA3DDEMO may draw nothing in vaeg (unresolved).\n"
        "PCODE is BASIC-version-specific; it is NOT loaded automatically.\n"
    )
    files["DOC/BASIC.TXT"] = instructions.replace("\n", "\r\n").encode("ascii")
    return files


def stage(packages, output):
    # Validate every input before extracting archives, running DOS code or
    # writing the destination. These archives are not arbitrary user inputs.
    inputs = {name: verified(packages / name, digest, "M126_INPUT_HASH")
              for name, digest in INPUTS.items()}
    with tempfile.TemporaryDirectory(prefix="vaeg-softlib-") as scratch:
        work = Path(scratch)
        tennis = work / "tennis"
        pcode = work / "pcode"
        tennis.mkdir()
        pcode.mkdir()
        subprocess.run(["lha", f"xfw={tennis}", str(packages / "TENIM3.COM")],
                       check=True, stdout=subprocess.DEVNULL)
        subprocess.run(["lha", f"xfw={pcode}", str(packages / "PCODESRC.LZH")],
                       check=True, stdout=subprocess.DEVNULL)
        source = inputs["VA3DDEMO.ASM"]
        old = b"stack segment stack stack 'stack'"
        if source.count(old) != 1:
            raise PackageError("M126_SOURCE_LAYOUT: VA3DDEMO.ASM")
        # Only the build copy changes: JWasm rejects the duplicate combine
        # keyword. The archived original, including its encoding, is retained.
        (work / "VA3DDEMO.ASM").write_bytes(
            source.replace(old, b"stack segment stack 'stack'"))
        (work / "PCODE.ASM").write_bytes((pcode / "pcode.asm").read_bytes())
        with zipfile.ZipFile(packages / "JWasm_v220_dos.zip") as archive:
            (work / "JWASMR.EXE").write_bytes(archive.read("JWasmr.exe"))
        config = work / "dosbox.conf"
        config.write_text("[sblaster]\nsbtype=none\n[speaker]\npcspeaker=false\n",
                          encoding="ascii")
        env = dict(os.environ, SDL_VIDEODRIVER="dummy", SDL_AUDIODRIVER="dummy")
        subprocess.run([
            "dosbox", "-conf", str(config), "-exit",
            "-c", f"mount c {work}", "-c", "c:",
            "-c", "jwasmr -q -mz -FoVA3DDEMO.EXE VA3DDEMO.ASM > va3d.log",
            "-c", "jwasmr -q -bin -FoPCODE.COM PCODE.ASM > pcode.log",
            "-c", "exit",
        ], check=True, timeout=60, env=env, stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL)
        # DOSBox's exit status does not report an assembler failure.
        binaries = {name: verified(work / name, digest, "M126_OUTPUT_HASH")
                    for name, digest in OUTPUTS.items()}
        files = {
            "ARCHIVE/TENIM3.COM": inputs["TENIM3.COM"],
            "ARCHIVE/PCODESRC.LZH": inputs["PCODESRC.LZH"],
            "ARCHIVE/VA3DDEMO.ASM": source,
            "DOC/TENIM3.DOC": inputs["TENIM3.DOC"],
            "DOC/VA3DDEMO.DOC": inputs["VA3DDEMO.DOC"],
            "SRC/PCODE/PCODE.ASM": (work / "PCODE.ASM").read_bytes(),
        }
        files.update({f"BIN/{name}": contents for name, contents in binaries.items()})
        files.update({f"TENIM3/{name}": (tennis / name.lower()).read_bytes()
                      for name in TENNIS_FILES})
        files.update(basic_files())
        for name, contents in files.items():
            path = output / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(contents)
    return files


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--packages", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    try:
        files = stage(args.packages.resolve(), args.output.resolve())
    except (PackageError, OSError, subprocess.SubprocessError, zipfile.BadZipFile) as error:
        parser.exit(1, f"{error}\n")
    print(f"Staged {len(files)} M126 Softlib files")


if __name__ == "__main__":
    main()
