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

"""Mapped X11 speed capture and separately callable local OCR/content validation.

Capture on Linux with Xvfb/Openbox or physical X11. Validate copied, hash-bound
captures locally with the macOS ocr_vision.m helper (Foundation/Vision frameworks).
No screenshots or ROM identities are uploaded. Keep every output outside Git.
This is NOT frozen-reference approval, physical-GPU/Cocoa/Windows/HiDPI coverage,
absolute performance qualification, or observation of the first post-load sample.
"""
import argparse
import copy
import json
from pathlib import Path
import re
import shutil
import subprocess
import time
import unicodedata

from screen_compare import QaError, require, read_png
from x11_capture import digest
from x11_gate import Client

ROOT = Path(__file__).resolve().parents[2]
CONTRACT = 'm143-runtime-speed-x11-v2'
CASES = ('normal', 'held', 'released', 'nowait', 'restored', 'paused', 'resumed',
         'narrow', 'loaded', 'reset')
NUMBER = r'([0-9]+(?:\.[0-9]+)?)'


def tooltip_metrics(text, paused=False):
    """Content-only validation; never infer digits from the desired result."""
    text = re.sub(r'\s+', '', unicodedata.normalize('NFKC', text))
    fields = {}
    for key, label in (('cpu', 'CPU'), ('base', 'Base'), ('sgp', 'SGP'),
                       ('draws', 'Draws'), ('frames', 'Guestframes')):
        unit = 'FPS' if key in ('draws', 'frames') else 'MHz'
        found = re.findall(label + ':' + NUMBER + unit, text, re.I)
        require(len(found) == 1, 'M143_SPEED_OCR_FIELDS')
        fields[key] = float(found[0])
    if paused:
        require('実行速度:一時停止' in text, 'M143_SPEED_PAUSED_TEXT')
        fields['paused'] = True
    else:
        # Vision can classify the closing parenthesis as a CJK bracket.
        # Accept that delimiter only; never repair or infer numeric characters.
        speed = re.findall(r'実行速度:([0-9]+\.[0-9]{2})倍\(([0-9]+)%[)】]', text)
        equivalent = re.findall('実効クロック換算:約' + NUMBER + 'MHz相当', text)
        require(len(speed) == 1 and len(equivalent) == 1, 'M143_SPEED_OCR_MEASUREMENT')
        fields.update(speed=float(speed[0][0]), equivalent=float(equivalent[0]))
        require(abs(fields['speed'] * 100 - int(speed[0][1])) <= 1,
                'M143_SPEED_PERCENT')
        require(abs(fields['equivalent'] - fields['cpu'] * fields['speed']) <= .06,
                'M143_SPEED_EQUIVALENT')
    fields['nowait'] = '早送り:NoWait' in text
    return fields


def validate_samples(samples):
    """Independent semantic layer; no Git history or real evidence dependency."""
    require(set(samples) == set(CASES), 'M143_SPEED_CASE_SET')
    normal = samples['normal']
    require(all(abs(normal[key] - 7.9872) < .000001 for key in ('cpu', 'base', 'sgp')),
            'M143_SPEED_BASELINE_CLOCK')
    for value in samples.values():
        require(all(value[key] == normal[key] for key in ('cpu', 'base', 'sgp')),
                'M143_SPEED_NOMINAL_CLOCK')
    for key in ('normal', 'released', 'restored', 'resumed', 'loaded', 'reset'):
        require(not samples[key]['nowait'], 'M143_SPEED_PACED_MODE')
        require(.75 <= samples[key]['speed'] <= 1.25, 'M143_SPEED_PACED_RETURN')
    require(samples['held']['speed'] > normal['speed'] * 1.15, 'M143_SPEED_ACCELERATION')
    # No Wait is not a promise that a presenter-limited host can exceed 1x.
    require(samples['nowait']['nowait'], 'M143_SPEED_NOWAIT_TEXT')
    for name, value in samples.items():
        if name != 'paused':
            require(value['frames'] > 0 and value['draws'] > 0, 'M143_SPEED_PROGRESS')
    for key in ('held', 'narrow'):
        value = samples[key]
        require(14.5 <= value['frames'] / value['draws'] <= 17.5, 'M143_SPEED_F11_DRAW_RATIO')
    require(.95 <= samples['nowait']['frames'] / samples['nowait']['draws'] <= 1.05,
            'M143_SPEED_NOWAIT_DRAW_RATIO')
    paused = samples['paused']
    require(paused.get('paused') is True and paused['frames'] == 0 and paused['draws'] == 0,
            'M143_SPEED_PAUSE')


