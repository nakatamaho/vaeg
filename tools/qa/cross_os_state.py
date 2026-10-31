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

"""Exchange actual app-written snapshots across OS kernels; private, local QA.

Export binds an unchanged prior SAV to its writer receipt and an independently
executed, next-PC reference on the producer OS. Consume loads those exact bytes
on another OS, executes to that PC, compares guest registers/video/full BMS RAM
and resolved media, then writes a real reply SAV. No synthetic foreign paths,
SAV rewriting, GUI/window claims, Git dependency or automatic reference approval.
Generated bundles contain private states/ROM-derived data: NEVER publish them.
"""
import argparse
import configparser
import io
import json
from pathlib import Path, PurePosixPath
import platform
import re
import shutil
import struct
import sys
from types import SimpleNamespace

import runtime_state_roundtrip as qa
from screen_compare import QaError, require
from state_restore_matrix import verify_restored, BMS, DISK, RECORD

SCHEMA = 'm142-cross-os-state-v1'


def image_target(data):
    require(len(data) >= 64, 'M142_XOS_IMAGE')
    if data[:6] == b'\x7fELF\x02\x01':
        machine = struct.unpack_from('<H', data, 18)[0]
        require(machine in (62, 183), 'M142_XOS_IMAGE')
        return {'os': 'Linux', 'arch': 'x86_64' if machine == 62 else 'arm64'}
    if data[:4] == b'\xcf\xfa\xed\xfe':
        machine = struct.unpack_from('<I', data, 4)[0]
        require(machine in (0x01000007, 0x0100000c), 'M142_XOS_IMAGE')
        return {'os': 'Darwin', 'arch': 'x86_64' if machine == 0x01000007 else 'arm64'}
    if data[:2] == b'MZ':
        offset = struct.unpack_from('<I', data, 60)[0]
        require(offset + 6 <= len(data) and data[offset:offset + 4] == b'PE\0\0' and
                struct.unpack_from('<H', data, offset + 4)[0] == 0x8664, 'M142_XOS_IMAGE')
        return {'os': 'Windows', 'arch': 'x86_64'}
    raise QaError('M142_XOS_IMAGE')


def actor(worker, source):
    data = worker.read_bytes()
    require(re.fullmatch('[0-9a-f]{40}', source or '') and source.encode() in data,
            'M142_XOS_WORKER_SOURCE')
    target = image_target(data)
    require(platform.system() == target['os'], 'M142_XOS_EXECUTION_OS')
    if target['os'] == 'Windows':
        import ctypes
        require(not hasattr(ctypes.WinDLL('ntdll'), 'wine_get_version'), 'M142_XOS_WINE_NOT_WINDOWS')
    host = platform.machine().lower().replace('aarch64', 'arm64').replace('amd64', 'x86_64')
    return {**target, 'host_arch': host, 'source_commit': source, 'worker_sha256': qa.digest(worker),
            'cpu_execution': 'native' if host == target['arch'] else 'host_cpu_translation',
            'kernel': platform.release()}


def different_os(producer, consumer):
    require(producer['os'] != consumer['os'], 'M142_XOS_SAME_OS')


def reader_config(text, mode):
    if mode == 'matching':
        return text
    config = configparser.ConfigParser(interpolation=None)
    config.optionxform = str
    config.read_string(text)
    section = config['NekoProjectII']
    section['Use_BMS_'] = 'false' if mode == 'off' else 'true'
    if mode == 'small-other-port':
        section['BMS_Size'], section['BMS_Port'] = '1', '00ec'
    result = io.StringIO()
    config.write(result, space_around_delimiters=False)
    return result.getvalue()


def registers(path):
    return dict(line.split('\t', 1) for line in path.read_text().splitlines())


def next_pc(origin, expected, pc):
    require(re.fullmatch('[0-9a-fA-F]{4}:[0-9a-fA-F]{4}', pc or '') is not None,
            'M142_XOS_PC')
    cs, ip = pc.lower().split(':')
    require(expected.get('cs') == cs and expected.get('ip') == ip and
            (origin.get('cs'), origin.get('ip')) != (cs, ip), 'M142_XOS_NO_CONTINUATION')


