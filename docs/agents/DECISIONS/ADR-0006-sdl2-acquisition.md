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
# ADR-0006: SDL2 Acquisition Policy

Date: 2026-07-06

Status: Accepted

## Context

M8 established system SDL2 discovery through `find_package` or
pkg-config, with no FetchContent path. M11 exposed a practical exception:
the Linux-to-MinGW cross preset can compile the emulator, but procuring a
matching target SDL2 development package is not reliably available in the
agent environment. Native Linux and macOS should keep using platform SDL2
packages, including MacPorts under `/opt/local` on macOS.

## Decision

Default SDL2 acquisition remains system discovery:

1. `find_package(SDL2 QUIET CONFIG)`
2. pkg-config `sdl2`

Add an opt-in CMake option, `VAEG_FETCH_SDL2`, default `OFF`, that uses
FetchContent to build SDL2 from a pinned upstream release when a preset
explicitly requests it. The `mingw-cross` preset enables
`VAEG_FETCH_SDL2=ON`; the `linux-*` and `macos-*` presets leave it off.

| Field | Value |
|---|---|
| Name | SDL2 |
| Version | `2.32.10` |
| Upstream URL | `https://github.com/libsdl-org/SDL` |
| Release URL | `https://github.com/libsdl-org/SDL/releases/tag/release-2.32.10` |
| Release tag | `release-2.32.10` |
| Tag SHA | `5d24957` |
| Source URL | `https://github.com/libsdl-org/SDL/releases/download/release-2.32.10/SDL2-2.32.10.tar.gz` |
| Source tarball SHA-256 | `5f5993c530f084535c65a6879e9b26ad441169b3e25d789d83287040a9ca5165` |
| License | zlib |

The FetchContent build uses SDL2's shared-library target by default on
all platforms. This matches normal package-manager linkage and the
Windows distribution shape: M12 release artifacts must place `SDL2.dll`
next to `vaeg.exe`. The FetchContent option only solves build-time
acquisition for link checks; it does not define packaging or release
layout.

## Consequences

- Linux and macOS keep testing against system SDL2, so the default user
  build remains distro/package-manager aligned.
- MinGW cross link checks no longer depend on an externally cross-built
  SDL2 package.
- Future SDL2 upgrades require updating this ADR, the pinned
  FetchContent URL, and the URL hash together.
- M12 must bundle `SDL2.dll` next to the Windows executable in release
  artifacts; the FetchContent option is not a substitute for packaging.
