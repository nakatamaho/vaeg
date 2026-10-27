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
"""Content-only FDC record validation. No Git history or private paths.

A complete physical record is an observation, NOT proof of Zilog conformance.
MODEL_T is synthetic host accounting; E is both-edge Port-40 VRTC counting.
Neither is automatically a physical CPU frequency measurement.
"""
from __future__ import annotations
import argparse
import json
import math
from pathlib import Path
import re
import struct
import sys

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import build as contract
VALUES = (0,1,2,7,8,15,16,32,40,64,85,127,128,170,215,255)

class RecordError(Exception):
    def __init__(self, code: str):
        self.code = code
        super().__init__(code)

def need(ok: bool, code: str) -> None:
    if not ok:
        raise RecordError(code)

def lines(data: bytes, recovery: bool) -> list[str]:
    try:
        text = data.rstrip(b'\x1a').decode('ascii')
    except UnicodeError:
        raise RecordError('M139_OUTPUT_ENCODING')
    rows = text.splitlines()
    if recovery:
        need(bool(rows) and rows[-1] == 'M139_FDC_RAM_RESTORED', 'M139_RECOVERY_RECORD')
    need(not any('M139_FDC_TIMEOUT' in x or 'FAILED' in x or 'M139_TIMER_WRAP' in x for x in rows), 'M139_GUEST_FAILURE')
    return rows

def zilog_af(a: int, f: int) -> list[int]:
    """Independent arithmetic expectation for the current Zilog host profile.

    POP AF precedes each instruction, but hidden Q/instruction history is not
    explicitly controlled by this diagnostic or modeled by this arithmetic.
    This is a host-model contract, not an independent Zilog silicon oracle or
    an assertion about NEC parts; SCF/CCF differences need history controls.
    """
    bit = (a<<8) | (f&1) | 16 | (a&40) | (0 if a&1 else 0x44)
    value = a&0x55
    flags = 16 | (0x40 if value==0 else 0) | (4 if bin(value).count('1')%2==0 else 0) | (value&40)
    and_result = (value<<8) | flags
    common = (a<<8) | (f&0xc4) | (a&40)
    return [(a<<8)|f, bit, and_result, common|1, common|(16 if f&1 else 1)]

def probe(data: bytes, *, unit: str, recovery: bool = True, host_model: bool = False) -> dict:
    need(unit in ('E','MODEL_T'), 'M139_TIMER_UNIT')
    rows = lines(data,recovery)
    need('M139 FDC PROBE 1' in rows and 'M139_FDC_PROBE_COMPLETE' in rows, 'M139_PROBE_COMPLETION')
    af, timing = {}, {}
    for row in rows:
        if row.startswith('F '):
            need(re.fullmatch(r'F [0-9A-F]{24}', row) is not None, 'M139_AF_FORMAT')
            words = [int(row[i:i+4],16) for i in range(2,26,4)]
            key = words[0]
            need(key not in af, 'M139_AF_DUPLICATE')
            a,f = key>>8,key&255
            need(all(x>>8==a for x in (words[1],words[2],words[4],words[5])) and words[3]>>8==(a&0x55), 'M139_AF_VALUE')
            if host_model:
                need(words[1:] == zilog_af(a,f), 'M139_HOST_AF_MISMATCH')
            af[key] = words[1:]
        elif row.startswith('T '):
            m = re.fullmatch(r'T (\w+) r=(\d+) n=(\d+) (E|MODEL_T)=([0-9A-Fa-f]+)',row)
            need(m is not None, 'M139_TIME_FORMAT')
            name,r,n,kind,value = m.groups()
            need(kind==unit, 'M139_TIMER_UNIT')
            key=(name,int(r),int(n))
            need(key not in timing, 'M139_TIME_DUPLICATE')
            need(re.fullmatch(r'[0-9A-F]{4}' if unit=='E' else r'[0-9]+',value) is not None, 'M139_TIME_FORMAT')
            timing[key]=int(value,16 if unit=='E' else 10)
    need(set(af)=={(a<<8)|f for a in VALUES for f in VALUES}, 'M139_AF_SET')
    expected = {('empty',0,0)} | {(k,r,n) for k in contract.BODIES for r in (1,2) for n in contract.ROUNDS}
    need(set(timing)==expected, 'M139_TIME_SET')
    slopes={}
    for name,(_,body_t) in contract.BODIES.items():
        fits=[]
        for repeat in (1,2):
            y=[timing[(name,repeat,n)] for n in contract.ROUNDS]
            need(all(x>0 for x in y), 'M139_TIMER_INACTIVE')
            # VRTC's two edges are NOT uniformly spaced. A window's edge
            # count has +/-2-edge phase error relative to 2*f*t. At N=4,
            # interpolation between N=1/16 has weights .8/.2: its worst
            # residual is 2 + .8*2 + .2*2 = 4 edges. Repeat slope differences
            # are bounded by 8/15 edges per N. Do not assume 50% duty cycle.
            slope=(y[2]-y[0])/15
            need(slope>0, 'M139_TIMER_INACTIVE')
            if unit=='E':
                need(abs(y[1]-(y[0]+3*slope))<=4, 'M139_TIME_NONLINEAR')
            if host_model:
                for n,v in zip(contract.ROUNDS,y):
                    need(v==269+n*(60000*(body_t+26)+29), 'M139_HOST_TIME_MISMATCH')
            fits.append(slope)
        if unit=='E':
            need(abs(fits[0]-fits[1])<=8/15+1e-9, 'M139_TIME_REPEATABILITY')
        slopes[name]=sum(fits)/2
    if host_model:
        need(timing[('empty',0,0)]==208, 'M139_HOST_TIME_MISMATCH')
    differences=[{'requested_af':f'{k:04X}','actual':v,'zilog':zilog_af(k>>8,k&255)} for k,v in af.items() if v!=zilog_af(k>>8,k&255)]
    return {'status':'COMPLETE_OBSERVATIONS','af_records':len(af),'timing_records':len(timing),
            'unit':unit,'ticks_per_60000_iterations':slopes,'zilog_difference_records':differences,
            'physical_cpu_frequency':'NOT_ESTABLISHED','recovery_record':recovery}

