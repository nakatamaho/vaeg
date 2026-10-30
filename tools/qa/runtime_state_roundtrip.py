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

"""Local full-state/configuration round trips, not GUI or cross-OS acceptance.

Input bindings, states, captures and their identities are private local output.
Content functions/selftests have no Git/history/large-corpus dependencies.
"""
import argparse
import configparser
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import re
import shutil
import struct
import subprocess
import sys

from screen_compare import QaError, require

ROOT = Path(__file__).resolve().parents[2]
HEADER = struct.pack('<16s28sI', b'VA-EG', b'created by VAEG.EXE', 0x08060a)
PROFILES = {'absent': (0, False), 'explicit-off': (0, False), 'enabled': (13, True)}


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def sections(data):
    require(data[:48] == HEADER, 'M142_STATE_HEADER')
    result, pos = {}, 48
    while pos < len(data):
        require(pos + 16 <= len(data), 'M142_STATE_CONTAINER')
        name, version, size = struct.unpack_from('<10sHI', data, pos)
        require(name not in result, 'M142_STATE_DUPLICATE')
        start, padded = pos + 16, (size + 15) & ~15
        require(start + padded <= len(data), 'M142_STATE_CONTAINER')
        require(not any(data[start + size:start + padded]), 'M142_STATE_CONTAINER')
        result[name] = (version, data[start:start + size])
        pos = start + padded
    return result


def expansion_sizes(data, ems, bms):
    parsed = sections(data)
    sizes = {}
    for name, expected in ((b'EXTMEM\0\0\0\0', ems * 1024 * 1024),
                           (b'BMSMEM\0\0\0\0', 128 * 128 * 1024 if bms else 0)):
        # The production writer omits sections with zero payload; absence is
        # the canonical disabled-board representation, not a truncated state.
        actual = len(parsed[name][1]) if name in parsed else 0
        require(actual == expected, 'M142_STATE_EXPANSION_SIZE')
        sizes[name.rstrip(b'\0').decode()] = expected
    require(b'DISK\0\0\0\0\0\0' in parsed and
            parsed[b'DISK\0\0\0\0\0\0'][0] == 1 and
            len(parsed[b'DISK\0\0\0\0\0\0'][1]) == 41080, 'M142_STATE_DISK_V1')
    return sizes


def compare_registers(expected, actual):
    # Observation timing is not a cycle-exact prefetch-queue restoration contract.
    omit = {'sequence', 'ordinal', 'clock'}
    left = {k: v for k, v in expected.items() if k not in omit}
    right = {k: v for k, v in actual.items() if k not in omit}
    require(left == right and left.get('schema') == 'vaeg-registers-v1', 'M142_STATE_REGISTERS')


def tvram_content(data):
    require(data[:8] == b'VAEGSCN1' and len(data) >= 16, 'M142_STATE_TVRAM_FORMAT')
    version, id_size = struct.unpack_from('<II', data, 8)
    start = 16 + id_size
    require(version == 1 and id_size <= 4096 and start + 28 <= len(data),
            'M142_STATE_TVRAM_FORMAT')
    memory_size = struct.unpack_from('<I', data, start + 24)[0]
    require(start + 28 + memory_size == len(data), 'M142_STATE_TVRAM_FORMAT')
    return data[start:]


def config_values(text, ems, bms, explicit=True):
    parser = configparser.ConfigParser(interpolation=None)
    parser.read_string(text)
    require(parser.has_section('NekoProjectII'), 'M142_STATE_CONFIG')
    section = parser['NekoProjectII']
    require(not explicit or ('ExMemory' in section and 'Use_BMS_' in section), 'M142_STATE_CONFIG')
    require(section.getint('ExMemory', 0) == ems and
            section.getboolean('Use_BMS_', fallback=False) == bms,
            'M142_STATE_CONFIG')
    if bms:
        require(section.getint('BMS_Size', -1) == 128 and
                int(section.get('BMS_Port', '0'), 16) == 0x1d0, 'M142_STATE_CONFIG')


def size_delta(profiles):
    absent_size = profiles['absent']['baseline_size']
    require(profiles['explicit-off']['baseline_size'] == absent_size, 'M142_STATE_DISABLED_SIZE')
    delta = profiles['enabled']['baseline_size'] - absent_size
    # Disabled sections are omitted; enabled ones each add a 16-byte header.
    require(delta == (13 + 16) * 1024 * 1024 + 32, 'M142_STATE_ENABLED_SIZE_DELTA')
    return delta


