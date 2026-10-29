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
"""Content-only SCF/CCF reduction; software references are not silicon oracles.

Inputs and output belong out of tree. No downloading, emulator execution, Git
history checks, physical-Q inference, or conformance acceptance occurs.
"""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
import results

SST_COMMIT = 'ebe1875d48f374bcfd4b505d8eb8ee751568b5f7'
SST_DIGESTS = {
    'S': '3cb7ca94c400154112fd03ead580f5e14146da9ea21dfca5521cdfe28e4c81cc',
    'C': '2792a4da5dc5cbde9e808bac618cf1cd23ff5cbb0b132733eca037840d96ad4f',
}


def flag_result(op: str, a: int, f: int, xy: int) -> int:
    results.need(op in ('S', 'C'), 'M140_REDUCTION_OPERATION')
    return (a << 8) | (f & 0xc4) | (xy & 0x28) | (
        1 if op == 'S' else (0x10 if f & 1 else 1))


def reference_content(records: list, op: str, expected_count: int) -> dict:
    """Check only the A/F/Q slice, not cycles, buses or all SST registers."""
    results.need(isinstance(records, list) and len(records) == expected_count,
                 'M140_REDUCTION_REFERENCE_COUNT')
    counts = {'records': len(records), 'a_only_mismatches': 0, 'q_zero': 0,
              'q_nonzero_equal_f': 0}
    names = set()
    for record in records:
        try:
            name = record['name']
            initial, final = record['initial'], record['final']
            a, f, q = (initial[k] for k in ('a', 'f', 'q'))
            aa, ff, qq = (final[k] for k in ('a', 'f', 'q'))
            valid = isinstance(name, str) and all(
                type(x) is int and 0 <= x <= 255 for x in (a, f, q, aa, ff, qq))
        except (KeyError, TypeError, ValueError):
            valid = False
        results.need(valid, 'M140_REDUCTION_REFERENCE_FORMAT')
        results.need(name not in names, 'M140_REDUCTION_REFERENCE_DUPLICATE')
        names.add(name)
        results.need(q in (0, f), 'M140_REDUCTION_REFERENCE_Q_DOMAIN')
        predicted = flag_result(op, a, f, (f ^ q) | a)
        results.need((aa << 8 | ff) == predicted and qq == ff,
                     'M140_REDUCTION_REFERENCE_AF')
        counts['q_zero' if q == 0 else 'q_nonzero_equal_f'] += 1
        counts['a_only_mismatches'] += (aa << 8 | ff) != flag_result(op, a, f, a)
    return counts


def reference_input(data: bytes, op: str, digest: str, expected_count: int) -> dict:
    """Identity then content; focused content tests need not satisfy a hash."""
    results.need(hashlib.sha256(data).hexdigest() == digest,
                 'M140_REDUCTION_REFERENCE_DIGEST')
    try:
        records = json.loads(data)
    except (ValueError, UnicodeError):
        raise results.RecordError('M140_REDUCTION_REFERENCE_FORMAT') from None
    return reference_content(records, op, expected_count)


def history_content(data: bytes, profile: str) -> dict:
    validated = results.history(data, profile=profile)
    counts = {'records': validated['records'], 'a_only_mismatches': 0,
              'unconditional_or_mismatches': 0,
              'software_history_post_mismatches': 0,
              'software_history_pre_mismatches': 0, 'outside_both': []}
    for row in results.lines(data, True):
        if not row.startswith('Q '):
            continue
        _, h, op, requested, before, after = row.split()
        before, after = int(before, 16), int(after, 16)
        a, f = before >> 8, before & 255
        plain = flag_result(op, a, f, a)
        merged = flag_result(op, a, f, a | f)
        counts['a_only_mismatches'] += after != plain
        counts['unconditional_or_mismatches'] += after != merged
        # MAME-style software prediction only: POP AF is not a Q flag writer;
        # OR/AND/CP/SCF/CCF are. This does not measure the physical Q latch.
        h = int(h)
        counts['software_history_post_mismatches'] += after != (merged if h <= 2 else plain)
        requested_af = int(requested, 16)
        pre_candidate = results.history_before(requested_af, h)
        if h >= 6:
            ra, rf = requested_af >> 8, requested_af & 255
            pre_candidate = flag_result('S' if h == 6 else 'C', ra, rf, ra | rf)
        counts['software_history_pre_mismatches'] += before != pre_candidate
        if after not in (plain, merged):
            counts['outside_both'].append({
                'history': int(h), 'operation': op, 'requested_af': requested,
                'before_af': f'{before:04X}', 'after_af': f'{after:04X}',
                'a_only_af': f'{plain:04X}', 'or_af': f'{merged:04X}',
            })
    return counts


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--scf-reference', type=Path, required=True)
    parser.add_argument('--ccf-reference', type=Path, required=True)
    parser.add_argument('--capture', nargs=3, action='append', required=True,
                        metavar=('CASE_ID', 'PROFILE', 'LOG'))
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    references = {
        op: reference_input(path.read_bytes(), op, SST_DIGESTS[op], 1000)
        for op, path in (('S', args.scf_reference), ('C', args.ccf_reference))
    }
    captures = {}
    for case, profile, path in args.capture:
        key = case + '/' + profile
        results.need(key not in captures, 'M140_REDUCTION_CAPTURE_DUPLICATE')
        data = Path(path).read_bytes()
        captures[key] = {'sha256': hashlib.sha256(data).hexdigest(),
                         **history_content(data, profile)}
    report = {'status': 'COMPLETE_ANALYSIS_NOT_CONFORMANCE',
              'reference_class': 'independent software corpus, not Zilog silicon',
              'sst_commit': SST_COMMIT, 'references': references, 'captures': captures}
    args.output.write_text(json.dumps(report, indent=2) + '\n')
    print('M140_REDUCTION_COMPLETE (not silicon conformance acceptance)')


if __name__ == '__main__':
    try:
        main()
    except results.RecordError as exc:
        raise SystemExit(exc.code)
