<!-- Copyright (c) 2026 Nakata Maho. SPDX-License-Identifier: BSD-2-Clause -->
# M100s9 — Presenter-owned Displayed readback

Requested follow-up: independently selectable Native/Displayed and FFV1/hardware
H.264, implemented macOS first, Windows/MinGW next, then supported Linux devices.
This task is the first, archive-side prerequisite, independent of the unimplemented
M100s3 host audio mapping. It does not enable realtime video without audio mapping.

## Scope and checkpoints

1. Connect the existing native presenter's final-client readback to the bounded
   owned Displayed callback. Execute the live renderer/shader only once; include
   its final in-client GUI. Reuse the recording allocation, and share one readback
   with a simultaneous screenshot. With recording off, add no allocations/readback.
2. Connect owned Displayed pixels to the guest-time FFV1/PCM session, including
   lifecycle/geometry/reset handling and fail-closed batch behavior.
3. Enable Displayed selection only after complete-path verification. Hardware
   compression selection remains disabled until M100s3/M100s4 provide synchronized
   host-time audio/video. Never substitute Native or a CPU encoder.

Checkpoint 1 alone is not a working Displayed movie feature. Keep that UI disabled
until checkpoints 2 and 3 pass. ScreenCaptureKit/decorated capture is M100s5, not
this actual client-image path. Keep the pre-existing Metal GUI texture update
separate from the new readback changes until independently verified.

## Required local checks

- Debug recording ON/OFF builds and applicable frontend/recording tests.
- Exact public RGBA/RGB callback bytes, immutable ownership through callback,
  duplicate-token suppression, recording-off no-op, geometry/presenter-generation
  changes and readback/callback failures with exact capture error enums.
- Final native readback occurs after the real GUI composition, not a second shader
  evaluation. A mock/helper/software fixture proves contracts, not physical Metal
  output parity; actual Metal capture/preset/GUI parity remains a runtime check.
- Focused strict warnings/sanitizer, repository encoding/EOL/case/whitespace checks.
- Before the final human gate: clean static MinGW handoff, public media and bundled
  demo regression as required by AGENTS.md; record exact commands, commits, status
  and image hashes. No private-input check may be invented or replaced by mocks.

## Gate and current status

IMPLEMENTED: checkpoints 1–3 now have local macOS archive-path verification.
Static macOS Release and clean MinGW compile/import checks pass again after the
latest-main integration below. Media/runtime prerequisites and the human gate remain open; this is not
release approval. Stop before beginning another milestone.
At final completion require actual macOS Displayed FFV1 playback and existing Native
regression acceptance. Windows/Linux hardware availability and realtime audio are
separate runtime gates, never inferred from an API name or a cross-build.

## Recording source preference follow-up

The maintainer requested that Displayed video remain selected after restarting.
The GUI choice now persists as host-only `RecordingSource` in `vaeg.cfg`:
0 = Native, 1 = Displayed. Explicit radio-button changes update the preference;
GUI initialization (including renderer reinitialization) restores it. CLI source
overrides do not change this preference, and restoring it never starts recording.
A saved unavailable Displayed choice is retained with its unavailable message,
not silently replaced. Existing/missing-key configurations default to Native and
invalid values normalize to Native. `--no-cfg` still suppresses file persistence.
No guest state/save-state format or source/compression contract changes.

Tests extend the production configuration save/load selftest for both choices,
invalid values and missing keys. GUI controls check that changing the preference
updates host settings without arming recording. ON/OFF frontend/recording and
isolated full selftests are rerun; final artifact identities follow separately.

## Latest-main rebuild requested by the maintainer

