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

# M143 — Measured runtime speed and fast-forward display

**G143 human gate PASSED; integration QA recorded, reference approval pending.** Branch: `topic/m143-runtime-speed`;
integration candidate: `topic/m143-runtime-integration`.
Base: `950efef393196497ed2a9f1c017a3f0ae30f5323`.

## Authorized scope

The maintainer approved the compact normal display and expanded F11/No Wait
speed display proposal, with detailed tooltip and narrower-window fallback.
Show nominal CPU clock separately from measured guest-time/host-time speed and
approximate equivalent MHz. Keep guest-frame rate separate from completed-draw
rate; F11 deliberately draws only one in 16 frames. Do not use a fixed 60FPS
reference or multiply nominal MHz by draw FPS.

Measurement must use the emulated board-clock timeline and its actual clock
rate. Discard mixed samples at reset/load, pause/resume, clock/domain and
fast-forward transitions. Handle wrap, zero time, stale values and overflow.
The equivalent figure is an estimate, not executed-instruction throughput or
an assertion about physical hardware. Do not change CPU/device timing, F11
pacing/draw-skip, save formats, accepted M142 scope or deferred M142 issues.

## Verification and gate

- Pure deterministic measurement and formatting tests: 1x/8x, independent
  guest/draw rates, varying frame durations, wrap, discontinuities and narrow
  layouts; stable failure codes for invalid contracts.
- Local native and static MinGW build with recording/shaders and correct
  `m143`/eight-digit source identity. Public policy keeps local encoders OFF.
- Explicit-request QA: actual mapped presenter (normal, F11 held/released,
  No Wait, pause/resume, reset/load, narrow window and tooltip), frozen
  visual-reference comparison and the full demo/media regression. The
  maintainer explicitly requested both; results and remaining blockers are
  recorded below. Do not claim missing targets or unapproved references passed.
- Repository checks remain required; retain expensive unchanged evidence
  under its original source/worker identity.
- G143 human gate required and now passed. The maintainer has requested
  completion, merge and cleanup; release publication is not authorized.

## Implementation and local development checks

- Frontend-only speed estimator uses the existing 64-bit guest clock and
  board Hz. Normal line remains compatible; accelerated line, tooltip,
  pause/warm-up labels and measured-width fallbacks are implemented.
- Reset/load and pause discard the old measurement window. Live clock,
  native/compatible mode, H/S, No Wait, held fast-forward and draw-skip
  changes start a new epoch. No clock, CPU execution, draw-skip or state
  format changes; the load hook only clears frontend metrics.
- Embedded-font glyph availability and six normal/accelerated width cases
  (380/640/960 window units) pass in an ImGui layout contract. This is not
  mapped-presenter capture or approved visual-reference evidence.
- Native Darwin arm64 development build: recording ON with isolated audited
  FFV1/PCM SDK, bundled local MP4 encoders OFF, librashader OFF for this test
  target. Initial system FFmpeg selection was correctly rejected by the
  static codec policy; a fresh isolated-SDK configuration passed.
- CTest campaign interrupted after 156 PASS / 16 SKIP. The unfinished trace
  equivalence and not-yet-started transfer-info tests subsequently passed.
  After layout/font changes, the ROM-less, storage and transfer-info tests
  passed again (3/3). This is staged development evidence, not yet an
  identity-bound final-commit campaign or mapped F11 guest observation.
- Repository encoding, EOL, case, Markdown-comment, milestone audit (49
  selftests), five release-identity tests and recording-release policy pass.

## Pushed candidate and final local results

