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
"""Fixed-position CP-writer / NOP / DD / FD history discriminator; not a CPU fix."""
import argparse
import json
from pathlib import Path
import struct
import sys
import build
import capture
import results
import xadr
from reduction import flag_result

BASE=tuple((mode,op,af) for af in (0x00bb,0x286a) for mode in range(4) for op in 'SC')
ROWS=tuple((r,m,op,af) for r in range(8) for m,op,af in (BASE if r%2==0 else tuple(reversed(BASE))))
MAGIC=b'M140PF1\0'
MARKER=b'M140_PREF_RAW\r\n'
RAW_SIZE=2840
PROFILES=('nec_a','zilog_q','st_cmos','unconditional_or')
MODES=('NOP before CP B; CP immediately before IUT','CP B; NOP; IUT',
       'CP B; DD IUT','CP B; FD IUT')

def sampler(mode):
    build.require(mode in range(4),'M140_PREF_MODE')
    middle=b'\x00\xb8' if mode==0 else bytes((0xb8,(0,0xdd,0xfd)[mode-1]))
    return b'\xf3\x3e\x00\xaf\xc5\xf1\x06\x28'+middle+b'\x00\xf5\xf5\xf5\xe1\xc1\xd1\xc9'

def prediction(op,af,mode,profile):
    results.need(profile in PROFILES,'M140_PREF_PROFILE')
    a,f=af>>8,af&255
    mask=0 if profile=='nec_a' or (profile in ('zilog_q','st_cmos') and mode==0) else (0x20 if profile=='st_cmos' else 0x28)
    return flag_result(op,a,f,a|(f&mask))

def source():
    build.require(len(ROWS)==len(xadr.ROWS) and RAW_SIZE==xadr.RAW_SIZE,'M140_PREF_PARENT_LAYOUT')
    text=xadr.derive((build.HERE/'capture.asm').read_bytes(),(build.HERE/'pref.asm').read_text())
    def table(magic,rows):
        return 'db '+','.join(map(str,magic+bytes(12)+b''.join(struct.pack('<BBB9HB',r,m,ord(op),af,*([0]*8),0) for r,m,op,af in rows)+b'END!'))
    text=build.replace_once(text,table(xadr.MAGIC,xadr.ROWS),table(MAGIC,ROWS))
    for old,new in (("'M140 FDC XADR 1'","'M140 FDC PREF 1'"),("'M140_XADR_RAW'","'M140_PREF_RAW'"),("    ld a,'X'","    ld a,'P'"),('TEMPLATES_DATA:','db '+','.join(map(str,b''.join(sampler(m) for m in range(4)))))):
        text=build.replace_once(text,old,new)
    return text

def loader_source(payload):
    return build.replace_once(build.replace_once(xadr.loader_source(payload),"'FDCXADR DAT'","'FDCPREF DAT'"),'M140_XADR_PATCH_VERIFY','M140_PREF_PATCH_VERIFY')

def assemble(assembler,output):
    asm=source();payload,sym=build.zex.assemble_with_symbols(asm,assembler,'fdcpref')
    build.require(0<len(payload)<0x3000 and sym['initial_table']+RAW_SIZE<=0x7000 and sym['sampler_low']==0x4800 and sym['caller_low']==0x48fe,'M140_PREF_LAYOUT')
    at=sym['sampler_template']-0x4000
    build.require(payload[at:at+72]==b''.join(sampler(m) for m in range(4)),'M140_PREF_SAMPLER_TEMPLATE')
    at=sym['caller_template']-0x4000
    build.require(payload[at:at+12]==b'\x11\0\0\x21\0\0\xcd\0\x48\xc3'+struct.pack('<H',sym['capture_return']),'M140_PREF_CALLER_TEMPLATE')
    loader=loader_source(payload);program,main=build.zex.assemble_with_symbols(loader,assembler,'fdcpref-loader')
    build.require(main['workspace_end']<0xd000 and len(program)==main['image_end']-0x100,'M140_PREF_WORKSPACE')
    at=main['payload']-0x100;build.require(program[at:at+len(payload)]==payload,'M139_EMBEDDED_PAYLOAD_MISMATCH')
    output.mkdir(parents=True,exist_ok=True)
    for name,data in {'fdcpref.asm':asm.encode(),'fdcpref-loader.asm':loader.encode(),'fdcpref.bin':payload,'FDCPREF.COM':program,'fdcpref-symbols.json':(json.dumps(sym,indent=2)+'\n').encode(),'fdcpref-loader-symbols.json':(json.dumps(main,indent=2)+'\n').encode()}.items():(output/name).write_bytes(data)
    return {'FDCPREF.COM':program}

