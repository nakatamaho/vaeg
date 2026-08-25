<!--
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
-->

# NEON RELAY 4 P3 VA skeleton

P2 was approved with `GO WITH RESTRICTIONS`.  This milestone adds only the
320x200 skeleton; it does not connect the eight NEON4 scenes, 640x200, OPNA,
or BITBLT.

## Stages

`demos/neon4/src/neon4_p3.asm` is built with `NEON4_STAGE=1..6`:

| Stage | Exercise |
|---|---|
| N4-1 | Flat entry, stack, segment setup, and loader return; no video call. |
| N4-2 | VA 320x200 entry followed by a CPU RGB332 solid clear. |
| N4-3 | CPU-written 32-column direct RGB332 bar pattern. |
| N4-4 | TSP VB polling and a per-VB marker-band update until `ESC`. |
| N4-5 | SGP `SET_WORK` + `SET_COLOR` + `CLS` + `END` clear. |
| N4-6 | CPU page-A clear, SGP page-B clear, FB0 DSA page exchange at VB. |

The payload uses the VA keyboard BIOS for `ESC` and the validated loader return
continuation.  It does not use DOS `INT 21h`.

## 8bpp calibration boundary

The existing payloads prove `INT 8Fh` `BX=E00Eh`, `CX=0404h` for 320x200 4bpp,
but no in-tree payload proves the 8bpp `CX` argument.  P3 therefore enters the
proven 4bpp transaction and then applies the documented/reconstructed direct
mode registers:

```text
GRRES = 0012h       ; G0: 320 dots, 8bpp
FB0.FBW = 320       ; bytes per source row
FB0.FBL = 199
FB0.DSH = 200
RGB composition = G0 direct (0008h)
```

These writes are explicitly marked `NOT VERIFIED ON VA SILICON` in the source.
`-dNEON4_DIRECT_REGS=0 -dNEON4_PIXEL_ARGS=0808h` remains available for a
separate BIOS-argument experiment; it is not the default path.

## Build

```sh
demos/neon4/build_p3.sh 1 /tmp/N4-1.COM
demos/neon4/build_p3.sh 2 /tmp/N4-2.COM
demos/neon4/build_p3.sh 3 /tmp/N4-3.COM
demos/neon4/build_p3.sh 4 /tmp/N4-4.COM
demos/neon4/build_p3.sh 5 /tmp/N4-5.COM
demos/neon4/build_p3.sh 6 /tmp/N4-6.COM
```

The script wraps each raw payload with the existing NEON3 VA loader so the
COMs use the same `3000:0000` entry and loader continuation as the validated
NEON3 gate.  A distribution D88 is a local test artifact and is not part of
this source commit.

## Gate checks

On VAEG or hardware, run each stage from the validation prompt:

1. N4-1 returns without changing the screen.
2. N4-2 is one uniform direct-colour page.
3. N4-3 shows 32 distinct RGB332 bars.
4. N4-4 changes the marker band only after VB edges and exits on `ESC`.
5. N4-5 is pixel-identical to N4-2; this is the first SGP/CPU gate.
6. N4-6 changes from the page-A colour to the page-B colour only after the
   SGP idle poll and VB edge.

VAEG headless guest execution was not available in this checkout because the
model ROM set was incomplete (`vafont` missing).  Therefore N4-2/N4-5 image
equality and FB0 page switching remain `HUMAN_GATE_PENDING`; no hardware timing
or silicon compatibility claim is made here.
