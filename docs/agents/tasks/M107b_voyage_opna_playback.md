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

# M107b — VOYAGE OPNA guest playback

The maintainer requests continuation after G107a's machine preparation pass.
This task integrates the original six-FM/three-SSG score and analytical ADPCM
samples into a separate `VOYOPNA.COM`, preserving the low-bank soundtrack.
No emulator, BIOS, CPU, backend, private-original or existing binary changes.
No long presentation expansion. Work is isolated from the shared checkout.

## Contract

- Native 44h/45h and 46h/47h only; read both status windows before ownership.
  FFh or bounded BUSY timeout disables future writes. Software silence build.
- Six FM voices; low register 29h six-channel mode without timer IRQ enables,
  key codes 0,1,2,4,5,6; high-bank frequency/operator/panning setup.
- Three independent SSG tone pitches and software-decaying volumes. Preserve
  mixer I/O direction bits; never write sound-board I/O ports 0Eh/0Fh.
- ADPCM-B exclusive first 4096 bytes in 8-bit DRAM mode, 32-byte address units.
  Kick/snare/sweep high-nibble-first original samples; reset before retrigger;
  inclusive end units; bounded BRDY and BUSY waits; no recording or ROM writes.
  Register 10h flags expose BRDY/EOS but register 29h enables no IRQ sources.
  Delta-N targets 11025 Hz using the nominal master-clock/144 relationship;
  distinct from hardware-measured playback rate.
- All visual/audio cues use the existing observed VBlank timeline. Preserve
  loader continuation, GVRAM clearing, keyboard and bounded wait contracts.
- Escape keys off six FM voices, mutes three SSG channels, resets/mutes ADPCM,
  stops owned timers and masks owned ADPCM flags. Previous audio/RAM cannot be
  restored; mute cannot be guaranteed on unresponsive hardware.
- Account for main's hardware-measured LINE direction correction in the OPNA
  variant, without rewriting the accepted legacy build.

## Validation / G107b

Focused generator/codec and guest-source contract tests, repeat out-of-tree
NASM raw/COM builds, forced-silent build, repository encoding/EOL/case/diff
checks. Record limitations; do not equate assembly or model tests with playback.
A normal trace-OFF Linux worker build and private ROMful boot/run/Escape/DIR/
relaunch captures are required before declaring runtime handoff readiness.
Record evaluated commit, worker/media/contract/policy identities, command,
status and output digests privately. Do not rerun unchanged completed profiles.
No hosted CI before local validation and final scope checks. Generated media,
waveforms, listings and private captures remain outside Git.

## Local result / handoff

[Report](../reports/m107b_voyage_opna_playback.md): 10 OPNA content/codec/source
checks and 12 existing VOYAGE tests pass; repeated COM/raw builds agree, the
forced-silent COM assembles/runs with zero driver writes, and the legacy COM
remains byte-identical. Linux Debug trace-OFF build and private-ROM smoke pass.
Corrected production guest observed for two NP2 loops and one ymfm loop:
4096 uploaded bytes, all six note-helper counters advance, three nonzero SSG
volumes, present=1/error=0. Escape/clean-console/relaunch/second-Escape captures
pass. DIR was submitted, but no visible directory listing was established.
Full-limit FFFFh replaces the initial narrow 4KiB wrap limit; byte ownership
remains bounded by upload count and sample stops. Generated media and all
identities/captures remain private. No hosted CI or unchanged SST rerun.

## Maintainer acceptance / continuation

The maintainer accepts the provided OPNA trial as feeling good and requests
continuation. This is the specific local-demo approval for M107c; it does not
supply new physical-hardware, clean-checkout or full V3/demo/OS evidence.

**HUMAN GATE scope:** listen to the new soundtrack, check synchronization/smoothness,
Escape and relaunch; normal clean-checkout V3/demo/OS review and real hardware
remain independent. No next milestone until approval. An implementation-only
checkpoint is not a passed G107b or a runtime-ready handoff.
