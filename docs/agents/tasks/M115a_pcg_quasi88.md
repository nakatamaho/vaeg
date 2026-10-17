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

# M115a — PCG behaviour from QUASI88, clearer tones, credits

Status: **complete; G115a passed on 2026-10-16; merged to `main` at [e492658](https://github.com/nakatamaho/vaeg/commit/e492658c044d257a054c5667b938c3c24036b306)**

## Request

After G115 (maintainer, 2026-10-16): QUASI88 writes PCG RAM only on a
write to port 02h and models the 8253 more fully; follow that, make the
tones clearer (issue #41), and credit X88000 and QUASI88 under 情報 ->
About.

## Decisions

- [POLICY] QUASI88 0.7.4 (showtime) is a behavioural reference only. Its
  core is under modified-BSD-equivalent terms, but its sound output
  (`src/snddrv/xmame/`, including `pcg.c`) falls under the MAME/XMAME terms
  and its files carry no licence header, so no QUASI88 code is used.
- [DERIVED] (QUASI88 `pc88main.c`) PCG RAM is written only by a write to
  port 02h with bit 4 set; writes to 00h and 01h while it is set do not
  store. Software that pulses bit 4 is unaffected; the first M115 sample
  program, which left it set, now shows its characters unshifted. Which
  behaviour the hardware has is not known (no documentation in hand).
- [DERIVED] (QUASI88 `pcg.c`) The 8253 runs at 3.9936 MHz; only counter
  mode 3 (square wave) gives a tone, while its gate bit (port 02h bits 3,
  6, 7) is set; a gate 0->1 restarts it; counts up to 266 are silent; an odd
  count is high for (n+1)/2 clocks. Modes 0-2, 4, 5 give no tone (their
  single edges or one-clock pulses are not reproduced).
- [POLICY] The square waves are band-limited (PolyBLEP) to remove the
  aliasing heard as unclear tones (#41).
- [POLICY] Credits: 情報 -> About names X88000 (Manuke, public domain) and
  QUASI88 (showtime); `docs/licenses/pc8801-references.txt`, embedded under
  Third-party licenses, describes what was used from each.
- [POLICY] The PCG-8800 bank switching (port 03h) stays as in X88000;
  QUASI88 models the PCG-8100 only.

## Implementation

- `io/pcg88.c`, `io/pcg88.h`: store on port 02h only; 8253 RL and mode
  per counter; 3.9936 MHz; mode 3 only; 266 limit; odd-count duty; gate
  restart; PolyBLEP output.
- `sdl2/gui/gui.cpp`: the About credit (dialog slightly taller).
- `docs/licenses/pc8801-references.txt`, `cmake/third_party_licenses.cmake`,
  `docs/licenses/THIRD_PARTY_NOTICES.md`; [`pcg.md`](../../modernization/pcg.md).

## Verification

- Selftest `PCG-8100/8800`: no store from ports 00h/01h with bit 4 set and
  a store from 02h; 3.9936 MHz counts (LSB/MSB, MSB only, BCD); 266
  silent and 267 audible; mode 2 silent; band-limited output (samples
  between the two levels). Mutation check: 8 of 8 mutants of `io/pcg88.c`
  killed (store from 00h restored, store removed, mode-3 check, 266 limit,
  clock, PolyBLEP, mode decode, MSB-only load).
- About dialog checked in a screenshot (credit wrapped below the version).
- Clean-checkout MinGW build of `9be3174d`: `vaeg.exe` SHA-256
  `57b838d87207d2581480238818b8fa8308316b41dbc37267ad1452703361b815`; ctest
  92 of 92. No media builder changed.

### G115a

- Passed on 2026-10-16 (maintainer), with the Windows build
  `57b838d87207...b815` (SHA-256, static MinGW) from
  [9be3174d](https://github.com/nakatamaho/vaeg/commit/9be3174d4c4eabcb14c6f7e407c3b1499323e2b7):
  the PCG tones sound clear and the stray PCG dots are gone; the About
  credit is shown. Closes issue #41.
