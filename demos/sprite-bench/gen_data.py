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
"""Characters, palettes and font of SGPBENCH.COM (sgpbench.asm).

Everything here is generated: four original characters (a round robot, a
squid, a box robot and a star) drawn from circles, rectangles and polygons
at 8x8, 16x16, 32x32 and 64x64 dots, 16 colours, colour 0 transparent.
The program expands the 4-bpp images to 8 and 16 bpp with the tables here.

Usage: gen_data.py OUTPUT.INC [PREVIEW.PPM]
"""

import math
import sys

SIZES = (8, 16, 32, 64)

# Index: RGB (8 bits a component). 0 is transparent; no other index may map
# to 0 at 8 or 16 bpp (BITBLT 0105h skips source 0).
COLOURS = (
    (0, 0, 0),        # 0 transparent
    (40, 36, 72),     # 1 outline
    (255, 255, 255),  # 2 white
    (72, 200, 96),    # 3 green
    (32, 128, 64),    # 4 green shade
    (168, 240, 160),  # 5 green light
    (232, 64, 72),    # 6 red
    (64, 120, 232),   # 7 blue
    (32, 64, 152),    # 8 blue shade
    (255, 216, 64),   # 9 yellow
    (240, 136, 32),   # 10 orange
    (168, 96, 224),   # 11 purple
    (104, 48, 160),   # 12 purple shade
    (255, 152, 184),  # 13 pink
    (176, 184, 200),  # 14 metal
    (104, 112, 128),  # 15 metal shade
)

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
    ',': '.....|.....|.....|.....|.##..|..#..|.#...',
    ':': '.....|.##..|.##..|.....|.##..|.##..|.....',
    '/': '.....|....#|...#.|..#..|.#...|#....|.....',
    '-': '.....|.....|.....|#####|.....|.....|.....',
    '+': '.....|..#..|..#..|#####|..#..|..#..|.....',
    '=': '.....|.....|#####|.....|#####|.....|.....',
    '(': '...#.|..#..|.#...|.#...|.#...|..#..|...#.',
    ')': '.#...|..#..|...#.|...#.|...#.|..#..|.#...',
}


class Canvas:
    """An S x S index image in unit coordinates (0..1 across)."""

    def __init__(self, size: int):
        self.s = size
        self.px = [[0] * size for _ in range(size)]

    def centre(self, x: int, y: int) -> tuple[float, float]:
        return (x + 0.5) / self.s, (y + 0.5) / self.s

    def fill(self, inside, colour, shade=None):
        """Every dot whose centre is inside; shade(u, v) may pick a colour."""
        for y in range(self.s):
            for x in range(self.s):
                u, v = self.centre(x, y)
                if inside(u, v):
                    self.px[y][x] = shade(u, v) if shade else colour

    def dot(self, u: float, v: float, r: float, colour: int):
        """A disc that keeps at least the dot under its centre."""
        self.fill(lambda a, b: (a - u) ** 2 + (b - v) ** 2 <= r * r, colour)
        x = min(self.s - 1, int(u * self.s))
        y = min(self.s - 1, int(v * self.s))
        self.px[y][x] = colour

    def outline(self, body: set[int]):
        """Body dots next to a transparent dot or the edge become outline."""
        if self.s < 16:
            return
        old = [row[:] for row in self.px]
        for y in range(self.s):
            for x in range(self.s):
                if old[y][x] not in body:
                    continue
                for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                    nx, ny = x + dx, y + dy
                    if not (0 <= nx < self.s and 0 <= ny < self.s) or old[ny][nx] == 0:
                        self.px[y][x] = 1
                        break


def ellipse(cu, cv, ru, rv):
    return lambda u, v: ((u - cu) / ru) ** 2 + ((v - cv) / rv) ** 2 <= 1.0


def rect(u0, v0, u1, v1):
    return lambda u, v: u0 <= u <= u1 and v0 <= v <= v1


def union(*shapes):
    return lambda u, v: any(f(u, v) for f in shapes)


