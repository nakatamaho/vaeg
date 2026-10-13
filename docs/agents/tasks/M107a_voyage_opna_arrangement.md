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

# M107a — OPNA ensemble arrangement preparation

## Authorization and single-task boundary

The maintainer accepts the cute M107 prototype and explicitly requests continuing
with OPNA **six FM voices, three SSG voices and ADPCM**. This records approval to
continue the short prototype; no new real-hardware, clean-checkout or regression
results are inferred from that message. M107 is merged through PR #19.

This task is the source-data/arrangement checkpoint, not a guest playback or
90-second expansion milestone. The old `VOYAGE.COM` build remains unchanged.
A separate worktree avoids concurrent documentation work in the shared checkout.

## Deliverable

`demos/va-voyage/opna_score.py`: independently authored 128-event arrangement
with six distinct FM lanes, three distinct SSG tone lanes, and an ADPCM trigger
lane. FM roles: bass, lead, pad, chord, delayed echo and shimmer. SSG roles:
counter-arpeggio, high accent and low rhythmic tone; volume/noise gating remains
future guest-driver work. ADPCM samples: analytical kick, synthetic snare and
pitched sweep; no sampled original/private audio, ROM rhythm sample, voice,
third-party music or imported waveform.

Generate Delta-T high-nibble-first four-bit samples with signed PCM predictor,
127..24576 adaptive step, 32-byte-aligned payloads. Source sample-rate intent is
11025 Hz, **not** a demonstrated chip playback rate. Validate scores and samples
before writing outside the repository. Keep all PCM/ADPCM/JSON/media output
untracked. Existing binary payloads and all emulator sources remain untouched.

## Integration contract for the next task (not implemented here)

- Separate VA2/OPNA variant; preserve the accepted low-bank OPN prototype.
- Detect low/high windows read-only; fail silent on absent/stuck hardware.
- FM voices 1..3 at 44h/45h, voices 4..6 at 46h/47h; key-on still uses low
  register 28h with channel codes 0,1,2,4,5,6. Audit/enable OPNA six-FM mode
  (low register 29h) without enabling unwanted IRQs; explicit stereo/pan setup.
- Three SSG channels with independent pitch/envelopes; preserve I/O direction.
- Audit ADPCM-B DRAM mode, address units, BRDY/EOS and Delta-N rate against the
  documented VA OPNA interface before upload. Exclusive sound-memory ownership,
  bounded writes/waits and announced sample range; no ROM/rhythm-ROM writes.
- Stop all six FM channels, all SSG volumes, ADPCM and owned timers on Escape;
  retain the existing GVRAM cleanup and loader return/relaunch contract.
- Common visual/audio cue timeline, bounded private boot/dispatch captures and
  separate audible human gate. No emulator workaround or performance change.

## G107a — machine-verifiable preparation checkpoint

Run focused tests of score dimensions/ranges, fixed codec vectors/nibble order,
PCM bounds/alignment, deterministic samples and reconstruction error. Negative
cases begin from passing fixtures and assert exact content-layer error codes;
no Git/history dependencies or production bypass flags. Generate twice outside
Git and compare every output. Run the existing 12 VOYAGE tests to ensure this
unwired module does not alter its accepted source contract; repository UTF-8,
LF, case and diff checks before push. No emulator rebuild, ROMful rerun, SST or
hosted CI is justified by this unwired generator. Record output in this task.

## Preparation result

```text
OPNA preparation tests: Ran 7 tests / OK
Existing VOYAGE tests: Ran 12 tests / OK
M107A_SCORE_OK fm=6 ssg=3 events=128 adpcm_samples=3; preparation only
Two out-of-tree output sets: diff -rq exit 0
Encoding: 0 violation(s)
EOL: 0 violation(s)
Case: 0 finding(s)
git diff --check: exit 0
```

G107a preparation checks pass. The accepted guest assembly, generator and build
script, emulator sources and CMake inputs are unchanged. This is source/data
preparation only; no emulator rebuild/behavior rerun or hosted CI was requested.
The archived reference tier and its provenance are unaffected.

Guest integration and audition are still pending. Passing G107a permits a later
integration task; it must not be reported as hearing six FM/three SSG/ADPCM in
VOYAGE.COM.
