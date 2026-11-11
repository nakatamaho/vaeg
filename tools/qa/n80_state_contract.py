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
# EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
# OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
# OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
# OF THE POSSIBILITY OF SUCH DAMAGE.
"""Content-only check of a local N80 live-state replay; no Git/private paths.

Input directory: capture/{baseline,expected,restored}.state plus the same three
PNG names at its root. Expected/restored must be equal-duration continuations
from baseline, as bound by the runner's separately retained commands/event log.
This validator does not establish that scheduling precondition or presenter
coverage. EVENT and BEEP are explicitly outside the equality claim.
"""
import argparse
import copy
import hashlib
import json
import struct
import zlib
from pathlib import Path

from statsave_sections import inspect

REQUIRED = {'UPD9CPU', 'UPD9002', 'UPD9Z80', 'MEMORY', 'TVRAMVA', 'GVRAMVA',
            'N80BANK', 'N80VIDEO', 'VIDEOVA'}
DYNAMIC = {'UPD9Z80', 'MEMORY', 'GVRAMVA'}
EXCLUDED = {'EVENT', 'BEEP'}


class ContractError(ValueError):
    pass


def require(condition, code):
    if not condition:
        raise ContractError(code)


def image_digest(data):
    require(data.startswith(b'\x89PNG\r\n\x1a\n'), 'N80_STATE_IMAGE_FORMAT')
    position, compressed, header, ended = 8, bytearray(), None, False
    while position + 12 <= len(data):
        size = struct.unpack_from('>I', data, position)[0]
        kind = data[position + 4:position + 8]
        end = position + 8 + size
        require(end + 4 <= len(data), 'N80_STATE_IMAGE_FORMAT')
        payload = data[position + 8:end]
        require(zlib.crc32(kind + payload) == struct.unpack_from('>I', data, end)[0],
                'N80_STATE_IMAGE_FORMAT')
        if header is None:
            require(kind == b'IHDR' and size == 13, 'N80_STATE_IMAGE_FORMAT')
            header = struct.unpack('>IIBBBBB', payload)
        elif kind == b'IDAT':
            compressed.extend(payload)
        elif kind == b'IEND':
            require(size == 0 and end + 4 == len(data), 'N80_STATE_IMAGE_FORMAT')
            ended = True
            break
        else:
            require(kind != b'IHDR' and bool(kind[0] & 32), 'N80_STATE_IMAGE_FORMAT')
        position = end + 4
    require(header is not None and ended, 'N80_STATE_IMAGE_FORMAT')
    width, height, bits, color, compression, filtering, interlace = header
    require(0 < width <= 8192 and 0 < height <= 8192 and bits == 8 and
            color in (2, 6) and (compression, filtering, interlace) == (0, 0, 0),
            'N80_STATE_IMAGE_FORMAT')
    stride = width * (3 if color == 2 else 4) + 1
    try:
        decoder = zlib.decompressobj()
        pixels = decoder.decompress(bytes(compressed), stride * height + 1)
    except zlib.error as error:
        raise ContractError('N80_STATE_IMAGE_FORMAT') from error
    require(len(pixels) == stride * height and decoder.eof and not decoder.unused_data and
            all(pixels[row * stride] <= 4 for row in range(height)), 'N80_STATE_IMAGE_FORMAT')
    return hashlib.sha256(data).hexdigest()


def validate(states, images):
    baseline, expected, restored = states
    require(REQUIRED <= baseline.keys() and REQUIRED <= expected.keys() and
            REQUIRED <= restored.keys(), 'N80_STATE_REQUIRED')
    require(baseline.keys() == expected.keys() == restored.keys(), 'N80_STATE_SECTION_SET')
    require(all(baseline[key] != expected[key] for key in DYNAMIC), 'N80_STATE_NO_PROGRESS')
    require(all(expected[key] == restored[key] for key in expected.keys() - EXCLUDED),
            'N80_STATE_REPLAY_MISMATCH')
    require(len(images) == 3 and all(images), 'N80_STATE_IMAGES_REQUIRED')
    require(images[0] != images[1], 'N80_STATE_IMAGE_NO_PROGRESS')
    require(images[1] == images[2], 'N80_STATE_IMAGE_REPLAY_MISMATCH')
    return {'status': 'PASS_DYNAMIC_MEMORY_AND_FRAME_REPLAY',
            'excluded_from_equality': sorted(EXCLUDED),
            'excluded_changed': sorted(k for k in EXCLUDED & expected.keys()
                                       if expected[k] != restored[k]),
            'presenter': 'NOT_RUN', 'sound_and_event_identity': 'NOT_QUALIFIED'}