def history_before(af: int, h: int) -> int:
    """Arithmetic precondition control; undocumented bits are not an oracle."""
    a,f=af>>8,af&255
    logical=(a&0xa8)|(0x40 if a==0 else 0)|(4 if bin(a).count('1')%2==0 else 0)
    if h<=2:return af
    flags={3:logical,4:logical|16,5:0x42|(a&40),
           6:(f&0xc4)|(a&40)|1,
           7:(f&0xc4)|(a&40)|(16 if f&1 else 1)}[h]
    return (a<<8)|flags


def history(data: bytes, *, profile: str, recovery: bool = True, host_model: bool = False) -> dict:
    need(profile in contract.HISTORY_PROFILES,'M140_HISTORY_PROFILE')
    rows=lines(data,recovery)
    need('M140 FDC HISTORY 1 '+profile in rows and 'M140_FDC_HISTORY_COMPLETE' in rows,
         'M140_HISTORY_COMPLETION')
    records={}
    av,fv=contract.HISTORY_PROFILES[profile]
    for row in rows:
        if not row.startswith('Q '):continue
        m=re.fullmatch(r'Q ([0-7]) ([SC]) ([0-9A-F]{4}) ([0-9A-F]{4}) ([0-9A-F]{4})',row)
        need(m is not None,'M140_HISTORY_FORMAT')
        h,op,requested,before,after=m.groups()
        h,requested,before,after=int(h),int(requested,16),int(before,16),int(after,16)
        key=(requested,h,op)
        need(key not in records,'M140_HISTORY_DUPLICATE')
        mask=0xffff if h<=2 else 0xffd7
        need((before&mask)==(history_before(requested,h)&mask),'M140_HISTORY_PRECONDITION')
        need(after>>8==requested>>8,'M140_HISTORY_ACCUMULATOR')
        records[key]=(before,after)
    expected={(a<<8|f,h,op) for a in av for f in fv for h in range(8) for op in ('S','C')}
    need(set(records)==expected,'M140_HISTORY_SET')
    differences=[]
    for (requested,h,op),(before,after) in sorted(records.items()):
        need(records[(requested,h,'S')][0]==records[(requested,h,'C')][0],
             'M140_HISTORY_PRECONDITION_REPEAT')
        baseline=zilog_af(before>>8,before&255)[3 if op=='S' else 4]
        if after!=baseline:
            differences.append({'requested_af':f'{requested:04X}','history':h,'operation':op,
                                'before_af':f'{before:04X}','after_af':f'{after:04X}',
                                'host_baseline':f'{baseline:04X}','xor':f'{after^baseline:04X}'})
    if host_model:need(not differences,'M140_HISTORY_HOST_MISMATCH')
    return {'status':'COMPLETE_OBSERVATIONS','profile':profile,'records':len(records),
            'predecessors':list(contract.HISTORY_PREDECESSORS),
            'host_baseline_difference_records':differences,
            'reference_class':'controlled instruction adjacency, not independently verified Q/silicon conformance',
            'recovery_record':recovery}


