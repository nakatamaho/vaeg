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

This is the SDL2 frontend for the portable PC-98 / PC-88VA build. It
links the CMake `vaeg_core`, `vaeg_va`, and `vaeg_common` targets and
includes the M10 Dear ImGui menu layer. See `../BUILD.md` for OS-level
build recipes.

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
`VAEG_FETCH_SDL2=ON` is reserved for the MinGW cross preset and fetches
the pinned SDL2 release recorded in ADR-0006.

## Run

```sh
./build/linux-debug/sdl2/vaeg [--smoke] [image1 [image2]]
```

Positional arguments mount FDD images in drive 1 and 2. Missing image files
are rejected with an error instead of silently starting without media.
`--pacelog` prints pacing counters once per second for jitter diagnosis.

Headless smoke check:

```sh
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy ./build/linux-debug/sdl2/vaeg --smoke
```

`--smoke` initializes video, audio, and the PC-98 core, runs a short fixed
frame loop, then exits with status 0 when initialization succeeds.

## ROM Placement

At startup the frontend uses the existing NP2 configuration field
`np2cfg.biospath`. If that field is empty and a repository-local
`romimage/` directory exists, it uses `romimage/` as the BIOS path. Without
ROM files the M8 frontend should still follow the core's defined no-ROM path
and must not crash.

## Configuration

The ini format is unchanged. The SDL2 frontend looks for `np2.cfg` in
the portable user state directory:

1. `$XDG_CONFIG_HOME/vaeg/np2.cfg`
2. `$HOME/.config/vaeg/np2.cfg`
3. `%APPDATA%\vaeg\np2.cfg` on Windows
4. `~/Library/Application Support/vaeg/np2.cfg` on macOS
5. `./np2.cfg` if no platform user directory is available

The directory is created on save when an environment or home path is
used. `vabkupmem.dat` and fixed GUI save-state slots use the same
directory. `vabkupmem.dat` also loads once from the configured ROM path
for migration; saves always go to the user state directory.

## Upgrading From An Older Config

Older `np2.cfg` files can keep PC-98 defaults after switching to the
portable VA build. For PC-88VA booting, check these keys:

- `pc_model=88VA1` or `pc_model=88VA2`: non-VA models can halt at V2.
- `SNDboard=200`: other values leave the VA Sound Board II unbound and can
  cause a silent hang in software that waits on the FM timer.
- `clk_base=3993600` and `clk_mult=2`: stale PC-98 clock settings put the
  VA in the wrong timing domain.

The frontend logs prominent warnings for stale VA sound-board or clock
settings. It never rewrites the user's configuration silently.

## Keyboard Mapping

The SDL2 keyboard path is scancode based. The default host layout is
`keyboard_host_layout=jis`; `us` is a fallback preset and `custom` stores
GUI-edited bindings as SDL scancode names in the user-state sidecar
`keyboard.map`. `keyboard_custom_map=file:keyboard.map` in `np2.cfg`
points to that sidecar.

Device / Keyboard in the ImGui menu exposes:

- Host layout: JIS, US, Custom
- Kana input: Off, JIS Kana, Roman Kana
- Auto Kana lock
- Full key binding table with capture-next-key

Roman Kana parses ASCII text input and emits the same guest keyboard
make/break sequence as physical keys. It never injects Unicode, CP932,
BIOS buffers, DOS buffers, RAM, or VRAM. When ImGui captures keyboard or
text input, neither raw keys nor Roman Kana output reach the guest.

The PC key has a proven VA guest code but no standard SDL physical
scancode default, so it is shown as unassigned until rebound. See
`docs/modernization/keyboard-mapping.md` for the full inventory and
evidence table.

## Font Manager Stub

Host GUI text is rendered by Dear ImGui using
`assets/NotoSansJP-Regular.ttf`; it does not use `sdl2/fontmng.c`.
The SDL2 `fontmng` stub remains linked because the shared core still
builds `font/fontmake.c`, whose `makepc98bmp()` path references
`fontmng_create()`, `fontmng_get()`, and `fontmng_destroy()`. Removing the
stub leaves those symbols unresolved. The current SDL2 consumers are
therefore `CMakeLists.txt` and the shared `font/fontmake.c` link path.
