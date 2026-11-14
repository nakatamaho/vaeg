#!/usr/bin/env python3
"""Build the bootable MS-DOS 4.0 HyPER-FRAME+ sample disk and its .d88.xz.

The system files come from the FreeDOS-88VA project's MIT-licensed MS-DOS 4.0
for the PC-88VA (release msdos4-va.2, msdos4-pc88va-2hd.d88), checked by its
published SHA-256, exactly as for demos/disks/all-demos-msdos4.d88.xz.  The
payload is built here from this directory's sources: HFSHOW.COM (hfshow.asm,
NASM), TETRA.RGB (raytrace.py, a synthetic ray-traced picture) and
HFSHOW.TXT.  Directory entries get a fixed time stamp, so the same inputs give
the same bytes.  The raw D88 is written outside the repository; the compressed
companion is demos/hyperframe+/hfshow-msdos4.d88.xz.
"""

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

from __future__ import annotations

import argparse
import hashlib
import importlib.util
import lzma
import os
import subprocess
import sys
import tempfile
from pathlib import Path

DEMO_DIR = Path(__file__).resolve().parent
REPOSITORY_ROOT = DEMO_DIR.parents[1]
MSDOS4_BUILDER = REPOSITORY_ROOT / "tools" / "pc88va" / "build-all-demos-msdos4-disk.py"
DEFAULT_COMPRESSED_OUTPUT = DEMO_DIR / "hfshow-msdos4.d88.xz"
PAYLOAD_NAMES = ("HFSHOW.COM", "HFSHOW.TXT", "TETRA.RGB")


class BuildError(Exception):
    """A deterministic input or layout error."""