def zex(data: bytes, *, smoke: bool = False, recovery: bool = True) -> dict:
    rows=lines(data,recovery)
    banner='M139 FDC NEG SMOKE ONLY' if smoke else 'M139 FDC relocated ZEXALL'
    need(banner in rows and 'Tests complete' in rows, 'M139_ZEX_COMPLETION')
    expected=re.findall(r"^\s*tmsg\s+'([^']+)'",contract.zex.read_zex_source('zexall'),re.M)
    if smoke:
        expected=[x for x in expected if x.startswith('neg.')]
    else:
        need(len(expected)==67, 'M139_ZEX_SOURCE_GROUPS')
    records=[]
    for row in rows:
        if ' STOCK=' in row:
            m=re.fullmatch(r'(.*?)  STOCK=([a-f0-9]{8}) CRC=([a-f0-9]{8})',row)
            need(m is not None,'M139_ZEX_FORMAT')
            records.append(m.groups())
    need([x[0] for x in records]==expected,'M139_ZEX_GROUP_SET')
    return {'status':'COMPLETE_OBSERVATIONS','groups':len(records),'smoke_only':smoke,
            'records':[{'group':i+1,'name':n,'stock_crc':s,'actual_crc':a} for i,(n,s,a) in enumerate(records)],
            'reference_class':'relocated target observations, not a stock-address conformance verdict'}

def normalize_capture_image(image: bytes) -> bytes:
    """Accept a closed 2D double-step table layout without changing sectors.

    The writable-media builder remains strict. Returned 40-cylinder captures
    may index C/H at 4*C+H instead of 2*C+H; sector C/H remain logical.
    Preserve the received image and normalize only an in-memory header copy.
    """
    try:
        contract.unpack(image)
        return image
    except contract.BuildError as exc:
        if exc.code != 'M139_D88_TRACKS':
            raise
    tracks=struct.unpack_from('<164I',image,32)
    indices=[4*c+h for c in range(40) for h in range(2)]
    need(image[27]==0 and [i for i,x in enumerate(tracks) if x]==indices,
         'M140_D88_CAPTURE_TRACKS')
    normalized=bytearray(image)
    struct.pack_into('<164I',normalized,32,*([tracks[i] for i in indices]+[0]*84))
    normalized=bytes(normalized)
    _,locations=contract.unpack(normalized)  # exact bounds, order and C/H/R/N
    need(all(normalized[pos-8]==0 for pos,size in locations), 'M140_D88_CAPTURE_STATUS')
    return normalized


