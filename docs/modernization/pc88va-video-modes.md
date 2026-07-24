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
POSSIBILITY OF SUCH DAMAGE.
-->
# PC-88VA Video-Mode and Framebuffer Control Reconstruction

> **Status:** Reconstructed, non-authoritative specification
>
> **Version:** 0.2
>
> **Date:** 2026-07-16 (JST)
>
> **Target:** NEC PC-88VA family
>
> **Repository path:** `docs/modernization/pc88va-video-modes.md`

> [!WARNING]
> This document separates timing, graphics fetch, framebuffer layout, SGP
> drawing, and final composition. It does not establish undocumented monitor
> timings as safe PC-88VA modes. Use the documented `SYNC` vectors first;
> arbitrary synchronization values can send an out-of-range signal to a CRT.

## 1. Purpose and evidence

Changing a PC-88VA display mode is not one register operation. At least five
independent functions must agree:

1. TSP CRT synchronization and active intervals;
2. graphics vertical interpretation and line count;
3. graphics horizontal fetch width and packed-pixel format;
4. GVRAM framebuffer layout and displayed sub-screen; and
5. the SGP destination descriptor used to draw into that layout.

The final D65101-side mixer then composes TSP text/sprites and graphics screens
G0/G1 in a common output coordinate system. The reconstructed TSP and SGP
specifications remain authoritative for their individual blocks:

- [uPD72022 TSP reconstruction](upd72022-tsp.md)
- [PC-88VA SGP reconstruction](upd65200-sgp.md)

Evidence labels used below are:

| Label | Meaning |
|---|---|
| `[DOCUMENTED]` | Stated by PC-88VA or uPD72022 technical material |
| `[IMPLEMENTATION]` | Observable in vaeg or another emulator; not automatically hardware behavior |
| `[INFERENCE]` | Derived from documented fields and necessary address arithmetic |
| `[CONFLICT]` | Surviving sources or source and implementation disagree |
| `[UNKNOWN]` | Real hardware or better primary material is still required |

## 2. Control ownership

| Function | Control location | Responsibility |
|---|---|---|
| CRT scan and synchronization | TSP `SYNC`, command/status port `0142h`, parameter port `0146h` | Dot-clock selection, horizontal/vertical active periods, borders, blanking, HSYNC, and VSYNC |
| Graphics line and scan interpretation | `GRMODE`, ports `0100h-0101h` | 200/204/400/408 lines and graphics raster-scan interpretation |
| G0/G1 horizontal fetch and pixel format | `GRRES`, ports `0102h-0103h` | Per-screen 640/320-dot fetch and 1/4/8/16-bpp interpretation |
| GVRAM image geometry and source scrolling | Framebuffer descriptors, ports `0200h-027fh` | Frame start, pitch, vertical extent, source offsets, and display start |
| Vertical sub-screen placement | Framebuffer `DSH` and `DSP` | Visible height and destination Y position |
| Drawing geometry | SGP `SET_SOURCE`/`SET_DESTINATION` descriptors | Drawing address, rectangle size, pixel format, and pitch |
| Final source composition | ports `0106h-0111h`, `0124h-0137h` | Priority, direct/palette color, masks, transparency, backdrop, and text/sprite boundary |

The SGP does not own monitor timing. Conversely, changing TSP `SYNC` does not
automatically change GVRAM pitch or the SGP drawing descriptor.

## 3. TSP `SYNC`: physical CRT scan

Software writes command `10h` to port `0142h`, then writes 14 parameter bytes
to port `0146h`. The timing fields are:

