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
"""Fixed-address, repeated seed controls. No inferred physical Q or timing."""
from __future__ import annotations
import argparse
import json
from pathlib import Path
import struct
import sys
import build
import results
import seed
from reduction import flag_result

BASE = tuple((s, op, af) for af in (0x28, 0x29) for s in range(5) for op in 'SC')
ROWS = tuple((r, s, op, af) for r in range(8)
             for s, op, af in (BASE if r % 2 == 0 else tuple(reversed(BASE))))
MAGIC = b'M140FX1\0'
MARKER = b'M140_FIXED_RAW\r\n'
RAW_SIZE = 8 + 9 * len(ROWS) + 4


def source() -> str:
    raw = MAGIC + b''.join(struct.pack('<BBBHHH', r, s, ord(op), af, 0, 0)
                            for r, s, op, af in ROWS) + b'END!'
    text = build.replace_once((build.HERE/'fixed.asm').read_text(), 'TABLE_DATA:',
                              'db ' + ','.join(map(str, raw)))
    return f'RAW_SIZE: equ {RAW_SIZE}\nROW_COUNT: equ {len(ROWS)}\n' + text


def loader_source(payload: bytes) -> str:
    # Exact-context derivation; never change the previous seed/legacy programs.
    text = seed.loader_source(payload)
    for old, new in (
        (f'    ld de,{seed.RAW_SIZE + 128}\n', f'    ld de,{RAW_SIZE + 128}\n'),
        (f'    ld hl,7000h\n    ld de,{seed.RAW_SIZE}\n', f'    ld hl,7000h\n    ld de,{RAW_SIZE}\n'),
        (f'    ld hl,(logptr)\n    ld de,{seed.RAW_SIZE}\n', f'    ld hl,(logptr)\n    ld de,{RAW_SIZE}\n'),
        ("    db 'FDCSEED DAT'", "    db 'FDCFIX  DAT'"),
        ('    cp 4\n    jr z,restore', '    cp 3\n    jp z,fixed_patch_error\n    cp 4\n    jr z,restore'),
        ('fatal:\n', "fixed_patch_error:\n    ld de,fixed_patch_msg\n    jp fatal_common\nfixed_patch_msg: db 'M140_FIXED_PATCH_VERIFY: reset; no disk I/O',13,10,'$'\nfatal:\n"),
    ):
        text = build.replace_once(text, old, new)
    return text


def assemble(assembler: str, output: Path) -> dict[str, bytes]:
    asm = source()
    payload, sym = build.zex.assemble_with_symbols(asm, assembler, 'fdcfix')
    build.require(0 < len(payload) < 0x3000 and sym['initial_table'] + RAW_SIZE <= 0x7000
                  and 0x7000 + RAW_SIZE <= 0x7d00, 'M140_FIXED_LAYOUT')
    loader = loader_source(payload)
    program, main = build.zex.assemble_with_symbols(loader, assembler, 'fdcfix-loader')
    build.require(main['workspace_end'] < 0xd000 and len(program) == main['image_end'] - 0x100,
                  'M140_FIXED_WORKSPACE')
    offset = main['payload'] - 0x100
    build.require(program[offset:offset+len(payload)] == payload, 'M139_EMBEDDED_PAYLOAD_MISMATCH')
    output.mkdir(parents=True, exist_ok=True)
    for name, data in {'fdcfix.asm': asm.encode(), 'fdcfix-loader.asm': loader.encode(),
                       'fdcfix.bin': payload, 'FDCFIX.COM': program,
                       'fdcfix-symbols.json': (json.dumps(sym, indent=2)+'\n').encode(),
                       'fdcfix-loader-symbols.json': (json.dumps(main, indent=2)+'\n').encode()}.items():
        (output/name).write_bytes(data)
    return {'FDCFIX.COM': program}


