<!--
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
-->
# SDL2 Frontend

This is the SDL2 frontend for the portable PC-98 scaffold. It links the
CMake `vaeg_core` and `vaeg_common` targets and includes the M10 Dear
ImGui menu layer. VA and platform code for Windows/macOS are still later
milestones.

## Build

```sh
cmake --preset linux-debug
cmake --build build/linux-debug --target vaeg_sdl2
```

The executable is written to:

```text
build/linux-debug/sdl2/vaeg
```

SDL2 is discovered through `find_package(SDL2)` first, then pkg-config.

## Run

```sh
./build/linux-debug/sdl2/vaeg [--smoke] [image1 [image2]]
```

Positional arguments mount FDD images in drive 1 and 2. Missing image files
are rejected with an error instead of silently starting without media.

Headless smoke check:

```sh
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy ./build/linux-debug/sdl2/vaeg --smoke
```

`--smoke` initializes video, audio, and the PC-98 core, runs a short fixed
frame loop, then exits with status 0 when initialization succeeds.

## ROM Placement

At startup the frontend uses the existing NP2 configuration field
`np2cfg.biospath`. Relative BIOS paths are resolved from the directory that
contains the `vaeg` executable. If `np2cfg.biospath` is empty and a
`romimage/` directory exists beside the executable, it uses that directory;
otherwise it looks beside the executable itself. ROM and WAV file lookup is
case-insensitive for the final `.rom` or `.wav` path component on
case-sensitive filesystems. Without ROM files the frontend should still follow
the core's defined no-ROM path and must not crash.

## Configuration

The ini format is unchanged. The SDL2 frontend looks for `np2.cfg` in:

1. `$XDG_CONFIG_HOME/vaeg/np2.cfg`
2. `$HOME/.config/vaeg/np2.cfg`
3. `./np2.cfg` if neither environment location is available

The directory is created on save when the XDG or home path is used.

## Font Manager Stub

Host GUI text is rendered by Dear ImGui using
`assets/NotoSansJP-Regular.ttf`; it does not use `sdl2/fontmng.c`.
The SDL2 `fontmng` stub remains linked because the shared core still
builds `font/fontmake.c`, whose `makepc98bmp()` path references
`fontmng_create()`, `fontmng_get()`, and `fontmng_destroy()`. Removing the
stub leaves those symbols unresolved. The current SDL2 consumers are
therefore `CMakeLists.txt` and the shared `font/fontmake.c` link path.