| Byte | Bits | Field | Function |
|---:|---:|---|---|
| 0 | 7:6 | `RM` | Raster mode |
| 1 | 5 | `EC` | Select internally divided dot-clock output when set |
| 1 | 4 | `ES` | Select synchronization output when set |
| 1 | 2:0 | `RS` | Internal dot-time divider selector |
| 2 | 5:0 | `LBL` | Left blanking |
| 3 | 5:0 | `LBR` | Left border |
| 4 | 7:0 | `HAD` | Horizontal active interval |
| 5 | 5:0 | `RBR` | Right border |
| 6 | 5:0 | `RBL` | Right blanking |
| 7 | 5:0 | `HS` | Horizontal synchronization interval |
| 8 | 5:0 | `TBL` | Top blanking |
| 9 | 5:0 | `TBR` | Top border |
| 10 | 7:0 | `VAD[7:0]` | Vertical active interval, low byte |
| 11 | 6 | `VAD[8]` | Vertical active interval, high bit |
| 11 | 5:0 | `BBR` | Bottom border |
| 12 | 5:0 | `BBL` | Bottom blanking |
| 13 | 5:0 | `VS` | Vertical synchronization interval |

`RM` has the generic uPD72022 meanings:

| `RM` | Generic raster mode |
|---:|---|
| `00b` | Noninterlace; canonical 640x400/24kHz example |
| `01b` | Interlace; canonical 640x400/15kHz example |
| `10b` | Vertical magnification; canonical 640x200/24kHz example |
| `11b` | Normal; canonical 640x200/15kHz example |

### 3.1 Generic internal-clock examples

`[DOCUMENTED]` For a 20MHz generic-device source clock, the uPD72022 data
book gives:

| `RS` | Divider | Example | `HAD` | `VAD` |
|---:|---:|---:|---:|---:|
| `000b` | /4 | 256x192 | 63 | 192 |
| `001b` | /3 | 320x200 | 79 | 200 |
| `010b` | /2 | 512x192 | 127 | 192 |
| `011b` | /1.5 | 640x200 | 159 | 200 |
| `100b` | /1 | 640x400 | 159 | 400 |

The horizontal active width follows:

~~~text
active dots = (HAD + 1) * 4
~~~

Thus `HAD=63`, for example, represents 256 active dots.

These are generic uPD72022 examples, not a list of complete PC-88VA machine
modes. `[DOCUMENTED]` Known PC-88VA vectors use an external dot clock and
carry `RS=111b`; their raw timing fields, graphics fetch, sprites, and mixer
must be treated as one platform configuration.

### 3.2 Documented PC-88VA timing profiles

The PC-88VA material records the following complete `SYNC` vectors:

| Platform mode | Horizontal profile | Raw TSP raster use | 14 parameter bytes |
|---:|---:|---|---|
| 200 lines | 15.98kHz | `RM=11b`, normal raster | `C1 57 1C 00 9F 00 10 0F 25 00 C8 00 0F 08` |
| 200 lines | 15.73kHz | `RM=11b`, normal raster | `C1 47 1C 00 9F 00 12 11 24 00 C8 00 17 04` |
| 200 lines | 24.8kHz | `RM=10b`, vertically magnified | `81 57 10 00 9F 00 10 0F 19 00 90 40 07 08` |
| 204 lines | 15.98kHz | `RM=11b`, normal raster | `C1 57 1C 00 9F 00 10 0F 23 00 CC 00 0D 08` |
| 204 lines | 15.73kHz | `RM=11b`, normal raster | `C1 47 1C 00 9F 00 12 11 22 00 CC 00 15 04` |
| 204 lines | 24.8kHz | `RM=10b`, vertically magnified | `81 57 10 00 9F 00 10 0F 19 00 98 40 07 08` |
| 400 lines | 15.73kHz | `RM=01b`, interlace, 200-line sprite data | `41 47 1C 00 9F 00 12 11 24 00 C8 00 17 04` |
| 400 lines | 24.8kHz | `RM=00b`, noninterlace, 200-line sprite data | `01 57 10 00 9F 00 10 0F 19 00 90 40 07 08` |
| 400 lines | 24.8kHz | `RM=11b`, normal raster, 400-line sprite data | `C1 57 10 00 9F 00 10 0F 19 00 90 40 07 08` |
| 408 lines | 15.73kHz | `RM=01b`, interlace, 200-line sprite data | `41 47 1C 00 9F 00 12 11 22 00 CC 00 15 04` |
| 408 lines | 24.8kHz | `RM=00b`, noninterlace, 200-line sprite data | `01 57 10 00 9F 00 10 0F 19 00 98 40 07 08` |
| 408 lines | 24.8kHz | `RM=11b`, normal raster, 400-line sprite data | `C1 57 10 00 9F 00 10 0F 19 00 98 40 07 08` |

