# REC14 report: minimal recording CLI and Native GUI

Status: IMPLEMENTED_UNVERIFIED

Feature ID and mapped legal M ID: REC14 / M100r15
Assigned branch: `topic/m100r3-recording-clock`
Base SHA: `39735d1b33a7b0947d36538f0d7f448c07d74717`
Evaluated source SHA: `ec966b9b61d487014aabd8cc3184bc3ef0c55a06`
Evaluated binary/toolchain identity: macOS arm64, AppleClang, CMake,
MacPorts SDL2, recording-enabled and recording-disabled macOS build trees
Evidence-only SHA, if applicable: Documentation commit immediately following
the evaluated source commit
Task/spec version: `docs/recording-v1/milestones/rec14-native-ui.md`

No private ROM, media, absolute path, hash, screenshot or raw log is included
here.

## Scope and changes

REC14 adds the first public recording controls while keeping the REC13 Native
session as the single implementation used by both frontend entry points.

Changed files:

* `sdl2/cliopts.c/.h` parse `--record FILE.mkv`,
  `--record-source native|displayed` and positive `--record-frames N`.
  Native is the default; explicit orphan recording options are rejected.
* `sdl2/np2.c` documents the options, rejects unavailable/off-build and
  unavailable Displayed requests before the SDL/ROM session, starts the common
  session at startup, and polls a requested stop after presentation and during
  host-only paused redraws.
* `sdl2/gui/gui.cpp` adds the Recording menu and source/save dialog with the
  exact `Native video` and `Displayed video` labels. Displayed is disabled with
  its reason. Active recording exposes Stop Recording, guest elapsed time,
  accepted interval count and finalizing/saved/error feedback.
* `sdl2/recording/recording_native_session.cpp/.h` adds a frontend-neutral
  source-aware start wrapper, destination validation, manual-stop polling and
  status counters without exposing FFmpeg types across the C interface.
* `sdl2/recording/recording.c` reports the optional FFmpeg-backed recording
  capability as available only in a recording-enabled build.
* `CMakeLists.txt` and `tests/frontend/recording/test_recording_cli.c` add
  focused ON/OFF CLI parsing coverage.
* `tests/frontend/recording/test_recording_native_session.cpp` covers source
  rejection, invalid/existing destinations, double start, Unicode paths,
  finite intervals and complete manual stop.

The frozen product scope is unchanged: Native and Displayed are the only two
user-facing sources; both include enabled guest-area emulator OSD; Native
remains a recording-owned pre-CRT guest raster; Displayed remains the final
client composition and is not falsely implemented by this milestone. Existing
guest-only screenshots, QA captures, input/menu behavior, C99 core boundaries,
bounded A/V buffers and recording-OFF behavior remain unchanged.

## Implementation findings and decisions

The CLI parses and validates the output suffix before SDL initialization. The
startup path checks the recording capability and source, then calls
`vaeg_recording_session_start` after the machine reset. The GUI calls the same
function, so it does not have a parallel writer or timing path.

The session validates local `.mkv` destinations, rejects existing final
targets and unavailable parent directories before starting audio capture or
the Native tap. Displayed requests return the explicit
`displayed_source_unavailable` result and do not create a file. A manual stop
is only acted on by the host/frontend polling point; it does not add work to
the audio callback or guest/core lock. Complete intervals are still finalized
through REC13's bounded queue and writer.

Stop feedback uses the same guest clock and accepted interval counter as the
session. A successful stop with an empty error string is treated as success by
the GUI; a worker or interval failure remains visible as an error.

## Checks

| Check ID | Exact command/environment | Exit code / outcome | Evidence |
|---|---|---|---|
| REC14-BUILD-ON | `cmake --build build/macos-rec13-on -j4` | 0 / pass | Recording-enabled frontend and tests linked |
| REC14-BUILD-OFF | `cmake --build build/macos-rec13-off -j4` | 0 / pass | Recording-disabled frontend and tests linked |
| REC14-TEST-ON | `ctest --test-dir build/macos-rec13-on --output-on-failure -L recording` | 0 / 15 of 15 pass | CLI, session and all prior recording suites |
| REC14-TEST-OFF | `ctest --test-dir build/macos-rec13-off --output-on-failure -L recording` | 0 / 10 of 10 pass | OFF contract and regression suites |
| REC14-SELFTEST-ON/OFF | `SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy .../vaeg --selftest --no-cfg --no-bkupmem` | 0 / both pass | ROM-less frontend selftests |
| REC14-SMOKE-ON/OFF | `SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy .../vaeg --smoke --no-cfg --no-bkupmem` | 0 / both pass | ROM-less smoke path |
| REC14-CLI | enabled `--help`; OFF `--record`; ON `--record --record-source displayed` | 0 for help; expected 1 for unavailable requests | Help lists the new options; failures occur before session/output creation |
| REC14-REPO | milestone ID, case, UTF-8, EOL, whitespace checks | 0 / pass | No findings or violations |

The selftest and smoke paths use the two existing build-tree identities; the
report intentionally omits their absolute paths. The CLI parser and Native
session tests are deterministic fixtures. A real startup recording with a
guest OS/media workload was not run in this environment.

## Integrity results

The session fixture confirms no output is created for an invalid extension,
existing target or Displayed request. It confirms a complete manual stop
publishes a movie after one closed synthetic interval, while a finite limit
stops after the specified complete interval count. The existing REC13 tests
continue to cover exact guest-time/sample accounting, FFV1/PCM stream format,
bounded queue behavior and partial-file publication. No new frame/sample
counts are inferred from a GUI-only run.

## Review and limitations

The new controls stay in the SDL2 frontend and C-compatible session header;
FFmpeg types remain private to the C++ backend. No audio callback, guest clock,
GPU presenter or screenshot/QA path was changed. Destination validation is
fail-closed and the active session remains bounded by the existing REC03/REC13
queue.

The actual human requirement—start and stop a Native movie while a guest
workload displays enabled OSD and produces sound—remains pending. Displayed
capture, SDL final-client readback, Windows/Linux runtime, VLC playback and
release validation are also not claimed here.

## Gate and next task

Machine gate: PASS for the implemented REC14 scope on the evaluated macOS
builds.
Human gate: PENDING; no maintainer acceptance is claimed.
Push/remote state under repository policy: no push performed; local branch only.
Next dependency-ready REC task and host requirement: REC15/M100r16, a host
with real SDL renderer output for the final-client readback seam.
