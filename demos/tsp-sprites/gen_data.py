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
"""Font, palettes and sprite images of TSPSPR.COM (tspspr.asm): synthetic
data (the aliens are drawn here).

The 5x7 font of demos/sgp-bench. Usage: gen_data.py OUTPUT.INC
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


# Set 0: G1 and the sprites (G, R, B). Set 1: G0, the text and bars.
PALETTE0 = [palette_word(*c) for c in (
    (0, 0, 0), (0, 15, 0), (0, 15, 0), (0, 0, 15), (15, 0, 0), (15, 15, 0), (0, 15, 15), (15, 0, 15),
    (8, 15, 0), (8, 8, 15), (15, 8, 8), (4, 10, 4), (10, 10, 10), (15, 8, 15), (8, 15, 15), (15, 15, 15))]
PALETTE1 = [palette_word(*c) for c in (
    (0, 0, 0), (15, 15, 15), (0, 15, 0), (15, 15, 0), (12, 12, 12), (15, 0, 15), (0, 0, 0), (0, 0, 0),
    (0, 0, 0), (0, 0, 0), (0, 0, 0), (0, 0, 0), (0, 0, 0), (0, 0, 0), (0, 0, 0), (0, 0, 0))]

# 16x16 aliens: '.' transparent, 'o' body (each sprite's colour), 'e' eyes
# (white), 'a' accent (yellow).
ALIENS = [
    """
................
...o........o...
....o......o....
...oooooooooo...
..oooeooooeooo..
..ooeeeooeeeoo..
.oooooeooeooooo.
.oooooooooooooo.
.o.oooooooooo.o.
.o.o........o.o.
.o.oo......oo.o.
....oo....oo....
...aa......aa...
................
................
................
""",
    """
......oooo......
....oooooooo....
...oooooooooo...
..ooeeooooeeoo..
..oeeeeooeeeeo..
..ooeeooooeeoo..
..oooooooooooo..
...oooooooooo...
....oo.oo.oo....
...oo..oo..oo...
..oo..o..o..oo..
..o..oo..oo..o..
.aa..a....a..aa.
................
................
................
""",
    """
................
................
......oooo......
....ooeeeeoo....
...ooeeeeeeoo...
..oooooooooooo..
.ooaooaooaooaoo.
oooooooooooooooo
.oooooooooooooo.
...ooo....ooo...
....o......o....
................
................
................
................
................
""",
]
# 8x16 shot: 'o' its colour, 'e' white.
SHOT = """
........
...ee...
...ee...
...oo...
...oo...
...oo...
...oo...
...oo...
...oo...
...oo...
...oo...
..o..o..
........
........
........
........
"""


def image(text: str, width: int) -> bytes:
    rows = text.strip('\n').split('\n')
    assert len(rows) == 16 and all(len(r) == width for r in rows), rows
    code = {'.': 0, 'o': 1, 'e': 2, 'a': 3}
    return bytes(code[c] for r in rows for c in r)


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
    lines = ['; Generated by gen_data.py: synthetic data.',
             '%define PALETTE32 ' + ','.join(f'{w:#06x}' for w in PALETTE0 + PALETTE1),
             '\t\tjmp\tstart\t\t\t; data first']
    lines += db('FONT', font())
    lines += db('ALIEN_IMAGES', b''.join(image(a, 16) for a in ALIENS))
    lines += db('SHOT_IMAGE', image(SHOT, 8))
    return '\n'.join(lines) + '\n'


if __name__ == '__main__':
    if len(sys.argv) != 2:
        sys.exit('usage: gen_data.py OUTPUT.INC')
    with open(sys.argv[1], 'w', encoding='ascii', newline='\n') as out:
        out.write(include())
