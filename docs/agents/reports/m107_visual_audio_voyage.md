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

# M107 — local prototype verification / G107 handoff

[Task](../tasks/M107_visual_audio_voyage.md),
[build and operating limitations](../../../demos/va-voyage/README.md).
**G107 is pending.** No original demonstration artwork, music or code, private
media, compiled payloads or raw captures are published. No emulator code changes.

## Scope and source checkpoints

- [Initial prototype](https://github.com/nakatamaho/vaeg/commit/0bb06a70640fff4957d547f9b58f7770d3606eeb):
  CPU fixed-point travelling mesh, independent bank/heave, SGP LINE/CLS, G1 pages,
  low-bank three-voice FM and SSG noise; original eight-bar score.
- [Calibration](https://github.com/nakatamaho/vaeg/commit/c5d245ccd1aceccba796c62ddb8ce153ac49fd74):
  black background, 900 observed VBlank ticks and three-tick geometry pacing.
  The initial 1200/four-tick version ran longer than the intended short loop.
- [Console cleanup correction](https://github.com/nakatamaho/vaeg/commit/656c5769cc19eeebdff3fcd6e23161e8ee4222f1):
  clear all 256 KiB of owned graphics memory before restoring the console.
- [Complete ESC discrimination](https://github.com/nakatamaho/vaeg/commit/f76f4aad431a242bd3bd1822c726128e505d528c):
  require scan 00h AND internal code 1Bh. A synthetic scan-zero Return model is
  rejected; the actual injected Return below was **1C0Dh**, not 000Dh.
- Final runtime candidate:
  [a0420a7ad5c50ef7c5433fe86a182be31f5bdacd](https://github.com/nakatamaho/vaeg/commit/a0420a7ad5c50ef7c5433fe86a182be31f5bdacd),
  adding bounded startup typeahead draining. This is input hygiene, not a
  demonstrated diagnosis of an emulator or ROM keyboard defect.

A separate worktree was used after another session changed the shared checkout.
The gate branch excludes that session's BIOS-index rename and README changes.

## Local machine results

| Check | Result / boundary |
|---|---|
| Python focused tests | 12 passed; schemas/counts, signed arithmetic, all 900 poses, wrap, command budget, score, sound polling models, full ESC contract |
| Generated content | `VOYAGE_DATA_OK period=900 poses=900 line_max=218 words_max=1783/4096` |
| NASM / wrapped COM | Built repeatedly out of tree; byte-identical paired outputs; includes reused, unchanged NEON loader |
| Forced-silent build | `VOYAGE_AUDIO=0` assembles; absent/stuck device tests are pure models, not physical missing-chip execution |
| Linux debug build | Standard trace-OFF build succeeded; ROMful `--smoke` succeeded |
| Repository | UTF-8/BOM, LF, lowercase-path and diff checks passed locally |

The optional `VAEG_Z80_COMPAT_INTEGRATION_TRACE=ON` build failed locally:
`io/subsystem.cpp` diagnostic calls access private `Clock::now()` at lines
216, 221 and 231. That pre-existing build-only issue was not corrected or bypassed
in this guest-demo milestone. Native M74 `wait-pc`/capture/counter actions work in
the normal build; instruction `trace` does not. No hosted CI or SST run is needed
for the unchanged emulator implementation. This does not assert that the
optional trace profile or the full cross-platform suite passed.

## Bounded private runtime observations

Runs used a pinned normal Linux debug worker, private read-only source ROMs and a
newly generated PC-Engine validation disk. Configuration: `--no-cfg`,
`--no-bkupmem`, NP2 sound, `--nowait`, full display, default CPU multiplier **2**
(7.9872 MHz base-times-multiplier) and model SGP clock. The multiplier was not
passed explicitly; private policy records resolve it from the checked source
default rather than incorrectly labeling it 1.

| Neutral case | Evaluated checkpoint | Observation |
|---|---|---|
| VA2 native / extra Return | f76f4aad | First visible frame at guest frame 1273; injected Return read as AX=1C0Dh at 1276; next publication at 1280 and subsequent motion remained active |
| VA2 native / two loops | f76f4aad | Loop markers at 2519 and 3768: 1249 completed guest frames between markers; 400 FM note-helper entries and 3878 successful low-bank data-write sites over two loops |
| Original VA / OPN | 656c5769 | One completed loop, changing mesh, third voice BX=2 / pitch 19CFh; 200 FM note-helper entries and 1986 data-write sites; not real-hardware validation |
| VA2 final Escape / DIR / relaunch | a0420a7a | First scene, Escape to clean prompt, DIR, visible second scene, second Escape to clean prompt; no ghost mesh behind console |

The steady VA2 loop interval measured at f76f4aad was about **22.14 emulated
seconds**, using the native clock delta at 7.9872 MHz; approximately 13.5 geometry
publications/second in the sampled interval. The final startup-only change does
not alter the steady projection/music/pacing routines. Earlier VA observations
show a different cadence; this is **not** a universal 20-second/96-BPM hardware
clock. Real VA/VA2 cadence and subjective smoothness are still gate questions.

The final keyboard/exit script uses complete command lines, including Return.
Earlier experiments with separate `@text`/`@enter` produced snapshots before the
second Enter was injected: those pictures do not prove a relaunch failure. A
private 16-publication auto-stop diagnostic also returned twice with zero video,
restore and wait errors; that modified diagnostic is not the production COM or
a substitute for the final Escape/relaunch observation.

Native note-helper counts equal 128 lead + 64 bass + 8 pad triggers per loop.
The data-write checkpoint is reached only after the low-bank BUSY/absence guards.
This demonstrates dispatch/status handling, **not audible sound quality**, correct
physical pitch, faithful synthesis, or perceived visual/audio synchronization.

Private run records retain evaluated commits, worker and generated guest digests,
corpus identities, comparison-contract and resolved target-policy identities,
exact commands, statuses and output digests. Failed preparation/trace attempts
and intermediate results were preserved. Generated disk hashes remained
unchanged across the successful read-only guest exercises. Expensive whole-core
profiles were not rerun for report-only edits.

## Demonstrated guest cleanup defect

Before the cleanup correction, Escape returned to a functioning prompt but
left the VA title/mesh visible behind it. Restoring the original video mode
reinterpreted our packed 8bpp pages, while normal console composition still
included G0. Mode restoration alone did not erase those bytes. Clearing the
owned graphics memory before the mode transition removed the ghost geometry;
DIR and repeated launch/exit remained usable. This correction belongs only to
VOYAGE, not to SGP, the BIOS or the old demonstration.

## Human gate still required

Clean-checkout build, normal V3/bundled-demo/OS regression checklist, then watch
at least two loops, listen to FM/SSG, judge waves/colour/bank and synchronization,
Escape and relaunch. No raw recording was auditioned here. Machine VA and VA2
cases do not validate physical hardware. Do not expand to 90 seconds, a filled
sea or a TSP-logo layer until the maintainer passes G107.
