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
# EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
# OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
# OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
# ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

"""Optional EGL shader raster tests: needs ModernGL + NumPy, no ROMs.

Only descriptor syntax is translated from Slang to desktop GLSL. This is not
a librashader/Windows/physical-GPU lifecycle test. No private screenshots used.
"""
from pathlib import Path
import re
import time
import moderngl
import numpy as np

SHADERS = Path(__file__).resolve().parents[3] / "assets/shaders/crt/shaders"
VERTEX = """#version 450
layout(location=0) out vec2 vTexCoord;
void main() {
    vec2 p = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
    vTexCoord = p;
    gl_Position = vec4(p * 2.0 - 1.0, 0.0, 1.0);
}
"""


def fragment(name, point_only=False):
    source = (SHADERS / name).read_text()
    source = source.replace('#include "vaeg-scanline-aa.inc"',
                            (SHADERS / "vaeg-scanline-aa.inc").read_text())
    common, stages = source.split("#pragma stage vertex", 1)
    source = common + stages.split("#pragma stage fragment", 1)[1]
    source = re.sub(r"^#pragma parameter.*$", "", source, flags=re.M)
    source = re.sub(r"layout\(push_constant\) uniform Push\s*\{(.*?)\}\s*params;",
                    r"struct Push {\1}; uniform Push params;", source, flags=re.S)
    source = re.sub(r"set\s*=\s*0\s*,\s*", "", source)
    if point_only:
        # Test-local counterfactual: isolate all non-AA equations for parity.
        line = "float brightness = vaeg_scan_average(phase, thin, footprint);"
        if source.count(line) != 1:
            raise RuntimeError("M99_SCAN_GPU_POINT_FIXTURE")
        source = source.replace(line, "float brightness = vaeg_scan_point(phase, thin);")
    return source


def check(condition, code):
    if not condition:
        raise RuntimeError(code)


ctx = moderngl.create_standalone_context(require=450, backend="egl")
print("Renderer:", ctx.info["GL_RENDERER"], ctx.info["GL_VERSION"], flush=True)
programs = [ctx.program(vertex_shader=VERTEX, fragment_shader=fragment(name, counterfactual))
            for name, counterfactual in (("crt-lottes-fast.slang", False),
                                          ("vaeg-crt-aa.slang", False),
                                          ("vaeg-crt-aa.slang", True))]
vaos = [ctx.vertex_array(p, []) for p in programs]
source = ctx.texture((656, 410), 3, dtype="f4")
source.filter = (moderngl.LINEAR, moderngl.LINEAR)
source.repeat_x = source.repeat_y = False
source.use(location=2)
defaults = dict(MASK=1., MASK_INTENSITY=.5, SCANLINE_THINNESS=.5, SCAN_BLUR=2.5,
                CURVATURE=.03, TRINITRON_CURVE=0., CORNER=3., CRT_GAMMA=2.4)


def configure(index, width, height, values):
    params = dict(defaults, **values)
    params.update(SourceSize=(656., 410., 1/656, 1/410),
                  OutputSize=(float(width), float(height), 1/width, 1/height))
    for key, value in params.items():
        name = "params." + key
        if name in programs[index]:
            programs[index][name].value = value


def draw(index, width, height, **values):
    configure(index, width, height, values)
    fbo = ctx.simple_framebuffer((width, height), components=4, dtype="f4")
    fbo.use()
    vaos[index].render(vertices=3)
    pixels = np.frombuffer(fbo.read(components=3, dtype="f4"), np.float32).reshape(height, width, 3).copy()
    fbo.release()
    check(np.isfinite(pixels).all(), "M99_SCAN_GPU_NONFINITE")
    return pixels


# Same colors/kernel/warp/mask as the audited reference when AA is isolated.
rng = np.random.default_rng(2600)
source.write(rng.random((410, 656, 3), dtype=np.float32).tobytes())
for mask in range(4):
    for curve in (0., .03, .25):
        settings = dict(MASK=float(mask), CURVATURE=curve)
        old = draw(0, 320, 200, **settings)
        same = draw(2, 320, 200, **settings)
        error = float(np.abs(old - same).max())
        print("point parity", mask, curve, error, flush=True)
        check(error < .001, "M99_SCAN_GPU_COLOR_KERNEL_PARITY")

# No extra color/edge blur when modulation is constant (thinness=0).
old = draw(0, 640, 400, SCANLINE_THINNESS=0.)
new = draw(1, 640, 400, SCANLINE_THINNESS=0.)
check(float(np.abs(old - new).max()) < .001, "M99_SCAN_GPU_EDGE_PARITY")


def amplitude(rows, frequency):
    phase = np.arange(len(rows)) * (2 * np.pi * frequency)
    return float(2 * abs(np.sum(rows * np.exp(1j * phase))) / len(rows))


for flat in ((.4, .4, .4), (.6, .9, .9), (.9, .1, .1)):
    source.write(np.broadcast_to(np.array(flat, np.float32), (410, 656, 3)).copy().tobytes())
    for height in (400, 800, 1600):
        settings = dict(MASK=0., CURVATURE=0., CORNER=0.)
        # Complete periods of both the scanline and the beat: cropping 40 rows
        # leaks the fundamental into the 40px Fourier bin at 1600px output.
        rows = [draw(index, 256, height, **settings)[:, 64:192].mean(axis=(1, 2))
                for index in (0, 1)]
        bands = [amplitude(row, 1/40) for row in rows]
        print("raster 40px band", flat, height, bands, flush=True)
        # Do not demand a relative reduction of an already sub-LSB baseline.
        # Half an 8-bit code value is the absolute display-domain noise ceiling.
        check(bands[1] < max(bands[0] * .15, .5 / 255), "M99_SCAN_GPU_40PX_BAND")
        check(abs(float(rows[0].mean() - rows[1].mean())) < .04,
              "M99_SCAN_GPU_MEAN_EXPOSURE")

# Small-output limits including zero original weights, all curvature settings.
for thin in (0., .5, 1.):
    for curve in (0., .03, .25):
        draw(1, 160, 100, SCANLINE_THINNESS=thin, CURVATURE=curve)

# Software raster time only; no physical-GPU performance claim.
for height in (400, 1600):
    fbo = ctx.simple_framebuffer((640, height), components=4, dtype="f4")
    fbo.use()
    for index in (0, 1):
        configure(index, 640, height, {})
        vaos[index].render(vertices=3)
        ctx.finish()
        times = []
        for _ in range(20):
            start = time.perf_counter()
            vaos[index].render(vertices=3)
            ctx.finish()
            times.append((time.perf_counter() - start) * 1000)
        print("raster ms", height, "AA" if index else "original", "median/p95",
              np.percentile(times, [50, 95]).tolist(), flush=True)
    fbo.release()
print("M99_SCAN_GPU_PASS", flush=True)