def disk_files(image: bytes) -> dict[str,bytes]:
    """Read supported user-zero CP/M, including validated double-step captures."""
    raw,_=contract.unpack(normalize_capture_image(image))
    records={}
    for i in range(128):
        e=raw[0x4000+32*i:0x4020+32*i]
        if e[0] in (0xe5,0xff):continue
        need(e[0]==0 and e[13]==0 and e[15]<=128,'M139_CPM_LAYOUT')
        name=bytes(x&127 for x in e[1:9]).decode('ascii').rstrip()+'.'+bytes(x&127 for x in e[9:12]).decode('ascii').rstrip()
        count=(e[12]&1)*128+e[15]
        blocks=e[16:16+(count*128+2047)//2048]
        need(all(2<=b<152 for b in blocks),'M139_CPM_LAYOUT')
        data=b''.join(raw[0x4000+b*2048:0x4000+(b+1)*2048] for b in blocks)[:count*128]
        records.setdefault(name,[]).append(((e[12]+(e[14]<<5))//2,data))
    result={}
    for n,entries in records.items():
        entries.sort()
        need([i for i,d in entries]==list(range(len(entries))),'M139_CPM_EXTENT_ORDER')
        result[n]=b''.join(d for i,d in entries)
    return result

def cpm_screen(data: bytes) -> list[str]:
    # fdc-cpm-01 CBIOS layout: DMA F300h, 120-byte rows, 80 characters.
    # N88-BASIC's F3C8h start is a DIFFERENT contract. Never search arbitrary
    # TVRAM for a plausible prompt or infer PASS from a screenshot alone.
    need(len(data)>=44 and data[:8]==b'VAEGSCN1', 'M139_SCREEN_DUMP')
    version,length=struct.unpack_from('<II',data,8)
    need(version==1 and length<=4096 and len(data)>=44+length, 'M139_SCREEN_DUMP')
    offset=40+length
    size=struct.unpack_from('<I',data,offset)[0]
    memory=data[offset+4:]
    need(len(memory)==size and size>=0x6300+25*120, 'M139_SCREEN_DUMP')
    return [''.join(chr(x) if 32<=x<127 else ' ' for x in memory[0x6300+i*120:0x6300+i*120+80]).rstrip() for i in range(25)]


def terminal(rows: list[str], name: str, *, existing: bool = False) -> None:
    commands=[i for i,row in enumerate(rows) if row.startswith('A>DIR')]
    need(bool(commands), 'M139_INTEGRATION_DIR_COMMAND')
    tail=rows[commands[-1]+1:]
    prompts=[i for i,row in enumerate(tail) if row.startswith('A>')]
    need(bool(prompts), 'M139_INTEGRATION_PROMPT')
    directory='\n'.join(tail[:prompts[0]])
    need(re.search(r'\b'+re.escape(name)+r'\s+TXT\b',directory) is not None, 'M139_INTEGRATION_DIR_RESULT')
    if existing:
        need(any('M139_LOG_EXISTS' in row for row in rows[:commands[-1]]), 'M139_INTEGRATION_REFUSAL_MESSAGE')


def calibrate(report: dict, vrtc_hz: float, reference_id: str) -> None:
    need(report.get('unit')=='E' and math.isfinite(vrtc_hz) and vrtc_hz>0 and bool(reference_id), 'M139_VRTC_REFERENCE')
    slopes=report['ticks_per_60000_iterations']
    # Conservative +/-2-edge uncertainty at each endpoint, differencing two
    # fitted kernels, N range 1..16. Reference-frequency error is additional.
    error_ticks=8/15
    values={}
    for name,slope in slopes.items():
        if name=='branch':continue
        multiplicity=len(contract.BODIES[name][0].splitlines())
        scale=2*vrtc_hz*60000
        delta=slope-slopes['branch']
        values[name]={'seconds_per_body_sequence':delta/scale,
                      'instructions_per_sequence':multiplicity,
                      'seconds_per_instruction_average':delta/(scale*multiplicity),
                      'quantization_error_bound_seconds_per_sequence':error_ticks/scale}
    report['user_bound_vrtc_reference']={'id':reference_id,'hz':vrtc_hz,'frequency_uncertainty':'not included; must be established independently'}
    report['baseline_subtracted_measurements']=values


def main() -> None:
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('input',type=Path)
    p.add_argument('--kind',choices=('probe','zex','smoke','history'),required=True)
    p.add_argument('--profile',choices=tuple(contract.HISTORY_PROFILES),default='FDCHIST')
    p.add_argument('--unit',choices=('E','MODEL_T'),default='E')
    p.add_argument('--host-model',action='store_true',help='require synthetic Zilog AF and exact model-cycle contracts; not for hardware observations')
    p.add_argument('--output',type=Path,required=True)
    p.add_argument('--vrtc-hz',type=float,help='independently approved physical reference; never guessed from nominal CPU MHz')
    p.add_argument('--reference-id',help='identity of the approved timer/reference contract')
    a=p.parse_args()
    data=a.input.read_bytes()
    if a.kind=='history':
        result=history(data,profile=a.profile,recovery=not a.host_model,host_model=a.host_model)
    else:
        result=probe(data,unit=a.unit,recovery=not a.host_model,host_model=a.host_model) if a.kind=='probe' else zex(data,smoke=a.kind=='smoke',recovery=not a.host_model)
    if a.vrtc_hz is not None:
        need(not a.host_model, 'M139_VRTC_REFERENCE')
        calibrate(result,a.vrtc_hz,a.reference_id)
    a.output.write_text(json.dumps(result,indent=2)+'\n')
    print('M139_FDC_RECORDS_COMPLETE (not physical conformance acceptance)')

if __name__=='__main__':
    try:main()
    except RecordError as exc:raise SystemExit(exc.code)
