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

IN_PROGRESS; checkpoint 1 implemented, checkpoints 2 and 3 NOT_STARTED. No new
human gate is claimed. Stop before beginning another milestone.
At final completion require actual macOS Displayed FFV1 playback and existing Native
regression acceptance. Windows/Linux hardware availability and realtime audio are
separate runtime gates, never inferred from an API name or a cross-build.

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
