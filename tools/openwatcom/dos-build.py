#!/usr/bin/env python3
"""
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
"""

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
import zipfile

# The same complete release archive as containerfile, including the DOS tools.
ARCHIVE_SHA256 = 'e1bc4e88fa47191118f29e53731e7f4542803c7f4c503e15d72f1f571ac0832f'
TOOLS = ('wasm.exe', 'wlink.exe', 'wlink.lnk', 'dos4gw.exe')
RESERVED = ('VABUILD.OBJ', 'VAOUT.COM', 'VABUILD.BAT', 'OWPASS.TXT', 'OWFAIL.TXT',
            'ASM.LOG', 'LINK.LOG')


class BuildError(ValueError):
    def __init__(self, code, detail):
        self.code = 'OWDOS_' + code
        super().__init__(self.code + ': ' + detail)


def require(ok, code, detail):
    if not ok:
        raise BuildError(code, detail)


def digest(path):
    h = hashlib.sha256()
    with path.open('rb') as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b''):
            h.update(block)
    return h.hexdigest()


def check_digest(actual, expected):
    require(actual == expected, 'ARCHIVE_HASH', 'wrong pinned Open Watcom archive')


def dos_name(name):
    require(re.fullmatch(r'[a-zA-Z0-9_]{1,8}\.[a-zA-Z0-9_]{1,3}', name) is not None,
            'NAME', 'source/output must be a simple DOS 8.3 filename')
    return name


def mount_command(letter, path):
    value = str(path.resolve())
    require(not any(c in value for c in '\r\n"'), 'MOUNT_PATH', 'unsafe DOS mount path')
    return 'mount ' + letter + ' "' + value + '"'


def tool_contents(archive):
    """Closed extraction; never interpret member-supplied destination paths."""
    result = {}
    names = archive.namelist()
    for name in TOOLS:
        member = 'binw/' + name
        require(names.count(member) == 1, 'TOOLS', 'missing/duplicate ' + member)
        data = archive.read(member)
        require(bool(data) and (not name.endswith('.exe') or data[:2] == b'MZ'),
                'TOOLS', 'invalid ' + member)
        result[name] = data
    return result


def result_bytes(work):
    # DOSBox's process exit alone does not report assembler/linker failure.
    files = {p.name.upper(): p for p in work.iterdir() if p.is_file()}
    require('OWFAIL.TXT' not in files and 'OWPASS.TXT' in files and
            files['OWPASS.TXT'].read_bytes().strip() == b'VAEG_OW_OK',
            'GUEST_STATUS', 'DOS batch did not attest both compiler stages')
    require(all(n in files and files[n].stat().st_size > 0
                for n in ('VABUILD.OBJ', 'VAOUT.COM', 'ASM.LOG', 'LINK.LOG')),
            'OUTPUT', 'missing/empty object, COM or build log')
    return files['VAOUT.COM'].read_bytes()


