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
"""Synthetic passing layout fixtures; no private images, assembler or Git needed."""
import struct
import unittest
import context
import results
import seed
import test_capture


def fixture(kind):
    data=bytearray(0x2000);symbols={}
    def put(a,b):data[a-0x4000:a-0x4000+len(b)]=b
    put(0x4000,b'\xf3\x31\xff\x7e')
    if kind=='seed':
        for i,(s,op,af) in enumerate(seed.ROWS):
            alu=b'\xaf' if s==0 else bytes((0x3e,(0,0,8,32,40)[s],0xb7))
            for suffix,offset,field in (('pre',3,4),('post',24,6)):
                a=0x5000+48*i+offset;symbols[f'case_{i}_{suffix}']=a
                put(a-3,b'\x01'+struct.pack('<H',af))
                body=b'\xf3'+alu+b'\xc5\xf1'
                if suffix=='post':body+=bytes((0x37 if op=='S' else 0x3f,))
                put(a,body+b'\xf5\xd1\xed\x53'+struct.pack('<H',0x7008+8*i+field))
        return data,symbols
    a=0x40d8 if kind=='fixed' else 0x4800
    symbols.update(sampler=a,sampler_iut=a+6,control_call=0x405f,measured_call=0x407d,
                   target=0x42ca,cap_a=0x42ce,cap_b=0x42d0,cap_c=0x42d2,save_captures=0x4134)
    put(a,b'\xf3\x3e\x00\xaf\xc5\xf1\x00'+
        (b'\xf5\xd1\xc9' if kind=='fixed' else b'\xf5\xf5\xf5\xe1\xc1\xd1\xc9'))
    for k in ('control_call','measured_call'):
        p=symbols[k];put(p,b'\xcd'+struct.pack('<H',a))
        if kind=='capture':
            put(p-4,b'\xed\x4b'+struct.pack('<H',symbols['target']))
            put(p+3,b'\xcd'+struct.pack('<H',symbols['save_captures']))
    put(symbols['save_captures'],b'\xed\x53\xce\x42\xed\x43\xd0\x42\x22\xd2\x42\xc9')
    return data,symbols


class ContextTests(unittest.TestCase):
    def mutation(self,kind,a,code):
        b,s=fixture(kind);context.layout(kind,b,s)
        b[a-0x4000]^=1
        with self.assertRaises(results.RecordError) as exc:context.layout(kind,b,s)
        self.assertEqual(exc.exception.code,code)

    def test_nominal_stack(self):
        for kind in ('fixed','capture'):
            b,s=fixture(kind);c=context.layout(kind,b,s)
            self.assertEqual(c['nominal_stack_words'],[0x7efb] if kind=='fixed' else [0x7efb,0x7ef9,0x7ef7])
        b,s=fixture('seed');c=context.layout('seed',b,s)
        self.assertEqual(len(c['contexts']),80)
        self.assertTrue(all(x['nominal_stack_words']==[0x7efd] for x in c['contexts']))
        self.assertTrue(all(not x['control_has_nop'] for x in c['contexts']))

    def test_entry(self):self.mutation('capture',0x4002,'M140_CONTEXT_ENTRY')
    def test_prefix(self):self.mutation('capture',0x4804,'M140_CONTEXT_PREFIX')
    def test_tail(self):self.mutation('capture',0x480b,'M140_CONTEXT_TAIL')
    def test_call(self):self.mutation('fixed',0x407d,'M140_CONTEXT_CALL')
    def test_reload(self):self.mutation('capture',0x4079,'M140_CONTEXT_RELOAD')
    def test_mailbox(self):self.mutation('capture',0x4138,'M140_CONTEXT_MAILBOX')
    def test_save_call(self):self.mutation('capture',0x4080,'M140_CONTEXT_SAVE_CALL')
    def test_seed_store(self):self.mutation('seed',0x5021,'M140_CONTEXT_SEED_SPAN')
    def test_seed_target(self):self.mutation('seed',0x5000,'M140_CONTEXT_TARGET')

    def test_symbol_range(self):
        b,s=fixture('fixed');context.layout('fixed',b,s);s['measured_call']=0x3fff
        with self.assertRaises(results.RecordError) as exc:context.layout('fixed',b,s)
        self.assertEqual(exc.exception.code,'M140_CONTEXT_RANGE')

    def test_program_binding(self):
        b,s=fixture('capture');program=b'loader'+b;context.program_binding(b,program)
        changed=bytearray(program);changed[0x4806-0x4000+6]^=1
        with self.assertRaises(results.RecordError) as exc:context.program_binding(b,changed)
        self.assertEqual(exc.exception.code,'M140_CONTEXT_PROGRAM_BINDING')

    def test_repeated_observation_falsifies_address_only_determinism(self):
        b,s=fixture('capture');c=context.layout('capture',b,s)
        passing=context.observations('capture',test_capture.fixture())
        self.assertEqual(context.evaluate(c,passing)['repeat_varying_groups'],[])
        # One coherent CPU-result event, with dependent copies rendered by fixture.
        changed=context.observations('capture',test_capture.fixture('cpu'))
        r=context.evaluate(c,changed)
        self.assertEqual(len(r['repeat_varying_groups']),1)
        self.assertEqual(r['repeat_varying_groups'][0]['values'][0]['after'],9)
        self.assertEqual(len(r['counterexamples']['literal_iut_bit3']),1)
        self.assertEqual([x&8 for x in c['pushes']],[0,8,8])
        self.assertEqual(c['nominal_stack_words'][0],0x7efb)

    def test_unrolled_return_is_not_applicable_not_a_pass(self):
        b,s=fixture('seed');c=context.layout('seed',b,s)
        r=context.evaluate(c,[{'row':0,'repeat':0,'seed':0,'operation':'S','target':8,'post':[1]}])
        self.assertEqual(r['hypothesis_samples']['literal_return_bit3'],0)
        self.assertEqual(r['hypothesis_status']['literal_return_bit3'],'NOT_APPLICABLE')
        self.assertEqual(r['hypothesis_samples']['literal_iut_bit3'],1)

    def test_integrity_is_not_hidden_by_hypothesis_evaluation(self):
        passing=test_capture.fixture();context.observations('capture',passing)
        changed=test_capture.fixture('mirror')
        with self.assertRaises(results.RecordError) as exc:context.observations('capture',changed)
        self.assertEqual(exc.exception.code,'M140_CONTEXT_INTEGRITY')

    def test_snapshot_count(self):
        b,s=fixture('capture');c=context.layout('capture',b,s)
        rows=context.observations('capture',test_capture.fixture());context.evaluate(c,rows)
        rows[0]['post'].pop()
        with self.assertRaises(results.RecordError) as exc:context.evaluate(c,rows)
        self.assertEqual(exc.exception.code,'M140_CONTEXT_SNAPSHOT_COUNT')


if __name__=='__main__':unittest.main()
