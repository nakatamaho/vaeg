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

# M144a integration QA: media/demo regression and presenter automation

## Identity and decision

The maintainer passed G144a on 2026-11-11 and then explicitly requested both
pre-merge campaigns ("run"). Evaluated candidate:
[dc38adc876403504d2e3d5fc443f33db417616fb](https://github.com/nakatamaho/vaeg/commit/dc38adc876403504d2e3d5fc443f33db417616fb).
Its emulator sources equal those of the accepted gate build
[983d2db6347d06b56875d936e9fb76300b1c58d1](https://github.com/nakatamaho/vaeg/commit/983d2db6347d06b56875d936e9fb76300b1c58d1);
the later commit only records the gate. Every check ran from a fresh detached
clone of the candidate, with outputs, inputs and logs outside Git.

| Check | Result and boundary |
| --- | --- |
| Demo media closure | **PASS 9/9**: fresh `.d88.xz` round-trip to the raw D88 and payload maps equal the committed disks; `all-demos-msdos4` and `sgp-boot` whole-image identical |
| Additional source builds | **PASS 10/10** build exits (build results only, no live execution) |
| All-demos drawing | **34/34 PASS_DRAW_CONTRACT** after one retained failure and its rerun (see below) |
| MANDEL.BAS from N88-BASIC | **PASS_PROGRESS_CONTRACT**, red-prefix rows 9 to 20 |
| Self-booting SGP | **PASS_BOOT_CONTRACT** |
| Utility FDD and two SASI images | Build exit 0; four boot contracts **PASS** (FDD on VA and VA2, each SASI alone on its model) |
| Mapped runtime-speed text/interaction | **PASS_TEXT_AND_INTERACTION_ONLY**, ten cases incl. actual quick-save/load restore and reset |
| Mapped layout matrix | Two independent 32-case captures: **32/32 matched; APPROVAL_REQUIRED** (exit 2), DRAFT references, not a visual gate PASS |
| Cocoa, Windows execution, physical GPU, true HiDPI | **NOT_RUN** |

## Workers and tools

| Worker or tool | SHA-256 |
| --- | --- |
| Darwin arm64 guest worker, Release, recording ON (isolated static FFV1/PCM SDK), public policy ON, local MP4 encoders OFF, shaders OFF, tests OFF | `ee555fa9863b1ee3f38fbad8fb71811b7f0abfaa5b6edf10f5888826b39a5fb5` |
| Linux arm64 X11 worker (container from `tools/qa/x11-qa.dockerfile`), Release, recording OFF, local MP4 encoders OFF, shaders OFF | `a6315339d696350712413900eaca01806bfc2b73e131349399197bb839177128` |
| Task-local NASM 2.16.01 (source archive SHA-256 `c77745f4802375efeee2ec5c0ad6b7f037ea9c87c92b149a9637ff099f162558`) | `a994c8383837ad4bbf235dcdae4877cc760a50655e878479e439ae184fd6f629` |
| macOS Vision OCR helper (`tools/qa/ocr_vision.m`) | `aa09ac6da3ff33f91cb6389ba3dd3c6a4cb8246020baefc3110b704968ab524d` |

The guest worker was built with tests OFF: the first configure with tests ON
was rejected because the test target requires the instrumented trace build
(retained failure). The candidate's native suite had already passed 183/183 on
the development build. The Open Watcom archive matched the pinned SHA-256
`e1bc4e88fa47191118f29e53731e7f4542803c7f4c503e15d72f1f571ac0832f`; the PSD
pipeline used an isolated Python 3.12.13 with psd-tools 1.26.0, Pillow 13.0.0
and NumPy 2.5.4; DOSBox is the isolated local build named in AGENTS.md.

## Demo media

Commands (variables are explicit private bindings):

```sh
demos/glass-orbit/build-d88.sh "$PCENGINE" "$OUT/m144a-glass-orbit.d88"
demos/neon3/build-d88.sh "$PCENGINE" "$OUT/m144a-neon3-distribution.d88"
demos/neon4/build-d88.sh "$PCENGINE" "$OUT/m144a-neon4-distribution.d88"
demos/sgp-pseudo-sprite/build-d88.sh "$PCENGINE" "$OUT/m144a-sgp-pseudo-sprite.d88"
demos/sgp-wireframe/build-d88.sh "$PCENGINE" "$OUT/m144a-sgp-wireframe.d88"
demos/zundamon-orbit/build-from-psd.sh --python "$PSD_PYTHON" \
  "$PSD" "$PCENGINE" "$OUT/m144a-zundamon-orbit.d88"
python3 tools/pc88va/build-all-demos-distribution-disk.py --source "$PCENGINE" \
  --distribution-dir "$DIST" --output "$OUT/all-demos.d88" \
  --compressed-output "$DIST/all-demos.d88.xz"
python3 tools/pc88va/build-all-demos-msdos4-disk.py --source "$MSDOS4" \
  --distribution-dir "$DIST" --output "$OUT/all-demos-msdos4.d88" \
  --compressed-output "$DIST/all-demos-msdos4.d88.xz"
python3 demos/sgp-boot/build-boot-d88.py --output "$OUT/sgp-boot.d88" \
  --compressed-output "$DIST/sgp-boot.d88.xz"
python3 tools/pc88va/build-all-demos-bootable-disk.py --source "$PCENGINE" \
  --distribution-dir "$DIST" --output "$OUT/pcengine-all-demos.d88"
python3 tools/qa/all_demos_draw.py --selftest
```

All exited 0 in their final form. Additional build entries, all exit 0:
TSPSPR, SGP contention, Z80 timing, VA Voyage, VA Voyage OPNA, both 16-colour
SGP scans, the 65536-colour SGP triangle/scan and both root wireframe scripts.

| Generated image | SHA-256 |
| --- | --- |
| GLASS | `a945410eb88ca9ead622b430a98092fee9c25550e76f8f4fab731011deee45db` |
| NEON3 | `2b4dd76490079b5f6765d22b4826770b29099650a82594ab30d4fe0bef090bf7` |
| NEON4 | `52a0323aa775284baaab770413dcd9a2d5b7c2972088e9a487ee4688be8c74b2` |
| SGP pseudo-sprite | `708fe3f2ee4d860016c21e48ae544769057b64f211720240cb7db5693189cf04` |
| SGP wireframe | `5b73be4910b1b9ea0af306187b18cccab0f3f2925dfba0af4319fd6b87d1e634` |
| Zundamon | `5e9ad22fa12c9a142615219a4dddcada211f0944131a7eff6904e428c9ed4b66` |
| All-demos distribution | `977acd7407f51a0f047b141957d00e73ee53201e733f5c0ad4b751f7d94b4282` |
| All-demos MS-DOS 4.0 | `e88c0e0332094048bcf55b334cd0430fc1c77e158344b00e57946ad9bbc84a78` |
| Self-booting SGP | `67ead94dba0cee7aa9d9a0268cf1987bf3a2764fe1275e092b2e5ece06337154` |
| PC-Engine all-demos (local, not redistributed) | `47541ecd3fd6fba6c62a24ab0e9c3fb5a4b5946be88c6360750f90c6291f2523` |
| Utility FDD | `02d7b6b4753885aa3314ee502421188f3521692b42a4d53a87f732b3eca3c678` |
| Utility SASI VA | `44ec6d8cfb2b02d3d9bbd87471a73c8658e599438a6435ed3c0db988653d51fd` |
| Utility SASI VA2 | `2b36a93f3ff8fdf6bf4e9577fa38c75f6fe43e11b207f94be8aefabbdd4f93eb` |

`all-demos-msdos4` and `sgp-boot` equal the M143 values byte for byte. The
other raw images differ in directory timestamps only; payload maps (path, size,
SHA-256 of every stored file) equal the committed distributions. No committed
disk needs updating.

Retained runner failures, none an emulator defect:

1. The first Zundamon build ran with `/opt/local/bin` on the child PATH, so the
   builder used ImageMagick `convert` for the atlas; `ZUNDAMON.BIN` then differed
   from the committed payload (and so did the two aggregate disks). The second
   build, with ImageMagick removed from the PATH, failed because `xz` was also
   removed (exit 127). The third, with only `xz` exposed through a task-local
   shim, went through psd-tools, and its payload matches. The failed outputs
   and logs are retained.
2. Two later runner re-invocations hit the builders' refusal to overwrite
   existing outputs (`sgp-boot`, and Zundamon). These re-invocations overwrote
   the logs of the earlier successful runs. Those runs' exit 0 is recorded only
   from the console. The outputs themselves are unchanged and are verified by
   the closure check.
3. The first SASI build exited 1 with `OFFLINE_MISS: cpm_license`, as in M142.
   The pinned CP/M license text was fetched into the shared cache's `cpm/licenses/`
   by `install_cpmva.py --download-only`, under the builder's documented
   acceptance contract (checksum-locked). The rerun exited 0. Nothing else in
   the cache changed.

## Live guest checks

Target: native macOS, SDL dummy/software guest frames, VA2, default H/8 MHz,
isolated per-case copies, HOME and logs. Each program is started by typed `CD`
commands (with a 1200-frame pause after each `CD` under MS-DOS) and judged by
the unchanged [content validator](../../../tools/qa/all_demos_draw.py) on a
pre-command frame (550) and two later completed frames (9000/12000;
Zundamon 15000/21000).

| Program group | PC-Engine | MS-DOS 4.0 |
| --- | ---: | ---: |
| GLASS, NEON3 (2), NEON4 (2) | 5/5 | 5/5 |
| 16-colour pseudo-sprites (5) | 5/5 | 5/5 |
| 256- and 65536-colour pseudo-sprites (3) | 3/3 | 3/3 |
| Wireframe (3) | 3/3 | 3/3 |
| Zundamon | 1/1 | 1/1 |
| **Total** | **17/17** | **17/17** |

The first eight-way parallel pass gave 33/34. MS-DOS `SPRITE\65536\SGP655S`
failed `G139_DEMO_DRAW_MISSING`: after `CD SPRITE` and Enter, MS-DOS printed no
new prompt and did not echo the later keystrokes up to frame 12000. The failed
case is retained. The same case then passed 3/3 alone and 16/16 under
eight-way load. A further 64 loaded runs per worker gave 64/64 for the
candidate and 64/64 for a main (`dc023c9b`) worker built the same way
(`c90052c31aa1a6a06b5c23e2901d5686a9ed4d82807792025060c448a5233c7d`). The host-paced key
injection lands on different guest frames under load (Enter at frame 2462 in
the failure, 2281-2875 in the inspected passing runs). The stall is therefore a single
unreproduced observation. It is neither attributed to M144a nor demonstrated
as an emulator defect, and it is left OPEN for the maintainer. It is not
recorded as PASS.

MANDEL.BAS: `BASIC /87`, `LOAD "MANDEL.BAS"`, `RUN` from the PC-Engine
all-demos disk passes the progress contract (first-row escape colours
red/orange/red; red-prefix rows 9 then 20 at frames 12000/18000).
The self-booting SGP disk passes its live TSP menu contract at frame 900.

Utility boots: the FDD on VA and on VA2, and each SASI image alone on its own
model, pass `classify_boot` after a typed `DIR` (directory set, system files and
final Ready). The first SASI boots used no INI and booted to N88-BASIC (VA) or
asked for a disk (VA2); these runs are retained. M142's documented
configuration (`Boot_FixedDisk=true`, `Main_RAM_Auto=true`, `clk_mult=2`,
`CPU_Speed=100`, 24 kHz, BMS absent) was then applied and both pass. The
accepted-payload manifest used by M143 was not retained, so no file-by-file
utility payload comparison was made. That sub-check is NOT_RUN.

## Presenter automation

Target: Linux arm64 container in vaeg's dedicated Colima runtime, mapped Xvfb
1280x1024 at 96 dpi with Openbox, SDL x11 with software and OpenGL (Mesa
software) renderers. This is a virtual-X11 software-integration target, not
Cocoa, Windows, physical GPU or true HiDPI. The VA2 backup seed was freshly
initialized by the worker. With the runners' INI it reaches the N88-BASIC
file prompt.

```sh
python3 tools/qa/runtime_speed_qa.py capture --worker "$LINUX_WORKER" --roms "$ROMS" \
  --backup-seed "$SEED" --output-dir "$CAPTURE" \
  --evaluated-commit dc38adc876403504d2e3d5fc443f33db417616fb \
  --display :1 --display-kind virtual-x11
python3 tools/qa/runtime_speed_qa.py validate --capture-dir "$CAPTURE" \
  --capture-sha256 f1cfcd65a9359d459e59eabb26017f93712d339c6d5feb620a689f2ac8406eef \
  --ocr-worker "$OCR_WORKER" --output-dir "$VALIDATION"
python3 tools/qa/x11_matrix.py capture ... --output-dir "$LAYOUT_A"   # and "$LAYOUT_B"
python3 tools/qa/x11_matrix.py prepare --capture-dir "$LAYOUT_A" --output-dir "$DRAFT"
python3 tools/qa/x11_matrix.py compare --reference-dir "$DRAFT" --capture-dir "$LAYOUT_B" \
  --manifest-sha256 9d2dc237dc55f9b01738e4f60229851d61fbd1d697827b55b43900b0d6a1d0f9 \
  --output-dir "$COMPARE"
```

Runtime speed (one bounded software-integration run; nominal CPU/base/SGP
7.9872 MHz preserved in all ten cases):

| Case | Speed | Equivalent MHz | Draw FPS | Guest FPS |
| --- | ---: | ---: | ---: | ---: |
| Normal | 1.00x | 7.99 | 56.4 | 56.4 |
| F11 held | 10.58x | 84.51 | 36.9 | 597.0 |
| F11 released | 1.01x | 8.05 | 56.4 | 56.8 |
| No Wait | 2.87x | 22.96 | 161.7 | 162.1 |
| Restored | 1.00x | 7.98 | 56.4 | 56.3 |
| Paused | paused | not shown | 0.0 | 0.0 |
| Resumed | 1.00x | 7.99 | 56.4 | 56.4 |
| Narrow F11 | 11.83x | 94.47 | 41.5 | 667.3 |
| State load | 1.00x | 7.97 | 56.4 | 56.3 |
| Reset | 1.00x | 7.97 | 56.4 | 56.3 |

Capture exit 0 (`CAPTURED_UNREVIEWED`, guest state restore and reset Ok
verified during capture); OCR/content validation exit 0,
`PASS_TEXT_AND_INTERACTION_ONLY`. The F11 and No Wait multipliers are higher
than M143's VM run, which reflects this host's speed, not a contract change. The
runner and matrix selftests pass (16 text/semantic + 7 evidence negatives).

Layout: both 32-case captures exit 0. A DRAFT reference set prepared from the
first matches the second **32/32** with no masks. The fail-closed approval
rule returns **APPROVAL_REQUIRED**. M143's draft references were not retained,
so there is no comparison against the earlier milestone's layout. Approving
references remains the maintainer's decision.

## Retention

Inputs, generated images, captures, logs, failed attempts and the runner
scripts are kept under the milestone's task scratch. They are never staged.
Private input identities are not recorded here. The container was removed
after the campaign. Its image remains in vaeg's dedicated runtime, and
`docker context show` is unchanged (`colima`).
