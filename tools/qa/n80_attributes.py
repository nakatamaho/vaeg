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
"""Synthetic BASIC-generated 3301 attributes and independent glyph oracle.

64 cells: all SECRET/blink/reverse/upper/under combinations for ordinary ANK
and semigraphics. Fixed 200-line, 40/80-column contract, blank odd scanlines,
white text and character code 65. Pixel comparisons are guest-buffer only.
The fixed DMA base is part of this fixture, not guessed from a passing image.
"""
import argparse
import json
from pathlib import Path


class AttributeErrorCode(ValueError):
    def __init__(self, code, details=None):
        super().__init__(code)
        self.details = details


def require(condition, code):
    if not condition:
        raise AttributeErrorCode(code)


def input_script(model, columns, global_reverse=False):
    require(model in ('mkii', 'sr') and columns in (40, 80), 'N80_ATTR_ARGUMENTS')
    address = 62208 if model == 'mkii' else '&HF3C8'
    clear = 'CMD CLS' if model == 'mkii' else 'CLS'
    lines = [f'WIDTH {columns}', f'10 {clear}:FOR R=0 TO 7:B={address}+120*(R+8)',
             '20 FOR I=0 TO 79:POKE B+I,65:NEXT I',
             '30 FOR I=0 TO 19:POKE B+80+I*2,80:POKE B+81+I*2,0:NEXT I',
             '40 POKE B+80,0:POKE B+81,232+16*INT(R/4)',
             '50 FOR C=0 TO 7:N=8*(R-4*INT(R/4))+C:A=(N AND 7)+2*(N AND 24)',
             '60 POKE B+82+C*2,2+C*8:POKE B+83+C*2,A:NEXT C',
             '70 POKE B+98,66:POKE B+99,0:POKE B+100,68:POKE B+101,232:NEXT R',
             ('80 OUT &H51,&H21:PRINT "ATTRDONE"' if global_reverse else '80 PRINT "ATTRDONE"'), 'RUN']
    return '@wait 5200\n@enter\n@wait 300\n' + ''.join('@text '+line+'\n@enter\n@wait 60\n' for line in lines)


def graphics_script(model):
    require(model in ('mkii', 'sr'), 'N80_ATTR_ARGUMENTS')
    prefix, color = ('CMD ', 3) if model == 'mkii' else ('', 7)
    lines = [prefix + 'SCREEN 1', prefix + f'PSET (300,150),{color}',
             prefix + f'LINE (250,130)-(300,150),{color},BF', 'PRINT "GFXDONE"']
    return '@wait 5200\n@enter\n@wait 300\n' + ''.join('@text '+line+'\n@enter\n@wait 120\n' for line in lines)


def rectangle():
    return bytes(int(250 <= x <= 300 and 282 <= y <= 322 and y % 2 == 0)
                 for y in range(280, 325) for x in range(248, 303))


def validate_rectangle(pixels):
    require(pixels == rectangle(), 'N80_GRAPHICS_PIXELS')
    return {'status': 'PASS_BASIC_RECTANGLE_ONLY', 'guest_box': [250, 130, 300, 150],
            'presenter': 'NOT_RUN'}


