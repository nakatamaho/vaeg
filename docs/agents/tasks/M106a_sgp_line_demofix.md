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

# M106a — Backport the hardware SGP LINE fix and rebuild demo distributions

## Authorization and scope

The maintainer requests a new main-based demo-fix PR and rebuilt disks after
reporting broken lines in all three wireframe colour depths. Backport only the
LINE correction and its required demo/build dependencies from the independently
worked M106 branch; do not merge its boot loaders, panel, utility media, BIOS
analysis or MS-DOS distribution work.

Hardware evidence already recorded in that branch establishes LINE
`HD=0800h`, `VD=0400h`, contrary to the Technical Manual table. The maintainer
confirmed GLASS and all three SGPWIRE variants on a PC-88VA2. No new hardware
measurement is claimed. BITBLT/PATBLT direction decoding remains unchanged.

The emulator and every affected demo source must change together. Rebuild the
wireframe, GLASS, NEON4, 65536-colour pseudo-sprite and NEON3 distributions,
then the aggregate non-bootable all-demos distribution. NEON3's approved
ADR-0017 source dependency is needed to rebuild rather than patch its COMs.
Keep unrelated demo payloads byte-identical and all raw media out of Git.

## Validation and gate

- Build the emulator and run the SGP selftest and applicable local CTests.
- Prove the four raw direction tests fail with the old LINE mapping.
- Rebuild all three SGPWIRE variants twice; compare with extracted disk files.
- Verify every rebuilt disk's complete listing, payload source identity and
  XZ round-trip before placement in the allow-listed `demos/disks/` directory.
- Verify aggregate payloads against component distributions.
- Run encoding, EOL, case, archive-policy and staged-diff checks.
- Push the final main-based branch and open a PR with exact commit identities
  and validation results. Human visual acceptance remains the maintainer's
  gate; do not claim new runtime/hardware acceptance or request iterative CI.

## Provenance

Original core correction:
[cc915511](https://github.com/nakatamaho/vaeg/commit/cc9155117e433ce333a2451ffcf1325202209aba).
Original demo re-encoding:
[ef036c12](https://github.com/nakatamaho/vaeg/commit/ef036c12154d3ae4919da2c744dce600f286d01f).
Prior wireframe hardware acceptance:
[2ba2c8d1](https://github.com/nakatamaho/vaeg/commit/2ba2c8d1b9530f48b04aab1c3629863bb89369cf).
Raw private evidence is not imported into this PR.
