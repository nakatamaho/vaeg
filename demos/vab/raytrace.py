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
"""Ray-trace the Video Art Board sample picture (synthetic, free data).

A different scene from the HyPER-FRAME+ picture of demos/hyperframe+: a
large gold mirror sphere stands in the middle of a ring of seven smaller
spheres in rainbow colours, on a terracotta-and-cream checkerboard under a
sunset sky.  A low, warm light from the left throws long shadows; the camera
looks down from a front corner.

usage: raytrace.py OUT.RGB [--ppm OUT.PPM] [--samples N] [--jobs N]

OUT.RGB is 320 x 200 x 3 bytes, R G B per pixel, rows top to bottom
(192000 bytes), composed for the board's 320x200 screen shown at 4:3
(pixels 1.2 times as tall as wide).  Pure standard library; deterministic
for a given --samples.
"""
import argparse
import math
import multiprocessing
import sys

W, H = 320, 200
PIXEL_ASPECT = 1.2          # 320x200 on a 4:3 monitor
MAX_DEPTH = 5
EPS = 1e-4

# ---- small vector helpers (tuples) ----------------------------------------


def add(a, b):
    return (a[0] + b[0], a[1] + b[1], a[2] + b[2])


def sub(a, b):
    return (a[0] - b[0], a[1] - b[1], a[2] - b[2])


def mul(a, k):
    return (a[0] * k, a[1] * k, a[2] * k)


def vmul(a, b):
    return (a[0] * b[0], a[1] * b[1], a[2] * b[2])


def dot(a, b):
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]


def norm(a):
    length = math.sqrt(dot(a, a))
    return (a[0] / length, a[1] / length, a[2] / length)


def cross(a, b):
    return (a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2],
            a[0] * b[1] - a[1] * b[0])


def reflect(d, n):
    return sub(d, mul(n, 2.0 * dot(d, n)))


# ---- scene -----------------------------------------------------------------

BIG = 1.6                                 # the gold sphere's radius
SMALL = 0.55                              # the ring spheres' radius
RING = 3.1                                # ring radius (centre to centre)
RAINBOW = (
    (0.90, 0.08, 0.06), (0.95, 0.45, 0.02), (0.95, 0.85, 0.05),
    (0.10, 0.75, 0.15), (0.05, 0.60, 0.85), (0.10, 0.15, 0.85),
    (0.55, 0.10, 0.80),
)


def ring_sphere(index, colour):
    a = math.radians(250.0 + index * 360.0 / len(RAINBOW))
    return ((RING * math.cos(a), SMALL, RING * math.sin(a)), SMALL,
            colour, 0.85, 0.10, 0.7, 80.0)


# (centre, radius, colour, diffuse, reflectivity, specular, shininess)
SPHERES = [((0.0, BIG, 0.0), BIG, (1.00, 0.78, 0.36), 0.10, 0.85, 1.0, 250.0)]
SPHERES += [ring_sphere(i, c) for i, c in enumerate(RAINBOW)]
FLOOR_REFLECT = 0.12
LIGHT = (-12.0, 6.0, 3.0)
LIGHT_COLOUR = (1.0, 0.86, 0.68)
AMBIENT = 0.16

EYE = (7.8, 5.6, -10.2)
TARGET = (0.0, 0.9, 0.0)
FOV_V = math.radians(33.0)                # vertical field of view


def sky(d):
    """Sunset: orange at the horizon, through rose to a deep blue zenith."""
    t = max(0.0, min(1.0, d[1] * 1.6))
    stops = ((0.0, (1.00, 0.62, 0.30)), (0.35, (0.85, 0.45, 0.55)),
             (1.0, (0.12, 0.16, 0.42)))
    for (t0, c0), (t1, c1) in zip(stops, stops[1:]):
        if t <= t1:
            k = (t - t0) / (t1 - t0)
            return add(mul(c0, 1.0 - k), mul(c1, k))
    return stops[-1][1]


def hit_sphere(o, d, c, r):
    oc = sub(o, c)
    b = dot(oc, d)
    q = dot(oc, oc) - r * r
    disc = b * b - q
    if disc < 0.0:
        return None
    s = math.sqrt(disc)
    t = -b - s
    if t > EPS:
        return t
    t = -b + s
    if t > EPS:
        return t
    return None


def nearest(o, d):
    best_t, best = None, None
    for i, sp in enumerate(SPHERES):
        t = hit_sphere(o, d, sp[0], sp[1])
        if t is not None and (best_t is None or t < best_t):
            best_t, best = t, i
    if d[1] < -1e-9:
        t = -o[1] / d[1]
        if t > EPS and (best_t is None or t < best_t):
            best_t, best = t, -1
    return best_t, best


