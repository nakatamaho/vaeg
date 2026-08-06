#!/usr/bin/env python3
#
# Copyright (c) 2026 Nakata Maho
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS
# OR IMPLIED WARRANTIES ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

from __future__ import annotations

import hashlib
import importlib.util
import json
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import unittest
import warnings
import zipfile


ROOT = Path(__file__).resolve().parents[3]
SCRIPT = ROOT / "tools" / "cpmva" / "install_cpmva.py"
LOCK_PATH = SCRIPT.with_name("sources.lock.json")


def load_installer():
    spec = importlib.util.spec_from_file_location("cpmva_installer_test", SCRIPT)
    module = importlib.util.module_from_spec(spec)
    assert spec.loader is not None
    spec.loader.exec_module(module)
    return module


def set_fat12(fat, cluster, value):
    offset = cluster + cluster // 2
    if cluster & 1:
        fat[offset] = (fat[offset] & 0x0F) | ((value & 0x0F) << 4)
        fat[offset + 1] = (value >> 4) & 0xFF
    else:
        fat[offset] = value & 0xFF
        fat[offset + 1] = (fat[offset + 1] & 0xF0) | ((value >> 8) & 0x0F)


def make_pcengine_fixture(with_autoexec=False):
    sector_size = 1024
    track_size = 8 * (16 + sector_size)
    header_size = 0x2B0
    image = bytearray(header_size + 160 * track_size)
    image[:17] = b"TEST-PCENGINE".ljust(17, b"\0")
    image[0x1B] = 0x20
    offsets = []
    for track in range(160):
        offset = header_size + track * track_size
        offsets.append(offset)
        struct.pack_into("<I", image, 0x20 + track * 4, offset)
        cylinder, head = divmod(track, 2)
        struct.pack_into("<H", image, offset + 4, 8)
        cursor = offset
        for record in range(1, 9):
            struct.pack_into(
                "<BBBBHBBBB3sBH",
                image,
                cursor,
                cylinder,
                head,
                record,
                3,
                8,
                0,
                0,
                0,
                0,
                b"\0\0\0",
                0,
                sector_size,
            )
            cursor += 16 + sector_size
    struct.pack_into("<I", image, 0x1C, len(image))

    def lba_offset(lba):
        track, record_index = divmod(lba, 8)
        return offsets[track] + record_index * (16 + sector_size) + 16

    fat = bytearray(2048)
    fat[:3] = b"\xfe\xff\xff"
    for begin, end in ((2, 5), (6, 66), (67, 82)):
        for cluster in range(begin, end):
            set_fat12(fat, cluster, cluster + 1)
        set_fat12(fat, end, 0xFFF)
    set_fat12(fat, 83, 0xFFF)
    if with_autoexec:
        set_fat12(fat, 84, 0xFFF)
    def write_lbas(start, data):
        for index in range(len(data) // sector_size):
            begin = index * sector_size
            image[lba_offset(start + index) : lba_offset(start + index) + sector_size] = data[begin : begin + sector_size]

    write_lbas(1, fat)
    write_lbas(3, fat)

    system_files = [
        ("ENGINEIO", "SYS", 2, 4096),
        ("PCENGINE", "SYS", 6, 62347),
        ("ADVGBIOS", "SYS", 67, 16364),
        ("PCENGINE", "COM", 83, 5),
    ]
    if with_autoexec:
        system_files.append(("AUTOEXEC", "BAT", 84, len(b"@ECHO OFF\r\nECHO READY\r\n")))
    root = bytearray(6144)
    for index, (base, extension, cluster, size) in enumerate(system_files):
        entry = bytearray(32)
        entry[:8] = base.ljust(8).encode("ascii")
        entry[8:11] = extension.ljust(3).encode("ascii")
        entry[11] = 0x20
        struct.pack_into("<H", entry, 26, cluster)
        struct.pack_into("<I", entry, 28, size)
        root[index * 32 : index * 32 + 32] = entry
    write_lbas(5, root)
    if with_autoexec:
        write_lbas(93, b"@ECHO OFF\r\nECHO READY\r\n" + bytes(sector_size - len(b"@ECHO OFF\r\nECHO READY\r\n")))
    return bytes(image)


class InstallerTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.installer = load_installer()
        cls.lock = cls.installer.load_lock(LOCK_PATH)

    def test_lock_schema_and_changed_digest(self):
        self.assertEqual(self.lock["schema_version"], 1)
        self.assertIn("vt100_games", self.lock["sources"])
        self.assertIn("bdsc", self.lock["sources"])
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "changed.json"
            changed = json.loads(LOCK_PATH.read_text())
            changed["sources"]["cpmva"]["sha256"] = "0" * 64
            path.write_text(json.dumps(changed))
            self.assertEqual(self.installer.load_lock(path)["schema_version"], 1)
            source = Path(directory) / "source.bin"
            source.write_bytes(b"actual")
            with self.assertRaises(self.installer.InstallerError) as raised:
                self.installer.verify_file(source, {"size": 6, "sha256": "0" * 64})
            self.assertEqual(raised.exception.code, "SOURCE_DIGEST")

    def test_archive_traversal_and_duplicate_rejection(self):
        limits = self.lock
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            traversal = root / "traversal.zip"
            with zipfile.ZipFile(traversal, "w") as archive:
                archive.writestr("../escape.bin", b"x")
            with self.assertRaises(self.installer.InstallerError) as raised:
                self.installer.safe_extract_archive(traversal, root / "work1", limits)
            self.assertEqual(raised.exception.code, "ARCHIVE_PATH")

            duplicate = root / "duplicate.zip"
            with warnings.catch_warnings():
                warnings.simplefilter("ignore", UserWarning)
                with zipfile.ZipFile(duplicate, "w") as archive:
                    archive.writestr("same.bin", b"one")
                    archive.writestr("same.bin", b"two")
            with self.assertRaises(self.installer.InstallerError) as raised:
                self.installer.safe_extract_archive(duplicate, root / "work2", limits)
            self.assertEqual(raised.exception.code, "ARCHIVE_DUPLICATE")

    def test_offline_cache_hit_and_miss(self):
        payload = b"locked source"
        digest = hashlib.sha256(payload).hexdigest()
        spec = {"sha256": digest, "size": len(payload), "archive_type": "zip", "resolved_url": "https://invalid"}
        lock = {"limits": self.lock["limits"]}
        with tempfile.TemporaryDirectory() as directory:
            cache = Path(directory)
            cached = cache / "sources" / f"demo-{digest}.zip"
            cached.parent.mkdir()
            cached.write_bytes(payload)
            found, origin, override = self.installer.fetch_locked_source(
                "demo", spec, None, cache, lock, True
            )
            self.assertEqual(found, cached)
            self.assertFalse(override)
            self.assertEqual(origin, spec["resolved_url"])
            missing = dict(spec)
            missing["sha256"] = "1" * 64
            with self.assertRaises(self.installer.InstallerError) as raised:
                self.installer.fetch_locked_source("missing", missing, None, cache, lock, True)
            self.assertEqual(raised.exception.code, "OFFLINE_MISS")

    def test_patch_applies_once_and_rejects_second_application(self):
        patch = (SCRIPT.parent / "patches" / "cpm22-64k.patch").read_bytes()
        lines = patch.decode("utf-8").splitlines()
        source_lines = []
        hunks = []
        index = 0
        while index < len(lines):
            if not lines[index].startswith("@@ "):
                index += 1
                continue
            header = lines[index]
            old_start = int(header.split()[1].split(",")[0][1:])
            index += 1
            body = []
            while index < len(lines) and not lines[index].startswith("@@ "):
                body.append(lines[index])
                index += 1
            old_lines = [line[1:] for line in body if line and line[0] in " -"]
            hunks.append((old_start, old_lines))
        maximum = max(start + len(old) for start, old in hunks)
        source_lines = [f"FILLER_{number}" for number in range(maximum + 1)]
        for start, old_lines in hunks:
            source_lines[start - 1 : start - 1 + len(old_lines)] = old_lines
        source = ("\r\n".join(source_lines) + "\r\n").encode("ascii")
        patched = self.installer.apply_unified_patch(source, patch)
        self.assertIn(b"MEM: EQU 64", patched)
        self.assertIn(b"ADD A,(HL)", patched)
        with self.assertRaises(self.installer.InstallerError) as raised:
            self.installer.apply_unified_patch(patched, patch)
        self.assertEqual(raised.exception.code, "PATCH_CONTEXT")

    def test_assembler_failure_is_propagated(self):
        source = b"MEM\tEQU\t62\nADD\tA,M\n"
        patch = b"--- original\n+++ patched\n@@ -1,2 +1,2 @@\n-MEM\tEQU\t62\n+MEM: EQU 64\n ADD\tA,M\n"
        with tempfile.TemporaryDirectory() as directory:
            with self.assertRaises(self.installer.InstallerError) as raised:
                self.installer.assemble_cpm22(
                    source, patch, "/no/such/z80asm", "1.8", Path(directory)
                )
            self.assertEqual(raised.exception.code, "ASSEMBLER_FAILED")

    def test_cpm_sys_sizes_signature_and_composition(self):
        ccp_bdos = bytes(range(256)) * 22
        bios = bytes(0x5FE) + b"VA"
        cpm_sys, equivalent = self.installer.compose_cpm_sys(
            ccp_bdos, bios, b"SYSOFS=10\n262-CPM*4\n"
        )
        self.assertTrue(equivalent)
        self.assertEqual(len(ccp_bdos), 0x1600)
        self.assertEqual(len(bios), 0x600)
        self.assertEqual(len(cpm_sys), 0x1C00)
        self.assertEqual(cpm_sys[:0x1600], ccp_bdos)
        self.assertEqual(cpm_sys[-0x600:], bios)
        self.assertEqual(cpm_sys[-2:], b"VA")

    def test_cpm_disk_geometry_reserved_blocks_and_multi_extent(self):
        files = {"EXIT.COM": b"E" * 128, "BIG.COM": b"B" * 32768}
        raw, metadata = self.installer.build_cpm_raw(files)
        self.assertEqual(len(raw), 327680)
        self.assertEqual(self.installer.CPM_DIRECTORY_OFFSET, 0x4000)
        self.assertEqual(raw[0x4000 + 0x1000 - 1], 0xE5)
        self.assertTrue(all(value >= 2 for value in metadata["allocated_blocks"]))
        self.assertEqual(self.installer.parse_cpm_raw(raw), files)
        image = self.installer.build_tools_disk(files)
        self.assertEqual(self.installer.unwrap_cpm_d88(image), raw)
        self.assertEqual(self.installer.parse_cpm_raw(self.installer.unwrap_cpm_d88(image)), files)
        track_two = struct.unpack_from("<I", image, 0x20 + 2 * 4)[0]
        self.assertEqual(image[track_two], 1)
        self.assertEqual(image[track_two + 1], 0)

    def test_cpm_exm1_groups_large_com_in_one_directory_entry(self):
        data = bytes(index & 0xFF for index in range(30592))
        raw, metadata = self.installer.build_cpm_raw({"BACKGMMN.COM": data})
        entry_offset = self.installer.CPM_DIRECTORY_OFFSET
        entry = raw[entry_offset : entry_offset + 32]
        self.assertEqual(metadata["directory_entries"], 1)
        self.assertEqual(entry[12], 1)
        self.assertEqual(entry[14], 0)
        self.assertEqual(entry[15], 111)
        self.assertEqual(list(entry[16:31]), list(range(2, 17)))
        self.assertEqual(raw[entry_offset + 32], 0xE5)
        self.assertEqual(self.installer.parse_cpm_raw(raw), {"BACKGMMN.COM": data})

    def test_cpm_exm1_multiple_logical_extent_groups_round_trip(self):
        data = bytes(index & 0xFF for index in range(49152))
        raw, metadata = self.installer.build_cpm_raw({"BIG.COM": data})
        directory = self.installer.CPM_DIRECTORY_OFFSET
        first = raw[directory : directory + 32]
        second = raw[directory + 32 : directory + 64]
        self.assertEqual(metadata["directory_entries"], 2)
        self.assertEqual((first[12], first[14], first[15]), (1, 0, 128))
        self.assertEqual((second[12], second[14], second[15]), (2, 0, 128))
        self.assertEqual(list(first[16:32]), list(range(2, 18)))
        self.assertEqual(list(second[16:24]), list(range(18, 26)))
        self.assertEqual(raw[directory + 64], 0xE5)
        self.assertEqual(self.installer.parse_cpm_raw(raw), {"BIG.COM": data})

    def test_cpm_exm1_partial_second_subextent_matches_cc2_shape(self):
        data = b"C" * 17280
        raw, metadata = self.installer.build_cpm_raw({"CC2.COM": data})
        entry = raw[self.installer.CPM_DIRECTORY_OFFSET : self.installer.CPM_DIRECTORY_OFFSET + 32]
        self.assertEqual(metadata["directory_entries"], 1)
        self.assertEqual((entry[12], entry[14], entry[15]), (1, 0, 7))
        self.assertEqual(list(entry[16:25]), list(range(2, 11)))
        self.assertEqual(self.installer.parse_cpm_raw(raw), {"CC2.COM": data})

    def test_cpm_legacy_one_entry_per_16k_layout_is_rejected(self):
        data = bytes(index & 0xFF for index in range(30592))
        raw, _ = self.installer.build_cpm_raw({"BIG.COM": data})
        directory = self.installer.CPM_DIRECTORY_OFFSET
        mutable = bytearray(raw)
        mutable[directory + 32 : directory + 64] = mutable[directory : directory + 32]
        for block_index, block in enumerate(range(17, 32)):
            mutable[directory + 32 + 16 + block_index] = block
        with self.assertRaises(self.installer.InstallerError) as raised:
            self.installer.parse_cpm_raw(bytes(mutable))
        self.assertEqual(raised.exception.code, "CPM_EXTENT_DUPLICATE")

    def test_cpm_extent_gap_is_rejected(self):
        data = bytes(index & 0xFF for index in range(49152))
        raw, _ = self.installer.build_cpm_raw({"BIG.COM": data})
        directory = self.installer.CPM_DIRECTORY_OFFSET
        mutable = bytearray(raw)
        mutable[directory + 32 + 12] = 4
        with self.assertRaises(self.installer.InstallerError) as raised:
            self.installer.parse_cpm_raw(bytes(mutable))
        self.assertEqual(raised.exception.code, "CPM_EXTENT_GAP")

    def test_game_and_development_mappings_and_padding(self):
        all_game_members = set(self.installer.GAME_BINARY_MAPPING.values())
        all_game_members.update(self.installer.GAME_SOURCE_MAPPING.values())
        all_game_members.update(self.installer.MESCC_BINARY_MAPPING.values())
        expected = set(self.lock["sources"]["vt100_games"]["expected_members"])
        self.assertEqual(all_game_members, expected)
        self.assertEqual(
            set(self.installer.BDSC_MAPPING.values()),
            set(self.lock["sources"]["bdsc"]["expected_members"]),
        )
        padded = self.installer.pad_cpm_records({"SOURCE.C": b"abc"})
        self.assertEqual(len(padded["SOURCE.C"]), 128)
        self.assertEqual(padded["SOURCE.C"][:3], b"abc")
        self.assertEqual(padded["SOURCE.C"][3:], b"\x1a" * 125)
        binary = self.installer.pad_cpm_records({"GAME.COM": b"abc"}, b"\x00")
        self.assertEqual(binary["GAME.COM"][3:], b"\x00" * 125)

    def test_d88_malformed_offset_is_rejected(self):
        image = bytearray(self.installer.wrap_cpm_d88(bytes(327680)))
        struct.pack_into("<I", image, 0x20, 0x2AF)
        with self.assertRaises(self.installer.InstallerError) as raised:
            self.installer.validate_d88_structure(bytes(image), expected_type=0x10)
        self.assertEqual(raised.exception.code, "D88_OFFSET")

    def test_native_boot_round_trip_and_input_unchanged(self):
        source = make_pcengine_fixture()
        before = hashlib.sha256(source).digest()
        with tempfile.TemporaryDirectory() as directory:
            result, metadata = self.installer.prepare_native_boot_image(
                source, {"TEST.BIN": b"T" * 1024}, False, Path(directory)
            )
            self.assertNotEqual(result, source)
            self.assertEqual(hashlib.sha256(source).digest(), before)
            self.installer.validate_d88_structure(result, expected_type=0x20)
            helpers = self.installer.import_pcengine_disk()
            disk = helpers[1](result)
            self.assertEqual(
                self.installer.fat_file_bytes(disk, "TEST.BIN", helpers), b"T" * 1024
            )
            self.assertEqual(metadata["inserted_files"], ["TEST.BIN"])

    def test_autostart_preserves_backup_and_line_endings(self):
        source = make_pcengine_fixture(with_autoexec=True)
        original = b"@ECHO OFF\r\nECHO READY\r\n"
        with tempfile.TemporaryDirectory() as directory:
            result, metadata = self.installer.prepare_native_boot_image(
                source, {"TEST.BIN": b"T" * 1024}, True, Path(directory)
            )
            helpers = self.installer.import_pcengine_disk()
            disk = helpers[1](result)
            self.assertIn("AUTOEXEC.BAK", metadata["inserted_files"])
            self.assertEqual(self.installer.fat_file_bytes(disk, "AUTOEXEC.BAK", helpers), original)
            autoexec = self.installer.fat_file_bytes(disk, "AUTOEXEC.BAT", helpers)
            self.assertIn(b"CALL CPMVA.BAT", autoexec)
            self.assertIn(b"\r\n", autoexec)

    def test_cli_help_license_refusal_and_dry_run(self):
        help_result = subprocess.run(
            [sys.executable, str(SCRIPT), "--help"], capture_output=True, text=True
        )
        self.assertEqual(help_result.returncode, 0)
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            boot = root / "boot.d88"
            boot.write_bytes(make_pcengine_fixture())
            output = root / "output"
            refused = subprocess.run(
                [sys.executable, str(SCRIPT), "--boot-disk", str(boot), "--output-dir", str(output)],
                capture_output=True, text=True,
            )
            self.assertEqual(refused.returncode, 2)
            self.assertIn("LICENSE_REQUIRED", refused.stderr)
            dry_run = subprocess.run(
                [
                    sys.executable,
                    str(SCRIPT),
                    "--boot-disk",
                    str(boot),
                    "--output-dir",
                    str(output),
                    "--dry-run",
                    "--accept-cpm-license",
                    "--accept-cpmva-license",
                    "--accept-games-license",
                    "--accept-bdsc-license",
                ],
                capture_output=True, text=True,
            )
            self.assertEqual(dry_run.returncode, 0, dry_run.stderr)
            self.assertFalse(output.exists())


if __name__ == "__main__":
    unittest.main()
