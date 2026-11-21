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

# M157 integration QA: media/demo regression

## Identity and decision

The maintainer passed G157 on 2026-11-21 and then requested the media/demo
regression campaign only; presenter automation was not requested (**NOT_RUN**).
Evaluated candidate:
[afe6692eba5749731cf57f4b8d486cc706096877](https://github.com/nakatamaho/vaeg/commit/afe6692eba5749731cf57f4b8d486cc706096877)
(emulator sources equal the gate build `d8f700b1`). Every check ran from fresh
detached clones, with inputs, outputs and logs outside Git, using the same
commands, runner and contracts as [M155](m155_integration_qa.md).

| Check | Result and boundary |
| --- | --- |
| Demo media closure | **PASS 9/9**: fresh `.d88.xz` round-trip to the raw D88 and payload maps equal the committed disks; `all-demos-msdos4` and `sgp-boot` whole-image identical (same SHA-256 as M155) |
| All-demos drawing | **34/34 PASS_DRAW_CONTRACT** (17 PC-Engine, 17 MS-DOS 4.0), first pass |
| MANDEL.BAS from N88-BASIC | **PASS_PROGRESS_CONTRACT**, red-prefix rows 16 to 27 |
| Self-booting SGP | **PASS_BOOT_CONTRACT** |
| Utility FDD and two SASI images | Build exit 0; four boot contracts **PASS** (FDD on VA and VA2, each SASI alone on its model) |
| Presenter automation, Cocoa, Windows execution, physical GPU, true HiDPI | **NOT_RUN** |

## Workers and tools

| Worker or tool | SHA-256 |
| --- | --- |
| Linux x86_64 worker from a clean clone, `linux-release`, recording OFF, local MP4 encoders OFF | `c0e9742b30c3f0add0c8c24394b4d90b6408b6fa92648068f26fec3bf066275d` |
| Task-local NASM 2.16.01 (source archive SHA-256 `c77745f4802375efeee2ec5c0ad6b7f037ea9c87c92b149a9637ff099f162558`) | `b2a8b5251fa7b16ae2a0f20156d63b9378761bce68c90d4e460cba341d28e3e3` |

The Open Watcom archive matched the pinned SHA-256
`e1bc4e88fa47191118f29e53731e7f4542803c7f4c503e15d72f1f571ac0832f` (DOSBox
backend). The PSD pipeline used an isolated virtual environment with
psd-tools 1.26.0, Pillow 13.0.0 and NumPy 2.5.4, ImageMagick hidden from the
child PATH; `lha` was lhasa 0.5.0 through a task-local PATH entry.

## Generated images

| Generated image | SHA-256 |
| --- | --- |
| GLASS | `12ea2eaae3531bb5103904449af6345d75752757acd92f29c350c833bec19143` |
| NEON3 | `4309e8c7bf38b49a3b4712336eb2207bf4bc4d4b4448c040f99b49bdbd3801dc` |
| NEON4 | `9e403043dc95631463b0327ad503477a3f80b3a558e13a76dfc44b01e1fb4e17` |
| SGP pseudo-sprite | `7e3c99da01f0c4229f06c200c0c0f4c7662c705d692793d1ce71eeab4626ea6b` |
| SGP wireframe | `78b70caf0d2e4a444f50f110b28876a1aa62fcad7626294ac210952241f3abee` |
| Zundamon | `98868482d6ff137b042f23c951e78fa529a8ad1089362221565e49dbaa572b46` |
| All-demos distribution | `06b6ec770502a4da05e3f6fcbf46128bfa4b96f4a60774fd9e524d7186b9849f` |
| All-demos MS-DOS 4.0 | `e88c0e0332094048bcf55b334cd0430fc1c77e158344b00e57946ad9bbc84a78` |
| Self-booting SGP | `67ead94dba0cee7aa9d9a0268cf1987bf3a2764fe1275e092b2e5ece06337154` |
| Utility FDD | `2613e4b4b84774892304a38b13422f209ef5befbc012e4faf955685a61608b09` |
| Utility SASI VA | `2bc468795a08b45231cf3f5f1b144679809de95b8dfa91cc545697c3d17658aa` |
| Utility SASI VA2 | `056b0ddec0b6b87801adefffee947c01925f7e7aed9e19c5a04306f80621966c` |

The other raw images differ in directory timestamps only. No committed disk
needs updating. The utility FDD installs 125 files; DIET processes 50
executables and saves 407,478 bytes, as in M155. All commands exited 0 on
their first run.

## Retention

Inputs, generated images, captures and logs are kept under the milestone's
task scratch and are never staged. Private input identities are not recorded
here.