def build(source_dir, source, output, archive, dosbox):
    dos_name(source)
    dos_name(output)
    source_dir = source_dir.resolve(strict=True)
    destination = source_dir / output
    require(not any(p.name.casefold() == output.casefold() for p in source_dir.iterdir()),
            'EXISTS', 'output already exists')
    require((source_dir / source).is_file(), 'SOURCE', 'source is not a file')
    require(not any(p.is_symlink() for p in source_dir.rglob('*')),
            'SOURCE_LINK', 'source tree must not contain symlinks')
    check_digest(digest(archive), ARCHIVE_SHA256)
    with zipfile.ZipFile(archive) as package:
        tools = tool_contents(package)
    worker = shutil.which(dosbox)
    require(worker is not None, 'WORKER', 'DOSBox is not executable')
    # Retain source copy, tool identities, config, logs and manifest for audit.
    root = Path(tempfile.mkdtemp(prefix='vaeg-watcom-dos-')).resolve()
    work = root / 'source'
    shutil.copytree(source_dir, work)
    require(not any(p.name.upper() in RESERVED for p in work.iterdir()),
            'RESERVED', 'source tree conflicts with isolated build filenames')
    sdk = root / 'sdk'
    (sdk / 'binw').mkdir(parents=True)
    for name, data in tools.items():
        (sdk / 'binw' / name).write_bytes(data)
    batch = ('@echo off\r\n'
             'wasm -zcm=tasm -fo=VABUILD.OBJ ' + source + ' > ASM.LOG\r\n'
             'if errorlevel 1 goto fail\r\n'
             'if not exist VABUILD.OBJ goto fail\r\n'
             'wlink format dos com name VAOUT.COM file VABUILD.OBJ > LINK.LOG\r\n'
             'if errorlevel 1 goto fail\r\n'
             'if not exist VAOUT.COM goto fail\r\n'
             'echo VAEG_OW_OK>OWPASS.TXT\r\n'
             'goto end\r\n:fail\r\necho VAEG_OW_FAIL>OWFAIL.TXT\r\n:end\r\n')
    (work / 'VABUILD.BAT').write_bytes(batch.encode('ascii'))
    config = root / 'dosbox.conf'
    config.write_text('[sdl]\noutput=surface\n[render]\nscaler=none\n'
                      '[cpu]\ncore=normal\ncycles=fixed 20000\n'
                      '[midi]\nmpu401=none\nmididevice=none\n'
                      '[sblaster]\nsbtype=none\n[gus]\ngus=false\n'
                      '[speaker]\npcspeaker=false\ntandy=off\ndisney=false\n')
    cmd = [worker, '-conf', str(config), '-noautoexec', '-exit']
    for command in (mount_command('c', work), mount_command('d', sdk), 'c:',
                    'set PATH=D:\\BINW', 'set WATCOM=D:\\', 'call VABUILD.BAT', 'exit'):
        cmd.extend(('-c', command))
    record = {'schema': 'vaeg-openwatcom-dos-build-v1', 'command': cmd,
              'archive_sha256': ARCHIVE_SHA256, 'launcher_sha256': digest(Path(worker)),
              'tools': {n: hashlib.sha256(d).hexdigest() for n, d in tools.items()},
              'source_files': {str(p.relative_to(work)): digest(p)
                               for p in sorted(work.rglob('*')) if p.is_file()},
              'status': 'INCOMPLETE'}
    print('Open Watcom DOS evidence: ' + str(root), flush=True)
    try:
        env = dict(os.environ, SDL_VIDEODRIVER='dummy', SDL_AUDIODRIVER='dummy',
                   SDL_RENDER_DRIVER='software')
        try:
            with (root / 'dosbox.log').open('w') as log:
                run = subprocess.run(cmd, cwd=work, env=env, stdout=log,
                                     stderr=subprocess.STDOUT, timeout=180)
        except subprocess.TimeoutExpired:
            record['exit'] = 'TIMEOUT'
            raise BuildError('TIMEOUT', 'DOSBox did not finish within 180 seconds')
        record['exit'] = run.returncode
        require(run.returncode == 0, 'WORKER_EXIT', 'DOSBox failed')
        data = result_bytes(work)
        # Exclusive creation; never overwrite the caller's file, even after a race.
        with destination.open('xb') as stream:
            stream.write(data)
        record.update(status='PASS_ASSEMBLE_LINK_NOT_RUNTIME', output_size=len(data),
                      output_sha256=hashlib.sha256(data).hexdigest())
    finally:
        record['outputs'] = {str(p.relative_to(root)): digest(p)
                             for p in sorted(root.rglob('*')) if p.is_file()}
        (root / 'build.json').write_text(json.dumps(record, indent=2) + '\n')
    return destination


def main():
    parser = argparse.ArgumentParser(description='Pinned DOS Open Watcom assembler/linker backend')
    parser.add_argument('--archive', required=True, type=Path)
    parser.add_argument('--source-dir', required=True, type=Path)
    parser.add_argument('--source', required=True)
    parser.add_argument('--output', required=True)
    parser.add_argument('--dosbox', default='dosbox')
    args = parser.parse_args()
    try:
        path = build(args.source_dir, args.source, args.output, args.archive, args.dosbox)
        print('OWDOS_BUILD_OK: ' + path.name + ' ' + digest(path))
    except (BuildError, OSError, zipfile.BadZipFile, subprocess.SubprocessError) as error:
        parser.exit(1, str(error) + '\n')


if __name__ == '__main__':
    main()
