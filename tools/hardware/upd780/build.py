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
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
"""Build FDC-resident diagnostics and add controllers to a COPY of a CP/M disk."""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys

ROOT = Path(__file__).resolve().parents[3]
HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(ROOT / 'tools/cpmva'))
sys.path.insert(0, str(ROOT / 'tools/cpmva/zex'))
import install_cpmva as cpm
import zexbuild as zex

ROUNDS = (1, 4, 16)
BODIES = {
    'branch': ('', 0),
    'nop': ('nop', 4),
    'nop8': ('\n'.join(['nop'] * 8), 32),
    'register': ('ld a,b', 4),
    'read': ('ld a,(hl)', 7),
    'write': ('ld (hl),a', 7),
    'alu': ('add a,55h', 7),
    'stack': ('push hl\npop hl', 21),
    'indexed': ('bit 0,(ix+0)', 20),
    'scf': ('scf', 4),
    'ccf': ('ccf', 4),
}

class BuildError(Exception):
    def __init__(self, code: str):
        self.code = code
        super().__init__(code)

def require(ok: bool, code: str) -> None:
    if not ok:
        raise BuildError(code)

def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()

def replace_once(text: str, old: str, new: str) -> str:
    require(text.count(old) == 1, 'M139_SOURCE_CONTRACT')
    return text.replace(old, new, 1)

def probe_source() -> str:
    calls, routines, labels = [], [], []
    calls += ['ld hl,empty_label', 'call puts', 'ld a,1', 'call char', 'ld a,2', 'call char']
    labels += ["empty_label: db 'T empty r=0 n=0',0"]
    for name, (body, _) in BODIES.items():
        for repeat in (1, 2):
            for rounds in ROUNDS:
                ident = f'{name}_{repeat}_{rounds}'
                calls += [f'ld hl,label_{ident}', 'call puts', 'ld a,1', 'call char',
                          f'ld de,{rounds}', f'call kernel_{name}', 'ld a,2', 'call char']
                labels += [f"label_{ident}: db 'T {name} r={repeat} n={rounds}',0"]
        routines += [f'kernel_{name}:', 'ld hl,7000h', 'ld ix,7000h',
                     f'outer_{name}:', 'ld bc,60000', f'loop_{name}:', body,
                     'dec bc', 'ld a,b', 'or c', f'jr nz,loop_{name}',
                     'dec de', 'ld a,d', 'or e', f'jp nz,outer_{name}', 'ret']
    source = HERE.joinpath('probe.asm').read_text()
    source = replace_once(source, 'TIMING_KERNELS:', '\n'.join(calls))
    return source + '\n' + '\n'.join(routines + labels) + '\n'

def zex_source(smoke: bool = False) -> str:
    # Transform the pinned GPL source, never the vendored file. Preserve its
    # notices, all groups, masks and scan vectors. The one declared base
    # operand adjustment below means this is NOT stock-address ZEX.
    raw = zex.read_zex_source('zexall')
    # LD r,n includes LD (HL),n with an untracked fixed 0339h operand.
    # In an FDC this would be ROM. Relocate only that base address into
    # scratch RAM; preserve opcode, masks and all increment/shift vectors.
    raw = replace_once(raw, '0d13dh,00339h,0de89h', '0d13dh,07339h,0de89h')
    source = zex.translate_zmac(raw)
    source = replace_once(source, '\torg\t100h', '\torg\t4000h')
    source = replace_once(source, 'ld\thl,(6)', 'di\n\tld sp,7effh\n\tld\thl,7effh\n\txor a\n\tout (0f8h),a\n\tld (7f09h),a\n\tld a,5\n\trst 18h')
    source = replace_once(source, 'jp\t0\t\t; warm boot', 'jp\tfdc_done')
    source = replace_once(source, 'call\t5', 'call\tfdc_bdos')
    source = replace_once(source, 'ei\t\t\t; enable interrupts', 'di\t\t\t; FDC: no asynchronous device interrupts')
    # Print every actual CRC, even if it equals the stock-address expectation.
    source = replace_once(source, 'jp\tz,tlpok', 'nop\n\tnop\n\tnop')
    source = replace_once(source, '  ERROR **** crc expected:', '  STOCK=')
    source = replace_once(source, ' found:', ' CRC=')
    source = replace_once(source, 'Z80all instruction exerciser', 'M139 FDC relocated ZEXALL')
    if smoke:
        begin = source.index('tests:')
        end = source.index('\n\tdw\t0', begin) + len('\n\tdw\t0')
        source = source[:begin] + 'tests:\n\tdw negop\n\tdw 0' + source[end:]
        source = replace_once(source, 'M139 FDC relocated ZEXALL', 'M139 FDC NEG SMOKE ONLY')
    return source + '''
fdc_bdos:
    ld a,c
    cp 2
    jr z,fdc_char
    cp 9
    jr nz,fdc_bad_bdos
fdc_string:
    ld a,(de)
    cp '$'
    ret z
    rst 18h
    inc de
    jr fdc_string
fdc_char:
    ld a,e
    rst 18h
    ret
fdc_bad_bdos:
    halt
    jr fdc_bad_bdos
fdc_done:
    ld a,4
    rst 18h
    jp 2ah
'''

