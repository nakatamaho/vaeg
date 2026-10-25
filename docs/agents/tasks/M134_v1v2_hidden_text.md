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

# M134 — V1/V2 hidden text with monochrome graphics

Status: **implemented; G134 human gate pending.**

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
  the PC-8801mkIISR demonstration shows the text hidden (148h = 80h) with
  port 31h = 00h while it starts, where the VA shows the blocks, and
  `OUT 53h,01h` with port 31h = 19h (colour graphics) over its title, where
  the maintainer saw them go. The colour-mode rule (whole text hidden) is
  derived from this sequence.
- PR #56's PC-8801 behaviour (characters and backgrounds hidden, colours
  kept for the graphics) is taken from M88 and the PC-8801mkIIFR
  demonstration; it is kept for the PC-8801/8001 models from `extra/`, as
  in M129.

## Changes

- [b55ce02a](https://github.com/nakatamaho/vaeg/commit/b55ce02a2e8635b8f448aae7ce5a55286f89e4f8)
  (author shitamo, PR #56): keep the colour raster while the text is hidden
  with monochrome graphics.
- [c0fbaa21](https://github.com/nakatamaho/vaeg/commit/c0fbaa2137a96240f8c651a852772b3569167fb0):
  VA boots keep the cell backgrounds with glyphs and lines suppressed;
  PC-8801/8001 models blank the text raster; the rule applies only in V1/V2
  mode with port 31h bit 4 clear (colour graphics and V3 hide the whole
  text). Selftest `TSP text hide`.
- V1/V2 decision X10 (and a note on G4), ledger entry, roadmap row.

## Gate G134 (human)

Standard V3 gate, plus on the VA/VA2 and in vaeg:

- the BASIC discriminator above: box and blank key blocks on VA and VA2,
  box only on the PC-8801mkIISR model;
- `SCREEN 0` (colour) then `OUT &H53,1` on a VA: the whole text, key blocks
  included, should disappear (the derived colour-mode rule);
- the PC-8801mkIISR demonstration on the VA: blank key blocks while it
  starts, none over its title or its RGB prompt;
- if available, the PC-8801mkIIFR demonstration's fishing scene draws.

## Local verification

- `--selftest`: exit 0, `TSP text hide ok`. Mutations caught: no
  colour-mode condition ("the VA kept hidden text with colour graphics"),
  backgrounds kept on a PC-8801 model, glyphs not suppressed ("a hidden
  glyph was drawn").
- Linux release build with ROM sets matching MAME's `pc88va` and
  `pc88va2`: the BASIC discriminator gives box and blank key blocks on VA
  and VA2, box only on the PC-8801mkIISR model; the PC-8801mkIISR
  demonstration on the VA shows the blocks at frame 150, the BASIC labels at
  200, and no blocks over the title (260-900) or the RGB prompt; the
  PC-8801mkIISR model shows no blocks while it starts.