def expected(glyph, columns, blink_off, global_reverse=False):
    require(len(glyph) == 8 and columns in (40, 80), 'N80_ATTR_ARGUMENTS')
    pixels = bytearray(512 * 160)
    scale = 2 if columns == 40 else 1
    for group in range(8):
        for cell in range(8):
            flags = (group % 4) * 8 + cell
            secret, blink, reverse, upper, under = (bool(flags & (1 << bit)) for bit in range(5))
            for y in range(10):
                if group < 4:
                    bits = glyph[y] if y < 8 else 0
                else:
                    bits = 0xf0 if y < 2 else 0x0f if 4 <= y < 6 else 0
                if secret or (blink and blink_off):
                    bits = 0
                if (upper and y == 0) or (under and y == 9):
                    bits = 255
                if reverse != global_reverse:
                    bits ^= 255
                for x in range(64):
                    pixels[(group * 20 + y * 2) * 512 + cell * 64 + x] = (bits >> (7 - (x // scale) % 8)) & 1
    return bytes(pixels)


def validate(frames, glyph, columns, global_reverse=False):
    on, off = (expected(glyph, columns, phase, global_reverse) for phase in (False, True))
    require(bool(frames), 'N80_ATTR_FRAMES_REQUIRED')
    for index, frame in enumerate(frames):
        if frame not in (on, off):
            mismatch = min(sum(a != b for a, b in zip(frame, ref)) for ref in (on, off))
            raise AttributeErrorCode('N80_ATTR_PIXELS', {'frame': index, 'mismatched_pixels': mismatch})
    require(on in frames and off in frames, 'N80_ATTR_BLINK_PHASES')
    return {'status': 'PASS_64_ATTRIBUTE_CELLS_BOTH_BLINK_PHASES', 'columns': columns,
            'global_reverse': global_reverse, 'presenter': 'NOT_RUN'}


def selftest():
    glyph = bytes([24, 36, 66, 126, 66, 66, 66, 0])
    for columns in (40, 80):
        frames = [expected(glyph, columns, phase) for phase in (False, True)]
        validate(frames, glyph, columns)
        reversed_frames = []
        for frame in frames:
            inverted = bytes(value ^ (1 if (i // 512) % 2 == 0 else 0)
                             for i, value in enumerate(frame))
            reversed_frames.append(inverted)
        validate(reversed_frames, glyph, columns, True)
        wrong = bytearray(frames[0]); wrong[0] ^= 1
        try:
            validate([bytes(wrong), frames[1]], glyph, columns)
        except AttributeErrorCode as error:
            require(str(error) == 'N80_ATTR_PIXELS', 'N80_ATTR_NEGATIVE_WRONG_CODE')
        else:
            raise AttributeErrorCode('N80_ATTR_NEGATIVE_ACCEPTED')
        try:
            validate([frames[0]], glyph, columns)
        except AttributeErrorCode as error:
            require(str(error) == 'N80_ATTR_BLINK_PHASES', 'N80_ATTR_NEGATIVE_WRONG_CODE')
        else:
            raise AttributeErrorCode('N80_ATTR_NEGATIVE_ACCEPTED')
    validate_rectangle(rectangle())
    wrong = bytearray(rectangle()); wrong[0] ^= 1
    try:
        validate_rectangle(bytes(wrong))
    except AttributeErrorCode as error:
        require(str(error) == 'N80_GRAPHICS_PIXELS', 'N80_ATTR_NEGATIVE_WRONG_CODE')
    else:
        raise AttributeErrorCode('N80_ATTR_NEGATIVE_ACCEPTED')
    print('n80-attributes: oracle fixtures and 5 exact-code negatives PASS; guest NOT_RUN')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--model', choices=['mkii', 'sr'], default='mkii')
    parser.add_argument('--columns', type=int, choices=[40, 80], default=40)
    parser.add_argument('--write-input', type=Path)
    parser.add_argument('--graphics', action='store_true')
    parser.add_argument('--global-reverse', action='store_true')
    parser.add_argument('--font', type=Path)
    parser.add_argument('--frames', type=Path, nargs='+')
    parser.add_argument('--selftest', action='store_true')
    args = parser.parse_args()
    try:
        if args.selftest:
            selftest()
        elif args.write_input:
            require(not args.write_input.exists(), 'N80_ATTR_OUTPUT_EXISTS')
            args.write_input.write_text(graphics_script(args.model) if args.graphics else
                                       input_script(args.model, args.columns, args.global_reverse), encoding='ascii')
        else:
            require(bool(args.frames) and (args.graphics or args.font is not None), 'N80_ATTR_ARGUMENTS')
            from PIL import Image
            font = b'' if args.graphics else args.font.read_bytes()
            require(args.graphics or len(font) == 2048, 'N80_ATTR_FONT_SIZE')
            frames = []
            for path in args.frames:
                with Image.open(path) as image:
                    require(image.size == (640, 422), 'N80_ATTR_IMAGE_SIZE')
                    area = image.crop((248, 280, 303, 325) if args.graphics else (16, 182, 528, 342)).convert('RGB')
                raw = area.tobytes()
                colors = list(zip(raw[::3], raw[1::3], raw[2::3]))
                require(all(c in ((0, 0, 0), (255, 254, 255)) for c in colors), 'N80_ATTR_COLOR')
                frames.append(bytes(int(c != (0, 0, 0)) for c in colors))
            if args.graphics:
                for frame in frames:
                    result = validate_rectangle(frame)
                print(json.dumps(result))
            else:
                print(json.dumps(validate(frames, font[65*8:66*8], args.columns, args.global_reverse)))
        return 0
    except (OSError, ValueError) as error:
        code = str(error) if isinstance(error, AttributeErrorCode) else 'N80_ATTR_INPUT'
        print(json.dumps({'status': 'FAIL', 'error_code': code, 'details': getattr(error, 'details', None)}))
        return 1


if __name__ == '__main__':
    raise SystemExit(main())