Main [f25cda38](https://github.com/nakatamaho/vaeg/commit/f25cda385965d765441db9d6498fd40beae0a285)
is imported into this recording branch at
[bc0198b9](https://github.com/nakatamaho/vaeg/commit/bc0198b9dbdd919d1046bd94081812affd18194c).
This is main-to-topic integration, not a follow-up merge into main or a passed
recording gate. The ROADMAP conflict preserves both main's G100s2 wording and this
M100s9 row. All M125 quick-save/load, F12/Shift+F12, OSD and selftest changes remain;
all recording controls and the Metal correction remain too. Quick load and undo
use `state_load()` / `statsave_load()`, preserving the existing recording epoch
cancellation hook. No save-state format or core behavior was separately changed.

Re-evaluated source is bc0198b9, not the earlier 73948eaa artifacts below:

- ON frontend/recording: 37 PASS + two opt-in SKIP; OFF: 29 PASS. Actual opted-in
  Metal GUI/font/filter/final-image FFV1 RGB/opaque-alpha/PCM comparison PASS.
- Focused ASan/UBSan 4 PASS; recording strict compile variants 10 PASS. Extra
  merged GUI/taskmng/selftest/ini strict checks pass; kbdmap's optional strict check
  fails an inherited `-Wsometimes-uninitialized` selftest-cleanup warning. Its
  source is byte-identical to imported main; no blanket Werror PASS is claimed.
- Full Debug ON/OFF ROM-less selftests pass with distinct TMPDIRs, including
  quick-save rotation/binding. Release full selftest passes. Initial parallel
  ON/OFF full selftests collided over the legacy fixed dropmedia temporary
  directory; failed logs are preserved. Isolated-temp reruns pass without source
  changes. These are not private-media or guest boot tests.
- macOS Release rebuilt and both public Native/Displayed four-frame CLI captures
  pass independent FFV1/PCM/source-tag checks under frameskip 4. Apple-only imports
  and embedded bc0198b9 identity pass; deployment/alignment warnings still remain.
- The required helper builds OFF from a new clean pushed-source clone, exit 0.
  Its task-local Docker adapter replaces only the temporary shader-SDK build step
  with independently hash-verified cache staging. Pin/Rust/target/feature/lock/
  archive/audit/builder identities are checked and every staged byte rechecked in
  the container. No application build tree or object is reused. A separate fresh
  recording ON tree builds and passes static PE imports/commit/date checks, exit 0.
  This reuses unchanged expensive dependency compilation, not emulator behavior.

Current macOS bytes 38,753,240, SHA-256
`2b3e68225d0ed111c94307f20e0ad84b74517ec812e0b0b364b3db2261a63783`.
Current MinGW ON bytes 58,917,027, SHA-256
`73b2011024c92bd0923fea3c8e940390de93456d99208195b1204f73d02e9a3e`.
Absolute executable/report paths are in the [handoff](../../recording-v1/handoff.md#active-m100s9-follow-up).
Evidence index `build/m100s9-main-refresh/validation.json`, SHA-256
`269650d4099ff8904b6018d6c00e0ce7f62202b8094b36c692e2042cedd71f42`,
retains exact command records, source/worker/corpus/contract/policy/SDK/output
identities, successful checks and failed exploratory checks. Original artifacts
and evidence remain retained. Checkout-local Colima is stopped, not removed.
Media/PSD/DOSBox prerequisites, media/demo/OS/playback human acceptance, native
Windows/Linux/older-macOS and legal/distribution gates remain open. H.264 realtime
remains disabled. No archived reference-tier source or provenance was changed.

## Displayed archive implementation

The source is fixed for each session. Displayed uses owned square-pixel RGB24
snapshots of the full client drawable (shader, letterboxing, OSD and GUI), the
existing indexed guest-time PCM FIFO, bounded lossless queues and exclusive
FFV1/PCM/MKV writer. Source metadata is `displayed`, never `native`. Native keeps
its existing capture, GUI auto-splitting, CLI failure and reset-rearm contracts.

The main loop forces a real presentation for every Displayed guest frame even
under draw-skip. The callback converts borrowed RGBA into preallocated owned RGB
and commits the prior held-image interval at that exact guest boundary. Worker
threads never receive borrowed pixels, GPU objects or GUI pointers. Repeated,
gapped or regressed frame boundaries fail closed instead of dropping frames.

Displayed geometry/presenter changes and reset save complete intervals and stop;
the old image is sealed through the change boundary when valid. Arbitrary state
load/rewind cancels, and batch acquisition changes remain errors. Empty recordings
publish no file. Missing drawables/readback failures cannot leave an apparently
active recorder indefinitely. Interactive failures do not terminate the guest.
The screen menu and source/compression controls are locked while Displayed records;
Displayed always includes the actual GUI/OSD, so Native-only OSD/rotation controls
are disabled for that source.

Both sources are selectable in the GUI, subject to current renderer availability;
`--record-source displayed` now works too. Startup presents the actual first GUI
layout before fixing the batch Displayed descriptor, because initial menu/font
metrics can resize the client. No extra offscreen shader evaluation is introduced.
The compression UI explicitly shows FFV1 and a disabled hardware H.264 choice with
the missing host-time audio-mapping reason. It never advertises a working realtime
path based solely on the earlier encoder probe.

### Verification of the complete macOS path

- Debug ON frontend/recording: 37 PASS, two opt-in hardware probes SKIP by default;
  Debug OFF: 29 PASS. Focused ASan/UBSan clock/session/runtime/integration: 4 PASS
  (`detect_leaks=0`, `halt_on_error=1`); not a whole-guest sanitizer result.
- The extended runtime integration test independently decodes colored/asymmetric,
  padded-pitch public Displayed snapshots and exact stereo PCM. It checks source
  tags, opaque alpha, ownership after overwriting the callback buffer, guest-time
  limits, manual stop, reset, state load, interactive geometry save, batch geometry
  failure, readback failure, token gaps, held-image sealing and empty stop.
- Nine changed first-party translation units pass replayed ON compile commands
  with `-Werror`; repository encoding/EOL/case/Markdown-comment/whitespace checks
  are required again before publication.
- The opt-in actual Metal probe passes filtered/unfiltered final GUI rectangle and
  first-frame font text, one presentation/readback per image, and exact FFV1 RGB/
  opaque-alpha/PCM decode of both public movies. It also launches the real frontend
  with an intentionally empty ROM directory: Native and Displayed each produce
  four complete intervals with `--frameskip 4`, the correct source tag, FFV1/PCM
  streams and actual Metal selection. Displayed includes the client menu. This
  public subprocess is test-only; production recording uses library APIs only.
  Empty ROMs prove frontend startup/draw-skip/recording, not guest boot or a ROM set.

Exact command: isolated HOME, `VAEG_RUN_METAL_ARCHIVE_PROBE=1`,
`build/macos-rec13-on/vaeg_recording_metal_archive_probe`. It emits
`METAL_ARCHIVE_FINAL_GUI_FFV1_PCM_PARITY_PASS` and
`METAL_FRONTEND_NATIVE_DISPLAYED_STARTUP_DRAW_SKIP_PASS`. The normal CTest entry
skips unless explicitly opted in; a skip is never promoted to runtime PASS.
The complete runtime was exercised separately with the opt-in environment.

Raw local logs, commands and worker/source digests are retained under
`build/m100s9-completion/`; private scratch remains outside Git. An earlier manual
launch referenced unavailable optional integration inputs and emitted missing-ROM
warnings: it is only frontend evidence, not a boot/demo/OS or private-game PASS.
Actual guest/media/PSD inputs and the final media regeneration/guest-runtime cycle
remain missing gate prerequisites. The clean MinGW handoff does not establish
physical Windows/Linux/older-macOS runtime evidence; those platform gates remain
separate. No new human gate passed.

## Clean-build and cross-host audit recovery

Evaluated feature commit:
[73948eaa](https://github.com/nakatamaho/vaeg/commit/73948eaa511d3a97896155a1d05af864e818b535).
The required clean helper was invoked with that full SHA. Its new clone compiled
recording OFF successfully, but the helper exited 1 in its import auditor: native
AArch64 Ubuntu `objdump` did not recognize the valid x86-64 PE. The exact traceback
is retained in the handoff's original `output/import-audit.txt`; compilation was
not rerun as if it had failed.

`check-static-windows-imports.py` now prefers the installed target-prefixed MinGW
inspector, retaining native `objdump` as the fallback when it is absent. This is a
build-tool portability correction, not an emulator change or an import allow-list
bypass. Run `python3 tests/frontend/librashader/test_static_windows_imports.py`:
both selector unit tests pass. The completed OFF executable passes the corrected
inspector on the host and inside the same pinned Ubuntu container.

Recording ON was then configured and compiled in a separate new build directory
of that clean source clone. A first staging attempt correctly failed
`VAEG_FFMPEG_STATIC_ARCHIVE_MISSING` because its sysroot was outside the toolchain's
search root; its configure log is retained. The final fresh build installs the
validated cached target FFmpeg archives at the standard MinGW target prefix and
passes compile, embedded commit/date and system-DLL-only PE audits (exit 0).
The successfully built pinned Rust archive was reused without rebuilding it.
There was no shared-library or codec fallback. This is cross-build evidence,
not native Windows movie/runtime proof.

macOS Release source 73948eaa also builds and imports only Apple system libraries.
Its real frontend records four public ROM-less frames for each source under
frameskip 4: Native 640x400 and Displayed 640x436 with menu, FFV1/PCM and matching
source tags. `ffprobe` independently confirms both outputs. The dependency
macOS-version/alignment linker warnings remain, so older-macOS support is not
established. The prior accepted macOS executable is retained, not discarded.
No new human or distribution gate is claimed.

## Initial 73948eaa handoff and evidence identities (historical)

- macOS Release, evaluated feature 73948eaa: 38,735,528 bytes;
  SHA-256 `82a93cdddef46d112f52f54e2d976b0ad44e191580b9c8f99bbc6c3313fcab60`.
- Clean MinGW recording ON: 58,904,869 bytes;
  SHA-256 `680d0a309890fa880da0a9e6b97dcffb58e6ee7a99fed4deda24c805c5e68c06`.
- OFF artifact retained: SHA-256
  `3a2fffd26b6c5d25797c74414005ac50b512bdb7105e842a878aaaa6b7a7188c`.

Absolute executable/report paths are in the [current handoff](../../recording-v1/handoff.md#active-m100s9-follow-up).
Local `build/m100s9-completion/validation.json` SHA-256:
`690fa960f6e7571591354e126c8905f66c50e44e8b141d9ed40af8b894f2747c`.
It retains evaluated commits, worker/source/dependency digests, public corpus,
comparison contract and target-policy identities, exact commands, exit statuses
and output hashes. The paired corrected Metal worker SHA-256 is
`1a57ea06c0ab2a559c0df45207fc87a18976f4d66e257e349336c6a97f46af3a`;
its baseline-object counterpart is
`d051cfa1433451a92a6e002fca10a7afbb85b4e523c7502d1694dfc47b19d7e9`.
Paired result: corrected exit 0; baseline SIGABRT (-6, shell 134), exact missing
WantCreate/GetTexID assertion. The completed positive run is retained/reused,
not discarded because a report or tool changed.

Before the latest-main refresh, evaluated frontend/core/CMake/dependency inputs
were unchanged from 73948eaa through dd3d488d;
the subsequent PE selector/tests and documentation do not justify rerunning the
completed emulator/GPU runs or Rust compilation. No hosted CI was requested.
Repository encoding/EOL/case/Markdown-comment and diff checks pass with zero
findings; their final output is retained beside the evidence index.

No utility/SASI/demo distribution images were produced in this checkpoint:
required private source media and PSD inputs are not available within the task's
approved inputs, and `command -v dosbox` fails on this host. Full clean-clone media
regeneration and guest launch/playback are **NOT_RUN/BLOCKED**, not passed based on
unchanged media sources or existing unit fixtures. V3/demo/OS/manual recording
acceptance is still required. Dedicated checkout-local Colima m100s9 is stopped;
all handoff/evidence artifacts are retained and no unrelated environments, private
inputs or branches were deleted. Do not begin M100s3/M100s4 or another milestone.

## Metal font-atlas correction

The inherited Metal texture-update change was preserved byte-for-byte and tested
with the new public opt-in Metal/archive probe. A baseline worker substituted only
the original `cd4fd8d6` Metal bridge object, keeping the same test and dependencies.
It aborted with exit 134 at `ImDrawCmd::GetTexID`: a texture requested during
`ImGui::NewFrame()` had not been uploaded before command texture resolution.
The corrected bridge handles pending textures after `Render()` has produced draw
data, before resolving draw commands. The corrected worker exits 0, including
first-frame text, a final-client GUI rectangle, filtered/unfiltered output, and
independent exact RGB/opaque-alpha/PCM decoding of both FFV1 movies. No native
screen recording permission, private ROM, media or screenshot was used.

Runtime command: isolated HOME, `VAEG_RUN_METAL_ARCHIVE_PROBE=1`, then
`build/macos-rec13-on/vaeg_recording_metal_archive_probe`. Evidence and exact A/B
compile/link commands are retained in `build/m100s9-completion/`. The standalone
probe validates the real Metal presenter and FFV1 session with public pixels and
synthetic audio; it is not a guest playback, physical audio-latency or Windows test.

## Checkpoint 1 local verification

Evaluated changes: `sdl2/scrnmng.c` and the capture header comment, on base
`df10f47706e4e842e2e89ac287e7ec39ae6e15be`. The implementation commit is recorded
in the following status/handoff evidence update. No emulator core, guest media,
FFV1 session, selection UI, or hardware encoder was changed.

Commands and results (all exit 0):

```sh
cmake --build build/macos-rec13-on -j 4
ctest --test-dir build/macos-rec13-on --output-on-failure -L 'frontend|recording'
cmake --build build/macos-rec13-off -j 4
ctest --test-dir build/macos-rec13-off --output-on-failure -L 'frontend|recording'
cmake --build build/macos-rec13-sanitize -j 4
python3 build/m100s9-evidence/strict_compile.py
python3 tools/repo/check_encoding.py
python3 tools/repo/check_eol.py
python3 tools/repo/check_case.py
git diff --check
```

ON: 37 PASS, one existing opt-in hardware-probe SKIP. OFF: 29 PASS. The focused
capture selftest was also run in each build, with the evidence directory as cwd,
an isolated HOME, `SDL_VIDEODRIVER=dummy`, `SDL_AUDIODRIVER=dummy`, and
`VAEG_SELFTEST_DISPLAY_CAPTURE_ONLY=1`, invoking that build's `sdl2/vaeg --selftest`.
All three passed; the sanitizer run additionally used
`ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1`.
The sanitizer build has both address and undefined sanitizers enabled; this is
not a whole-guest sanitizer result or a leak check. The strict script replays the
actual ON `scrnmng.c` compile command with `-Werror`, changing only its output path.

The fixture checks exact public final-client RGB bytes and capture metadata,
reusable owned output, recording-off no-op and duplicate-token suppression.
Each geometry, generation, backend, incomplete-readback and callback-rejection
negative test first passes its fixture, then applies one mutation and asserts
its exact capture error enum. A callback that copies pixels and stops capture
also passes without post-callback access to released storage.

Evidence is retained out of Git under `build/m100s9-evidence/`: command logs,
strict command JSON, source/worker/log digests in `validation.json`. ON builds
include the unchanged pre-existing Metal GUI texture patch; that patch remains
uncommitted and is not attributed to this checkpoint. OFF/sanitizer builds have
librashader disabled. Existing development dependency deployment-target warnings
remain; these builds are not release artifacts. Actual Metal/preset/GUI parity,
Displayed FFV1 movies, MinGW, Linux, media regeneration and final clean-build
handoff have NOT been verified here. The task and its human gate remain open.