def output_new(path):
    path = path.resolve()
    require(path != qa.ROOT and qa.ROOT not in path.parents and not path.exists(), 'M142_XOS_OUTPUT')
    path.mkdir(parents=True)
    return path


def writer_receipt(record, key, digest):
    outputs = record.get('outputs', {})
    item = outputs.get(key, {}) if isinstance(outputs, dict) else {}
    require(record.get('exit') == 0 and isinstance(item, dict) and item.get('sha256') == digest,
            'M142_XOS_WRITER_RECEIPT')


def writer_attestation(record, receipt_digest, state_digest, producer):
    if record.get('schema') == 'm142-prior-state-restore-v1':
        require(record.get('status') == 'PASS_PRIOR_FILE_BMS_AND_MEDIA_CONTRACT_NOT_CROSS_OS_OR_GUI' and
                record.get('worker_sha256') == producer['worker_sha256'] and
                record.get('evaluated_commit') == producer['source_commit'], 'M142_XOS_ATTESTATION')
        matches = [row for case in record.get('cases', {}).values() for row in case.values()
                   if row.get('status') == 'PASS_GUEST_CONTINUATION_BMS_AND_MEDIA_BINDING' and
                   row.get('run_sha256') == receipt_digest and row.get('output_sha256') == state_digest]
        require(len(matches) == 1, 'M142_XOS_ATTESTATION')
    else:
        require(record.get('schema') == SCHEMA and
                record.get('status') == 'PASS_ACTUAL_CROSS_OS_STATE_NOT_GUI_OR_UNIVERSAL_ABI' and
                record.get('consumer') == producer and
                record.get('outputs', {}).get('load/run.json') == receipt_digest and
                record.get('reply_sav_sha256') == state_digest and
                record.get('outputs', {}).get('load/capture/continued.state') == state_digest,
                'M142_XOS_ATTESTATION')


def media_identity(data, media):
    parts = qa.sections(data)
    require(BMS in parts and DISK in parts and parts[DISK][0] == 2 and
            len(parts[DISK][1]) == 13 * RECORD, 'M142_XOS_STATE')
    disk = parts[DISK][1]
    require(disk[0] and not any(disk[i] for i in range(RECORD, len(disk), RECORD)) and
            struct.unpack_from('<Q', disk, 4100)[0] == media.stat().st_size and
            disk[4108:4140] == bytes.fromhex(qa.digest(media)), 'M142_XOS_MEDIA')


def execute(worker, roms, out, config, state, media, pc):
    script = ('debug-script 1\nlimit-frame 60\n' + f'resource saved {state}\nwait-frame 5\n'
              f'load-state saved\nwait-pc {pc} 1\ncapture continued registers tvram gvram\n'
              'save-state continued\nexit\n')
    result = qa.execute(SimpleNamespace(worker=worker, roms=roms), out, config, script, media)
    require(result['exit'] == 0, 'M142_XOS_LOAD')
    verify_restored(state.read_bytes(), (out / 'capture/continued.state').read_bytes(), str(media))
    return result


def safe_key(key):
    require(isinstance(key, str) and key and '\\' not in key and ':' not in key and
            not PurePosixPath(key).is_absolute() and
            all(part not in ('', '.', '..') for part in key.split('/')), 'M142_XOS_PATH')
    return key


