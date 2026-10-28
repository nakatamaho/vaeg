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
"""Capture/stack/storage controls; findings are diagnostic, never a CPU fix."""
from __future__ import annotations
import argparse
import json
from pathlib import Path
import struct
import sys
import build
import fixed
import results
import seed
from reduction import flag_result

BASE = tuple((s, op, af) for af in (0x28, 0x29) for s in (0, 1, 3) for op in 'SC')
ROWS = tuple((r, s, op, af) for r in range(8)
             for s, op, af in (BASE if r % 2 == 0 else tuple(reversed(BASE))))
MAGIC = b'M140CP1\0'
MARKER = b'M140_CAPTURE_RAW\r\n'
RAW_SIZE = 20 + 22*len(ROWS) + 4
FIELDS = ('target', 'before_a', 'before_b', 'before_c', 'after_a', 'after_b',
          'after_c', 'mirror', 'inverse')


def source() -> str:
    header = MAGIC + bytes(12)
    table = header + b''.join(struct.pack('<BBB9HB', r, s, ord(op), af, *([0]*8), 0)
                              for r, s, op, af in ROWS) + b'END!'
    text = build.replace_once((build.HERE/'capture.asm').read_text(), 'TABLE_DATA:',
                              'db '+','.join(map(str, table)))
    return f'RAW_SIZE: equ {RAW_SIZE}\nROW_COUNT: equ {len(ROWS)}\n'+text


def loader_source(payload: bytes) -> str:
    text = fixed.loader_source(payload)
    for old, new in (
        (f'    ld de,{fixed.RAW_SIZE+128}\n', f'    ld de,{RAW_SIZE+128}\n'),
        (f'    ld hl,7000h\n    ld de,{fixed.RAW_SIZE}\n', f'    ld hl,7000h\n    ld de,{RAW_SIZE}\n'),
        (f'    ld hl,(logptr)\n    ld de,{fixed.RAW_SIZE}\n', f'    ld hl,(logptr)\n    ld de,{RAW_SIZE}\n'),
        ("    db 'FDCFIX  DAT'", "    db 'FDCCAP  DAT'"),
        ('M140_FIXED_PATCH_VERIFY', 'M140_CAPTURE_PATCH_VERIFY'),
    ):
        text = build.replace_once(text, old, new)
    return text


def assemble(assembler: str, output: Path) -> dict[str, bytes]:
    asm = source()
    payload, sym = build.zex.assemble_with_symbols(asm, assembler, 'fdccap')
    build.require(0 < len(payload) < 0x3000 and sym['initial_table']+RAW_SIZE <= 0x7000
                  and 0x7000+RAW_SIZE <= 0x7d00 and sym['sampler'] == 0x4800
                  and sym['sampler_iut'] == 0x4806, 'M140_CAPTURE_LAYOUT')
    loader = loader_source(payload)
    program, main = build.zex.assemble_with_symbols(loader, assembler, 'fdccap-loader')
    build.require(main['workspace_end'] < 0xd000 and len(program) == main['image_end']-0x100,
                  'M140_CAPTURE_WORKSPACE')
    offset = main['payload']-0x100
    build.require(program[offset:offset+len(payload)] == payload, 'M139_EMBEDDED_PAYLOAD_MISMATCH')
    output.mkdir(parents=True, exist_ok=True)
    for name, data in {'fdccap.asm':asm.encode(), 'fdccap-loader.asm':loader.encode(),
                       'fdccap.bin':payload, 'FDCCAP.COM':program,
                       'fdccap-symbols.json':(json.dumps(sym,indent=2)+'\n').encode(),
                       'fdccap-loader-symbols.json':(json.dumps(main,indent=2)+'\n').encode()}.items():
        (output/name).write_bytes(data)
    return {'FDCCAP.COM':program}


