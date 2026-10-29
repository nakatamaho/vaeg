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
"""Closed XADR frames, installed-bank fixtures and single-byte fault safety."""
import argparse
import json
from pathlib import Path
import struct
import subprocess
import tempfile
import unittest
import build
import results
import seed
import test_fixed
import xadr


def fixture(kind=None):
    text=bytearray(b'M140 FDC XADR 1\r\n')
    raw=bytearray(xadr.MAGIC+struct.pack('<HHBBHHH',1 if kind=='ram' else 0,
                                       0x74d7 if kind=='ram' else 0,127,0,0,0,0))
    for i,(r,s,op,af) in enumerate(xadr.ROWS):
        post=1 if op=='S' or af==0x28 else 0x10
        if i==0 and kind=='axis':post^=8
        words=[af,af,af,af,post,post,post,post,post^0xffff]
        text+=f'X {r:02X} {s:02X} {op} '.encode()+' '.join(f'{v:04X}' for v in words).encode()+b' 00\r\n'
        raw+=struct.pack('<BBB9HB',r,s,ord(op),*words,0)
    return bytes(text)+xadr.MARKER+raw+b'END!'+seed.RESTORED+b'\x1a'


class XadrTests(unittest.TestCase):
    def test_complete(self):
        r=xadr.content(fixture(),host_model=True)
        self.assertEqual(r['records'],128)
        self.assertEqual(r['integrity_status'],'NO_FINDINGS_IN_TESTED_CONTRACT')
        self.assertEqual(r['paired_effects'],{'sampler_axis':[],'caller_axis':[]})
        self.assertEqual(len(set(xadr.ROWS)),128)

    def test_crossed_addresses(self):
        sites=xadr.sites()
        self.assertEqual([v['iut']&8 for v in sites.values()],[0,0,8,8])
        self.assertEqual([v['return']&8 for v in sites.values()],[0,8,0,8])
        for r in range(8):self.assertEqual(sum(row[0]==r for row in xadr.ROWS),16)

    def mutation(self,offset,code):
        good=fixture();xadr.content(good,host_model=True);bad=bytearray(good)
        start=good.index(xadr.MARKER)+len(xadr.MARKER);bad[start+offset]^=1
        with self.assertRaises(results.RecordError) as caught:xadr.content(bytes(bad))
        self.assertEqual(caught.exception.code,code)

    def test_frame(self):self.mutation(0,'M140_XADR_BINARY_FRAME')
    def test_descriptor(self):self.mutation(21,'M140_XADR_DESCRIPTOR')
    def test_transport(self):self.mutation(31,'M140_XADR_PATH_MISMATCH')
    def test_pattern_mask(self):self.mutation(12,'M140_XADR_PATTERN_SET')
    def test_header(self):self.mutation(13,'M140_XADR_HEADER')

    def test_findings_retained(self):
        xadr.content(fixture(),host_model=True);data=fixture('ram');r=xadr.content(data)
        self.assertEqual(r['header_findings'],['pre_ram_failures'])
        with self.assertRaises(results.RecordError) as caught:xadr.content(data,host_model=True)
        self.assertEqual(caught.exception.code,'M140_XADR_HOST_INTEGRITY')

    def test_pairing(self):
        xadr.content(fixture(),host_model=True);data=fixture('axis');r=xadr.content(data)
        self.assertEqual(r['integrity_status'],'NO_FINDINGS_IN_TESTED_CONTRACT')
        self.assertEqual(len(r['paired_effects']['sampler_axis']),1)
        self.assertEqual(len(r['paired_effects']['caller_axis']),1)
        self.assertEqual(r['paired_effects']['sampler_axis'][0]['sites'],[0,2])
        self.assertEqual(len(r['primary_repeat_varying_groups']),1)
        self.assertEqual(r['primary_repeat_varying_groups'][0]['site'],0)
        with self.assertRaises(results.RecordError) as caught:xadr.content(data,host_model=True)
        self.assertEqual(caught.exception.code,'M140_XADR_HOST_MISMATCH')

    def test_source_dispatch(self):
        text=xadr.source()
        self.assertEqual(text.count('    jp (hl)\n'),1)
        self.assertIn('sample_dispatch:\n    ld bc,(target)\n    ld hl,(caller_target)\n    jp (hl)',text)
        self.assertNotIn('    call sampler\n',text)
        self.assertIn('caller_template:\n    ld de,0\n    ld hl,0\n    call 4800h\n    jp capture_return',text)
        self.assertIn('    ld b,14\n    call patch_copy',text)
        self.assertIn('    ld b,12\n    call patch_copy',text)

    def test_parent_identity(self):
        parent=(build.HERE/'capture.asm').read_bytes();fragments=(build.HERE/'xadr.asm').read_text()
        xadr.derive(parent,fragments);bad=bytearray(parent);bad[2]^=1
        with self.assertRaises(build.BuildError) as caught:xadr.derive(bytes(bad),fragments)
        self.assertEqual(caught.exception.code,'M140_XADR_PARENT')

    def test_fragment_set(self):
        parent=(build.HERE/'capture.asm').read_bytes();fragments=(build.HERE/'xadr.asm').read_text()
        xadr.derive(parent,fragments);bad=fragments.replace('; @loop','; @oops',1)
        with self.assertRaises(build.BuildError) as caught:xadr.derive(parent,bad)
        self.assertEqual(caught.exception.code,'M140_XADR_SOURCE')

    def test_loader(self):
        text=xadr.loader_source(b'\xc9')
        self.assertIn("    db 'FDCXADR DAT'",text)
        self.assertIn('    ld hl,7000h\n    ld de,2840\n',text)
        self.assertIn('M140_XADR_PATCH_VERIFY',text)
        self.assertLess(text.index('verify_loop:'),text.index('    ld c,22\n'))


