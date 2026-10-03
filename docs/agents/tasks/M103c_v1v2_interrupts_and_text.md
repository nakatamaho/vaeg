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


# M103c - V1/V2 interrupts, keyboard and 88-mode text display

Status: **implementation complete for the gate scope; G103c pending**

Series: V1/V2 mode (ADR-0016). Branch `topic/m103c-v1v2-text` off the
integration branch `topic/v1v2-mode` at `main`
`c1c4cd5c33b13e0097de504aaaa4ed6609130f2a` (M103a and M103b merged).
Commit prefix `M103c:`. Plan and hardware model:
[`docs/modernization/v1v2-mode-plan.md`](../../modernization/v1v2-mode-plan.md).

## Goal

Disk BASIC, booted automatically from V2 media as in M103b, shows its
first prompt on screen and echoes typed keys. This needs interrupts
delivered into compatible code, the keyboard reaching BASIC, and the
88-mode text RAM rendered through the TSP's µPD3301-compatible mode.

## Background (established before implementation)

`[VA-TM]` BNN manual §5.2.2. At reset the interrupt system is in
**8214 mode**; writing port 158H switches to 8259 mode (V3), and only a
reset returns to 8214 mode. In 8214 mode the µPD9002 ICU has a
µPD8214-compatible controller as its slave, with eight levels:

| Level | Source | V30 vector | µPD780 vector |
|---|---|---|---|
| INT0 | RS-232C receive | 40h | 00h |
| INT1 | VRTC | 41h | 02h |
| INT2 | general timer 2 (8801-compatible, 600 Hz) | 42h | 04h |
| INT3 | UINT0 (bus) | 43h | 06h |
| INT4 | sound controller | 44h | 08h |
| INT5 | UINT1 (bus) | 45h | 0Ah |
| INT6 | general timer 3 (mouse timer) | 46h | 0Ch |
| INT7 | SGP | 47h | 0Eh |

The controller selects the vector type from the CPU mode at the time of
the interrupt: compatible code takes a µPD780 interrupt (the Z80 `IM 2`
low byte), native code a V30 interrupt through the IVT. This replaces
the earlier `[DERIVED]` guess in the plan (§4) that firmware rewrites a
native frame. Ports: `E4h` OUT current status (bits 2–0 level, bit 3
XSGS: 0 = compare with the current status, 1 = priority only),
initialised with `OUT E4H,7`; `E6h` OUT mask (bit 0 general timer 2,
bit 1 VRTC, bit 2 RS-232C RXRDY; 1 = enable). The ICU initialisation
for 8214 mode masks all ICU inputs except the slave on IR7.

General timer 2 has no 8259-mode interrupt line, and vaeg does not model
it yet. vaeg's interrupt controller models only 8259 mode, and M103b's
key-wait trace (200 000 compatible instructions after 30 million)
contains no interrupt at all.

## Scope

1. **Interrupt mode latch.** 8214 mode at reset; port 158H selects 8259
   mode until reset. Saved state. Confirm by trace that V3 software
   (PC-Engine) writes 158H; if it does not, stop and re-plan, because
   V3 interrupts must not change.
2. **8214-compatible controller.** `E4h`/`E6h`, request latches, the
   acceptance rule and re-arming after acknowledge, with the rule's
   provenance recorded (VA manual for registers; Intel 8214 data sheet /
   PC-8801 behaviour, X88000 as behavioural reference, for acceptance
   and re-arm). In 8214 mode the mapped sources (VRTC, RS-232C receive,
   sound, SGP, mouse timer, bus lines) raise 8214 levels instead of
   8259 lines.
3. **General timer 2.** A 600 Hz event raising INT2 in 8214 mode.
4. **Delivery.** In compatible mode with IFF1 set: Z80 interrupt with
   the µPD780 vector through the adapter's acknowledge path (a dedicated
   virtual acknowledge port; today it is port 0, which is keyboard row
   0). In native mode with IF set: V30 vector 40h + level through the
   IVT. Tests for both, for acceptance/re-arm, masking and save/load.
5. **Keyboard.** Verify that BASIC's interrupt-driven scan of ports
   00h–0Eh reads the existing matrix and fills its key buffer; fix only
   what the trace shows is missing.
6. **88-mode text.** TVRAM at 88-mode `F000h`–`FFFFh` (physical
   `1F000h`, backed by V3 TVRAM `A6000h`) and the TSP 3301-compatible
   rendering of the text the trapped CRTC/DMAC programming describes
   (plan §2.1), enough for the BASIC screen: 80×25/20 text, 8801
   attributes, colours 8–15. ROM handler behaviour for `50h`–`53h` and
   `60h`–`68h` drives what the TSP must be told.