def shadowed(p, to_light, dist):
    for sp in SPHERES:
        t = hit_sphere(p, to_light, sp[0], sp[1])
        if t is not None and t < dist:
            return True
    return False


def shade(p, n, view, colour, diffuse, specular, shininess):
    lvec = sub(LIGHT, p)
    dist = math.sqrt(dot(lvec, lvec))
    ldir = mul(lvec, 1.0 / dist)
    out = vmul(colour, mul((0.55, 0.60, 0.85), AMBIENT * 1.6))   # bluish sky fill
    lambert = dot(n, ldir)
    if lambert > 0.0 and not shadowed(add(p, mul(n, EPS * 10)), ldir, dist):
        out = add(out, mul(vmul(colour, LIGHT_COLOUR), diffuse * lambert))
        h = norm(sub(ldir, view))
        spec = max(0.0, dot(n, h)) ** shininess * specular
        out = add(out, mul(LIGHT_COLOUR, spec))
    return out


def trace(o, d, depth):
    t, idx = nearest(o, d)
    if idx is None:
        return sky(d)
    p = add(o, mul(d, t))
    if idx < 0:
        n = (0.0, 1.0, 0.0)
        tile = (math.floor(p[0] / 1.2) + math.floor(p[2] / 1.2)) & 1
        colour = (0.93, 0.88, 0.76) if tile == 0 else (0.62, 0.22, 0.12)
        fog = min(1.0, max(0.0, (t - 14.0) / 28.0))
        local = shade(p, n, d, colour, 0.9, 0.2, 30.0)
        reflectivity = FLOOR_REFLECT
    else:
        c, _, colour, diffuse, reflectivity, specular, shininess = SPHERES[idx]
        n = norm(sub(p, c))
        fog = 0.0
        local = shade(p, n, d, colour, diffuse, specular, shininess)
    if depth < MAX_DEPTH and reflectivity > 0.0:
        rdir = norm(reflect(d, n))
        rcol = trace(add(p, mul(n, EPS * 10)), rdir, depth + 1)
        if idx >= 0 and SPHERES[idx][4] > 0.5:
            rcol = vmul(rcol, colour)       # gold tint
        local = add(mul(local, 1.0 - reflectivity), mul(rcol, reflectivity))
    if fog > 0.0:
        local = add(mul(local, 1.0 - fog), mul(sky(d), fog))
    return local


def camera():
    fwd = norm(sub(TARGET, EYE))
    right = norm(cross((0.0, 1.0, 0.0), fwd))
    up = cross(fwd, right)
    half_h = math.tan(FOV_V / 2.0)
    half_w = half_h * (W / (H * PIXEL_ASPECT))
    return fwd, right, up, half_w, half_h


def to_byte(v):
    v = max(0.0, min(1.0, v))
    return int(round((v ** (1.0 / 2.2)) * 255.0))


def render_row(args):
    y, samples = args
    fwd, right, up, half_w, half_h = camera()
    row = bytearray()
    n = samples
    for x in range(W):
        acc = (0.0, 0.0, 0.0)
        for sy in range(n):
            for sx in range(n):
                u = ((x + (sx + 0.5) / n) / W) * 2.0 - 1.0
                v = 1.0 - ((y + (sy + 0.5) / n) / H) * 2.0
                d = norm(add(fwd, add(mul(right, u * half_w), mul(up, v * half_h))))
                acc = add(acc, trace(EYE, d, 0))
        acc = mul(acc, 1.0 / (n * n))
        row += bytes((to_byte(acc[0]), to_byte(acc[1]), to_byte(acc[2])))
    return y, bytes(row)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('out')
    ap.add_argument('--ppm')
    ap.add_argument('--samples', type=int, default=4, help='N x N rays per pixel')
    ap.add_argument('--jobs', type=int, default=0)
    a = ap.parse_args()
    jobs = a.jobs or multiprocessing.cpu_count()
    rows = [None] * H
    with multiprocessing.Pool(jobs) as pool:
        for y, data in pool.imap_unordered(render_row, [(y, a.samples) for y in range(H)]):
            rows[y] = data
    image = b''.join(rows)
    assert len(image) == W * H * 3
    with open(a.out, 'wb') as f:
        f.write(image)
    if a.ppm:
        with open(a.ppm, 'wb') as f:
            f.write(b'P6\n%d %d\n255\n' % (W, H))
            f.write(image)
    return 0


if __name__ == '__main__':
    sys.exit(main())
