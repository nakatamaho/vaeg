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

"""Actual mapped Linux/X11 M142 clock/reset/restart and GUI-SAV contracts.

Uses production menus, paced mapped keyboard input and quick-save/load, not dummy video or
configuration-only simulations. Footer OCR is a text contract, NOT approval of
pixel references, clipping/fullscreen/HiDPI, absolute timing or physical silicon.
Keep all generated captures, files, complete settings and ROM identities private.
"""
import argparse
import json
import os
from pathlib import Path
import re
import struct
import subprocess
import sys
import time

from screen_compare import QaError, require, read_png, png_bytes
from x11_gate import Client, read_clock, check_text
from x11_capture import digest

ROOT = Path(__file__).resolve().parents[2]
CHOICES = (4, 8, 10, 12, 20)


def clock_setting(mhz, standard):
    require(mhz in CHOICES, 'M142_CLOCK_CHOICE')
    if standard:
        return mhz // 2, 100
    return {4: (1, 100), 8: (2, 100), 10: (5, 50), 12: (3, 100), 20: (5, 100)}[mhz]


def validate_latches(state, model, standard):
    core, port = state.get('PCCORE', b''), state.get('SYSPORTVA', b'')
    require(len(core) == 32 and len(port) > 0, 'M142_CLOCK_STATE_SCHEMA')
    require(core[28] == (2 if model == 'va2' else 1), 'M142_CLOCK_MODEL')
    require(bool(port[0] & 32) != standard, 'M142_CLOCK_HS')
    require(struct.unpack_from('<II', core) == (3993600, 2) and
            struct.unpack_from('<I', core, 24)[0] == 7987200, 'M142_CLOCK_BOARD_DOMAIN')


def pit_configuration(state):
    payload = state.get('PIT', b'')
    require(len(payload) == 40, 'M142_CLOCK_PIT_SCHEMA')
    # Exclude running count/latch/status/read-write phase, not reload settings.
    return [(payload[i], struct.unpack_from('<H', payload, i + 4)[0]) for i in range(0, 40, 8)]


def validate_status(text, mhz, sgp_mhz):
    compact = re.sub(r'\s', '', text)
    require(f'CPU{mhz}MHz' in compact, 'M142_CLOCK_RENDERED_CPU')
    require(f'SGP{sgp_mhz}MHz' in compact, 'M142_CLOCK_RENDERED_SGP')


