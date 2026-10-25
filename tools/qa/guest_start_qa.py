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

"""Private guest-start regression: HUD geometry and frozen prior-baseline regions.

Eight fresh VA/VA2 V1/V2 H/S boots. Guest-only dummy frames are deliberate and
never count as final-client/presenter QA. Policy, media, captures and private
identities remain outside Git. Process exit zero alone cannot produce PASS.
"""
from __future__ import annotations
import argparse
from collections import deque
from concurrent.futures import ThreadPoolExecutor
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile

from screen_compare import read_png, png_bytes, require, QaError
from x11_capture import digest, ROOT


def validate_policy(policy):
    require(set(policy) == {"contract", "colour", "roi", "components", "width", "height",
                            "reference_sha256", "reference_file", "regions", "basis"} and
            policy["contract"] == "private-guest-hud-v1", "G132_GAME_POLICY")
    require(policy["width"] == 640 and policy["height"] == 422 and
            policy["components"] == 4 and len(policy["colour"]) == 3 and
            policy["colour"] == [200, 200, 60] and policy["roi"] == [470, 195, 165, 185], "G132_GAME_POLICY")
    require(policy["reference_file"] == "reference.png" and
            re.fullmatch("[a-f0-9]{64}", policy["reference_sha256"]) and
            policy["basis"] == "prior-accepted-g131" and
            policy["regions"] == [[496,195,138,165],[392,130,245,50]], "G132_GAME_POLICY")
    return policy


def hud_boxes(image, policy):
    w, h, pixels = image
    require((w, h) == (policy["width"], policy["height"]), "G132_GAME_GEOMETRY")
    x0, y0, width, height = policy["roi"]
    points = {(x, y) for y in range(y0, y0+height) for x in range(x0, x0+width)
              if pixels[(y*w+x)*4] >= 200 and pixels[(y*w+x)*4+1] >= 200 and pixels[(y*w+x)*4+2] <= 60}
    boxes = []
    while points:
        point = points.pop()
        todo, component = deque([point]), [point]
        while todo:
            x, y = todo.popleft()
            for neighbor in ((x-1, y), (x+1, y), (x, y-1), (x, y+1)):
                if neighbor in points:
                    points.remove(neighbor)
                    todo.append(neighbor)
                    component.append(neighbor)
        left, right = min(x for x, _ in component), max(x for x, _ in component)
        top, bottom = min(y for _, y in component), max(y for _, y in component)
        bw, bh = right-left+1, bottom-top+1
        if 25 <= bw <= 42 and 24 <= bh <= 43 and len(component) >= bw*bh*.9:
            boxes.append([left, top, bw, bh])
    boxes.sort(key=lambda b: b[1])
    require(len(boxes) == policy["components"] and max(b[0] for b in boxes)-min(b[0] for b in boxes) <= 2 and
            all(30 <= b[1]-a[1] <= 44 for a, b in zip(boxes, boxes[1:])), "G132_GAME_HUD_BOXES")
    return boxes


def reference_regions(image, reference, policy):
    require(image[:2] == reference[:2], 'G132_GAME_GEOMETRY')
    w, h, pixels = image
    for x,y,width,height in policy['regions']:
        for row in range(y,y+height):
            start,end=(row*w+x)*4,(row*w+x+width)*4
            require(pixels[start:end] == reference[2][start:end], 'G132_GAME_HUD_REFERENCE')


def classify(image, reference, policy):
    validate_policy(policy)
    boxes = hud_boxes(image, policy)
    reference_regions(image, reference, policy)
    return {'status':'PASS', 'boxes':boxes, 'exact_prior_baseline_regions':len(policy['regions'])}


def run_case(args, model, version, standard, policy, reference, root):
    name = f'{model}-v{version}{"s" if standard else "h"}'
    out = root / name
    out.mkdir()
    shutil.copyfile(args.media, out / "media.d88")
    seed = args.backup_seeds / f'basic-{model}-v{version}{"s" if standard else "h"}.bin'
    shutil.copyfile(seed, out / "backup.bin")
    (out / "config.ini").write_text('[NekoProjectII]\nBootMode=1\nMain_RAM=640\nMain_RAM_Auto=false\nCPU_Speed=100\n'
                                     + f'V1V2_Standard={str(standard).lower()}\n')
    (out / "input.txt").write_text('@wait 1800\n@enter\n@wait 900\n')
    argv = [str(args.worker.resolve()), '--roms', str(args.roms.resolve()), '--cfg', str(out/'config.ini'),
            '--bkupmem', str(out/'backup.bin'), '--model', model, '--cpumult', '2', '--mute', '--nowait',
            '--windowed', '--fdd1', str(out/'media.d88'), '--fdd2', str(out/'media.d88'), '--fdd2-image', '2',
            '--headless-input-script', str(out/'input.txt'), '--screen-tvram-dump', str(out/'tvram.bin'),
            '--screenshot', f'1790:{out}/title.png', '--screenshot', f'3600:{out}/after.png']
    for slot in ['sasi1', 'sasi2', *(f'scsi{i}' for i in range(7))]:
        argv += ['--'+slot, 'none']
    env = {**os.environ, 'SDL_VIDEODRIVER': 'dummy', 'SDL_AUDIODRIVER': 'dummy'}
    report = {"case": name, "status": "INCOMPLETE", "argv": argv, "backup_seed_sha256": digest(seed),
              "capture_scope": "dummy-guest-only", "headless_script_sha256": digest(out/'input.txt')}
    try:
        with (out/'stdout.log').open('wb') as so, (out/'stderr.log').open('wb') as se:
            child = subprocess.run(argv, env=env, stdout=so, stderr=se, cwd=out, timeout=360)
        report['worker_exit_status'] = child.returncode
        require(child.returncode == 0, 'G132_GAME_WORKER')
        require(digest(out/'title.png') != digest(out/'after.png'), 'G132_GAME_NO_TRANSITION')
        image = read_png((out/'after.png').read_bytes())
        report.update(classify(image, reference, policy))
    except (QaError, OSError, ValueError, subprocess.SubprocessError) as error:
        report.update(status='FAIL', error_code=error.code if isinstance(error, QaError) else 'G132_GAME_EXECUTION')
    report['outputs'] = {p.name: digest(p) for p in out.iterdir() if p.is_file()}
    (out/'result.json').write_text(json.dumps(report, indent=2)+'\n')
    print(name, report['status'], report.get('error_code',''), flush=True)
    return report


