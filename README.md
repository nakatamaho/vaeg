# 88VA Eternal Grafx

88VA Eternal Grafx, or `vaeg`, is a maintained fork of the abandoned
`project-vaeg` PC-88VA emulator lineage, itself derived from Neko Project
II. This fork is the living tree: the old upstream should be treated as
historical source material, not as the active project.

The current goal is a portable PC-88VA emulator that builds and runs on
modern Windows, Linux, and macOS systems while preserving the behavior of
the legacy Visual Studio reference build.

## Current Frontend

The active frontend is the SDL2 + Dear ImGui build under `sdl2/`. It
targets:

- Windows via MSYS2 / MinGW-w64
- Linux via CMake, Ninja, SDL2, gcc or clang
- macOS via MacPorts SDL2 under `/opt/local`

The executable is named `vaeg`.

```sh
vaeg [--smoke] [--pacelog] [image1 [image2]]
```

`image1` and `image2` are floppy disk images mounted in drives 1 and 2.
`--smoke` runs a short headless initialization check. `--pacelog` prints
emulation pacing counters for timing diagnosis.

## Quick Build

Detailed build instructions live in [BUILD.md](BUILD.md). The short
versions are:

```sh
# Linux
cmake --preset linux-release
cmake --build --preset linux-release
```

```sh
# Windows, from an MSYS2 MINGW64 shell
cmake --preset mingw-release
cmake --build --preset mingw-release
```

```sh
# macOS, with MacPorts SDL2
sudo port install cmake ninja libsdl2 pkgconfig
cmake --preset macos-release
cmake --build --preset macos-release
```

Linux-to-Windows cross-link checks are also available:

```sh
cmake --preset mingw-cross
cmake --build --preset mingw-cross
```

## Runtime Files

Machine ROM images, guest font ROMs, optional mechanical sound WAV files,
and operating system disks are not provided by this repository. Use
legally obtained files from your own environment. The host GUI font used
by Dear ImGui is bundled separately under `assets/`.

The portable frontend stores writable state in the platform user state
directory:

- Linux: `$XDG_CONFIG_HOME/vaeg` or `$HOME/.config/vaeg`
- Windows: `%APPDATA%\vaeg`
- macOS: `~/Library/Application Support/vaeg`

`np2.cfg`, `vabkupmem.dat`, and fixed GUI save-state slots live there.
The legacy `win9x/` build remains exe-relative and is intentionally not
changed.

For PC-88VA booting, `np2.cfg` should select the VA machine, VA Sound
Board II, and the VA clock domain:

```ini
pc_model=88VA1
SNDboard=200
clk_base=3993600
clk_mult=2
```

`pc_model=88VA2` is also valid. Stale PC-98 defaults can halt at V2,
leave the VA sound board unbound, or run in the wrong clock domain.

## Text Encoding Policy

This fork has moved the source tree to modern UTF-8 text.

- Active source files and documentation are UTF-8 without BOM.
- Line endings are LF, except legacy Visual Studio project files that must
  stay CRLF.
- The `hlp/` directory remains CP932 because Microsoft HTML Help Workshop
  cannot compile the files as UTF-8.
- Portable configuration files are UTF-8 only. Reading old CP932 legacy
  `np2.ini` files is not part of the phase-2 portable frontend.
- On Windows, the SDL2 frontend keeps paths as UTF-8 internally and
  converts them at the filesystem boundary to UTF-16.

This policy is about the repository and host frontend. It does not mean
the emulated guest machine is UTF-8; the PC-88VA and PC-98 software
environments keep their original character encodings and ROM behavior.

The frozen Visual Studio reference build uses UTF-8 source input with a
CP932 execution charset where that is required for legacy Win32 behavior.

## Legacy Reference Build

The original Win9x Visual Studio projects are still kept as a behavioral
reference. They are not the portability target.

- `win9x/np2_v141.sln` is the VS2017 v141 reference solution.
- Older `.dsp`, `.dsw`, `.vcproj`, and `.sln` files remain in the tree for
  comparison and migration history.
- Do not refactor the legacy build when working on the portable frontend.

The portable build uses the C CPU and VA cores, SDL2 for host I/O, and
Dear ImGui for the host GUI. The legacy build remains useful until the
modernized tree has fully replaced it.

## Documentation Map

- [BUILD.md](BUILD.md): current Windows, Linux, macOS build recipes.
- [sdl2/README.md](sdl2/README.md): SDL2 frontend runtime behavior.
- [docs/agents/ROADMAP.md](docs/agents/ROADMAP.md): modernization
  milestones and gate history.
- [docs/agents/CONVENTIONS.md](docs/agents/CONVENTIONS.md): repository
  invariants for contributors and agents.

## Status

The project is actively modernizing an old emulator codebase. The SDL2
frontend is the path forward for Windows, Linux, and macOS. The legacy
reference exists to prevent behavior drift while the portable build
continues to absorb PC-88VA-specific functionality.

## License Status

This is the current license map for the repository. It is a summary, not
a replacement for the original notices, source headers, and license files.

- Original emulator lineage: this fork is derived from project-vaeg and
  Neko Project II. The historical `win9x/readme.txt` records that vaeg
  follows the Neko Project II terms and that its source code is under a
  modified BSD-style license.
- Neko Project II attribution: `win9x/readme.txt` credits "Neko Project
  II (c) NP2 developer team, 1999-2001,2003,2004".
- Z80 emulation attribution: `win9x/readme.txt` records the PC-8801
  emulator M88 source as the basis for Z80 emulation, credited as
  "M88 - PC8801 Series Emulator, Copyright (C) by cisc 1998, 2002."
- New phase-2 code and documentation by Nakata Maho are licensed under
  the 2-clause BSD license. New files carry the full notice in their file
  header; the required header template is in
  `docs/agents/CONVENTIONS.md`.
- Dear ImGui is vendored under `external/imgui/` and is licensed under
  the MIT license. See `external/imgui/LICENSE.txt` and
  `docs/agents/DECISIONS/ADR-0004-imgui-vendor.md`.
- The bundled host GUI font `assets/NotoSansJP-Regular.ttf` is licensed
  under the SIL Open Font License 1.1. See `assets/OFL.txt` and
  `assets/NOTICE.md`.
- SDL2 is normally provided by the operating system or package manager.
  The optional MinGW cross-build FetchContent path uses SDL2 2.32.10,
  which is zlib-licensed, as recorded in
  `docs/agents/DECISIONS/ADR-0006-sdl2-acquisition.md`.
- Machine ROM images, guest font ROMs, optional mechanical sound WAV
  files, and operating system disks are not distributed by this
  repository. They remain governed by their own rights and licenses.

When changing existing files, keep their existing notices intact. When
adding new files, use the 2-clause BSD header for Nakata Maho-authored
phase-2 work unless the file is third-party code or an explicitly
documented asset.
