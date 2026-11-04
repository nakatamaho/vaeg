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
"""Build a separate, pinned MS-DOS 4.0 W1SCAN disk outside Git; no auto probe."""
import argparse
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[2]
HELP = '''W1SCAN - WSR-E address-setting probe (V3 VA only)
Build: {build}

No test runs automatically. Use VA video, not accelerator output.
Use only with WSR-E and no other board at the candidate I/O addresses.

W1SCAN        Read-only: settings 0 1 2 3 5 6 7 (4 is forbidden).
W1SCAN /P     Test SR06 12/0F twice at all seven settings; no wake-up.
W1SCAN /W 7   PROVISIONAL wake-up and SR06 test at setting 7 ONLY.
              Replace 7 with the intended setting if known.
              Wake is borrowed from GA-98NB, NOT verified for WSR-E.
              Writes common 46E8, selected misc/status and SR06 ports.
              May change clock/polarity/setup; no relay/VRAM writes.
              Prior FF misc/setup values are UNKNOWN, NOT written back.
              In that case wake leaves setup=08 and misc=E3 until reset
              or power-off; do not interpret cleanup as full restoration.

Log: WSRSCAN.TXT (appended). Photograph the screen; return the disk.
MATCH n = signature at candidate n, NOT complete board validation.
MULTIPLE = more than one candidate responds; NONE_INCONCLUSIVE is not
proof of absence or failure. READ_ONLY is not a board detection PASS.
Exit: 0 read-only completed or one match; 1 multiple; 2 none; 3 error.
SR06/index are restored when interpretable, otherwise locked/index kept.
No timing measurement, IRQ hook, port 152 changes, GR banks or BLT.
Do not run the WSR-E probe against Hyper Frame+.
'''


def module(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    obj = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(obj)
    return obj


def sha(data):
    return hashlib.sha256(data).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, required=True)
    parser.add_argument('--output-dir', type=Path, required=True)
    parser.add_argument('--build-id')
    args = parser.parse_args()
    policy = module('msdos_policy', ROOT/'tools/pc88va/build-all-demos-msdos4-disk.py')
    disktool = module('scan_disk', ROOT/'tools/pc88va/pcengine_disk.py')
    if sha(args.source.read_bytes()) != policy.MSDOS4_SHA256:
        raise ValueError('WSRE_SCAN_SYSTEM_IMAGE')
    output = args.output_dir.resolve()
    if output == ROOT or ROOT in output.parents:
        raise ValueError('WSRE_SCAN_OUT_OF_TREE')
    commit = subprocess.check_output(['git', '-C', str(ROOT), 'rev-parse', 'HEAD'], text=True).strip()
    build = args.build_id or commit[:8]
    if not re.fullmatch('[0-9a-f]{8}', build):
        raise ValueError('WSRE_SCAN_BUILD_ID')
    output.mkdir(parents=True, exist_ok=False)
    payload = output/'payload/root'
    payload.mkdir(parents=True)
    binary = output/'W1SCAN.COM'
    command = [os.environ.get('NASM', 'nasm'), '-f', 'bin',
               '-I'+str(ROOT/'demos/wsr-e-pseudo-sprite')+'/', f"-DBUILD_ID='{build}'",
               str(ROOT/'demos/wsr-e-pseudo-sprite/tests/w1scan.asm'), '-o', str(binary)]
    subprocess.run(command, check=True)
    (payload/'W1SCAN.COM').write_bytes(binary.read_bytes())
    (payload/'SCANHELP.TXT').write_bytes(HELP.format(build=build).replace('\n', '\r\n').encode('ascii'))
    (payload/'SCANHELP.BAT').write_bytes(b'@ECHO OFF\r\nTYPE SCANHELP.TXT\r\n')
    image = output/'wsr-e-address-scan-msdos4.d88'
    disktool.fat_now = lambda: (0, 0x5c21)  # fixed 2026-01-01 FAT timestamp
    disktool.create_vanilla(args.source, image)
    disktool.install_payload(image, output/'payload')
    disk = disktool.PcEngineDisk(image.read_bytes())
    if sha(disk.boot_sector) != policy.MSDOS4_BOOT_SECTOR_SHA256:
        raise ValueError('M106_MSDOS4_BOOT_SECTOR_MISMATCH')
    stored = policy.disk_files(disk, disktool)
    for name, digest in policy.MSDOS4_FILES_SHA256.items():
        if name not in stored or sha(stored.pop(name)) != digest:
            raise ValueError('M106_MSDOS4_SYSTEM_FILE_MISMATCH')
    expected = {p.name: p.read_bytes() for p in payload.iterdir()}
    if stored != expected:
        raise ValueError('WSRE_SCAN_PAYLOAD_MISMATCH')
    report = {'source_commit': commit, 'embedded_build': build,
              'nasm_command': command, 'exit': 0,
              'validation': 'pinned boot/system/license hashes and exact payload readback',
              'com_sha256': sha(binary.read_bytes()), 'd88_sha256': sha(image.read_bytes()),
              'hardware_validation': 'NOT_RUN'}
    (output/'build.json').write_text(json.dumps(report, indent=2)+'\n')
    print(json.dumps(report, indent=2))


if __name__ == '__main__':
    main()
