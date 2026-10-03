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

Status: **in progress; G103c pending**

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