def content(data: bytes, *, recovery: bool = True, host_model: bool = False) -> dict:
    results.need(data.count(MARKER) == 1, 'M140_CAPTURE_RAW_MARKER')
    text, raw = data.split(MARKER)
    results.need(len(raw) >= RAW_SIZE and raw[:8] == MAGIC and raw[RAW_SIZE-4:RAW_SIZE] == b'END!',
                 'M140_CAPTURE_BINARY_FRAME')
    pre_count, pre_addr, patterns, reserved, delayed, delayed_addr, reserved2 = struct.unpack_from('<HHBBHHH',raw,8)
    results.need(reserved == reserved2 == 0, 'M140_CAPTURE_HEADER')
    results.need(patterns == 0x7f, 'M140_CAPTURE_PATTERN_SET')
    tail = raw[RAW_SIZE:]
    if recovery:
        results.need(tail.startswith(seed.RESTORED), 'M139_RECOVERY_RECORD')
        tail = tail[len(seed.RESTORED):]
    results.need(all(x == 0x1a for x in tail), 'M140_CAPTURE_PADDING')
    try:lines = text.decode('ascii').splitlines()
    except UnicodeError:raise results.RecordError('M140_CAPTURE_TEXT_FORMAT') from None
    results.need(len(lines) == len(ROWS)+1 and lines[0] == 'M140 FDC CAPTURE 1', 'M140_CAPTURE_TEXT_SET')
    findings, host_diff, or_diff, mirror_bad, groups = [], [], [], [], {}
    for i,(r,s,op,af) in enumerate(ROWS):
        rr,ss,oo,*values,status = struct.unpack_from('<BBB9HB',raw,20+22*i)
        results.need((rr,ss,oo,values[0]) == (r,s,ord(op),af), 'M140_CAPTURE_DESCRIPTOR')
        results.need(status & ~7 == 0, 'M140_CAPTURE_STATUS')
        encoded = ' '.join(f'{v:04X}' for v in values)
        results.need(lines[i+1] == f'D {r:02X} {s:02X} {op} {encoded} {status:02X}',
                     'M140_CAPTURE_PATH_MISMATCH')
        v = dict(zip(FIELDS,values))
        control_equal = v['before_a'] == v['before_b'] == v['before_c']
        post_equal = v['after_a'] == v['after_b'] == v['after_c']
        mirror_equal = v['mirror'] == v['after_a'] and v['inverse'] == (v['after_a'] ^ 0xffff)
        predicted = flag_result(op,af>>8,af&255,af>>8)
        kinds = []
        if any(v[k] != af for k in ('before_a','before_b','before_c')):kinds.append('control_target')
        if not control_equal:kinds.append('control_triple')
        if not post_equal:kinds.append('post_triple')
        if any((v[k]&0xffd7) != (predicted&0xffd7) for k in ('after_a','after_b','after_c')):
            kinds.append('post_documented_flags')
        if status & 1:kinds.append('reported_immediate_store')
        if status & 2:kinds.append('reported_control_triple')
        if status & 4:kinds.append('reported_post_triple')
        if bool(status&2) == control_equal:kinds.append('control_status_contradiction')
        if bool(status&4) == post_equal:kinds.append('post_status_contradiction')
        if not mirror_equal:
            kinds.append('mirror_relation')
            mirror_bad.append(i)
        identity = {'repeat':r,'seed':s,'operation':op,'af':f'{af:04X}'}
        if kinds:findings.append({**identity,'kinds':kinds,'words':{k:f'{x:04X}' for k,x in v.items()},'status':status})
        for expected, destination in ((predicted,host_diff),
                                      (flag_result(op,af>>8,af&255,(af>>8)|(af&255)),or_diff)):
            if v['after_a'] != expected:destination.append({**identity,'after':f'{v["after_a"]:04X}','expected':f'{expected:04X}'})
        groups.setdefault((af,op,s),[]).append(v['after_a'])
    header_findings = []
    if pre_count:header_findings.append('pre_ram_failures')
    if (pre_count == 0 and pre_addr != 0) or (pre_count and not 0x7000 <= pre_addr < 0x7000+RAW_SIZE):
        header_findings.append('pre_address_contradiction')
    expected_first = 0x7014+22*mirror_bad[0]+11 if mirror_bad else 0
    if delayed:header_findings.append('reported_delayed_failures')
    if delayed != len(mirror_bad) or delayed_addr != expected_first:header_findings.append('delayed_summary_contradiction')
    integrity = bool(findings or header_findings)
    if host_model:
        results.need(not integrity, 'M140_CAPTURE_HOST_INTEGRITY')
        results.need(not host_diff, 'M140_CAPTURE_HOST_MISMATCH')
    return {'status':'COMPLETE_OBSERVATIONS', 'integrity_status':'FINDINGS' if integrity else 'NO_FINDINGS_IN_TESTED_CONTRACT',
            'records':len(ROWS), 'repetitions':8, 'text_raw_agreement':True, 'recovery_record':recovery,
            'ram_pretest':{'failures':pre_count,'first_address':f'{pre_addr:04X}','completed_mask':patterns},
            'delayed_scan':{'reported_failures':delayed,'first_address':f'{delayed_addr:04X}','decoded_mirror_failures':len(mirror_bad)},
            'header_findings':header_findings, 'record_findings':findings,
            'primary_host_differences':host_diff, 'primary_or_differences':or_diff,
            'primary_repeat_varying_groups':[{'af':f'{af:04X}','operation':op,'seed':s,'values':[f'{x:04X}' for x in vs]}
                                             for (af,op,s),vs in groups.items() if len(set(vs))>1],
            'reference_class':'three PUSH-AF stack cells, code-RAM mailboxes and row-specific mirrors; no independent CPU/Q readout or exhaustive RAM qualification'}


