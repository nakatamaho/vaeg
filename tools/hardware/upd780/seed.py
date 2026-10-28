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
"""Bounded seed/readback follow-up. Outputs and private inputs stay out of Git."""
from __future__ import annotations
import argparse
import json
from pathlib import Path
import struct
import sys
import build
import results
from reduction import flag_result

TARGETS = tuple(a << 8 | f for a in (0, 0x55) for f in (8, 15, 40, 41))
RECIPES = ('xor a', 'ld a,0\nor a', 'ld a,8\nor a',
           'ld a,32\nor a', 'ld a,40\nor a')
ROWS = tuple((seed, op, af) for af in TARGETS for seed in range(5) for op in 'SC')
MAGIC = b'M140SD1\0'
MARKER = b'M140_SEED_RAW\r\n'
RESTORED = b'M139_FDC_RAM_RESTORED\r\n'
RAW_SIZE = 8 + 8 * len(ROWS) + 4


def source() -> str:
    captures = []
    table = bytearray(MAGIC)
    for i, (seed, op, af) in enumerate(ROWS):
        table += struct.pack('<BBHHH', seed, ord(op), af, 0, 0)
        for suffix, instruction, field in (('pre', '', 4), ('post', 'scf' if op == 'S' else 'ccf', 6)):
            captures += [f'ld bc,{af}', f'case_{i}_{suffix}:', 'di', RECIPES[seed],
                         'push bc', 'pop af', *([instruction] if instruction else []), 'push af', 'pop de',
                         f'ld ({0x7008 + i * 8 + field}),de']
    table += b'END!'
    text = (build.HERE / 'seed.asm').read_text()
    text = build.replace_once(text, 'CAPTURES:', '\n'.join(captures))
    text = build.replace_once(text, 'TABLE_DATA:', 'db ' + ','.join(map(str, table)))
    return f'RAW_SIZE: equ {RAW_SIZE}\nROW_COUNT: equ {len(ROWS)}\n' + text


def loader_source(payload: bytes) -> str:
    """Derive ONLY the new loader; the old loader file/programs stay unchanged."""
    text = (build.HERE / 'loader.asm').read_text()
    text = build.replace_once(text, 'restore:\n', f'''restore:
    xor a
    ld (streaming),a
    ld hl,(logptr)
    ld de,{RAW_SIZE + 128}
    add hl,de
    ld de,logend
    or a
    sbc hl,de
    jp nc,log_overflow
    ld a,0bh
    call command
    ld hl,7000h
    ld de,{RAW_SIZE}
    call parameters
    ld hl,(logptr)
    ld de,{RAW_SIZE}
seed_read_loop:
    call receive
    ld (hl),a
    inc hl
    dec de
    ld a,d
    or e
    jr nz,seed_read_loop
    ld (logptr),hl
''')
    text = build.replace_once(text, '    incbin \'payload.bin\'', '\n'.join(
        'db ' + ','.join(map(str, payload[i:i+24])) for i in range(0, len(payload), 24)))
    text = build.replace_once(text, '    db LOGNAME', "    db 'FDCSEED DAT'")
    return f'PAYLOAD_SIZE: equ {len(payload)}\n' + text


def assemble(assembler: str, output: Path) -> dict[str, bytes]:
    asm = source()
    payload, symbols = build.zex.assemble_with_symbols(asm, assembler, 'fdcseed')
    build.require(0 < len(payload) < 0x3000 and symbols['initial_table'] + RAW_SIZE <= 0x7000,
                  'M140_SEED_PAYLOAD_OVERLAP')
    loader = loader_source(payload)
    program, main = build.zex.assemble_with_symbols(loader, assembler, 'fdcseed-loader')
    build.require(main['workspace_end'] < 0xd000 and len(program) == main['image_end'] - 0x100,
                  'M140_SEED_WORKSPACE')
    offset = main['payload'] - 0x100
    build.require(program[offset:offset + len(payload)] == payload, 'M139_EMBEDDED_PAYLOAD_MISMATCH')
    output.mkdir(parents=True, exist_ok=True)
    for name, data in {'fdcseed.asm': asm.encode(), 'fdcseed-loader.asm': loader.encode(),
                       'fdcseed.bin': payload, 'FDCSEED.COM': program,
                       'fdcseed-symbols.json': (json.dumps(symbols, indent=2) + '\n').encode()}.items():
        (output / name).write_bytes(data)
    return {'FDCSEED.COM': program}


