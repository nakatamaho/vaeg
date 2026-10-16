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

# M115b — PCG-8100/8800 sample program

Status: **complete; G115b passed on 2026-10-16; merged to `main` at [70d506b](https://github.com/nakatamaho/vaeg/commit/70d506bf40b0b1e03674472cb68da5eeaeefe50e)**

## Request

Add the PCG sample written during G115 to the repository (maintainer,
2026-10-16).

## Work

- `demos/pcg-sample/pcg.bas`: four user characters, a scale on 8253
  counter 0 and a walking figure; N-BASIC and N88-BASIC; BSD header as
  comment lines 1-18, the program from line 100.
- `demos/pcg-sample/README.md`: what each part does, and the write order
  (address bit 12 pulsed) that works on every emulator.

## Verification

- vaeg (VA2, V2 BASIC, PCG on): the pasted program runs; the four
  characters show whole (eight rows each) and the figure is drawn eight
  times. The maintainer ran the earlier version in VA2 N mode.

### G115b

- Passed on 2026-10-16 (maintainer).