The descriptive platform mode must not replace raw-byte decoding. In
particular, the 200/204-line 15.73kHz vectors begin with `C1h` and therefore
select `RM=11b`, not the `RM=01b` interlace mode used by the 400/408-line
15.73kHz vectors.

### 3.2.1 Invariant fields and the first two bytes

Every vector has `HAD=9Fh`, so every documented PC-88VA profile has a 640-dot
TSP active interval:

~~~text
(9Fh + 1) * 4 = 640 dots
~~~

Every first byte also has `ILM=1`, selecting the interleave memory interface.
The four observed first-byte values differ primarily in `RM`:

| Byte 0 | `RM` | `ILM` | Raw TSP interpretation |
|---:|---:|---:|---|
| `C1h` | `11b` | 1 | Normal raster progression |
| `81h` | `10b` | 1 | Vertical magnification |
| `41h` | `01b` | 1 | Interlace |
| `01h` | `00b` | 1 | Noninterlace |

The two observed second-byte values are:

| Byte 1 | `RF` | `EC` | `ES` | `RS` | Interpretation |
|---:|---:|---:|---:|---:|---|
| `57h` | 1 | 0 | 1 | `111b` | External dot clock; TSP outputs HSYN/VSYN |
| `47h` | 1 | 0 | 0 | `111b` | External dot clock; TSP accepts external HSYN/VSYN |

Thus none of these vectors uses the generic internal-clock `RS=000b` through
`100b` modes. The 15.73kHz family alone uses `ES=0`; identifying the exact
board-level synchronization master in this configuration still requires a
schematic or hardware measurement.

### 3.2.2 Three horizontal timing families

`[IMPLEMENTATION]` Current vaeg calculates PC-88VA line width without adding
one to `LBR` or `RBR`:

~~~text
H_total_dots =
    ((LBL + 1) + LBR + (HAD + 1) +
     RBR + (RBL + 1) + (HS + 1)) * 4
~~~

With the current vaeg external-dot-clock constants, the twelve vectors reduce
to three horizontal families:

| Profile | `LBL LBR HAD RBR RBL HS` | Total dots | vaeg dot clock | Derived horizontal rate |
|---:|---|---:|---:|---:|
| 15.98kHz | `1C 00 9F 00 10 0F` | 888 | 14.189837MHz | 15.9795kHz |
| 15.73kHz | `1C 00 9F 00 12 11` | 904 | 14.252364MHz | 15.7659kHz |
| 24.8kHz | `10 00 9F 00 10 0F` | 840 | 20.854022MHz | 24.8262kHz |

The generic uPD72022 formula adds one to both border fields and would instead
give 896, 912, and 848 dots. With the vaeg clock constants, those totals do not
match the named horizontal profiles as closely. This strengthens, but does not
prove, the vaeg real-machine-fit hypothesis that PC-88VA `LBR/RBR` use differs
from the generic data-book explanation. The clock constants themselves include
implementation fitting and are not all independently established crystal
frequencies.

Current vaeg chooses the clock family from the monitor selection and
`GRMODE` bit 7, not by matching a `SYNC` byte sequence. Therefore an absolute
frequency can be derived only after the TSP vector and board-level graphics
mode are paired.

### 3.2.3 Vertical totals and logical-line interpretation

`[IMPLEMENTATION]` Current vaeg uses:

~~~text
V_total_lines = TBL + TBR + VAD + BBR + BBL + VS
field_rate     = horizontal_rate / V_total_lines
~~~

This gives:

