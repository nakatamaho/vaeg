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
"""Ray-trace three fused metaballs over the checkerboard floor (synthetic).

usage: metaball.py OUT.RGB [--ppm OUT.PPM] [--samples N] [--jobs N]
                   [--height 400|200] [--pair-frame 0..9]

The floor, sky, light and camera are those of ../hyperframe+/raytrace.py.
Each ball contributes the sextic field (1 - r^2/R^2)^3 (a degree-6
polynomial in r, zero beyond R); the surface is where the sum reaches
THRESHOLD, so the three balls flow into one another. The colour is HSV:
every ball has a hue, and a surface point takes the circular mean of the
hues weighted by 1 - r^2/R^2, so the necks run through the hues in between.

--pair-frame N draws frame N of a ten-frame sequence instead: a red and a
blue ball (the magenta between them where they meet), floating side by
side, coming together from far apart (centre distance PAIR_DISTANCES[N])
until they have fused.

OUT.RGB is 640 x H x 3 bytes, R G B per pixel, rows top to bottom, composed
for a 4:3 monitor. Pure standard library; deterministic for a given
--samples.
"""
import argparse
import math
import multiprocessing
import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)),
                                os.pardir, 'hyperframe+'))
import raytrace as rt  # noqa: E402  (floor, sky, light, camera, helpers)

add, sub, mul, vmul, dot, norm, reflect = (rt.add, rt.sub, rt.mul, rt.vmul,
                                            rt.dot, rt.norm, rt.reflect)
W = rt.W
EPS = rt.EPS
MAX_DEPTH = 3

THRESHOLD = 0.25
# a lone ball of influence radius R shows radius R * sqrt(1 - THRESHOLD^(1/3))
VISIBLE = math.sqrt(1.0 - THRESHOLD ** (1.0 / 3.0))


def ball(x, y, z, visible_radius, hue_deg):
    radius = visible_radius / VISIBLE
    return ((x, y, z), radius, radius * radius,
            math.cos(math.radians(hue_deg)), math.sin(math.radians(hue_deg)))


# (centre, influence radius R, R^2, cos hue, sin hue)
BALLS = [
    ball(-1.05, 1.50, -0.45, 1.05, 60.0),     # yellow, front left
    ball(1.05, 1.42, -0.30, 0.95, 180.0),     # cyan, front right
    ball(0.05, 2.25, 0.75, 1.10, 300.0),      # magenta, back, riding up
]
SATURATION = 0.88
VALUE = 0.95
DIFFUSE = 0.85
SPECULAR = 0.75
SHININESS = 90.0
REFLECT = 0.14
FLOOR_REFLECT = rt.FLOOR_REFLECT
# a bound on |grad field|: one ball's slope peaks at 6/(sqrt(5) R) * (4/5)^2


def lipschitz(balls):
    return sum(6.0 / (math.sqrt(5.0) * b[1]) * 0.64 for b in balls)


LIPSCHITZ = lipschitz(BALLS)

# --pair-frame: centre distances, far apart to fused (a lone ball's visible
# radius is 1.0, so the surfaces would touch at 2.0 without the blending)
PAIR_DISTANCES = (5.0, 4.2, 3.6, 3.2, 2.9, 2.6, 2.3, 2.0, 1.6, 1.2)


def pair_balls(frame):
    half = PAIR_DISTANCES[frame] / 2.0
    return [ball(-half, 1.50, 0.0, 1.0, 0.0),        # red, left
            ball(half, 1.50, 0.0, 1.0, 240.0)]       # blue, right


def select_scene(pair_frame):
    global BALLS, LIPSCHITZ
    if pair_frame is not None:
        BALLS = pair_balls(pair_frame)
        LIPSCHITZ = lipschitz(BALLS)
MIN_STEP = 2e-3


def field(p):
    total = 0.0
    for c, _, r2, _, _ in BALLS:
        dx = p[0] - c[0]
        dy = p[1] - c[1]
        dz = p[2] - c[2]
        q = (dx * dx + dy * dy + dz * dz) / r2
        if q < 1.0:
            k = 1.0 - q
            total += k * k * k
    return total


def gradient(p):
    gx = gy = gz = 0.0
    for c, _, r2, _, _ in BALLS:
        dx = p[0] - c[0]
        dy = p[1] - c[1]
        dz = p[2] - c[2]
        q = (dx * dx + dy * dy + dz * dz) / r2
        if q < 1.0:
            k = 1.0 - q
            s = -6.0 * k * k / r2
            gx += s * dx
            gy += s * dy
            gz += s * dz
    return (gx, gy, gz)


def hsv_colour(p):
    cx = cy = 0.0
    for c, _, r2, hc, hs in BALLS:
        dx = p[0] - c[0]
        dy = p[1] - c[1]
        dz = p[2] - c[2]
        q = (dx * dx + dy * dy + dz * dz) / r2
        if q < 1.0:
            w = 1.0 - q
            cx += w * hc
            cy += w * hs
    h = (math.degrees(math.atan2(cy, cx)) % 360.0) / 60.0
    i = int(h) % 6
    f = h - math.floor(h)
    v = VALUE
    pp = v * (1.0 - SATURATION)
    qq = v * (1.0 - SATURATION * f)
    tt = v * (1.0 - SATURATION * (1.0 - f))
    rgb = ((v, tt, pp), (qq, v, pp), (pp, v, tt), (pp, qq, v), (tt, pp, v), (v, pp, qq))[i]
    # HSV values are display-referred; the renderer works in linear light
    return tuple(x ** 2.2 for x in rgb)