def polygon(points):
    def inside(u, v):
        hit = False
        j = len(points) - 1
        for i, (xi, yi) in enumerate(points):
            xj, yj = points[j]
            if (yi > v) != (yj > v) and u < (xj - xi) * (v - yi) / (yj - yi) + xi:
                hit = not hit
            j = i
        return hit
    return inside


def shader(light, base, dark):
    """Light upper left, dark lower right."""
    def pick(u, v):
        t = (u + v) - 1.0
        if t < -0.45:
            return light
        if t > 0.35:
            return dark
        return base
    return pick


def round_bot(s: int) -> Canvas:
    c = Canvas(s)
    c.fill(rect(0.47, 0.06, 0.53, 0.25), 15)                    # antenna
    c.dot(0.5, 0.08, 0.07, 6)
    c.fill(union(ellipse(0.30, 0.90, 0.13, 0.08), ellipse(0.70, 0.90, 0.13, 0.08)), 4)
    c.fill(ellipse(0.5, 0.56, 0.42, 0.36), 3, shader(5, 3, 4))  # body
    c.outline({3, 4, 5, 15, 6})
    for u in (0.36, 0.64):                                      # eyes
        c.dot(u, 0.50, 0.10, 2)
        c.dot(u + 0.02, 0.52, 0.05, 1)
    if s >= 16:
        c.dot(0.24, 0.64, 0.05, 13)
        c.dot(0.76, 0.64, 0.05, 13)
        c.fill(lambda u, v: abs(u - 0.5) < 0.09 and 0.68 < v < 0.72, 1)
    return c


def squid(s: int) -> Canvas:
    c = Canvas(s)
    legs = []
    for i in range(4):
        cu = 0.20 + 0.20 * i
        legs.append(lambda u, v, cu=cu: 0.55 <= v <= 0.96 and
                    abs(u - cu - 0.05 * math.sin(v * 14.0 + cu * 6)) < 0.065)
    c.fill(union(*legs), 12)
    head = lambda u, v: (ellipse(0.5, 0.42, 0.40, 0.36)(u, v) and v <= 0.62)
    c.fill(head, 11, shader(13, 11, 12))
    c.fill(rect(0.12, 0.55, 0.88, 0.63), 12)
    c.outline({11, 12, 13})
    for u in (0.36, 0.64):
        c.dot(u, 0.40, 0.09, 2)
        c.dot(u, 0.42, 0.045, 1)
    return c


def box_bot(s: int) -> Canvas:
    c = Canvas(s)
    c.fill(union(rect(0.02, 0.45, 0.14, 0.62), rect(0.86, 0.45, 0.98, 0.62)), 15)  # arms
    c.fill(union(rect(0.24, 0.84, 0.40, 0.97), rect(0.60, 0.84, 0.76, 0.97)), 8)   # feet
    body = lambda u, v: (0.14 <= u <= 0.86 and 0.10 <= v <= 0.86 and
                         not ((u < 0.20 or u > 0.80) and (v < 0.16 or v > 0.80)))
    c.fill(body, 7, shader(14, 7, 8))
    c.outline({7, 8, 14, 15})
    c.fill(rect(0.24, 0.28, 0.76, 0.48), 1)                     # visor
    c.dot(0.38, 0.38, 0.06, 9)
    c.dot(0.62, 0.38, 0.06, 9)
    if s >= 16:
        c.fill(rect(0.30, 0.62, 0.70, 0.72), 15)
        for i in range(4):
            c.fill(rect(0.33 + 0.1 * i, 0.64, 0.37 + 0.1 * i, 0.70), 9)
    return c


def star(s: int) -> Canvas:
    c = Canvas(s)
    pts = []
    for i in range(10):
        a = -math.pi / 2 + i * math.pi / 5
        r = 0.48 if i % 2 == 0 else 0.21
        pts.append((0.5 + r * math.cos(a), 0.54 + r * math.sin(a)))
    c.fill(polygon(pts), 9, shader(2, 9, 10))
    c.outline({9, 10, 2})
    for u in (0.42, 0.58):
        c.dot(u, 0.52, 0.04, 1)
    if s >= 16:
        c.fill(lambda u, v: (u - 0.5) ** 2 + (v - 0.60) ** 2 < 0.07 ** 2 and v > 0.61, 6)
    return c


