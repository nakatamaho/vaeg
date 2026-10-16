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

Status: **complete; G109a passed on 2026-10-16**

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

## Hardware checks requested (original PC-88VA and PC-88VA2)

The maintainer's original PC-88VA works again (2026-10-14); these are to be
run on it and, where marked, on the PC-88VA2. Disk: the self-booting SGP
disk (`demos/disks/sgp-boot.d88.xz`, menu letters below) unless stated.
Record each result here with its model, whatever it shows.

1. SGPTEST (menu O), VA and VA2: photograph the screen; record every `*`
   line and the T21 line.
   - T06-T12 show how the hardware reads the low bits of FBW. M97b's
     VA-only rule came from the BNN manual alone and M109a replaced it from
     software behaviour; neither is measured.
   - T21 times one transfer. vaeg gives about 625 polls on the VA and 11
     on the VA2, far from the 2:1 SGP clock ratio; the two hardware values
     calibrate vaeg's SGP speed.
2. The commercial game of this task, VA: its vertically reversed enemies
   must not be striped (turns the M109a evidence from derived to measured).
3. Open original-VA defects of the bug-fix ledger, VA: GLASS, NEON3 and
   NEON4 (all-demos disks) and the glyph blocks of SGPD_7A (menu A). Correct
   on hardware means a vaeg fault; the same fault means hardware behaviour.
4. If time allows:
   - SGP demo A frame rate, VA (the VA2 shows 30 FPS against vaeg's 60).
   - `V480PAT A` and `TSPMODE` (menu L, M), VA: M104 measured them only on
     the VA2, and the VA has 64 KiB of TVRAM.

## Hardware results (2026-10-16)

PC-88VA (photograph, maintainer): SGPTEST T01-T06, T08 and T10-T20 equal
vaeg; T07 85F1 (vaeg 5006) and T09 773C (vaeg 2A25), the source FBW
FFF2h and FFF0h; T21: 1171 polls, 3 frames. FFF3h (T06) and FFF4h (T08)
agree with vaeg's decoding (-12, -10).

- No source FBW from -64 to 25 reproduces 85F1 or 773C in vaeg (each value
  forced through the decoder and SGPTEST rerun), so the hardware result is
  not a different FBW alone. SGPDUMP (menu P, `-DDUMP`) shows what the
  hardware wrote for T07 and T09 to find the rule.
- The text cursor was a stray multicoloured bar after SGPTEST, also right
  after boot. In vaeg the self-booting disk's menu shows a red bar before
  any program runs: the loader starts the text BIOS without an operating
  system, and the cursor's sprite pattern is never defined. The loader now
  turns the cursor off (INT 83h 25h, AL = 00h); not an SGPTEST fault.
  SGPDUMP also prints port 153h before SGP use and after its restore (41h,
  51h, 41h in vaeg on the VA).

### SGPDUMP on hardware (2026-10-16)

PC-88VA and PC-88VA2 (photographs, maintainer), menu P:

- Port 153h reads 41h before SGP use, 51h is written, and 41h is read back
  after the restore on both models, as in vaeg: the read-modify-write does
  not change the memory map. With the loader's cursor turned off, no stray
  cursor is shown.
- T07 (FBW FFF2h): CRC 74AD on both models; SGPTEST had given 85F1 on the
  VA. T09 (FFF0h): 75B6 on the VA, 7E32 on the VA2 (SGPTEST: 773C on the
  VA). The same transfer gives different results between runs and between
  models, so with these FBW values the hardware reads bytes that the test
  does not set.
- What was written (rows from DST + 9 lines, 13 bytes from DST + 8): T07
  leaves rows 9-15 and 19 at the fill pattern, changes row 16 a little,
  fills row 17 with FFh and writes source data into row 18. T09 writes
  FFh (VA) or a repeated CB FF pattern (VA2) into rows 9-17 from byte 1 on,
  and the same rows 16-18 as T07. vaeg writes source data into rows 9-20
  for both.
- [DERIVED] With a negative source FBW whose low bits are 10b or 00b
  (FFF2h, FFF0h) and VD, the hardware does not step through the source as
  vaeg does; its output depends on memory outside the test's buffers and
  is not reproducible. FFF3h and FFF4h, the values software uses, agree
  with vaeg. T07 and T09 therefore cannot be checked by a fixed CRC.

### SGPTEST on the PC-88VA2 (2026-10-16)

PC-88VA2 (photograph, maintainer): the same 18 tests equal vaeg; T07 85F1
(as on the VA) and T09 7CB8 (VA: 773C); T21: 963 polls, 3 frames. T21
takes 3 frames on both models, while vaeg gives about 625 polls on the VA
and 10 polls (0 frames) on the VA2: vaeg's VA2 SGP is far too fast (input
to issue #38).

SGPTEST now shows T07 and T09 with "?" and does not count them, since
their hardware results vary; every other test is compared.

## Gate G109a (human)

Standard V3 gate (clean static MinGW build of the pushed commit), plus: the
game's enemies are drawn without stripes in vaeg's VA mode; SGPTEST from the
self-booting disk runs on the PC-88VA2 and its screen is recorded whatever
the differences are.

### G109a

- Passed on 2026-10-16 (maintainer), with the Windows build
  `4d080b243e8d...9faf` (SHA-256, static MinGW) from
  [9d79ae4d](https://github.com/nakatamaho/vaeg/commit/9d79ae4d050720e1e0701d416a86452dfb5b8652):
  the commercial game's vertically reversed enemies are drawn without
  stripes in vaeg's VA mode; SGPTEST and SGPDUMP were run on the PC-88VA
  and the PC-88VA2 and their screens recorded above.
