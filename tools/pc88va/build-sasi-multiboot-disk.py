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
# OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
# OF THE POSSIBILITY OF SUCH DAMAGE.

"""Build a PC-88VA SASI multiboot HDI (requires MS-DOS 4.0 VA with D5.1 or later)."""

import argparse
import os
import re
import struct
import subprocess
import tempfile
from datetime import date, datetime
from pathlib import Path
from typing import NamedTuple

from pcengine_disk import DiskError, PcEngineDisk, find_entry, short_name

HEADER = 4096
SECTOR = 1024  # NEC filesystem sector (four physical blocks)
# Cylinders, heads, filesystem sectors, root entries, sectors/cluster, sectors/FAT.
GEOMETRY = {5: (153, 4, 4950, 608, 8, 1), 10: (310, 4, 9900, 992, 8, 2),
            20: (615, 4, 19800, 1504, 8, 4), 40: (615, 8, 39593, 3072, 16, 4)}
ENGINE = ('ENGINEIO.SYS', 'PCENGINE.SYS', 'ADVGBIOS.SYS', 'PCENGINE.COM')
ENGINE_SIZES = ((4096, 52090, 30956, 5), (4096, 62347, 16364, 5))
DOS = ('IO.SYS', 'MSDOS.SYS', 'COMMAND.COM', 'SYS.COM', 'CHKDSK.COM')
LABELS = ('MS-DOS 4.00', 'PC-Engine 1.05 (VA)', 'PC-Engine 1.1 (VA2/3)')
DOS_REQUIREMENT = 'MS-DOS 4.0 VA with D5.1 or later'
STATE_VERSION = 3
LABEL_OFFSET = 138
LABEL_SIZE = 24  # Includes the terminating NUL.
MODELS = {'va': '[PC-88VA]', 'va2': '[PC-88VA2/VA3/VA-91]'}


class BuildError(Exception):
    def __init__(self, code, detail):
        self.code = code
        super().__init__(f'{code}: {detail}')


def require(condition, code, detail):
    if not condition:
        raise BuildError(code, detail)


def crc16(data):
    crc = 0
    for byte in data:
        crc ^= byte
        for _ in range(8):
            crc = (crc >> 1) ^ (0xa001 if crc & 1 else 0)
    return crc


class FloppyFile(NamedTuple):
    payload: bytes
    stamp: tuple[int, int]  # FAT write time, write date


