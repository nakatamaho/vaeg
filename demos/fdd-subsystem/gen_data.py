# Copyright (c) 2026 Nakata Maho
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
# INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
# STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
# IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
"""Font, palettes and the FDD subsystem program of FDDSUB.COM (fddsub.asm):
synthetic data. The subsystem program is assembled here by a small Z80
assembler (the handful of instructions it uses).

Usage: gen_data.py OUTPUT.INC
"""

import sys

FONT = {
    ' ': '.....|.....|.....|.....|.....|.....|.....',
    '0': '.###.|#...#|#..##|#.#.#|##..#|#...#|.###.',
    '1': '..#..|.##..|..#..|..#..|..#..|..#..|.###.',
    '2': '.###.|#...#|....#|...#.|..#..|.#...|#####',
    '3': '#####|...#.|..#..|...#.|....#|#...#|.###.',
    '4': '...#.|..##.|.#.#.|#..#.|#####|...#.|...#.',
    '5': '#####|#....|####.|....#|....#|#...#|.###.',
    '6': '..##.|.#...|#....|####.|#...#|#...#|.###.',
    '7': '#####|....#|...#.|..#..|.#...|.#...|.#...',
    '8': '.###.|#...#|#...#|.###.|#...#|#...#|.###.',
    '9': '.###.|#...#|#...#|.####|....#|...#.|.##..',
    'A': '.###.|#...#|#...#|#####|#...#|#...#|#...#',
    'B': '####.|#...#|#...#|####.|#...#|#...#|####.',
    'C': '.###.|#...#|#....|#....|#....|#...#|.###.',
    'D': '###..|#..#.|#...#|#...#|#...#|#..#.|###..',
    'E': '#####|#....|#....|####.|#....|#....|#####',
    'F': '#####|#....|#....|####.|#....|#....|#....',
    'G': '.###.|#...#|#....|#.###|#...#|#...#|.####',
    'H': '#...#|#...#|#...#|#####|#...#|#...#|#...#',
    'I': '.###.|..#..|..#..|..#..|..#..|..#..|.###.',
    'J': '..###|...#.|...#.|...#.|...#.|#..#.|.##..',
    'K': '#...#|#..#.|#.#..|##...|#.#..|#..#.|#...#',
    'L': '#....|#....|#....|#....|#....|#....|#####',
    'M': '#...#|##.##|#.#.#|#.#.#|#...#|#...#|#...#',
    'N': '#...#|#...#|##..#|#.#.#|#..##|#...#|#...#',
    'O': '.###.|#...#|#...#|#...#|#...#|#...#|.###.',
    'P': '####.|#...#|#...#|####.|#....|#....|#....',
    'Q': '.###.|#...#|#...#|#...#|#.#.#|#..#.|.##.#',
    'R': '####.|#...#|#...#|####.|#.#..|#..#.|#...#',
    'S': '.####|#....|#....|.###.|....#|....#|####.',
    'T': '#####|..#..|..#..|..#..|..#..|..#..|..#..',
    'U': '#...#|#...#|#...#|#...#|#...#|#...#|.###.',
    'V': '#...#|#...#|#...#|#...#|#...#|.#.#.|..#..',
    'W': '#...#|#...#|#...#|#.#.#|#.#.#|#.#.#|.#.#.',
    'X': '#...#|#...#|.#.#.|..#..|.#.#.|#...#|#...#',
    'Y': '#...#|#...#|.#.#.|..#..|..#..|..#..|..#..',
    'Z': '#####|....#|...#.|..#..|.#...|#....|#####',
    '.': '.....|.....|.....|.....|.....|.##..|.##..',
    ':': '.....|.##..|.##..|.....|.##..|.##..|.....',
    '/': '.....|....#|...#.|..#..|.#...|#....|.....',
    '-': '.....|.....|.....|#####|.....|.....|.....',
    '+': '.....|..#..|..#..|#####|..#..|..#..|.....',
    '=': '.....|.....|#####|.....|#####|.....|.....',
    '(': '...#.|..#..|.#...|.#...|.#...|..#..|...#.',
    ')': '.#...|..#..|...#.|...#.|...#.|..#..|.#...',
    '>': '.#...|..#..|...#.|....#|...#.|..#..|.#...',
}


