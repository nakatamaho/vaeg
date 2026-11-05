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

# M143 runtime-speed integration QA

## Identity and decision boundary

Evaluated integration source:
[0acfa7dc8931d58d7a4731880401e24cda351aba](https://github.com/nakatamaho/vaeg/commit/0acfa7dc8931d58d7a4731880401e24cda351aba).
It combines the accepted runtime-speed branch with accepted main/M144. It does
not include WSR-E's provisional model, optional state section or hardware probes,
and does not include M145. **No main merge, new human acceptance or release is
claimed.** The original runtime-speed human acceptance remains valid; the new
reference corpus has not been independently approved.

Reusable QA implementation:
[b8137fd1b49f7cac72e793792470a1f114bde5a8](https://github.com/nakatamaho/vaeg/commit/b8137fd1b49f7cac72e793792470a1f114bde5a8).
Only QA tools changed after the evaluated source; no emulator/build input changed.
Workers and expensive evidence retain their original identities, not the later
QA or documentation commit's identity.

| Check | Result and boundary |
| --- | --- |
| Native integrated-tree CTest | **158 PASS / 16 SKIP / 0 FAIL**, exit 0, 225.68 s; precommit source tree exactly matches the committed candidate |
| Canonical native candidate | Configure/build exit 0; five focused checks PASS, including ROM-less, real state storage and trace equivalence; release identity and MacPorts helper checks also passed |
| Linux arm64 QA worker | Configure/build exit 0; ROM-less, state-storage and trace-equivalence checks **3/3 PASS** |
| Demo/media regression | **PASS**: nine disk closures, 34 coloured-drawing contracts, MANDEL, self-booting SGP, utility generation and four utility boot contracts |
| Mapped runtime-speed text/interaction | **PASS**, ten cases including actual quick-save/load guest restoration and reset |
| Mapped layout | Two independent 32-case captures; **32/32 exact matches**, no dynamic masks; **APPROVAL_REQUIRED**, comparator exit 2, not a visual gate PASS |
| Native Cocoa capture | **BLOCKED**: no active display; screen-capture permission unavailable |
| Windows execution | **NOT_RUN**; the old accepted Windows executable is not relabeled as an integrated-source test |
| New standard clean MinGW handoff | **BLOCKED**, dedicated container wrapper exit 1 before the build; no daemon fallback or runtime startup |
| Physical GPU / true HiDPI | **NOT_RUN**; virtual X11 and 200% UI scale do not establish either |

The maintainer must approve the frozen references and explicitly resolve/defer
remaining requested targets before the main merge. This is not a request to
repeat the already accepted G143 human gate.

## Build profiles

Both new workers are non-personal QA builds: recording ON, isolated static
FFV1/PCM FFmpeg, public-build policy ON, bundled local MP4 encoders explicitly
OFF, tests/integration tracing ON, librashader OFF. The macOS build uses static
SDL2; Linux uses the distribution's SDL2. Neither is the shader-enabled Windows
handoff. The Linux target is an isolated ARM64 VM, mapped Xvfb/Openbox at 96 dpi,
logical/drawable ratio 1, software SDL renderer and dummy audio for interaction tests; the layout
matrix also exercises SDL OpenGL on that software integration target.

| Worker | SHA-256 |
| --- | --- |
| Darwin arm64, canonical candidate | `d7b1904dd87e17e81ced772cb4581b4429c43d667a8d20de3e745d94a210046e` |
| Linux arm64, canonical candidate | `095272db9fb566819b327531107ce39b134c3b1389685278872eef81960d544d` |

The full native test campaign predates commit creation, including its old
embedded build label. After creating the exact-tree commit, the canonical
worker was rebuilt and the five focused checks were run against it. These are
separate records, not a claim that the canonical binary ran all 174 tests.
Subsequent QA-only edits re-run affected Python contracts, not unchanged
emulator/media workloads. Exact commands, source-tree proofs, worker/log digests,
input bindings and failures remain in the private campaign manifest.

## Demo and utility media

Media was generated from a fresh clean clone of the pushed evaluated commit.
NASM 2.16.01 and retained PSD-processing prerequisites were explicitly bound;
Open Watcom uses the pinned DOS archive and the isolated DOSBox backend. No
host-system package/compiler installation, shared container fallback, private input
modification or replacement of committed disks was used.

All nine committed demo distributions have byte-identical freshly built stored
payloads. This is payload closure, not a claim that timestamps/containers of all
new raw images match the old raw disks. The self-booting SGP image is also an exact
raw match. MS-DOS 4.0 system/boot/license identities were checked by the repository
builder. Additional TSP-sprite, SGP contention/scan/triangle, wireframe,
Z80-timing and VA-voyage targets also build.

Each of the 17 all-demos executables was started under both PC-Engine and
MS-DOS 4.0: **34 PASS_DRAW_CONTRACT** results. The independently tested contract
checks changing coloured guest pixels, not merely a screenshot or process exit.
MANDEL passes its progress contract from N88-BASIC, and the self-booting SGP menu
passes its boot/content contract. These dummy-video guest tests are not presenter
or complete artistic/animation acceptance.

Utility FDD and both SASI builds exit 0. Readback validates 129, 632 and 623
stored files respectively, including FAT consistency; their payloads match the
retained accepted utility payload manifest without changed files. The FDD boots
on both VA and VA2; each SASI boots alone on its intended model. All four live
boot/content contracts exit 0. Previously built CP/M prerequisites are retained
inputs, with unchanged builder-source identity, not newly claimed CP/M tests.

### Commands and exits

The private orchestration records the fully expanded commands and input hashes.
Below, variables designate explicit private bindings, not repository defaults or
publishable source media. All commands listed here completed with exit **0**.

```sh
# Each distribution, using its documented system-disk/output arguments:
demos/glass-orbit/build-d88.sh "$PCENGINE" "$OUT/m143-glass-orbit.d88"
demos/neon3/build-d88.sh "$PCENGINE" "$OUT/m143-neon3-distribution.d88"
demos/neon4/build-d88.sh "$PCENGINE" "$OUT/m143-neon4-distribution.d88"
demos/sgp-pseudo-sprite/build-d88.sh "$PCENGINE" "$OUT/m143-sgp-pseudo-sprite.d88"
demos/sgp-wireframe/build-d88.sh "$PCENGINE" "$OUT/m143-sgp-wireframe.d88"
demos/zundamon-orbit/build-from-psd.sh --python "$PSD_PYTHON" \
  "$PSD" "$PCENGINE" "$OUT/m143-zundamon-orbit.d88"
# Fresh distribution archives are copied to their canonical names in $DIST.
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

# Child-only PATH selects isolated DOSBox; the shared cache is explicitly bound.
VAEG_OPENWATCOM_BACKEND=dosbox VAEG_OPENWATCOM_DOS_ARCHIVE="$OW_ARCHIVE" \
  tools/pc88va/build-utility-disk.sh --source "$SYSTEM_VA2" --output "$OUT/utility.d88"
VAEG_OPENWATCOM_BACKEND=dosbox VAEG_OPENWATCOM_DOS_ARCHIVE="$OW_ARCHIVE" \
  tools/pc88va/build-sasi-utility-disks.sh --source-va "$SYSTEM_VA" \
  --source-va2 "$SYSTEM_VA2" --payload-d88 "$OUT/utility.d88" \
  --cpm-tools-d88 "$CPM_TOOLS" --cpm-source-d88 "$CPM_SOURCE" \
  --cpm-dev-d88 "$CPM_DEV" --output-dir "$OUT/sasi"
```

The orchestration additionally runs the PC-Engine all-demos builder, extra demo
build scripts, extracted-payload comparisons, 34 guest drawing cases, BASIC/SGP
checks and four utility boot contracts. Their command arrays, exit files and
content-validation results are preserved outside Git.

### Generated image SHA-256

These are generated-output identities required by the media gate, not identities
of private input assets. No raw image is committed.

| Output | SHA-256 |
| --- | --- |
| GLASS | `0a6bc81a2040cbe6369731f510e6c85632ddb3debdae7e9e29edfb5c05b8c5d2` |
| NEON3 | `4a7c4b473f34f319e862d6ec5307dd7cf97997460e3a101bb3f94d694e569269` |
| NEON4 | `dc28ed0ebf6c12a52f362c2784ed6ab7792e11ad4c520b8dc09eec4ab2a2de57` |
| SGP pseudo-sprite | `4847820376629e55501a93d16bfe773ab7eadb35d382ac25831071be0e9d2375` |
| SGP wireframe | `6d0aae3d767c12394c9ceee4838d9ecdda28f19847eb2b28782e7cc34cc1b84c` |
| Zundamon | `1a31b9457af07ebb3b10da39d95b4b41c0a2398200dbd60dbbcbb6e5df82288f` |
| All-demos distribution | `4b90e1f06f908218ea69f195e107558835406a453c009fb7fbe288534e6512aa` |
| All-demos MS-DOS 4.0 | `e88c0e0332094048bcf55b334cd0430fc1c77e158344b00e57946ad9bbc84a78` |
| Self-booting SGP | `67ead94dba0cee7aa9d9a0268cf1987bf3a2764fe1275e092b2e5ece06337154` |
| PC-Engine all-demos | `de46019e32c37c978e222f2c5ce859d64fce37a09b02ae520cb4afba837ac2b8` |
| Utility FDD | `373d6cb62b2d7d7dc5d0736c61d361def3a40e50ddef9636e58af024833d6299` |
| Utility SASI VA | `fd5502e4fd624e088c50dacf0d230ed32fa27fbf59389c63a1702e3ce0c015eb` |
| Utility SASI VA2 | `49b76d891da93d27a5d08e8b6695465f39b1327d324fdd615f55e151b183625a` |

## Actual mapped runtime-speed checks

[The reusable runner](../../../tools/qa/runtime_speed_qa.py) separates capture
from content validation. It uses real mapped key/menu operations, not injected
emulator state. The target is VA2/V2H with nominal CPU/base/SGP 7.987200 MHz.
All ten cases preserve those nominal values. Observed values below are one
bounded software-integration run, not hardware performance specifications.

| Case | Speed | Equivalent MHz | Draw FPS | Guest FPS |
| --- | ---: | ---: | ---: | ---: |
| Normal | 1.00x | 8.00 | 56.4 | 56.4 |
| F11 held | 1.77x | 14.14 | 6.0 | 99.9 |
| F11 released | 1.01x | 8.05 | 56.3 | 56.8 |
| No Wait, full presentation | 1.09x | 8.73 | 61.6 | 61.6 |
| Restored to 100% | 1.00x | 7.99 | 56.4 | 56.4 |
| Paused | paused | not shown | 0.0 | 0.0 |
| Resumed | 1.00x | 7.98 | 56.4 | 56.3 |
| Narrow F11 window | 1.92x | 15.35 | 6.5 | 108.4 |
| Actual state load | 1.00x | 7.98 | 55.4 | 56.3 |
| Actual reset | 1.00x | 7.99 | 55.9 | 56.4 |

Checks include rendered multiplier/percentage/equivalent arithmetic, distinct
completed-draw/guest counters, F11's approximately 1/16 draw ratio, No Wait's
label and full-presentation ratio, pause/resume, and complete narrow tooltip
fields. Quick-save/load restores the earlier guest marker and removes the
changed marker; reset returns to the BASIC file-count prompt and then `Ok`.
Settled post-load/reset measurements are checked. The transient first sample
still has deterministic unit coverage, not a new mapped-frame claim.

```sh
# Linux, explicit mapped display and private inputs:
python3 tools/qa/runtime_speed_qa.py capture --worker "$WORKER" --roms "$ROMS" \
  --backup-seed "$SEED" --output-dir "$NEW_CAPTURE" \
  --evaluated-commit 0acfa7dc8931d58d7a4731880401e24cda351aba \
  --display "$DISPLAY" --display-kind virtual-x11
# macOS, on-device OCR of the copied/hash-verified captures only:
clang -fobjc-arc -framework Foundation -framework Vision \
  tools/qa/ocr_vision.m -o "$OCR_WORKER"
python3 tools/qa/runtime_speed_qa.py validate --capture-dir "$CAPTURE" \
  --capture-sha256 "$CAPTURE_MANIFEST_SHA256" --ocr-worker "$OCR_WORKER" \
  --output-dir "$NEW_VALIDATION"
python3 tools/qa/runtime_speed_qa.py selftest
```

Capture and OCR helper build exit 0. Initial validation exits 1 because Vision
classified a closing parenthesis as `】`. Revalidation of the same hash-bound
raw OCR records with the explicit delimiter allowance exits **0**. No numeric
characters are repaired or inferred, tolerances are not widened, and neither
OCR nor emulator execution is repeated for that parser-only correction.
The final content selftest passes **16 text/semantic + 7 evidence** controlled
single-mutation negatives, with exact error codes and independent fixtures.

## Layout and retained failed attempts

The layout runner uses paced execution, so changing No Wait measurements are
not hidden under broad masks. It captures normal/narrow/fullscreen/200%-UI
configurations, status/panel combinations, and software/OpenGL backends. All
32 second-run images match their first-run DRAFT references exactly, including
independent geometry/panel-presence checks. The existing fail-closed reference
approval rule still returns **APPROVAL_REQUIRED**, not PASS. Capture-adapter
identity is now included in reuse validation, with a one-mutation negative.

Retained failures include the initial Openbox invocation rejecting `--display`
(the display is supplied through its environment), the original capture helper
stopping the guest during expensive PNG conversion, and five Tesseract-based
attempts with misread small digits/units. PNG conversion now follows SIGCONT;
XWD pixels were already frozen. Tesseract failure records remain failures. The
replacement pipeline captures all interactions before separate on-device Vision
recognition, and retains the later delimiter-only failure/revalidation too.
None of these observations is recorded as an emulator correctness defect.

Private evidence contains source/worker/input/contract identities, all command
arrays and exit statuses, image hashes, logs, captures, state files and source
snapshots of failed adapters. Unchanged emulator workers, media results and
layout captures are reused rather than rebuilt after QA/report-only changes.
No private capture, ROM/media input identity, supplied asset, VM image or raw
state file is staged, committed or uploaded to hosted CI. WSR-E/M145 work and
retained evidence are untouched; no cleanup is authorized by this checkpoint.
