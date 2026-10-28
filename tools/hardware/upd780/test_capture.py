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
"""Capture controls: pure diagnostic content, mapped execution and ROM fault QA."""
import argparse
import json
from pathlib import Path
import struct
import subprocess
import tempfile
import unittest
import capture
import results
import seed
import test_fixed


def fixture(kind=None):
    text=bytearray(b'M140 FDC CAPTURE 1\r\n')
    raw=bytearray(capture.MAGIC+struct.pack('<HHBBHHH',1 if kind=='ram' else 0,
                  0x74d7 if kind=='ram' else 0,0x7f,0,1 if kind=='mirror' else 0,
                  0x701f if kind=='mirror' else 0,0))
    for i,(r,s,op,af) in enumerate(capture.ROWS):
        after=1 if op=='S' or af==0x28 else 0x10
        if i==0 and kind=='cpu':after^=8
        words=[af,af,af,af,after,after,after,after,after^0xffff]
        status=0
        if i==0 and kind=='triple':words[5]^=8;status=4
        if i==0 and kind in ('mirror','late'):words[7]^=8
        encoded=' '.join(f'{x:04X}' for x in words)
        text+=f'D {r:02X} {s:02X} {op} {encoded} {status:02X}\r\n'.encode()
        raw+=struct.pack('<BBB9HB',r,s,ord(op),*words,status)
    return bytes(text)+capture.MARKER+raw+b'END!'+seed.RESTORED+b'\x1a'


class CaptureContent(unittest.TestCase):
    def test_complete(self):
        r=capture.content(fixture(),host_model=True)
        self.assertEqual(r['records'],96)
        self.assertEqual(r['integrity_status'],'NO_FINDINGS_IN_TESTED_CONTRACT')
        self.assertEqual(r['record_findings'],[])
        self.assertEqual(r['header_findings'],[])

    def mutation(self,offset,code,mask=1):
        good=fixture()
        capture.content(good,host_model=True)
        changed=bytearray(good)
        start=good.index(capture.MARKER)+len(capture.MARKER)
        changed[start+offset]^=mask
        with self.assertRaises(results.RecordError) as caught:capture.content(bytes(changed))
        self.assertEqual(caught.exception.code,code)

    def test_frame(self):self.mutation(0,'M140_CAPTURE_BINARY_FRAME')
    def test_header(self):self.mutation(13,'M140_CAPTURE_HEADER')
    def test_pattern_completion(self):self.mutation(12,'M140_CAPTURE_PATTERN_SET')
    def test_descriptor(self):self.mutation(20,'M140_CAPTURE_DESCRIPTOR')
    def test_status(self):self.mutation(41,'M140_CAPTURE_STATUS',mask=8)
    def test_transport(self):self.mutation(20+11,'M140_CAPTURE_PATH_MISMATCH',mask=8)

    def test_recovery(self):
        good=fixture();capture.content(good)
        with self.assertRaises(results.RecordError) as caught:capture.content(good.replace(seed.RESTORED,b'UNKNOWN'))
        self.assertEqual(caught.exception.code,'M139_RECOVERY_RECORD')

    def test_padding(self):
        good=fixture();capture.content(good)
        with self.assertRaises(results.RecordError) as caught:capture.content(good[:-1]+b'!')
        self.assertEqual(caught.exception.code,'M140_CAPTURE_PADDING')

    def diagnostic(self,kind,record_kinds,header_kinds):
        capture.content(fixture(),host_model=True)
        changed=fixture(kind)
        r=capture.content(changed)
        self.assertEqual([x['kinds'] for x in r['record_findings']],record_kinds)
        self.assertEqual(r['header_findings'],header_kinds)
        self.assertEqual(r['integrity_status'],'FINDINGS')
        with self.assertRaises(results.RecordError) as caught:capture.content(changed,host_model=True)
        self.assertEqual(caught.exception.code,'M140_CAPTURE_HOST_INTEGRITY')

    def test_triple_diagnostic(self):
        self.diagnostic('triple',[['post_triple','reported_post_triple']],[])

    def test_mirror_diagnostic(self):
        self.diagnostic('mirror',[['mirror_relation']],['reported_delayed_failures'])

    def test_late_change_diagnostic(self):
        self.diagnostic('late',[['mirror_relation']],['delayed_summary_contradiction'])

    def test_ram_diagnostic(self):
        self.diagnostic('ram',[],['pre_ram_failures'])

    def test_coherent_cpu_observation_not_integrity_failure(self):
        capture.content(fixture(),host_model=True)
        changed=fixture('cpu')
        r=capture.content(changed)
        self.assertEqual(r['integrity_status'],'NO_FINDINGS_IN_TESTED_CONTRACT')
        self.assertEqual(len(r['primary_host_differences']),1)
        with self.assertRaises(results.RecordError) as caught:capture.content(changed,host_model=True)
        self.assertEqual(caught.exception.code,'M140_CAPTURE_HOST_MISMATCH')

    def test_source_sampler(self):
        source=capture.source()
        span=source.split('sampler:\n',1)[1].split('initial_table:',1)[0]
        self.assertEqual(span,'    di\nsampler_seed:\n    ld a,0\nsampler_alu:\n    xor a\n'
                         '    push bc\n    pop af\nsampler_iut:\n    nop\n    push af\n'
                         '    push af\n    push af\n    pop hl\n    pop bc\n    pop de\n    ret\n')
        self.assertIn('patterns: db 0,0ffh,55h,0aah,29h,21h',source)
        self.assertLess(source.index('    call ram_test'),source.index('capture_loop:'))
        self.assertLess(source.index('    call delayed_scan'),source.index('print_loop:'))
        self.assertEqual(source.count('    call sampler\n'),2)

    def test_loader(self):
        text=capture.loader_source(b'\xc9')
        self.assertIn("    db 'FDCCAP  DAT'",text)
        self.assertIn('    ld hl,7000h\n    ld de,2136\n',text)
        self.assertIn('M140_CAPTURE_PATCH_VERIFY',text)
        self.assertLess(text.index('seed_read_loop:'),text.index('restore_loop:'))
        self.assertLess(text.index('verify_loop:'),text.index('    ld c,22\n'))