def content(data: bytes, *, recovery: bool = True, host_model: bool = False) -> dict:
    results.need(data.count(MARKER) == 1, 'M140_FIXED_RAW_MARKER')
    text, raw = data.split(MARKER)
    results.need(len(raw) >= RAW_SIZE and raw[:8] == MAGIC and raw[RAW_SIZE-4:RAW_SIZE] == b'END!',
                 'M140_FIXED_BINARY_FRAME')
    tail = raw[RAW_SIZE:]
    if recovery:
        results.need(tail.startswith(seed.RESTORED), 'M139_RECOVERY_RECORD')
        tail = tail[len(seed.RESTORED):]
    results.need(all(x == 0x1a for x in tail), 'M140_FIXED_PADDING')
    try:
        lines = text.decode('ascii').splitlines()
    except UnicodeError:
        raise results.RecordError('M140_FIXED_TEXT_FORMAT') from None
    results.need(len(lines) == len(ROWS)+1 and lines[0] == 'M140 FDC FIXED 1', 'M140_FIXED_TEXT_SET')
    groups, differences = {}, []
    for i, (r, s, op, af) in enumerate(ROWS):
        rr, ss, oo, target, before, after = struct.unpack_from('<BBBHHH', raw, 8+i*9)
        results.need((rr, ss, oo, target) == (r, s, ord(op), af), 'M140_FIXED_DESCRIPTOR')
        results.need(before == af, 'M140_FIXED_PRECONDITION')
        predicted = flag_result(op, af >> 8, af & 255, af >> 8)
        results.need((after & 0xffd7) == (predicted & 0xffd7), 'M140_FIXED_DOCUMENTED_FLAGS')
        results.need(lines[i+1] == f'R {r:02X} {s:02X} {op} {af:04X} {before:04X} {after:04X}',
                     'M140_FIXED_PATH_MISMATCH')
        groups.setdefault((af, op, s), []).append(after)
        if after != predicted:
            differences.append({'repeat': r, 'seed': s, 'operation': op,
                                'af': f'{af:04X}', 'after': f'{after:04X}', 'host': f'{predicted:04X}'})
    unstable = [{'af': f'{af:04X}', 'operation': op, 'seed': s,
                 'results': [f'{v:04X}' for v in values]}
                for (af, op, s), values in groups.items() if len(set(values)) > 1]
    by_target = {}
    for (af, op, s), values in groups.items():
        by_target.setdefault((af, op), {})[str(s)] = sorted({f'{x:04X}' for x in values})
    seed_groups = [{'af': f'{af:04X}', 'operation': op, 'seeds': values}
                   for (af, op), values in by_target.items()
                   if len({tuple(v) for v in values.values()}) > 1]
    if host_model:
        results.need(not differences, 'M140_FIXED_HOST_MISMATCH')
    return {'status': 'COMPLETE_OBSERVATIONS', 'records': len(ROWS), 'repetitions': 8,
            'order_policy': 'alternating forward/reverse', 'text_raw_agreement': True,
            'recovery_record': recovery, 'repeat_varying_groups': unstable,
            'seed_different_result_sets': seed_groups, 'host_baseline_differences': differences,
            'reference_class': 'common sampler address, verified RAM patches; not physical Q or silicon acceptance'}


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
    a = parser.parse_args()
    if a.action == 'check':
        a.output.write_text(json.dumps(content(a.input.read_bytes(), host_model=a.host_model), indent=2)+'\n')
        print('M140_FIXED_OBSERVATIONS_COMPLETE (not silicon acceptance)')
        return
    build.require(not a.output_dir.exists(), 'M139_OUTPUT_EXISTS')
    assembler = str(Path(a.assembler).resolve())
    build.require(build.cpm.assembler_version_ok(assembler, '1.8'), 'M139_ASSEMBLER_VERSION')
    original = a.source_disk.read_bytes()
    build.require('FDCFIX.DAT' not in results.disk_files(original), 'M140_FIXED_LOG_EXISTS')
    images = assemble(assembler, a.output_dir)
    images['FIXHELP.TXT'] = (build.HERE/'fixed-help.txt').read_bytes().replace(b'\n', b'\r\n')+b'\x1a'
    image = build.add_files(original, images)
    space = build.log_headroom(image, {'FDCFIX.DAT': 8192})
    build.require(a.source_disk.read_bytes() == original, 'M139_SOURCE_DISK_CHANGED')
    (a.output_dir/'fdc-fixed-r1.d88').write_bytes(image)
    symbols = json.loads((a.output_dir/'fdcfix-symbols.json').read_text())
    loader_symbols = json.loads((a.output_dir/'fdcfix-loader-symbols.json').read_text())
    report = {'schema': 'vaeg-m140-fdc-fixed-build-v1', 'status': 'BUILT_NOT_HARDWARE_ACCEPTED',
              'input_sha256': build.digest(original), 'output_sha256': build.digest(image),
              'files_sha256': {n: build.digest(d) for n, d in images.items()},
              'files_size': {n: len(d) for n, d in images.items()}, 'log_headroom': space,
              'assembler_sha256': build.digest(Path(assembler).read_bytes()),
              'patch_check_file_offset': loader_symbols['payload'] - 0x100 + symbols['patch_check'] - 0x4000,
              'sampler_addresses': {k: symbols[k] for k in ('sampler','sampler_seed','sampler_alu','sampler_iut','control_call','measured_call')},
              'source_sha256': {p.name: build.digest(p.read_bytes()) for p in (
                  Path(__file__), build.HERE/'fixed.asm', build.HERE/'fixed-help.txt',
                  build.HERE/'seed.py', build.HERE/'loader.asm', build.HERE/'build.py')}, 'command': sys.argv}
    (a.output_dir/'build-report.json').write_text(json.dumps(report, indent=2)+'\n')
    print('M140_FIXED_BUILD_OK (private copy, not hardware acceptance)')


if __name__ == '__main__':
    try:
        main()
    except (build.BuildError, results.RecordError) as exc:
        raise SystemExit(exc.code)