def font() -> bytes:
    """ASCII 32..95, 7 bytes a glyph, bit 4 the leftmost dot, XOR A5h."""
    out = bytearray()
    for code in range(32, 96):
        rows = FONT.get(chr(code), FONT[' ']).split('|')
        assert len(rows) == 7 and all(len(r) == 5 for r in rows), chr(code)
        out += bytes(0xa5 ^ sum(1 << (4 - i) for i, c in enumerate(r) if c == '#') for r in rows)
    return bytes(out)


def palette_word(g: int, r: int, b: int) -> int:
    """G at bits 15-12, R at 9-6, B at 4-1, unused low bits set when not 0."""
    return (g << 12) | ((3 if g else 0) << 10) | (r << 6) | ((1 if r else 0) << 5) | (b << 1) | (1 if b else 0)


# Set 0: G1 (unused but black). Set 1: G0, the text.
PALETTE0 = [palette_word(0, 0, 0)] * 16
PALETTE1 = [palette_word(*c) for c in (
    (0, 0, 0), (15, 15, 15), (0, 15, 0), (15, 15, 0), (12, 12, 12), (15, 0, 15), (0, 0, 0), (0, 0, 0),
    (0, 0, 0), (0, 0, 0), (0, 0, 0), (0, 0, 0), (0, 0, 0), (0, 0, 0), (0, 0, 0), (0, 0, 0))]

# ---------------------------------------------------------------------------
# The subsystem program, at FDC_ORG in the subsystem's RAM, entered by the
# monitor's command 0Dh. Commands from the main CPU, a byte each, over the
# same 8255 handshake as the monitor's (DAV, RFD, DAC; ports FCh-FFh):
#   1 n_lo n_hi data...  store n bytes at 5000h, then send AAh
#   2 n_lo n_hi          send n bytes from 5000h
#   3                    send 55h (a round trip)
#   4                    count 256-step DJNZ loops until the main CPU's ATN
#                        (port C bit 3), then send the count (low, high)
#   6 64 bytes           then send 32 bytes (a frame's exchange)
#   other                back to the monitor (JP 002Ah)
FDC_ORG = 0x4000

PROGRAM = """
start:  ld sp,7effh
main:   call recv
        cp 1
        jr z,opw
        cp 2
        jr z,opr
        cp 3
        jr z,opp
        cp 4
        jr z,opc
        cp 6
        jr z,opx
        jp 002ah
opw:    call recv
        ld c,a
        call recv
        ld b,a
        ld hl,5000h
w1:     call recv
        ld (hl),a
        inc hl
        dec bc
        ld a,b
        or c
        jr nz,w1
        ld a,0aah
        call send
        jr main
opr:    call recv
        ld c,a
        call recv
        ld b,a
        ld hl,5000h
r1:     ld a,(hl)
        call send
        inc hl
        dec bc
        ld a,b
        or c
        jr nz,r1
        jr main
opp:    ld a,55h
        call send
        jr main
opc:    ld hl,0
c1:     ld b,0
c2:     djnz c2
        inc hl
        in a,(0feh)
        bit 3,a
        jr z,c1
        ld a,l
        call send
        ld a,h
        call send
        jr main
opx:    ld b,64
x1:     call recv
        djnz x1
        ld b,32
x2:     ld a,b
        call send
        djnz x2
        jr main
recv:   ld a,0bh
        out (0ffh),a
rv1:    in a,(0feh)
        rrca
        jr nc,rv1
        ld a,0ah
        out (0ffh),a
        in a,(0fch)
        ld e,a
        ld a,0dh
        out (0ffh),a
rv2:    in a,(0feh)
        rrca
        jr c,rv2
        ld a,0ch
        out (0ffh),a
        ld a,e
        ret
send:   ld e,a
sd1:    in a,(0feh)
        bit 1,a
        jr z,sd1
        ld a,e
        out (0fdh),a
        ld a,9
        out (0ffh),a
sd2:    in a,(0feh)
        bit 2,a
        jr z,sd2
        ld a,8
        out (0ffh),a
sd3:    in a,(0feh)
        bit 2,a
        jr nz,sd3
        ret
"""

REG8 = {'b': 0, 'c': 1, 'd': 2, 'e': 3, 'h': 4, 'l': 5, '(hl)': 6, 'a': 7}
CC = {'nz': 0, 'z': 1, 'nc': 2, 'c': 3}