def footer(client, name):
    client.hover(950, 350)
    client.snapshot(name)
    w, h, pixels = read_png((client.output / (name + '.png')).read_bytes())
    top = h - 24 - 14 * max(1, (w + 320) // 640)
    scaled = b''.join(b''.join(pixels[((top + y) * w + x) * 4:((top + y) * w + x) * 4 + 4] * 4
                               for x in range(w)) * 4 for y in range(24))
    result = subprocess.run(['tesseract', 'stdin', 'stdout', '--psm', '7', '-c',
                             'tessedit_char_whitelist=0123456789PCVAHSMzUG|-FR.E'],
                            input=png_bytes((w * 4, 96, scaled)), capture_output=True, check=True, timeout=15)
    text = result.stdout.decode()
    (client.output / (name + '.ocr.txt')).write_text(text)
    return text


def select_clock(client, mhz):
    client.clock_menu()
    client.click(240, 194 + 20 * CHOICES.index(mhz))
    client.hover(950, 350)


def guest_line(client, text):
    # Deliberately independent of clipboard-paste pacing. A separate retained
    # paste campaign must cover that path, especially at reduced CPU speeds.
    # Hold Shift explicitly across both events. Separate xdotool keydown/keyup
    # calls for a shifted keysym do not provide an enduring Shift key hold.
    keysyms = {':': 'shift+semicolon', '"': 'shift+apostrophe', '=': 'equal', ';': 'semicolon',
               '*': 'shift+8', '+': 'shift+equal', ' ': 'space'}
    for char in text:
        key = 'shift+' + char.lower() if char.isupper() else keysyms.get(char, char)
        if key.startswith('shift+'):
            client.xdo('keydown', 'Shift_L')
            time.sleep(.15)
            client.key(key[6:], hold=.15)
            client.xdo('keyup', 'Shift_L')
            time.sleep(.15)
        else:
            client.key(key, hold=.15)
    client.key('Return')
    time.sleep(.3)


def selftest():
    import copy
    from unittest.mock import patch
    calls = []
    class Input:
        def xdo(self, *args):
            calls.append(args)
        def key(self, key, hold=.12):
            calls.append(('key', key, hold))
    with patch.object(time, 'sleep'):
        guest_line(Input(), 'A:1')
    require(calls == [('keydown', 'Shift_L'), ('key', 'a', .15), ('keyup', 'Shift_L'),
                      ('keydown', 'Shift_L'), ('key', 'semicolon', .15), ('keyup', 'Shift_L'),
                      ('key', '1', .15), ('key', 'Return', .12)],
            'M142_CLOCK_INPUT_API')
    core = bytearray(32)
    struct.pack_into('<II', core, 0, 3993600, 2)
    struct.pack_into('<I', core, 24, 7987200)
    core[28] = 2
    state = {'PCCORE': bytes(core), 'SYSPORTVA': b'\x20', 'PIT': bytes(40)}
    def positive():
        validate_latches(state, 'va2', False)
        require(len(pit_configuration(state)) == 5, 'M142_CLOCK_SELFTEST')
        validate_status('PC-88VA2 | V1H | CPU 20MHz | SGP 8MHz', 20, 8)
        require(clock_setting(10, False) == (5, 50) and clock_setting(10, True) == (5, 100),
                'M142_CLOCK_SELFTEST')
    def changed(offset, value):
        result = copy.deepcopy(state)
        data = bytearray(result['PCCORE']); data[offset] = value
        result['PCCORE'] = bytes(data)
        return result
    negatives = [
        (lambda: clock_setting(32, False), 'M142_CLOCK_CHOICE'),
        (lambda: validate_latches(changed(28, 1), 'va2', False), 'M142_CLOCK_MODEL'),
        (lambda: validate_latches({**state, 'SYSPORTVA': b'\0'}, 'va2', False), 'M142_CLOCK_HS'),
        (lambda: validate_latches(changed(4, 5), 'va2', False), 'M142_CLOCK_BOARD_DOMAIN'),
        (lambda: validate_latches(changed(24, 1), 'va2', False), 'M142_CLOCK_BOARD_DOMAIN'),
        (lambda: pit_configuration({**state, 'PIT': bytes(39)}), 'M142_CLOCK_PIT_SCHEMA'),
        (lambda: validate_status('CPU 4MHz | SGP 8MHz', 20, 8), 'M142_CLOCK_RENDERED_CPU'),
        (lambda: validate_status('CPU 20MHz | SGP 20MHz', 20, 8), 'M142_CLOCK_RENDERED_SGP')]
    for action, code in negatives:
        positive()
        try:
            action()
        except QaError as error:
            require(error.code == code, 'M142_CLOCK_SELFTEST_MASKED')
        else:
            raise QaError('M142_CLOCK_SELFTEST_ACCEPTED')
    print('runtime-clock-matrix: PASS (8 exact-code content negatives)')


def run(args):
    for key in ('worker', 'roms', 'backup_seed', 'output_dir'):
        setattr(args, key, getattr(args, key).resolve())
    require(args.evaluated_commit.encode() in args.worker.read_bytes(), 'M142_CLOCK_WORKER')
    out = args.output_dir
    require(not out.exists() and out != ROOT and ROOT not in out.parents, 'M142_CLOCK_OUTPUT')
    inputs = {p: digest(p) for p in [args.worker, args.backup_seed, *args.roms.rglob('*.rom')] if p.is_file()}
    args.probe_disk = None
    report = {'contract': 'm142-mapped-x11-clock-v1', 'status': 'INCOMPLETE', 'gate_pass': False,
              'worker_source': args.evaluated_commit, 'worker_sha256': digest(args.worker),
              'runner_sha256': digest(__file__), 'operations_adapter_sha256': digest(Path(sys.modules[Client.__module__].__file__)),
              'display_scope': 'mapped virtual X11 software / 96 DPI, not physical GPU or HiDPI',
              'arithmetic_input': 'mapped keyboard, held 150ms with 150ms release gap; NOT clipboard-paste coverage', 'cases': {}}
    client = None
    try:
        client = Client(args, out)
        for model in ('va', 'va2'):
            for version, standard in ((1, False), (1, True), (2, True), (2, False)):
                mode = f'{model}-v{version}{"s" if standard else "h"}'
                client.boot(model, version, standard, mode)
                initial = client.save(mode + '-baseline')
                reference_pit = pit_configuration(initial)
                for mhz in CHOICES:
                    name = f'{mode}-{mhz}mhz'
                    select_clock(client, mhz)
                    setting = read_clock(client.process.pid, client.worker)
                    require(setting == clock_setting(mhz, standard), 'M142_CLOCK_APPLIED_SETTING')
                    value = mhz * 11 + version + 100 * int(standard)
                    guest_line(client, f'CLS:PRINT "QAMHZ=";{mhz}*11+{version}+{100 * int(standard)}')
                    check_text(client.text(name + '-guest'), rf'^QAMHZ=\s*{value}\s*$', 'M142_CLOCK_GUEST_EXECUTION')
                    state = client.save(name)
                    validate_latches(state, model, standard)
                    require(pit_configuration(state) == reference_pit, 'M142_CLOCK_PIT_CONFIGURATION')
                    rendered = {'status': 'PASS'}
                    try:
                        validate_status(footer(client, name + '-footer'), mhz, 8 if model == 'va2' else 4)
                    except QaError as error:
                        if error.code not in ('M142_CLOCK_RENDERED_CPU', 'M142_CLOCK_RENDERED_SGP'):
                            raise
                        # Continue independent semantic cases, but keep the
                        # failed text contract and fail the aggregate below.
                        rendered = {'status': 'FAIL', 'error': error.code}
                    report['cases'][name] = {'status': rendered['status'], 'rendered_text': rendered,
                                             'execution_and_state': 'PASS', 'clock_setting': list(setting),
                                             'guest_result': value, 'board_hz': 7987200,
                                             'pit_configuration_unchanged': True, 'actual_saved_state': True}
                    (out / 'checkpoint.json').write_text(json.dumps(report, indent=2) + '\n')
                    print(name, report['cases'][name]['status'], flush=True)
        # Last case is VA2/V2H, 20MHz. Check actual process restart, not only INI bytes.
        client.close()
        client = Client(args, out, resume=True)
        client.identity(2, False, 'restarted')
        require(read_clock(client.process.pid, client.worker) == (5, 100), 'M142_CLOCK_RESTART')
        validate_status(footer(client, 'restarted-footer'), 20, 8)
        # A real GUI save followed by mutation/load must recover a guest variable.
        guest_line(client, 'Q=1421:PRINT "QASAVE=";Q')
        check_text(client.text('variable-saved'), r'^QASAVE=\s*1421\s*$', 'M142_CLOCK_GUEST_EXECUTION')
        before = client.save('variable-before')
        guest_line(client, 'Q=9876:PRINT "QAMUT=";Q')
        check_text(client.text('variable-mutated'), r'^QAMUT=\s*9876\s*$', 'M142_CLOCK_GUEST_EXECUTION')
        client.key('shift+F12')
        time.sleep(1)
        guest_line(client, 'PRINT "QAREST=";Q')
        check_text(client.text('variable-restored'), r'^QAREST=\s*1421\s*$', 'M142_CLOCK_STATE_VARIABLE')
        after = client.save('variable-after')
        require(before['PCCORE'] == after['PCCORE'] and before['SYSPORTVA'] == after['SYSPORTVA'] and
                before['MEM88MODE'] == after['MEM88MODE'], 'M142_CLOCK_STATE_LATCH')
        report['restart_and_gui_state'] = 'PASS_GUEST_VARIABLE_AND_MACHINE_LATCHES'
        require(all(case['status'] == 'PASS' for case in report['cases'].values()),
                'M142_CLOCK_RENDERED_CASES_FAILED')
        report['status'] = 'PASS_CLOCK_MENU_EXECUTION_PIT_CONFIG_SGP_TEXT_RESTART_GUI_STATE_NOT_LAYOUT_OR_FDC_TIMING'
    except (QaError, OSError, ValueError, KeyError, subprocess.SubprocessError) as error:
        report.update(status='FAIL', error=error.code if isinstance(error, QaError) else 'M142_CLOCK_EXECUTION', detail=str(error))
    finally:
        if client is not None:
            (out / 'events.json').write_text(json.dumps(client.events, indent=2) + '\n')
            client.close()
        if out.exists():
            require(inputs == {p: digest(p) for p in inputs}, 'M142_CLOCK_INPUT_CHANGED')
            report['outputs'] = {p.relative_to(out).as_posix(): digest(p) for p in out.rglob('*') if p.is_file()}
            (out / 'result.json').write_text(json.dumps(report, indent=2) + '\n')
    print(report['status'])
    return 1 if report['status'] == 'FAIL' else 0


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
            'M142_CLOCK_ARGUMENTS')
    return run(args)


if __name__ == '__main__':
    sys.exit(main())
