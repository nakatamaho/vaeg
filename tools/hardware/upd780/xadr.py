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
"""FDC sampler/caller relocation factorial; no assumption of address causality."""
import argparse
import json
from pathlib import Path
import struct
import sys
import build
import capture
import results

PARENT_SHA256='c1c2526506547b147b3b40af0e2f2d0a277d6ebd535c6b692badb83806554e51'
BASE=tuple((site,op,af) for af in (0x28,0x29) for site in range(4) for op in 'SC')
ROWS=tuple((r,site,op,af) for r in range(8)
           for site,op,af in (BASE if r%2==0 else tuple(reversed(BASE))))
MAGIC=b'M140XA1\0'
MARKER=b'M140_XADR_RAW\r\n'
RAW_SIZE=20+22*len(ROWS)+4
SAMPLER=b'\xf3\x3e\x00\xaf\xc5\xf1\x00\xf5\xf5\xf5\xe1\xc1\xd1\xc9'


def sites():
    return {s:{'sampler':0x4800+8*(s>>1),'iut':0x4806+8*(s>>1),
               'caller':0x48fe+8*(s&1),'call':0x4904+8*(s&1),
               'return':0x4907+8*(s&1)} for s in range(4)}


def source():
    return derive((build.HERE/'capture.asm').read_bytes(),(build.HERE/'xadr.asm').read_text())


def derive(parent,fragments):
    """Content-only checked derivation; no Git or private assets required."""
    build.require(build.digest(parent)==PARENT_SHA256,'M140_XADR_PARENT')
    parts=fragments.split('; @')
    build.require(len(parts)==4 and parts[1].startswith('loop\n') and
                  parts[2].startswith('installer\n') and parts[3].startswith('banks\n'),'M140_XADR_SOURCE')
    text=parent.decode()
    for start,end,part in [('capture_loop:\n','mirror_load:\n',parts[1]),
                           ('install_seed:\n','patch_failed:\n',parts[2]),
                           ('    ds 4800h-$\n','TABLE_DATA:\n',parts[3])]:
        a=text.index(start);b=text.index(end,a)+(len(end) if end=='TABLE_DATA:\n' else 0)
        text=build.replace_once(text,text[a:b],part.split('\n',1)[1])
    text=build.replace_once(text,"'M140 FDC CAPTURE 1'","'M140 FDC XADR 1'")
    text=build.replace_once(text,"'M140_CAPTURE_RAW'","'M140_XADR_RAW'")
    text=build.replace_once(text,"    ld a,'D'","    ld a,'X'")
    table=MAGIC+bytes(12)+b''.join(struct.pack('<BBB9HB',r,s,ord(op),af,*([0]*8),0)
                                     for r,s,op,af in ROWS)+b'END!'
    text=build.replace_once(text,'TABLE_DATA:','db '+','.join(map(str,table)))
    return f'RAW_SIZE: equ {RAW_SIZE}\nROW_COUNT: equ {len(ROWS)}\n'+text


def loader_source(payload):
    text=capture.loader_source(payload)
    for old,new in ((f'    ld de,{capture.RAW_SIZE+128}\n',f'    ld de,{RAW_SIZE+128}\n'),
                    (f'    ld hl,7000h\n    ld de,{capture.RAW_SIZE}\n',f'    ld hl,7000h\n    ld de,{RAW_SIZE}\n'),
                    (f'    ld hl,(logptr)\n    ld de,{capture.RAW_SIZE}\n',f'    ld hl,(logptr)\n    ld de,{RAW_SIZE}\n'),
                    ("    db 'FDCCAP  DAT'","    db 'FDCXADR DAT'"),
                    ('M140_CAPTURE_PATCH_VERIFY','M140_XADR_PATCH_VERIFY')):
        text=build.replace_once(text,old,new)
    return text


def assemble(assembler,output):
    asm=source();payload,sym=build.zex.assemble_with_symbols(asm,assembler,'fdcxadr')
    build.require(0<len(payload)<0x3000 and sym['initial_table']+RAW_SIZE<=0x7000
                  and 0x7000+RAW_SIZE<=0x7d00 and sym['sampler_low']==0x4800
                  and sym['sampler_high']==0x4808 and sym['caller_low']==0x48fe
                  and sym['caller_high']==0x4906,'M140_XADR_LAYOUT')
    build.require(payload[sym['sampler_template']-0x4000:sym['sampler_template']-0x4000+14]==SAMPLER,
                  'M140_XADR_SAMPLER_TEMPLATE')
    wrapper=b'\x11\x00\x00\x21\x00\x00\xcd\x00\x48\xc3'+struct.pack('<H',sym['capture_return'])
    build.require(payload[sym['caller_template']-0x4000:sym['caller_template']-0x4000+12]==wrapper,
                  'M140_XADR_CALLER_TEMPLATE')
    loader=loader_source(payload);program,main=build.zex.assemble_with_symbols(loader,assembler,'fdcxadr-loader')
    build.require(main['workspace_end']<0xd000 and len(program)==main['image_end']-0x100,'M140_XADR_WORKSPACE')
    offset=main['payload']-0x100
    build.require(program[offset:offset+len(payload)]==payload,'M139_EMBEDDED_PAYLOAD_MISMATCH')
    output.mkdir(parents=True,exist_ok=True)
    for name,data in {'fdcxadr.asm':asm.encode(),'fdcxadr-loader.asm':loader.encode(),
                      'fdcxadr.bin':payload,'FDCXADR.COM':program,
                      'fdcxadr-symbols.json':(json.dumps(sym,indent=2)+'\n').encode(),
                      'fdcxadr-loader-symbols.json':(json.dumps(main,indent=2)+'\n').encode()}.items():
        (output/name).write_bytes(data)
    return {'FDCXADR.COM':program}


