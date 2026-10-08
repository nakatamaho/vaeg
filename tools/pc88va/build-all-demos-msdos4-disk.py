#!/usr/bin/env python3
"""Build the bootable MS-DOS 4.0 disk with every demo, and its .d88.xz.

The system files come from the FreeDOS-88VA project's MIT-licensed MS-DOS 4.0
for the PC-88VA (release msdos4-va.2, msdos4-pc88va-2hd.d88), checked by its
published SHA-256.  The demos come from the checked-in distributions, as for
build-all-demos-bootable-disk.py.  Directory entries get a fixed time stamp,
so the same inputs give the same bytes.  The raw D88 is written outside the
repository; the compressed companion is demos/disks/all-demos-msdos4.d88.xz.
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
import struct
import tempfile
from pathlib import Path


REPOSITORY_ROOT = Path(__file__).resolve().parents[2]
BOOTABLE_BUILDER = Path(__file__).with_name("build-all-demos-bootable-disk.py")
DEFAULT_DISTRIBUTION_DIR = REPOSITORY_ROOT / "demos" / "disks"
DEFAULT_COMPRESSED_OUTPUT = DEFAULT_DISTRIBUTION_DIR / "all-demos-msdos4.d88.xz"
MSDOS4_RELEASE = "https://github.com/FreeDOS-88VA/MS-DOS/releases/tag/msdos4-va.2"
MSDOS4_SHA256 = "7c4b141d31e0034120e0b07eb93b9bea808e1c54822e48e15189cc7398385db0"
FIXED_FAT_DATE = ((2026 - 1980) << 9) | (1 << 5) | 1  # 2026-01-01
FIXED_FAT_TIME = 0
# The release's boot sector and the files a vanilla copy keeps (SHA-256).
MSDOS4_BOOT_SECTOR_SHA256 = "e6c98cc546962eb921363cf717b3309cdd544ba730a7eaadb69d2b8eac73e73f"
MSDOS4_FILES_SHA256 = {
    "IO.SYS": "e5adc498428111807873a6a1a9855e40cf7937abbdcd5d2df99f21d264649647",
    "MSDOS.SYS": "573626782a3c45f0d9e5f00af1bd0f52a7de44bea96d2a2d3d660e9ae06158d5",
    "COMMAND.COM": "a0f53401e2faa8b4c94b3b644864faddfaaced85a45e1eda3ab206988d496a05",
    "AUTOEXEC.BAT": "a4fbf991785dc2e3fd03af3a6b9872114c0e19bceb86ad381b6c531013325e01",
    "CONFIG.SYS": "51cf653c65c4f6cfb8ef2d07ea846cf823da20f92b8d578ba89731cd6c23af31",
    "LICENSE.TXT": "5941221227eb7e4bd0ed4e905762899428b9be84a1c730ce24d076da1ede1eac",
    "README.TXT": "f353c899f72b8207c1a75513c519b23e8ccb76f066a660c9911725521ed71d8b",
}


class BuildError(Exception):
    """A deterministic input or layout error."""


def load_module(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    if spec is None or spec.loader is None:
        raise BuildError(f"could not load {path}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def disk_files(disk, module, directory=None, prefix=""):
    """Map every file path on the disk to its contents."""
    helper = load_module("pc88va_all_demos_bootable_builder", BOOTABLE_BUILDER)
    files = {}
    for _, entry in helper.read_directory(disk, module, directory):
        name = module.display_name(entry[:11])
        if entry[11] & 0x08 or name in (".", ".."):
            continue
        if entry[11] & 0x10:
            files.update(disk_files(disk, module, struct.unpack_from("<H", entry, 26)[0],
                                    f"{prefix}{name}/"))
        else:
            files[prefix + name] = helper.read_file(disk, entry)
    return files


def validate_image(image, distribution_dir):
    """Content check: return stable error codes, empty when the image holds
    only the release's MS-DOS 4.0 files and the distributions' demo files."""
    helper = load_module("pc88va_all_demos_bootable_builder", BOOTABLE_BUILDER)
    module = helper.load_pcengine_module()
    try:
        disk = module.PcEngineDisk(image)  # raises unless MS-DOS or PC-Engine
    except module.DiskError:
        return ["M106_MSDOS4_NOT_A_SYSTEM_DISK"]
    errors = []
    if hashlib.sha256(disk.boot_sector).hexdigest() != MSDOS4_BOOT_SECTOR_SHA256:
        errors.append("M106_MSDOS4_BOOT_SECTOR_MISMATCH")
    expected = {}
    with tempfile.TemporaryDirectory(prefix="vaeg-all-demos-msdos4-check-") as temporary:
        payload_root = Path(temporary)
        helper.build_payload(distribution_dir, payload_root, module, basic=False)
        for path in payload_root.rglob("*"):
            if path.is_file():
                name = path.relative_to(payload_root).as_posix()
                expected[name.removeprefix("root/")] = path.read_bytes()
    stored = disk_files(disk, module)
    system = {name: stored.pop(name, None) for name in MSDOS4_FILES_SHA256}
    if any(contents is None or hashlib.sha256(contents).hexdigest() != digest
           for (name, digest), contents in zip(MSDOS4_FILES_SHA256.items(),
                                               system.values())):
        errors.append("M106_MSDOS4_SYSTEM_FILE_MISMATCH")
    if stored != expected:
        errors.append("M106_MSDOS4_DEMO_PAYLOAD_MISMATCH")
    return errors


def build_image(source, distribution_dir):
    """Return the raw D88 bytes of the bootable MS-DOS 4.0 all-demos disk."""
    if hashlib.sha256(Path(source).read_bytes()).hexdigest() != MSDOS4_SHA256:
        raise BuildError(f"source is not msdos4-pc88va-2hd.d88 of {MSDOS4_RELEASE}")
    helper = load_module("pc88va_all_demos_bootable_builder", BOOTABLE_BUILDER)
    module = helper.load_pcengine_module()
    module.fat_now = lambda: (FIXED_FAT_TIME, FIXED_FAT_DATE)
    with tempfile.TemporaryDirectory(prefix="vaeg-all-demos-msdos4-") as temporary:
        temporary_root = Path(temporary)
        payload_root = temporary_root / "payload"
        payload_root.mkdir()
        try:
            helper.build_payload(distribution_dir, payload_root, module, basic=False)
        except helper.BuildError as error:
            raise BuildError(str(error)) from error
        disk = temporary_root / "msdos4.d88"
        try:
            module.create_vanilla(str(source), str(disk))
            module.install_payload(str(disk), str(payload_root))
        except (OSError, module.DiskError) as error:
            raise BuildError(f"could not install demo payload: {error}") from error
        image = disk.read_bytes()
    if module.PcEngineDisk(image).system != "MS-DOS":
        raise BuildError("the result is not an MS-DOS system disk")
    return image


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--source", required=True, type=Path,
                        help=f"msdos4-pc88va-2hd.d88 of {MSDOS4_RELEASE}")
    parser.add_argument("--distribution-dir", type=Path, default=DEFAULT_DISTRIBUTION_DIR,
                        help="directory containing the demo .d88.xz distributions")
    parser.add_argument("--output", required=True, type=Path,
                        help="raw D88 to write, outside the repository")
    parser.add_argument("--compressed-output", type=Path, default=DEFAULT_COMPRESSED_OUTPUT,
                        help="compressed companion (default demos/disks/all-demos-msdos4.d88.xz)")
    args = parser.parse_args(argv)
    output = args.output.resolve()
    compressed_output = args.compressed_output.resolve()
    try:
        if output == REPOSITORY_ROOT or REPOSITORY_ROOT in output.parents:
            raise BuildError("the raw D88 must be written outside the repository")
        for path in (output, compressed_output):
            if path.exists():
                raise BuildError(f"output already exists: {path}")
        image = build_image(args.source.resolve(), args.distribution_dir.resolve())
        errors = validate_image(image, args.distribution_dir.resolve())
        if errors:
            raise BuildError(" ".join(errors))
        compressed = lzma.compress(image, format=lzma.FORMAT_XZ,
                                   preset=lzma.PRESET_EXTREME | 9)
        if lzma.decompress(compressed) != image:
            raise BuildError("the compressed image does not round-trip")
        output.write_bytes(image)
        compressed_output.write_bytes(compressed)
    except (BuildError, OSError) as error:
        parser.exit(1, f"error: {error}\n")
    print("Created bootable MS-DOS 4.0 all-demo D88")
    print(f"output: {output}")
    print(f"SHA-256: {hashlib.sha256(image).hexdigest()}")
    print(f"compressed: {compressed_output}")
    print(f"compressed SHA-256: {hashlib.sha256(compressed).hexdigest()}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
