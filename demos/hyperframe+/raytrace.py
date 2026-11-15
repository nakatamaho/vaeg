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
"""Ray-trace the M149 HyPER-FRAME+ sample picture (synthetic, free data).

Four spheres touch each other at the vertices of a regular tetrahedron in
the middle of the scene: red, green and blue spheres form the base on the
floor, a mirror sphere rests on top. The floor is a black-and-white tiled
checkerboard with a little gloss, under a sky gradient, lit by one point
light with hard shadows.

usage: raytrace.py OUT.RGB [--ppm OUT.PPM] [--samples N] [--jobs N]
                   [--height 400|200]

OUT.RGB is 640 x 400 x 3 bytes, R G B per pixel, rows top to bottom (what
HFSHOW.COM sends to the board, 768000 bytes). The picture is composed for
a 640x400 screen shown at 4:3 (pixels 1.2 times as tall as wide). Pure
standard library; deterministic for a given --samples.
--height 200 renders 640 x 200 for the PC-88VA's own 640x200 65536-colour
screen (demos/native65536), with pixels 2.4 times as tall as wide.
"""
import argparse
import math
import multiprocessing
import sys

W, H = 640, 400
PIXEL_ASPECT = 1.2          # 640x400 on a 4:3 monitor
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

R = 1.0                                   # sphere radius
EDGE = 2.0 * R                            # touching spheres
CIRCUM = EDGE / math.sqrt(3.0)            # base triangle circumradius
TOP = EDGE * math.sqrt(2.0 / 3.0)         # tetrahedron height


def base(angle_deg):
    a = math.radians(angle_deg)
    return (CIRCUM * math.sin(a), R, CIRCUM * math.cos(a))


# (centre, colour, diffuse, reflectivity, specular, shininess)
SPHERES = [
    (base(205.0), (0.85, 0.06, 0.05), 0.85, 0.08, 0.6, 60.0),    # red, front
    (base(85.0), (0.05, 0.70, 0.12), 0.85, 0.08, 0.6, 60.0),     # green, right
    (base(325.0), (0.06, 0.16, 0.90), 0.85, 0.08, 0.6, 60.0),    # blue, back left
    ((0.0, R + TOP, 0.0), (0.92, 0.94, 0.97), 0.05, 0.90, 1.0, 200.0),  # mirror
]
FLOOR_REFLECT = 0.18
LIGHT = (-6.0, 10.0, -7.0)
LIGHT_COLOUR = (1.0, 0.97, 0.92)
AMBIENT = 0.12

EYE = (0.0, 5.6, -9.8)
TARGET = (0.0, 1.35, 0.0)
FOV_V = math.radians(32.0)                # vertical field of view


def sky(d):
    t = max(0.0, min(1.0, 0.5 * (d[1] + 1.0)))
    horizon = (0.86, 0.90, 0.96)
    zenith = (0.20, 0.36, 0.72)
    return add(mul(horizon, 1.0 - t), mul(zenith, t))


def hit_sphere(o, d, c):
    oc = sub(o, c)
    b = dot(oc, d)
    q = dot(oc, oc) - R * R
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
        t = hit_sphere(o, d, sp[0])
        if t is not None and (best_t is None or t < best_t):
            best_t, best = t, i
    if d[1] < -1e-9:
        t = -o[1] / d[1]
        if t > EPS and (best_t is None or t < best_t):
            best_t, best = t, -1
    return best_t, best


def shadowed(p, to_light, dist):
    for sp in SPHERES:
        t = hit_sphere(p, to_light, sp[0])
        if t is not None and t < dist:
            return True
    return False


def shade(p, n, view, colour, diffuse, specular, shininess):
    lvec = sub(LIGHT, p)
    dist = math.sqrt(dot(lvec, lvec))
    ldir = mul(lvec, 1.0 / dist)
    out = mul(colour, AMBIENT)
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
        tile = (math.floor(p[0]) + math.floor(p[2])) & 1
        colour = (0.92, 0.92, 0.90) if tile == 0 else (0.10, 0.10, 0.12)
        # fade the far floor into the sky haze
        fog = min(1.0, max(0.0, (t - 12.0) / 30.0))
        local = shade(p, n, d, colour, 0.9, 0.25, 40.0)
        reflectivity = FLOOR_REFLECT
    else:
        c, colour, diffuse, reflectivity, specular, shininess = SPHERES[idx]
        n = norm(sub(p, c))
        fog = 0.0
        local = shade(p, n, d, colour, diffuse, specular, shininess)
    if depth < MAX_DEPTH and reflectivity > 0.0:
        rdir = norm(reflect(d, n))
        rcol = trace(add(p, mul(n, EPS * 10)), rdir, depth + 1)
        if idx >= 0 and SPHERES[idx][3] > 0.5:
            rcol = vmul(rcol, colour)       # the mirror's slight tint
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


def camera_for(height):
    # same 4:3 picture at another line count
    fwd = norm(sub(TARGET, EYE))
    right = norm(cross((0.0, 1.0, 0.0), fwd))
    up = cross(fwd, right)
    half_h = math.tan(FOV_V / 2.0)
    half_w = half_h * (4.0 / 3.0)
    return fwd, right, up, half_w, half_h


def to_byte(v):
    v = max(0.0, min(1.0, v))
    return int(round((v ** (1.0 / 2.2)) * 255.0))


def render_row(args):
    y, samples, height = args
    if height == H:
        fwd, right, up, half_w, half_h = camera()
    else:
        fwd, right, up, half_w, half_h = camera_for(height)
    row = bytearray()
    n = samples
    for x in range(W):
        acc = (0.0, 0.0, 0.0)
        for sy in range(n):
            for sx in range(n):
                u = ((x + (sx + 0.5) / n) / W) * 2.0 - 1.0
                v = 1.0 - ((y + (sy + 0.5) / n) / height) * 2.0
                d = norm(add(fwd, add(mul(right, u * half_w), mul(up, v * half_h))))
                acc = add(acc, trace(EYE, d, 0))
        acc = mul(acc, 1.0 / (n * n))
        row += bytes((to_byte(acc[0]), to_byte(acc[1]), to_byte(acc[2])))
    return y, bytes(row)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('out')
    ap.add_argument('--ppm')
    ap.add_argument('--samples', type=int, default=2, help='N x N rays per pixel')
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
