<!--
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
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

# M134 — V1/V2 hidden text with monochrome graphics

Status: **G134 human gate passed on 2026-10-25.**

Branch `milestone/m134-text-hide`; commit prefix `M134:`.

## Request

[PR #56](https://github.com/nakatamaho/vaeg/pull/56) (shitamo): in V1/V2
monochrome graphics (multiplane 1 bit/pixel, G4) `OUT 53h` bit 0 hid the
text and vaeg blanked the colour raster too, so the PC-8801mkIIFR
demonstration's fishing scene went black; on a PC-8801 port 53h bit 0 hides
only the characters.

The maintainer (2026-10-25): take the PR as for M129 (authorship kept,
ledger, selftest, decision record, milestone). Hardware photographs of
the review's discriminator on both machines (kept privately).

## Findings

- `[MEAS]` (maintainer) PC-88VA and PC-88VA2, V2 N88-BASIC:
  `SCREEN 1,0:CLS 3:LINE(100,50)-(500,150),7,B:PRINT "VISIBLE TEXT"`, then
  `OUT &H53,1`. The text disappears, the box stays, and the five reversed
  function-key labels become blank white blocks.
- The blocks are the "tofu" of
  [issue #50](https://github.com/nakatamaho/vaeg/issues/50): a vaeg trace of
  the PC-8801mkIISR demonstration shows the text hidden (148h = 80h) while
  it starts, where the VA shows the blocks, and `OUT 53h,01h` with port 31h
  = 19h (colour graphics) over its title.
- The maintainer (2026-10-25): the blocks belong to every V1/V2 boot of a
  VA or VA2, whatever the graphics mode, and to no other machine. A first
  version hid the whole text in colour mode, derived from the
  demonstration; it was replaced. In the gate build the maintainer saw
  the demonstration's blocks go as on hardware.
- PR #56's PC-8801 behaviour (characters and backgrounds hidden, colours
  kept for the graphics) is taken from M88 and the PC-8801mkIIFR
  demonstration; it is kept for the PC-8801/8001 models from `extra/`, as
  in M129.

## Changes

- [b55ce02a](https://github.com/nakatamaho/vaeg/commit/b55ce02a2e8635b8f448aae7ce5a55286f89e4f8)
  (author shitamo, PR #56): keep the colour raster while the text is hidden
  with monochrome graphics.
- [c0fbaa21](https://github.com/nakatamaho/vaeg/commit/c0fbaa2137a96240f8c651a852772b3569167fb0),
  [cc201696](https://github.com/nakatamaho/vaeg/commit/cc20169600d7d2fbe83930426e45d94089063d4b):
  VA boots keep the cell backgrounds with glyphs and lines suppressed in
  every V1/V2 graphics mode; PC-8801/8001 models blank the text and keep the
  colours only for monochrome multiplane graphics; V3 hides the whole text.
  Selftest `TSP text hide`.
- [f845a379](https://github.com/nakatamaho/vaeg/commit/f845a379b5a435b65d638431aa07e0cf3199f6b8):
  the media builders find cached packages whatever the case of their names.
- V1/V2 decision X10 (and a note on G4), ledger entry, roadmap row.

## Gate G134 (human)

Standard V3 gate, plus:

- the BASIC discriminator above: box and blank key blocks on VA and VA2,
  box only on the PC-8801mkIISR model (**maintainer: OK, 2026-10-25**);
- `SCREEN 0` then `OUT &H53,1`: blank key blocks on VA and VA2 too;
- the PC-8801mkIISR demonstration on the VA: blank key blocks while it
  starts;
- if available, the PC-8801mkIIFR demonstration's fishing scene draws.

## Local verification

- `--selftest`: exit 0, `TSP text hide ok`. Mutations caught: VA blanked
  in colour mode ("the VA lost backgrounds with colour graphics"), PC-8801
  model keeping hidden text in colour mode, glyphs not suppressed or not
  blanked on a PC-8801 model ("a hidden glyph was drawn").
- Linux release build with ROM sets matching MAME's `pc88va` and
  `pc88va2`: the BASIC discriminator with `SCREEN 1` and with `SCREEN 0`
  gives box and blank key blocks on VA and VA2 and the box only on the
  PC-8801mkIISR model; the PC-8801mkIISR demonstration on the VA shows the
  blocks from frame 150 on, also over its title and RGB prompt; the
  PC-8801mkIISR model shows none.

## Media build regression

Clean clone of `cadbb4068a73a8d2f85a3a1e622a0d881ae4c974`, cache
`~/work/pc88va/pc88va-private-docs/pc88va-utilities-cache/`,
`TMPDIR=~/work/pc88va/tmp/m134/tmp`:

- `build-utility-disk.sh` (PC-Engine 1.1): exit 0, 4096 bytes free.
- The five demo `build-d88.sh`, `build-from-psd.sh`, all-demos,
  all-demos-msdos4 (MS-DOS 4.0 fetched into the cache and checked) and the
  SGP boot disk (at `b0de69a1`; no demo or demo-builder input changed
  since): exit 0, every file identical to the committed images; the
  all-demos-msdos4 and SGP boot `.xz` byte-identical.
- `build-sasi-utility-disks.sh`: **NOT RUN to completion (blocked)**. After
  the case fix it stops at `PCP_7A.LZH`: eleven packages (FATMAP11.DOC,
  FD98_232.LZH, FD_VA.LZH, MARINVA.DOC/.LZH/.LZP, NYANCO25.DOC/.LZH,
  PCP_7A.LZH, PCP_CA.LZH, VASG100.LZH) are absent from the new cache, and
  the softlib site answers 403. The maintainer passed the gate with this
  open; the SASI images need these packages in the cache.

## Clean-checkout build

`tools/release/build-mingw-static-container.sh --commit
cadbb4068a73a8d2f85a3a1e622a0d881ae4c974`: commit embedded, Windows system
DLL imports only; `vaeg.exe` SHA-256
`292f0f0acb3bc1c700b7bb3a8201a9f9da9ba17023f797dfedae1636407a5749`.
`linux-ci-gcc` ctest: 119 of 119 passed.

## G134 passed

The maintainer stated **G134 passed** on 2026-10-25 with that build: the
BASIC discriminator as on hardware, and the PC-8801mkIISR demonstration's
blank blocks going as on hardware.
