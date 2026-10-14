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
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.
-->

# M109a — SGP pitch on the original VA, and an SGP regression test

Status: **in progress; G109a human gate pending**

## Request

The maintainer reported (2026-10-14) that in vaeg's VA (not VA2) mode some
enemies of a commercial game were drawn as horizontal stripes, and asked for
an SGP regression test that can also run on real hardware.

## Findings

- `[DERIVED]` The striped sprites are BITBLT 0905h (VD, TP=1) transfers
  whose source descriptor holds FBW `FFF3h` for 12-byte lines. The command
  stream is the same on both models. vaeg read it as -12 on the VA2 (bit 0
  dropped, a negative pitch two bytes shorter) and, since M97b, as -16 on
  the VA (low two bits dropped, from the BNN rule that they are zero). With
  -16 every source line moved 4 bytes, giving the stripes. The game was
  written for the original VA, so the VA reads the value as -12 too.
- Checked on the way and ruled out: the CPU rewriting the SGP list or its
  sources during a transfer, CPU GVRAM writes while the SGP is busy, starts
  while busy, aborts; TSP sprites (the stripes stay with sprites hidden).
- vaeg's SGP timing differs from the 2:1 VA/VA2 clock ratio by far: one
  320x100 4bpp copy takes about 625 status polls on the VA and 11 on the VA2.
  Recorded for the hardware comparison (SGPTEST T21), not changed here.

## Work

- FBW decoded the same way on both models (`sgp_decode_fbw`), width and
  height masks stay per model; selftest: `FFF3h` gives -12 on both, a
  positive odd pitch gives the same on both. In vaeg's VA mode the game drew
  no striped sprite in 15 sampled scenes (frames 3500-7000), including the
  formation that striped before. [d288656c](https://github.com/nakatamaho/vaeg/commit/d288656ce79e1c56eeb2022647079a8c498e1f23).
- `tools/pc88va/sgptest`: SGPTEST.COM, twenty CRC tests (BITBLT 4/8/16bpp,
  start dots, VD, HD, TP=1, the FBW values FFF3h/FFF2h/FFF4h/FFF0h and
  000Ch/000Dh/000Eh, 1bpp expansion, PATBLT, LINE, CLS) and one timing, all
  in the program's own main-RAM buffers; expected values generated from
  vaeg on both models (they agree), `update-expected.py` to regenerate,
  CTest `vaeg_sgptest_consistency` (five mutations caught). In vaeg the VA
  and VA2 runs show 0 differences under PC-Engine 1.1 and on the
  self-booting disk; with M97b's VA rule restored the VA run shows 5
  (T06-T09, T12). [43ddd672](https://github.com/nakatamaho/vaeg/commit/43ddd6727ef12c2bb41b40785712605be338d7da).
- Self-booting SGP disk: menu item O runs SGPTEST; rebuilt, reproducible,
  round-trip checked. [d3680ae1](https://github.com/nakatamaho/vaeg/commit/d3680ae110f583b3edb99b10af5a89f78f2f36ee).

## Gate G109a (human)

Standard V3 gate (clean static MinGW build of the pushed commit), plus: the
game's enemies are drawn without stripes in vaeg's VA mode; SGPTEST from the
self-booting disk runs on the PC-88VA2 and its screen is recorded whatever
the differences are.