def assemble_images(assembler: str, output: Path) -> dict[str, bytes]:
    images = {}
    for name, source in [('FDCPRB', probe_source()), ('FDCZEX', zex_source()), ('FDCONE', zex_source(True))]:
        payload, symbols = zex.assemble_with_symbols(source, assembler, name.lower())
        require(0 < len(payload) < 0x3000, 'M139_PAYLOAD_SIZE')
        (output / (name.lower() + '.asm')).write_text(source)
        (output / (name.lower() + '.bin')).write_bytes(payload)
        # assemble_with_symbols has its own private working directory; embed
        # bytes explicitly rather than relying on an assembler include path.
        loader = HERE.joinpath('loader.asm').read_text()
        loader = loader.replace("incbin 'payload.bin'", '\n'.join(
            'db ' + ','.join(str(x) for x in payload[i:i+24]) for i in range(0, len(payload), 24)))
        loader = f"PAYLOAD_SIZE: equ {len(payload)}\nLOGNAME: equ '{name.ljust(8)}TXT'\n" + loader
        # z80asm does not accept an EQU for a string used by DB.
        loader = loader.replace(f"LOGNAME: equ '{name.ljust(8)}TXT'\n", '').replace('db LOGNAME', f"db '{name.ljust(8)}TXT'")
        image, sym = zex.assemble_with_symbols(loader, assembler, name.lower() + '-loader')
        require(sym['workspace_end'] < 0xD000, 'M139_TPA_STATIC_SPACE')
        offset = sym['payload']-0x100
        require(image[offset:offset+len(payload)] == payload, 'M139_EMBEDDED_PAYLOAD_MISMATCH')
        require(len(image) == sym['image_end']-0x100, 'M139_MAIN_STACK_IMAGE')
        (output / (name.lower() + '-loader.asm')).write_text(loader)
        (output / (name.lower() + '-symbols.json')).write_text(json.dumps(sym, sort_keys=True))
        images[name + '.COM'] = image
    return images