def content(data: bytes, *, recovery: bool = True, host_model: bool = False) -> dict:
    results.need(data.count(MARKER) == 1, 'M140_SEED_RAW_MARKER')
    text, binary = data.split(MARKER)
    results.need(len(binary) >= RAW_SIZE and binary[:8] == MAGIC and
                 binary[RAW_SIZE-4:RAW_SIZE] == b'END!', 'M140_SEED_BINARY_FRAME')
    tail = binary[RAW_SIZE:]
    if recovery:
        results.need(tail.startswith(RESTORED), 'M139_RECOVERY_RECORD')
        tail = tail[len(RESTORED):]
    results.need(all(x == 0x1a for x in tail), 'M140_SEED_PADDING')
    try:
        lines = text.decode('ascii').splitlines()
    except UnicodeError:
        raise results.RecordError('M140_SEED_TEXT_FORMAT') from None
    results.need(len(lines) == len(ROWS) + 1 and lines[0] == 'M140 FDC SEED 1',
                 'M140_SEED_TEXT_SET')
    differences, groups = [], {}
    for i, (seed, op, af) in enumerate(ROWS):
        s, o, requested, before, after = struct.unpack_from('<BBHHH', binary, 8 + i * 8)
        results.need((s, o, requested) == (seed, ord(op), af), 'M140_SEED_DESCRIPTOR')
        results.need(before == af, 'M140_SEED_PRECONDITION')
        predicted = flag_result(op, af >> 8, af & 255, af >> 8)
        results.need((after & 0xffd7) == (predicted & 0xffd7), 'M140_SEED_DOCUMENTED_FLAGS')
        expected_line = f'B {seed:02X} {op} {af:04X} {before:04X} {after:04X}'
        results.need(lines[i+1] == expected_line, 'M140_SEED_PATH_MISMATCH')
        if after != predicted:
            differences.append({'seed': seed, 'operation': op, 'af': f'{af:04X}',
                                'after': f'{after:04X}', 'host': f'{predicted:04X}'})
        groups.setdefault((af, op), []).append(after)
    if host_model:
        results.need(not differences, 'M140_SEED_HOST_MISMATCH')
    return {'status': 'COMPLETE_OBSERVATIONS', 'records': len(ROWS),
            'text_raw_agreement': True, 'recovery_record': recovery,
            'seed_dependent_groups': [{'af': f'{af:04X}', 'operation': op}
                                     for (af, op), values in groups.items() if len(set(values)) > 1],
            'host_baseline_differences': differences,
            'reference_class': 'seed instructions and shared captured RAM; NOT a physical Q read or silicon acceptance'}


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest='action', required=True)
    p = commands.add_parser('build')
    p.add_argument('--source-disk', type=Path, required=True)
    p.add_argument('--output-dir', type=Path, required=True)
    p.add_argument('--assembler', required=True)
    p = commands.add_parser('check')
    p.add_argument('input', type=Path)
    p.add_argument('--output', type=Path, required=True)
    p.add_argument('--host-model', action='store_true')
    args = parser.parse_args()
    if args.action == 'check':
        args.output.write_text(json.dumps(content(args.input.read_bytes(), host_model=args.host_model), indent=2) + '\n')
        print('M140_SEED_OBSERVATIONS_COMPLETE (not silicon acceptance)')
        return
    build.require(not args.output_dir.exists(), 'M139_OUTPUT_EXISTS')
    build.require(build.cpm.assembler_version_ok(args.assembler, '1.8'), 'M139_ASSEMBLER_VERSION')
    original = args.source_disk.read_bytes()
    images = assemble(args.assembler, args.output_dir)
    images['SEEDHELP.TXT'] = (build.HERE / 'seed-help.txt').read_bytes().replace(b'\n', b'\r\n') + b'\x1a'
    result = build.add_files(original, images)
    space = build.log_headroom(result, {'FDCSEED.DAT': 4096})
    build.require(args.source_disk.read_bytes() == original, 'M139_SOURCE_DISK_CHANGED')
    destination = args.output_dir / 'fdc-seed-r1.d88'
    destination.write_bytes(result)
    report = {'schema': 'vaeg-m140-fdc-seed-build-v1', 'status': 'BUILT_NOT_HARDWARE_ACCEPTED',
              'input_sha256': build.digest(original), 'output_sha256': build.digest(result),
              'files_sha256': {n: build.digest(d) for n, d in images.items()},
              'files_size': {n: len(d) for n, d in images.items()}, 'log_headroom': space,
              'assembler_sha256': build.digest(Path(args.assembler).read_bytes()),
              'source_sha256': {p.name: build.digest(p.read_bytes()) for p in (
                  Path(__file__), build.HERE/'seed.asm', build.HERE/'loader.asm',
                  build.HERE/'build.py', build.HERE/'seed-help.txt')}, 'command': sys.argv}
    (args.output_dir / 'build-report.json').write_text(json.dumps(report, indent=2) + '\n')
    print('M140_SEED_BUILD_OK (private copy, not hardware acceptance)')


if __name__ == '__main__':
    try:
        main()
    except (build.BuildError, results.RecordError) as exc:
        raise SystemExit(exc.code)