def payloads(runner,assembler):
    with tempfile.TemporaryDirectory(prefix='m140-xadr-') as td:
        work=Path(td);xadr.assemble(assembler,work)
        sym=json.loads((work/'fdcxadr-symbols.json').read_text());binary=(work/'fdcxadr.bin').read_bytes()
        def run(data,name):
            path=work/(name+'.bin');path.write_bytes(data);dump=work/(name+'-ram.bin')
            result=subprocess.run([runner,str(path),str(dump)],capture_output=True,timeout=30)
            return result,dump.read_bytes() if dump.exists() else b''
        result,ram=run(binary,'production');assert result.returncode==0,result.stderr
        report=xadr.content(result.stdout+ram[0x3000:0x3000+xadr.RAW_SIZE],recovery=False,host_model=True)
        assert report['records']==128 and not any(report['paired_effects'].values())
        # Test-only one-row fixtures: not production data and no validator bypass.
        # Start from the same source, restrict loop/print count, select each site.
        one=build.replace_once(xadr.source(),'ROW_COUNT: equ 128','ROW_COUNT: equ 1')
        small,ss=build.zex.assemble_with_symbols(one,assembler,'xadr-one-row-fixture')
        for site,addresses in xadr.sites().items():
            data=bytearray(small);data[ss['initial_table']-0x4000+21]=site
            result,ram=run(data,'site-'+str(site));assert result.returncode==0,result.stderr
            r,s,o,*words,status=struct.unpack_from('<BBB9HB',ram,0x3014)
            assert (r,s,o)==(0,site,ord('S')) and words==[0x28]*4+[1]*4+[0xfffe] and status==0
            sampler=bytearray(xadr.SAMPLER);sampler[6]=0x37
            start=addresses['sampler']-0x4000;assert ram[start:start+14]==sampler
            wrapper=b'\x11\x00\x00\x21\x00\x00\xcd'+struct.pack('<H',addresses['sampler'])+b'\xc3'+struct.pack('<H',ss['capture_return'])
            start=addresses['caller']-0x4000;assert ram[start:start+12]==wrapper
            for label,key in [('sampler_target','sampler'),('iut_target','iut'),('caller_target','caller')]:
                assert struct.unpack_from('<H',ram,ss[label]-0x4000)[0]==addresses[key]
        # Baseline production passed; one template POP mutation must be diagnosed.
        at=sym['sampler_template']-0x4000+11;bad=bytearray(binary);assert bad[at]==0xc1;bad[at]=0xd1
        result,ram=run(bad,'triple-fault');assert result.returncode==0,result.stderr
        r=xadr.content(result.stdout+ram[0x3000:0x3000+xadr.RAW_SIZE],recovery=False)
        assert r['integrity_status']=='FINDINGS' and len(r['record_findings'])==128
        assert all('reported_post_triple' in row['kinds'] for row in r['record_findings'])
        assert not r['header_findings']
        # A distinct single-byte verification fault must halt through stream 03.
        at=sym['patch_check']-0x4000;bad=bytearray(binary);assert bad[at]==0xbe;bad[at]=0xb6
        result,ram=run(bad,'patch-fault')
        assert result.returncode==1 and result.stdout==b'\x03' and not ram,(result.stdout,result.stderr)
        assert result.stderr.split()[0]==b'M139_FDC_HOST_EXEC_FAILED'
    print('M140_XADR_PAYLOAD_PASS (software bank fixtures, not physical fetch/SP/Q trace)')


if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--runner');p.add_argument('--assembler')
    for name in ('rom-worker','roms','source-image','build-report','passing-integration','output'):
        p.add_argument('--'+name,type=Path)
    args,remaining=p.parse_known_args()
    if args.rom_worker:
        if not all((args.roms,args.source_image,args.build_report,args.passing_integration,args.output)):
            p.error('ROM fault QA requires explicit bindings and passing xadr integration')
        test_fixed.fault_qa(args,case='xadr',program='FDCXADR',message='M140_XADR_PATCH_VERIFY')
    elif args.runner:payloads(args.runner,args.assembler)
    else:unittest.main(argv=['test_xadr',*remaining])
