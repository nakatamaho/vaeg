<!-- Copyright (c) 2026 Nakata Maho. SPDX-License-Identifier: BSD-2-Clause -->
# OS-assisted recording resume audit

## Current outcome: synchronized; OSR implementation still pending

The blocker recorded below is historical and has now been resolved with explicit
maintainer authorization. Dedicated branch: `topic/m100os-recording-resume`.

- GUI-only preservation commit: `d327cb73a15adcef8fb3ed20ac33b065583e1830`.
- Authorized main CP/MVA content adoption: `f0a2d005fe1ef0d5cd85b70fc272c8ddde8beac0`.
- Normal integration merge / implementation_base:
  `d0de86910baaf9969f231954491e75676e3cf9a2`.
- Baseline build correction / evaluated HEAD:
  `20e1eea4b158a5f7bcc5dae7956f783764e327cb`.
- Pinned integration remains `6c9fb34b5be21051740be3b7d120e8673097d265`;
  ancestor check returned 0. No additional fetch or remote publication.

Merge conflicts were resolved in `CMakeLists.txt` (retain both recording and
8087 tests), `machine/pccore.c` (retain guest recording clock reset and main RAM
reset), `sdl2/gui/gui.cpp` (both tape and recording dialog input guards), and
`sdl2/np2.c` (retain recording startup, with tape setup in the successful startup
branch without duplicating initialization). Existing recording code was retained.

The three original unstaged source/test changes remain unstaged and were not
included in these commits: Metal bridge, Native session, Native session test.
Thus the current baseline is the evaluated HEAD **plus those preserved edits**,
not a clean-commit runtime claim.

Baseline commands/results:

- First ON/OFF builds failed because cached MacOSX26.5 SDK paths no longer
  supplied standard headers. Reconfigured each build with
  `cmake -S . -B build/macos-rec13-{on,off}` and
  `-DCMAKE_OSX_SYSROOT="$(xcrun --show-sdk-path)"`; no global SDK change.
- Subsequent builds exposed missing `upd8087/upd8087.h` in recording test
  targets after main's pccore header change. Added the cpu include directory
  only to CLI and Native session test targets, in the separate correction.
- `cmake --build build/macos-rec13-on -j 6`: exit 0.
- `cmake --build build/macos-rec13-off -j 6`: exit 0.
- `ctest --test-dir build/macos-rec13-on -L recording --output-on-failure`:
  exit 0, 16/16 PASS.
- `ctest --test-dir build/macos-rec13-off -L recording --output-on-failure`:
  exit 0, 11/11 PASS.
- `git diff --check`: exit 0.

No OSR host-time coordinator, hardware H.264 provider, decorated-window capture,
performance measurement or manual/release validation has yet been implemented
or proven by these results. Continue from this integrated tree, re-read updated
instructions, then establish the feature-local implementation plan. Do not repeat
synchronization or replay REC00--REC15. Below is the preserved initial audit and
resolution history; its BLOCKED/next-action statements are superseded here.

## Initial synchronization: BLOCKED before production edits

- `starting_head`: `eb5517e94dc1fcfc012bce4d84f6ed18ce49c5f6`
- Starting/current branch: `topic/m100r3-recording-clock`.
- Authoritative target: maintained fork `origin`, `refs/heads/main`.
  Root AGENTS identifies the original upstream as abandoned; local main tracks
  origin/main. The recording topic's older tracking branch is not integration.
- Fetch command: `git fetch origin refs/heads/main`; exit 0.
- Pinned `integration_tip`: `6c9fb34b5be21051740be3b7d120e8673097d265`.
- `implementation_base`: NOT_ESTABLISHED; integration did not occur.
- Source inspected: starting HEAD **plus existing unstaged changes**, not a
  synchronized or clean evaluated implementation.
- Merge base: `1544a1b8e19cc324c203a94c4e055361abb57d8b`.
- Divergence: `git rev-list --left-right --count HEAD...origin/main` = `35 409`.
- Index initially empty; no nonignored untracked files; no merge/rebase in progress.
- No branch switch, merge, reset, stash, commitment of user edits, or push.
  Fetch updated origin/main only; local main and the topic HEAD remain unchanged.

Git's installed `git help -m merge` documentation warns that merging with
nontrivial uncommitted changes can make original changes unreconstructable.
Incoming changes overlap the following six dirty paths (comparison from the
merge base to the pinned integration tip):

- `docs/cpmva-setup.md`
- `sdl2/gui/gui.cpp`
- `tools/cpmva/README.md`
- `tools/cpmva/install_cpmva.py`
- `tools/cpmva/sources.lock.json`
- `tools/cpmva/tests/test_install_cpmva.py`

Additional dirty paths, also preserved:

- `sdl2/librashader/backends/metal_bridge.mm`
- `sdl2/recording/recording_native_session.cpp`
- `tests/frontend/recording/test_recording_native_session.cpp`

The existing session changes include audio arming/snapshot validation, a
one-sample origin adjustment and checked wide arithmetic. Existing tests change
with that implementation. The GUI changes localize/reposition the recording
menu. These are actual local progress, not task-owned changes to absorb into a
new commit. No private assets or ignored build trees were inventoried.

## Actual reuse map (pre-integration; re-audit after merge)

- `sdl2/recording/recording_native_session.h` exposes
  `vaeg_recording_session_start`, Native Start/Stop/poll and epoch controls.
  The live archive implementation is already present; do not reinstall seeds
  or replace it.
- `recording_native_composition.c`, `recording_osd_snapshot.c` and
  `recording_native_tap.c` supply owned Native raster/OSD and guest boundaries.
- `recording_displayed_capture.h` supplies bounded borrowed RGBA32/top-down
  descriptors, validation and RGB24 normalization. Existing REC15 source and
  reports describe final SDL client readback; it is not decorated capture or
  a connected Displayed MKV recorder.
- `recording_ffmpeg_video_encoder.cpp` opens FFV1 with BGR0, GOP=1 and no
  B-frames. `recording_ffmpeg_matroska.cpp` validates that FFV1 configuration
  and explicitly writes FFV1/PCM stream IDs. Its seekable exclusive-file I/O
  can be reused, but H.264 requires an explicit codec/metadata contract rather
  than merely choosing another encoder name.
- `recording_native_session.cpp` uses `session_.wait_submit`; this intentional
  archive backpressure is not a real-time latest-frame policy.
- `sdl2/soundmng.c` has an archive-active playback branch and SDL mixing.
  A complete playback-clock/gain/device-lead audit is still required; existing
  guest sample accounting does not establish a shared monotonic host clock.
- Search of sdl2, CMake/cmake and recording tests found no ScreenCaptureKit,
  Windows.Graphics.Capture, PipeWire or named VideoToolbox/NVENC/VA-API H.264
  implementations. Missing platform implementation is distinct from runtime
  unavailability.

Existing REC00--REC15 IDs and historical evidence remain untouched. Their PASS
entries are historical, not results for this dirty checkout or the integration
candidate. No current build/test, provider/device probe, benchmark, decoded
H.264, decorated capture or human gate PASS is claimed. Existing builds were
not treated as current evidence. OSR00 is partial/BLOCKED; OSR01--OSR15 have
not been executed. Neither IMPLEMENTATION_COMPLETE nor RELEASE_VALIDATED.

## Recording-specific authority for the resumed task

The current user authorization permits sequential small recording milestones,
real-time hardware H.264 and real OS window decoration capture, and safe normal
integration on a dedicated task branch. Publication is LOCAL_ONLY. It supersedes
older recording-only FFV1/no-OS-capture restrictions and routine intermediate
approval stops, but not technical prerequisites, preservation, actual consent,
licensing or pending final manual validation. Global instructions and unrelated
workflow remain unchanged. No old REC milestone is renumbered or marked passed.

## Subsequent maintainer resolution

The maintainer explicitly selected main's contents for the four dirty files
under `tools/cpmva/` listed above and rejected the local ZEX documentation
change in `docs/cpmva-setup.md`. These five worktree files were replaced using
`git restore --source=6c9fb34b5be21051740be3b7d120e8673097d265 --worktree --`
with an explicit path list. `git diff --quiet` against that SHA for all five
paths returned 0. This is authorized content replacement, not integration:
HEAD and index remain unchanged, and the files still differ from the old topic
HEAD. The GUI localization/repositioning and three other local implementation
changes remain preserved. No merge or commit has occurred. The GUI's unstaged
incoming overlap still needs a preservation resolution before integration.

## Exact next action / smallest required resolution

The owner must preserve and resolve the six overlapping unstaged paths above
(for example, review and commit that existing work themselves). This task cannot
automatically commit, stash, discard or overwrite it. The three other dirty
paths need not be discarded; reassess whether a merge can safely preserve them.
Then create a dedicated local recording-resume branch from the preserved topic
history, inspect and normally merge the **pinned** integration SHA, retaining
both histories. Do not reset main, rebase existing commits, chase another tip,
or silently use a different checkout. Re-read resulting instructions/APIs and
run affected OFF/ON baselines before behavior edits. Establish implementation_base
only after successful integration. Allocate unused legal M IDs against that
resulting roadmap, extend the existing reporting layout, and begin locally
runnable host-time/audio and hardware-provider work. Real platform/manual tests
remain pending separately, not routine approval blockers.
