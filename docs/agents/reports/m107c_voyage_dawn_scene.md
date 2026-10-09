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

# M107c — original filled sea and dawn continuation

[Task](../tasks/M107c_voyage_dawn_scene.md). The maintainer accepts the M107b
OPNA trial and requests continuation. This records that demo-specific approval,
not additional physical-hardware, clean-checkout or whole OS regression evidence.

## New scene, unchanged sound source

`VOYDAWN.COM` is a separate build: dark sea followed by rising sun, RGB332 dawn
sky, brighter water and beat-reactive gold reflection, then repeat. The existing
bank/heave and projected wire grid remain. The two visual stages span two
observed 900-tick music cycles. Six-FM/three-SSG/ADPCM score, driver, samples,
sound ownership and shutdown source are unchanged from the accepted OPNA build.
Old `VOYOPNA.COM` and legacy `VOYAGE.COM` remain byte-identical.

The sea is a min/max scanline **silhouette envelope**, not triangulated cell
polygons: CPU signed integer/Q8 edge DDA over the closed 38-edge projected
perimeter, half-open Y intervals, no horizontal-edge division, bounded 320x200
spans. SGP horizontal LINE draws the water/reflection/sun; five contiguous
20-row CLS commands draw the sky bands. Reflection is clamped to existing sea
spans. Frame tick/loop is frozen before construction while audio time stays live.
A frame finishing after clock wrap can therefore still coherently show the
previous stage; diagnostic capture names do not override its recorded stage.

All imagery/tables are independently authored public-source algorithms. No
historical-demo artwork/code/music, recording or ROM waveform is imported.
No emulator, BIOS, CPU, sound backend, vendor or existing binary payload changes.
Archived reference-tier behavior/provenance is unaffected.

## Implementation chain

- [Initial scene](https://github.com/nakatamaho/vaeg/commit/d6ca7306c237661312037167a0d57963eaf91e51).
- [Correct sky palette segment](https://github.com/nakatamaho/vaeg/commit/872219b45c4e00c0ac7964ec9de8f459bdf3105a).
- [Five bounded sky CLS bands](https://github.com/nakatamaho/vaeg/commit/d94e569e028b1d890ed26c53097c15ad713d27d8).

The initial unmerged palette lookup used BP without DS override. Captured caller
SS differed from guest DS, producing a pale sky and poor title contrast. The
corrected explicit DS read restores the intended colours; an exact-code negative
fixture removes that single override and rejects `M107C_SKY_SEGMENT`. The
[permanent ledger](../../modernization/bug-fixes.md#m107c--dawn-sky-palette-indexed-the-callers-stack-segment)
records the demonstrated cause/correction. Initial and corrected private results
are retained rather than overwritten.

The final CLS-band change removes 100 sky LINE descriptors and their per-row CPU
setup without changing the intended flat-band colours. The final evaluated
runtime candidate is `d94e569e028b1d890ed26c53097c15ad713d27d8`.

## Local validation

```text
Dawn content/DDA/staging/segment/band tests: Ran 10 tests / OK
Existing OPNA generator/codec/source tests: Ran 10 tests / OK
Existing VOYAGE tests: Ran 12 tests / OK
M107C_SCENE_OK edges=38 poses=900 words_max=6525/8192
Repeated raw/COM: byte-identical
Silent build: assembles
Old OPNA and legacy COM comparisons: cmp exit 0
Raw payload: 29560 bytes, below D000h reserve boundary
Encoding/EOL/case: 0 violation(s), 0 violation(s), 0 finding(s)
Working/staged/final diff checks: exit 0
```

Focused negatives begin with a passing content fixture, mutate one property and
assert its exact code, with no Git/history/private-corpus dependence. The DDA
model tests all 900 projected poses and signed/fractional edge vectors; it mirrors
arithmetic rather than independently proving hardware raster output. Conservative
capacity includes every primitive changing colour: 6525 words, with 8192 reserved.

## Native observations and limits

Reuse the normal trace-OFF M107b Linux worker only after byte/diff proof of
unchanged emulator/CMake/vendor inputs. Its build and ROMful smoke are retained
from M107b; no new worker or SST run is justified for these guest-only changes.
New guest/media builds and changed runtime profiles were evaluated, retaining
worker/guest/corpus/contract/policy identities, commands, statuses and output
hashes privately. No private filenames, identities or captures are tracked.

Final VA2/OPNA, ymfm backend, standard CPU multiple 2, model-default SGP,
no-cfg/full-frame/no-wait observations:

- Both stages reached: night, intermediate dawn and full dawn; sun/reflection
  and restored title contrast are visible. Selected list sizes 2672..3706 words,
  below the conservative/allocated bounds; bounded-wait failure flag is zero.
- Two completed live music loops plus wrapped cue: 565 FM note-helper requests,
  14784 guarded bank-data write sites; per-voice counts 129,257,17,17,128,17.
- ADPCM upload 4096 bytes, 73 playback-start helpers; present=1/error=0.
  Three SSG volumes are nonzero at the wrapped cue. This proves dispatch/guard
  progression, not audible decoding, timbre quality or physical-chip accuracy.
- Complete-line input: Escape clean Ready console, visible relaunch, second
  Escape clean Ready console; all runs exit 0 and media identity is unchanged.
  An early relaunch screenshot was during black startup; a focused later capture
  reached the visible scene. Prior results retained. DIR was submitted but no
  visible directory listing/full OS-operation gate is claimed.
- Sampled two-cycle interval approximately **52.37 emulated seconds**, with
  **11.34 geometry publications/second** on this worker. These are observed
  guest-clock measurements, not hardware or portable duration/FPS guarantees.

The accepted **music source** is unchanged, but denser rendering and different
poll opportunities change missed-VBlank observability, cadence and perceived
music tempo. Do not describe this as identical timing to M107b or fixed BPM.
No emulator timing correction or performance override is introduced.

## Handoff / G107c

```sh
VOYAGE_DAWN=1 bash demos/va-voyage/build-opna.sh /outside/output
```

Use the wrapped `VOYDAWN.COM` on a private validation D88, V3/VA2+OPNA; type
`VOYDAWN`, wait through the first music cycle for dawn, Escape to return.
Generated disks, COMs, samples, listings and screenshots remain outside Git.

**G107c human review remains pending:** colours, undulation/smoothness, changed
cadence, musical balance, Escape/relaunch. Physical VA/VA2 and full clean-checkout
V3/bundled-demo/OS checks remain separate. No next scene/milestone is begun.
