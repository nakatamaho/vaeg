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

# M155 integration QA: media/demo regression and presenter automation

## Identity and decision

The maintainer passed G155 on 2026-11-21 and then explicitly requested both
pre-merge campaigns. Evaluated candidate:
[60f5176247b7a14970db6f002fda960ded30da74](https://github.com/nakatamaho/vaeg/commit/60f5176247b7a14970db6f002fda960ded30da74)
(emulator sources equal the gate build `804e8c9e`). The utility media were
built from
[36fd5bef0cfd971ee20c778427ad54c745485219](https://github.com/nakatamaho/vaeg/commit/36fd5bef0cfd971ee20c778427ad54c745485219),
which differs from the candidate only in `tools/cpmva/` (the lhasa listing
fix found by this campaign). Every check ran from a fresh detached clone, with
inputs, outputs and logs outside Git. The later status-text commit
`75ba032d` is not covered by these captures.

| Check | Result and boundary |
| --- | --- |
| Demo media closure | **PASS 9/9**: fresh `.d88.xz` round-trip to the raw D88 and payload maps equal the committed disks; `all-demos-msdos4` and `sgp-boot` whole-image identical (same SHA-256 as M144a) |
| All-demos drawing | **34/34 PASS_DRAW_CONTRACT** (17 PC-Engine, 17 MS-DOS 4.0) after one retained runner failure |
| MANDEL.BAS from N88-BASIC | **PASS_PROGRESS_CONTRACT**, red-prefix rows 16 to 26 |
| Self-booting SGP | **PASS_BOOT_CONTRACT** |
| Utility FDD and two SASI images | Build exit 0; four boot contracts **PASS** (FDD on VA and VA2, each SASI alone on its model) |
| Mapped runtime-speed capture | Capture exit 0, `CAPTURED_UNREVIEWED`, ten cases; OCR/content validation **NOT_RUN** (macOS-only helper; maintainer accepted Linux capture only) |
| Mapped layout matrix | Two independent 32-case captures: **32/32 matched; APPROVAL_REQUIRED** (exit 2), DRAFT references, not a visual gate PASS |
| Cocoa, Windows execution, physical GPU, true HiDPI | **NOT_RUN** |

## Workers and tools

| Worker or tool | SHA-256 |
| --- | --- |
| Linux x86_64 worker from the clean clone, `linux-release`, recording OFF, local MP4 encoders OFF | `9dc397dd2ba5591a6caf096ec2cbdf197fba1b7ce1135fd85c67c2d5f7009555` |
| Task-local NASM 2.16.01 (source archive SHA-256 `c77745f4802375efeee2ec5c0ad6b7f037ea9c87c92b149a9637ff099f162558`) | `f1cbbb4e33cf740f319aefa5b3e88377d4d655eb4adcb949b3739671d5a66ffa` |

The Open Watcom archive matched the pinned SHA-256
`e1bc4e88fa47191118f29e53731e7f4542803c7f4c503e15d72f1f571ac0832f` (DOSBox
backend, host DOSBox). The PSD pipeline used an isolated Python virtual
environment with psd-tools 1.26.0, Pillow 13.0.0 and NumPy 2.5.4, with
ImageMagick hidden from the child PATH. `lha` was lhasa 0.5.0 through a
task-local PATH entry.

## Demo media

The same commands as [M144a](m144a_integration_qa.md#demo-media), with the
build scripts' `demos/disks/` outputs collected into a scratch distribution
directory. All exited 0.

| Generated image | SHA-256 |
| --- | --- |
| GLASS | `41615096f5e610543269ea4fa9948003c212136199c6f37ae20e585fe4d67f54` |
| NEON3 | `1bfd68068acf5c7d31b8d33e2e530b040711741cc2eef69f0dd401a3ff3718c9` |
| NEON4 | `c2fdfdea4c85bbc383143009ff31cfd4b58b93e0764e9afaa22c3ed99ca89cbe` |
| SGP pseudo-sprite | `6a6f04de30cb695004e1b439fa8d667d46676605cd84ce1d3bf0494e44a84482` |
| SGP wireframe | `ebcceb50ee574ed3254f46f70f528aeb87436f6d76139317c9aef3e322cd0a39` |
| Zundamon | `d293cdb0e619f2356425f27bc50ba21ff6a827b0bde6d5a018bac905f0edc0d5` |
| All-demos distribution | `c35a220470b585e7d2adbc0f9ea9b9ea3ff1a84a0053f5f2b8874f288825dc12` |
| All-demos MS-DOS 4.0 | `e88c0e0332094048bcf55b334cd0430fc1c77e158344b00e57946ad9bbc84a78` |
| Self-booting SGP | `67ead94dba0cee7aa9d9a0268cf1987bf3a2764fe1275e092b2e5ece06337154` |
| Utility FDD | `d683b422cc575dae2e69ac2ecc29f08718a5b2c6a71aef858979a6c1d34e27f2` |
| Utility SASI VA | `18d09bbe65ab2f88dfe5d705945d71d4d6e6282ded8f563c54aaff1526c5fc33` |
| Utility SASI VA2 | `22aaf5c2258ef87beca0c235d67deb94e69924f34b6b0183c0a619558518fa51` |

The other raw images differ in directory timestamps only; payload maps (path,
size, SHA-256 of every stored file) equal the committed distributions. No
committed disk needs updating. The utility FDD installs 125 files; DIET
processes 50 executables and saves 407,478 bytes, as in M142.

Retained runner failures, none an emulator defect:

1. The first build used the host's NASM 3.01. `NEON200.COM` and
   `NEON400.COM` (57,277 instead of 57,357 bytes) and `ZUNDAORB.COM` then
   differed from the committed payloads. With the pinned NASM 2.16.01 every
   payload matches.
2. The first CP/M prerequisite build found `lha` to be jLHA, which extracted
   nothing. With lhasa 0.5.0 the installer stopped with `ARCHIVE_FORMAT:
   cannot parse lha member size`: lhasa 0.5 prints a UID/GID column before
   the size. This is a demonstrated tool defect, corrected in `36fd5bef` with
   a test and recorded in the [ledger](../../modernization/bug-fixes.md#m155--cpm-installer-misread-lhasa-05-listings).
   The rerun exited 0.
3. The first two SASI boots ran with `--no-bkupmem`, so `Boot_FixedDisk` was
   never applied to a backup image (N88-BASIC on VA, a disk request on VA2).
   With a fresh per-case backup path both pass.

## Live guest checks

Target: Linux x86_64, SDL dummy guest frames, VA2, default H/8 MHz, isolated
per-case copies, HOME and logs. Each program is started by typed `CD`
commands (first input at frame 1000 under PC-Engine and 1300 under MS-DOS,
then 300/1200 frames after each `CD`) and judged by the unchanged
[content validator](../../../tools/qa/all_demos_draw.py) on a pre-command frame
(550) and two later completed frames (9000/12000; Zundamon 15000/21000).

| Program group | PC-Engine | MS-DOS 4.0 |
| --- | ---: | ---: |
| GLASS, NEON3 (2), NEON4 (2) | 5/5 | 5/5 |
| 16-colour pseudo-sprites (5) | 5/5 | 5/5 |
| 256- and 65536-colour pseudo-sprites (3) | 3/3 | 3/3 |
| Wireframe (3) | 3/3 | 3/3 |
| Zundamon | 1/1 | 1/1 |
| **Total** | **17/17** | **17/17** |

The first eight-way pass reported 8/34: the runner passed subdirectory names
through `xargs`, which removed the backslash (`CD SPRITE16`). The failed
outputs are retained; the corrected runner passed all 26 rerun cases.

MANDEL.BAS: `BASIC /87`, `LOAD "MANDEL.BAS"`, `RUN` from the PC-Engine
all-demos disk passes the progress contract (first-row escape iterations
1/3/2; red-prefix rows 16 then 26 at frames 12000/18000). The self-booting SGP
disk passes its live TSP menu contract at frame 900.

Utility boots: the FDD on VA and on VA2, and each SASI image alone on its own
model, pass `classify_boot` after a typed `DIR`, with M142's configuration
(`Boot_FixedDisk=true`, `Main_RAM_Auto=true`, `clk_mult=2`, `CPU_Speed=100`,
24 kHz, BMS absent) and a fresh per-case backup file.

## Presenter automation

Target: Linux x86_64 host, Xvfb 1280x1024 at 96 dpi with Openbox, SDL x11.
This is a virtual-X11 software-integration target, not Cocoa, Windows,
physical GPU or true HiDPI. The VA2 backup seed was freshly initialized by the
worker (`630cb785778617cf59d0f84f9fa79a3390453f1cde32e69d9e6401a4040ef78c`).

```sh
python3 tools/qa/runtime_speed_qa.py capture --worker "$WORKER" --roms "$ROMS" \
  --backup-seed "$SEED" --output-dir "$CAPTURE" \
  --evaluated-commit 60f5176247b7a14970db6f002fda960ded30da74 \
  --display :91 --display-kind virtual-x11
python3 tools/qa/x11_matrix.py capture ... --output-dir "$LAYOUT_A"   # and "$LAYOUT_B"
python3 tools/qa/x11_matrix.py prepare --capture-dir "$LAYOUT_A" --output-dir "$DRAFT"
python3 tools/qa/x11_matrix.py compare --reference-dir "$DRAFT" --capture-dir "$LAYOUT_B" \
  --manifest-sha256 63d939f99b692104460bc8a5a40e03ec8d8d5f2f234be2051b85fbd1bb266595 \
  --output-dir "$COMPARE"
```

Runtime speed: capture exit 0 (`CAPTURED_UNREVIEWED`, ten cases including
quick-save/load and reset). Without the OCR helper the text contract is
**NOT_RUN**; an unvalidated visual inspection of the F11 case showed 3.98x,
about 31.8 MHz equivalent and the configured 7.9872 MHz CPU unchanged. The
runner and matrix selftests pass (16 text/semantic + 7 evidence negatives).

Layout: both 32-case captures exit 0. The DRAFT reference set prepared from
the first matches the second **32/32**. The fail-closed approval rule returns
**APPROVAL_REQUIRED**; approving references remains the maintainer's
decision.

## Retention

Inputs, generated images, captures, logs, failed attempts and runner scripts
are kept under the milestone's task scratch and are never staged. Private
input identities are not recorded here.
