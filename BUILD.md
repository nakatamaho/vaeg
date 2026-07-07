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
  mingw-w64-x86_64-gcc mingw-w64-x86_64-SDL2 mingw-w64-x86_64-pkgconf
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

The cross preset sets `VAEG_FETCH_SDL2=ON`, so CMake downloads and builds
the pinned SDL2 release recorded in ADR-0006 for link checks. This does
not solve runtime distribution: Windows release artifacts must still ship
`SDL2.dll` next to `vaeg.exe`. MinGW ASan is not enabled in this tree;
sanitizer availability depends on the MinGW runtime/package set, so G11
keeps sanitizer acceptance on Linux and macOS.

## macOS (MacPorts)

The M11 baseline is MacPorts, not Homebrew. Install:

```sh
sudo port install cmake ninja libsdl2 pkgconfig
```

Build:

```sh
cmake --preset macos-release
cmake --build --preset macos-release
```

The macOS presets set `CMAKE_PREFIX_PATH=/opt/local` and
`PKG_CONFIG_PATH=/opt/local/lib/pkgconfig`. The sanitizer preset is:

```sh
cmake --preset macos-asan
cmake --build --preset macos-asan
```

## Runtime State And Config

`np2.cfg`, `vabkupmem.dat`, and fixed GUI save-state slots are stored in
the portable per-user state directory:

- Linux: `$XDG_CONFIG_HOME/vaeg` or `$HOME/.config/vaeg`
- Windows: `%APPDATA%\vaeg`
- macOS: `~/Library/Application Support/vaeg`

`vabkupmem.dat` loads from that state directory first and falls back to
the ROM path once for migration; saves always go to the state directory.
Legacy `win9x/` remains exe-relative and unchanged.

For VA booting, `np2.cfg` must use:

```ini
pc_model=88VA1
SNDboard=200
clk_base=3993600
clk_mult=2
```

`88VA2` is also valid for `pc_model`. Stale configs can cause a V2 halt,
a silent FM-timer hang, or the wrong clock domain. Startup logs warn
about stale VA `SNDboard` and clock settings but do not rewrite the
user's configuration.

Configuration files are UTF-8 only in the portable frontend. Reading old
CP932 `np2.ini` files from the legacy lineage is out of scope for phase 2.

## Known Issue

Short-term pacing jitter has been observed under WSLg: over a longer
sample the average tempo matches the v141 reference, but note timing may
briefly stretch and compress when WSLg presents stall. Native Windows is
the G11 measurement platform for deciding whether this is environmental.