| Platform mode | Raw `VAD` | Total lines | Raw `RM` | Derived field/frame cadence |
|---:|---:|---:|---:|---:|
| 200/15.98 | 200 | 260 | `11b` | 61.46Hz |
| 204/15.98 | 204 | 260 | `11b` | 61.46Hz |
| 200/15.73 | 200 | 263 | `11b` | 59.95Hz |
| 204/15.73 | 204 | 263 | `11b` | 59.95Hz |
| 200/24.8 | 400 | 440 | `10b` | 56.42Hz |
| 204/24.8 | 408 | 448 | `10b` | 55.42Hz |
| 400/15.73 | 200 per field | 263 per field | `01b` | 59.95 fields/s, about 29.97 two-field frames/s |
| 408/15.73 | 204 per field | 263 per field | `01b` | 59.95 fields/s, about 29.97 two-field frames/s |
| 400/24.8 | 400 | 440 | `00b` or `11b` | 56.42Hz |
| 408/24.8 | 408 | 448 | `00b` or `11b` | 55.42Hz |

Several relationships are now visible:

- The 15kHz 200-to-204 change adds four active lines but removes four lines
  from top/bottom blanking, preserving totals of 260 or 263 lines.
- The 24.8kHz 200/204 modes use raw `VAD=400/408` with `RM=10b`, so the TSP
  scans 400/408 physical rasters while vertically magnifying 200/204-line data.
- The 200-line and 400-line 15.73kHz vectors have the same horizontal and
  vertical interval bytes. Changing byte 0 from `C1h` to `41h` changes raster
  progression to interlace rather than changing the CRT interval counts.
- The three 24.8kHz 200/400 variants likewise share physical interval values.
  Byte 0 selects vertical magnification, noninterlace with 200-line sprite
  data, or normal raster progression with 400-line sprite data.

The rates above are derived from the current vaeg timing model. They are not
yet direct oscilloscope measurements of every PC-88VA model.

### 3.2.4 Current confidence boundary

The vectors now determine, or strongly constrain:

- every timing-field value and the 640-dot active width;
- the three horizontal timing families;
- raw active and total line counts in the current model;
- normal, magnified, interlace, and noninterlace raster selection;
- the distinction between logical 200/204-line data and physical 400/408-line
  scanning; and
- why 200/400 or 204/408 profiles can share almost all timing bytes.

The remaining hardware questions are:

- the precise TSP/D65101 synchronization-master relationship when `ES=0`;
- exact external dot-clock frequency and model variation;
- hardware confirmation of the no-`+1` `LBR/RBR` rule;
- even/odd-field phase, half-line behavior, and sync polarity in interlace;
- exact text/sprite address progression for the two 24.8kHz 400-line
  interpretations; and
- the required `GRMODE` and monitor-switch pairing for every vector.

## 4. Port `0100h`: `GRMODE`

`GRMODE` is a little-endian 16-bit register at ports `0100h-0101h`. The
recoverable graphics line selections are:

| Bits | Value | Meaning |
|---:|---:|---|
| 1:0 | `00b` | 400 lines |
| 1:0 | `01b` | 408 lines |
| 1:0 | `10b` | 200 lines |
| 1:0 | `11b` | 204 lines |

The frozen VA diagnostic also decodes bits 7:6 as:

| Bits 7:6 | Graphics raster interpretation |
|---:|---|
| `00b` | Noninterlace 0 |
| `01b` | Noninterlace 1 |
| `10b` | Interlace 0 |
| `11b` | Interlace 1 |

Other relevant mode bits include bit 10 for single-plane graphics and bit 11
for one/two graphics screens. The complete mode word must be preserved when
only the line field changes. For example:

~~~c
/* Select the 200-line field without disturbing other GRMODE bits. */
grmode = (grmode & ~0x0003u) | 0x0002u;
~~~

`GRMODE.VW` describes how the graphics path treats its lines. Physical CRT
timing still comes from the matching TSP `SYNC` vector. Writing only port
`0100h` does not safely change 15/24kHz synchronization.

## 5. Port `0102h`: `GRRES`

`GRRES` is a little-endian 16-bit register at ports `0102h-0103h`:

| Bits | Name | Screen | Meaning |
|---:|---|---:|---|
| 1:0 | `PM0` | G0 | `00=1`, `01=4`, `10=8`, `11=16` bpp |
| 4 | `HW0` | G0 | `0=640`, `1=320` logical dots |
| 9:8 | `PM1` | G1 | `00=1`, `01=4`, `10=8`, `11=16` bpp |
| 12 | `HW1` | G1 | `0=640`, `1=320` logical dots |

`[IMPLEMENTATION]` vaeg expands each 320-dot graphics pixel to two pixels in
the common 640-dot output buffer. TSP text and sprite X coordinates therefore
remain on their 640-dot basis.

Examples that preserve unrelated bits are:

~~~c
/* G0: 320 dots, 4 bpp. */
grres = (grres & ~0x0013u) | 0x0011u;

/* G1: 320 dots, 4 bpp. */
grres = (grres & ~0x1300u) | 0x1100u;
~~~

The name "16 bpp" is retained here. The exact direct-color component layout
should be established separately before naming it RGB565 normatively.

## 6. Ports `0200h-027fh`: framebuffer descriptors

Four `20h`-byte register blocks describe GVRAM source layout and displayed
sub-screens:

| Descriptor | Port range | Current graphics use |
|---:|---:|---|
| FB0 | `0200h-021fh` | First G0 sub-screen |
| FB1 | `0220h-023fh` | G1 |
| FB2 | `0240h-025fh` | Second G0 sub-screen |
| FB3 | `0260h-027fh` | Third G0 sub-screen |

Each block has this layout:

| Offset | Field | Stored form | Reconstructed role |
|---:|---|---|---|
| `+00h..+02h` | `FSA` | 18-bit address, low two bits zero | Virtual framebuffer start |
| `+04h..+05h` | `FBW` | 11-bit value, low two bits zero | Address pitch between source lines |
| `+06h..+07h` | `FBL` | 10-bit value | Virtual framebuffer final line/vertical wrap extent |
| `+08h` | `DOT` | 5-bit value | Starting pixel lane within the source word group |
| `+0ah..+0bh` | `OFX` | aligned source X offset | Horizontal source offset/scroll |
| `+0ch..+0dh` | `OFY` | 10-bit source Y offset | Vertical source offset/scroll |
| `+0eh..+10h` | `DSA` | 18-bit address, low two bits zero | Displayed source start address |
| `+12h..+13h` | `DSH` | 9-bit value | Displayed sub-screen height |
| `+16h..+17h` | `DSP` | 9-bit value | Destination Y position on the CRT raster |
| other | - | Reserved | Write zero |

`[IMPLEMENTATION]` vaeg advances each single-plane source line by `FBW`. With
`OFY=0`, it performs vertical wrap after `FBL+1` lines. It activates a
descriptor when the current output Y equals `DSP`, and stops it at
`DSP+DSH`. These relations make the current interpretation:

~~~text
source pitch             = FBW address units
virtual height           = FBL + 1, when OFY is zero
visible destination rows = DSP through DSP + DSH - 1
~~~

FB1 is special in the current implementation: writes to its `FSA`, `FBL`,
`OFX`, and `OFY` are ignored and it is initialized without vertical wrap.

`[CONFLICT]` The vaeg structure comment calls `OFX` a 10-bit field, while the
write handler retains three high bits and therefore accepts an 11-bit aligned
value. This document does not resolve the real-hardware field width.

### 6.1 Width, start, position, and scrolling

The fields divide into three responsibilities:

~~~text
FSA / FBW / FBL
    virtual GVRAM image and vertical wrap

DOT / OFX / OFY / DSA
    source start and source scrolling within that image

DSH / DSP
    visible height and destination Y position
~~~

No equivalent of the TSP split descriptor's destination `RXP` has yet been
identified for graphics. `OFX` changes the GVRAM source position; it must not
be documented as a proven destination-X placement register.

For a tightly packed single-plane image:

~~~text
minimum byte pitch = logical_width * bits_per_pixel / 8
~~~

`FBW` is aligned to four bytes. Examples are:

| Width | 1 bpp | 4 bpp | 8 bpp | 16 bpp |
|---:|---:|---:|---:|---:|
| 320 | 40B | 160B | 320B | 640B |
| 384 | 48B | 192B | 384B | 768B |
| 640 | 80B | 320B | 640B | 1280B |

These are arithmetic minimum pitches, not proof that every width is a native
graphics-fetch mode. `GRRES` exposes only 320 and 640 logical dots.

## 7. SGP drawing pitch

The SGP destination descriptor carries its own address, block width, block
height, pixel mode, start dot, and `FBW`. When display geometry changes, the
SGP descriptor must use the same packed-pixel format and line pitch as the
GVRAM layout.

Conceptually:

~~~text
logical width
    -> packed bytes per line
    -> graphics framebuffer FBW
    -> vertical wrap and DSA stepping
    -> SGP SET_DESTINATION FBW and rectangle width
~~~

The SGP has no command that directly selects "320x200" or "640x400" monitor
resolution.

## 8. Reconstructed normal-mode combinations

The principal PC-88VA design combinations are 320x200, 640x200, 320x400, and
640x400, with 204/408 variants. Horizontal and vertical selection are in
different registers, but a combination is not called hardware-confirmed until
it has passed a real-machine test with a matching TSP timing profile.

| Logical graphics size | `GRMODE` bits 1:0 | `HW` | 4-bpp minimum `FBW` |
|---:|---:|---:|---:|
| 320x200 | `10b` | 1 | 160B |
| 640x200 | `10b` | 0 | 320B |
| 320x400 | `00b` | 1 | 160B |
| 640x400 | `00b` | 0 | 320B |

### 8.1 Candidate G0 320x200, 4-bpp setup

The following describes the field relationships, not a complete executable
initialization routine:

~~~text
TSP SYNC
    documented 24.8kHz / 200-line vertical-magnification vector

GRMODE bits 1:0
    10b, 200-line graphics interpretation

GRRES.HW0 / PM0
    HW0=1, PM0=01b, 320 dots and 4 bpp

FB0
    FBW=160 bytes
    FBL=199 candidate for a 200-line buffer in the vaeg interpretation
    DSH=200 for a full-height sub-screen
    DSP=0 for top placement

SGP destination
    4 bpp, 320-pixel rectangle, 200 lines, 160-byte pitch
~~~

The corresponding field updates are:

~~~c
grmode = (grmode & ~0x0003u) | 0x0002u;
grres  = (grres  & ~0x0013u) | 0x0011u;
~~~

`0102h` alone is insufficient. Leaving a 640-dot 4-bpp pitch of 320 bytes
while drawing and fetching a tightly packed 320-dot image at 160 bytes causes
different blocks to disagree about the next scan line.

## 9. Non-native widths

### 9.1 Generic 256x192 is not a confirmed PC-88VA mode

The generic uPD72022 example `RS=000b`, `HAD=63`, `VAD=192` generates a
256x192 active interval with its internal-clock assumptions. The PC-88VA's
documented graphics controls do not expose a 256-dot `HW` selection, and its
known TSP vectors use external dot-clock input.

Therefore:

> The uPD72022 alone can generate 256x192, but no complete native PC-88VA
> 256x192 mode composited with GVRAM has been established.

A TSP-only 256-dot width can disagree with G0/G1 fetch, the permanent 640-dot
sprite coordinate basis, and mixer boundaries.

### 9.2 Safe initial 384x256 viewport experiment

There is no documented native 384-dot `GRRES` selection. The safe initial
experiment is therefore a content rectangle inside a documented raster:

~~~text
physical raster       640 x 400
graphics source rows  at least 640 pixels wide
content rectangle     384 x 256
content origin        (128, 0) within each source row
destination Y         DSP = 72
visible height        DSH = 256
outside content       transparent or backdrop composition
~~~