Out of scope: 88-mode graphics display and ALU (M103d), FDD use under
V1/V2 beyond boot, sound, V1 mode, timing accuracy.

## Gate G103c (human)

Standard V3 gate unchanged, plus with the VA2 ROM set and V2 BASIC
media: the disk BASIC banner and first prompt appear on screen, typed
characters echo, and a short BASIC line (`PRINT 1+1`) answers `2`.

## Summary for G103c

Machine-verifiable parts (local): full CTest without failures (108
entries; the external SST and, without private paths configured,
`vaeg_m103b_basic_boot` skipped); V3 smoke passes; with the VA2 ROMs and V2
BASIC media `tools/qa/m103b_basic_boot.py run` shows ` 2` after
`PRINT 1+1` on the 88-mode text screen. Human-gate items: the standard V3
gate, the BASIC screen with typed characters echoing in an interactive
session, and acceptance of the documented policies (8214 acceptance and
re-arm rule, general-timer-2 start on enable, inferred 8Eh/97h semantics,
3301 attribute carry across rows).

Deferred: 3301 semigraphics, 40-column mode, kanji ROM ports, 88-mode
graphics display (M103d), sound under V1/V2, FDD use beyond boot.

## Implementation progress

### 8214-mode interrupts (scope items 1–4)

- Traces before implementation: a V3 boot to PC-Engine writes port 158H
  (the VA2 ROM's shared initialiser `F000:2210` reprograms the ICU for
  8259 mode, ICW4 1Dh/09h); the V1/V2 BASIC boot never writes 158H and
  uses E4h/E6h. At reset the ROM programs the ICU exactly as the manual's
  8214-mode table (`F000:1317`: ICW1 11h, ICW2 00h, ICW3 80h, ICW4 03h =
  auto-EOI, OCW1 7Fh).
- `io/pic.c` now has an 8214 controller as the ICU's IR7 slave in 8214
  mode: E4h current status and XSGS, E6h masks applied at the request
  input, request latches, acceptance only above the current status (or
  any level with XSGS), and no further acceptance until E4h is written
  again. The acceptance and re-arm rule follows the Intel 8214 behaviour
  that 8801 software relies on; the VA manual documents only the
  registers. Port 158H selects 8259 mode until reset.
