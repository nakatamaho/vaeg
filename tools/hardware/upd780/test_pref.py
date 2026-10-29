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
import argparse
import json
from pathlib import Path
import struct
import subprocess
import tempfile
import unittest
import build
import pref
import results
import seed
import test_fixed


def fixture(profile='nec_a',extra=False):
    text=bytearray(b'M140 FDC PREF 1\r\n');raw=bytearray(pref.MAGIC+struct.pack('<HHBBHHH',0,0,127,0,0,0,0))
    for i,(r,m,op,af) in enumerate(pref.ROWS):
        post=pref.prediction(op,af,m,profile)
        if extra and i==0:post^=0x20
        words=[af]*4+[post]*4+[post^0xffff]
        text+=f'P {r:02X} {m:02X} {op} '.encode()+' '.join(f'{v:04X}' for v in words).encode()+b' 00\r\n'
        raw+=struct.pack('<BBB9HB',r,m,ord(op),*words,0)
    return bytes(text)+pref.MARKER+raw+b'END!'+seed.RESTORED+b'\x1a'


class PrefTests(unittest.TestCase):
    def test_complete(self):
        r=pref.content(fixture(),host_model=True)
        self.assertEqual(r['records'],128);self.assertEqual(r['candidate_matches']['nec_a'],128)
        self.assertEqual(r['integrity_status'],'NO_FINDINGS_IN_TESTED_CONTRACT')
        self.assertEqual(len(set(pref.ROWS)),128)

    def test_distinct_profiles(self):
        for profile in pref.PROFILES:
            with self.subTest(profile=profile):
                r=pref.content(fixture(profile));self.assertEqual(r['candidate_matches'][profile],128)
                self.assertEqual(r['outside_candidate_union'],[])
                self.assertEqual(sum(v==128 for v in r['candidate_matches'].values()),1)

    def test_profile_guard(self):
        pref.prediction('S',0xbb,0,'nec_a')
        with self.assertRaises(results.RecordError) as caught:pref.prediction('S',0xbb,0,'unknown')
        self.assertEqual(caught.exception.code,'M140_PREF_PROFILE')

    def test_external_predictions(self):
        self.assertEqual([pref.prediction('S',0xbb,m,'zilog_q') for m in range(4)],[0x81,0xa9,0xa9,0xa9])
        self.assertEqual([pref.prediction('C',0xbb,m,'st_cmos') for m in range(4)],[0x90,0xb0,0xb0,0xb0])
        self.assertEqual(pref.prediction('S',0xbb,0,'unconditional_or'),0xa9)

    def test_outside(self):
        pref.content(fixture(),host_model=True);r=pref.content(fixture(extra=True));self.assertEqual(r['outside_candidate_union'],[{'row':0,'after':'00A1'}])

    def mutation(self,offset,code):
        good=fixture();pref.content(good,host_model=True);bad=bytearray(good);start=good.index(pref.MARKER)+len(pref.MARKER);bad[start+offset]^=1
        with self.assertRaises(results.RecordError) as caught:pref.content(bytes(bad))
        self.assertEqual(caught.exception.code,code)

    def test_frame(self):self.mutation(0,'M140_PREF_BINARY_FRAME')
    def test_descriptor(self):self.mutation(21,'M140_PREF_DESCRIPTOR')
    def test_transport(self):self.mutation(31,'M140_PREF_PATH_MISMATCH')
    def test_pattern(self):self.mutation(12,'M140_PREF_PATTERN_SET')

    def test_host_mismatch(self):
        pref.content(fixture(),host_model=True)
        with self.assertRaises(results.RecordError) as caught:pref.content(fixture('zilog_q'),host_model=True)
        self.assertEqual(caught.exception.code,'M140_PREF_HOST_MISMATCH')

    def test_templates(self):
        expected=[b'\x06\x28\x00\xb8',b'\x06\x28\xb8\x00',b'\x06\x28\xb8\xdd',b'\x06\x28\xb8\xfd']
        for m in range(4):
            b=pref.sampler(m);self.assertEqual(len(b),18);self.assertEqual(b[6:10],expected[m]);self.assertEqual(b[10:],b'\x00\xf5\xf5\xf5\xe1\xc1\xd1\xc9')
        pref.sampler(0)
        with self.assertRaises(build.BuildError) as caught:pref.sampler(4)
        self.assertEqual(caught.exception.code,'M140_PREF_MODE')

    def test_source(self):
        text=pref.source();self.assertIn('    ld hl,480ah',text);self.assertIn('    ld b,18\n    call patch_copy',text)
        self.assertIn('    cp 4\n    jp nc,patch_failed',text);self.assertIn('sample_dispatch:\n    ld bc,(target)',text)

    def test_loader(self):
        text=pref.loader_source(b'\xc9');self.assertIn("'FDCPREF DAT'",text);self.assertIn('M140_PREF_PATCH_VERIFY',text)
        self.assertLess(text.index('verify_loop:'),text.index('    ld c,22\n'))


