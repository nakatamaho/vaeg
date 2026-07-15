# Bundled Asset Notice

## Z80 CPU core

The production PC-88VA subsystem includes the pinned `suzukiplan/z80` header
under the MIT License, Copyright (c) 2019 Yoji Suzuki.

- Upstream: `https://github.com/suzukiplan/z80`
- Approved base: `e3926769a790fab0af1c34a5540e317f8d4f0ddc`
- Approved patch SHA-256:
  `d8624085139ef4e7b400b918b2b498e79bea1af4a1942e4ac935545846e746a4`
- Reproduced tree: `8a606eb39332a6e79b69bb62d9dedca042b923dc`
- License text: `external/suzukiplan-z80/LICENSE.txt`
- Full provenance: `external/suzukiplan-z80/provenance.txt`

The vaeg wrapper, revision-1 codec, and disassembler are independently
authored BSD-2-Clause code. The removed historical M88/cisc-derived Z80 files
were not relicensed; Git history was not rewritten.

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

`vaeg.bmp` is the 320x200 startup graphic used by the frozen Win9x VAEG
frontend. It was copied byte-for-byte from `hlp/vaeg.bmp` into the active
asset directory and is embedded in the SDL2 executable at build time, so the
runtime does not depend on either asset path.

- SHA-256:
  `ad68394eb52a7cc75d9759a83982132725ddb66c4bd2260662526d67b8ce0c4e`

## Historical application icon

`vaeg.ico` is the VAEG application icon used by the frozen Win9x frontend.
It was copied byte-for-byte from `win9x/icons/np2.ico` into the active asset
directory. CMake embeds the unchanged ICO in every SDL2 executable for the
runtime window icon; Windows builds also use it as the native executable icon
resource.

- SHA-256:
  `a27533f679a31fdb8e2812c1d4906e705e544ba49b976154dde6794ce31a32f4`