- Routing in 8214 mode: VRTC (IR2) → INT1, sound (IR12) → INT4, mouse
  timer (IR13) → INT6, SGP (IR8) → INT7, bus UINT0/UINT1 → INT3/INT5,
  RS-232C receive → INT0 (an explicit call; vaeg's IR4 also carries
  transmit). Master IR lines keep their old behaviour (masked by the
  ROM's OCW1); the uPD8259 slave is absent in 8214 mode.
- Delivery: compatible mode takes a Z80 interrupt through a virtual
  acknowledge port (0x10000) that returns the uPD780 vector `2*level`;
  native mode takes V30 vector 40h+level. The 8259 model now honours ICW4
  auto-EOI; only the ROM's 8214-mode setting uses it.
- General timer 2: a 600 Hz event (`NEVENT_GENTIMER2`, saved as `gtm2`)
  that runs only while E6h bit 0 enables it, so an idle timer cannot
  perturb V3 scheduling; the phase of the first tick after enabling is a
  modelling choice.
- State: new section `PIC8214`, version 0. A file without it predates
  8214 mode, and those builds always behaved as 8259 mode, so loading
  keeps 8259 mode in that case.
- Tests: `vaeg_romless_tests` gains an 8214 case (reset state, masks,
  status comparison and XSGS, priority, native vector 41h/44h with AEOI,
  re-arm, Z80 IM 2 taking vector 02h, timer 2, save/load, an older file
  loading as 8259 mode, and port 158H). Removing compatible delivery or
  auto-EOI makes it fail. The 8087 interrupt fixtures and the statsave
  test were written for an always-8259 model; they now select 8259 mode
  as V3 software does, through the same function as the port handler,
  so the M42 reset-state fixture is unchanged. Compatible code reads
  data and operands through DS, so the new fixture sets DS = CS as the
  ROM does before BRKEM2.
- Results: full CTest without failures (108 entries, two skipped); V3
  smoke passes; a V3 PC-Engine boot accepts `dir` and its clock advances.
  With V2 BASIC media, headless input `3`, Enter, `PRINT 1+1`, Enter
  produced in the 88-mode text area (local memory dump, not committed):
  the version line `NEC N-88 BASIC Version 2.4`, `Ok`, `PRINT 1+1`, ` 2`,
  `Ok` — BASIC runs interactively; only the display is missing. The VRTC
  handler scans keyboard rows 0Bh–00h with `IN A,(C)`. The M103b trace
  classifier in `vaeg_m103b_basic_boot` expects the interrupt-free
  key-wait loop and no longer matches; it is replaced by a content check
  once the text is visible (next stage).

### 88-mode text display (scope item 6)

- `[ROM]` TSP commands sent by the VA2 ROM for V1/V2 (local trace, not
  committed): SYNC, DSPDEF `00 00 21 0F 00 18` (16-raster rows; 13h for
  20 rows), ACTSCR, CURDEF, SPRON, `EMUL 8Ch 00 4E 13 18` (split screen 0,
  80 characters, 20 attribute pairs, 25 rows; the 20-row variant ends in
  13h), DSPON with the table at TVRAM 0000h. Each time BASIC reprograms the
  trapped CRTC/DMAC the ROM sends EXIT, then 8Eh/8Fh/97h sequences, then
  EMUL again. 8Eh/8Fh/97h are not in the VA manual's command list; they
  write split-screen table fields while TVRAM is unreachable from the CPU.
  `[DERIVED]` 8Eh sets a TVRAM byte address (the observed values 00h, 08h,
  0Ah and 10h are the offsets of the frame's address, pitch, mode and start
  fields), 97h writes the following parameter bytes from it until the next
  command, and 8Fh takes three bytes whose meaning is unknown (always
  `01 00 00`). BASIC's DMA start F3C8h becomes a start field of 678Ch.
- `[VA-TM]` §8.2.1 and the TSP chapter: 88-mode F000h–FFFFh is the 4 KiB of
  V3 TVRAM at A6000h; the TSP runs in byte-access mode and EMUL expands
  uPD3301 attributes dynamically. vaeg now maps that window when no GVRAM
  plane is selected and port 32h TMODE (bit 4) is clear (BASIC writes
  A8h/A9h), records EMUL's geometry, stops emulation on EXIT and SYNC, and
  implements 8Eh/97h as above (state in former reserved TSP bytes; the
  state layout is unchanged).
- `[DERIVED]` Byte-mode addressing: TSP local byte 3000h is TVRAM byte
  6000h, and the table's start and pitch fields are twice the byte values
  (pitch F0h = 120-byte rows). The ROM programs the start two characters
  early and rxp = 1008 hides them, matching the TSP's rw/8 + 2 fetch. The
  renderer reads the 3301 row (80 characters, then 20 column/attribute
  pairs), applies pairs in column order with the state carried across
  rows, uses PC-8801 attribute bits (colour: bits 7–5 G/R/B → colour code
  8–15; decoration: secret, blink, reverse, upper line, under line), and
  draws the ANK font. Not yet modelled: semigraphics, 40-column doubling,
  CURS-positioned cursor (the cursor sprite the ROM manages is drawn by
  the existing sprite path).
- Tests: `vaeg_romless_tests` gains a TSP 3301 case (EMUL decoding, 8Eh/97h
  writes ending at EXIT, SYNC stopping emulation; a synthetic frame shows
  the hidden two-character lead, red reverse at column 1 and its carry to
  the next row; the 88-mode window with TMODE, GVRAM precedence and V3).
  Removing the window, the carry or the byte-mode addressing makes it
  fail. The M103b RAM-window/GVRAM fixture now selects TMODE 1 because it
  uses F000h–FFFFh as main RAM.
- Acceptance: `tools/qa/m103b_basic_boot.py` now types `3` and
  `PRINT 1+1` and checks the TVRAM dump written at exit: a row starting
  `PRINT 1+1`, the next row ` 2`, and an `Ok` row in the 88-mode text area
  (stable codes `M103B_NO_TVRAM_DUMP`, `M103B_NO_BASIC_PROMPT`,
  `M103B_NO_ANSWER`, `M103B_WORKER_FAILED`). It passes in about 73 s with
  the VA2 ROMs and V2 BASIC media and fails with `M103B_NO_BASIC_PROMPT` for
  media that boots natively. A local screenshot shows the banner, `Ok`,
  `PRINT 1+1`, ` 2`, the cursor and the reverse function-key row.
- Full CTest without failures (108 entries, two skipped); V3 smoke passes;
  the V3 PC-Engine screen with kanji is unchanged.
