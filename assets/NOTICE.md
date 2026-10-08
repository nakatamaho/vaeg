# Bundled Asset Notice

## Shared Z80 compatibility core

The production PC-88VA subsystem includes the pinned `suzukiplan/z80` header
under the MIT License, Copyright (c) 2019 Yoji Suzuki.

- Upstream: `https://github.com/suzukiplan/z80`
- Approved base: `e3926769a790fab0af1c34a5540e317f8d4f0ddc`
- Approved patch SHA-256:
  `d8624085139ef4e7b400b918b2b498e79bea1af4a1942e4ac935545846e746a4`
- Reproduced tree: `8a606eb39332a6e79b69bb62d9dedca042b923dc`
- Source license text: `external/suzukiplan-z80/LICENSE.txt`
- Runtime-package copy: `licenses/suzukiplan-z80.txt`
- Full provenance: `external/suzukiplan-z80/provenance.txt`

The vaeg wrapper, revision-1 codec, and disassembler are independently
authored BSD-2-Clause code. The removed historical M88/cisc-derived Z80 files
were not relicensed; Git history was not rewritten.

## CPU-role naming

The active tree distinguishes the emulated hardware role from the shared
instruction implementation:

- `cpu/upd9002_upd70008.*` is the uPD9002 main-CPU adapter's
  uPD70008-compatible mode.
- `io/subsystem.cpp` uses the shared implementation for the FDC's
  `UPD780C`-named CPU instance.
- `cpu/z80_compat_cpu.*`, `z80_compat_bus.h`, `z80_compat_registers.h`, and
  `z80_compat_state.*` remain the common Z80 compatibility backend and
  compatibility files. `cpu/upd780/upd780_disasm.*` is the FDC-facing uPD780
  disassembler. The remaining Z80 terminology describes the suzukiplan
  instruction backend, not an additional FDC device or a claim that the FDC is
  a generic Z80. The FDC-facing API and diagnostics use the uPD780 name.

The `UPD9Z80` save-state section identifier is retained intentionally so that
states created before the M76 naming clarification remain loadable.

## External V20 comparison data

The uPD9002 comparison infrastructure can use the SingleStepTests V20
`v1_native` corpus under the MIT License, Copyright (c) 2024 Daniel Balsom.
The corpus and its license are not bundled in vaeg source or runtime archives.
The exact upstream commit and verified license/content digests are recorded in
`tests/ssts/v20_dataset_manifest.json`; acquisition and verification are
documented in `tests/ssts/README.md`.

## GUI font

This directory includes `NotoSansJP-Regular.ttf`, a static Regular
instance generated from the Google Fonts `NotoSansJP[wght].ttf` source.

- Upstream: `https://github.com/google/fonts/tree/main/ofl/notosansjp`
- License: SIL Open Font License, Version 1.1
- License text: `assets/OFL.txt`
- Generated file: `assets/NotoSansJP-Regular.ttf`
- Generated SHA-256:
  `c2b5cef5710fdf6d1bd96b7e51725c9c24945cc2db59f4d31947e44b48cebde1`

The font is embedded in each active executable at build time for the host
Dear ImGui user interface. Runtime packages keep this notice and `OFL.txt`
but do not need a separate TTF. The font is not derived from, and must not
be replaced by reading from, the emulated guest font ROM.

## Historical startup graphic

`vaeg.bmp` is the 320x200 startup graphic used by the former Win9x VAEG
frontend. It was copied byte-for-byte from
[`hlp/vaeg.bmp` at the archived G56 tree](https://github.com/nakatamaho/vaeg/blob/b72e641733ddea6f0e8faef2507093f7c3aee5a4/hlp/vaeg.bmp)
into the active asset directory and is embedded in the SDL2 executable at
build time, so the runtime does not depend on the archived path.

- SHA-256:
  `ad68394eb52a7cc75d9759a83982132725ddb66c4bd2260662526d67b8ce0c4e`

## Historical application icon

`vaeg.ico` is the VAEG application icon used by the former Win9x frontend.
It was copied byte-for-byte from
[`win9x/icons/np2.ico` at the archived G56 tree](https://github.com/nakatamaho/vaeg/blob/b72e641733ddea6f0e8faef2507093f7c3aee5a4/win9x/icons/np2.ico)
into the active asset directory. CMake embeds the unchanged ICO in every SDL2
executable for the runtime window icon; Windows builds also use it as the
native executable icon resource.

- SHA-256:
  `a27533f679a31fdb8e2812c1d4906e705e544ba49b976154dde6794ce31a32f4`

## Front panel artwork

`front-panel-va.png`, `front-panel-va2.png` and `front-panel-va3.png` are
drawings of the PC-88VA, PC-88VA2 and PC-88VA3 front panels made by the
maintainer (Nakata Maho) for vaeg in 2026, and are distributed under the
same two-clause BSD terms as the rest of the repository, Copyright (c) 2026
Nakata Maho. The manufacturer's logo was removed from the drawings before
they were added; product names remain only as descriptive labels of the
hardware depicted. The images were cropped or padded to 1900x600 (VA) and
1900x750 (VA2, VA3) without resampling. CMake embeds them in the SDL2
executable, which decodes them with its own PNG decoder (`sdl2/pngdecode.c`)
for the front panel shown below the screen (`sdl2/frontpanel.c`).

- `front-panel-va.png` SHA-256:
  `af56c28b3e55cfaee74c893e2b52f9921153aba9f010c468b5b492cb5fda4698`
- `front-panel-va2.png` SHA-256:
  `8bfda3cc98245f660528c50f18bb644a9cc8a72bc331dc8804053330b733f643`
- `front-panel-va3.png` SHA-256:
  `f9be10de942237e47b7253eb28d8a35160ab611eb5b9040bc4e9f945e89c6307`
