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
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
# INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
# STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
# IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.

import hashlib
import importlib.util
from pathlib import Path
import struct
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
TOOLS = ROOT / "tools/pc88va"
PATHS = ("BIN/V480PAT.COM", "SRC/VTIMING/V480PAT.ASM", "DOC/VTIMING.TXT")


def load(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


DISK = load("vtiming_disk", TOOLS / "pcengine_disk.py")
SASI = load("vtiming_sasi", TOOLS / "build-sasi-utility-disk.py")
FIXTURE = load("vtiming_fixture", ROOT / "tools/cpmva/tests/test_install_cpmva.py")


def hdi_file(image, path):
    fat = image[SASI.FAT1_OFFSET:SASI.FAT1_OFFSET + SASI.FAT_SIZE]
    directory = image[SASI.ROOT_OFFSET:SASI.ROOT_OFFSET + SASI.ROOT_ENTRIES * 32]

    def chain(cluster):
        data = bytearray()
        seen = set()
        while cluster < 0xFF8:
            if cluster < 2 or cluster in seen:
                raise AssertionError("invalid generated FAT chain")
            seen.add(cluster)
            offset = SASI.DATA_OFFSET + (cluster - 2) * SASI.CLUSTER_SIZE
            data.extend(image[offset:offset + SASI.CLUSTER_SIZE])
            cluster = SASI.fat_get(fat, cluster)
        return data

    parts = path.split("/")
    for index, part in enumerate(parts):
        offset, exists = DISK.find_entry(directory, DISK.short_name(part))
        if not exists:
            raise AssertionError(f"missing generated HDD path: {path}")
        cluster = struct.unpack_from("<H", directory, offset + 26)[0]
        size = struct.unpack_from("<I", directory, offset + 28)[0]
        data = chain(cluster)
        if index == len(parts) - 1:
            return bytes(data[:size])
        directory = data


class VtimingMediaTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.work = Path(self.temp.name)
        self.stage = self.work / "stage"
        subprocess.run([str(TOOLS / "stage-vtiming.sh"), "--output", str(self.stage)],
                       check=True, capture_output=True)
        self.manifest = self.work / "stage.manifest.tsv"
        rows = []
        for path in PATHS:
            contents = (self.stage / path).read_bytes()
            rows.append(f"sasi\t{path}\t{hashlib.sha256(contents).hexdigest()}\t{len(contents)}")
        self.manifest.write_text("\n".join(rows) + "\n", encoding="utf-8")

    def test_source_build_and_exact_layout(self):
        files = {p.relative_to(self.stage).as_posix()
                 for p in self.stage.rglob("*") if p.is_file()}
        self.assertEqual(files, set(PATHS))
        self.assertEqual((self.stage / PATHS[1]).read_bytes(),
                         (TOOLS / "vtiming/v480pat.asm").read_bytes())
        self.assertEqual((self.stage / PATHS[2]).read_bytes(),
                         (TOOLS / "vtiming/vtiming.txt").read_bytes())
        rebuilt = self.work / "rebuilt.com"
        subprocess.run(["nasm", "-f", "bin", "-o", str(rebuilt),
                        str(self.stage / PATHS[1])], check=True)
        self.assertGreater(rebuilt.stat().st_size, 0)
        self.assertEqual(rebuilt.read_bytes(), (self.stage / PATHS[0]).read_bytes())

    def test_fdd_and_hdd_round_trip_with_and_without_payload(self):
        # Fully synthetic system files: no private ROM or disk is required.
        source = self.work / "source.d88"
        source.write_bytes(FIXTURE.make_pcengine_fixture())
        floppy = self.work / "utility.d88"
        floppy.write_bytes(source.read_bytes())
        DISK.install_payload(floppy, self.stage)
        disk = DISK.PcEngineDisk(floppy.read_bytes())
        for path in PATHS:
            self.assertEqual(SASI.d88_file(disk, path.split("/"), DISK),
                             (self.stage / path).read_bytes())
        for payload in (None, floppy):
            with self.subTest(transplant=payload is not None):
                output = self.work / ("transplant.hdi" if payload else "system.hdi")
                SASI.build(source, output, "va2", payload_d88=payload,
                           supplemental_tree=self.stage,
                           supplemental_manifest=self.manifest)
                image = output.read_bytes()
                for path in PATHS:
                    self.assertEqual(hdi_file(image, path),
                                     (self.stage / path).read_bytes())

    def test_builder_wiring_keeps_v480pat_out_of_diet(self):
        common = (TOOLS / "stage-development-tools.sh").read_text()
        self.assertIn('"$script_dir/stage-vtiming.sh"', common)
        self.assertLess(common.index('"$script_dir/stage-vtiming.sh"'),
                        common.rindex("write_manifest"))
        fdd = (TOOLS / "build-utility-disk.sh").read_text()
        self.assertGreater(fdd.index('bin/V480PAT.COM'),
                           fdd.index("DIET processed"))
        for path in PATHS:
            self.assertIn(f'"$common_stage_dir/{path}"', fdd)
        sasi = (TOOLS / "build-sasi-utility-disks.sh").read_text()
        self.assertIn('"$script_dir/stage-development-tools.sh"', sasi)
        self.assertEqual(sasi.count('--supplemental-tree "$supplemental_tree"'), 2)


if __name__ == "__main__":
    unittest.main()