def new_output(path):
    path = path.resolve()
    require(not path.exists() and path != ROOT and ROOT not in path.parents, 'M143_SPEED_OUTPUT_NOT_NEW')
    path.mkdir(parents=True)
    return path


def snapshot_tooltip(client, name):
    client.hover(300, 250)
    client.snapshot(name + '-before')
    width, height, _ = read_png((client.output / (name + '-before.png')).read_bytes())
    panel = 14 * max(1, (width + 320) // 640)
    client.hover(100, height - panel - 12)
    client.snapshot(name)
    return client.output / (name + '.png')


def capture(args):
    require(re.fullmatch('[0-9a-f]{40}', args.evaluated_commit) is not None and
            args.evaluated_commit.encode() in args.worker.read_bytes(), 'M143_SPEED_WORKER_IDENTITY')
    require(args.display and args.display_kind in ('virtual-x11', 'physical-x11'),
            'M143_SPEED_DISPLAY_REQUIRED')
    output = new_output(args.output_dir)
    run = output / 'speed'
    (run / 'bin').mkdir(parents=True)
    shutil.copy2(args.worker, run / 'bin/vaeg')
    shutil.copyfile(args.backup_seed, run / 'backup.bin')
    config = ('[NekoProjectII]\nMain_RAM=640\nMain_RAM_Auto=false\nBootMode=1\nCPU_Speed=100\n'
              'clk_mult=2\nV1V2_Standard=false\nDspClock=15\nF12_bind=8\nSkpFrame=1\n'
              'NativeCRT=false\nMachineStatus=1\nFrontPanelMode=1\nGUI_win_cx=960\n'
              'GUI_win_cy=640\nGUI_ui_scale=100\ns_NOWAIT=false\n')
    (run / 'config.ini').write_text(config)
    (run / 'config.initial.ini').write_text(config)
    args.probe_disk = None
    report = {'contract': CONTRACT, 'status': 'INCOMPLETE', 'gate_pass': False,
              'evaluated_commit': args.evaluated_commit, 'worker_sha256': digest(args.worker),
              'runner_sha256': digest(__file__), 'client_adapter_sha256': digest(Path(__file__).with_name('x11_gate.py')),
              'display_kind': args.display_kind, 'backup_seed_sha256': digest(args.backup_seed),
              'rom_files': {str(p.relative_to(args.roms)): digest(p)
                            for p in sorted(args.roms.rglob('*.rom'))}, 'cases': {}}
    client = None
    def take(name):
        time.sleep(2.4)
        path = snapshot_tooltip(client, name)
        report['cases'][name] = str(path.relative_to(output))
    try:
        client = Client(args, run, resume=True)
        client.line('CLS:PRINT "RUNTIME SPEED QA"')
        take('normal')
        client.xdo('keydown', 'F11')
        take('held')
        client.xdo('keyup', 'F11')
        take('released')
        client.click(20, 10)
        client.xdo('keydown', 'ctrl')
        client.click(90, 123)
        client.xdo('keyup', 'ctrl')
        client.xdo('key', 'ctrl+a')
        client.xdo('type', '410')
        client.key('Return')
        client.key('Escape')
        take('nowait')
        client.click(20, 10)
        client.click(90, 147)
        client.key('Escape')
        take('restored')
        client.key('F12')
        take('paused')
        client.key('F12')
        take('resumed')
        client.xdo('windowsize', client.window, '380', '480')
        client.xdo('keydown', 'F11')
        take('narrow')
        client.xdo('keyup', 'F11')
        (run / 'events.json').write_text(json.dumps(client.events, indent=2) + '\n')
        client.close()
        client = None
        # A separate production profile assigns F12 to quick-save/load.
        client = Client(args, output / 'state')
        client.line('CLS:PRINT "SAVEDSTATE=12345"')
        client.save('checkpoint')
        client.line('CLS:PRINT "CHANGEDSTATE=67890"')
        require('CHANGEDSTATE=67890' in client.text('changed'), 'M143_SPEED_STATE_FIXTURE')
        client.key('shift+F12')
        loaded = client.wait_text('loaded-text', r'^SAVEDSTATE=12345\s*$', 20)
        require('CHANGEDSTATE=67890' not in loaded, 'M143_SPEED_STATE_RESTORE')
        take('loaded')
        client.click(20, 10)
        client.click(80, 38)
        client.wait_text('reset-boot', r'How many files', 60)
        client.key('Return')
        client.wait_text('reset-ready', r'^Ok\s*$', 15)
        take('reset')
        report.update(status='CAPTURED_UNREVIEWED', state_guest_restore=True, reset_guest_ready=True)
    except (QaError, OSError, ValueError, subprocess.SubprocessError) as error:
        report.update(status='FAIL', error_code=getattr(error, 'code', 'M143_SPEED_EXECUTION'), detail=str(error))
    finally:
        if client:
            (client.output / 'events.json').write_text(json.dumps(client.events, indent=2) + '\n')
            try:
                client.xdo('keyup', 'F11', 'ctrl')
                client.close()
            except (QaError, OSError, subprocess.SubprocessError) as error:
                report.update(status='FAIL', error_code='M143_SPEED_CLOSE', detail=str(error))
                if client.process.poll() is None:
                    client.process.kill()
                    client.process.wait()
        report['output_digests'] = {str(p.relative_to(output)): digest(p)
                                    for p in output.rglob('*') if p.is_file()}
        (output / 'capture.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({key: report.get(key) for key in ('status', 'error_code', 'cases')}))
    return 0 if report['status'] == 'CAPTURED_UNREVIEWED' else 1


def validate_capture_content(root, report):
    require(report.get('contract') == CONTRACT and report.get('status') == 'CAPTURED_UNREVIEWED',
            'M143_SPEED_CAPTURE_INCOMPLETE')
    for key, size in (('worker_sha256', 64), ('evaluated_commit', 40),
                      ('runner_sha256', 64), ('client_adapter_sha256', 64), ('backup_seed_sha256', 64)):
        value = report.get(key)
        require(isinstance(value, str) and re.fullmatch('[0-9a-f]{' + str(size) + '}', value) is not None,
                'M143_SPEED_IDENTITY_SCHEMA')
    require(report.get('display_kind') in ('virtual-x11', 'physical-x11'), 'M143_SPEED_TARGET')
    require(set(report['cases']) == set(CASES), 'M143_SPEED_CASE_SET')
    require(report.get('state_guest_restore') is True and report.get('reset_guest_ready') is True,
            'M143_SPEED_GUEST_TRANSITIONS')
    for name, expected in report['output_digests'].items():
        path = (root / name).resolve()
        require(root in path.parents and digest(path) == expected, 'M143_SPEED_CAPTURE_BINDING')
    require(all(name in report['output_digests'] for name in report['cases'].values()),
            'M143_SPEED_CAPTURE_BINDING')
    for name in ('speed/bin/vaeg', 'state/bin/vaeg'):
        require(name in report['output_digests'] and digest(root / name) == report['worker_sha256'] and
                report['evaluated_commit'].encode() in (root / name).read_bytes(),
                'M143_SPEED_WORKER_BINDING')


def validate(args):
    root = args.capture_dir.resolve()
    require(digest(root / 'capture.json') == args.capture_sha256, 'M143_SPEED_MANIFEST_BINDING')
    report = json.loads((root / 'capture.json').read_text())
    validate_capture_content(root, report)
    output = new_output(args.output_dir)
    result = {'contract': CONTRACT, 'status': 'INCOMPLETE', 'gate_pass': False,
              'capture_sha256': args.capture_sha256, 'validator_sha256': digest(__file__),
              'ocr_worker_sha256': digest(args.ocr_worker), 'cases': {}, 'samples': {},
              'not_covered': ['approved frozen visual references', 'first post-reset/load sample',
                              'Cocoa', 'Windows', 'physical GPU', 'true HiDPI', 'absolute performance']}
    for name in CASES:
        command = [str(args.ocr_worker.resolve()), str(root / report['cases'][name])]
        completed = subprocess.run(command, capture_output=True, timeout=60)
        (output / (name + '.ocr.json')).write_bytes(completed.stdout)
        (output / (name + '.stderr')).write_bytes(completed.stderr)
        row = {'command': command, 'exit': completed.returncode}
        try:
            require(completed.returncode == 0, 'M143_SPEED_OCR_WORKER')
            optical = json.loads(completed.stdout)
            text = '\n'.join(entry['text'] for entry in optical['rows'])
            (output / (name + '.txt')).write_text(text + '\n')
            result['samples'][name] = tooltip_metrics(text, name == 'paused')
            row['status'] = 'PASS_TEXT'
        except (QaError, ValueError, KeyError, TypeError) as error:
            row.update(status='FAIL', error_code=getattr(error, 'code', 'M143_SPEED_OCR_SCHEMA'))
        result['cases'][name] = row
    try:
        require(all(row['status'] == 'PASS_TEXT' for row in result['cases'].values()),
                'M143_SPEED_TEXT_INCOMPLETE')
        validate_samples(result['samples'])
        result['status'] = 'PASS_TEXT_AND_INTERACTION_ONLY'
    except QaError as error:
        result.update(status='FAIL', error_code=error.code)
    result['output_digests'] = {p.name: digest(p) for p in output.iterdir() if p.is_file()}
    (output / 'result.json').write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps({key: result.get(key) for key in ('status', 'error_code', 'samples')}))
    return 0 if result['status'] == 'PASS_TEXT_AND_INTERACTION_ONLY' else 1


def selftest():
    text = ('CPU: 7.987200 MHz (設定)\nBase: 7.987200 MHz\nSGP: 7.987200 MHz\n'
            '実行速度：1.00倍（100%）\n実効クロック換算：約7.99MHz相当\n'
            'Draws: 56.4 FPS\nGuest frames: 56.4 FPS\n')
    normal = tooltip_metrics(text)
    require(tooltip_metrics(text.replace('）', '】')) == normal, 'M143_SPEED_SELFTEST')
    samples = {name: copy.deepcopy(normal) for name in CASES}
    for key in ('held', 'narrow'):
        samples[key].update(speed=8., equivalent=63.9, frames=800., draws=50.)
    samples['nowait']['nowait'] = True
    samples['paused'].update(paused=True, frames=0., draws=0.)
    validate_samples(samples)
    tooltip_metrics(text.replace('実行速度：1.00倍（100%）', '実行速度：一時停止'), True)
    negatives = [
        (lambda: tooltip_metrics(text.replace('CPU:', 'Missing:')), 'M143_SPEED_OCR_FIELDS'),
        (lambda: tooltip_metrics(text.replace('1.00倍', '不明')), 'M143_SPEED_OCR_MEASUREMENT'),
        (lambda: tooltip_metrics(text.replace('約7.99', '約9.99')), 'M143_SPEED_EQUIVALENT'),
        (lambda: tooltip_metrics(text.replace('（100%）', '（200%）')), 'M143_SPEED_PERCENT'),
        (lambda: tooltip_metrics(text, True), 'M143_SPEED_PAUSED_TEXT'),
    ]
    for case, field, value, code in (
        ('normal', 'cpu', 9., 'M143_SPEED_BASELINE_CLOCK'),
        ('held', 'cpu', 9., 'M143_SPEED_NOMINAL_CLOCK'),
        ('released', 'speed', 8., 'M143_SPEED_PACED_RETURN'),
        ('restored', 'nowait', True, 'M143_SPEED_PACED_MODE'),
        ('held', 'speed', 1., 'M143_SPEED_ACCELERATION'),
        ('held', 'draws', 800., 'M143_SPEED_F11_DRAW_RATIO'),
        ('nowait', 'nowait', False, 'M143_SPEED_NOWAIT_TEXT'),
        ('nowait', 'frames', 0., 'M143_SPEED_PROGRESS'),
        ('nowait', 'draws', 5., 'M143_SPEED_NOWAIT_DRAW_RATIO'),
        ('paused', 'paused', False, 'M143_SPEED_PAUSE'),
        ('paused', 'frames', 1., 'M143_SPEED_PAUSE'),
    ):
        changed = copy.deepcopy(samples)
        changed[case][field] = value
        negatives.append((lambda changed=changed: validate_samples(changed), code))
    for action, expected in negatives:
        validate_samples(samples)
        tooltip_metrics(text)
        try:
            action()
        except QaError as error:
            require(error.code == expected, 'M143_SPEED_SELFTEST_WRONG_ERROR')
        else:
            raise QaError('M143_SPEED_SELFTEST_ACCEPTED_MUTATION')
    # The evidence-content layer gets a tiny independent fixture, not a real
    # capture corpus or the repository's current Git history.
    import tempfile
    from screen_compare import png_bytes
    with tempfile.TemporaryDirectory(prefix='m143-speed-fixture-') as directory:
        root = Path(directory).resolve()
        for profile in ('speed', 'state'):
            (root / profile / 'bin').mkdir(parents=True)
            (root / profile / 'bin/vaeg').write_bytes(b'fixture worker ' + b'1' * 40)
        mapping = {name: name + '.png' for name in CASES}
        for name in mapping.values():
            (root / name).write_bytes(png_bytes((1, 1, bytes((0, 0, 0, 255)))))
        evidence = {'contract': CONTRACT, 'status': 'CAPTURED_UNREVIEWED', 'cases': mapping,
                    'worker_sha256': digest(root / 'speed/bin/vaeg'), 'evaluated_commit': '1' * 40,
                    'runner_sha256': '2' * 64, 'client_adapter_sha256': '3' * 64,
                    'backup_seed_sha256': '4' * 64, 'display_kind': 'virtual-x11',
                    'state_guest_restore': True, 'reset_guest_ready': True,
                    'output_digests': {str(p.relative_to(root)): digest(p)
                                       for p in root.rglob('*') if p.is_file()}}
        mutations = [
            ({'worker_sha256': '5' * 64}, 'M143_SPEED_WORKER_BINDING'),
            ({'evaluated_commit': 'bad'}, 'M143_SPEED_IDENTITY_SCHEMA'),
            ({'display_kind': 'dummy'}, 'M143_SPEED_TARGET'),
            ({'state_guest_restore': False}, 'M143_SPEED_GUEST_TRANSITIONS'),
            ({'cases': {k: v for k, v in mapping.items() if k != 'held'}}, 'M143_SPEED_CASE_SET'),
            ({'output_digests': {**evidence['output_digests'], 'normal.png': '6' * 64}},
             'M143_SPEED_CAPTURE_BINDING'),
            ({'output_digests': {**evidence['output_digests'], '../escape': '6' * 64}},
             'M143_SPEED_CAPTURE_BINDING'),
        ]
        for mutation, expected in mutations:
            validate_capture_content(root, evidence)
            try:
                validate_capture_content(root, {**evidence, **mutation})
            except QaError as error:
                require(error.code == expected, 'M143_SPEED_SELFTEST_WRONG_ERROR')
            else:
                raise QaError('M143_SPEED_SELFTEST_ACCEPTED_MUTATION')
        validate_capture_content(root, evidence)
        validate_samples(samples)
    print('PASS: content composition; 16 text/semantic and 7 evidence single-mutation negatives')
    return 0


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('action', choices=('capture', 'validate', 'selftest'))
    for name in ('worker', 'roms', 'backup-seed', 'output-dir', 'capture-dir', 'ocr-worker'):
        parser.add_argument('--' + name, type=Path)
    for name in ('evaluated-commit', 'display', 'capture-sha256'):
        parser.add_argument('--' + name)
    parser.add_argument('--display-kind', choices=('virtual-x11', 'physical-x11'))
    args = parser.parse_args()
    try:
        if args.action == 'selftest':
            return selftest()
        require(args.output_dir is not None, 'M143_SPEED_ARGUMENTS')
        if args.action == 'validate':
            require(all((args.capture_dir, args.capture_sha256, args.ocr_worker)), 'M143_SPEED_ARGUMENTS')
            return validate(args)
        require(all((args.worker, args.roms, args.backup_seed, args.evaluated_commit)), 'M143_SPEED_ARGUMENTS')
        return capture(args)
    except (QaError, OSError, ValueError, KeyError, subprocess.SubprocessError) as error:
        print(json.dumps({'status': 'BLOCKED', 'error_code': getattr(error, 'code', 'M143_SPEED_EXECUTION')}))
        return 1


if __name__ == '__main__':
    raise SystemExit(main())
