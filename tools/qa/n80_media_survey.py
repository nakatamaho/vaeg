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
# EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
# OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
# OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
# OF THE POSSIBILITY OF SUCH DAMAGE.
"""Local N80 guest-buffer survey with explicit, private input bindings.

Plan is a JSON list of {id, model, media, slot, input}; slot is fdd1 or tape,
input is headless-input-script text. IDs are neutral lowercase names. Media is
copied, never mounted writable at its source. Zero exit is UNASSESSED, not PASS.
A separate semantic contract or explicitly qualified observation is required.
All output, including paths, identities and captures, must remain outside Git.
"""
import argparse
from concurrent.futures import ThreadPoolExecutor
import copy
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess


class SurveyError(ValueError):
    pass


def require(condition, code):
    if not condition:
        raise SurveyError(code)


def validate_plan(plan):
    require(isinstance(plan, list) and 0 < len(plan) <= 64, 'N80_SURVEY_PLAN')
    seen = set()
    for case in plan:
        require(isinstance(case, dict) and set(case) == {'id', 'model', 'media', 'slot', 'input'},
                'N80_SURVEY_SCHEMA')
        require(isinstance(case['id'], str) and re.fullmatch(r'[a-z][a-z0-9-]{0,63}', case['id']),
                'N80_SURVEY_ID')
        require(case['id'] not in seen, 'N80_SURVEY_DUPLICATE')
        seen.add(case['id'])
        require(case['model'] in ('PC-8001', 'PC-8001mkII', 'PC-8001mkIISR'), 'N80_SURVEY_MODEL')
        require(case['slot'] in ('fdd1', 'tape'), 'N80_SURVEY_SLOT')
        require(isinstance(case['media'], str) and case['media'], 'N80_SURVEY_MEDIA')
        require(isinstance(case['input'], str), 'N80_SURVEY_INPUT')
    return plan


def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def validate_output_location(output, source):
    require(not output.is_relative_to(source), 'N80_SURVEY_OUTPUT_IN_TREE')


def validate_worker_digest(expected, actual):
    require(expected == actual, 'N80_SURVEY_WORKER_CHANGED')


def capture_schedule(final, checkpoints):
    require(len(checkpoints) == len(set(checkpoints)) and
            all(0 < frame < final for frame in checkpoints), 'N80_SURVEY_FRAMES')
    return sorted(checkpoints) + [final]


def run_case(args, case):
    validate_worker_digest(args.worker_digest, digest(args.worker))
    out = args.output / case['id']
    out.mkdir()
    source = Path(case['media']).resolve()
    media = out / ('media.t88' if case['slot'] == 'tape' else 'media.d88')
    shutil.copyfile(source, media)
    (out / 'config.ini').write_text('[NekoProjectII]\nMain_RAM=640\nMain_RAM_Auto=false\n'
                                  'CPU_Speed=100\nBootMode=3\nCMT_Fast=true\n', encoding='ascii')
    (out / 'input.txt').write_text(case['input'], encoding='utf-8')
    command = [str(args.worker), '--roms', str(args.roms), '--cfg', str(out / 'config.ini'),
               '--bkupmem', str(out / 'backup.bin'), '--model', 'va2', '--extra-machine', case['model'],
               '--nowait', '--mute', '--windowed', '--screen-tvram-dump', str(out / 'tvram.bin')]
    for frame in args.capture_frames:
        name = 'final.png' if frame == args.frames else f'frame{frame}.png'
        command += ['--screenshot', f'{frame}:{out / name}']
    if case['input']:
        command += ['--headless-input-script', str(out / 'input.txt')]
    for slot in ['fdd1', 'fdd2', 'sasi1', 'sasi2', *[f'scsi{i}' for i in range(7)]]:
        command += ['--' + slot, str(media) if slot == case['slot'] else 'none']
    if case['slot'] == 'tape':
        command += ['--tape', str(media)]
    report = {'id': case['id'], 'status': 'INCOMPLETE', 'scope': 'SDL dummy guest buffer only',
              'argv': command, 'worker_sha256': args.worker_digest,
              'runner_sha256': args.runner_digest, 'plan_sha256': args.plan_digest,
              'rom_manifest_sha256': args.rom_digest, 'input_media_sha256': digest(source),
              'script_sha256': digest(out / 'input.txt'), 'timeout_seconds': args.timeout}
    (out / 'result.json').write_text(json.dumps(report, indent=2) + '\n')
    with (out / 'stdout.log').open('wb') as stdout, (out / 'stderr.log').open('wb') as stderr:
        try:
            result = subprocess.run(command, cwd=out, stdout=stdout, stderr=stderr,
                                    timeout=args.timeout, env={**os.environ, 'HOME': str(out),
                                    'SDL_VIDEODRIVER': 'dummy', 'SDL_AUDIODRIVER': 'dummy'})
            report.update(exit=result.returncode, status='UNASSESSED' if result.returncode == 0 else 'FAIL_WORKER')
        except subprocess.TimeoutExpired:
            report.update(status='FAIL_TIMEOUT')
    report['outputs'] = {p.name: digest(p) for p in out.iterdir() if p.is_file() and p.name != 'result.json'}
    (out / 'result.json').write_text(json.dumps(report, indent=2) + '\n')
    return {'id': case['id'], 'status': report['status']}