def load_module(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    if spec is None or spec.loader is None:
        raise BuildError(f"could not load {path}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def build_payload(root, nasm, samples):
    """Write HFSHOW.COM, HFSHOW.TXT and TETRA.RGB into root."""
    root.mkdir(parents=True, exist_ok=True)
    subprocess.run([sys.executable, str(DEMO_DIR / "raytrace.py"), str(root / "TETRA.RGB"),
                    "--samples", str(samples)], check=True)
    subprocess.run([nasm, "-f", "bin", "-o", str(root / "HFSHOW.COM"),
                    str(DEMO_DIR / "hfshow.asm")], check=True)
    text = (DEMO_DIR / "hfshow.txt").read_bytes().replace(b"\r\n", b"\n")
    (root / "HFSHOW.TXT").write_bytes(text.replace(b"\n", b"\r\n"))
    if (root / "TETRA.RGB").stat().st_size != 640 * 400 * 3:
        raise BuildError("TETRA.RGB is not 640x400x3 bytes")


def validate_image(image, payload_root, msdos4):
    """Content check: stable error codes, empty when the image holds only the
    release's boot sector and kept MS-DOS files plus this demo's payload."""
    helper = load_module("pc88va_all_demos_bootable_builder", msdos4.BOOTABLE_BUILDER)
    module = helper.load_pcengine_module()
    try:
        disk = module.PcEngineDisk(image)
    except module.DiskError:
        return ["M149_HFSHOW_NOT_A_SYSTEM_DISK"]
    errors = []
    if hashlib.sha256(disk.boot_sector).hexdigest() != msdos4.MSDOS4_BOOT_SECTOR_SHA256:
        errors.append("M149_HFSHOW_BOOT_SECTOR_MISMATCH")
    stored = msdos4.disk_files(disk, module)
    system = {name: stored.pop(name, None) for name in msdos4.MSDOS4_FILES_SHA256}
    if any(contents is None or hashlib.sha256(contents).hexdigest() != digest
           for (name, digest), contents in zip(msdos4.MSDOS4_FILES_SHA256.items(),
                                               system.values())):
        errors.append("M149_HFSHOW_SYSTEM_FILE_MISMATCH")
    expected = {name: (payload_root / name).read_bytes() for name in PAYLOAD_NAMES}
    if stored != expected:
        errors.append("M149_HFSHOW_PAYLOAD_MISMATCH")
    return errors


def build_image(source, payload_root, msdos4):
    if hashlib.sha256(Path(source).read_bytes()).hexdigest() != msdos4.MSDOS4_SHA256:
        raise BuildError(f"source is not msdos4-pc88va-2hd.d88 of {msdos4.MSDOS4_RELEASE}")
    helper = load_module("pc88va_all_demos_bootable_builder", msdos4.BOOTABLE_BUILDER)
    module = helper.load_pcengine_module()
    module.fat_now = lambda: (msdos4.FIXED_FAT_TIME, msdos4.FIXED_FAT_DATE)
    with tempfile.TemporaryDirectory(prefix="vaeg-hfshow-disk-") as temporary:
        disk = Path(temporary) / "hfshow.d88"
        module.create_vanilla(str(source), str(disk))
        module.install_payload(str(disk), str(payload_root.parent))
        image = disk.read_bytes()
    if module.PcEngineDisk(image).system != "MS-DOS":
        raise BuildError("the result is not an MS-DOS system disk")
    return image


def main(argv=None):
    msdos4 = load_module("pc88va_all_demos_msdos4_builder", MSDOS4_BUILDER)
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--source", type=Path,
                        help=f"msdos4-pc88va-2hd.d88 of {msdos4.MSDOS4_RELEASE} (default: "
                             f"the shared cache, downloaded when missing)")
    parser.add_argument("--output", required=True, type=Path,
                        help="raw D88 to write, outside the repository")
    parser.add_argument("--compressed-output", type=Path, default=DEFAULT_COMPRESSED_OUTPUT,
                        help="compressed companion (default demos/hyperframe+/"
                             "hfshow-msdos4.d88.xz)")
    parser.add_argument("--check", action="store_true",
                        help="build, and compare the payload with the committed .d88.xz "
                             "instead of writing it")
    parser.add_argument("--samples", type=int, default=3)
    args = parser.parse_args(argv)
    nasm = os.environ.get("NASM", "nasm")
    output = args.output.resolve()
    compressed_output = args.compressed_output.resolve()
    try:
        if output == REPOSITORY_ROOT or REPOSITORY_ROOT in output.parents:
            raise BuildError("the raw D88 must be written outside the repository")
        if output.exists():
            raise BuildError(f"output already exists: {output}")
        if not args.check and compressed_output.exists():
            raise BuildError(f"output already exists: {compressed_output}")
        source = args.source if args.source is not None else msdos4.cached_source()
        with tempfile.TemporaryDirectory(prefix="vaeg-hfshow-payload-") as temporary:
            payload_root = Path(temporary) / "root"
            build_payload(payload_root, nasm, args.samples)
            image = build_image(source.resolve(), payload_root, msdos4)
            errors = validate_image(image, payload_root, msdos4)
        if errors:
            raise BuildError(" ".join(errors))
        compressed = lzma.compress(image, format=lzma.FORMAT_XZ,
                                   preset=lzma.PRESET_EXTREME | 9)
        if lzma.decompress(compressed) != image:
            raise BuildError("the compressed image does not round-trip")
        if args.check:
            committed = lzma.decompress(compressed_output.read_bytes())
            if committed != image:
                raise BuildError("M149_HFSHOW_COMMITTED_IMAGE_DIFFERS")
        output.write_bytes(image)
        if not args.check:
            compressed_output.write_bytes(compressed)
    except (BuildError, OSError, subprocess.CalledProcessError) as error:
        parser.exit(1, f"error: {error}\n")
    print("Created bootable MS-DOS 4.0 HyPER-FRAME+ sample D88")
    print(f"output: {output}")
    print(f"SHA-256: {hashlib.sha256(image).hexdigest()}")
    if args.check:
        print(f"matches: {compressed_output}")
    else:
        print(f"compressed: {compressed_output}")
        print(f"compressed SHA-256: {hashlib.sha256(compressed).hexdigest()}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
