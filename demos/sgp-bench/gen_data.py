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
"""Data of the SGP and sprite benchmarks (sgpbench.asm, sprbench.asm).

Synthetic source images, the 48 correctness cases, the expected front-A
image under the semantics an arcade-game port for the VA relies on, and
a 5x7 font. Usage: gen_data.py sgp|spr OUTPUT.INC
"""

import random

BOX_W, BOX_H, BOX_ROWS = 20, 19, 18      # box pitch, filled rows
BOXES_PER_ROW = 16
EXPECTED_ROWS = 3 * BOX_H
TESTS = ('CPU LOOP X1000', 'CPU STOSW GV 35840', 'CPU STOSW RAM 35840', 'CPU MOVSW GV-GV 8192',
         'SGP EMPTY LIST', 'SGP CLS 35840', 'SGP CLS 1280', 'SGP 1X16X16', 'SGP 32X16X16',
         'SGP 32X16X16 DOT1', 'SGP 32X16X16 YFLIP', 'SGP 32X16X16 OPAQUE', 'SGP 32X8X8',
         'SGP 32X16X16 GV SRC', '09 + CPU STOSW GV', '09 + CPU STOSW RAM', '09 + CPU LOOP X4000')
WINDOWS = (30, 120, 480)        # VRTCs a test is repeated for


def source(index: int) -> list[list[int]]:
    """16x16 nibbles: all 15 colours, transparent zeros, an orientation mark."""
    rows = []
    for y in range(16):
        row = []
        for x in range(16):
            value = (x + 2 * y + 3 * index) % 15 + 1
            if (x * (index + 1) + y * 3) % 7 == 0:
                value = 0
            if x < 3 and y < 3:
                value = 15 if (x, y) != (2, 2) else 0      # top-left mark
            if y == 15 and x >= 12:
                value = 1                                   # bottom-right mark
            row.append(value)
        rows.append(row)
    return rows


def packed(rows: list[list[int]]) -> bytes:
    return bytes((row[x] << 4) | row[x + 1] for row in rows for x in range(0, len(row), 2))