def content(data,*,recovery=True,host_model=False):
    try:
        report=capture.row_content(data,rows=ROWS,magic=MAGIC,marker=MARKER,
                                   banner='M140 FDC XADR 1',tag='X',descriptor='site',
                                   recovery=recovery,host_model=host_model)
    except results.RecordError as exc:
        raise results.RecordError(exc.code.replace('M140_CAPTURE_','M140_XADR_')) from None
    raw=data.split(MARKER)[1];groups={}
    for i,(r,s,op,af) in enumerate(ROWS):
        after=struct.unpack_from('<H',raw,20+22*i+11)[0]
        groups.setdefault((r,af,op),{})[s]=after
    effects={'sampler_axis':[],'caller_axis':[]}
    for (r,af,op),v in groups.items():
        for axis,pairs in [('sampler_axis',((0,2),(1,3))),('caller_axis',((0,1),(2,3)))]:
            for lo,hi in pairs:
                if v[lo]!=v[hi]:effects[axis].append({'repeat':r,'af':f'{af:04X}','operation':op,
                                                    'sites':[lo,hi],'values':[f'{v[lo]:04X}',f'{v[hi]:04X}']})
    report.update(site_addresses=sites(),paired_effects=effects,
                  interpretation='Relocated sampler and caller banks, not isolated PC-bit causality. Full code-bank copy/readback is not a physical fetch trace. Shared CPU/export paths and phase/history remain. Treat paired effects as diagnostic if integrity findings exist.')
    return report


def main():
    p=argparse.ArgumentParser(description=__doc__);sub=p.add_subparsers(dest='action',required=True)
    b=sub.add_parser('build');b.add_argument('--source-disk',type=Path,required=True)
    b.add_argument('--output-dir',type=Path,required=True);b.add_argument('--assembler',required=True)
    c=sub.add_parser('check');c.add_argument('input',type=Path);c.add_argument('--output',type=Path,required=True)
    c.add_argument('--host-model',action='store_true');a=p.parse_args()
    if a.action=='check':
        a.output.write_text(json.dumps(content(a.input.read_bytes(),host_model=a.host_model),indent=2)+'\n')
        print('M140_XADR_OBSERVATIONS_COMPLETE (inspect integrity findings; not cause or silicon acceptance)');return
    build.require(not a.output_dir.exists(),'M139_OUTPUT_EXISTS')
    assembler=str(Path(a.assembler).resolve());build.require(build.cpm.assembler_version_ok(assembler,'1.8'),'M139_ASSEMBLER_VERSION')
    original=a.source_disk.read_bytes();build.require('FDCXADR.DAT' not in results.disk_files(original),'M140_XADR_LOG_EXISTS')
    images=assemble(assembler,a.output_dir)
    images['XADRHELP.TXT']=(build.HERE/'xadr-help.txt').read_bytes().replace(b'\n',b'\r\n')+b'\x1a'
    image=build.add_files(original,images);space=build.log_headroom(image,{'FDCXADR.DAT':12288})
    build.require(a.source_disk.read_bytes()==original,'M139_SOURCE_DISK_CHANGED')
    (a.output_dir/'fdc-xadr-r1.d88').write_bytes(image)
    sym=json.loads((a.output_dir/'fdcxadr-symbols.json').read_text());main_sym=json.loads((a.output_dir/'fdcxadr-loader-symbols.json').read_text())
    report={'schema':'vaeg-m140-fdc-xadr-build-v1','status':'BUILT_NOT_HARDWARE_ACCEPTED',
            'input_sha256':build.digest(original),'output_sha256':build.digest(image),
            'files_sha256':{n:build.digest(d) for n,d in images.items()},'files_size':{n:len(d) for n,d in images.items()},
            'log_headroom':space,'assembler_sha256':build.digest(Path(assembler).read_bytes()),'site_addresses':sites(),
            'patch_check_file_offset':main_sym['payload']-0x100+sym['patch_check']-0x4000,
            'source_sha256':{f.name:build.digest(f.read_bytes()) for f in (
                Path(__file__),build.HERE/'xadr.asm',build.HERE/'xadr-help.txt',build.HERE/'capture.asm',
                build.HERE/'capture.py',build.HERE/'fixed.py',build.HERE/'seed.py',build.HERE/'loader.asm',build.HERE/'build.py')},
            'command':sys.argv}
    (a.output_dir/'build-report.json').write_text(json.dumps(report,indent=2)+'\n')
    print('M140_XADR_BUILD_OK (private copy; physical results NOT_RUN)')


if __name__=='__main__':
    try:main()
    except (build.BuildError,results.RecordError) as exc:raise SystemExit(exc.code)
