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
"""Static spans/nominal stack and literal address-bit counterexamples, not cause."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys
import capture
import fixed
import results
import seed


def layout(kind, payload, symbols):
    """Validate binary spans only; nominal stack assumes balanced, no-IRQ flow."""
    def address(name):
        results.need(name in symbols and type(symbols[name]) is int, 'M140_CONTEXT_SYMBOL')
        return symbols[name]

    def span(a, expected, code):
        results.need(0x4000 <= a and a+len(expected) <= 0x4000+len(payload), 'M140_CONTEXT_RANGE')
        results.need(payload[a-0x4000:a-0x4000+len(expected)] == expected, code)

    span(0x4000,b'\xf3\x31\xff\x7e','M140_CONTEXT_ENTRY')
    if kind == 'seed':
        rows=[]
        for i,(s,op,af) in enumerate(seed.ROWS):
            alu=b'\xaf' if s==0 else bytes((0x3e,(0,0,8,32,40)[s],0xb7))
            prefix=b'\xf3'+alu+b'\xc5\xf1'
            positions={}
            for suffix,field in (('pre',4),('post',6)):
                a=address(f'case_{i}_{suffix}')
                span(a-3,b'\x01'+struct.pack('<H',af),'M140_CONTEXT_TARGET')
                body=prefix+(bytes((0x37 if op=='S' else 0x3f,)) if suffix=='post' else b'')
                body+=b'\xf5\xd1\xed\x53'+struct.pack('<H',0x7008+8*i+field)
                span(a,body,'M140_CONTEXT_SEED_SPAN')
                positions[suffix]=a
            iut=positions['post']+len(prefix)
            rows.append({'seed':s,'operation':op,'target':af,'iut':iut,'pushes':[iut+1],
                         'return_address':None,'nominal_stack_words':[0x7efd],
                         'control_has_nop':False})
        return {'kind':kind,'contexts':rows,'scope':'Unrolled spans; no sampler CALL. Seed 0 is XOR-only; no NOP control substitution.'}
    results.need(kind in ('fixed','capture'),'M140_CONTEXT_KIND')
    a=address('sampler');iut=address('sampler_iut')
    results.need(iut==a+6,'M140_CONTEXT_IUT_OFFSET')
    span(a,b'\xf3\x3e\x00\xaf\xc5\xf1\x00','M140_CONTEXT_PREFIX')
    tail=b'\xf5\xd1\xc9' if kind=='fixed' else b'\xf5\xf5\xf5\xe1\xc1\xd1\xc9'
    span(iut+1,tail,'M140_CONTEXT_TAIL')
    calls={}
    for name in ('control_call','measured_call'):
        call=address(name);span(call,b'\xcd'+struct.pack('<H',a),'M140_CONTEXT_CALL')
        calls[name]={'call':call,'return':call+3}
        if kind=='capture':
            span(call-4,b'\xed\x4b'+struct.pack('<H',address('target')),'M140_CONTEXT_RELOAD')
    if kind=='capture':
        saved=(b'\xed\x53'+struct.pack('<H',address('cap_a'))+
               b'\xed\x43'+struct.pack('<H',address('cap_b'))+
               b'\x22'+struct.pack('<H',address('cap_c'))+b'\xc9')
        span(address('save_captures'),saved,'M140_CONTEXT_MAILBOX')
        for name in calls:
            span(calls[name]['return'],b'\xcd'+struct.pack('<H',address('save_captures')),
                 'M140_CONTEXT_SAVE_CALL')
    n=1 if kind=='fixed' else 3
    return {'kind':kind,'sampler':a,'iut':iut,'calls':calls,'pushes':list(range(iut+1,iut+1+n)),
            'nominal_stack_words':[0x7efb-2*j for j in range(n)],'control_has_nop':True,
            'scope':'Static shared sampler; patched ALU/IUT verified separately by original builder/runtime. Nominal stack, not physical SP/fetch trace.'}


def observations(kind,data):
    """Smallest content layer: existing closed validators precede decoding."""
    module={'seed':seed,'fixed':fixed,'capture':capture}[kind]
    report=module.content(data)
    if kind=='capture':results.need(report['integrity_status']=='NO_FINDINGS_IN_TESTED_CONTRACT','M140_CONTEXT_INTEGRITY')
    raw=data.split(module.MARKER)[1];rows=[]
    for i in range(len(module.ROWS)):
        if kind=='seed':
            s,o,af,before,after=struct.unpack_from('<BBHHH',raw,8+8*i);r=0;values=[after]
        elif kind=='fixed':
            r,s,o,af,before,after=struct.unpack_from('<BBBHHH',raw,8+9*i);values=[after]
        else:
            r,s,o,*words,status=struct.unpack_from('<BBB9HB',raw,20+22*i)
            af=words[0];values=words[4:7]
        rows.append({'row':i,'repeat':r,'seed':s,'operation':chr(o),'target':af,'post':values})
    return rows


def evaluate(context,rows):
    """Closed literal hypotheses; retain witnesses, never fit an address law."""
    counterexamples={k:[] for k in ('literal_iut_bit3','literal_return_bit3','literal_push_bit3')}
    samples={k:0 for k in counterexamples}
    groups={}
    for row in rows:
        c=context['contexts'][row['row']] if context['kind']=='seed' else context
        iut=c['iut'];ret=c.get('return_address') if context['kind']=='seed' else c['calls']['measured_call']['return']
        for key,a in (('literal_iut_bit3',iut),('literal_return_bit3',ret)):
            if a is not None:
                samples[key]+=1
                if row['post'][0]&8 != a&8:
                    counterexamples[key].append({'row':row['row'],'address':a,'after':row['post'][0]})
        results.need(len(row['post'])==len(c['pushes']),'M140_CONTEXT_SNAPSHOT_COUNT')
        for j,(a,post) in enumerate(zip(c['pushes'],row['post'])):
            samples['literal_push_bit3']+=1
            if post&8 != a&8:counterexamples['literal_push_bit3'].append({'row':row['row'],'snapshot':j,'address':a,'after':post})
        key=(row['target'],row['seed'],row['operation'])
        groups.setdefault(key,[]).append({'repeat':row['repeat'],'after':row['post'][0]})
    varying=[{'target':af,'seed':s,'operation':op,'values':values} for (af,s,op),values in groups.items()
             if len({v['after'] for v in values})>1]
    statuses={k:('NOT_APPLICABLE' if not samples[k] else
                 'FALSIFIED' if counterexamples[k] else 'NOT_FALSIFIED_IN_DATA') for k in samples}
    return {'records':len(rows),'hypothesis_samples':samples,'hypothesis_status':statuses,
            'counterexamples':counterexamples,'repeat_varying_groups':varying,
            'conclusion':'Literal bit-copy counterexamples only. Repeat variation at fixed addresses rules out target-AF/seed/op/static-address-only determinism; hidden context/phase/common export is not excluded. No causal or NEC rule inferred.'}


def program_binding(payload,program):
    results.need(bool(payload) and program.count(payload)==1,'M140_CONTEXT_PROGRAM_BINDING')


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--kind',choices=('seed','fixed','capture'),required=True)
    for name in ('payload','symbols','program','output'):p.add_argument('--'+name,type=Path,required=True)
    p.add_argument('--log',type=Path)
    a=p.parse_args()
    root=Path(__file__).resolve().parents[3];target=a.output.resolve()
    results.need(target!=root and root not in target.parents,'M140_CONTEXT_OUTPUT_LOCATION')
    results.need(not target.exists(),'M140_CONTEXT_OUTPUT_EXISTS')
    payload=a.payload.read_bytes();program=a.program.read_bytes()
    program_binding(payload,program)
    context=layout(a.kind,payload,json.loads(a.symbols.read_text()))
    report={'status':'STATIC_CONTEXT_AUDIT_NOT_CAUSE','layout':context,
            'command':sys.argv,
            'contract_sha256':{str(f):hashlib.sha256(f.read_bytes()).hexdigest() for f in
                               (Path(__file__),Path(seed.__file__),Path(fixed.__file__),
                                Path(capture.__file__),Path(results.__file__),Path(seed.__file__).with_name('reduction.py'))},
            'inputs_sha256':{str(f):hashlib.sha256(f.read_bytes()).hexdigest()
                                               for f in (a.payload,a.symbols,a.program)}}
    if a.log:
        report['inputs_sha256'][str(a.log)]=hashlib.sha256(a.log.read_bytes()).hexdigest()
        report['observations']=evaluate(context,observations(a.kind,a.log.read_bytes()))
    target.parent.mkdir(parents=True,exist_ok=True)
    target.write_text(json.dumps(report,indent=2)+'\n')
    print('M140_CONTEXT_AUDIT_COMPLETE (static spans/observations, not physical trace or cause)')


if __name__=='__main__':
    try:main()
    except results.RecordError as exc:raise SystemExit(exc.code)
