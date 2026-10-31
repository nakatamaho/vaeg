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
# OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
# ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

"""Private mapped-X11 PC-8801mkIISR 4MHz clipboard-paste gate contract.

Requires local extra-machine ROMs. No VA-compatible, universal machine,
physical-GPU or approved-layout claim. Generated evidence must stay private.
"""
import argparse
import json
from pathlib import Path
import re
import shutil
import subprocess
import sys

from screen_compare import QaError, require
from x11_capture import digest
from x11_gate import Client, read_clock, check_text

ROOT = Path(__file__).resolve().parents[2]
PROGRAM = '10 A=100:B=3000:PRINT "PASTE142=";A+B\n20 PRINT "REPEAT142=";111+222\nRUN\n'


def validate_machine(state, clock):
    require(state.get('MCPUROLE') == b'\x01', 'M142_8801_ROLE')
    require(clock == (2, 100) and len(state.get('SYSPORTVA', b'')) > 0 and
            not state['SYSPORTVA'][0] & 32, 'M142_8801_CLOCK')


def validate_text(text):
    check_text(text, r'^PASTE142=\s*3100\s*$', 'M142_8801_SUM')
    check_text(text, r'^REPEAT142=\s*333\s*$', 'M142_8801_REPEAT')


def selftest():
    state = {'MCPUROLE': b'\x01', 'SYSPORTVA': b'\0'}
    text = 'PASTE142= 3100\nREPEAT142= 333\n'
    for action, code in (
        (lambda: validate_machine({**state, 'MCPUROLE': b'\0'}, (2, 100)), 'M142_8801_ROLE'),
        (lambda: validate_machine(state, (1, 100)), 'M142_8801_CLOCK'),
        (lambda: validate_machine({**state, 'SYSPORTVA': b'\x20'}, (2, 100)), 'M142_8801_CLOCK'),
        (lambda: validate_text(text.replace('3100', '310')), 'M142_8801_SUM'),
        (lambda: validate_text(text.replace('333', '33')), 'M142_8801_REPEAT')):
        validate_machine(state, (2, 100))
        validate_text(text)
        try:
            action()
        except QaError as error:
            require(error.code == code, 'M142_8801_SELFTEST_MASKED')
        else:
            raise QaError('M142_8801_SELFTEST_ACCEPTED')
    print('pc8801-paste: PASS (5 exact-code single-mutation negatives)')


def run(args):
    for key in ('worker', 'roms', 'backup_seed', 'output_dir'):
        setattr(args, key, getattr(args, key).resolve())
    out = args.output_dir
    require(not out.exists() and out != ROOT and ROOT not in out.parents, 'M142_8801_OUTPUT')
    require(re.fullmatch('[0-9a-f]{40}', args.evaluated_commit) and
            args.evaluated_commit.encode() in args.worker.read_bytes(), 'M142_8801_WORKER')
    retained = {p: digest(p) for p in [args.worker, args.backup_seed, *args.roms.rglob('*')] if p.is_file()}
    (out / 'bin').mkdir(parents=True)
    shutil.copy2(args.worker, out / 'bin/vaeg')
    shutil.copyfile(args.backup_seed, out / 'backup.bin')
    config = ('[NekoProjectII]\nMain_RAM=640\nMain_RAM_Auto=false\nBootMode=1\n'
              'CPU_Speed=100\nclk_mult=2\nExtraMachine=PC-8801mkIISR\nV1V2_Standard=true\n'
              'DspClock=5\nF12_bind=9\nNativeCRT=false\nMachineStatus=1\nFrontPanelMode=1\n'
              'GUI_win_cx=960\nGUI_win_cy=640\nGUI_ui_scale=100\n')
    (out / 'config.ini').write_text(config)
    (out / 'config.initial.ini').write_text(config)
    (out / 'program.txt').write_text(PROGRAM)
    args.probe_disk = None
    report = {'contract': 'm142-pc8801-paste-v1', 'status': 'INCOMPLETE', 'gate_pass': False,
              'worker_source': args.evaluated_commit, 'worker_sha256': digest(args.worker),
              'runner_sha256': digest(__file__),
              'adapter_sha256': digest(Path(sys.modules[Client.__module__].__file__)),
              'roms': {p.relative_to(args.roms).as_posix(): digest(p) for p in args.roms.rglob('*') if p.is_file()},
              'backup_seed_sha256': digest(args.backup_seed), 'display': args.display,
              'scope': 'mapped virtual X11; PC-8801mkIISR 4MHz, not VA/VA2 or layout approval'}
    client = None
    try:
        client = Client(args, out, resume=True)
        report['argv'] = client.argv
        validate_machine(client.save('before'), read_clock(client.process.pid, client.worker))
        client.start_paste(PROGRAM)
        client.finish_paste(PROGRAM)
        validate_text(client.text('paste-result'))
        client.snapshot('paste-result')
        validate_machine(client.save('after'), read_clock(client.process.pid, client.worker))
        client.line('PRINT "AFTER142=";A+B')
        check_text(client.text('after-result'), r'^AFTER142=\s*3100\s*$', 'M142_8801_CONTINUE')
        report['status'] = 'PASS_PC8801_4MHZ_PASTE_GUEST_RESULTS_NOT_VA_COMPAT_OR_LAYOUT'
    except (QaError, OSError, ValueError, KeyError, subprocess.SubprocessError) as error:
        report.update(status='FAIL', error=getattr(error, 'code', 'M142_8801_EXECUTION'), detail=str(error))
    finally:
        if client:
            (out / 'events.json').write_text(json.dumps(client.events, indent=2) + '\n')
            client.close()
            report['worker_exit'] = client.process.returncode
        require(retained == {p: digest(p) for p in retained}, 'M142_8801_INPUT_CHANGED')
        report['outputs'] = {p.relative_to(out).as_posix(): digest(p) for p in out.rglob('*') if p.is_file()}
        (out / 'result.json').write_text(json.dumps(report, indent=2) + '\n')
    print(report['status'])
    return 0 if report['status'].startswith('PASS_') else 1


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--selftest', action='store_true')
    for name in ('worker', 'roms', 'backup-seed', 'output-dir'):
        parser.add_argument('--' + name, type=Path)
    parser.add_argument('--evaluated-commit')
    parser.add_argument('--display')
    args = parser.parse_args()
    if args.selftest:
        selftest()
        return 0
    require(all((args.worker, args.roms, args.backup_seed, args.output_dir, args.evaluated_commit, args.display)),
            'M142_8801_ARGUMENTS')
    return run(args)


if __name__ == '__main__':
    sys.exit(main())