For 4 bpp, retain a 640-dot source pitch of 320 bytes and place the first
content pixel 64 bytes from the start of each row. This avoids assuming that
the graphics fetcher stops after 384 pixels. A tightly packed 192-byte pitch
for a 384-pixel row is useful as a later discriminator test, but is not yet a
documented 384-dot display mode.

`DSP` supplies vertical destination placement. No proven graphics
destination-X register has been identified, so horizontal centering should
initially be represented in the stored 640-dot row and final transparency or
masking, not attributed to `OFX`.

## 10. Safe real-hardware test order

1. Establish the known 640x400/24.8kHz profile and display a one-dot grid.
2. Change to a documented 640x200 combination and determine whether the
   logical lines are vertically enlarged or surrounded by blank output.
3. Select `HW0=1` for 320x200 and verify physical two-dot horizontal pixels.
4. Change `FBW` independently and verify the exact next-line address.
5. Test 320x400 to determine whether horizontal and vertical selections are
   independent on hardware.
6. Test the padded 384x256 content rectangle while retaining 640x400 timing.
7. Only after those controls are isolated, test a tightly packed 384-pixel
   pitch and record fetch, wrap, and composition behavior.

Every capture should retain the raw `SYNC` bytes, `GRMODE`, `GRRES`, all active
framebuffer descriptors, SGP destination descriptor, GVRAM before/after, CRT
monitor selection, and measured horizontal/vertical frequency.

## 11. Implementation evidence and unresolved points

The active vaeg implementation provides the following evidence:

- [`iova/videova.c`](../../iova/videova.c) binds `GRMODE`, `GRRES`, and four
  framebuffer descriptor blocks and shows each writable bit mask;
- [`iova/videova.h`](../../iova/videova.h) names the descriptor fields;
- [`vramva/makegrphva.c`](../../vramva/makegrphva.c) implements 320-dot
  horizontal duplication, `FBW` line stepping, `FBL` wrap, `DSH` height,
  `DSP` placement, and FB0/FB2/FB3 versus FB1 selection; and
- the archived
  [`win9x/debuguty/viewvideova.cpp`](https://github.com/nakatamaho/vaeg/blob/b72e641733ddea6f0e8faef2507093f7c3aee5a4/win9x/debuguty/viewvideova.cpp)
  records the original diagnostic interpretation of `GRMODE` and `GRRES`.

The archived file is evidence only and must not be modified.

The principal unresolved points are:

1. exact hardware field width and units for `OFX`;
2. exact count-versus-last-index convention for every framebuffer dimension;
3. whether FB1's non-writable fields reflect hardware or only legacy design;
4. behavior when `FBW` is smaller than the selected 320/640 graphics fetch;
5. whether a graphics destination-X placement control exists elsewhere;
6. native status of the apparent 320x400 combination;
7. exact direct-color layout of `PM=11b`; and
8. model differences among VA, VA2, and VA3.

## 12. Change log

### Version 0.2 - 2026-07-16

- Decoded all twelve PC-88VA `SYNC` vectors into raw `RM`, clock/sync, active,
  blanking, border, and synchronization fields.
- Reduced the vectors to three horizontal timing families and derived their
  current vaeg horizontal and field rates.
- Explained the 200/204 versus 400/408 logical/physical raster relationships.
- Corrected the 200/204-line 15.73kHz description: its raw TSP mode is
  `RM=11b` normal raster, not `RM=01b` interlace.
- Separated deductions supported by raw bytes from vaeg timing-model evidence
  and remaining real-hardware questions.

### Version 0.1 - 2026-07-16

- Separated TSP scan timing, `GRMODE`, `GRRES`, framebuffer descriptors, SGP
  pitch, and final composition.
- Recorded the generic uPD72022 `RS=000b` through `100b` examples without
  promoting them to PC-88VA system modes.
- Reconstructed ports `0100h`, `0102h`, and `0200h-027fh` from documentation
  and vaeg behavior.
- Added a candidate 320x200/4-bpp field relationship and safe hardware-test
  order.
- Defined 256x192 as generic-TSP capability only and changed the first
  384x256 experiment to padded 640-dot rows rather than an assumed native
  384-dot fetch.
