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
"""Synthetic W1 expected logs for a VA2, 15kHz, standard V3 DOS boot.

Not hardware measurements: CR27=98, board reads=FF, word aliases and initial
register values are provisional [OPEN]. R0 count=1 is a placeholder; fixture
comparison MUST use --ignore-r0-counts. E8h intentionally remains FF per the
specified W1 contract, not fitted to the current incompatible V3 decoder.
"""
import argparse
from pathlib import Path
import compare_logs


def expected(program, enabled):
    rows = [compare_logs.HEADER, f'# {program} VA2/3 FFFE 00000000 15kHz']
    board, cr = '-', '--'

    def emit(name, param, result='', run='', count='', vrtc=''):
        rows.append(','.join(('W1', '00000000', 'VA2/3', '15', board, cr,
                              name, param, run, count, vrtc, result)))

    def timing():
        for run in ('1', '2', '3'):
            emit('R0', 'LOOP1024', run=run, count='1', vrtc='600')

    def check(name, param, read, want):
        emit(name, param, f'R={read} E={want} ' + ('PASS' if read == want else 'FAIL'))

    if program == 'W1PORT':
        timing()
        ports = ('00E0', '00E1', '00E8', '40E0', '40E1', '44E0', '45E0',
                 '46E8', '4EE0', '4FE0', '54E0', '55E0', '5AE0', '42E1',
                 '51E1', '59E1', '5BE1')
        driven = {'40E0', '44E0', '45E0', '46E8', '4EE0', '4FE0', '54E0', '55E0', '5AE0'}
        for port in ports:
            emit('P', port, 'DRIVEN 00' if enabled and port in driven else 'FF')
        # Public memctrlva reset value at 153h is 41h; ROM-bank low byte is 00h.
        emit('B152', 'READ', '4100')
        for param, value in (('SAME', '00'), ('SLOT', '08'), ('RESTORE', '00')):
            check('B152', param, value, value)
    else:
        for port in ('45E0', '4FE0'):
            emit('PRE', port, 'DRIVEN 00' if enabled else 'FF')
        board, cr = ('yes', '98') if enabled else ('none', '--')
        if enabled:
            timing()
        check('SR06B', '12', '12' if enabled else 'FF', '12')
        check('SR06B', '00', '0F' if enabled else 'FF', '0F')
        if enabled:
            check('SR06W', '12', '12', '12')
            check('SR06W', '00', '0F', '0F')
            emit('CR27', 'ID', '98')
            for name in ('GR09B', 'GR0AB', 'GR09W', 'GR0AW'):
                for value in ('55', 'AA'):
                    check(name, value, value, value)
            for port in ('40E1', '42E1', '46E8', '51E1', '59E1', '5BE1'):
                emit('P', port, 'DRIVEN 08' if port == '46E8' else 'FF')
    emit('SUM', program, 'NO-BOARD' if program == 'W1PROBE' and not enabled else 'PASS')
    return '\n'.join(rows) + '\n'


def generate(output):
    repo = Path(__file__).resolve().parents[2]
    output = output.resolve()
    if output == repo or repo in output.parents:
        raise ValueError('WSRE_EXPECTED_OUT_OF_TREE')
    output.mkdir(parents=True, exist_ok=False)
    # A CSV comment license precedes the header; guest-produced bytes stay unchanged.
    notice = Path(__file__).read_text().split('"""', 1)[0]
    notice += '# Synthetic fixture, NOT measured hardware. CR27/board/word values [OPEN].\n'
    for program in ('W1PORT', 'W1PROBE'):
        for enabled in (False, True):
            data = (notice + expected(program, enabled)).encode('ascii')
            parsed = compare_logs.parse_log(data)
            if compare_logs.compare_logs(parsed, parsed, True)[1]:
                raise ValueError('WSRE_EXPECTED_INVALID')
            (output / f'{program.lower()}-{"on" if enabled else "off"}.csv').write_bytes(data)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    generate(parser.parse_args().output)