Evaluated implementation: [acee8dd6ccd813f0af1f08f2f90986e9ab3a8992](https://github.com/nakatamaho/vaeg/commit/acee8dd6ccd813f0af1f08f2f90986e9ab3a8992).
Display: `Rel.20261101 (m143, acee8dd6)`; declared milestone remains `M143-dev`.

- Native configure/build exit 0. `ctest --test-dir <native-build>
  --output-on-failure -j4`: exit 0, **158 PASS / 16 SKIP / 0 FAIL**, 174
  registered tests, 209.20 seconds. This includes the new guest-time,
  formatting, font and width contracts, existing held/repeated/released F11
  pacing controls, actual state-storage tests and trace equivalence.
  Native worker SHA-256:
  `b7c5ba67c567c08790f4e5053db0b9f4f42079d03c8e75fd0e34c86f16fdaee5`.
  Final CTest log SHA-256:
  `9a3659448f0764662cb3ac823d6bb82686ef2a06963786c53a0b72114dd19922`.
- Clean static Windows cross-build via `tools/release/build-mingw-macports.py
  --commit acee8dd6 --work-dir <new-work> --sdk-dir <isolated-sdk>
  --build-track m143 --jobs 4`: exit 0. Uses the existing reviewed SDK, not
  application objects or a container. Recording, native Windows MP4 adapter
  and librashader ON; bundled MP4 encoders OFF. Static codec and system-DLL
  import audits PASS. `vaeg.exe` SHA-256:
  `f9bc6b8db78e2d6080fb458cf9045b5d4cce5d9c5bcf230fab1ae835cdfaa684`.
  Executable, build report, manifests and notices copied and compared into
  the local `release-handoff/m143-acee8dd6/` handoff directory.
- Final repository checks: encoding 0 violations, EOL/case 0 findings,
  Markdown comments clean, milestone selftest 49 PASS, five release-identity
  and three MacPorts helper tests PASS, recording-release policy PASS and
  `git diff --check` clean. Exact commands, exit files, binary/log digests
  and the CTest command inventory remain in task scratch.

These are local test/cross-build results, not Windows execution or real
presenter validation. At that checkpoint, actual mapped F11/No Wait guest
observations, approved visual references and the media/demo regression were pending.
The maintainer subsequently accepted the human gate as recorded below.
Outstanding machine checks are not retroactively promoted to PASS. No merge,
hosted-CI success or release publication is claimed.

## Human gate acceptance

The maintainer explicitly stated **“passed”**, followed by **“human gate
passed”**, in response to the candidate handoff above. G143's human gate is
therefore **PASSED** for implementation
`acee8dd6ccd813f0af1f08f2f90986e9ab3a8992` and the handed-off executable with
SHA-256 `f9bc6b8db78e2d6080fb458cf9045b5d4cce5d9c5bcf230fab1ae835cdfaa684`.
Do not request that human acceptance again.

This acceptance does not supply per-case automated presenter evidence or
approve a frozen reference corpus. At acceptance, those checks and the full
media/demo regression were still milestone obligations. The maintainer's
subsequent policy instruction below supersedes that requirement; it does
not change their NOT_RUN status. No new milestone or release is authorized.

## Maintainer policy change: explicitly requested campaigns

After requesting M143 completion, merge and cleanup, the maintainer first
proposed release-only media/demo and actual-screen QA, then explicitly
superseded that proposal: run these campaigns only when instructed, and ask
at every milestone's main merge. AGENTS.md now applies the final policy
globally, including this task; release publication alone does not trigger it.
The completed local build, 158 PASS / 16 SKIP / 0 FAIL campaign, focused UI
contracts and accepted human gate remain valid. The general merge request
is not a request to run these campaigns or an explicit opt-out. Obtain and
record the M143 pre-merge choice before merging. Unperformed checks must
remain NOT_RUN, never PASS. Preserve existing evidence and handoff artifacts
when carrying out the authorized cleanup.

## Explicit pre-merge campaign request

The maintainer confirmed the proposed scope, **merge only the accepted runtime-speed
work into main and leave the provisional WSR-E implementation on its paused branch**,
and explicitly requested **both** the full demo/media regression and actual-screen QA.
This resolves the preceding pending choice; do not ask that choice or the original
human acceptance again. Requested checks must finish, or their exact blockers must
be reported and explicitly deferred, before the main merge.

The integration candidate combines the accepted runtime branch at
[7543bac56390e4ecb732f2bea55f35ea275430ea](https://github.com/nakatamaho/vaeg/commit/7543bac56390e4ecb732f2bea55f35ea275430ea)
with accepted main/M144 at
[cb4edc1a05221971c5a048236eeeb172d2abf73e](https://github.com/nakatamaho/vaeg/commit/cb4edc1a05221971c5a048236eeeb172d2abf73e).
The roadmap conflict retains both task records. It includes no WSR-E board/model,
WSRE state section, hardware probe, M145 work or new release authorization.
The source milestone remains development-marked until separate release preparation.

Use fresh out-of-tree outputs and a clean clone of the pushed media-source candidate.
Bind every runtime result to the actual worker, source, inputs and comparison contract.
Prior 158-PASS evidence remains historical, not a result for the integrated source.
Actual presenter capture is distinct from SDL dummy guest-frame tests. Frozen visual
references still need independent approval; earlier unapproved drafts are not accepted
merely because this QA was requested. Preserve WSR-E/M145 directories and all supplied
inputs throughout this task; shared M143 names are not a cleanup allow-list.

## Integration QA checkpoint (2026-11-05)

Evaluated source:
[0acfa7dc8931d58d7a4731880401e24cda351aba](https://github.com/nakatamaho/vaeg/commit/0acfa7dc8931d58d7a4731880401e24cda351aba).
The [integration QA report](../reports/m143_runtime_integration_qa.md) records
commands, exits, generated-image SHA-256 values, worker identities and limits.

- Native integrated-tree CTest: **158 PASS / 16 SKIP / 0 FAIL**. The precommit
  tree matches the committed candidate; its old embedded build label is not
  relabeled. The rebuilt canonical worker passes five focused checks. A separate
  Linux arm64 worker builds and passes three focused checks.
- Fresh clean-clone media regression: **PASS**. Nine distribution payload
  closures; 34 PC-Engine/MS-DOS drawing contracts; MANDEL and self-booting SGP;
  utility FDD/two SASI generation, payload readback and four boot contracts.
- Actual mapped virtual-X11 runtime checks: **10-case PASS** for rendered speed,
  nominal clock separation, F11/No Wait, pause/resume, narrow tooltip, actual
  quick-save/load guest restoration and reset. This does not qualify the first
  post-load sample, physical timing, Cocoa, Windows or true HiDPI.
- Layout: two independent 32-case captures match exactly with no masks, but the
  frozen first set remains **DRAFT / APPROVAL_REQUIRED**. Reference approval is
  human work; routine capture/comparison has already been automated.
- Native Cocoa capture is **BLOCKED** by the absent display/permission. Windows
  execution and physical-GPU/HiDPI targets are **NOT_RUN**. The initial container
  build attempt exited 1; calling that a macOS MinGW blocker was incorrect.
  The available MacPorts standard route now passes, as recorded below.
- QA-only changes are in
  [b8137fd1b49f7cac72e793792470a1f114bde5a8](https://github.com/nakatamaho/vaeg/commit/b8137fd1b49f7cac72e793792470a1f114bde5a8).
  The new content contracts pass 16 text/semantic and seven evidence negatives;
  existing X11/repository checks pass. Emulator workers/media are unchanged and
  reused with their actual identities. Failed Openbox/OCR attempts remain in
  private evidence; they are not emulator fixes or retroactive PASS results.

**Main remains unmerged.** Approve the new frozen reference set and explicitly
resolve or defer the remaining requested targets before merging. Do not ask for
G143's original human acceptance again. No hosted-CI run, release or cleanup is
claimed; WSR-E/M145 work and supplied assets remain preserved.

## MacPorts standard-route correction and handoff

The maintainer reaffirmed that macOS uses its installed MacPorts MinGW-w64,
not Docker/Colima, for the standard clean static Windows build. AGENTS.md and
BUILD.md now say so, with process-local PATH selection, an isolated reviewed SDK,
a fresh clone/application build, and unchanged recording/shader/encoder audits.
Policy commit:
[757e880730bd0fc0f29269935b35876e3552f788](https://github.com/nakatamaho/vaeg/commit/757e880730bd0fc0f29269935b35876e3552f788).

A new clean MacPorts build of pushed
[1b7e6cbc6defabf240d4c5f3a9a4572021305b36](https://github.com/nakatamaho/vaeg/commit/1b7e6cbc6defabf240d4c5f3a9a4572021305b36)
completes with **exit 0**. Recording, native Windows MP4 and librashader are ON;
bundled local encoders are OFF. Static encoder and Windows-system-DLL import
audits PASS. Display is `Rel.20261105 (m143, 1b7e6cbc)`; `vaeg.exe` SHA-256 is
`fb952336ffa06a56d12d6512c406cf08d25754cf1e66894349d1ec29260c3c31`.
The [follow-up report](../reports/m143_runtime_integration_qa.md#standard-macos-mingw-route-correction-and-completed-handoff)
records commands, tools, checks and evidence reuse. The earlier container
failure remains preserved; it is not a missing-toolchain diagnosis.

This closes the clean MinGW handoff blocker, not Windows execution or visual
reference approval. Main remains unmerged, with no new release or cleanup.
