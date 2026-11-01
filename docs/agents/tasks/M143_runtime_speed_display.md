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
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
-->

# M143 — Measured runtime speed and fast-forward display

**IN_PROGRESS; G143 NOT_READY.** Branch: `topic/m143-runtime-speed`.
Base: `950efef393196497ed2a9f1c017a3f0ae30f5323`.

## Authorized scope

The maintainer approved the compact normal display and expanded F11/No Wait
speed display proposal, with detailed tooltip and narrower-window fallback.
Show nominal CPU clock separately from measured guest-time/host-time speed and
approximate equivalent MHz. Keep guest-frame rate separate from completed-draw
rate; F11 deliberately draws only one in 16 frames. Do not use a fixed 60FPS
reference or multiply nominal MHz by draw FPS.

Measurement must use the emulated board-clock timeline and its actual clock
rate. Discard mixed samples at reset/load, pause/resume, clock/domain and
fast-forward transitions. Handle wrap, zero time, stale values and overflow.
The equivalent figure is an estimate, not executed-instruction throughput or
an assertion about physical hardware. Do not change CPU/device timing, F11
pacing/draw-skip, save formats, accepted M142 scope or deferred M142 issues.

## Verification and gate

- Pure deterministic measurement and formatting tests: 1x/8x, independent
  guest/draw rates, varying frame durations, wrap, discontinuities and narrow
  layouts; stable failure codes for invalid contracts.
- Local native and static MinGW build with recording/shaders and correct
  `m143`/eight-digit source identity. Public policy keeps local encoders OFF.
- Actual mapped presenter: normal, F11 held/released, No Wait, pause/resume,
  reset/load, narrow window and tooltip. Frozen-reference comparison is
  separate from content/geometry tests; do not claim missing targets passed.
- Standard demo/media regression and repository checks per AGENTS.md; retain
  expensive unchanged evidence under its original source/worker identity.
- G143 human gate required. No automatic merge, release or cleanup.

## Implementation and local development checks

- Frontend-only speed estimator uses the existing 64-bit guest clock and
  board Hz. Normal line remains compatible; accelerated line, tooltip,
  pause/warm-up labels and measured-width fallbacks are implemented.
- Reset/load and pause discard the old measurement window. Live clock,
  native/compatible mode, H/S, No Wait, held fast-forward and draw-skip
  changes start a new epoch. No clock, CPU execution, draw-skip or state
  format changes; the load hook only clears frontend metrics.
- Embedded-font glyph availability and six normal/accelerated width cases
  (380/640/960 window units) pass in an ImGui layout contract. This is not
  mapped-presenter capture or approved visual-reference evidence.
- Native Darwin arm64 development build: recording ON with isolated audited
  FFV1/PCM SDK, bundled local MP4 encoders OFF, librashader OFF for this test
  target. Initial system FFmpeg selection was correctly rejected by the
  static codec policy; a fresh isolated-SDK configuration passed.
- CTest campaign interrupted after 156 PASS / 16 SKIP. The unfinished trace
  equivalence and not-yet-started transfer-info tests subsequently passed.
  After layout/font changes, the ROM-less, storage and transfer-info tests
  passed again (3/3). This is staged development evidence, not yet an
  identity-bound final-commit campaign or mapped F11 guest observation.
- Repository encoding, EOL, case, Markdown-comment, milestone audit (49
  selftests), five release-identity tests and recording-release policy pass.

Clean MinGW artifact, final evaluated identity, actual mapped F11/No Wait
observations and the standard media/demo regression remain pending. G143 is
NOT_READY. M142's prior human acceptance does not accept this new behavior.
Release publication remains unauthorized.
