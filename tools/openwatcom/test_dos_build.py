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

import importlib.util
import io
import json
from pathlib import Path
import subprocess
import tempfile
import unittest
from unittest import mock
import warnings
import zipfile

ROOT = Path(__file__).resolve().parent
SPEC = importlib.util.spec_from_file_location('dos_build', ROOT / 'dos-build.py')
ow = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(ow)


def fixture_zip(mutation=None):
    entries = [('binw/' + n, b'MZtool' if n.endswith('.exe') else b'link options')
               for n in ow.TOOLS]
    if mutation == 'missing':
        entries.pop()
    elif mutation == 'duplicate':
        entries.append(entries[0])
    elif mutation == 'invalid':
        entries[0] = (entries[0][0], b'XXtool')
    result = io.BytesIO()
    with warnings.catch_warnings():
        warnings.simplefilter('ignore', UserWarning)
        with zipfile.ZipFile(result, 'w') as z:
            for name, data in entries:
                z.writestr(name, data)
    result.seek(0)
    return result


def result_fixture(folder):
    data = {'OWPASS.TXT': b'VAEG_OW_OK\r\n', 'VABUILD.OBJ': b'object',
            'VAOUT.COM': b'generated COM', 'ASM.LOG': b'assembler OK',
            'LINK.LOG': b'linker OK'}
    for name, contents in data.items():
        (folder / name).write_bytes(contents)


class Tests(unittest.TestCase):
    def code(self, expected, fn, *args):
        with self.assertRaises(ow.BuildError) as error:
            fn(*args)
        self.assertEqual(error.exception.code, 'OWDOS_' + expected)

    def test_digest(self):
        ow.check_digest('a' * 64, 'a' * 64)
        self.code('ARCHIVE_HASH', ow.check_digest, 'b' + 'a' * 63, 'a' * 64)

    def test_names(self):
        self.assertEqual(ow.dos_name('VIEW480.ASM'), 'VIEW480.ASM')
        for value in ('../x.asm', 'x.asm & echo', 'longfilename.asm', 'a.bat\nx'):
            with self.subTest(value=value):
                self.code('NAME', ow.dos_name, value)

    def test_mount(self):
        self.assertTrue(ow.mount_command('c', Path('fixture with spaces')).endswith('"'))
        self.code('MOUNT_PATH', ow.mount_command, 'c', Path('fixture"quote'))

    def test_tools(self):
        with zipfile.ZipFile(fixture_zip()) as z:
            self.assertEqual(set(ow.tool_contents(z)), set(ow.TOOLS))
        for mutation in ('missing', 'duplicate', 'invalid'):
            with self.subTest(mutation=mutation), zipfile.ZipFile(fixture_zip(mutation)) as z:
                self.code('TOOLS', ow.tool_contents, z)

    def test_result_mutations(self):
        for mutation, code in (('marker', 'GUEST_STATUS'), ('failure', 'GUEST_STATUS'),
                               ('object', 'OUTPUT'), ('empty-log', 'OUTPUT')):
            with self.subTest(mutation=mutation), tempfile.TemporaryDirectory() as tmp:
                folder = Path(tmp)
                result_fixture(folder)
                self.assertEqual(ow.result_bytes(folder), b'generated COM')
                if mutation == 'marker':
                    (folder / 'OWPASS.TXT').write_bytes(b'wrong marker')
                elif mutation == 'failure':
                    (folder / 'OWFAIL.TXT').write_bytes(b'VAEG_OW_FAIL')
                elif mutation == 'object':
                    (folder / 'VABUILD.OBJ').unlink()
                else:
                    (folder / 'ASM.LOG').write_bytes(b'')
                self.code(code, ow.result_bytes, folder)

    def test_composed_build(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            source = root / 'input'
            source.mkdir()
            (source / 'VIEW480.ASM').write_bytes(b'; synthetic source\n')
            archive = root / 'tools.zip'
            archive.write_bytes(fixture_zip().read())
            worker = root / 'dosbox'
            worker.write_bytes(b'synthetic launcher identity')
            evidence = root / 'evidence'
            evidence.mkdir()

            def run(cmd, **kwargs):
                work = kwargs['cwd']
                self.assertIn('-noautoexec', cmd)
                self.assertIn('call VABUILD.BAT', cmd)
                self.assertIn('set PATH=D:\\BINW', cmd)
                batch = (work / 'VABUILD.BAT').read_bytes()
                self.assertEqual(batch.count(b'if errorlevel 1 goto fail'), 2)
                result_fixture(work)
                return subprocess.CompletedProcess(cmd, 0)

            with mock.patch.object(ow, 'ARCHIVE_SHA256', ow.digest(archive)), \
                    mock.patch.object(ow.shutil, 'which', return_value=str(worker)), \
                    mock.patch.object(ow.tempfile, 'mkdtemp', return_value=str(evidence)), \
                    mock.patch.object(ow.subprocess, 'run', side_effect=run) as execute:
                output = ow.build(source, 'VIEW480.ASM', 'view480.com', archive, 'dosbox')
                self.assertEqual(output.read_bytes(), b'generated COM')
                self.assertEqual((source / 'VIEW480.ASM').read_bytes(), b'; synthetic source\n')
                record = json.loads((evidence / 'build.json').read_text())
                self.assertEqual(record['status'], 'PASS_ASSEMBLE_LINK_NOT_RUNTIME')
                self.code('EXISTS', ow.build, source, 'VIEW480.ASM', 'view480.com', archive, 'dosbox')
                self.assertEqual(execute.call_count, 1)

    def test_archive_pin_matches_container(self):
        text = (ROOT / 'containerfile').read_text()
        self.assertIn('ARG OW_SHA256=' + ow.ARCHIVE_SHA256, text)


if __name__ == '__main__':
    unittest.main()
