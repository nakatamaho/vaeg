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
"""Pure content/allocator tests; optional assembled-payload CPU integration."""
from __future__ import annotations
import argparse
import copy
from pathlib import Path
import subprocess
import struct
import tempfile
import unittest
import build
import results


def fixture(unit='MODEL_T'):
    rows=['M139 FDC PROBE 1']
    for a in results.VALUES:
        for f in results.VALUES:
            row=[(a<<8)|f,*results.zilog_af(a,f)]
            rows.append('F '+''.join(f'{x:04X}' for x in row))
    rows.append('T empty r=0 n=0 '+('MODEL_T=208' if unit=='MODEL_T' else 'E=0000'))
    for name,(_,body) in build.BODIES.items():
        for repeat in (1,2):
            for n in build.ROUNDS:
                v=269+n*(60000*(body+26)+29)
                ticks=f'MODEL_T={v}' if unit=='MODEL_T' else f'E={n*100:04X}'
                rows.append(f'T {name} r={repeat} n={n} {ticks}')
    rows+=['M139_FDC_PROBE_COMPLETE','M139_FDC_RAM_RESTORED']
    return ('\r\n'.join(rows)+'\r\n').encode('ascii')


class Content(unittest.TestCase):
    def test_passing(self):
        self.assertEqual(results.probe(fixture(),unit='MODEL_T',host_model=True)['af_records'],256)
        self.assertEqual(results.probe(fixture('E'),unit='E')['timing_records'],1+6*len(build.BODIES))

    def mutate(self,data,old,new,code,unit='MODEL_T'):
        results.probe(data,unit=unit,host_model=unit=='MODEL_T')
        self.assertEqual(data.count(old),1)
        with self.assertRaises(results.RecordError) as caught:
            results.probe(data.replace(old,new,1),unit=unit,host_model=unit=='MODEL_T')
        self.assertEqual(caught.exception.code,code)

    def test_missing_af(self):
        line=next(x for x in fixture().splitlines(keepends=True) if x.startswith(b'F '))
        self.mutate(fixture(),line,b'','M139_AF_SET')

    def test_duplicate_af(self):
        line=next(x for x in fixture().splitlines(keepends=True) if x.startswith(b'F '))
        self.mutate(fixture(),line,line+line,'M139_AF_DUPLICATE')

    def test_wrong_af(self):
        line=next(x for x in fixture().splitlines(keepends=True) if x.startswith(b'F '))
        changed=bytearray(line);changed[13]=ord('5') if changed[13]!=ord('5') else ord('4')
        self.mutate(fixture(),line,bytes(changed),'M139_HOST_AF_MISMATCH')

    def test_wrong_model_cycles(self):
        self.mutate(fixture(),b'T branch r=1 n=1 MODEL_T=1560298',b'T branch r=1 n=1 MODEL_T=1560299','M139_HOST_TIME_MISMATCH')

    def test_missing_restoration(self):
        self.mutate(fixture(),b'M139_FDC_RAM_RESTORED\r\n',b'','M139_RECOVERY_RECORD')

    def test_timer_inactive(self):
        self.mutate(fixture('E'),b'T branch r=1 n=1 E=0064',b'T branch r=1 n=1 E=0000','M139_TIMER_INACTIVE','E')

    def test_timer_nonlinear(self):
        self.mutate(fixture('E'),b'T branch r=1 n=4 E=0190',b'T branch r=1 n=4 E=01FF','M139_TIME_NONLINEAR','E')

    def test_nonuniform_vrtc_edges(self):
        # Independent analytic +/-2-edge measurement-phase extrema. Middle
        # residual is exactly four, not two, for two nonuniform VRTC edges.
        data=fixture('E').replace(b'T branch r=1 n=1 E=0064',b'T branch r=1 n=1 E=0066').replace(b'T branch r=1 n=4 E=0190',b'T branch r=1 n=4 E=018E').replace(b'T branch r=1 n=16 E=0640',b'T branch r=1 n=16 E=0642')
        results.probe(data,unit='E')
        self.mutate(data,b'T branch r=1 n=4 E=018E',b'T branch r=1 n=4 E=018D','M139_TIME_NONLINEAR','E')

    def test_cpm_screen_terminal(self):
        memory=bytearray(0x40000)
        for i,text in ((20,b'A>DIR'),(21,b'A: FDCPRB   TXT'),(22,b'A>')):
            memory[0x6300+120*i:0x6300+120*i+len(text)]=text
        data=b'VAEGSCN1'+struct.pack('<II6II',1,0,*([0]*6),len(memory))+memory
        rows=results.cpm_screen(data)
        results.terminal(rows,'FDCPRB')
        # One content mutation, after a passing fixed-layout fixture.
        bad=list(rows);bad[20]=bad[20].replace('DIR','XIR')
        with self.assertRaises(results.RecordError) as caught:
            results.terminal(bad,'FDCPRB')
        self.assertEqual(caught.exception.code,'M139_INTEGRATION_DIR_COMMAND')
        malformed=bytearray(data);malformed[12:16]=struct.pack('<I',8192)
        with self.assertRaises(results.RecordError) as caught:
            results.cpm_screen(bytes(malformed))
        self.assertEqual(caught.exception.code,'M139_SCREEN_DUMP')

    def test_calibration_reference(self):
        report=results.probe(fixture('E'),unit='E')
        results.calibrate(copy.deepcopy(report),50.0,'synthetic-reference')
        with self.assertRaises(results.RecordError) as caught:
            results.calibrate(copy.deepcopy(report),float('nan'),'synthetic-reference')
        self.assertEqual(caught.exception.code,'M139_VRTC_REFERENCE')

    def test_smoke_classification(self):
        s=b'M139 FDC NEG SMOKE ONLY\r\nneg...........................  STOCK=d638dd6a CRC=d638dd6a\r\nTests complete\r\nM139_FDC_RAM_RESTORED\r\n'
        self.assertEqual(results.zex(s,smoke=True)['groups'],1)
        with self.assertRaises(results.RecordError) as caught:
            results.zex(s,smoke=False)
        self.assertEqual(caught.exception.code,'M139_ZEX_COMPLETION')