def selftest():
    def section(name, version, body):
        return struct.pack('<10sHI', name, version, len(body)) + body + b'\0' * (-len(body) % 16)
    fixture = HEADER + section(b'DISK', 1, b'\0' * 41080)
    expansion_sizes(fixture, 0, False)
    cases = [('M142_STATE_HEADER', bytes([fixture[0] ^ 1]) + fixture[1:]),
             ('M142_STATE_CONTAINER', fixture[:-1]),
             ('M142_STATE_DUPLICATE', fixture + section(b'DISK', 1, b''))]
    for code, changed in cases:
        expansion_sizes(fixture, 0, False)
        try:
            expansion_sizes(changed, 0, False)
        except QaError as error:
            require(error.code == code, 'M142_STATE_SELFTEST_MASKED')
        else:
            raise QaError('M142_STATE_SELFTEST_ACCEPTED')
    positive = {'schema': 'vaeg-registers-v1', 'ax': '1234', 'sequence': '1', 'ordinal': '1'}
    compare_registers(positive, positive)
    changed = dict(positive, ax='1235')
    try:
        compare_registers(positive, changed)
    except QaError as error:
        require(error.code == 'M142_STATE_REGISTERS', 'M142_STATE_SELFTEST_MASKED')
    else:
        raise QaError('M142_STATE_SELFTEST_ACCEPTED')
    text = '[NekoProjectII]\nExMemory=0\nUse_BMS_=false\n'
    tvram = b'VAEGSCN1' + struct.pack('<II7I', 1, 0, *([0] * 7))
    config_values('[NekoProjectII]\n', 0, False, explicit=False)
    extra = [
        ('M142_STATE_EXPANSION_SIZE', lambda: expansion_sizes(fixture, 0, False),
         lambda: expansion_sizes(fixture, 1, False)),
        ('M142_STATE_CONFIG', lambda: config_values(text, 0, False),
         lambda: config_values(text.replace('ExMemory=0', 'ExMemory=1'), 0, False)),
        ('M142_STATE_CONFIG', lambda: config_values(text, 0, False),
         lambda: config_values(text.replace('ExMemory=0\n', ''), 0, False)),
        ('M142_STATE_TVRAM_FORMAT', lambda: tvram_content(tvram),
         lambda: tvram_content(tvram[:8] + struct.pack('<I', 2) + tvram[12:])),
    ]
    for code, passing, mutation in extra:
        passing()
        try:
            mutation()
        except QaError as error:
            require(error.code == code, 'M142_STATE_SELFTEST_MASKED')
        else:
            raise QaError('M142_STATE_SELFTEST_ACCEPTED')
    profiles = {'absent': {'baseline_size': 100}, 'explicit-off': {'baseline_size': 100},
                'enabled': {'baseline_size': 100 + 29 * 1024 * 1024 + 32}}
    for name, code in [('explicit-off', 'M142_STATE_DISABLED_SIZE'),
                       ('enabled', 'M142_STATE_ENABLED_SIZE_DELTA')]:
        size_delta(profiles)
        changed = {key: dict(value) for key, value in profiles.items()}
        changed[name]['baseline_size'] += 1
        try:
            size_delta(changed)
        except QaError as error:
            require(error.code == code, 'M142_STATE_SELFTEST_MASKED')
        else:
            raise QaError('M142_STATE_SELFTEST_ACCEPTED')
    print('runtime-state-selftest: PASS (10 exact-code single-mutation content negatives)')


def execute(args, folder, config, script, disk):
    folder.mkdir(exist_ok=True)
    (folder / 'home').mkdir(exist_ok=True)
    capture = folder / 'capture'
    capture.mkdir(exist_ok=True)
    script_path = folder / 'debug.txt'
    script_path.write_text(script)
    command = [str(args.worker), '--cfg', str(config), '--no-bkupmem', '--roms', str(args.roms),
               '--model', 'va2', '--fdd1', str(disk) if disk else 'none', '--fdd2', 'none',
               '--sasi1', 'none', '--sasi2', 'none']
    for index in range(7):
        command += [f'--scsi{index}', 'none']
    command += ['--nowait', '--frameskip', 'full', '--debug-script', str(script_path),
                '--debug-output-dir', str(capture)]
    env = {**os.environ, 'HOME': str(folder / 'home'), 'SDL_VIDEODRIVER': 'dummy',
           'SDL_AUDIODRIVER': 'dummy'}
    record = {'command': command, 'config_before': config.read_text(),
              'config_before_sha256': digest(config), 'script_sha256': digest(script_path)}
    with (folder / 'worker.log').open('w') as log:
        try:
            record['exit'] = subprocess.run(command, env=env, cwd=folder, stdout=log,
                                            stderr=subprocess.STDOUT, timeout=180).returncode
        except subprocess.TimeoutExpired:
            record['exit'] = 'TIMEOUT'
    record['outputs'] = {str(p.relative_to(folder)): {'size': p.stat().st_size, 'sha256': digest(p)}
                         for p in folder.rglob('*') if p.is_file()}
    (folder / 'run.json').write_text(json.dumps(record, indent=2) + '\n')
    return record


