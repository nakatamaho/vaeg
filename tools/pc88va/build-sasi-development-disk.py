#!/usr/bin/env python3
"""Build a boot-layout 40 MB PC-88VA SASI HDI development disk.

The input is a PC-Engine 1.05 or 1.1 system D88.  The generated HDI uses the
40 MB SASI geometry implemented by fdd/sxsi.c and places the four boot system
files in the conventional first clusters.  Every COM and EXE found anywhere
on the source D88 is copied into ``\\BIN``.

This is a host-side image builder.  It does not require a running emulator or
DOSBox; the generated image contains the source PC-Engine IPL and the FAT12
layout used by the PC-88VA SASI formatter.  Real-machine boot validation
remains a separate gate.
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
import os
import re
import struct
import tempfile
from datetime import datetime
from pathlib import Path


REPOSITORY_ROOT = Path(__file__).resolve().parents[2]
PCENGINE_DISK_PATH = Path(__file__).with_name("pcengine_disk.py")

HEADER_SIZE = 4096
SECTOR_SIZE = 256
SECTORS = 33
SURFACES = 8
CYLINDERS = 615
DATA_SIZE = SECTOR_SIZE * SECTORS * SURFACES * CYLINDERS
IMAGE_SIZE = HEADER_SIZE + DATA_SIZE

# HDFORM's 40 MB SASI layout, expressed in 256-byte physical blocks.
BOOT_BLOCKS = 4                 # one 1024-byte IPL sector
FAT_BLOCKS = 16                 # one 4096-byte FAT copy
# HDFORM reserves 96 KiB for the root directory (3,072 32-byte entries).
# The large root area is why the first data cluster begins at 01B400h in a
# formatted 40 MB SASI image.
ROOT_BLOCKS = 384
FAT1_OFFSET = HEADER_SIZE + BOOT_BLOCKS * SECTOR_SIZE
FAT2_OFFSET = FAT1_OFFSET + FAT_BLOCKS * SECTOR_SIZE
ROOT_OFFSET = FAT2_OFFSET + FAT_BLOCKS * SECTOR_SIZE
DATA_OFFSET = ROOT_OFFSET + ROOT_BLOCKS * SECTOR_SIZE
CLUSTER_SIZE = 16 * 1024
CLUSTER_COUNT = (IMAGE_SIZE - DATA_OFFSET) // CLUSTER_SIZE
ROOT_ENTRIES = (ROOT_BLOCKS * SECTOR_SIZE) // 32
FAT_SIZE = FAT_BLOCKS * SECTOR_SIZE

SYSTEM_FILES = ("ENGINEIO.SYS", "PCENGINE.SYS", "ADVGBIOS.SYS", "PCENGINE.COM")
SYSTEM_ATTRIBUTES = {
    "ENGINEIO.SYS": 0x27,
    "PCENGINE.SYS": 0x27,
    "ADVGBIOS.SYS": 0x27,
    "PCENGINE.COM": 0x21,
}


class BuildError(Exception):
    """A deterministic input or layout error."""


def load_pcengine_module():
    spec = importlib.util.spec_from_file_location(
        "pc88va_sasi_pcengine_disk", PCENGINE_DISK_PATH)
    if spec is None or spec.loader is None:
        raise BuildError(f"could not load {PCENGINE_DISK_PATH}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def d88_file(disk, components: list[str], module) -> bytes:
    directory = disk.root
    for index, component in enumerate(components):
        offset, exists = module.find_entry(directory, module.short_name(component))
        if not exists:
            raise BuildError("source D88 is missing " + "/".join(components))
        entry = bytes(directory[offset:offset + 32])
        cluster = struct.unpack_from("<H", entry, 26)[0]
        if index + 1 < len(components):
            if not entry[11] & 0x10:
                raise BuildError("D88 path component is not a directory: " + component)
            directory_bytes = bytearray()
            for item in disk.cluster_chain(cluster):
                directory_bytes.extend(disk.read_cluster(item))
            directory = directory_bytes
            continue
        if entry[11] & 0x10:
            raise BuildError("D88 path names a directory: " + "/".join(components))
        size = struct.unpack_from("<I", entry, 28)[0]
        payload = bytearray()
        for item in disk.cluster_chain(cluster):
            payload.extend(disk.read_cluster(item))
        return bytes(payload[:size])
    raise BuildError("D88 path does not name a file")


def collect_executables(disk, module, directory=None, prefix=()):
    if directory is None:
        directory = disk.root
    files = []
    for _, entry in module.iter_entries(directory):
        raw_name = entry[:11]
        if entry[11] & 0x08:
            continue
        name = module.display_name(raw_name)
        if name in (".", ".."):
            continue
        path = prefix + (name,)
        if entry[11] & 0x10:
            cluster = struct.unpack_from("<H", entry, 26)[0]
            contents = bytearray()
            for item in disk.cluster_chain(cluster):
                contents.extend(disk.read_cluster(item))
            files.extend(collect_executables(disk, module, contents, path))
            continue
        if name.upper().endswith((".COM", ".EXE")):
            files.append((path, d88_file(disk, list(path), module)))
    return files


def fat_set(fat: bytearray, cluster: int, value: int) -> None:
    offset = cluster + cluster // 2
    value &= 0xFFF
    if offset + 1 >= len(fat):
        raise BuildError(f"FAT cluster is outside the image: {cluster}")
    if cluster & 1:
        fat[offset] = (fat[offset] & 0x0F) | ((value & 0x0F) << 4)
        fat[offset + 1] = (value >> 4) & 0xFF
    else:
        fat[offset] = value & 0xFF
        fat[offset + 1] = (fat[offset + 1] & 0xF0) | ((value >> 8) & 0x0F)


def fat_get(fat: bytes, cluster: int) -> int:
    offset = cluster + cluster // 2
    pair = fat[offset] | (fat[offset + 1] << 8)
    if cluster & 1:
        pair >>= 4
    return pair & 0xFFF


def fat_now() -> tuple[int, int]:
    now = datetime.now()
    year = min(max(now.year, 1980), 2107)
    date = ((year - 1980) << 9) | (now.month << 5) | now.day
    time = (now.hour << 11) | (now.minute << 5) | (now.second // 2)
    return time, date


def make_entry(name: bytes, attributes: int, cluster: int, size: int) -> bytes:
    fat_time, fat_date = fat_now()
    entry = bytearray(32)
    entry[:11] = name
    entry[11] = attributes
    struct.pack_into("<H", entry, 14, fat_time)
    struct.pack_into("<H", entry, 16, fat_date)
    struct.pack_into("<H", entry, 18, fat_date)
    struct.pack_into("<H", entry, 22, fat_time)
    struct.pack_into("<H", entry, 24, fat_date)
    struct.pack_into("<H", entry, 26, cluster)
    struct.pack_into("<I", entry, 28, size)
    return bytes(entry)


class HdiBuilder:
    def __init__(self, boot: bytes, module):
        if CLUSTER_COUNT < 1:
            raise BuildError("40 MB layout has no data clusters")
        if len(boot) > BOOT_BLOCKS * SECTOR_SIZE:
            raise BuildError("source PC-Engine IPL exceeds one 1024-byte sector")
        self.module = module
        self.image = bytearray(IMAGE_SIZE)
        self.fat = bytearray(FAT_SIZE)
        # HDFORM leaves a zero first byte in each unused root entry and uses
        # E5h for the remaining bytes.  A populated entry replaces its whole
        # 32-byte slot, so DOS still sees the first following zero as the end
        # of the directory.
        self.root = bytearray()
        for _ in range(ROOT_ENTRIES):
            self.root.extend(b"\x00" + b"\xE5" * 31)
        self.next_cluster = 2

        struct.pack_into("<I", self.image, 8, HEADER_SIZE)
        struct.pack_into("<I", self.image, 12, DATA_SIZE)
        struct.pack_into("<I", self.image, 16, SECTOR_SIZE)
        struct.pack_into("<I", self.image, 20, SECTORS)
        struct.pack_into("<I", self.image, 24, SURFACES)
        struct.pack_into("<I", self.image, 28, CYLINDERS)

        boot_sector = bytearray(BOOT_BLOCKS * SECTOR_SIZE)
        boot_sector[:len(boot)] = boot
        self.image[HEADER_SIZE:HEADER_SIZE + len(boot_sector)] = boot_sector
        self.fat[:3] = b"\xD3\xFF\xFF"

    def allocate(self, contents: bytes) -> int:
        count = max(1, (len(contents) + CLUSTER_SIZE - 1) // CLUSTER_SIZE)
        first = self.next_cluster
        last = first + count - 1
        if last > CLUSTER_COUNT + 1:
            raise BuildError("40 MB SASI image has insufficient free clusters")
        for cluster in range(first, last + 1):
            fat_set(self.fat, cluster, 0xFFF if cluster == last else cluster + 1)
            begin = (cluster - first) * CLUSTER_SIZE
            data = contents[begin:begin + CLUSTER_SIZE]
            offset = DATA_OFFSET + (cluster - 2) * CLUSTER_SIZE
            self.image[offset:offset + len(data)] = data
        self.next_cluster = last + 1
        return first

    def add_root(self, name: str, attributes: int, contents: bytes) -> int:
        slot = self.root_slot(name)
        first = self.allocate(contents)
        self.root[slot:slot + 32] = make_entry(
            self.module.short_name(name), attributes, first, len(contents))
        return first

    def root_slot(self, name: str) -> int:
        raw_name = self.module.short_name(name)
        for offset in range(0, len(self.root), 32):
            first = self.root[offset]
            if first not in (0x00, 0xE5) and self.root[offset:offset + 11] == raw_name:
                raise BuildError(f"duplicate root entry: {name}")
            if first in (0x00, 0xE5):
                return offset
        raise BuildError("root directory is full")

    def add_bin(self, files: list[tuple[tuple[str, ...], bytes]]) -> None:
        directory_cluster = self.allocate(bytes(CLUSTER_SIZE))
        directory = bytearray(b"\xE5" * CLUSTER_SIZE)
        directory[:32] = make_entry(self.module.special_directory_name("."), 0x10,
                                    directory_cluster, 0)
        directory[32:64] = make_entry(self.module.special_directory_name(".."), 0x10,
                                      0, 0)
        slot = 64
        names = set()
        for path, contents in files:
            name = path[-1].upper()
            if name in names:
                raise BuildError(f"duplicate BIN basename: {name}")
            names.add(name)
            if slot + 32 > len(directory):
                raise BuildError("BIN directory is full")
            first = self.allocate(contents)
            directory[slot:slot + 32] = make_entry(
                self.module.short_name(name), 0x20, first, len(contents))
            slot += 32
        offset = DATA_OFFSET + (directory_cluster - 2) * CLUSTER_SIZE
        self.image[offset:offset + CLUSTER_SIZE] = directory
        bin_offset = self.root_slot("BIN")
        self.root[bin_offset:bin_offset + 32] = make_entry(
            self.module.short_name("BIN"), 0x10, directory_cluster, 0)

    def finish(self) -> bytes:
        self.image[FAT1_OFFSET:FAT1_OFFSET + FAT_SIZE] = self.fat
        self.image[FAT2_OFFSET:FAT2_OFFSET + FAT_SIZE] = self.fat
        self.image[ROOT_OFFSET:ROOT_OFFSET + len(self.root)] = self.root
        if self.image[FAT1_OFFSET:FAT1_OFFSET + FAT_SIZE] != \
                self.image[FAT2_OFFSET:FAT2_OFFSET + FAT_SIZE]:
            raise BuildError("FAT copies differ")
        return bytes(self.image)


def build(source: Path, output: Path, variant: str) -> dict[str, object]:
    module = load_pcengine_module()
    try:
        # Some preserved 1.05 disks have the same system files at different
        # cluster numbers after later files were added.  Identify the release
        # by the system-file sizes, not by mutable allocation order.
        source_disk = module.PcEngineDisk(
            source.read_bytes(), require_system_files=False)
    except (OSError, module.DiskError) as error:
        raise BuildError(f"cannot read PC-Engine source D88: {error}") from error
    expected_version = "1.1" if variant == "va2" else "1.05"
    observed_sizes = {}
    for name in SYSTEM_FILES:
        offset, exists = module.find_entry(source_disk.root, module.short_name(name))
        if not exists:
            raise BuildError(f"source D88 is missing {name}")
        observed_sizes[name] = struct.unpack_from(
            "<I", source_disk.root, offset + 28)[0]
    version_by_sizes = {
        "1.05": {"ENGINEIO.SYS": 4096, "PCENGINE.SYS": 52090,
                  "ADVGBIOS.SYS": 30956, "PCENGINE.COM": 5},
        "1.1": {"ENGINEIO.SYS": 4096, "PCENGINE.SYS": 62347,
                "ADVGBIOS.SYS": 16364, "PCENGINE.COM": 5},
    }
    source_version = next(
        (version for version, sizes in version_by_sizes.items()
         if observed_sizes == sizes), None)
    if source_version != expected_version:
        raise BuildError(
            f"{variant} requires PC-Engine {expected_version}; "
            f"source system-file sizes identify {source_version or 'unknown'}")
    executables = collect_executables(source_disk, module)
    by_name = {}
    for path, contents in executables:
        name = path[-1].upper()
        if name in by_name:
            raise BuildError(
                f"duplicate executable basename {name}: "
                f"{'/'.join(by_name[name][0])} and {'/'.join(path)}")
        by_name[name] = (path, contents)

    # HDFORM installs the PC-Engine IPL from the system disk.  Reuse the
    # validated source D88 boot sector rather than the generic empty-disk IPL
    # used by the emulator's New SASI image menu.
    builder = HdiBuilder(source_disk.boot_sector, module)
    for name in SYSTEM_FILES:
        builder.add_root(name, SYSTEM_ATTRIBUTES[name],
                         d88_file(source_disk, [name], module))
    builder.add_bin(sorted(executables, key=lambda item: "/".join(item[0]).upper()))
    image = builder.finish()
    if output.exists():
        raise BuildError(f"output already exists: {output}")
    output.parent.mkdir(parents=True, exist_ok=True)
    temporary = None
    try:
        with tempfile.NamedTemporaryFile(
                dir=output.parent, prefix=output.name + ".tmp.", delete=False) as stream:
            temporary = Path(stream.name)
            stream.write(image)
        os.chmod(temporary, 0o644)
        os.replace(temporary, output)
        temporary = None
    finally:
        if temporary is not None:
            temporary.unlink(missing_ok=True)
    return {
        "variant": variant,
        "source": str(source),
        "output": str(output),
        "system_version": source_version,
        "geometry": {
            "header_bytes": HEADER_SIZE,
            "sector_bytes": SECTOR_SIZE,
            "sectors": SECTORS,
            "surfaces": SURFACES,
            "cylinders": CYLINDERS,
            "cluster_bytes": CLUSTER_SIZE,
            "clusters": CLUSTER_COUNT,
            "image_bytes": len(image),
        },
        "executables_in_bin": [
            {"source": "/".join(path), "name": path[-1].upper(),
             "bytes": len(contents),
             "sha256": hashlib.sha256(contents).hexdigest()}
            for path, contents in executables
        ],
        "sha256": hashlib.sha256(image).hexdigest(),
    }


def default_source(variant: str) -> Path:
    if variant == "va2":
        return REPOSITORY_ROOT / "docs" / "disks" / "PC-Engine 1.1.d88"
    return REPOSITORY_ROOT / "docs" / "disks" / "PC-Engine 1.05.d88"


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(
        description="Build a boot-layout 40 MB PC-88VA SASI development HDI")
    parser.add_argument("--variant", required=True, choices=("va", "va2"),
                        help="VA uses PC-Engine 1.05; VA2 uses PC-Engine 1.1")
    parser.add_argument("--source", type=Path,
                        help="source PC-Engine D88 (default is the repository docs path)")
    parser.add_argument("--output", required=True, type=Path,
                        help="new 40 MB SASI HDI path (must not already exist)")
    args = parser.parse_args(argv)
    source = (args.source or default_source(args.variant)).resolve()
    output = args.output.resolve()
    if not source.is_file():
        parser.error(f"source D88 not found: {source}")
    try:
        result = build(source, output, args.variant)
    except (BuildError, OSError) as error:
        parser.exit(1, f"error: {error}\n")
    print("Created boot-layout 40 MB SASI development disk")
    print(f"variant: {result['variant']}")
    print(f"output: {result['output']}")
    print(f"size: {result['geometry']['image_bytes']} bytes")
    print(f"SHA-256: {result['sha256']}")
    print(f"BIN executables: {len(result['executables_in_bin'])}")
    for item in result["executables_in_bin"]:
        print(f"  {item['name']}: {item['bytes']} bytes ({item['source']})")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