def selftest():
    good = [{'id': 'case-01', 'model': 'PC-8001mkII', 'media': 'fixture.d88', 'slot': 'fdd1', 'input': ''}]
    validate_plan(good)
    for field, value, code in [('id', '../bad', 'N80_SURVEY_ID'),
                               ('model', 'unknown', 'N80_SURVEY_MODEL'),
                               ('slot', 'sasi1', 'N80_SURVEY_SLOT'),
                               ('media', '', 'N80_SURVEY_MEDIA'),
                               ('input', 1, 'N80_SURVEY_INPUT')]:
        fixture = copy.deepcopy(good)
        validate_plan(fixture)
        fixture[0][field] = value
        try:
            validate_plan(fixture)
        except SurveyError as error:
            require(str(error) == code, 'N80_SURVEY_WRONG_ERROR')
        else:
            raise SurveyError('N80_SURVEY_NEGATIVE_ACCEPTED')
    for code, mutate in [('N80_SURVEY_DUPLICATE', lambda f: f.append(copy.deepcopy(f[0]))),
                         ('N80_SURVEY_SCHEMA', lambda f: f[0].update(extra=0))]:
        fixture = copy.deepcopy(good)
        validate_plan(fixture)
        mutate(fixture)
        try:
            validate_plan(fixture)
        except SurveyError as error:
            require(str(error) == code, 'N80_SURVEY_WRONG_ERROR')
        else:
            raise SurveyError('N80_SURVEY_NEGATIVE_ACCEPTED')
    for check, good_args, bad_args, code in [
        (validate_output_location, (Path('fixture/output'), Path('fixture/source')),
         (Path('fixture/source/output'), Path('fixture/source')), 'N80_SURVEY_OUTPUT_IN_TREE'),
        (validate_worker_digest, ('abc', 'abc'), ('abc', 'def'), 'N80_SURVEY_WORKER_CHANGED'),
        (capture_schedule, (12000, [6000, 9000]), (12000, [6000, 12000]), 'N80_SURVEY_FRAMES')]:
        check(*good_args)
        try:
            check(*bad_args)
        except SurveyError as error:
            require(str(error) == code, 'N80_SURVEY_WRONG_ERROR')
        else:
            raise SurveyError('N80_SURVEY_NEGATIVE_ACCEPTED')
    print('n80-media-survey: PASS plan content and identity tests (10 negatives); no guest executed')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('worker', 'roms', 'plan', 'output'):
        parser.add_argument('--' + name, type=Path)
    parser.add_argument('--frames', type=int, default=12000)
    parser.add_argument('--checkpoint-frame', type=int, action='append', default=[])
    parser.add_argument('--timeout', type=int, default=180)
    parser.add_argument('--jobs', type=int, default=2)
    parser.add_argument('--selftest', action='store_true')
    args = parser.parse_args()
    if args.selftest:
        selftest()
        return 0
    try:
        require(all(getattr(args, key) for key in ('worker', 'roms', 'plan', 'output')), 'N80_SURVEY_ARGUMENTS')
        require(1 <= args.jobs <= 4 and 1 <= args.frames <= 100000 and args.timeout > 0, 'N80_SURVEY_LIMITS')
        args.capture_frames = capture_schedule(args.frames, args.checkpoint_frame)
        plan = validate_plan(json.loads(args.plan.read_text()))
        args.worker, args.roms, args.output = args.worker.resolve(), args.roms.resolve(), args.output.resolve()
        require(args.worker.is_file() and args.roms.is_dir(), 'N80_SURVEY_BINDINGS')
        require(all(Path(c['media']).is_file() for c in plan), 'N80_SURVEY_BINDINGS')
        validate_output_location(args.output, Path(__file__).resolve().parents[2])
        require(not args.output.exists(), 'N80_SURVEY_OUTPUT_EXISTS')
        args.output.mkdir(parents=True)
        args.worker_digest = digest(args.worker)
        args.runner_digest = digest(__file__)
        args.plan_digest = digest(args.plan)
        roms = {str(p.relative_to(args.roms)): digest(p) for p in sorted(args.roms.rglob('*.rom'))}
        (args.output / 'roms.json').write_text(json.dumps(roms, sort_keys=True, indent=2) + '\n')
        args.rom_digest = digest(args.output / 'roms.json')
        shutil.copyfile(args.plan, args.output / 'plan.json')
        with ThreadPoolExecutor(max_workers=args.jobs) as pool:
            results = list(pool.map(lambda case: run_case(args, case), plan))
        (args.output / 'summary.json').write_text(json.dumps(results, indent=2) + '\n')
        print(json.dumps(results))
        return int(any(r['status'].startswith('FAIL') for r in results))
    except (OSError, ValueError) as error:
        print(json.dumps({'status': 'FAIL', 'error_code': str(error) if isinstance(error, SurveyError) else 'N80_SURVEY_INPUT_IO'}))
        return 1


if __name__ == '__main__':
    raise SystemExit(main())