def bounds(o, d):
    """Ray interval inside the union of the influence spheres, or None."""
    t0, t1 = None, None
    for c, r, r2, _, _ in BALLS:
        oc = sub(o, c)
        b = dot(oc, d)
        disc = b * b - (dot(oc, oc) - r2)
        if disc <= 0.0:
            continue
        s = math.sqrt(disc)
        a, z = -b - s, -b + s
        if z <= EPS:
            continue
        a = max(a, EPS)
        t0 = a if t0 is None else min(t0, a)
        t1 = z if t1 is None else max(t1, z)
    if t0 is None:
        return None
    return t0, t1


def hit_blob(o, d, t_max=None):
    span = bounds(o, d)
    if span is None:
        return None
    t, t1 = span
    if t_max is not None:
        t1 = min(t1, t_max)
    prev_t = t
    while t < t1:
        p = (o[0] + d[0] * t, o[1] + d[1] * t, o[2] + d[2] * t)
        g = THRESHOLD - field(p)
        if g <= 0.0:
            lo, hi = prev_t, t
            for _ in range(28):
                mid = 0.5 * (lo + hi)
                pm = (o[0] + d[0] * mid, o[1] + d[1] * mid, o[2] + d[2] * mid)
                if field(pm) >= THRESHOLD:
                    hi = mid
                else:
                    lo = mid
            return hi
        prev_t = t
        t += max(g / LIPSCHITZ, MIN_STEP)
    return None


def shadowed(p, to_light, dist):
    return hit_blob(p, to_light, dist) is not None


def shade(p, n, view, colour, diffuse, specular, shininess):
    lvec = sub(rt.LIGHT, p)
    dist = math.sqrt(dot(lvec, lvec))
    ldir = mul(lvec, 1.0 / dist)
    out = mul(colour, rt.AMBIENT)
    lambert = dot(n, ldir)
    if lambert > 0.0 and not shadowed(add(p, mul(n, EPS * 10)), ldir, dist):
        out = add(out, mul(vmul(colour, rt.LIGHT_COLOUR), diffuse * lambert))
        h = norm(sub(ldir, view))
        spec = max(0.0, dot(n, h)) ** shininess * specular
        out = add(out, mul(rt.LIGHT_COLOUR, spec))
    return out


def trace(o, d, depth):
    tb = hit_blob(o, d)
    tf = None
    if d[1] < -1e-9:
        tf = -o[1] / d[1]
        if tf <= EPS:
            tf = None
    if tb is None and tf is None:
        return rt.sky(d)
    if tb is not None and (tf is None or tb < tf):
        p = add(o, mul(d, tb))
        g = gradient(p)
        n = norm((-g[0], -g[1], -g[2]))
        colour = hsv_colour(p)
        local = shade(p, n, d, colour, DIFFUSE, SPECULAR, SHININESS)
        reflectivity = REFLECT
        fog = 0.0
    else:
        p = add(o, mul(d, tf))
        n = (0.0, 1.0, 0.0)
        tile = (math.floor(p[0]) + math.floor(p[2])) & 1
        colour = (0.92, 0.92, 0.90) if tile == 0 else (0.10, 0.10, 0.12)
        fog = min(1.0, max(0.0, (tf - 12.0) / 30.0))
        local = shade(p, n, d, colour, 0.9, 0.25, 40.0)
        reflectivity = FLOOR_REFLECT
    if depth < MAX_DEPTH and reflectivity > 0.0:
        rdir = norm(reflect(d, n))
        rcol = trace(add(p, mul(n, EPS * 10)), rdir, depth + 1)
        local = add(mul(local, 1.0 - reflectivity), mul(rcol, reflectivity))
    if fog > 0.0:
        local = add(mul(local, 1.0 - fog), mul(rt.sky(d), fog))
    return local


def render_row(args):
    y, samples, height, pair_frame = args
    select_scene(pair_frame)
    fwd, right, up, half_w, half_h = rt.camera_for(height)
    row = bytearray()
    n = samples
    for x in range(W):
        acc = (0.0, 0.0, 0.0)
        for sy in range(n):
            for sx in range(n):
                u = ((x + (sx + 0.5) / n) / W) * 2.0 - 1.0
                v = 1.0 - ((y + (sy + 0.5) / n) / height) * 2.0
                d = norm(add(fwd, add(mul(right, u * half_w), mul(up, v * half_h))))
                acc = add(acc, trace(rt.EYE, d, 0))
        acc = mul(acc, 1.0 / (n * n))
        row += bytes((rt.to_byte(acc[0]), rt.to_byte(acc[1]), rt.to_byte(acc[2])))
    return y, bytes(row)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('out')
    ap.add_argument('--ppm')
    ap.add_argument('--samples', type=int, default=2, help='N x N rays per pixel')
    ap.add_argument('--jobs', type=int, default=0)
    ap.add_argument('--height', type=int, choices=(400, 200), default=400)
    ap.add_argument('--pair-frame', type=int, choices=range(len(PAIR_DISTANCES)))
    a = ap.parse_args()
    jobs = a.jobs or multiprocessing.cpu_count()
    rows = [None] * a.height
    with multiprocessing.Pool(jobs) as pool:
        for y, data in pool.imap_unordered(render_row, [(y, a.samples, a.height, a.pair_frame)
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
