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
# Build

The portable frontend is built with CMake and SDL2. The legacy Visual
Studio projects remain as reference artifacts and are not part of these
instructions.

## Linux

Install CMake 3.20 or newer, Ninja, a C/C++ compiler, and SDL2
development files. On Debian-like systems:

```sh
sudo apt install cmake ninja-build gcc g++ clang libsdl2-dev pkg-config
cmake --preset linux-release
cmake --build --preset linux-release
```

The Linux sanitizer smoke preset is:

```sh
cmake --preset linux-asan
cmake --build --preset linux-asan
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy ASAN_OPTIONS=detect_leaks=0 \
  ./build/asan/sdl2/vaeg --smoke
```

## Windows (MSYS2 / MinGW-w64)

Use the MSYS2 MinGW64 shell. Install:

```sh
pacman -S --needed mingw-w64-x86_64-cmake mingw-w64-x86_64-ninja \
  mingw-w64-x86_64-gcc mingw-w64-x86_64-pkgconf
```

Build:

```sh
cmake --preset mingw-release
cmake --build --preset mingw-release
```

`mingw-release` builds a GUI subsystem executable. For a console debug
build, configure manually with `-DVAEG_WINDOWS_CONSOLE=ON`.

Linux cross-checks use:

```sh
cmake --preset mingw-cross
cmake --build --preset mingw-cross
```

The MinGW presets download the pinned SDL2 release recorded in ADR-0006
and link it statically. Windows release artifacts therefore require only
`vaeg.exe` at runtime. MinGW ASan is not enabled in this tree;
sanitizer availability depends on the MinGW runtime/package set, so G11
keeps sanitizer acceptance on Linux and macOS.

## macOS Release

The release preset downloads pinned SDL2 and links it statically:

```sh
cmake --preset macos-release
cmake --build --preset macos-release
```

## macOS Development (MacPorts)

The M11 baseline is MacPorts, not Homebrew. Install:

```sh
sudo port install cmake ninja libsdl2 pkgconfig
```

Build:

```sh
cmake --preset macos-macports
cmake --build --preset macos-macports
```

The MacPorts presets set `CMAKE_PREFIX_PATH=/opt/local` and
`PKG_CONFIG_PATH=/opt/local/lib/pkgconfig`. The sanitizer preset is:

```sh
cmake --preset macos-asan
cmake --build --preset macos-asan
```

## Runtime State And Config

`vaeg.cfg`, `vabkupmem.dat`, and fixed GUI save-state slots are normally
stored in the portable per-user state directory:

- Linux: `$XDG_CONFIG_HOME/vaeg` or `$HOME/.config/vaeg`
- Windows: `%APPDATA%\vaeg`
- macOS: `~/Library/Application Support/vaeg`

An executable-local `vaeg.cfg` overrides the user-state configuration. An
existing executable-local `vabkupmem.dat` similarly overrides the
user-state backup memory, and saves return to the selected file. Save
states and keyboard sidecars remain in the user directory.

`vabkupmem.dat` does not fall back to the ROM path. Legacy `win9x/`
remains exe-relative and unchanged.

ROMs are read beside the active executable. VA uses unsuffixed names, while
VA2/VA3 uses MAME-compatible suffixed names:

```text
VA:       vadic.rom, vafont.rom, varom00.rom, varom08.rom, varom1.rom
VA2/VA3:  vadic_va2.rom, vafont_va2.rom, varom00_va2.rom,
          varom08_va2.rom, varom1_va2.rom
Extra:    vasubsys.rom
```

ROMs are not distributed and must be extracted from hardware you own. The
VA2 names follow MAME's `pc88va2` ROM declaration and do not fall back to VA
names. The current working directory is accepted only as a development
fallback. The frontend compares ROM size, CRC32, and SHA-1 with MAME and
warns without aborting when a file differs.

For VA booting, `vaeg.cfg` must use:

```ini
pc_model=88VA1
SNDboard=100
clk_base=3993600
clk_mult=2
```

For a VA with Sound Board II, use `SNDboard=200`. `88VA2` is also valid
for `pc_model` and uses `SNDboard=200` for its YM2608/OPNA. Stale configs
can cause a V2 halt, a silent FM-timer hang, or the wrong clock domain.
Startup logs warn about invalid model/sound combinations and stale clock
settings but do not rewrite the user's configuration.

Configuration files are UTF-8 only in the portable frontend. Obsolete
`np2.ini`, `np2.cfg`, and `vaeg.ini` files are not read.

## Known Issue

Short-term pacing jitter has been observed under WSLg: over a longer
sample the average tempo matches the v141 reference, but note timing may
briefly stretch and compress when WSLg presents stall. Native Windows is
the G11 measurement platform for deciding whether this is environmental.
