# 88VA Eternal Grafx

[![build](https://github.com/nakatamaho/vaeg/actions/workflows/build.yml/badge.svg)](https://github.com/nakatamaho/vaeg/actions/workflows/build.yml)

88VA Eternal Grafx, or `vaeg`, is a maintained fork of the abandoned
`project-vaeg` PC-88VA emulator lineage, itself derived from Neko Project
II. This fork is the living tree: the old upstream should be treated as
historical source material, not as the active project.

The active product is a portable PC-88VA emulator that builds and runs
on modern Windows, Linux, and macOS systems. A frozen Visual Studio
reference tier is kept for behavior archaeology, but normal development
targets the CMake/SDL2 tree.

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

Save-state files are local runtime artifacts. They are not portable
across architectures, compilers, or build families; do not move a state
file between 32-bit and 64-bit builds, between legacy and portable
builds, or between different host platforms and expect it to load.

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

## PC-88VA Hardware Notes

The list below summarizes the PC-88VA hardware described by the PC-88VA
Technical Manual. The manual mainly describes the first PC-88VA; its
Music BIOS and ADPCM BIOS sections also cover VA2/VA3 and Sound Board II
behavior.

- Main CPU: V30/Z80-instruction-compatible CPU at 8 MHz. The manual
  describes V1/V2 compatibility timing relative to the older uPD780/Z80
  software environment, but does not name a separate main CPU package.
- Disk subsystem CPU: Z80-equivalent 4 MHz sub CPU with 8 KB ROM and
  16 KB RAM for intelligent FDD operation.
- Interrupt control: uPD8214-equivalent 8-level mode for V1/V2
  compatibility, and uPD8259-equivalent 13-level mode for V3 operation.
- DMA: four-channel priority DMA unit; channel 2 is assigned to the FDD
  interface, with channels 0 and 3 exposed to the bus slots.
- Timers: CPU internal timer/counter unit with uPD8253-compatible
  behavior; the counters are used for the general timer, BEEP frequency,
  and RS-232C baud generation. The FDD interface also has a motor-control
  timer, and the sound controller has its own timers.
- FDD controller: uPD765-compatible FDC for the internal 5-inch
  2HD/2D drives.
- Serial controller: uPD8251-compatible USART for RS-232C.
- Calendar clock: uPD4990/uPD4990AC-compatible battery-backed clock.
- Parallel/scanner/system ports: uPD8255-compatible parallel port
  interface appears in the scanner and system-port descriptions.
- Video system: SGP drawing processor, TSP/DPMC display composition,
  TVRAM, GVRAM, CGROM/CGRAM, 4096-color palette, sprite/text/graphics
  priority composition, and an optional video-board digitize path.
- Sound: the base sound controller is YM2203/OPN-class, providing three
  SSG voices and three FM voices, alongside BEEP and port sound. VA2/VA3
  and VA Sound Board II Music BIOS support YM2608/OPNA, adding six-FM
  operation, rhythm functions, and extended channel control.
- Other I/O: intelligent keyboard interface, Centronics-compatible
  printer interface, mouse/joystick/tablet port, optional hard disk
  interface, two general PC-98-compatible expansion slots, and one
  dedicated video-board slot.

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

The frozen Visual Studio reference files use UTF-8 source input with a
CP932 execution charset where that is required for legacy Win32 behavior.

## Frozen Reference Tier

The original Win9x Visual Studio reference tier is still kept, but it is
not the active product and it has no CI compile guarantee.

- `win9x/np2_v141.sln` is the VS2017 v141 reference solution.
- `win9x/` contains the frozen Win32 frontend, project files, resources,
  and NASM helper files.
- `i286x/` and `cpuxva/memoryva.x86` are the frozen assembly CPU and VA
  memory references used by the v141 tree.
- `hlp/` is the CP932 HTML Help payload paired with the frozen Win32
  tree.

Do not refactor or improve the frozen tier during portable work. It was
kept because same-tree v141 comparison was decisive during the G9 VA
debugging chain: differential FDC traces and the legacy V30 DMA pump
identified the portable defect. Future fixes should land in the active
CMake/C/SDL2 tree unless a task explicitly says to update the reference
tier.

## Documentation Map

- [BUILD.md](BUILD.md): current Windows, Linux, macOS build recipes.
- [sdl2/README.md](sdl2/README.md): SDL2 frontend runtime behavior.
- [docs/agents/ROADMAP.md](docs/agents/ROADMAP.md): modernization
  milestones and gate history.
- [docs/agents/CONVENTIONS.md](docs/agents/CONVENTIONS.md): repository
  invariants for contributors and agents.

## Status

The phase-2 portable tree is the path forward for Windows, Linux, and
macOS. The active build is CMake/C/SDL2/Dear ImGui. The frozen reference
tier remains for historical comparison and behavior archaeology.

## License Status

This is the current license map for the repository. It is a summary, not
a replacement for the original notices, source headers, and license files.

- Original emulator lineage: this fork is derived from project-vaeg and
  [Neko Project II (NP2)](http://www.retropc.net/yui/np2help.html).
  The historical `win9x/readme.txt` records that vaeg follows the Neko
  Project II terms: "Neko Project II に準じます。" It also records the source
  license as: "ソースコードは 修正BSDライセンスとします。"
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