def validate_bundle(bundle):
    manifest = json.loads((bundle / 'manifest.json').read_text())
    require(manifest.get('schema') == SCHEMA and manifest.get('status') == 'READY_FOR_CROSS_OS_CONSUMER',
            'M142_XOS_BUNDLE')
    required = {'worker.bin', 'state.sav', 'fixture.d88', 'source-run.json', 'source-attestation.json', 'origin.registers.tsv',
                'reference/capture/continued.registers.tsv', 'reference/capture/continued.tvram.bin',
                'reference/capture/continued.gvram.bin', 'reference/capture/continued.state'}
    outputs = manifest.get('outputs')
    require(isinstance(outputs, dict) and required <= outputs.keys(), 'M142_XOS_BUNDLE')
    for key, value in outputs.items():
        relative = PurePosixPath(safe_key(key))
        path = bundle
        for part in relative.parts:
            path = path / part
            require(not path.is_symlink(), 'M142_XOS_PATH')
        require(path.is_file() and qa.digest(path) == value, 'M142_XOS_DIGEST')
    target = image_target((bundle / 'worker.bin').read_bytes())
    source = manifest['producer']
    require(all(source.get(k) == value for k, value in target.items()) and
            source.get('worker_sha256') == outputs['worker.bin'] and
            re.fullmatch('[0-9a-f]{40}', source.get('source_commit', '')) and
            source['source_commit'].encode() in (bundle / 'worker.bin').read_bytes(), 'M142_XOS_PRODUCER')
    receipt = json.loads((bundle / 'source-run.json').read_text())
    writer_receipt(receipt, manifest['source_receipt_key'], outputs['state.sav'])
    writer_attestation(json.loads((bundle / 'source-attestation.json').read_text()),
                       outputs['source-run.json'], outputs['state.sav'], source)
    media_identity((bundle / 'state.sav').read_bytes(), bundle / 'fixture.d88')
    next_pc(registers(bundle / 'origin.registers.tsv'),
            registers(bundle / 'reference/capture/continued.registers.tsv'), manifest['reference_pc'])
    return manifest