class PatchContent(unittest.TestCase):
    def fixture(self):
        source='before\n'
        patch='--- a/fixture\n+++ b/fixture\n@@ -1 +1 @@\n-before\n+after\n'
        entry={'file':'fixture.patch','input_sha256':build.digest(source.encode()),
               'patch_sha256':build.digest(patch.encode()),
               'output_sha256':build.digest(b'after\n')}
        self.assertEqual(build.apply_fdc_patch(source,patch,entry),'after\n')
        return source,patch,entry

    def test_patch_digest(self):
        source,patch,entry=self.fixture()
        with self.assertRaises(build.BuildError) as caught:
            build.apply_fdc_patch(source,patch+'# altered\n',entry)
        self.assertEqual(caught.exception.code,'M140_ZEX_PATCH_DIGEST')

    def test_patch_input(self):
        source,patch,entry=self.fixture()
        with self.assertRaises(build.BuildError) as caught:
            build.apply_fdc_patch(source.replace('before','Before'),patch,entry)
        self.assertEqual(caught.exception.code,'M140_ZEX_PATCH_INPUT')

    def test_patch_output(self):
        source,patch,entry=self.fixture()
        changed=dict(entry,output_sha256='0'*64)
        with self.assertRaises(build.BuildError) as caught:
            build.apply_fdc_patch(source,patch,changed)
        self.assertEqual(caught.exception.code,'M140_ZEX_PATCH_OUTPUT')


class CaptureContent(unittest.TestCase):
    def fixture(self):
        files=build.cpm.pad_cpm_records({'OWN.COM':b'\xc9'})
        raw,_=build.cpm.build_cpm_raw(files)
        original=bytearray(build.cpm.wrap_cpm_d88(raw))
        original[27]=0  # this fixture is a 2D capture, not the helper's 2DD default
        original=bytes(original)
        tracks=struct.unpack_from('<164I',original,32)
        table=[0]*164
        for c in range(40):
            for h in range(2):table[4*c+h]=tracks[2*c+h]
        capture=bytearray(original)
        struct.pack_into('<164I',capture,32,*table)
        capture=bytes(capture)
        self.assertEqual(results.normalize_capture_image(capture),original)
        self.assertEqual(results.disk_files(capture),results.disk_files(original))
        return original,capture

    def test_roundtrip_and_immutable_source(self):
        original,capture=self.fixture()
        saved=bytes(capture)
        normalized=results.normalize_capture_image(capture)
        self.assertEqual(normalized[:32],capture[:32])
        self.assertEqual(normalized[688:],capture[688:])
        self.assertEqual(capture,saved)
        self.assertEqual(results.normalize_capture_image(original),original)

    def test_one_extra_track(self):
        original,capture=self.fixture()
        changed=bytearray(capture)
        struct.pack_into('<I',changed,32+2*4,688)
        with self.assertRaises(results.RecordError) as caught:
            results.normalize_capture_image(bytes(changed))
        self.assertEqual(caught.exception.code,'M140_D88_CAPTURE_TRACKS')

    def test_one_media_type(self):
        original,capture=self.fixture()
        changed=bytearray(capture);changed[27]=0x10
        with self.assertRaises(results.RecordError) as caught:
            results.normalize_capture_image(bytes(changed))
        self.assertEqual(caught.exception.code,'M140_D88_CAPTURE_TRACKS')

    def test_one_sector_id(self):
        original,capture=self.fixture()
        changed=bytearray(capture);changed[688+2]=17
        with self.assertRaises(build.BuildError) as caught:
            results.normalize_capture_image(bytes(changed))
        self.assertEqual(caught.exception.code,'M139_D88_GEOMETRY')

    def test_one_sector_status(self):
        original,capture=self.fixture()
        changed=bytearray(capture);changed[688+8]=0xb0
        with self.assertRaises(results.RecordError) as caught:
            results.normalize_capture_image(bytes(changed))
        self.assertEqual(caught.exception.code,'M140_D88_CAPTURE_STATUS')


