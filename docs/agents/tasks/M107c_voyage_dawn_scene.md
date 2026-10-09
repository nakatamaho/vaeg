<!-- Copyright (c) 2026 Nakata Maho
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
OF THE POSSIBILITY OF SUCH DAMAGE. -->

# M107c — original filled sea / dawn continuation

The maintainer accepts the M107b OPNA trial as feeling good and requests the
next increment. Record that specific demonstration acceptance; do not infer
physical-hardware or full clean-checkout V3/demo/OS validation. PR #22 is merged.

## One-task deliverable

Separate `VOYDAWN.COM`: a two-music-cycle visual study (night followed by dawn),
not a 90-second presentation. Preserve the existing OPNA six-FM/three-SSG/ADPCM
score, driver, memory ownership, bounded waits and Escape/relaunch contract.
The accepted VOYOPNA/legacy builds remain available and byte-identical.

Add independently authored RGB332 sky bands, analytical sun disk/reflection,
and coloured sea silhouette spans below the existing projected wave grid.
This is CPU boundary-DDA/span generation plus SGP horizontal LINE rasterization,
not a triangulated/deformable polygon mesh or evidence about the historical
demonstration renderer. Retain the shared observed-VBlank timing/camera motion;
freeze frame tick/loop for coherent night/dawn staging. Actual elapsed duration
and frame rate must be observed rather than advertised as fixed.

- Closed 38-edge mesh perimeter; half-open Y intervals, skip horizontal edges.
  Signed integer plus Q8 fractional DDA, bounded 320x200 left/right spans.
- Clamp reflection to valid sea spans; all descriptors remain in owned pages.
- Closed command-capacity proof, at most 8192 words, raw payload below D000h.
  Poll the shared clock and service music during additional guest work.
- All new artwork/tables from public source algorithms, no private images,
  recordings, screenshots, original-demo data or new tracked binaries.
- No emulator/CPU/BIOS/backend/vendor changes, BIOS probing or original-media
  writes. Isolated worktree; all generation and private boot media out of Git.

## Validation and G107c

Independent content layer: schema/range/count/boundary/command-budget checks;
negative tests begin passing and assert exact codes for one controlled mutation.
Focused DDA vectors and every 900 projected poses; check signed division bounds,
span ranges and night/dawn staging. No history dependencies or bypass flags.
Existing 22 content/codec/source tests, repeated NASM raw/COM builds, old variant
byte equality, forced-silent build, encoding/EOL/case/diff/scope checks.

Reuse M107b's normal trace-OFF worker only with unchanged emulator/CMake inputs
and matching worker digest. Preserve new guest/media/contract/policy identities,
exact commands, status and output digests privately. Capture both stages,
continued six-voice/SSG/ADPCM dispatch, command-list safety, Escape and relaunch.
Reuse completed unchanged profiles; no hosted CI/SST for unchanged emulator
inputs. A documentation-only handoff edit does not rerun the guest.

## Local result

[Report](../reports/m107c_voyage_dawn_scene.md): 10 dawn tests plus the existing
22 tests pass, repeat raw/COM builds agree, forced-silent build assembles and
old COM variants are byte-identical. Command bound is 6525/8192 words; raw payload
29560 bytes. Native default-ymfm captures reach night/dawn, show 565 six-voice
note requests, 4096 uploaded bytes / 73 ADPCM starts and present=1/error=0;
selected lists are 2672..3706 words with wait_failed=0. Escape and later visible
relaunch/second-Escape pass. Worker/build inputs remain unchanged from M107b.
Observed two-cycle interval is about 52.37 emulated seconds / 11.34 geometry
publications per second; score is unchanged, but rendering changes polled-clock
observability and perceived tempo. No fixed duration/BPM or audible accuracy.

An unmerged BP/SS sky-table error was corrected with explicit DS addressing and
an exact-code negative fixture. Five CLS sky bands reduce command/CPU setup.
Initial/corrected/final evidence is privately retained; final documentation
changes do not rerun unchanged guest behavior. No new SST/hosted CI.

**HUMAN GATE:** view the new colours/sun/sea and check motion/music balance,
Escape/relaunch. Real VA/VA2, audition accuracy, clean-checkout/full V3/demo/OS
regression remain independent. Do not begin the next increment before approval.