def export(args):
    worker, roms = args.worker.resolve(), args.roms.resolve()
    producer = actor(worker, args.source_commit)
    state, media, receipt = [p.resolve() for p in (args.source_state, args.media, args.source_run)]
    record = json.loads(receipt.read_text())
    key = str(state.relative_to(receipt.parent))
    require(Path(record['command'][0]).resolve() == worker, 'M142_XOS_WRITER_RECEIPT')
    writer_receipt(record, key, qa.digest(state))
    attestation = args.source_attestation.resolve()
    writer_attestation(json.loads(attestation.read_text()), qa.digest(receipt), qa.digest(state), producer)
    media_identity(state.read_bytes(), media)
    origin = state.with_suffix('.registers.tsv')
    retained = {p: qa.digest(p) for p in (worker, state, media, receipt, origin, attestation)}
    retained.update({p: qa.digest(p) for p in roms.iterdir() if p.is_file()})
    out = output_new(args.output_dir)
    for source, name in ((worker, 'worker.bin'), (state, 'state.sav'), (media, 'fixture.d88'),
                         (receipt, 'source-run.json'), (attestation, 'source-attestation.json'),
                         (origin, 'origin.registers.tsv')):
        shutil.copyfile(source, out / name)
    config = out / 'reference.cfg'
    config.write_text(record['config_before'])
    # This is an attested task-generated fixture, not an original private disk.
    # On the writer OS its recorded absolute path still exists and wins over
    # basename remapping. Use that independently supplied source binding for
    # the reference; only the foreign consumer must bind the transferred copy.
    recorded_path = qa.sections(state.read_bytes())[DISK][1][:4096].split(b'\0', 1)[0].decode('utf-8')
    require(recorded_path == str(media), 'M142_XOS_SOURCE_MEDIA_BINDING')
    execute(worker, roms, out / 'reference', config, out / 'state.sav', media, args.reference_pc)
    next_pc(registers(origin), registers(out / 'reference/capture/continued.registers.tsv'), args.reference_pc)
    manifest = {'schema': SCHEMA, 'status': 'READY_FOR_CROSS_OS_CONSUMER', 'producer': producer,
                'source_receipt_key': key, 'reference_pc': args.reference_pc,
                'config_before': record['config_before'], 'roms': {p.name: qa.digest(p) for p in roms.iterdir() if p.is_file()},
                'runner_sha256': qa.digest(Path(__file__)),
                'state_contract_sha256': qa.digest(Path(qa.__file__)),
                'restoration_contract_sha256': qa.digest(Path(sys.modules[verify_restored.__module__].__file__)),
                'outputs': {p.relative_to(out).as_posix(): qa.digest(p) for p in out.rglob('*') if p.is_file()}}
    (out / 'manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
    validate_bundle(out)
    require(retained == {p: qa.digest(p) for p in retained}, 'M142_XOS_RETAINED_INPUT_CHANGED')
    print(manifest['status'])


def consume(args):
    bundle = args.bundle.resolve()
    manifest = validate_bundle(bundle)
    worker, roms = args.worker.resolve(), args.roms.resolve()
    consumer = actor(worker, args.source_commit)
    different_os(manifest['producer'], consumer)
    require(manifest['runner_sha256'] == qa.digest(Path(__file__)) and
            manifest['state_contract_sha256'] == qa.digest(Path(qa.__file__)) and
            manifest['restoration_contract_sha256'] == qa.digest(Path(sys.modules[verify_restored.__module__].__file__)),
            'M142_XOS_CONTRACT')
    require(manifest['roms'] == {p.name: qa.digest(p) for p in roms.iterdir() if p.is_file()}, 'M142_XOS_ROMS')
    retained = {bundle / key: value for key, value in manifest['outputs'].items()}
    retained.update({worker: qa.digest(worker), bundle / 'manifest.json': qa.digest(bundle / 'manifest.json')})
    retained.update({p: qa.digest(p) for p in roms.iterdir() if p.is_file()})
    out = output_new(args.output_dir)
    for name in ('state.sav', 'fixture.d88'):
        shutil.copyfile(bundle / name, out / name)
    config = out / 'reader.cfg'
    config.write_text(reader_config(manifest['config_before'], args.reader_bms))
    initial = {out / name: qa.digest(out / name) for name in ('state.sav', 'fixture.d88')}
    execute(worker, roms, out / 'load', config, out / 'state.sav', out / 'fixture.d88', manifest['reference_pc'])
    ref, actual = bundle / 'reference/capture', out / 'load/capture'
    qa.compare_registers(registers(ref / 'continued.registers.tsv'), registers(actual / 'continued.registers.tsv'))
    require(qa.tvram_content((ref / 'continued.tvram.bin').read_bytes()) ==
            qa.tvram_content((actual / 'continued.tvram.bin').read_bytes()) and
            (ref / 'continued.gvram.bin').read_bytes() == (actual / 'continued.gvram.bin').read_bytes(),
            'M142_XOS_GUEST_MEMORY')
    next_pc(registers(bundle / 'origin.registers.tsv'), registers(actual / 'continued.registers.tsv'), manifest['reference_pc'])
    require(initial == {p: qa.digest(p) for p in initial} and retained == {p: qa.digest(p) for p in retained},
            'M142_XOS_RETAINED_INPUT_CHANGED')
    result = {'schema': SCHEMA, 'status': 'PASS_ACTUAL_CROSS_OS_STATE_NOT_GUI_OR_UNIVERSAL_ABI',
              'producer': manifest['producer'], 'consumer': consumer, 'reader_bms': args.reader_bms,
              'bundle_manifest_sha256': qa.digest(bundle / 'manifest.json'), 'runner_sha256': qa.digest(Path(__file__)),
              'source_sav_sha256': qa.digest(out / 'state.sav'), 'reply_sav_sha256': qa.digest(actual / 'continued.state'),
              'continuation_pc': manifest['reference_pc'],
              'outputs': {p.relative_to(out).as_posix(): qa.digest(p) for p in out.rglob('*') if p.is_file()}}
    (out / 'result.json').write_text(json.dumps(result, indent=2) + '\n')
    print(result['status'])


def selftest():
    elf = bytearray(64);elf[:6] = b'\x7fELF\x02\x01';struct.pack_into('<H', elf, 18, 183)
    mach = bytearray(64);mach[:4] = b'\xcf\xfa\xed\xfe';struct.pack_into('<I', mach, 4, 0x0100000c)
    require(image_target(elf) == {'os': 'Linux', 'arch': 'arm64'} and
            image_target(mach) == {'os': 'Darwin', 'arch': 'arm64'}, 'M142_XOS_SELFTEST')
    origin, expected = {'cs': '2000', 'ip': '086b'}, {'cs': '2000', 'ip': '086c'}
    receipt = {'exit': 0, 'outputs': {'capture/continued.state': {'sha256': 'a' * 64}}}
    producer = {'worker_sha256': 'a' * 64, 'source_commit': 'e' * 40}
    prior = {'schema': 'm142-prior-state-restore-v1',
             'status': 'PASS_PRIOR_FILE_BMS_AND_MEDIA_CONTRACT_NOT_CROSS_OS_OR_GUI',
             'worker_sha256': 'a' * 64, 'evaluated_commit': 'e' * 40,
             'cases': {'fixture': {'load': {'status': 'PASS_GUEST_CONTINUATION_BMS_AND_MEDIA_BINDING',
                                          'run_sha256': 'b' * 64, 'output_sha256': 'c' * 64}}}}
    reply = {'schema': SCHEMA, 'status': 'PASS_ACTUAL_CROSS_OS_STATE_NOT_GUI_OR_UNIVERSAL_ABI',
             'consumer': producer, 'reply_sav_sha256': 'c' * 64,
             'outputs': {'load/run.json': 'b' * 64, 'load/capture/continued.state': 'c' * 64}}
    for action, code in ((lambda: different_os(image_target(elf), image_target(elf)), 'M142_XOS_SAME_OS'),
                         (lambda: next_pc(expected, expected, '2000:086c'), 'M142_XOS_NO_CONTINUATION'),
                         (lambda: next_pc(origin, expected, 'bad'), 'M142_XOS_PC'),
                         (lambda: safe_key('../escape'), 'M142_XOS_PATH'),
                         (lambda: safe_key('/escape'), 'M142_XOS_PATH'),
                         (lambda: safe_key('a\\b'), 'M142_XOS_PATH'),
                         (lambda: safe_key('C:/escape'), 'M142_XOS_PATH'),
                         (lambda: image_target(bytes([elf[0] ^ 1]) + elf[1:]), 'M142_XOS_IMAGE'),
                         (lambda: writer_receipt({**receipt, 'exit': 1}, 'capture/continued.state', 'a' * 64),
                          'M142_XOS_WRITER_RECEIPT'),
                         (lambda: writer_receipt(receipt, 'capture/continued.state', 'b' * 64),
                          'M142_XOS_WRITER_RECEIPT'),
                         (lambda: writer_receipt(receipt, 'capture/absent.state', 'a' * 64),
                          'M142_XOS_WRITER_RECEIPT'),
                         (lambda: writer_attestation({**prior, 'worker_sha256': 'f' * 64},
                                                     'b' * 64, 'c' * 64, producer), 'M142_XOS_ATTESTATION'),
                         (lambda: writer_attestation({**prior, 'cases': {}},
                                                     'b' * 64, 'c' * 64, producer), 'M142_XOS_ATTESTATION'),
                         (lambda: writer_attestation({**reply, 'reply_sav_sha256': 'd' * 64},
                                                     'b' * 64, 'c' * 64, producer), 'M142_XOS_ATTESTATION')):
        different_os(image_target(elf), image_target(mach))
        next_pc(origin, expected, '2000:086c')
        safe_key('reference/capture/continued.state')
        writer_receipt(receipt, 'capture/continued.state', 'a' * 64)
        writer_attestation(prior, 'b' * 64, 'c' * 64, producer)
        writer_attestation(reply, 'b' * 64, 'c' * 64, producer)
        try:
            action()
        except QaError as error:
            require(error.code == code, 'M142_XOS_SELFTEST_MASKED')
        else:
            raise QaError('M142_XOS_SELFTEST_ACCEPTED')
    print('cross-os-state: PASS (14 exact-code single-mutation content negatives)')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest='command', required=True)
    commands.add_parser('selftest')
    for name in ('export', 'consume'):
        p = commands.add_parser(name)
        for key in ('worker', 'roms', 'output-dir'):
            p.add_argument('--' + key, type=Path, required=True)
        p.add_argument('--source-commit', required=True)
        if name == 'export':
            for key in ('source-state', 'source-run', 'source-attestation', 'media'):
                p.add_argument('--' + key, type=Path, required=True)
            p.add_argument('--reference-pc', required=True)
        else:
            p.add_argument('--bundle', type=Path, required=True)
            p.add_argument('--reader-bms', choices=('matching', 'off', 'small-other-port'), default='matching')
    args = parser.parse_args()
    try:
        {'selftest': selftest, 'export': lambda: export(args), 'consume': lambda: consume(args)}[args.command]()
    except QaError as error:
        print(json.dumps({'error': error.code}), file=sys.stderr)
        return 1
    except (OSError, ValueError, KeyError) as error:
        print(json.dumps({'error': 'M142_XOS_IO', 'detail': str(error)}), file=sys.stderr)
        return 1
    return 0


if __name__ == '__main__':
    sys.exit(main())
