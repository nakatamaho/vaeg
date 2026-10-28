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
"""Pure fixed-address content tests and mapped patch/readback integration."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess
import tempfile
import unittest
import build
import fixed
import results
import seed


def fixture(changed_row=None):
    text = bytearray(b'M140 FDC FIXED 1\r\n')
    raw = bytearray(fixed.MAGIC)
    for i, (r, s, op, af) in enumerate(fixed.ROWS):
        after = (1 if op == 'S' or af == 0x28 else 0x10)
        if i == changed_row:after ^= 8
        text += f'R {r:02X} {s:02X} {op} {af:04X} {af:04X} {after:04X}\r\n'.encode()
        raw += struct.pack('<BBBHHH', r, s, ord(op), af, af, after)
    return bytes(text) + fixed.MARKER + raw + b'END!' + seed.RESTORED + b'\x1a'


class FixedContent(unittest.TestCase):
    def test_complete(self):
        r = fixed.content(fixture(), host_model=True)
        self.assertEqual(r['records'], 160)
        self.assertEqual(r['repetitions'], 8)
        self.assertEqual(r['repeat_varying_groups'], [])
        self.assertEqual(r['seed_different_result_sets'], [])

    def mutation(self, offset, code, mask=1):
        good = fixture()
        fixed.content(good, host_model=True)
        changed = bytearray(good)
        start = good.index(fixed.MARKER) + len(fixed.MARKER)
        changed[start+offset] ^= mask
        with self.assertRaises(results.RecordError) as caught:
            fixed.content(bytes(changed))
        self.assertEqual(caught.exception.code, code)

    def test_frame(self):self.mutation(0, 'M140_FIXED_BINARY_FRAME')
    def test_repeat_descriptor(self):self.mutation(8, 'M140_FIXED_DESCRIPTOR')
    def test_seed_descriptor(self):self.mutation(9, 'M140_FIXED_DESCRIPTOR')
    def test_precondition(self):self.mutation(13, 'M140_FIXED_PRECONDITION')
    def test_documented_flags(self):self.mutation(15, 'M140_FIXED_DOCUMENTED_FLAGS')
    def test_path_disagreement(self):self.mutation(15, 'M140_FIXED_PATH_MISMATCH', mask=8)

    def test_ascii_only_disagreement(self):
        good = fixture()
        fixed.content(good)
        old = b'R 00 00 S 0028 0028 0001'
        self.assertEqual(good.count(old), 1)
        with self.assertRaises(results.RecordError) as caught:
            fixed.content(good.replace(old, b'R 00 00 S 0028 0028 0009'))
        self.assertEqual(caught.exception.code, 'M140_FIXED_PATH_MISMATCH')

    def test_recovery(self):
        good = fixture()
        fixed.content(good)
        with self.assertRaises(results.RecordError) as caught:
            fixed.content(good.replace(seed.RESTORED, b'UNKNOWN'))
        self.assertEqual(caught.exception.code, 'M139_RECOVERY_RECORD')

    def test_padding(self):
        good = fixture()
        fixed.content(good)
        with self.assertRaises(results.RecordError) as caught:
            fixed.content(good[:-1]+b'!')
        self.assertEqual(caught.exception.code, 'M140_FIXED_PADDING')

    def test_missing_text_row(self):
        good = fixture()
        fixed.content(good)
        with self.assertRaises(results.RecordError) as caught:
            fixed.content(good.replace(good.splitlines(keepends=True)[1], b'', 1))
        self.assertEqual(caught.exception.code, 'M140_FIXED_TEXT_SET')

    def test_repeat_variation_is_observation_not_host_pass(self):
        fixed.content(fixture(), host_model=True)
        changed = fixture(changed_row=0)
        r = fixed.content(changed)
        self.assertEqual(len(r['repeat_varying_groups']), 1)
        self.assertEqual(len(r['seed_different_result_sets']), 1)
        with self.assertRaises(results.RecordError) as caught:
            fixed.content(changed, host_model=True)
        self.assertEqual(caught.exception.code, 'M140_FIXED_HOST_MISMATCH')

    def test_order_and_shared_sampler(self):
        for r in range(8):
            rows = [(s, op, af) for rr, s, op, af in fixed.ROWS if rr == r]
            self.assertEqual(len(rows), 20)
            self.assertEqual(rows, list(fixed.BASE if r % 2 == 0 else reversed(fixed.BASE)))
        text = fixed.source()
        span = text.split('sampler:\n', 1)[1].split('patch_failed:', 1)[0]
        self.assertEqual(span, '    di\nsampler_seed:\n    ld a,0\nsampler_alu:\n'
                         '    xor a\n    push bc\n    pop af\nsampler_iut:\n    nop\n'
                         '    push af\n    pop de\n    ret\n')
        self.assertEqual(text.count('    call sampler\n'), 2)

    def test_fault_fixture(self):
        files = build.cpm.pad_cpm_records({'FDCFIX.COM':b'\xbe\xc9'})
        raw, _ = build.cpm.build_cpm_raw(files)
        image = build.cpm.wrap_cpm_d88(raw)
        mutated = fault_image(image, 0)
        self.assertEqual(sum(a != b for a, b in zip(image, mutated)), 1)
        self.assertEqual(results.disk_files(image)['FDCFIX.COM'][0], 0xbe)
        self.assertEqual(results.disk_files(mutated)['FDCFIX.COM'][0], 0xb6)

    def test_fault_wrong_source(self):
        files = build.cpm.pad_cpm_records({'FDCFIX.COM':b'\xbe\xc9'})
        raw, _ = build.cpm.build_cpm_raw(files)
        image = build.cpm.wrap_cpm_d88(raw)
        mutated = fault_image(image, 0)
        with self.assertRaises(results.RecordError) as caught:
            fault_image(mutated, 0)
        self.assertEqual(caught.exception.code, 'M140_FIXED_FAULT_SOURCE')

    def test_loader_order_and_fatal_signal(self):
        text = fixed.loader_source(b'\xc9')
        self.assertIn('    cp 3\n    jp z,fixed_patch_error\n', text)
        self.assertIn('fixed_patch_error:\n    ld de,fixed_patch_msg\n    jp fatal_common\n', text)
        self.assertIn('    ld hl,7000h\n    ld de,1452\n', text)
        self.assertLess(text.index('seed_read_loop:'), text.index('restore_loop:'))
        self.assertLess(text.index('verify_loop:'), text.index('    ld c,22\n'))
        self.assertIn('    jp nc,log_overflow', text[text.index('restore:'):text.index('seed_read_loop:')])


def fault_image(image, offset):
    """Exactly one COM opcode byte; not a relaxed package/content validator."""
    files = results.disk_files(image)
    results.need('FDCFIX.COM' in files and 0 <= offset < len(files['FDCFIX.COM'])
                 and files['FDCFIX.COM'][offset] == 0xbe, 'M140_FIXED_FAULT_SOURCE')
    raw, locations = build.unpack(image)
    entries = [raw[p:p+32] for p in range(0x4000, 0x5000, 32)
               if raw[p] == 0 and raw[p+1:p+12] == b'FDCFIX  COM']
    results.need(len(entries) == 1 and offset < 16384, 'M140_FIXED_FAULT_LAYOUT')
    block = entries[0][16+offset//2048]
    rawpos = 0x4000 + block*2048 + offset%2048
    diskpos = locations[rawpos//256][0] + rawpos%256
    results.need(image[diskpos] == 0xbe, 'M140_FIXED_FAULT_SOURCE')
    mutated = bytearray(image)
    mutated[diskpos] = 0xb6
    return bytes(mutated)


def fault_qa(args):
    """Reuse a digest-bound passing ROM fixture, then test ONE opcode mutation."""
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=False)
    digest = lambda p: hashlib.sha256(Path(p).read_bytes()).hexdigest()
    original = args.source_image.read_bytes()
    recipe = json.loads(args.build_report.read_text())
    passed = json.loads(args.passing_integration.read_text())
    results.need(passed['status'] == 'PASS_SOFTWARE_INTEGRATION_ONLY' and passed['case'] == 'fixed'
                 and passed['input_sha256'] == digest(args.source_image)
                 and passed['worker_sha256'] == digest(args.rom_worker)
                 and passed['recipe_sha256'] == digest(args.build_report), 'M140_FIXED_FAULT_PASSING_IDENTITY')
    for name, value in passed['contracts'].items():
        results.need(digest(build.HERE/name) == value, 'M140_FIXED_FAULT_PASSING_IDENTITY')
    results.need({p.name: digest(p) for p in args.roms.iterdir() if p.is_file()} == passed['rom_files'],
                 'M140_FIXED_FAULT_PASSING_IDENTITY')
    mutated = fault_image(original, recipe['patch_check_file_offset'])
    results.need(sum(a != b for a, b in zip(original, mutated)) == 1, 'M140_FIXED_FAULT_MUTATION_COUNT')
    disk = out/'fault.d88'
    disk.write_bytes(mutated)
    home = out/'home'
    home.mkdir()
    cfg = out/'config.ini'
    positive_command = passed['command']
    cfg.write_bytes(Path(positive_command[positive_command.index('--cfg')+1]).read_bytes())
    script = out/'input.txt'
    script.write_text('@wait 1600\n@text FDCFIX\n@enter\n')
    tvram = out/'tvram.bin'
    cmd = list(positive_command)
    for flag, value in {'--fdd1':disk, '--cfg':cfg, '--roms':args.roms.resolve(),
                        '--headless-input-script':script, '--screenshot':f'6000:{out/"final.png"}',
                        '--screen-tvram-dump':tvram}.items():
        cmd[cmd.index(flag)+1] = str(value)
    cmd[0] = str(args.rom_worker.resolve())
    env = dict(os.environ, HOME=str(home), XDG_CONFIG_HOME=str(home),
               SDL_VIDEODRIVER='dummy', SDL_AUDIODRIVER='dummy')
    report = {'status':'NOT_RUN', 'command':cmd, 'passing_fixture_sha256':digest(args.passing_integration),
              'worker_sha256':digest(args.rom_worker), 'recipe_sha256':digest(args.build_report),
              'fixture_sha256':hashlib.sha256(mutated).hexdigest(), 'single_mutation':'CP (HL) -> OR (HL)',
              'contract_sha256':digest(__file__), 'target_policy':'SDL dummy software fault safety, not silicon/physical halt-state readout'}
    try:
        run = subprocess.run(cmd, env=env, capture_output=True, timeout=240)
        (out/'stdout.log').write_bytes(run.stdout)
        (out/'stderr.log').write_bytes(run.stderr)
        report['exit_status'] = run.returncode
        results.need(run.returncode == 0, 'M140_FIXED_FAULT_WORKER_EXIT')
        rows = results.cpm_screen(tvram.read_bytes())
        (out/'screen-rows.json').write_text(json.dumps(rows, indent=2)+'\n')
        results.need(any('M140_FIXED_PATCH_VERIFY' in r for r in rows), 'M140_FIXED_FAULT_MESSAGE')
        results.need(disk.read_bytes() == mutated, 'M140_FIXED_FAULT_DISK_CHANGED')
        results.need(args.source_image.read_bytes() == original, 'M140_FIXED_FAULT_SOURCE_CHANGED')
        report['status'] = 'PASS_SOFTWARE_FAULT_SAFETY_ONLY'
    except results.RecordError as exc:
        report.update(status='FAIL', code=exc.code)
    except subprocess.TimeoutExpired:
        report.update(status='FAIL', code='M140_FIXED_FAULT_WORKER_TIMEOUT')
    finally:
        report['outputs_sha256'] = {p.name:digest(p) for p in out.iterdir() if p.is_file()}
        (out/'run.json').write_text(json.dumps(report, indent=2)+'\n')
    print(report['status'], report.get('code',''))
    if report['status'] != 'PASS_SOFTWARE_FAULT_SAFETY_ONLY':raise SystemExit(1)


def payloads(runner, assembler):
    with tempfile.TemporaryDirectory(prefix='m140-fixed-') as td:
        work = Path(td)
        fixed.assemble(assembler, work)
        sym = json.loads((work/'fdcfix-symbols.json').read_text())
        image = (work/'fdcfix.bin').read_bytes()
        expected = b'\xf3\x3e\x00\xaf\xc5\xf1\x00\xf5\xd1\xc9'
        pos = sym['sampler']-0x4000
        assert image[pos:pos+10] == expected
        for callsite in ('control_call', 'measured_call'):
            at = sym[callsite]-0x4000
            assert image[at:at+3] == b'\xcd'+struct.pack('<H', sym['sampler'])
        dump = work/'ram.bin'
        run = subprocess.run([runner, str(work/'fdcfix.bin'), str(dump)], capture_output=True, timeout=30)
        assert run.returncode == 0, run.stderr
        ram = dump.read_bytes()
        assert len(ram) == 0x3f00
        fixed.content(run.stdout+ram[0x3000:0x3000+fixed.RAW_SIZE], recovery=False, host_model=True)
        assert ram[pos:pos+10] == b'\xf3\x3e\x00\xaf\xc5\xf1\x37\xf5\xd1\xc9'
        # One opcode mutation forces the seed-patch verification failure.
        # Error byte 03 identifies this path; an unrelated worker failure cannot pass.
        bad = bytearray(image)
        at = sym['patch_check']-0x4000
        assert bad[at] == 0xbe
        bad[at] = 0xb6
        fault = work/'fault.bin'
        fault.write_bytes(bad)
        run = subprocess.run([runner, str(fault)], capture_output=True, timeout=10)
        assert run.returncode == 1 and run.stdout == b'\x03', (run.stdout, run.stderr)
        assert run.stderr.split()[0] == b'M139_FDC_HOST_EXEC_FAILED', run.stderr
    print('M140_FIXED_PAYLOAD_PASS (mapped software, not physical Q)')


if __name__ == '__main__':
    p = argparse.ArgumentParser()
    p.add_argument('--runner')
    p.add_argument('--assembler')
    p.add_argument('--rom-worker', type=Path)
    p.add_argument('--roms', type=Path)
    p.add_argument('--source-image', type=Path)
    p.add_argument('--build-report', type=Path)
    p.add_argument('--passing-integration', type=Path)
    p.add_argument('--output', type=Path)
    args, remaining = p.parse_known_args()
    if args.rom_worker:
        if not all((args.roms, args.source_image, args.build_report, args.passing_integration, args.output)):
            p.error('ROM fault QA requires explicit input/output bindings and a passing integration manifest')
        fault_qa(args)
    elif args.runner:payloads(args.runner, args.assembler)
    else:unittest.main(argv=['test_fixed', *remaining])