def run(args):
    args.worker, args.roms, args.kit = [p.resolve() for p in (args.worker, args.roms, args.kit)]
    out = args.output_dir.resolve()
    require(out != ROOT and ROOT not in out.parents and not out.exists(), 'M142_STATE_OUTPUT')
    require(re.fullmatch('[0-9a-f]{40}', args.evaluated_commit) and
            args.evaluated_commit.encode() in args.worker.read_bytes(), 'M142_STATE_WORKER_IDENTITY')
    out.mkdir(parents=True)
    spec = importlib.util.spec_from_file_location('tvvb_build', ROOT / 'tools/hardware/tvram_vb/build.py')
    build = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(build)
    expected_source = build.variant((build.DEMO / 'tspspr.asm').read_text(), 128, False)[1]
    asm = args.kit / 'source/v128a.asm'
    require(asm.read_text() == expected_source, 'M142_STATE_PROGRAM_SOURCE')
    assembly = ['nasm', '-f', 'bin', '-I', str(asm.parent) + '/', '-o', str(out / 'program.com'),
                '-l', str(out / 'listing.txt'), str(asm)]
    subprocess.run(assembly, check=True)
    require((out / 'program.com').read_bytes() == (args.kit / 'payload/root/V128A.COM').read_bytes(),
            'M142_STATE_PROGRAM_BINARY')
    labels = {}
    for line in (out / 'listing.txt').read_text().splitlines():
        match = re.search(r'^\s*\d+\s+([0-9A-F]{8})\s+\S+\s+(?:<\d+>\s+)?(\w+):', line)
        if match:
            labels[match[2]] = int(match[1], 16) + 0x100
    q, mapped = labels['t_tv_vb'], labels['map_tvram']
    manifest = {'schema': 'm142-runtime-state-roundtrip-v1', 'status': 'INCOMPLETE',
                'evaluated_commit': args.evaluated_commit, 'worker_sha256': digest(args.worker),
                'runner_sha256': digest(Path(__file__)), 'assembly_command': assembly,
                'program_sha256': digest(out / 'program.com'),
                'kit_manifest_sha256': digest(args.kit / 'manifest.json'),
                'kit_image_sha256': digest(args.kit / 'tvram-vb-msdos4.d88'),
                'roms': {p.name: digest(p) for p in args.roms.iterdir() if p.is_file()},
                'target': 'SDL dummy native software; not GUI, presenter or Windows/Linux exchange',
                'profiles': {}}
    (out / 'manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
    for name, (ems, bms) in PROFILES.items():
        case = out / name
        case.mkdir()
        disk = case / 'fixture.d88'
        shutil.copy2(args.kit / 'tvram-vb-msdos4.d88', disk)
        config = case / 'vaeg.cfg'
        text = '[NekoProjectII]\nMonitor15kHz=true\nDIPswtch=3e 73 7b\nclk_mult=2\nCPU_Speed=100\n'
        if name != 'absent':
            text += f'ExMemory={ems}\nUse_BMS_={str(bms).lower()}\nBMS_Size=128\nBMS_Port=01d0\n'
        config.write_text(text)
        save = case / 'roundtrip/capture/baseline.state'
        script = (f'debug-script 1\nlimit-frame 11000\nresource saved {save}\n'
                  f'wait-frame 180\ninput-line V128A 1\nwait-pc 2000:{q:04x} 1\n'
                  f'save-state baseline\nwait-pc 2000:{mapped:04x} 1\n'
                  'capture expected registers tvram gvram\nwait-frame 6000\nload-state saved\n'
                  f'wait-pc 2000:{mapped:04x} 1\ncapture restored registers tvram gvram\n'
                  'save-state restored\nexit\n')
        record = execute(args, case / 'roundtrip', config, script, disk)
        require(record['exit'] == 0, 'M142_STATE_WORKER_EXIT')
        capture = case / 'roundtrip/capture'
        for id in ('expected', 'restored'):
            require((capture / f'{id}.registers.tsv').is_file(), 'M142_STATE_CAPTURE_MISSING')
        read = lambda id: dict(line.split('\t', 1) for line in
                               (capture / f'{id}.registers.tsv').read_text().splitlines())
        compare_registers(read('expected'), read('restored'))
        require(tvram_content((capture / 'expected.tvram.bin').read_bytes()) ==
                tvram_content((capture / 'restored.tvram.bin').read_bytes()), 'M142_STATE_MEMORY')
        require((capture / 'expected.gvram.bin').read_bytes() ==
                (capture / 'restored.gvram.bin').read_bytes(), 'M142_STATE_MEMORY')
        sizes = expansion_sizes(save.read_bytes(), ems, bms)
        expansion_sizes((capture / 'restored.state').read_bytes(), ems, bms)
        config_values(config.read_text(), ems, bms, explicit=name != 'absent')
        restart = execute(args, case / 'restart', config,
                          'debug-script 1\nlimit-frame 20\nwait-frame 5\nsave-state restarted\nexit\n', None)
        require(restart['exit'] == 0, 'M142_STATE_RESTART_EXIT')
        restarted = case / 'restart/capture/restarted.state'
        expansion_sizes(restarted.read_bytes(), ems, bms)
        config_values(config.read_text(), ems, bms, explicit=name != 'absent')
        manifest['profiles'][name] = {'status': 'PASS_STATE_CONFIG_CONTRACT',
                                     'baseline_size': save.stat().st_size, 'sections': sizes,
                                     'continuation_clock_samples': [read('expected')['clock'],
                                                                    read('restored')['clock']],
                                     'baseline_sha256': digest(save), 'restart_sha256': digest(restarted),
                                     'run_sha256': digest(case / 'roundtrip/run.json'),
                                     'restart_run_sha256': digest(case / 'restart/run.json')}
        (out / 'manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
    finish(args, out, manifest)


def finish(args, out, manifest):
    """Finalize already validated profiles; no guest rerun is needed here."""
    manifest['enabled_size_delta'] = size_delta(manifest['profiles'])
    guard_script = ('debug-script 1\nlimit-frame 20\nwait-frame 5\n'
                    'save-state first\nsave-state second\nexit\n')
    guard_config = out / 'absent/vaeg.cfg'
    positive = execute(args, out / 'guard-positive', guard_config, guard_script, None)
    require(positive['exit'] == 0 and (out / 'guard-positive/capture/second.state').is_file(),
            'M142_STATE_GUARD_FIXTURE')
    original = (out / 'guard-positive/capture/first.state').read_bytes()
    existing_dir = out / 'guard-existing/capture'
    existing_dir.mkdir(parents=True)
    existing_path = existing_dir / 'first.state'
    existing_path.write_bytes(original)
    for name, script, code in [('guard-existing', guard_script, 'M142_DEBUG_STATE_EXISTS'),
                               ('guard-id', guard_script.replace('save-state first',
                                                                'save-state ../escape', 1),
                                'M142_DEBUG_STATE_ID')]:
        rejected = execute(args, out / name, guard_config, script, None)
        codes = re.findall(r'^M142_[A-Z_]+$', (out / name / 'worker.log').read_text(), re.M)
        require(rejected['exit'] != 0 and codes == [code], 'M142_STATE_GUARD_MASKED')
    require(existing_path.read_bytes() == original, 'M142_STATE_OVERWRITE')
    require(not (out / 'guard-id/escape.state').exists(), 'M142_STATE_PATH_ESCAPE')
    manifest['save_guards'] = ['M142_DEBUG_STATE_EXISTS', 'M142_DEBUG_STATE_ID']
    manifest['content_validator_sha256'] = digest(Path(__file__))
    manifest['status'] = 'PASS_STATE_CONFIG_CONTRACT_NOT_GUI_OR_CROSS_OS'
    (out / 'manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
    print(json.dumps({'status': manifest['status'], 'profiles': manifest['profiles']}, indent=2))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--selftest', action='store_true')
    parser.add_argument('--worker', type=Path)
    parser.add_argument('--roms', type=Path)
    parser.add_argument('--kit', type=Path)
    parser.add_argument('--evaluated-commit')
    parser.add_argument('--output-dir', type=Path)
    args = parser.parse_args()
    try:
        if args.selftest:
            selftest()
        else:
            require(all((args.worker, args.roms, args.kit, args.evaluated_commit, args.output_dir)),
                    'M142_STATE_ARGUMENTS')
            run(args)
    except QaError as error:
        print(json.dumps({'error': error.code}), file=sys.stderr)
        return 1
    except (OSError, KeyError, ValueError, configparser.Error, subprocess.CalledProcessError) as error:
        print(json.dumps({'error': 'M142_STATE_INPUT_OR_RUN_IO', 'detail': str(error)}), file=sys.stderr)
        return 1
    return 0


if __name__ == '__main__':
    sys.exit(main())
