<!--
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.
-->


# M103f - V1/V2 text addressing and multi-image media

Status: **in progress**

Series: V1/V2 mode (ADR-0016). Branch `topic/m103f-v1v2-text-media` off
`topic/m103e-v1v2-srdemo-sound` at `e4f16e577a6f09ab614aea0ea737ca976d950366`
(M103e, G103e pending; the maintainer asked to continue to M103f before the
M103d and M103e gates). Commit prefix `M103f:`.

## Goal

Take over the items M103e left open: the stray text that GAME-A shows
after it stops the 3301 display, and loading the second disk of a
multi-image D88 without splitting the file. V1 mode remains in the series
plan but needs V1 media, which is not available here.

## Scope

1. **TSP byte-mode addresses.** The rule that turns a split-screen start
   address into a TVRAM byte address in 3301 emulation.
2. **Multi-image D88.** Selecting an image inside a D88 file that holds
   several disks, from the command line (and the GUI if it fits).
3. **Tests** for both.

Out of scope: V1 mode (no media), sound beyond the M103e gate check.

## Gate G103f (human)

Standard V3 gate unchanged, plus: GAME-A, loaded from its original
two-image file, shows its screens without the text grid; SR-DEMO,
MA2-DEMO and V2 BASIC text still display as at G103e.

## Implementation progress

### TSP byte-mode addresses (scope item 1)

- `[VA-TM]` BNN §8.2.1: in V1/V2 mode the TSP runs in byte-access mode,
  and its local word addresses 3000h–37FFh and B000h–B7FFh (V3) become
  byte addresses 3000h–3FFFh and B000h–BFFFh (the halves from 3800h and
  B800h are unusable). The 4 KiB window is TVRAM A6000h–A6FFFh.
- M103c used TVRAM byte = 3000h + local, where local is half the table's
  start field. That matches the 3000h range (BASIC: 678Ch → local 33C6h →
  63C6h) but maps B000h to E000h instead of 16000h. `[DERIVED]` Keeping the
  low 12 bits and doubling the upper four, `((L & F000h) << 1) |
  (L & 0FFFh)`, matches both documented ranges.
- `[ROM]` GAME-A stops the text with a 3301 RESET; the VA2 ROM then sets
  the start field to 1000h (local 0800h). The old rule read TVRAM 3800h,
  where the ROM keeps words 0080h/00E8h, and showed them as characters;
  the new rule reads TVRAM 0800h, which is clear.
