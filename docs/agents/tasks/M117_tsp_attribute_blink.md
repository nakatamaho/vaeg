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

# M117 — TSP attribute modes 4-7 and blink

Status: **implemented; G117 human gate pending**

## Request

The maintainer reported (2026-10-16) that text which blinks on the hardware
does not blink in vaeg: the blink attribute is not read. The TSPMODE record
(M104) itself is valid.

## Cause

`vram/maketextva.c` decoded only attribute modes 0 and 1; modes 2-7 were all
read as mode 0 (bits 7:4 background, 3:0 foreground, no attributes). Modes 4
and 5 keep blink in bit 7 (Technical Manual), so their blink was lost and
bit 7 showed as a brighter background (F0h: grey instead of white).

## Work

- Modes 4 and 5: bit 7 blink, bits 6:4 background, 3:0 foreground; mode 5
  adds the horizontal line when `(foreground & 7) == 1`.
- Modes 6 and 7 (undocumented): 4-bit background and foreground, mode 6
  with bit 7 as blink, mode 7 without; matches the PC-88VA2 TSPMODE
  observations with F0h (6: grey, blinking; 7: grey, steady).
- Modes 2 and 3 are unchanged (still read as mode 0).
- Selftest `TSP attribute modes`, mutation-checked.

## Verification

- TSPMODE 4 under PC-Engine (VA2, vaeg headless): before, the block stays
  grey and identical in every sampled frame; after, it is white on the
  white main split and disappears and returns with the blink phase.
- ctest 93/93, selftest all passed.

## Gate G117 (human)

Standard gate, plus: the software where the maintainer saw blinking on the
hardware blinks in vaeg; TSPMODE 4-7 matches the M104 record.