def payloads(runner,assembler):
    with tempfile.TemporaryDirectory(prefix='m140-pref-') as td:
        work=Path(td);pref.assemble(assembler,work);binary=(work/'fdcpref.bin').read_bytes();sym=json.loads((work/'fdcpref-symbols.json').read_text())
        def run(data,name):
            path=work/(name+'.bin');path.write_bytes(data);dump=work/(name+'-ram.bin');r=subprocess.run([runner,str(path),str(dump)],capture_output=True,timeout=30)
            return r,dump.read_bytes() if dump.exists() else b''
        r,ram=run(binary,'production');assert r.returncode==0,r.stderr
        content=pref.content(r.stdout+ram[0x3000:0x3000+pref.RAW_SIZE],recovery=False,host_model=True);assert content['candidate_matches']['nec_a']==128
        one=build.replace_once(pref.source(),'ROW_COUNT: equ 128','ROW_COUNT: equ 1');small,ss=build.zex.assemble_with_symbols(one,assembler,'pref-one-row')
        for mode in range(4):
            data=bytearray(small);data[ss['initial_table']-0x4000+21]=mode;r,ram=run(data,'mode-'+str(mode));assert r.returncode==0,r.stderr
            rr,mm,op,*words,status=struct.unpack_from('<BBB9HB',ram,0x3014)
            assert (rr,mm,op)==(0,mode,ord('S')) and words==[0xbb]*4+[0x81]*4+[0xff7e] and status==0
            expected=bytearray(pref.sampler(mode));expected[10]=0x37;assert ram[0x800:0x812]==expected
            assert ram[0x904:0x907]==b'\xcd\x00\x48'
        # Baseline passed. One CP operand mutation must retain diagnostic findings.
        bad=bytearray(binary);at=sym['sampler_template']-0x4000+9;assert bad[at]==0xb8;bad[at]=0xb9
        r,ram=run(bad,'cp-fault');assert r.returncode==0
        diagnostic=pref.content(r.stdout+ram[0x3000:0x3000+pref.RAW_SIZE],recovery=False)
        assert diagnostic['integrity_status']=='FINDINGS' and any('control_target' in row['kinds'] for row in diagnostic['record_findings'])
        # One code-copy verifier mutation: dedicated stream03, no disk interaction.
        bad=bytearray(binary);at=sym['patch_check']-0x4000;assert bad[at]==0xbe;bad[at]=0xb6
        r,ram=run(bad,'patch-fault');assert r.returncode==1 and r.stdout==b'\x03' and not ram
        # One mode mutation from a passing single-row bank fixture fails closed.
        bad=bytearray(small);bad[ss['initial_table']-0x4000+21]=4
        r,ram=run(bad,'mode-fault');assert r.returncode==1 and r.stdout==b'\x03' and not ram
    print('M140_PREF_PAYLOAD_PASS (software only; CP/prefix byte order verified, no physical Q readout)')


if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--runner');p.add_argument('--assembler')
    for name in ('rom-worker','roms','source-image','build-report','passing-integration','output'):p.add_argument('--'+name,type=Path)
    args,remaining=p.parse_known_args()
    if args.rom_worker:
        if not all((args.roms,args.source_image,args.build_report,args.passing_integration,args.output)):p.error('explicit bindings required')
        test_fixed.fault_qa(args,case='pref',program='FDCPREF',message='M140_PREF_PATCH_VERIFY')
    elif args.runner:payloads(args.runner,args.assembler)
    else:unittest.main(argv=['test_pref',*remaining])
