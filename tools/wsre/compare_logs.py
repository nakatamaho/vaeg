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
"""Compare WSRLOG CSV contracts, not photographs or an inferred board identity.

Build identifiers may differ. Row order, session boundaries and all other
metadata must agree. FF and FLOAT probe results share the undriven class.
--ignore-r0-counts is solely for deterministic emulator fixtures, never a
hardware timing PASS. No private logs are uploaded or copied into the repo.
"""
import argparse
import csv
from dataclasses import dataclass
from pathlib import Path
import re
import sys

HEADER = 'stage,build,machine,khz,board,cr27,row,param,run,count,vrtc,result'


class LogError(ValueError):
    pass


@dataclass(frozen=True)
class Row:
    session: int
    program: str
    fields: tuple


def parse_log(data):
    try:
        text = data.decode('ascii')
    except UnicodeDecodeError as exc:
        raise LogError('WSRE_LOG_ENCODING') from exc
    lines = text.splitlines()
    if not lines or lines[0] != HEADER:
        raise LogError('WSRE_LOG_HEADER')
    result, seen = [], set()
    session, program = -1, ''
    for line in lines[1:]:
        if line.startswith('#'):
            match = re.fullmatch(r'# (W1PORT|W1PROBE) (\S+) ([0-9A-F]{4}) (\S+) (15|24)kHz', line)
            if match:
                session += 1
                program = match[1]
            continue
        if not line:
            raise LogError('WSRE_LOG_EMPTY_ROW')
        try:
            fields = tuple(next(csv.reader([line], strict=True)))
        except csv.Error as exc:
            raise LogError('WSRE_LOG_CSV') from exc
        if len(fields) != 12 or ','.join(fields) != line or session < 0:
            raise LogError('WSRE_LOG_FIELDS')
        stage, build, machine, khz, board, cr27, name, param, run, count, vrtc, value = fields
        if (stage != 'W1' or not re.fullmatch(r'[0-9a-fA-F]{8}|nogit000', build) or
                not re.fullmatch(r'VA|VA2/3|VA\+91|\?[0-9A-F]{4}', machine) or
                khz not in ('15', '24') or board not in ('-', 'none', 'yes') or
                not re.fullmatch(r'--|[0-9A-F]{2}', cr27) or not name or not param):
            raise LogError('WSRE_LOG_METADATA')
        if run or count or vrtc:
            if (name != 'R0' or param != 'LOOP1024' or run not in ('1', '2', '3') or
                    not re.fullmatch(r'[1-9][0-9]*', count) or vrtc != '600' or value):
                raise LogError('WSRE_LOG_TIMING')
        elif not (re.fullmatch(r'FF|FLOAT A=[0-9A-F]{4} B=[0-9A-F]{4} C=[0-9A-F]{4}|'
                              r'DRIVEN [0-9A-F]{2}|[0-9A-F]{2,4}|'
                              r'R=[0-9A-F]{2} E=[0-9A-F]{2} (PASS|FAIL)|'
                              r'PASS|FAIL [0-9]+|NO-BOARD|FAIL NO-VRTC', value)):
            raise LogError('WSRE_LOG_RESULT')
        key = (session, name, param, run)
        if key in seen:
            raise LogError('WSRE_LOG_DUPLICATE')
        seen.add(key)
        result.append(Row(session, program, fields))
    if not result:
        raise LogError('WSRE_LOG_NO_ROWS')
    for number in range(session + 1):
        group = [row for row in result if row.session == number]
        if not group or group[-1].fields[6] != 'SUM' or group[-1].fields[7] != group[-1].program:
            raise LogError('WSRE_LOG_SUMMARY')
        timings = [row for row in group if row.fields[6] == 'R0']
        if timings and [row.fields[8] for row in timings] != ['1', '2', '3']:
            raise LogError('WSRE_LOG_RUN_SET')
    return result


def probe_class(value):
    return 'UNDRIVEN' if value == 'FF' or value.startswith('FLOAT ') else value


def compare_logs(machine, vaeg, ignore_r0_counts=False):
    reports, errors = [], []
    if len(machine) != len(vaeg):
        raise LogError('WSRE_LOG_ROW_SET')
    for left, right in zip(machine, vaeg):
        a, b = left.fields, right.fields
        label = f'{left.session}:{left.program}:{a[6]}:{a[7]}:{a[8]}'
        if (left.session, left.program, a[0], a[6:9]) != (right.session, right.program, b[0], b[6:9]):
            raise LogError('WSRE_LOG_ROW_SET')
        if a[2:6] != b[2:6] or a[10] != b[10]:
            errors.append('WSRE_LOG_METADATA_MISMATCH')
            reports.append(f'{label} metadata machine={a[2:6]} vaeg={b[2:6]} DIFFERENT')
        if a[8]:
            ratio = (int(b[9]) / int(b[10])) / (int(a[9]) / int(a[10]))
            ignored = ignore_r0_counts and a[6] == 'R0'
            equal = ignored or 0.9 <= ratio <= 1.1
            reports.append(f'{label} machine={a[9]}/{a[10]} vaeg={b[9]}/{b[10]} '
                           f'ratio={ratio:.6f} ' + ('IGNORED_NOT_TIMING_PASS' if ignored else 'PASS' if equal else 'FAIL'))
            if not equal:
                errors.append('WSRE_LOG_TIMING_RATIO')
        else:
            equal = probe_class(a[11]) == probe_class(b[11])
            reports.append(f'{label} machine={a[11]} vaeg={b[11]} ' + ('EQUAL' if equal else 'DIFFERENT'))
            if not equal:
                errors.append('WSRE_LOG_CHECK_MISMATCH')
    if not ignore_r0_counts:
        for source, rows in [('machine', machine), ('vaeg', vaeg)]:
            for session in sorted({row.session for row in rows}):
                counts = [int(row.fields[9]) for row in rows if row.session == session and row.fields[8]]
                if counts and (max(counts) - min(counts)) / min(counts) > 0.01:
                    reports.append(f'{source}:{session}:R0 variation exceeds 1%')
                    errors.append('WSRE_LOG_TIMING_VARIATION')
    return reports, sorted(set(errors))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('machine', type=Path)
    parser.add_argument('vaeg', type=Path)
    parser.add_argument('--ignore-r0-counts', action='store_true')
    args = parser.parse_args()
    try:
        reports, errors = compare_logs(parse_log(args.machine.read_bytes()), parse_log(args.vaeg.read_bytes()),
                                       args.ignore_r0_counts)
    except (OSError, LogError) as exc:
        print(str(exc), file=sys.stderr)
        return 2
    print('\n'.join(reports))
    for code in errors:
        print(code, file=sys.stderr)
    return 1 if errors else 0


if __name__ == '__main__':
    sys.exit(main())
