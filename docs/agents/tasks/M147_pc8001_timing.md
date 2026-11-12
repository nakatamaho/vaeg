<!--
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
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

# M147 — PC-8001 family clock, sound and 40-column display

Status: **G147 human gate passed on 2026-11-12.**

Branch `milestone/m147-pc8001-timing`; commit prefix `M147:`.

## Request

The maintainer (2026-11-12), PC-8001-family update:

1. PC-8001/mkII/SR: 4 MHz when the clock is the default; lower an 8 MHz
   selection.
2. N80SR graphics too slow; the demo breaks part way
   (N80SR Disk BASIC, `load"demo1.nsr",r`).
3. Crackling sound.
4. PC-8001: the cursor stays half width after `WIDTH 40,25` (not on the
   mkII/mkIISR).
5. PC-8801 family: `WIDTH 40,25` shows half-width ANK and cursor (as in
   80x25).

Overall impression: the timing is off.

## Findings

- `[CODE]` M144a ([b25c8e61](https://github.com/nakatamaho/vaeg/commit/b25c8e61413a603285a88a6fbe0a57e233a9ccd4))
  reached the PC-8001 family's 4 MHz by lowering the machine clock in the
  machine hook (`pccore.multiple` 1, `realclock` 3.9936 MHz). `pccore_reset()`
  sets the sound and beep clocks (`realclock / 25`) before that hook, and the
  VA memory waits (GVRAM read 7, write 4 plus a 5-clock interval) are counted
  in VA machine clocks. The CPU itself already ran at 4 MHz.
- `[MEAS]` Before the change a native recording (`--headless-audio --record`)
  stops with `audio_discard_underflow` on the PC-8001, mkII and mkIISR: the
  mixer falls behind guest time (item 3). After it the mkIISR demo records
  6000 frames with 106.339 s of audio, and the PC-8001 and mkII record too.
- `[MEAS]` The mkIISR demo (private N80SR system disk) is further along at
  the same guest frame after the change (frame 18000: the jungle scene, before
  it: "Now loading"); both loop through the demo without a visible break in
  headless captures. The reported break was not reproduced headlessly.
- `[MEAS]` Port 30h on the PC-8801MA2: the boot writes 22h then 23h,
  N88-BASIC `WIDTH 40` writes 22h, and the CRTC stays at 80 columns with
  text on every other byte. M144a ([8da32dcd](https://github.com/nakatamaho/vaeg/commit/8da32dcd92bc82dd2e9f1bf21d1a969d418e3167))
  ignored port 30h on the PC-8801 (item 5).
- `[MEAS]` The original PC-8001 (model 0) stays on the VA firmware route
  (M144a); its CURDEF cursor is 8 dots in 40 columns. The VA2's own V2 mode
  shows the same 8-dot cursor in `WIDTH 40`; that VA behaviour is left as is.
  Moving model 0 to the independent route was tried and reverted: its rows
  render at half height (the 10-raster 200-line compose and the 8x8 font are
  not wired for model 0 there).

## Changes

- [a55cb063](https://github.com/nakatamaho/vaeg/commit/a55cb0638db628410ff22aa5dfef44e3bdc925ae):
  the machine timeline stays at the VA's realclock; `pccore_clockrestore()`
  maps the default x2 (and the 8 MHz preset, the same x2) to x1 for the
  PC-8001 family, and `pccore_reset()` re-applies it after the machine hook
  (a VA after a PC-8001 session gets its x2 back). The family has no H/S
  input: a leftover S selection does not halve it. Explicit 10/12/20 MHz stay.
  Items 1-3.
- [b84bb439](https://github.com/nakatamaho/vaeg/commit/b84bb439db3111b34a0963c0aa923f6c5c1d38dd):
  CPU clock menu: the default entry is checked from the configured value
  (4 MHz on the PC-8001 family), no 8 MHz entry there, MHz presets ignore H/S.
- [2fb6bbf9](https://github.com/nakatamaho/vaeg/commit/2fb6bbf926b166a2ac5dbc6c40cdf2003868ac37):
  the PC-8801 follows port 30h bit 0 for 40/80 columns like the mkII/SR.
  Item 5.
- [8fdf8159](https://github.com/nakatamaho/vaeg/commit/8fdf815945113c877dbbe80186dd9e56c06d7659):
  the original PC-8001's cursor sprite is drawn at double width in 40
  columns. Item 4.

## Local verification

- Headless (dummy video): PC-8801MA2 boot at 80 columns, `WIDTH 40,25`
  doubled glyphs with a 16-dot cursor, `WIDTH 80,25` back; PC-8001 model 0
  40-column cursor 16 dots at column 0 and at x=32 after `AB`, 8 dots after
  `WIDTH 80,25`; mkII 40-column cursor still 16 dots; VA2 V2 unchanged.
- Recordings as above (before: `audio_discard_underflow`; after: complete).
- Selftest: "PC-8001 machine" checks the VA machine clock and FDD CPU
  multiple, CPU x1 with a leftover S, an explicit x4 kept and the VA's x2
  after a PC-8001 session; removing the x2-to-x1 rule fails it with "the
  PC-8001 family runs its CPU at 4 MHz". "PC-8801 WIDTH 40" checks 22h/23h
  over 80 CRTC columns; restoring the M144a PC-8801 exclusion fails it.
  `vaeg_upd9002_brkem_upd70008`: S no longer doubles the PC-8001 role.
  Native CTest 183/183.
- Not verified: the real-time GUI sound and the reported demo break, which
  need the maintainer's check.

## Gate G147 (human)

Standard V3 gate, plus: PC-8001, mkII and mkIISR start at 4 MHz (status bar
and CPU clock menu), sound without crackle, the N80SR demo
(`load"demo1.nsr",r`) runs through; `WIDTH 40,25` on the PC-8001 (full-cell
cursor) and on a PC-8801 model (full-width ANK and cursor), `WIDTH 80,25`
back; a VA boot afterwards runs at 8 MHz.

Gate build: `tools/release/build-mingw-static-container.sh --commit
cc168b53` (full SHA in the build report) with `VAEG_BUILD_TRACK=m147`
exited 0. It displays `Rel.20261112 (m147, cc168b53)`, keeps the local-only
encoders OFF and imports only Windows system DLLs. `vaeg.exe` SHA-256
`e36542ed6efed86d4f97cf029078a7a7912708c37b20159d0dc012286680fd1d`.

## G147 human gate

The maintainer stated on 2026-11-12 that G147 passed, on the static MinGW
handoff of `cc168b53` (`vaeg.exe` SHA-256
`e36542ed6efed86d4f97cf029078a7a7912708c37b20159d0dc012286680fd1d`), and
asked for the main merge. The integration-campaign choice is asked before the
merge as AGENTS.md requires.