class Allocator(unittest.TestCase):
    def fixture(self):
        files=build.cpm.pad_cpm_records({'KEEP.COM':b'private-input-placeholder'})
        raw,_=build.cpm.build_cpm_raw(files)
        raw=bytearray(raw)
        # Put the existing file in the SECOND half of the 128-entry directory.
        raw[0x4000+100*32:0x4020+100*32]=raw[0x4000:0x4020]
        raw[0x4000:0x4020]=b'\xe5'*32
        return build.cpm.wrap_cpm_d88(bytes(raw))

    def test_preserves_files_and_header(self):
        original=self.fixture();expected=results.disk_files(original)
        output=build.add_files(original,{'OWN.COM':b'\xc9'})
        actual=results.disk_files(output)
        for n,data in expected.items():self.assertEqual(actual[n],data)
        self.assertEqual(actual['OWN.COM'],b'\xc9'+b'\x1a'*127)
        self.assertEqual(original[:688],output[:688])

    def test_multi_extent_license(self):
        original=self.fixture()
        data=bytes(range(256))*140
        output=build.add_files(original,{'LICENSE.TXT':data})
        self.assertEqual(results.disk_files(output)['LICENSE.TXT'],data)
        self.assertEqual(results.disk_files(output)['KEEP.COM'],results.disk_files(original)['KEEP.COM'])

    def test_existing_name(self):
        original=self.fixture();build.add_files(original,{'OWN.COM':b'\xc9'})
        with self.assertRaises(build.BuildError) as caught:
            build.add_files(original,{'KEEP.COM':b'\xc9'})
        self.assertEqual(caught.exception.code,'M139_CPM_NAME_EXISTS')

    def test_one_geometry_mutation(self):
        original=self.fixture();build.add_files(original,{'OWN.COM':b'\xc9'})
        changed=bytearray(original);changed[688+2]=2
        with self.assertRaises(build.BuildError) as caught:
            build.add_files(bytes(changed),{'OWN.COM':b'\xc9'})
        self.assertEqual(caught.exception.code,'M139_D88_GEOMETRY')


def payloads(runner,assembler):
    with tempfile.TemporaryDirectory(prefix='m140-payload-') as td:
        work=Path(td)
        build.assemble_images(assembler,work)
        for kind in ('fdcprb','fdcone'):
            run=subprocess.run([runner,str(work/(kind+'.bin'))],capture_output=True,timeout=60)
            if run.returncode:raise AssertionError(run.stderr.decode())
            if kind=='fdcprb':results.probe(run.stdout,unit='MODEL_T',recovery=False,host_model=True)
            else:results.zex(run.stdout,smoke=True,recovery=False)
        # Passing RAM map -> one instruction operand mutation: mapped write to
        # scratch 7339h becomes an unmapped write. No real evidence dependency.
        good=build.zex.assemble('org 4000h\nld sp,7effh\nxor a\nout (0f8h),a\nld (7339h),a\nld a,4\nrst 18h\njp 2ah\n',assembler,'map-fixture')
        path=work/'map.bin';path.write_bytes(good)
        run=subprocess.run([runner,str(path)],capture_output=True,timeout=10)
        assert run.returncode==0,run.stderr
        bad=bytearray(good);pos=bad.index(b'\x32\x39\x73');bad[pos+2]=0x23;path.write_bytes(bad)
        run=subprocess.run([runner,str(path)],capture_output=True,timeout=10)
        assert run.returncode!=0 and run.stderr.split()[0]==b'M139_FDC_MAP_WRITE',run.stderr
    print('M139_FDC_PAYLOAD_TESTS_PASS (mapped synthetic host, not hardware)')

if __name__=='__main__':
    parser=argparse.ArgumentParser()
    parser.add_argument('--runner')
    parser.add_argument('--assembler')
    args,remaining=parser.parse_known_args()
    if args.runner:
        payloads(args.runner,args.assembler)
    else:
        unittest.main(argv=['test_fdc_tools',*remaining])