def unpack(image: bytes) -> tuple[bytes, list[tuple[int, int]]]:
    require(len(image) >= 688 and struct.unpack_from('<I', image, 28)[0] == len(image), 'M139_D88_SIZE')
    require(image[27] in (0, 0x10), 'M139_D88_MEDIA')
    tracks = struct.unpack_from('<164I', image, 32)
    require(all(tracks[:80]) and not any(tracks[80:]), 'M139_D88_TRACKS')
    raw, locations = bytearray(), []
    for index, start in enumerate(tracks[:80]):
        pos = start
        for sector in range(16):
            require(pos+272 <= len(image), 'M139_D88_SECTOR')
            c, h, r, n, count = struct.unpack_from('<BBBBH', image, pos)
            size = struct.unpack_from('<H', image, pos+14)[0]
            require((c,h,r,n,count,size) == (index//2,index%2,sector+1,1,16,256), 'M139_D88_GEOMETRY')
            locations.append((pos+16, 256))
            raw.extend(image[pos+16:pos+272])
            pos += 272
        require(pos == (tracks[index+1] if index < 79 else len(image)), 'M139_D88_TRACK_END')
    return bytes(raw), locations

def add_files(image: bytes, files: dict[str, bytes]) -> bytes:
    original, locations = unpack(image)
    raw = bytearray(original)
    used, names, slots = {0,1}, set(), []
    for i in range(128):
        e = raw[0x4000+32*i:0x4020+32*i]
        if e[0] == 0xE5:
            slots.append(i)
        elif e[0] <= 15:
            require(e[13] == 0 and e[15] <= 128, 'M139_CPM_EXTENT')
            records = (e[12] & 1)*128 + e[15]
            blocks = e[16:16+(records*128+2047)//2048]
            require(all(2 <= b < 152 and b not in used for b in blocks) and len(set(blocks)) == len(blocks), 'M139_CPM_ALLOCATION')
            used.update(blocks)
            names.add(bytes(x & 127 for x in e[1:12]))
        else:
            require(e[0] == 0xFF, 'M139_CPM_USER')
    allowed = set()
    for name, data in sorted(files.items()):
        base, ext = cpm.cpm_name(name)
        require(base+ext not in names, 'M139_CPM_NAME_EXISTS')
        require(0 < len(data) <= 524288, 'M139_CPM_FILE_SIZE')
        names.add(base+ext)
        all_blocks = []
        for group, begin in enumerate(range(0,len(data),32768)):
            require(bool(slots), 'M139_CPM_DIRECTORY_FULL')
            chunk = data[begin:begin+32768]
            records = (len(chunk)+127)//128
            count = (records*128+2047)//2048
            blocks = [b for b in range(2,152) if b not in used][:count]
            require(len(blocks) == count, 'M139_CPM_DISK_FULL')
            used.update(blocks)
            all_blocks.extend(blocks)
            extent = 2*group + (1 if records > 128 else 0)
            e = bytearray(32)
            e[1:12] = base+ext
            e[12] = extent&31
            e[14] = extent>>5
            e[15] = records-(128 if records > 128 else 0)
            e[16:16+count] = bytes(blocks)
            slot = slots.pop(0)
            off = 0x4000+32*slot
            raw[off:off+32] = e
            allowed.update(range(off,off+32))
            for j,b in enumerate(blocks):
                off = 0x4000+b*2048
                part = chunk[j*2048:(j+1)*2048].ljust(2048,b'\x1a')
                raw[off:off+2048] = part
                allowed.update(range(off,off+2048))
        rebuilt = b''.join(raw[0x4000+b*2048:0x4000+(b+1)*2048] for b in all_blocks)
        require(rebuilt[:len(data)] == data, 'M139_CPM_ROUNDTRIP')
    require(all(a==b or i in allowed for i,(a,b) in enumerate(zip(original,raw))), 'M139_CPM_EXISTING_CONTENT_CHANGED')
    result = bytearray(image)
    for i,(offset,size) in enumerate(locations):
        result[offset:offset+size] = raw[i*256:(i+1)*256]
    require(unpack(bytes(result))[0] == bytes(raw), 'M139_D88_ROUNDTRIP')
    return bytes(result)

def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source-disk', type=Path, required=True)
    parser.add_argument('--output-dir', type=Path, required=True)
    parser.add_argument('--cache-dir', type=Path, required=True)
    parser.add_argument('--assembler')
    args = parser.parse_args()
    args.output_dir.mkdir(parents=True,exist_ok=True)
    destination = args.output_dir/args.source_disk.name
    require(not destination.exists() and destination.resolve() != args.source_disk.resolve(), 'M139_OUTPUT_EXISTS')
    original = args.source_disk.read_bytes()
    lock = cpm.load_lock(ROOT/'tools/cpmva/sources.lock.json')
    if args.assembler:
        assembler = str(Path(args.assembler).resolve())
        require(cpm.assembler_version_ok(assembler,'1.8'), 'M139_ASSEMBLER_VERSION')
        provenance = {'origin':'explicit verified z80asm 1.8'}
    else:
        # The shared cache stores downloaded archives ONLY. Compiler work and
        # executables always stay in this task's output tree.
        archive = cpm.fetch_assembler_source(lock,args.cache_dir.resolve(),False)
        executable = (args.output_dir/'assembler-tool/z80asm').resolve()
        cpm.build_assembler(archive,(args.output_dir/'assembler-work').resolve(),executable,'1.8')
        assembler = str(executable)
        provenance = {'origin':'locked source, out-of-tree tool build','source_sha256':lock['assembler']['source_sha256']}
    provenance['executable_sha256'] = digest(Path(assembler).read_bytes())
    images = assemble_images(assembler,args.output_dir)
    images['FDCHELP.TXT'] = HERE.joinpath('help.txt').read_bytes().replace(b'\n',b'\r\n') + b'\x1a'
    images['ZEXLIC.TXT'] = (ROOT/'external/zex/LICENSE.txt').read_bytes().replace(b'\r\n',b'\n').replace(b'\n',b'\r\n') + b'\x1a'
    for name,data in images.items():
        (args.output_dir/name).write_bytes(data)
    result = add_files(original,images)
    require(args.source_disk.read_bytes() == original, 'M139_SOURCE_DISK_CHANGED')
    destination.write_bytes(result)
    report = {'schema':'vaeg-m139-fdc-build-v1','input_sha256':digest(original),'output_sha256':digest(result),
              'assembler':provenance,'program_sha256':{n:digest(d) for n,d in images.items() if n.endswith('.COM')},
              'files_sha256':{n:digest(d) for n,d in images.items()},
              'files_size':{n:len(d) for n,d in images.items()},
              'payload_sha256':{p.name:digest(p.read_bytes()) for p in args.output_dir.glob('fdc*.bin')},
              'source_sha256':{str(p.relative_to(ROOT)):digest(p.read_bytes()) for p in [HERE/'build.py',HERE/'loader.asm',HERE/'probe.asm',HERE/'help.txt',ROOT/'external/zex/zexall.src',ROOT/'tools/cpmva/zex/zexbuild.py']},
              'command':sys.argv,
              'status':'BUILT_NOT_HARDWARE_ACCEPTED','physical_target':'FDC monitor family; not main-CPU CP/M execution'}
    (args.output_dir/'build-report.json').write_text(json.dumps(report,indent=2)+'\n')
    print('M139_FDC_BUILD_OK: private disk COPY; hardware validation outstanding')

if __name__ == '__main__':
    try:
        main()
    except BuildError as exc:
        raise SystemExit(exc.code)
