# Bundled Asset Notice

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
