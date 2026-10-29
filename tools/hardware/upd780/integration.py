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
"""Explicit private-input software integration. NEVER physical acceptance.

Outputs include sensitive images/logs/identities; keep the whole output tree
outside Git. The caller supplies every private binding. No hosted CI upload.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import time

import build
import results
import seed
import fixed
import capture
import xadr
import pref


def sha(data):
    return hashlib.sha256(data).hexdigest()


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--worker', type=Path, required=True)
    p.add_argument('--roms', type=Path, required=True)
    p.add_argument('--image', type=Path, required=True)
    p.add_argument('--build-report', type=Path, required=True)
    p.add_argument('--output', type=Path, required=True)
    p.add_argument('--case', choices=('probe', 'smoke', 'existing', 'history', 'flags', 'history-pair', 'seed', 'seed-existing', 'fixed', 'fixed-existing', 'capture', 'capture-existing', 'xadr', 'xadr-existing', 'pref', 'pref-existing'), required=True)
    p.add_argument('--standard', choices=('true', 'false'), default='true')
    a = p.parse_args()
    out = a.output.resolve()
    out.mkdir(parents=True, exist_ok=False)
    worker = a.worker.resolve()
    original = a.image.read_bytes()
    before = results.disk_files(original)
    recipe = json.loads(a.build_report.read_text())
    for name, digest in recipe['files_sha256'].items():
        results.need(name in before and sha(before[name][:recipe['files_size'][name]]) == digest,
                     'M139_INTEGRATION_PACKAGE_IDENTITY')
    disk = out / 'input.d88'
    disk.write_bytes(original)
    home = out / 'home'
    home.mkdir()
    cfg = out / 'config.ini'
    cfg.write_text('[NekoProjectII]\nMain_RAM=640\nMain_RAM_Auto=false\n'
                   'CPU_Speed=100\nBootMode=1\nV1V2_Standard=' + a.standard + '\n')
    name = {'smoke':'FDCONE','history':'FDCHIST','flags':'FDCFLAG','history-pair':'FDCFLAG',
            'seed':'FDCSEED','seed-existing':'FDCSEED',
            'fixed':'FDCFIX','fixed-existing':'FDCFIX',
            'capture':'FDCCAP','capture-existing':'FDCCAP',
            'xadr':'FDCXADR','xadr-existing':'FDCXADR',
            'pref':'FDCPREF','pref-existing':'FDCPREF'}.get(a.case,'FDCPRB')
    extension = 'DAT' if a.case in ('seed','seed-existing','fixed','fixed-existing','capture','capture-existing','xadr','xadr-existing','pref','pref-existing') else 'TXT'
    existing = a.case in ('existing','seed-existing','fixed-existing','capture-existing','xadr-existing','pref-existing')
    wait, frames = (24000, 30000) if a.case == 'probe' else (9000, 13000)
    if a.case in ('history','flags'):
        wait, frames = 12000, 16000
    if existing:
        results.need(name + '.' + extension in before, 'M139_INTEGRATION_EXISTING_FIXTURE')
        wait, frames = 3000, 7000
    script = out / 'input.txt'
    script.write_text(f'@wait 1600\n@text {name}\n@enter\n@wait {wait}\n@text DIR\n@enter\n')
    if a.case=='history-pair':
        frames=30000
        script.write_text('@wait 1600\n@text FDCHIST\n@enter\n@wait 12000\n'
                          '@text FDCFLAG\n@enter\n@wait 12000\n@text DIR\n@enter\n')
    tvram = out / 'tvram.bin'
    command = [str(worker), '--roms', str(a.roms.resolve()), '--cfg', str(cfg),
               '--no-bkupmem', '--model', 'va2', '--nowait', '--mute',
               '--fdd1', str(disk), '--fdd2', 'none', '--sasi1', 'none', '--sasi2', 'none',
               '--headless-input-script', str(script),
               '--screenshot', f'{frames}:{out / "final.png"}',
               '--screen-tvram-dump', str(tvram)]
    env = dict(os.environ, HOME=str(home), XDG_CONFIG_HOME=str(home),
               SDL_VIDEODRIVER='dummy', SDL_AUDIODRIVER='dummy')
    report = {
        'schema': 'vaeg-m139-fdc-integration-v1',
        'status': 'NOT_RUN',
        'case': a.case,
        'command': command,
        'worker_sha256': sha(worker.read_bytes()),
        'input_sha256': sha(original),
        'recipe_sha256': sha(a.build_report.read_bytes()),
        'rom_files': {x.name: sha(x.read_bytes()) for x in sorted(a.roms.iterdir()) if x.is_file()},
        'contracts': {x.name: sha(x.read_bytes()) for x in (Path(__file__), Path(results.__file__), Path(build.__file__), Path(seed.__file__), Path(fixed.__file__), Path(capture.__file__), Path(xadr.__file__), Path(pref.__file__))},
        'target_policy': 'VA2 CP/M protocol/persistence integration; SDL dummy; NOT physical FDC or presenter acceptance',
        'config_standard': a.standard,
    }
    start = time.monotonic()
    try:
        with (out / 'stdout.log').open('wb') as stdout, (out / 'stderr.log').open('wb') as stderr:
            run = subprocess.run(command, env=env, stdout=stdout, stderr=stderr, timeout=360)
        report['exit_status'] = run.returncode
        results.need(run.returncode == 0, 'M139_INTEGRATION_WORKER_EXIT')
        after = results.disk_files(disk.read_bytes())
        results.need(all(after.get(n) == data for n, data in before.items()),
                     'M139_INTEGRATION_EXISTING_FILES_CHANGED')
        system_before = build.unpack(results.normalize_capture_image(original))[0][:0x4000]
        system_after = build.unpack(results.normalize_capture_image(disk.read_bytes()))[0][:0x4000]
        results.need(system_after == system_before, 'M140_INTEGRATION_SYSTEM_CHANGED')
        logname = name + '.' + extension
        if existing:
            results.need(after == before, 'M139_INTEGRATION_REFUSAL_CHANGED_FILES')
        else:
            results.need(logname in after, 'M139_INTEGRATION_LOG_MISSING')
            data = after[logname]
            (out / logname).write_bytes(data)
            if a.case == 'pref':
                report['content'] = pref.content(data, host_model=True)
            elif a.case == 'xadr':
                report['content'] = xadr.content(data, host_model=True)
            elif a.case == 'capture':
                report['content'] = capture.content(data, host_model=True)
            elif a.case == 'fixed':
                report['content'] = fixed.content(data, host_model=True)
            elif a.case == 'seed':
                report['content'] = seed.content(data, host_model=True)
            elif a.case == 'probe':
                report['content'] = results.probe(data, unit='E')
                results.need(not report['content']['zilog_difference_records'], 'M139_INTEGRATION_HOST_AF')
            elif a.case in ('history','flags'):
                report['content'] = results.history(data,profile=name,host_model=True)
            elif a.case=='history-pair':
                report['content']={}
                for profile in build.HISTORY_PROFILES:
                    results.need(profile+'.TXT' in after,'M140_HISTORY_PAIR_LOG_MISSING')
                    (out/(profile+'.TXT')).write_bytes(after[profile+'.TXT'])
                    report['content'][profile]=results.history(after[profile+'.TXT'],profile=profile,host_model=True)
            else:
                report['content'] = results.zex(data, smoke=True)
        rows = results.cpm_screen(tvram.read_bytes())
        (out / 'screen-rows.json').write_text(json.dumps(rows, ensure_ascii=True, indent=2) + '\n')
        results.terminal(rows, name, existing=existing, extension=extension)
        results.need(a.image.read_bytes() == original, 'M139_INTEGRATION_SOURCE_CHANGED')
        report['status'] = 'PASS_SOFTWARE_INTEGRATION_ONLY'
    except results.RecordError as exc:
        report.update(status='FAIL', code=exc.code)
    except subprocess.TimeoutExpired:
        report.update(status='FAIL', code='M139_INTEGRATION_WORKER_TIMEOUT')
    finally:
        report['host_wall_seconds'] = time.monotonic() - start
        report['outputs'] = {x.name: sha(x.read_bytes()) for x in sorted(out.iterdir()) if x.is_file()}
        (out / 'run.json').write_text(json.dumps(report, indent=2) + '\n')
    print(report['status'], report.get('code', ''))
    if report['status'] != 'PASS_SOFTWARE_INTEGRATION_ONLY':
        raise SystemExit(1)


if __name__ == '__main__':
    main()