def run(args):
    root = args.output_dir.resolve()
    require(not root.exists() and ROOT != root and ROOT not in root.parents, 'G132_GAME_OUTPUT')
    require(re.fullmatch('[a-f0-9]{40}', args.evaluated_commit) and
            args.evaluated_commit.encode() in args.worker.read_bytes(), 'G132_GAME_WORKER_IDENTITY')
    policy = validate_policy(json.loads(args.policy.read_text()))
    reference_file = args.policy.parent / policy['reference_file']
    require(digest(reference_file) == policy['reference_sha256'], 'G132_GAME_REFERENCE_HASH')
    reference = read_png(reference_file.read_bytes())
    root.mkdir(parents=True)
    report = {'contract': 'guest-start-qa-v1', 'gate_pass': False, 'status': 'INCOMPLETE',
              'worker_commit': args.evaluated_commit, 'worker_sha256': digest(args.worker),
              'runner_sha256': digest(__file__), 'policy_sha256': digest(args.policy),
              'media_sha256': digest(args.media), 'capture_scope': 'dummy-guest-only',
              'rom_files': {str(p.relative_to(args.roms)): digest(p) for p in args.roms.rglob('*') if p.is_file()}}
    cases = [(m, v, s) for m in ('va', 'va2') for v in (1,2) for s in (False,True)]
    with ThreadPoolExecutor(max_workers=args.jobs) as pool:
        results = list(pool.map(lambda case: run_case(args, *case, policy, reference, root), cases))
    report.update(cases=results, status='PASS' if all(r['status']=='PASS' for r in results) else 'FAIL')
    require(digest(args.worker) == report['worker_sha256'] and digest(args.media) == report['media_sha256'],
            'G132_GAME_INPUT_CHANGED')
    (root/'summary.json').write_text(json.dumps(report,indent=2)+'\n')
    print(report['status'])
    return 0 if report['status']=='PASS' else 1


def selftest():
    policy = {'contract':'private-guest-hud-v1', 'colour':[200,200,60], 'roi':[470,195,165,185],
              'components':4, 'width':640, 'height':422, 'reference_sha256':'0'*64,
              'reference_file':'reference.png', 'regions':[[496,195,138,165],[392,130,245,50]],
              'basis':'prior-accepted-g131'}
    p = bytearray(bytes((0,0,0,255))*640*422)
    for y0 in (208,246,284,322):
        for y in range(y0,y0+32):
            for x in range(496,528):p[(y*640+x)*4:(y*640+x)*4+4] = bytes((255,255,0,255))
    image = (640,422,bytes(p))
    classify(image,image,policy)
    changed=bytearray(p)
    for y in range(208,240):
        for x in range(496,528):changed[(y*640+x)*4:(y*640+x)*4+4]=bytes((0,0,0,255))
    label_changed=bytearray(p);label_changed[(210*640+600)*4]=1
    tests=[(lambda: classify((640,422,bytes(changed)),image,policy),'G132_GAME_HUD_BOXES'),
           (lambda: classify((640,422,bytes(label_changed)),image,policy),'G132_GAME_HUD_REFERENCE'),
           (lambda: classify(image,image,{**policy,'components':3}),'G132_GAME_POLICY')]
    for action,expected in tests:
        try:action()
        except QaError as error:require(error.code==expected,'G132_GAME_SELFTEST')
        else:raise QaError('G132_GAME_SELFTEST')
    with tempfile.TemporaryDirectory() as d:
        file=Path(d)/'synthetic.png';file.write_bytes(png_bytes(image))
        classify(read_png(file.read_bytes()),image,policy)
    print('PASS: synthetic policy/HUD geometry/prior-reference pixels, three exact-code mutations, composed PNG integration')
    return 0


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--selftest',action='store_true')
    for name in ('worker','roms','media','backup-seeds','policy','output-dir'):p.add_argument('--'+name,type=Path)
    p.add_argument('--evaluated-commit');p.add_argument('--jobs',type=int,default=2)
    args=p.parse_args()
    if args.selftest:return selftest()
    require(all((args.worker,args.roms,args.media,args.backup_seeds,args.policy,args.output_dir,args.evaluated_commit))
            and 1<=args.jobs<=4,'G132_GAME_ARGUMENTS')
    return run(args)


if __name__=='__main__':
    raise SystemExit(main())
