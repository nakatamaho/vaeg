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

# M107b — original OPNA Visual Audio Voyage integration

## Candidate and ownership

[Task](../tasks/M107b_voyage_opna_playback.md). PR #21's preparation is merged.
The separate `VOYOPNA.COM` integrates the original six-FM/three-SSG score and
analytical ADPCM-B kick/snare/sweep into the accepted short visual study.
No emulator, CPU, BIOS, sound backend or existing binary payload is changed.
No private art, music, code, ROM sample, waveform or media is redistributed.

- FM: bass, lead, pad, chord, delayed figure and shimmer, with explicit pan;
  mode register 29h enables six voices but no IRQ sources. Key-on remains in
  low register 28h; operator/pitch/pan channels 4..6 use the upper bank.
- SSG: three independently pitched and software-decaying tone lanes. Mixer
  I/O direction bits are preserved. No writes to chip I/O registers 0Eh/0Fh.
- ADPCM-B: 4096 source-built bytes, 8-bit DRAM mode, inclusive 32-byte start/end
  units, one-shot retriggering, bounded BRDY/BUSY handling. Normal nominal
  Delta-N is 13026, targeting 11025 Hz at 7.9872 MHz / 144. This is not a
  measured real-chip sampling rate or a guarantee for arbitrary prescalers.
- Detection reads both native status windows before ownership. Missing/stuck
  hardware disables subsequent writes; previous sound/RAM is not restored and
  mute cannot be guaranteed on unresponsive hardware.
- The OPNA variant uses hardware-measured LINE HD=0800h, VD=0400h from main's
  M106a correction. Legacy `VOYAGE.COM` is byte-identical to the accepted
  prototype, including its older LINE convention; it is not requalified here
  for the changed SGP backend. Use the new variant for this integration gate.

## Commits

- [Guest integration](https://github.com/nakatamaho/vaeg/commit/46eefaf4fbc65d0d28c278cccf4222f29e7f0302).
- [Remove the narrow persistent ADPCM wrap limit](https://github.com/nakatamaho/vaeg/commit/d408a3d2e309b292eba5b8ac36122b325175c423).

The latter is the evaluated runtime candidate. The initial unmerged candidate's
4KiB global wrap limit was inappropriate for later programs; the corrected
candidate sets FFFFh while enforcing its own upload count and sample stop units.
See the [correctness ledger](../../modernization/bug-fixes.md#m107b--avoid-retaining-a-demo-sized-adpcm-wrap-limit).

## Local content/build results

```text
OPNA generator/codec/guest-source contracts: Ran 10 tests / OK
Existing VOYAGE content/geometry/input contracts: Ran 12 tests / OK
VOYAGE_DATA_OK period=900 poses=900 line_max=218 words_max=1783/4096
M107B_SCORE_OK fm=6 ssg=3 events=128 adpcm_samples=3; generated data only
Repeated raw/COM builds: byte-identical
VOYAGE_AUDIO=0: assembles; native driver bank-data count 0
Legacy VOYAGE.COM: cmp against accepted prototype exit 0
Linux Debug trace-OFF worker build: exit 0
Private-ROM VA2 smoke: exit 0
Encoding/EOL/case: 0 violation(s), 0 violation(s), 0 finding(s)
Working/staged git diff --check: exit 0
```

New source-contract tests inspect selected helpers, not the emulator's
instruction execution. Existing negative fixtures remain content-only and assert
exact codes; the legacy boundary test now uses its explicit two-file profile
rather than sweeping in the optional OPNA include. No production bypass flag.

## Private runtime observations (not audible/hardware verification)

The corrected production COM ran on a normal trace-OFF worker at standard
CPU multiple 2, model-default SGP, VA2/OPNA, full frames, no-wait and no persisted
configuration. Register-preserving guest snapshots and M74 normal PC counters
were used; no trace-ON workaround or emulator patch.

| Observation | Result |
|---|---|
| NP2 FM backend, two completed music loops plus first cue | 565 FM note-helper calls; 14793 guarded bank-data write sites |
| Per-voice helper calls at final snapshot | 129, 257, 17, 17, 128, 17 |
| ADPCM upload / playback-start helper calls | 4096 bytes / 73 starts |
| Audio state | present=1, error=0 |
| SSG snapshot after the wrapped cue | volumes 7, 5, 6; all three lanes active |
| ymfm FM backend, one completed loop plus first cue | 285 FM note-helper calls; 9569 guarded bank-data write sites |
| Forced-silent build, two completed loops | 0 bank-data writes; 0 uploaded bytes / 0 ADPCM starts |
| Separate complete-line input run | first scene, Escape clean console, visible relaunch, second Escape clean console |
| Media identity | unchanged before/after each run |

Counters demonstrate dispatch and successful guarded paths, not actual waveform
quality, correct analog output, perceptual synchronization, or physical VA2
compatibility. The per-voice counters count helper requests, including in silent
mode; the error-free present state and actual bank-data counter separately
establish enabled dispatch. ADPCM starts are not evidence of audible decoding.
`DIR` was submitted between launches, but its chosen snapshot shows the clean
Ready console rather than a directory listing; do not claim a visible listing
or full OS-operation gate from it.

Evaluated commit, worker/guest/media/ROM identities, comparison script identity,
policy identity, exact command, status and output digests are recorded privately.
Initial and corrected results are separately retained. The worker is reused
because all emulator/CMake build inputs are identical; corrected guest inputs
were rebuilt and affected runtime checks rerun. No unchanged SST or hosted CI
profiles were requested. Documentation-only handoff edits do not rerun runtime.

## Handoff / G107b

Build outside Git with `bash demos/va-voyage/build-opna.sh /outside/output`.
Install the wrapped `VOYOPNA.COM`, not the raw payload, into a private vanilla
PC-Engine validation disk; launch `VOYOPNA` in V3 mode with VA2/OPNA, Escape to
return. Private trial media and captures are not tracked or published.

**G107b HUMAN GATE remains pending:** listen to balance/timbre/percussion,
check motion/music cues and smoothness, Escape and relaunch. Clean-checkout
V3/bundled-demo/full OS operations and actual VA/VA2 hardware are independent
verification claims. No 90-second sequence or next milestone is started.
Archived reference-tier paths, binaries and provenance are unaffected.
