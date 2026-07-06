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
# Modernized Build Recipes

These are the G11 maintainer recipes for the portable SDL2/ImGui build.
The legacy Visual Studio projects remain reference artifacts and are not
covered here.

## Windows Native: MSYS2 MINGW64

Open the MSYS2 MINGW64 shell, then install the build prerequisites:

```sh
pacman -S --needed mingw-w64-x86_64-cmake mingw-w64-x86_64-ninja \
  mingw-w64-x86_64-gcc mingw-w64-x86_64-SDL2 mingw-w64-x86_64-pkgconf
```

Configure and build:

```sh
cmake --preset mingw-release
cmake --build --preset mingw-release
```

The output is `build/mingw-release/sdl2/vaeg.exe`. The preset builds a
GUI-subsystem executable. For a console while debugging, configure a
separate tree with `-DVAEG_WINDOWS_CONSOLE=ON`.

## macOS Native: MacPorts

The M11 baseline is MacPorts under `/opt/local`, not Homebrew. Install:

```sh
sudo port install cmake ninja libsdl2 pkgconfig
```

Configure and build:

```sh
cmake --preset macos-release
cmake --build --preset macos-release
```

The macOS presets set `CMAKE_PREFIX_PATH=/opt/local` and
`PKG_CONFIG_PATH=/opt/local/lib/pkgconfig`. A plain binary is accepted for
G11; no app bundle is required.

## Linux To Windows Cross Check

The agent-side MinGW cross check uses the pinned SDL2 FetchContent path
from ADR-0006:

```sh
cmake --preset mingw-cross
cmake --build --preset mingw-cross
```

The output is `build/mingw-cross/sdl2/vaeg.exe`. This preset is a link
check, not a distribution recipe. Windows release artifacts in M12 must
ship `SDL2.dll` next to `vaeg.exe`.

## VA Configuration Prerequisites

Before running the G11 VA checklist, ensure the portable `np2.cfg`
selects the VA machine and VA Sound Board II:

```ini
pc_model=88VA1
SNDboard=200
clk_base=3993600
clk_mult=2
```

`pc_model=88VA2` is also valid. Missing or stale values can produce a V2
halt, an FM-timer wait with no sound board bound, or the wrong clock
domain. See `../../sdl2/README.md` for the full state directory and stale
config warning policy.