def content(data,*,recovery=True,host_model=False):
    try:r=capture.row_content(data,rows=ROWS,magic=MAGIC,marker=MARKER,banner='M140 FDC PREF 1',tag='P',descriptor='mode',recovery=recovery,host_model=host_model)
    except results.RecordError as exc:raise results.RecordError(exc.code.replace('M140_CAPTURE_','M140_PREF_')) from None
    raw=data.split(MARKER)[1];counts={p:0 for p in PROFILES};outside=[]
    for i,(repeat,mode,op,af) in enumerate(ROWS):
        after=struct.unpack_from('<H',raw,20+22*i+11)[0]
        predictions={p:prediction(op,af,mode,p) for p in PROFILES}
        for p,value in predictions.items():counts[p]+=after==value
        if after not in predictions.values():outside.append({'row':i,'after':f'{after:04X}'})
    r.update(candidate_matches=counts,outside_candidate_union=outside,modes=MODES,
             interpretation='Candidate comparisons only. CP-before-IUT versus NOP/DD/FD boundary, fixed opcode480A/call4904/return4907; no physical Q readout. Pre-control CP AF differences are diagnostic findings, not silently accepted silicon conformance. Shared CPU/export remains.')
    return r

def main():
    p=argparse.ArgumentParser(description=__doc__);sub=p.add_subparsers(dest='action',required=True)
    b=sub.add_parser('build');b.add_argument('--source-disk',type=Path,required=True);b.add_argument('--output-dir',type=Path,required=True);b.add_argument('--assembler',required=True)
    c=sub.add_parser('check');c.add_argument('input',type=Path);c.add_argument('--output',type=Path,required=True);c.add_argument('--host-model',action='store_true');a=p.parse_args()
    if a.action=='check':
        a.output.write_text(json.dumps(content(a.input.read_bytes(),host_model=a.host_model),indent=2)+'\n');print('M140_PREF_OBSERVATIONS_COMPLETE (inspect findings; not silicon acceptance)');return
    build.require(not a.output_dir.exists(),'M139_OUTPUT_EXISTS');assembler=str(Path(a.assembler).resolve());build.require(build.cpm.assembler_version_ok(assembler,'1.8'),'M139_ASSEMBLER_VERSION')
    original=a.source_disk.read_bytes();build.require('FDCPREF.DAT' not in results.disk_files(original),'M140_PREF_LOG_EXISTS')
    images=assemble(assembler,a.output_dir);images['PREFHELP.TXT']=(build.HERE/'pref-help.txt').read_bytes().replace(b'\n',b'\r\n')+b'\x1a'
    image=build.add_files(original,images);space=build.log_headroom(image,{'FDCPREF.DAT':12288});build.require(a.source_disk.read_bytes()==original,'M139_SOURCE_DISK_CHANGED')
    (a.output_dir/'fdc-pref-r1.d88').write_bytes(image)
    sym=json.loads((a.output_dir/'fdcpref-symbols.json').read_text());main_sym=json.loads((a.output_dir/'fdcpref-loader-symbols.json').read_text())
    report={'schema':'vaeg-m140-fdc-pref-build-v1','status':'BUILT_PHYSICAL_NOT_RUN','input_sha256':build.digest(original),'output_sha256':build.digest(image),'files_sha256':{n:build.digest(d) for n,d in images.items()},'files_size':{n:len(d) for n,d in images.items()},'log_headroom':space,'assembler_sha256':build.digest(Path(assembler).read_bytes()),'patch_check_file_offset':main_sym['payload']-0x100+sym['patch_check']-0x4000,'source_sha256':{f:build.digest((build.HERE/f).read_bytes()) for f in ('pref.py','pref.asm','pref-help.txt','xadr.py','capture.asm','capture.py','fixed.py','seed.py','loader.asm','build.py','reduction.py')},'command':sys.argv}
    (a.output_dir/'build-report.json').write_text(json.dumps(report,indent=2)+'\n');print('M140_PREF_BUILD_OK (physical NOT_RUN)')

if __name__=='__main__':
    try:main()
    except (build.BuildError,results.RecordError) as exc:raise SystemExit(exc.code)