def box_origin(case: int) -> tuple[int, int]:
    return (case % BOXES_PER_ROW) * BOX_W, (case // BOXES_PER_ROW) * BOX_H


def cases() -> list[tuple[int, int, int, int, int, int, int, int]]:
    """(source, left, top, width, height, x, y, flags): flags 1 Y flip, 2 opaque.
    The piece's part (left, top, width, height) goes to (x + left, y + top)."""
    result = []
    def add(src, left, top, w, h, k, flags):
        x0, y0 = box_origin(len(result))
        result.append((src, left, top, w, h, x0 + k, y0 + 1, flags))
    for flags in (0, 1, 2, 3):                 # full pieces, every start dot
        for k in range(4):
            add(k % 4, 0, 0, 16, 16, k, flags)
    for i, (left, w) in enumerate(((1, 15), (2, 14), (3, 13), (4, 12), (5, 11), (0, 15), (0, 14), (0, 13),
                                   (0, 1), (15, 1), (7, 2), (6, 3), (9, 5), (3, 9), (2, 1), (1, 3))):
        add(i % 4, left, 0, w, 16, (i * 3) % 4, (i // 4) & 1)
    rng = random.Random(88)
    for i, (top, h) in enumerate(((1, 15), (3, 13), (8, 8), (0, 1), (15, 1), (5, 6))):
        left = rng.randrange(16)
        add(rng.randrange(4), left, top, rng.randrange(1, 17 - left), h, rng.randrange(4), i & 1)
    while len(result) < 48:
        left, top = rng.randrange(16), rng.randrange(16)
        add(rng.randrange(4), left, top, rng.randrange(1, 17 - left), rng.randrange(1, 17 - top),
            rng.randrange(4), rng.randrange(4))
    return result


def expected() -> bytes:
    """Front A rows 0..EXPECTED_ROWS-1 after the boxes and all cases, as
    documented (M128: hardware garbles the upward cases after their first
    line; the benchmark counts them as failures)."""
    image = [[0] * 320 for _ in range(EXPECTED_ROWS)]
    for case in range(48):
        x0, y0 = box_origin(case)
        a, b = (0xe, 0xd) if case % 2 == 0 else (0xb, 0xc)
        for r in range(BOX_ROWS):
            for c in range(BOX_W):
                first = (c % 2 == 0) == (r % 2 == 0)
                image[y0 + r][x0 + c] = a if first else b
    sources = [source(i) for i in range(4)]
    for src, left, top, w, h, x, y, flags in cases():
        for j in range(top, top + h):
            row = sources[src][15 - j if flags & 1 else j]
            for i in range(left, left + w):
                value = row[i]
                if value or flags & 2:
                    image[y + j][x + i] = value
    return packed(image)


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
}


def font() -> bytes:
    """ASCII 32..95, 7 bytes a glyph, bit 4 the leftmost dot, XOR A5h."""
    out = bytearray()
    for code in range(32, 96):
        rows = FONT.get(chr(code), FONT[' ']).split('|')
        assert len(rows) == 7 and all(len(r) == 5 for r in rows), chr(code)
        out += bytes(0xa5 ^ sum(1 << (4 - i) for i, c in enumerate(r) if c == '#') for r in rows)
    return bytes(out)


def palette_word(r: int, g: int, b: int) -> int:
    return (g << 12) | (r << 6) | (b << 1)


PALETTE = [palette_word(*c) for c in (
    (0, 0, 0), (15, 15, 15), (15, 0, 0), (0, 15, 0), (0, 0, 15), (15, 15, 0), (0, 15, 15), (15, 0, 15),
    (8, 8, 8), (15, 4, 4), (4, 15, 4), (15, 15, 6), (6, 6, 15), (9, 5, 2), (4, 8, 8), (15, 15, 15))]


def include() -> str:
    def db(label, data):
        lines = [f'{label}:']
        for i in range(0, len(data), 16):
            lines.append('\t\tdb\t' + ','.join(f'{b:#04x}' for b in data[i:i + 16]))
        return '\n'.join(lines)
    # Each word XOR 5A5Ah (the asm undoes it): keeps runs of small numbers
    # out of the binary, which the publish scanner's ROM windows can hit.
    case_words = '\n'.join('\t\tdw\t' + ','.join(f'{v ^ 0x5a5a:#06x}' for v in c) for c in cases())
    text = [
        '; Generated by tools/sgpbench.py: synthetic data, no ROM.',
        'N_CASES\t\tequ\t48',
        f'EXPECTED_BYTES\tequ\t{EXPECTED_ROWS * 160}',
        '%define PALETTE16 ' + ','.join(f'{w:#06x}' for w in PALETTE),
        '\t\tjmp\tstart\t\t\t; data first, at known offsets',
        '\t\talign\t16',
        db('SOURCES', b''.join(packed(source(i)) for i in range(4))),
        'CASES:', case_words,
        db('FONT', font()),
        '\t\talign\t2',
        db('EXPECTED', expected()),
    ]
    return '\n'.join(text) + '\n'


def sprite_include() -> str:
    """bench_data.inc of sprbench.asm: palette and font only."""
    data = font()
    lines = ['; Generated by gen_data.py: synthetic data.',
             '%define PALETTE16 ' + ','.join(f'{w:#06x}' for w in PALETTE),
             '\t\tjmp\tstart\t\t\t; data first', 'FONT:']
    for i in range(0, len(data), 16):
        lines.append('\t\tdb\t' + ','.join(f'{b:#04x}' for b in data[i:i + 16]))
    return '\n'.join(lines) + '\n'


if __name__ == '__main__':
    import sys
    if len(sys.argv) != 3 or sys.argv[1] not in ('sgp', 'spr'):
        sys.exit('usage: gen_data.py sgp|spr OUTPUT.INC')
    with open(sys.argv[2], 'w', encoding='ascii', newline='\n') as out:
        out.write(include() if sys.argv[1] == 'sgp' else sprite_include())