KINDS = (round_bot, squid, box_bot, star)


def images():
    """(size, kind, rows) in the order of the program's table."""
    return [(s, k, f(s).px) for s in SIZES for k, f in enumerate(KINDS)]


def pack4(rows) -> bytes:
    out = bytearray()
    for row in rows:
        for x in range(0, len(row), 2):
            out.append(row[x] << 4 | row[x + 1])
    return bytes(out)


def palette_word(r: int, g: int, b: int) -> int:
    """The 4-bpp palette register: G 15-12, R 9-6, B 4-1 (4 bits each)."""
    return (g >> 4) << 12 | (r >> 4) << 6 | (b >> 4) << 1


def byte8(r: int, g: int, b: int) -> int:
    """8 bpp: GGGRRRBB."""
    return (g >> 5) << 5 | (r >> 5) << 2 | b >> 6


def word16(r: int, g: int, b: int) -> int:
    """16 bpp: G 15-10, R 9-5, B 4-0."""
    return (g >> 2) << 10 | (r >> 3) << 5 | b >> 3


def include() -> str:
    for i, c in enumerate(COLOURS[1:], 1):
        assert byte8(*c) and word16(*c), i
    lines = ['; Generated by gen_data.py: synthetic data only.']
    lines.append('PAL4\t\tdw\t' + ','.join(f'{palette_word(*c):#06x}' for c in COLOURS))
    lines.append('PAL8\t\tdb\t' + ','.join(f'{byte8(*c):#04x}' for c in COLOURS))
    lines.append('PAL16\t\tdw\t' + ','.join(f'{word16(*c):#06x}' for c in COLOURS))
    font = bytearray()
    for code in range(32, 96):
        rows = FONT.get(chr(code), FONT[' ']).split('|')
        assert len(rows) == 7 and all(len(r) == 5 for r in rows), chr(code)
        font += bytes(sum(1 << (4 - i) for i, ch in enumerate(r) if ch == '#') for r in rows)
    lines.append('FONT:')
    for i in range(0, len(font), 16):
        lines.append('\t\tdb\t' + ','.join(f'{b:#04x}' for b in font[i:i + 16]))
    data = bytearray()
    offsets = []
    for s, k, rows in images():
        offsets.append(len(data))
        data += pack4(rows)
    lines.append('; 4-bpp offsets in SPRITES: size 8, 16, 32, 64; kinds 0-3 each')
    lines.append('SPR_OFS\t\tdw\t' + ','.join(str(o) for o in offsets))
    lines.append(f'SPR_BYTES\tequ\t{len(data)}')
    lines.append('SPRITES:')
    for i in range(0, len(data), 16):
        lines.append('\t\tdb\t' + ','.join(f'{b:#04x}' for b in data[i:i + 16]))
    return '\n'.join(lines) + '\n'


def preview(path: str):
    """Every image side by side at 4x, on grey: a PPM for checking."""
    scale = 4
    cells = images()
    width = (sum(SIZES) + 8 * 5) * scale
    height = (64 + 8) * 4 * scale
    img = [[(96, 96, 112)] * width for _ in range(height)]
    for s, k, rows in cells:
        ox = (sum(SIZES[:SIZES.index(s)]) + 8 * (SIZES.index(s) + 1)) * scale
        oy = ((64 + 8) * k + 4) * scale
        for y in range(s):
            for x in range(s):
                if rows[y][x]:
                    for dy in range(scale):
                        for dx in range(scale):
                            img[oy + y * scale + dy][ox + x * scale + dx] = COLOURS[rows[y][x]]
    with open(path, 'wb') as out:
        out.write(f'P6 {width} {height} 255\n'.encode())
        out.write(bytes(v for row in img for p in row for v in p))


if __name__ == '__main__':
    if len(sys.argv) not in (2, 3):
        sys.exit('usage: gen_data.py OUTPUT.INC [PREVIEW.PPM]')
    with open(sys.argv[1], 'w', encoding='ascii', newline='\n') as out:
        out.write(include())
    if len(sys.argv) == 3:
        preview(sys.argv[2])
