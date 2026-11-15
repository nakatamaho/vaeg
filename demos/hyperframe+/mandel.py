#!/usr/bin/env python3
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
"""Render a Mandelbrot-set close-up for the HyPER-FRAME+ (synthetic data).

usage: mandel.py OUT.RGB [--ppm OUT.PPM] [--samples N] [--jobs N]
                 [--height 400|200]

OUT.RGB is 640 x 400 x 3 bytes, R G B per pixel, rows top to bottom
(768000 bytes), composed for a 640x400 screen shown at 4:3. Smooth
(continuous) iteration colouring through a cyclic palette; the set itself
is black. Pure standard library; deterministic for a given --samples.
--height 200 renders the same view at 640 x 200 for the PC-88VA's own
640x200 65536-colour screen (demos/native65536).
"""
import argparse
import math
import multiprocessing
import sys

W, H = 640, 400
PIXEL_ASPECT = 1.2          # 640x400 on a 4:3 monitor

# Seahorse valley: a double spiral near the main cardioid's neck.
CENTRE_RE = -0.743643887037151
CENTRE_IM = 0.131825904205330
WIDTH = 0.000165            # real-axis span of the picture
MAX_ITER = 2400
BAILOUT = 256.0             # large bailout for smooth colouring
LOG2 = math.log(2.0)


def palette(t):
    """Cyclic palette: deep blue, cyan, white, gold, red-brown."""
    stops = (
        (0.00, (0.00, 0.03, 0.20)),
        (0.16, (0.10, 0.40, 0.80)),
        (0.42, (0.93, 1.00, 1.00)),
        (0.64, (1.00, 0.67, 0.00)),
        (0.86, (0.45, 0.07, 0.02)),
        (1.00, (0.00, 0.03, 0.20)),
    )
    t -= math.floor(t)
    for (t0, c0), (t1, c1) in zip(stops, stops[1:]):
        if t <= t1:
            k = (t - t0) / (t1 - t0)
            k = k * k * (3.0 - 2.0 * k)        # smoothstep between stops
            return tuple(a + (b - a) * k for a, b in zip(c0, c1))
    return stops[-1][1]


def colour(cr, ci):
    zr = zi = 0.0
    zr2 = zi2 = 0.0
    n = 0
    while n < MAX_ITER and zr2 + zi2 <= BAILOUT:
        zi = 2.0 * zr * zi + ci
        zr = zr2 - zi2 + cr
        zr2 = zr * zr
        zi2 = zi * zi
        n += 1
    if n >= MAX_ITER:
        return (0.0, 0.0, 0.0)
    # continuous escape count
    nu = n + 1.0 - math.log(math.log(zr2 + zi2) / 2.0 / LOG2) / LOG2
    return palette(math.sqrt(nu) * 0.11)


def render_row(args):
    y, samples, lines = args
    if lines == H:
        height = WIDTH * (H * PIXEL_ASPECT) / W
    else:
        height = WIDTH * 3.0 / 4.0          # the same 4:3 view
    left = CENTRE_RE - WIDTH / 2.0
    top = CENTRE_IM + height / 2.0
    row = bytearray()
    n = samples
    for x in range(W):
        acc = [0.0, 0.0, 0.0]
        for sy in range(n):
            for sx in range(n):
                cr = left + (x + (sx + 0.5) / n) * WIDTH / W
                ci = top - (y + (sy + 0.5) / n) * height / lines
                c = colour(cr, ci)
                acc[0] += c[0]
                acc[1] += c[1]
                acc[2] += c[2]
        k = 1.0 / (n * n)
        row += bytes(int(round(max(0.0, min(1.0, v * k)) * 255.0)) for v in acc)
    return y, bytes(row)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('out')
    ap.add_argument('--ppm')
    ap.add_argument('--samples', type=int, default=2, help='N x N samples per pixel')
    ap.add_argument('--jobs', type=int, default=0)
    ap.add_argument('--height', type=int, choices=(400, 200), default=H)
    a = ap.parse_args()
    jobs = a.jobs or multiprocessing.cpu_count()
    rows = [None] * a.height
    with multiprocessing.Pool(jobs) as pool:
        for y, data in pool.imap_unordered(render_row, [(y, a.samples, a.height)
                                                        for y in range(a.height)]):
            rows[y] = data
    image = b''.join(rows)
    assert len(image) == W * a.height * 3
    with open(a.out, 'wb') as f:
        f.write(image)
    if a.ppm:
        with open(a.ppm, 'wb') as f:
            f.write(b'P6\n%d %d\n255\n' % (W, a.height))
            f.write(image)
    return 0


if __name__ == '__main__':
    sys.exit(main())