def payloads(runner,assembler):
    with tempfile.TemporaryDirectory(prefix='m140-capture-') as td:
        work=Path(td)
        capture.assemble(assembler,work)
        sym=json.loads((work/'fdccap-symbols.json').read_text())
        binary=(work/'fdccap.bin').read_bytes()
        pos=sym['sampler']-0x4000
        assert sym['sampler']==0x4800 and sym['sampler_iut']==0x4806
        expected=b'\xf3\x3e\x00\xaf\xc5\xf1\x00\xf5\xf5\xf5\xe1\xc1\xd1\xc9'
        assert binary[pos:pos+len(expected)]==expected
        for label in ('control_call','measured_call'):
            p=sym[label]-0x4000
            assert binary[p:p+3]==b'\xcd\x00\x48'
        dump=work/'ram.bin'
        run=subprocess.run([runner,str(work/'fdccap.bin'),str(dump)],capture_output=True,timeout=30)
        assert run.returncode==0,run.stderr
        ram=dump.read_bytes();assert len(ram)==0x3f00
        report=capture.content(run.stdout+ram[0x3000:0x3000+capture.RAW_SIZE],recovery=False,host_model=True)
        assert not report['record_findings'] and not report['header_findings']
        assert ram[pos+6]==0x37
        for label in ('cap_a','cap_b','cap_c'):
            assert struct.unpack_from('<H',ram,sym[label]-0x4000)[0]==1
        # Each mutant starts with the same passing specimen; one byte only.
        mutations=[('ram',sym['pattern_check']-0x4000,0xbe,0xb6),
                   ('triple',pos+11,0xc1,0xd1),
                   ('store',sym['store_word']-0x4000,0x73,0x72)]
        mirror=sym['mirror_load']-0x4000
        assert binary[mirror:mirror+2]==b'\xed\x5b' and binary[mirror+2]!=255
        mutations.append(('mirror',mirror+2,binary[mirror+2],binary[mirror+2]+1))
        for kind,at,old,new in mutations:
            changed=bytearray(binary);assert changed[at]==old;changed[at]=new
            path=work/(kind+'.bin');path.write_bytes(changed)
            dump=work/(kind+'-ram.bin')
            run=subprocess.run([runner,str(path),str(dump)],capture_output=True,timeout=30)
            assert run.returncode==0,(kind,run.stderr)
            ram=dump.read_bytes()
            observed=capture.content(run.stdout+ram[0x3000:0x3000+capture.RAW_SIZE],recovery=False)
            assert observed['integrity_status']=='FINDINGS',kind
            if kind=='ram':
                assert observed['ram_pretest']['failures']==5*capture.RAW_SIZE
                assert observed['ram_pretest']['first_address']=='7000'
                assert observed['header_findings']==['pre_ram_failures']
                assert not observed['record_findings']
            elif kind=='mirror':
                assert observed['delayed_scan']['reported_failures']==len(capture.ROWS)
                assert observed['header_findings']==['reported_delayed_failures']
                assert all(x['kinds']==['mirror_relation'] for x in observed['record_findings'])
            else:
                assert len(observed['record_findings'])==len(capture.ROWS)
                required='reported_post_triple' if kind=='triple' else 'reported_immediate_store'
                assert all(required in x['kinds'] for x in observed['record_findings'])
                assert not observed['header_findings']
        # Passing specimen -> one verification opcode mutation -> exact error path.
        at=sym['patch_check']-0x4000;bad=bytearray(binary);assert bad[at]==0xbe;bad[at]=0xb6
        fault=work/'fault.bin';fault.write_bytes(bad)
        run=subprocess.run([runner,str(fault)],capture_output=True,timeout=10)
        assert run.returncode==1 and run.stdout==b'\x03',(run.stdout,run.stderr)
        assert run.stderr.split()[0]==b'M139_FDC_HOST_EXEC_FAILED',run.stderr
    print('M140_CAPTURE_PAYLOAD_PASS (bounded software diagnostics, not silicon)')


if __name__=='__main__':
    p=argparse.ArgumentParser()
    p.add_argument('--runner')
    p.add_argument('--assembler')
    for name in ('rom-worker','roms','source-image','build-report','passing-integration','output'):
        p.add_argument('--'+name,type=Path)
    args,remaining=p.parse_known_args()
    if args.rom_worker:
        if not all((args.roms,args.source_image,args.build_report,args.passing_integration,args.output)):
            p.error('ROM fault QA requires explicit bindings and a passing capture integration manifest')
        test_fixed.fault_qa(args,case='capture',program='FDCCAP',message='M140_CAPTURE_PATCH_VERIFY')
    elif args.runner:payloads(args.runner,args.assembler)
    else:unittest.main(argv=['test_capture',*remaining])