def fat_stamp(local):
    require(1980 <= local.year <= 2107, 'FAT_TIMESTAMP', 'year outside FAT range')
    return ((local.hour << 11) | (local.minute << 5) | (local.second // 2),
            ((local.year - 1980) << 9) | (local.month << 5) | local.day)


def check_stamp(stamp, name):
    clock, day = stamp
    try:
        require(0 <= day <= 0xffff, 'FAT_TIMESTAMP', f'{name}: invalid FAT date')
        date(1980 + (day >> 9), (day >> 5) & 15, day & 31)
    except ValueError as exc:
        raise BuildError('FAT_TIMESTAMP', f'{name}: invalid FAT date') from exc
    require(0 <= clock <= 0xffff and clock >> 11 < 24
            and (clock >> 5) & 63 < 60 and clock & 31 < 30,
            'FAT_TIMESTAMP', f'{name}: invalid FAT time')


class Volume:
    """Root-only NEC FAT12 volume. Never generate or modify D5 menu state."""

    def __init__(self, mb, image=None):
        cyl, heads, sectors, entries, spc, spf = GEOMETRY[mb]
        size = cyl * heads * 33 * 256
        header = struct.pack('<8I', 0, 0, HEADER, size, 256, 33, heads, cyl)
        self.fat = HEADER + SECTOR
        self.fat_size = spf * SECTOR
        self.root = self.fat + 2 * self.fat_size
        self.data = self.root + entries * 32
        self.cluster_size = spc * SECTOR
        self.end_cluster = 2 + (sectors - (self.data - HEADER) // SECTOR) // spc
        media = bytes((0xd0 + tuple(GEOMETRY).index(mb), 255, 255))
        if image is None:
            image = bytearray(HEADER + size)
            image[:len(header)] = header
            for offset in (self.fat, self.fat + self.fat_size):
                image[offset:offset + 3] = media
        self.image = image
        require(len(image) == HEADER + size and image[:len(header)] == header,
                'HDI_GEOMETRY', f'{mb} MB')
        fat1 = image[self.fat:self.root - self.fat_size]
        fat2 = image[self.root - self.fat_size:self.root]
        require(fat1 == fat2 and fat1[:3] == media, 'FAT_COPIES', 'headers or mirrors differ')
        self.entries = {}
        self.slots = []
        end = False
        for offset in range(self.root, self.data, 32):
            first = image[offset]
            end |= first == 0
            if end or first == 0xe5:
                self.slots.append(offset)
                continue
            name = bytes(image[offset:offset + 11])
            require(name not in self.entries and not image[offset + 11] & 0x18,
                    'FAT_ROOT', f'unexpected or duplicate root entry {name!r}')
            self.entries[name] = offset

    def fat_entry(self, cluster):
        return (struct.unpack_from('<H', self.image, self.fat + cluster * 3 // 2)[0]
                >> (4 * (cluster % 2))) & 0xfff

    def put(self, name, payload, attr=0x20, *, stamp):
        check_stamp(stamp, name)
        raw = short_name(name)
        require(raw not in self.entries and self.slots, 'FAT_ROOT', f'cannot add {name}')
        count = (len(payload) + self.cluster_size - 1) // self.cluster_size
        chain = [c for c in range(2, self.end_cluster) if not self.fat_entry(c)][:count]
        require(len(chain) == count, 'FAT_SPACE', name)
        for index, cluster in enumerate(chain):
            following = chain[index + 1] if index + 1 < count else 0xfff
            shift = 4 * (cluster % 2)
            for fat in (self.fat, self.fat + self.fat_size):
                pos = fat + cluster * 3 // 2
                value, = struct.unpack_from('<H', self.image, pos)
                struct.pack_into('<H', self.image, pos,
                                 (value & ~(0xfff << shift)) | (following << shift))
            pos = self.data + (cluster - 2) * self.cluster_size
            chunk = payload[index * self.cluster_size:(index + 1) * self.cluster_size]
            self.image[pos:pos + len(chunk)] = chunk
        offset = self.slots.pop(0)
        if self.image[offset] == 0 and offset + 32 < self.data:
            # Slots after the end marker may contain stale entries.
            self.image[offset + 32:offset + 64] = bytes(32)
        self.image[offset:offset + 32] = (raw + bytes([attr]) + bytes(10)
                                         + struct.pack('<HHHI', *stamp, chain[0] if chain else 0,
                                                       len(payload)))
        self.entries[raw] = offset

    def read(self, name, used=None):
        raw = short_name(name) if isinstance(name, str) else name
        require(raw in self.entries, 'ROOT_FILE_MISSING', repr(name))
        offset = self.entries[raw]
        cluster, size = struct.unpack_from('<HI', self.image, offset + 26)
        used = set() if used is None else used
        result = bytearray()
        while len(result) < size:
            require(2 <= cluster < self.end_cluster and cluster not in used,
                    'FAT_CHAIN', repr(name))
            used.add(cluster)
            pos = self.data + (cluster - 2) * self.cluster_size
            result.extend(self.image[pos:pos + self.cluster_size])
            cluster = self.fat_entry(cluster)
        require(cluster >= 0xff8 if size else cluster == 0, 'FAT_CHAIN', repr(name))
        return bytes(result[:size])

    def check_fat(self):
        used = set()
        for name in self.entries:
            self.read(name, used)
        require(all(bool(self.fat_entry(c)) == (c in used)
                    for c in range(2, self.end_cluster)), 'FAT_LOST_CHAIN', 'unowned allocation')

    @property
    def ipl(self):
        return bytes(self.image[HEADER:HEADER + SECTOR])


def validate(volume, captures, owner, default, *, host_stamps=None):
    state = volume.read('BOOTMENU.DAT')
    require(len(state) == 4096 and state[:4] == b'VBM1'
            and crc16(state[:254]) == int.from_bytes(state[254:256], 'little'),
            'D5_STATE_CRC', 'invalid BOOTMENU.DAT size, signature or checksum')
    require(state[118] == STATE_VERSION, 'D5_LABEL_FORMAT',
            f'unsupported menu state version {state[118]}; requires {DOS_REQUIREMENT}')
    require(state[4:8] == bytes((owner, default, 0, 0)),
            'D5_STATE', 'unexpected owner, default or phase')
    require(volume.ipl[255] == 1 and volume.read('BOOTMENU.SYS'),
            'D5_STAGE', 'menu is not enabled or has no stage')
    for entry_id, (ipl, files) in enumerate(captures, 1):
        saved_ipl = state[entry_id * 1024:(entry_id + 1) * 1024]
        base = 16 + (entry_id - 1) * 34
        label_start = LABEL_OFFSET + (entry_id - 1) * LABEL_SIZE
        label = state[label_start:label_start + LABEL_SIZE].split(b'\0', 1)[0]
        require(label == LABELS[entry_id].encode(), 'D5_LABEL_REJECTED',
                f'SYS did not save {LABELS[entry_id]!r}; requires {DOS_REQUIREMENT}')
        checksum, = struct.unpack_from('<H', state, base + 16)
        require(saved_ipl == ipl and checksum == crc16(ipl), 'D5_ENTRY', LABELS[entry_id])
        for index, (name, payload) in enumerate(zip(ENGINE, files)):
            if owner != entry_id:
                name = f'VA{entry_id}{index}0000.SYS'
            offset = volume.entries.get(short_name(name))
            require(offset is not None, 'D5_FILE_MISSING', name)
            slot, checksum = struct.unpack_from('<HH', state, base + 18 + index * 4)
            require(offset == volume.root + slot * 32
                    and checksum == crc16(volume.image[offset + 12:offset + 32])
                    and volume.read(name) == payload, 'D5_FILE_CHANGED', name)
    volume.check_fat()
    for name, expected in (host_stamps or {}).items():
        offset = volume.entries.get(short_name(name))
        require(offset is not None, 'ROOT_FILE_MISSING', name)
        actual = struct.unpack_from('<HH', volume.image, offset + 22)
        check_stamp(actual, name)
        require(actual == expected, 'FAT_TIMESTAMP_CHANGED', name)


def screen_text(dump):
    require(dump[:12] == b'VAEGSCN1\x01\0\0\0', 'TVRAM', 'unsupported dump header')
    try:
        pos = 16 + struct.unpack_from('<I', dump, 12)[0]
        table, _, line_height, _, _, _, size = struct.unpack_from('<7I', dump, pos)
        text = dump[pos + 28:pos + 28 + size]
        start, height, width = struct.unpack_from('<IHH', text, table + 16)
        require(size == len(text) and width & 0x3ff == 640 and line_height > 0,
                'TVRAM', 'expected an 80-column text screen')
        return '\n'.join(''.join(chr(c) if 32 <= c < 127 else ' ' for c in
                                 struct.unpack_from('<80H', text, start + 160 * row)).rstrip()
                         for row in range((height & 0x1fe) // line_height))
    except struct.error as exc:
        raise BuildError('TVRAM', 'truncated text dump') from exc


class Guest:
    def __init__(self, args, work):
        self.args = args
        self.work = work
        self.disk = work / 'disk.hdi'
        self.disk.write_bytes(Volume(args.size).image)
        (work / 'vaeg.ini').write_text('[NekoProjectII]\nMain_RAM=640\nMain_RAM_Auto=false\n'
                                       'Boot_FixedDisk=true\ncalendar=false\n', encoding='ascii')

    def run(self, stage, floppy='none', steps=(), frames=2000, *, checkpoint=None):
        script = ['debug-script 1', f'limit-frame {frames + 100}']
        if checkpoint is not None:
            script.extend((f'wait-frame {checkpoint}', f'capture-tvram {stage}-checkpoint'))
        for frame, line in steps:
            script.extend((f'wait-frame {frame}', 'enter' if line is None else f'input-line {line}'))
        script.extend((f'wait-frame {frames}', 'exit', ''))
        (self.work / f'{stage}.debug').write_text('\n'.join(script), encoding='ascii')
        command = [str(self.args.vaeg), '--model', self.args.model, '--roms', str(self.args.roms),
                   '--cfg', 'vaeg.ini', '--bkupmem', 'memory.dat', '--debug', '--nowait',
                   '--frameskip', 'full', '--debug-output-dir', '.', '--fdd1', floppy,
                   '--fdd2', 'none', '--sasi1', 'disk.hdi', '--debug-script', f'{stage}.debug',
                   '--screen-tvram-dump', f'{stage}.tvram']
        with (self.work / f'{stage}.log').open('wb') as log:
            subprocess.run(command, cwd=self.work, stdout=log, stderr=subprocess.STDOUT,
                           check=True, timeout=180, env=dict(os.environ, TMPDIR=str(self.work),
                                                            SDL_VIDEODRIVER='dummy', SDL_AUDIODRIVER='dummy'))
        text = screen_text((self.work / f'{stage}.tvram').read_bytes())
        (self.work / f'{stage}.txt').write_text(text + '\n', encoding='ascii')
        return text

    def volume(self):
        return Volume(self.args.size, bytearray(self.disk.read_bytes()))


def copy_floppy(work, source, name, required, system):
    data = bytearray(source.read_bytes())
    require(len(data) >= 0x2b0, 'D88_INPUT', name)
    data[0x1a] = 0  # Normalize the scratch copy only.
    disk = PcEngineDisk(data, require_system_files=False)
    require(disk.is_msdos() == (system == LABELS[0]),
            'D88_SYSTEM', f'{name}: expected {system}')
    files = []
    for filename in required:
        where, found = find_entry(disk.root, short_name(filename))
        require(found and not disk.root[where + 11] & 0x18, 'D88_FILE', filename)
        cluster, length = struct.unpack_from('<HI', disk.root, where + 26)
        content = b''.join(disk.read_cluster(c) for c in disk.cluster_chain(cluster))
        require(0 < length <= len(content), 'D88_FILE', filename)
        stamp = struct.unpack_from('<HH', disk.root, where + 22)
        files.append(FloppyFile(content[:length], stamp))
    (work / name).write_bytes(data)
    return tuple(files)


def expect(text, code, *markers):
    require(all(marker in text for marker in markers), code, f'expected {markers!r}\n{text}')


def expect_capture(text, label):
    require('up to 15 ASCII characters' not in text and 'Label must be printable ASCII' not in text,
            'D5_LABEL_REJECTED', f'SYS did not accept {label!r}; requires {DOS_REQUIREMENT}')
    expect(text, 'GUEST_CAPTURE', 'System transferred')


def expect_menu(text, model, default_key, code):
    expect(text, code, MODELS[model], f'Enter = {default_key}')
    lines = set(line.strip() for line in text.splitlines())
    for key in (1, 2, 3):
        marker = f'{key}: {LABELS[key % 3]}'
        require(marker in lines, code, f'expected menu line {marker!r}\n{text}')


def check_timeout(guest, owner):
    before = guest.disk.read_bytes()
    checkpoint = guest.work / 'timeout-checkpoint.tvram.bin'
    checkpoint.unlink(missing_ok=True)
    text = guest.run('timeout', frames=1900, checkpoint=900)
    require(checkpoint.is_file(), 'GUEST_TIMEOUT_MENU', 'missing menu checkpoint')
    expect_menu(screen_text(checkpoint.read_bytes()), guest.args.model, owner or 3,
                'GUEST_TIMEOUT_MENU')
    expect(text, 'GUEST_TIMEOUT', 'Ready' if owner else 'Enter new date')
    require(guest.disk.read_bytes() == before, 'GUEST_TIMEOUT', 'unprompted boot changed the HDI')


def startup_names(entry_id, owner):
    if entry_id == owner:
        return 'CONFIG.SYS', 'AUTOEXEC.BAT'
    return f'VACFG{entry_id}.SYS', f'VAEXEC{entry_id}.BAT'


def install_startup(volume, args, owner, build_stamp):
    stamps = {}
    for entry_id, system in enumerate(('msdos', 'pce105', 'pce11')):
        for kind, name in zip(('config', 'autoexec'), startup_names(entry_id, owner)):
            path = getattr(args, f'{system}_{kind}')
            if path is not None:
                with path.open('rb') as source:
                    payload = source.read()
                    stamp = fat_stamp(datetime.fromtimestamp(os.fstat(source.fileno()).st_mtime))
            elif entry_id == 0 and kind == 'autoexec':
                payload = b'@ECHO OFF\r\n'  # Suppress DOS DATE/TIME setup prompts.
                stamp = build_stamp
            else:
                continue
            volume.put(name, payload, 0x20 if entry_id == owner else 0x22, stamp=stamp)
            stamps[name] = stamp
    return stamps


def publish(image, output, work):
    # Finish writing before a no-clobber publication. Keep the published HDI
    # independent of the guest's working disk and never mount the final path.
    with tempfile.TemporaryDirectory(dir=work) as directory:
        staged = Path(directory) / 'final.hdi'
        staged.write_bytes(image)
        os.link(staged, output)


def build(args, work):
    build_stamp = fat_stamp(datetime.now())
    guest = Guest(args, work)
    engines = [copy_floppy(work, args.pce105, 'pce105.d88', ENGINE, LABELS[1]),
               copy_floppy(work, args.pce11, 'pce11.d88', ENGINE, LABELS[2])]
    for label, files, sizes in zip(LABELS[1:], engines, ENGINE_SIZES):
        require(tuple(len(f.payload) for f in files) == sizes,
                'D88_SYSTEM', f'expected {label} system-file sizes')
    dos = copy_floppy(work, args.msdos, 'msdos.d88', DOS, LABELS[0])
    captures = []
    for entry, floppy in enumerate(('pce105.d88', 'pce11.d88'), 1):
        if entry == 2:
            text = guest.run('prepare', 'msdos.d88', ((1100, 'SYS C: /P'),))
            expect(text, 'GUEST_PREPARE', 'Prepared for another PC-Engine SYS')
        before = guest.volume().ipl
        # Probe the target from PC-Engine before SYS (also used by the D5 harness).
        guest.run(f'install-{entry}', floppy, ((1400, 'DIR C:'), (2100, 'SYS C:')), 3600)
        volume = guest.volume()
        files = tuple(volume.read(name) for name in ENGINE)
        require(volume.ipl != before and files == tuple(f.payload for f in engines[entry - 1]),
                'GUEST_ENGINE_INSTALL', LABELS[entry])
        captures.append((volume.ipl, files))
        text = guest.run(f'capture-{entry}', 'msdos.d88',
                         ((1100, 'SYS C:'), (1800, 'Y'), (2200, LABELS[entry])), 3500)
        expect_capture(text, LABELS[entry])
        validate(guest.volume(), captures, 0, entry)

    volume = guest.volume()
    # Copy just the maintenance utilities; SYS installs the DOS kernel and shell.
    require(tuple(volume.read(name) for name in DOS[:3]) == tuple(f.payload for f in dos[:3]),
            'GUEST_DOS_INSTALL', 'kernel or command interpreter differs from distribution')
    host_stamps = {}
    for name, (payload, stamp) in zip(DOS[3:], dos[3:]):
        volume.put(name, payload, stamp=stamp)
        host_stamps[name] = stamp
    guest.disk.write_bytes(volume.image)
    text = guest.run('menu', frames=900)
    expect_menu(text, args.model, 2, 'GUEST_MENU')
    # A fresh DOS installation asks for date and time without AUTOEXEC.BAT.
    dos_boot = ((920, '3'), (1460, None), (1630, None))
    for key in (1, 2, 3):
        steps = dos_boot if key == 3 else ((920, str(key)),)
        text = guest.run(f'boot-{key}', steps=steps, frames=1900)
        expect(text, 'GUEST_BOOT', 'C>' if key == 3 else 'Ready')
        validate(guest.volume(), captures, key % 3, key % 3)
    text = guest.run('list', steps=(*dos_boot, (1900, 'SYS C: /L')), frames=2300)
    expect(text, 'GUEST_LIST', *LABELS[1:], 'VACFG1.SYS', 'VACFG2.SYS')
    text = guest.run('chkdsk', steps=(*dos_boot, (1900, 'CHKDSK C:')), frames=2600)
    expect(text, 'GUEST_CHKDSK', 'C>CHKDSK C:', 'available allocation units')
    require(not re.search(r'lost|invalid|cross.link|error|correct', text, re.I),
            'GUEST_CHKDSK', text)

    owner = {'msdos': 0, 'pce105': 1, 'pce11': 2}[args.default]
    if owner:
        text = guest.run('select-default', steps=((920, str(owner)),), frames=1750)
        expect(text, 'GUEST_BOOT', 'Ready')
    check_timeout(guest, owner)
    volume = guest.volume()
    validate(volume, captures, owner, owner)
    host_stamps.update(install_startup(volume, args, owner, build_stamp))
    image = bytes(volume.image)
    validate(Volume(args.size, image), captures, owner, owner, host_stamps=host_stamps)
    publish(image, args.output, work)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for option in ('pce105', 'pce11', 'msdos', 'vaeg', 'roms', 'output', 'work-dir'):
        parser.add_argument(f'--{option}', type=Path, required=True)
    parser.add_argument('--model', choices=MODELS, default='va2')
    parser.add_argument('--size', type=int, choices=GEOMETRY, default=40, help='SASI size in MB')
    parser.add_argument('--default', choices=('msdos', 'pce105', 'pce11'), default='msdos')
    for system in ('msdos', 'pce105', 'pce11'):
        for kind in ('config', 'autoexec'):
            parser.add_argument(f'--{system}-{kind}', type=Path)
    args = parser.parse_args()
    try:
        require(args.output.suffix.lower() == '.hdi' and not os.path.lexists(args.output),
                'OUTPUT', 'expected a new .hdi path')
        require(args.output.parent.stat().st_dev == args.work_dir.stat().st_dev,
                'OUTPUT', 'output and scratch must be on the same filesystem')
        args.vaeg = args.vaeg.resolve(strict=True)
        args.roms = args.roms.resolve(strict=True)
        work = Path(tempfile.mkdtemp(prefix='sasi-multiboot-', dir=args.work_dir.resolve(strict=True)))
        print(f'Build logs and scratch media: {work}', flush=True)
        build(args, work)
    except (BuildError, DiskError, OSError, subprocess.SubprocessError) as exc:
        parser.exit(1, f'Build failed: {exc}\n')
    print(f'Wrote {args.output}')


if __name__ == '__main__':
    main()
