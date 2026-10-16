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

# SGPTEST — SGP regression test (M109a)

`SGPTEST.COM` runs twenty small SGP command lists and one timed transfer,
and prints a CRC of each result next to the value vaeg gives. The same
program runs in vaeg and on a real PC-88VA, so a hardware run shows at a
glance where vaeg's SGP differs.

It runs from the self-booting SGP disk
([`demos/sgp-boot`](../../../demos/sgp-boot/README.md), menu item **O**),
under PC-Engine 1.1 and under MS-DOS 4.0. It needs no options and no key
input.

## Safety

Every source and destination is a buffer in the program's own 64 KiB
segment of main RAM. No screen mode is changed, GVRAM and TVRAM are not
written, and the SCAN commands, which run until they meet a boundary
colour, are not used. Port 153h bit 4 (single-plane access, which the SGP
needs) is set only while a list runs and restored afterwards. A list that
does not finish within about two million status polls is aborted and
reported as `TIMEOUT`.

## Output

```
SGPTEST (vaeg M109a): CRC, "=" as vaeg, "*" vaeg value
T01 8D02 =      T02 B517 =      T03 0C46 =      T04 A733 =
...
differences from vaeg: 0
T21 320x100 4bpp: polls 625 frames 1
```

`Tnn CRC =` means the hardware result equals vaeg's; `Tnn CRC *VVVV`
shows vaeg's value `VVVV` after a different hardware result. The CRC
(CRC-16/CCITT, initial FFFFh) covers the destination area, 256 guard bytes
before and after included, so a transfer that writes outside its rectangle
also changes it. The destination is filled with byte `i XOR A5h` before
each test.

T21 times one 320 x 100 pixel 4bpp copy in main RAM: `polls` counts reads
of port 506h until BUSY clears, `frames` the VRTC rising edges meanwhile.
It has no expected value. In vaeg (M109a) the VA gives about 625 polls and
the VA2 about 11; the 4 MHz / 8 MHz SGP clocks would give a ratio of two,
so a hardware value tells how far vaeg's SGP speed is from the machine's.

## Tests

Pixel modes: 1, 4, 8, 16 bits per pixel. Width and height in pixels, FBW
as the descriptor holds it. Destinations have a 64-byte pitch.

| Test | Command, mode | Source | Destination | Purpose |
| --- | --- | --- | --- | --- |
| T01 | BITBLT 0005h | 4bpp 24x12, dot 0, FBW 16 | 4bpp dot 0 | plain copy |
| T02 | BITBLT 0005h | as T01 | dot 1 | destination shift |
| T03 | BITBLT 0005h | as T01 | dot 3 | destination shift |
| T04 | BITBLT 0005h | dot 2 | dot 0 | source shift |
| T05 | BITBLT 0805h (VD) | FBW 16, from its last line | from line 20 up | bottom-up copy |
| T06 | BITBLT 0905h (VD, TP=1) | 12-byte lines, FBW FFF3h | dot 3, from line 20 up | the M109a case: vaeg decodes -12 |
| T07 | as T06 | FBW FFF2h | as T06 | not compared ("?"): hardware result varies |
| T08 | as T06 | FBW FFF4h | as T06 | vaeg: -10 |
| T09 | as T06 | FBW FFF0h | as T06 | not compared ("?"): hardware result varies |
| T10 | BITBLT 0005h | 12-byte lines, FBW 000Ch | dot 0 | reference for T11, T12 |
| T11 | as T10 | FBW 000Dh | as T10 | vaeg: 12 (same as T10) |
| T12 | as T10 | FBW 000Eh | as T10 | vaeg: 14 |
| T13 | BITBLT 0405h (HD) | right end, dot 3 | right end, dot 3 | right-to-left; vaeg gives T01's image |
| T14 | BITBLT 0105h (TP=1) | as T01 | dot 1 | transparent colour 0 |
| T15 | BITBLT 0005h | 8bpp 10x10 | 8bpp dot 1 | 8bpp |
| T16 | BITBLT 0005h | 16bpp 8x10 | 16bpp | 16bpp |
| T17 | BITBLT 0005h, colour 5555h | 1bpp 24x12 | 4bpp dot 1 | 1bpp expansion |
| T18 | PATBLT 0005h | 4bpp 8x4 pattern | 24x12, dot 1 | pattern tiling |
| T19 | LINE, colour 9999h | — | four 4bpp lines, modes 0000h, 0800h (HD), 0400h (VD) | line stepping and directions |
| T20 | CLS, colour 1234h | — | 100 words | fill |

The FBW tests T06-T12 decide how the hardware reads the low bits of a
pitch, which the BNN manual says must be zero (section 7 of
[`upd92017-sgp.md`](../../../docs/modernization/upd92017-sgp.md)).

## Running it on hardware

1. Write `demos/disks/sgp-boot.d88.xz` (unpacked) to a 2HD disk, boot it,
   and press **O**; or copy `SGPTEST.COM` to a PC-Engine or MS-DOS disk
   and run it.
2. Photograph the screen. Report the model (VA, VA2 or VA3), every `*`
   line and the T21 line.

## Expected values

[`sgptest-expected.inc`](sgptest-expected.inc) holds vaeg's CRCs. It is
generated by [`update-expected.py`](update-expected.py), which needs the
maintainer's ROMs and a PC-Engine 1.1 system disk (both private, passed by
path, never committed):

```sh
tools/pc88va/sgptest/update-expected.py --vaeg build/linux-release/sdl2/vaeg \
    --roms ROM-DIR --pcengine PC-ENGINE-1.1.d88
```

It assembles the program without expected values, runs it headless on the
VA and the VA2 model, reads the CRCs from the TVRAM dump and fails unless
both models agree. Regenerate it only after a deliberate change to vaeg's
SGP, and rebuild the self-booting disk afterwards.

## Hardware results (2026-10-16)

On a PC-88VA and a PC-88VA2 every test except T07 and T09 gives vaeg's
CRC. T07 and T09 (source FBW FFF2h and FFF0h with VD) give different CRCs
between runs and between the models (SGPTEST: T07 85F1 on both, T09 773C
on the VA and 7CB8 on the VA2; SGPDUMP: 74AD, and 75B6 / 7E32): the
hardware reads bytes the test does not set. SGPTEST therefore shows their
CRCs with "?" and does not count them. T21 took 3 frames on both models
(1171 and 963 polls); vaeg gives about 625 polls on the VA and 11 on the
VA2.

## SGPDUMP (diagnostic build)

`nasm -DDUMP -DEXPECTED_NONE` builds `SGPDUMP.COM` (menu P of the
self-booting SGP disk). It runs only T07 and T09 and shows, on one screen,
their CRCs, the offsets from `DST` where the two results differ, 11 rows of
13 bytes from the first difference for both tests side by side, and port
153h as read before SGP use, the value written to enable it, and the value
read back after the restore. It was added after the hardware run on a
PC-88VA (M109a) gave other CRCs than vaeg for T07 and T09 (FBW FFF2h,
FFF0h) and no FBW value from -64 to 25 reproduces them in vaeg; a photo of
its screen shows what the hardware wrote. The 153h line checks whether
the read-modify-write of port 153h changes the memory map (the text
cursor was corrupted after SGPTEST on the PC-88VA, also right after boot).
