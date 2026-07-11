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

Normal startup displays the historical 320x200 VAEG graphic from
`assets/vaeg.bmp` for at least 1.5 seconds while continuing to process SDL
events. CMake embeds the graphic in the executable; it is not a runtime file.
ROM-less `--smoke` and `--selftest` runs skip the graphic and delay. There is
no alternate-image fallback.

## SASI HDD Images

SASI HDD images are configured through `vaeg.cfg`:

```ini
HDD1FILE=/path/to/disk.hdi
HDD2FILE=
```

The SDL2 GUI also exposes HardDisk -> New SASI image plus SASI-1/SASI-2
Open and Remove. New SASI image creates HDI images using the existing
5/10/15/20/30/40 MB SASI geometry table and refuses to overwrite an
existing file. After changing a SASI image, reset the guest so the
existing SxSI/SASI open and bind path is rebuilt. SCSI and IDE GUI
mounting are not implemented yet.

## ROM Placement

ROMs are not included and must be extracted from hardware you own. Place the
selected set beside the executable:

| Model | Model ROM files |
|---|---|
| VA | `vadic.rom`, `vafont.rom`, `varom00.rom`, `varom08.rom`, `varom1.rom` |
| VA2/VA3 | `vadic_va2.rom`, `vafont_va2.rom`, `varom00_va2.rom`, `varom08_va2.rom`, `varom1_va2.rom` |

The VA2/VA3 names follow MAME's `pc88va2` `ROM_START` declaration in
[`src/mame/nec/pc88va.cpp`](https://github.com/mamedev/mame/blob/master/src/mame/nec/pc88va.cpp).
VA2/VA3 does not fall back to the unsuffixed VA files. Both models also use
`vasubsys.rom` as an extra: unlike MAME's currently unconnected FDD subsystem
ROM entry, vaeg executes the Z80 FDD subsystem.

After resolving a complete set, the frontend compares each file's size,
CRC32, and SHA-1 with MAME's `pc88va` or `pc88va2` declaration. The extra
`vasubsys.rom` uses the CRC32/SHA-1 from MAME's disabled FDD subsystem
declaration. A mismatch logs a warning with expected and actual values but
does not stop the emulator.

The active frontend resolves ROMs in this order:

1. the executable directory, using the filename set selected by model;
2. the current working directory, for development.

If neither complete set exists, the executable directory remains the expected
root and the frontend reports the selected model and first missing ROM. The
old `biospath` INI key is ignored by SDL2 and is no longer written.
`np2cfg.biospath` remains the shared core loader path but is derived at
runtime.

Use `Emulate -> Boot model -> VA` for `pc_model=88VA1` and unsuffixed files.
Use `VA2/VA3` for `pc_model=88VA2` and the five `*_va2.rom` files. Changing
the selection performs the existing reset flow and retains configured FDD
and SASI media.

## Configuration

The configuration syntax is unchanged. The SDL2 frontend selects the first
existing `vaeg.cfg` in this order:

1. `vaeg.cfg` beside the executable
2. `vaeg.cfg` in the portable user state directory

If neither file exists, the frontend creates `vaeg.cfg` in the user state
directory when settings are saved. The user state directory is
`$XDG_CONFIG_HOME/vaeg` or `$HOME/.config/vaeg` on Linux,
`%APPDATA%\vaeg` on Windows, and
`~/Library/Application Support/vaeg` on macOS. If no platform user
directory is available, it falls back to the current directory.

Obsolete `np2.cfg`, `np2.ini`, and `vaeg.ini` files are not read.

`vabkupmem.dat` and fixed GUI save-state slots use the user state
directory. `vabkupmem.dat` also loads once from the configured ROM path
for migration; saves always go to the user state directory.

## OPN/OPNA FM Backend

The Sound menu exposes `OPN backend -> NP2` and `OPN backend -> ymfm`.
The selection is saved in the selected `vaeg.cfg` as:

```ini
opn_backend=ymfm
```

`ymfm` is the default and selects the BSD-3-Clause ymfm YM2203/YM2608 FM
operator implementation. Select `np2` for the established NP2 sound behavior.
The backend change performs the normal GUI reset so the selected synthesizer
starts from a fully replayed board state; mounted FDD/SASI paths are retained.
Timer/IRQ, SSG, ADPCM, rhythm, board I/O, and final mixing remain on the NP2
path in this stage. Missing or unknown configuration values fall back to `ymfm`.

## VA Configuration Requirements

For PC-88VA booting, check these keys in the selected configuration:

- `pc_model=88VA1` or `pc_model=88VA2`: non-VA models can halt at V2.
- `SNDboard=200`: other values leave the VA Sound Board II unbound and can
  cause a silent hang in software that waits on the FM timer.
- `clk_base=3993600` and `clk_mult=2`: stale PC-98 clock settings put the
  VA in the wrong timing domain.

The frontend logs prominent warnings for stale VA sound-board or clock
settings. It never rewrites the user's configuration silently.

## Keyboard Mapping

The SDL2 keyboard path is scancode based. The default host layout is
`keyboard_host_layout=jis`; `us` is a US-keytop preset for text entry,
and `custom` stores GUI-edited bindings as SDL scancode names in the
user-state sidecar `keyboard.map`.
`keyboard_custom_map=file:keyboard.map` in the selected configuration
points to that
sidecar.

Device / Keyboard in the ImGui menu exposes:

- Host layout: JIS physical, US keytop, Custom
- Kana input: JIS Kana, Roman Kana
- Tenkey overlay: maps YUI/HJK/NM,. to guest keypad 789/456/123/0
- Full key binding table with capture-next-key

JIS physical maps host scancode position to PC-88VA physical key
position. US keytop maps printable US punctuation keytops/chords to
guest keys or guest Shift chords that produce the intended ASCII symbol.
The tenkey overlay is a game-oriented mode for tenkeyless keyboards and
is independent of the host layout preset. No Unicode or text-buffer
injection is used. Set `VAEG_KBD_TRACE=1` to log keyboard event routing
and selected guest actions.

Roman Kana parses A-Z and apostrophe host scancodes and emits the same
guest keyboard make/break sequence as physical keys. It never injects
Unicode, CP932, BIOS buffers, DOS buffers, RAM, or VRAM. When ImGui
captures keyboard or text input, neither raw keys nor Roman Kana output
reach the guest. The menu selects the kana input method only. Enter and
leave guest kana mode with the assigned KANA key, which defaults to
`RightAlt`: one press locks KANA, the next press unlocks it. When the
menu is set to Roman Kana and KANA is locked, A-Z host scancodes feed the
helper and are not sent as direct alphabetic guest keys; when KANA is
unlocked, A-Z is normal guest input.

The PC key defaults to `ScrollLock`. VA2/3 use PC-held reset or power-on
for the BIOS setup path, and some VA popup helpers use PC key chords such
as PC+D. See `docs/modernization/keyboard-mapping.md` for the full
inventory and evidence table.

## Font Manager Stub

Host GUI text is rendered by Dear ImGui using
the build-time embedded `assets/NotoSansJP-Regular.ttf`; it does not use
an external runtime font or `sdl2/fontmng.c`.
The SDL2 `fontmng` stub remains linked because the shared core still
builds `font/fontmake.c`, whose `makepc98bmp()` path references
`fontmng_create()`, `fontmng_get()`, and `fontmng_destroy()`. Removing the
stub leaves those symbols unresolved. The current SDL2 consumers are
therefore `CMakeLists.txt` and the shared `font/fontmake.c` link path.
