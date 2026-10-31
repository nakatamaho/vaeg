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

"""Private-input, fresh-process SAV loads; not cross-OS or window acceptance.

Consume an identity-bound runtime_state_roundtrip producer output. Keep all
states, configuration, media, raw logs and their identities outside Git/CI.
Content comparison and its single-mutation selftests do not require Git,
private inputs or an emulator process.
"""
import argparse
import json
from pathlib import Path
import platform
import re
import shutil
import struct
import sys
from types import SimpleNamespace

import runtime_state_roundtrip as qa
from screen_compare import QaError, require

BMS = b'BMSSTATE\0\0'
DISK = b'DISK\0\0\0\0\0\0'
RECORD = 4160
PATH_BYTES = 4096


def pack(parts):
    result = bytearray(qa.HEADER)
    for name, (version, body) in parts.items():
        result += struct.pack('<10sHI', name, version, len(body))
        result += body + b'\0' * (-len(body) % 16)
    return bytes(result)


def prepare(data):
    parts = qa.sections(data)
    require(BMS in parts and len(parts[BMS][1]) >= 16, 'M142_RESTORE_BMS')
    version, body = parts[BMS]
    if body[0]:
        ram = len(body) - 16
        require(ram > 0 and ram % (128 * 1024) == 0, 'M142_RESTORE_BMS')
        pattern = (bytes(range(256)) * ((ram + 255) // 256))[:ram]
        parts[BMS] = version, body[:16] + b'M142_BMS_MARKER!' + pattern[16:]
    version, disk = parts[DISK]
    require(version == 2 and len(disk) == 13 * RECORD, 'M142_RESTORE_DISK')
    disk = bytearray(disk)
    for start in range(0, len(disk), RECORD):
        if disk[start]:
            path = b'Q:\\saved\\fixture.d88'
            disk[start:start + PATH_BYTES] = path + b'\0' * (PATH_BYTES - len(path))
    parts[DISK] = version, bytes(disk)
    return pack(parts)


def verify_restored(expected, actual, local_path):
    left, right = qa.sections(expected), qa.sections(actual)
    require(BMS in right and left[BMS] == right[BMS], 'M142_RESTORE_BMS')
    require(DISK in right and right[DISK][0] == 2 and
            len(right[DISK][1]) == 13 * RECORD, 'M142_RESTORE_DISK')
    for start in range(0, 13 * RECORD, RECORD):
        source = left[DISK][1][start:start + RECORD]
        restored = right[DISK][1][start:start + RECORD]
        path = restored[:PATH_BYTES].split(b'\0', 1)[0]
        require(path == (local_path.encode() if source[0] else b'') and
                source[PATH_BYTES:] == restored[PATH_BYTES:], 'M142_RESTORE_BINDING')


def selftest():
    header = struct.pack('<BBBBHHII', 1, 1, 0, 0, 0x1d0, 0xffff, 128 * 1024, 0)
    disk = bytearray(13 * RECORD)
    disk[:11] = b'fixture.d88'
    source = pack({BMS: (1, header + bytes(128 * 1024)), DISK: (2, bytes(disk))})
    prepared = prepare(source)
    parts = qa.sections(prepared)
    body = bytearray(parts[DISK][1])
    body[:PATH_BYTES] = b'fixture.d88' + bytes(PATH_BYTES - 11)
    parts[DISK] = 2, bytes(body)
    positive = pack(parts)
    for kind in ('missing-bms', 'ram', 'device', 'binding'):
        verify_restored(prepared, positive, 'fixture.d88')
        changed = qa.sections(positive)
        if kind == 'missing-bms':
            del changed[BMS]
        elif kind in ('ram', 'device'):
            value = bytearray(changed[BMS][1])
            value[16 if kind == 'ram' else 4] ^= 1
            changed[BMS] = 1, bytes(value)
        else:
            value = bytearray(changed[DISK][1])
            value[0] ^= 1
            changed[DISK] = 2, bytes(value)
        code = 'M142_RESTORE_BINDING' if kind == 'binding' else 'M142_RESTORE_BMS'
        try:
            verify_restored(prepared, pack(changed), 'fixture.d88')
        except QaError as error:
            require(error.code == code, 'M142_RESTORE_SELFTEST_MASKED')
        else:
            raise QaError('M142_RESTORE_SELFTEST_ACCEPTED')
    print('state-restore-matrix: PASS (4 exact-code single-mutation content negatives)')


def run(args):
    producer, out = args.producer_output.resolve(), args.output_dir.resolve()
    worker, roms = args.worker.resolve(), args.roms.resolve()
    require(out != qa.ROOT and qa.ROOT not in out.parents and not out.exists(), 'M142_RESTORE_OUTPUT')
    manifest = json.loads((producer / 'manifest.json').read_text())
    require(manifest['status'] == 'PASS_STATE_CONFIG_CONTRACT_NOT_GUI_OR_CROSS_OS' and
            manifest['worker_sha256'] == qa.digest(worker) and
            manifest['evaluated_commit'].encode() in worker.read_bytes() and
            manifest['runner_sha256'] == qa.digest(Path(qa.__file__)), 'M142_RESTORE_PRODUCER')
    require({p.name: qa.digest(p) for p in roms.iterdir() if p.is_file()} == manifest['roms'],
            'M142_RESTORE_ROMS')
    sources = {}
    retained = {worker: qa.digest(worker), producer / 'manifest.json': qa.digest(producer / 'manifest.json')}
    retained.update({p: qa.digest(p) for p in roms.iterdir() if p.is_file()})
    for name in ('enabled', 'explicit-off'):
        profile = producer / name
        state = profile / 'roundtrip/capture/baseline.state'
        require(qa.digest(state) == manifest['profiles'][name]['baseline_sha256'], 'M142_RESTORE_INPUT')
        disk = profile / 'fixture.d88'
        retained.update({state: qa.digest(state), disk: qa.digest(disk)})
        record_path = profile / 'roundtrip/run.json'
        require(qa.digest(record_path) == manifest['profiles'][name]['run_sha256'], 'M142_RESTORE_INPUT')
        record = json.loads(record_path.read_text())
        retained[record_path] = qa.digest(record_path)
        for suffix in ('registers.tsv', 'tvram.bin', 'gvram.bin'):
            relative = 'capture/expected.' + suffix
            reference = profile / 'roundtrip' / relative
            require(qa.digest(reference) == record['outputs'][relative]['sha256'], 'M142_RESTORE_INPUT')
            retained[reference] = qa.digest(reference)
        raw = state.read_bytes()
        qa.expansion_sizes(raw, 13 if name == 'enabled' else 0, name == 'enabled')
        media = qa.sections(raw)[DISK][1]
        require(media[0] and not any(media[start] for start in range(RECORD, len(media), RECORD)) and
                struct.unpack_from('<Q', media, 4100)[0] == disk.stat().st_size and
                media[4108:4140] == bytes.fromhex(qa.digest(disk)), 'M142_RESTORE_MEDIA_IDENTITY')
        sources[name] = profile, prepare(raw), disk, record['config_before']
    out.mkdir(parents=True)
    report = {'schema': 'm142-prior-state-restore-v1', 'status': 'INCOMPLETE',
              'evaluated_commit': manifest['evaluated_commit'], 'worker_sha256': qa.digest(worker),
              'runner_sha256': qa.digest(Path(__file__)), 'producer_manifest_sha256': qa.digest(producer / 'manifest.json'),
              'comparison_contract_sha256': qa.digest(Path(qa.__file__)),
              'target': f'{platform.system()} {platform.machine()} SDL dummy; not a window or genuine cross-OS exchange',
              'cases': {}}
    write = lambda: (out / 'results.json').write_text(json.dumps(report, indent=2) + '\n')
    write()
    plans = [('matching-on', 'enabled', None, None),
             ('reader-off', 'enabled', ('Use_BMS_=true', 'Use_BMS_=false'), None),
             ('reader-smaller', 'enabled', ('BMS_Size=128', 'BMS_Size=1'), None),
             ('reader-other-port', 'enabled', ('BMS_Port=01d0', 'BMS_Port=00ec'), None),
             ('saved-off-reader-on', 'explicit-off', ('Use_BMS_=false', 'Use_BMS_=true'), None),
             ('missing-reference', 'enabled', None, 'M142_DISK_MISSING'),
             ('changed-media', 'enabled', None, 'M142_DISK_IDENTITY')]
    for name, source, replacement, failure in plans:
        profile, data, image, text = sources[source]
        case = out / name
        case.mkdir()
        disk, state, config = case / 'fixture.d88', case / 'input.sav', case / 'vaeg.cfg'
        shutil.copy2(image, disk)
        state.write_bytes(data)
        if replacement:
            require(text.count(replacement[0]) == 1, 'M142_RESTORE_CONFIG_FIXTURE')
            text = text.replace(*replacement)
        script = ('debug-script 1\nlimit-frame 60\n' + f'resource saved {state}\nwait-frame 5\nload-state saved\n'
                  'wait-pc 2000:086b 1\ncapture restored registers tvram gvram\nsave-state restored\nexit\n')
        outcomes = {}
        for attempt in (('control', 'mutation') if failure else ('load',)):
            if attempt == 'mutation':
                if failure == 'M142_DISK_MISSING':
                    parts = qa.sections(data)
                    body = bytearray(parts[DISK][1])
                    path = b'Q:\\saved\\absent.d88'
                    body[:PATH_BYTES] = path + bytes(PATH_BYTES - len(path))
                    parts[DISK] = 2, bytes(body)
                    state.write_bytes(pack(parts))
                else:
                    body = bytearray(disk.read_bytes())
                    body[0] ^= 0x20
                    disk.write_bytes(body)
            config.write_text(text)
            before = {str(p): qa.digest(p) for p in (state, disk)}
            folder = case / attempt
            record = qa.execute(SimpleNamespace(worker=worker, roms=roms), folder, config, script, disk)
            require(before == {str(p): qa.digest(p) for p in (state, disk)}, 'M142_RESTORE_INPUT_CHANGED')
            result = {'exit': record['exit'], 'run_sha256': qa.digest(folder / 'run.json'),
                      'input_sha256': qa.digest(state), 'media_sha256': qa.digest(disk)}
            outcomes[attempt] = result
            if attempt == 'mutation':
                log = (folder / 'worker.log').read_text()
                codes = re.findall(r'^load-state: (M142_[A-Z_]+)\r?$', log, re.M)
                require(record['exit'] != 0 and codes == [failure] and
                        not (folder / 'capture/restored.state').exists(), 'M142_RESTORE_GUARD_MASKED')
                result['status'] = 'PASS_EXACT_REJECTION_NOT_RESTORATION'
                result['error_code'] = failure
            else:
                require(record['exit'] == 0, 'M142_RESTORE_WORKER')
                capture = folder / 'capture'
                restored = capture / 'restored.state'
                verify_restored(data, restored.read_bytes(), str(disk))
                reference = profile / 'roundtrip/capture'
                read = lambda p: dict(line.split('\t', 1) for line in p.read_text().splitlines())
                qa.compare_registers(read(reference / 'expected.registers.tsv'), read(capture / 'restored.registers.tsv'))
                require(qa.tvram_content((reference / 'expected.tvram.bin').read_bytes()) ==
                        qa.tvram_content((capture / 'restored.tvram.bin').read_bytes()) and
                        (reference / 'expected.gvram.bin').read_bytes() == (capture / 'restored.gvram.bin').read_bytes(),
                        'M142_RESTORE_GUEST_MEMORY')
                result['status'] = 'PASS_GUEST_CONTINUATION_BMS_AND_MEDIA_BINDING'
                result['output_sha256'] = qa.digest(restored)
            report['cases'][name] = outcomes
            write()
            print(name, attempt, result['status'], flush=True)
    require(retained == {p: qa.digest(p) for p in retained}, 'M142_RESTORE_RETAINED_INPUT_CHANGED')
    report['status'] = 'PASS_PRIOR_FILE_BMS_AND_MEDIA_CONTRACT_NOT_CROSS_OS_OR_GUI'
    write()
    print(report['status'])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--selftest', action='store_true')
    for name in ('worker', 'roms', 'producer-output', 'output-dir'):
        parser.add_argument('--' + name, type=Path)
    args = parser.parse_args()
    try:
        if args.selftest:
            selftest()
        else:
            require(all((args.worker, args.roms, args.producer_output, args.output_dir)), 'M142_RESTORE_ARGUMENTS')
            run(args)
    except QaError as error:
        print(json.dumps({'error': error.code}), file=sys.stderr)
        return 1
    except (OSError, KeyError, ValueError) as error:
        print(json.dumps({'error': 'M142_RESTORE_IO', 'detail': str(error)}), file=sys.stderr)
        return 1
    return 0


if __name__ == '__main__':
    sys.exit(main())