def selftest():
    base = {key: (1, 1, 'a') for key in REQUIRED | EXCLUDED}
    expected = dict(base)
    for key in DYNAMIC:
        expected[key] = (1, 1, 'b')
    states = [base, expected, dict(expected)]
    images = ['first', 'second', 'second']
    validate(states, images)
    mutations = [
        ('N80_STATE_REQUIRED', lambda s, i: s[2].pop('N80BANK')),
        ('N80_STATE_SECTION_SET', lambda s, i: s[2].update(EXTRA=(1, 1, 'a'))),
        ('N80_STATE_NO_PROGRESS', lambda s, i: s[0].update(MEMORY=s[1]['MEMORY'])),
        ('N80_STATE_REPLAY_MISMATCH', lambda s, i: s[2].update(GVRAMVA=(1, 1, 'x'))),
        ('N80_STATE_IMAGES_REQUIRED', lambda s, i: i.__setitem__(2, '')),
        ('N80_STATE_IMAGE_NO_PROGRESS', lambda s, i: i.__setitem__(0, i[1])),
        ('N80_STATE_IMAGE_REPLAY_MISMATCH', lambda s, i: i.__setitem__(2, 'wrong')),
    ]
    for code, mutate in mutations:
        s, i = copy.deepcopy(states), list(images)
        validate(s, i)
        mutate(s, i)
        try:
            validate(s, i)
        except ContractError as error:
            require(str(error) == code, 'N80_STATE_NEGATIVE_WRONG_CODE')
        else:
            raise ContractError('N80_STATE_NEGATIVE_ACCEPTED')
    def chunk(kind, payload):
        return struct.pack('>I', len(payload)) + kind + payload + struct.pack('>I', zlib.crc32(kind + payload))
    png = (b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', 1, 1, 8, 2, 0, 0, 0)) +
           chunk(b'IDAT', zlib.compress(b'\x00\x00\x00\x00')) + chunk(b'IEND', b''))
    image_digest(png)
    changed = bytearray(png)
    changed[-1] ^= 1
    try:
        image_digest(bytes(changed))
    except ContractError as error:
        require(str(error) == 'N80_STATE_IMAGE_FORMAT', 'N80_STATE_NEGATIVE_WRONG_CODE')
    else:
        raise ContractError('N80_STATE_NEGATIVE_ACCEPTED')
    print('n80-state-contract: PASS baseline, 7 semantic negatives and PNG CRC negative')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--directory', type=Path)
    parser.add_argument('--selftest', action='store_true')
    args = parser.parse_args()
    if args.selftest:
        selftest()
        return 0
    if args.directory is None:
        parser.error('--directory or --selftest required')
    try:
        states, images = [], []
        for name in ('baseline', 'expected', 'restored'):
            rows = inspect(args.directory / 'capture' / (name + '.state'))
            states.append({r['tag']: (r['version'], r['declared_size'], r['sha256']) for r in rows})
            image = (args.directory / (name + '.png')).read_bytes()
            images.append(image_digest(image))
        print(json.dumps(validate(states, images), sort_keys=True))
        return 0
    except (OSError, ValueError) as error:
        code = str(error) if isinstance(error, ContractError) else 'N80_STATE_INPUT'
        print(json.dumps({'status': 'FAIL', 'error_code': code}))
        return 1


if __name__ == '__main__':
    raise SystemExit(main())