def number(text: str) -> int:
    text = text.strip()
    if text.endswith('h'):
        return int(text[:-1], 16)
    return int(text)


def assemble(source: str, org: int) -> bytes:
    """Two passes over the subset used above."""
    lines = []
    for raw in source.strip().splitlines():
        label, _, rest = raw.partition(':') if ':' in raw.split()[0] else ('', '', raw)
        lines.append((label.strip(), rest.strip()))
    labels = {}

    def encode(op: str, args: list, pc: int, final: bool) -> bytes:
        def target(name):
            if name in labels:
                return labels[name]
            if final:
                return number(name)
            return pc
        if op == 'ld':
            d, s = args
            if d in REG8 and s in REG8:
                return bytes([0x40 | REG8[d] << 3 | REG8[s]])
            if d in REG8:
                return bytes([0x06 | REG8[d] << 3, number(s) & 0xff])
            pairs = {'bc': 0x01, 'de': 0x11, 'hl': 0x21, 'sp': 0x31}
            v = number(s)
            return bytes([pairs[d], v & 0xff, v >> 8])
        if op == 'cp':
            return bytes([0xfe, number(args[0])])
        if op == 'or':
            return bytes([0xb0 | REG8[args[0]]])
        if op in ('jr', 'djnz'):
            if op == 'djnz':
                base, name = 0x10, args[0]
            elif len(args) == 2:
                base, name = 0x20 | CC[args[0]] << 3, args[1]
            else:
                base, name = 0x18, args[0]
            disp = target(name) - (pc + 2)
            if final and not -128 <= disp <= 127:
                raise SystemExit(f'jump too far: {op} {args}')
            return bytes([base, disp & 0xff])
        if op in ('jp', 'call'):
            v = target(args[0])
            return bytes([0xc3 if op == 'jp' else 0xcd, v & 0xff, v >> 8])
        if op == 'ret':
            return b'\xc9'
        if op == 'rrca':
            return b'\x0f'
        if op == 'inc' and args[0] == 'hl':
            return b'\x23'
        if op == 'dec' and args[0] == 'bc':
            return b'\x0b'
        if op == 'out':
            return bytes([0xd3, number(args[0].strip('()'))])
        if op == 'in':
            return bytes([0xdb, number(args[1].strip('()'))])
        if op == 'bit':
            return bytes([0xcb, 0x40 | number(args[0]) << 3 | REG8[args[1]]])
        raise SystemExit(f'unknown instruction: {op} {args}')

    for final in (False, True):
        pc = org
        out = bytearray()
        for label, text in lines:
            if label:
                labels[label] = pc
            if not text:
                continue
            op, _, rest = text.partition(' ')
            args = [a.strip() for a in rest.split(',')] if rest else []
            code = encode(op, args, pc, final)
            out += code
            pc += len(code)
    return bytes(out)


def font() -> bytes:
    """ASCII 32..95, 7 bytes a glyph, bit 4 the leftmost dot, XOR A5h."""
    out = bytearray()
    for code in range(32, 96):
        rows = FONT.get(chr(code), FONT[' ']).split('|')
        assert len(rows) == 7 and all(len(r) == 5 for r in rows), chr(code)
        out += bytes(0xa5 ^ sum(1 << (4 - i) for i, c in enumerate(r) if c == '#') for r in rows)
    return bytes(out)


def db(label: str, data: bytes) -> list:
    lines = [label + ':']
    for i in range(0, len(data), 16):
        lines.append('\t\tdb\t' + ','.join(f'{b:#04x}' for b in data[i:i + 16]))
    return lines


def include() -> str:
    program = assemble(PROGRAM, FDC_ORG)
    lines = ['; Generated by gen_data.py: synthetic data.',
             '%define PALETTE32 ' + ','.join(f'{w:#06x}' for w in PALETTE0 + PALETTE1),
             f'FDC_ORG\t\tequ\t{FDC_ORG:#06x}',
             f'FDC_BYTES\tequ\t{len(program)}',
             '\t\tjmp\tstart\t\t\t; data first']
    lines += db('FONT', font())
    lines += db('FDC_PROGRAM', program)
    return '\n'.join(lines) + '\n'


if __name__ == '__main__':
    if len(sys.argv) != 2:
        sys.exit('usage: gen_data.py OUTPUT.INC')
    with open(sys.argv[1], 'w', encoding='ascii', newline='\n') as out:
        out.write(include())