def main():
    p=argparse.ArgumentParser(description=__doc__)
    sub=p.add_subparsers(dest='action',required=True)
    b=sub.add_parser('build')
    b.add_argument('--source-disk',type=Path,required=True)
    b.add_argument('--output-dir',type=Path,required=True)
    b.add_argument('--assembler',required=True)
    c=sub.add_parser('check')
    c.add_argument('input',type=Path)
    c.add_argument('--output',type=Path,required=True)
    c.add_argument('--host-model',action='store_true')
    a=p.parse_args()
    if a.action=='check':
        a.output.write_text(json.dumps(content(a.input.read_bytes(),host_model=a.host_model),indent=2)+'\n')
        print('M140_CAPTURE_OBSERVATIONS_COMPLETE (inspect integrity findings; not silicon acceptance)')
        return
    build.require(not a.output_dir.exists(),'M139_OUTPUT_EXISTS')
    assembler=str(Path(a.assembler).resolve())
    build.require(build.cpm.assembler_version_ok(assembler,'1.8'),'M139_ASSEMBLER_VERSION')
    original=a.source_disk.read_bytes()
    build.require('FDCCAP.DAT' not in results.disk_files(original),'M140_CAPTURE_LOG_EXISTS')
    images=assemble(assembler,a.output_dir)
    images['CAPHELP.TXT']=(build.HERE/'capture-help.txt').read_bytes().replace(b'\n',b'\r\n')+b'\x1a'
    image=build.add_files(original,images)
    space=build.log_headroom(image,{'FDCCAP.DAT':12288})
    build.require(a.source_disk.read_bytes()==original,'M139_SOURCE_DISK_CHANGED')
    (a.output_dir/'fdc-capture-r1.d88').write_bytes(image)
    sym=json.loads((a.output_dir/'fdccap-symbols.json').read_text())
    main_sym=json.loads((a.output_dir/'fdccap-loader-symbols.json').read_text())
    report={'schema':'vaeg-m140-fdc-capture-build-v1','status':'BUILT_NOT_HARDWARE_ACCEPTED',
            'input_sha256':build.digest(original),'output_sha256':build.digest(image),
            'files_sha256':{n:build.digest(d) for n,d in images.items()},'files_size':{n:len(d) for n,d in images.items()},
            'log_headroom':space,'assembler_sha256':build.digest(Path(assembler).read_bytes()),
            'sampler_addresses':{k:sym[k] for k in ('sampler','sampler_iut','control_call','measured_call')},
            'patch_check_file_offset':main_sym['payload']-0x100+sym['patch_check']-0x4000,
            'source_sha256':{f.name:build.digest(f.read_bytes()) for f in (
                Path(__file__),build.HERE/'capture.asm',build.HERE/'capture-help.txt',build.HERE/'fixed.py',
                build.HERE/'seed.py',build.HERE/'loader.asm',build.HERE/'build.py')},'command':sys.argv}
    (a.output_dir/'build-report.json').write_text(json.dumps(report,indent=2)+'\n')
    print('M140_CAPTURE_BUILD_OK (private copy; physical tests outstanding)')


if __name__=='__main__':
    try:main()
    except (build.BuildError,results.RecordError) as exc:raise SystemExit(exc.code)
