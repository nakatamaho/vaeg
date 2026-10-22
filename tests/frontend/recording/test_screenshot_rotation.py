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

"""Actual SDL/source-derived pixel tests; isolated from Git history and emulator state.

The in-application selftest separately round-trips all screenshot save paths.
These focused tests compile the production orientation helper verbatim, then
apply single controlled mutations. Native GPU/window behavior is not claimed.
Scratch code, command/exit identities and binaries are retained, not removed.
"""
import hashlib
import json
from pathlib import Path
import shlex
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[3]
SOURCE = ROOT / "sdl2/scrnmng.c"
FIXTURE = r'''
#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef Uint8 BYTE;
enum { VAEG_ROTATE_RIGHT = 1 };
static struct { int rotation; } scrnmng;
HELPER
int main(void) {
    static const int sizes[][2] = {{3,2}, {37,23}, {23,37}, {1,7}};
    static const Uint32 formats[] = {SDL_PIXELFORMAT_RGB565, SDL_PIXELFORMAT_ARGB8888};
    unsigned f, s;
    for (f = 0; f < 2; ++f) {
        SDL_PixelFormat *format = SDL_AllocFormat(formats[f]);
        if (!format) return 2;
        for (s = 0; s < 4; ++s) {
            const int w = sizes[s][0], h = sizes[s][1], bytes = format->BytesPerPixel;
            const int pitch = ((w * bytes + 3) & ~3) + 8;
            const size_t size = (size_t)pitch * h + 16;
            BYTE *storage = (BYTE *)malloc(size), *copy = (BYTE *)malloc(size);
            int x, y, turn;
            if (!storage || !copy) return 2;
            memset(storage, 0xa5, size);
            for (y = 0; y < h; ++y) {
                for (x = 0; x < w; ++x) {
                    const Uint32 pixel = SDL_MapRGBA(format, (x * 47 + y * 11) & 255,
                        (x * 13 + y * 41) & 255, (x * 7 + y * 53) & 255, 31 + (x+y)%200);
                    if (bytes == 2) {
                        const Uint16 small = (Uint16)pixel;
                        memcpy(storage + 8 + (size_t)y*pitch + x*bytes, &small, 2);
                    } else {
                        memcpy(storage + 8 + (size_t)y*pitch + x*bytes, &pixel, 4);
                    }
                }
            }
            memcpy(copy, storage, size);
            for (turn = 0; turn < 2; ++turn) {
                SDL_Surface *source = SDL_CreateRGBSurfaceWithFormatFrom(storage+8, w, h,
                    format->BitsPerPixel, pitch, formats[f]);
                SDL_Surface *out;
                if (!source) return 2;
                scrnmng.rotation = turn ? VAEG_ROTATE_RIGHT : 0;
                out = scrnmng_orient_guest_screenshot(source);
                if (!out) return 2;
                if (out->w != (turn ? h : w) || out->h != (turn ? w : h)) {
                    puts("SCREENSHOT_ROTATION_DIMENSION"); return 1;
                }
                for (y = 0; y < h; ++y) {
                    for (x = 0; x < w; ++x) {
                        const int dx = turn ? h-1-y : x, dy = turn ? x : y;
                        if (memcmp((BYTE *)out->pixels + (size_t)dy*out->pitch + dx*bytes,
                                   copy + 8 + (size_t)y*pitch + x*bytes, bytes)) {
                            puts("SCREENSHOT_ROTATION_PIXELS"); return 1;
                        }
                    }
                }
                if (memcmp(storage, copy, size)) {
                    puts("SCREENSHOT_SOURCE_CHANGED"); return 1;
                }
                SDL_FreeSurface(out);
            }
            free(storage); free(copy);
        }
        SDL_FreeFormat(format);
    }
    puts("PASS");
    return 0;
}
'''


class ScreenshotRotation(unittest.TestCase):
    def setUp(self):
        source = SOURCE.read_text()
        start = "static SDL_Surface *scrnmng_orient_guest_screenshot("
        end = "\nBOOL scrnmng_save_guest_frame("
        self.assertEqual(source.count(start), 1)
        self.assertEqual(source.count(end), 1)
        self.helper = source[source.index(start):source.index(end)]

    def run_helper(self, helper):
        compiler = shutil.which("cc")
        pkg_config = shutil.which("pkg-config")
        self.assertIsNotNone(compiler)
        self.assertIsNotNone(pkg_config)
        flags = shlex.split(subprocess.check_output(
            [pkg_config, "--cflags", "--libs", "sdl2"], text=True
        ))
        directory = Path(tempfile.mkdtemp(prefix="m100s11-screenshot-"))
        code = directory / "fixture.c"
        code.write_text(FIXTURE.replace("HELPER", helper))
        binary = directory / "fixture"
        command = [compiler, "-std=c99", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
                   str(code), *flags, "-o", str(binary)]
        compiled = subprocess.run(command, capture_output=True, text=True)
        (directory / "compile.log").write_text(compiled.stdout + compiled.stderr)
        self.assertEqual(compiled.returncode, 0, compiled.stderr)
        result = subprocess.run([str(binary)], capture_output=True, text=True)
        (directory / "run.log").write_text(result.stdout + result.stderr)
        (directory / "identity.json").write_text(json.dumps({
            "source_sha256": hashlib.sha256(SOURCE.read_bytes()).hexdigest(),
            "fixture_sha256": hashlib.sha256(code.read_bytes()).hexdigest(),
            "binary_sha256": hashlib.sha256(binary.read_bytes()).hexdigest(),
            "command": command, "compile_exit": compiled.returncode,
            "run_exit": result.returncode, "result": result.stdout.strip(),
        }, indent=2) + "\n")
        return result

    def passing_baseline(self):
        result = self.run_helper(self.helper)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(result.stdout.strip(), "PASS")

    def test_pixel_identity_and_ownership(self):
        self.passing_baseline()

    def test_missing_rotation_single_mutation(self):
        self.passing_baseline()
        needle = "scrnmng.rotation != VAEG_ROTATE_RIGHT"
        self.assertEqual(self.helper.count(needle), 1)
        result = self.run_helper(self.helper.replace(needle, "1"))
        self.assertEqual(result.returncode, 1)
        self.assertEqual(result.stdout.strip(), "SCREENSHOT_ROTATION_DIMENSION")

    def test_transpose_single_mutation(self):
        self.passing_baseline()
        needle = "source->h - 1 - y"
        self.assertEqual(self.helper.count(needle), 1)
        result = self.run_helper(self.helper.replace(needle, "y"))
        self.assertEqual(result.returncode, 1)
        self.assertEqual(result.stdout.strip(), "SCREENSHOT_ROTATION_PIXELS")


if __name__ == "__main__":
    unittest.main()
