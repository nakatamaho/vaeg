# REC15 report: final-client capture contract and SDL readback

Status: IMPLEMENTED_UNVERIFIED

Feature ID and mapped legal M ID: REC15 / M100r16
Assigned branch: `topic/m100r3-recording-clock`
Base SHA: `5fcdbc23` (REC14 documentation checkpoint)
Evaluated source SHA: `84c1b85a`
Evaluated binary/toolchain identity: macOS arm64, AppleClang, CMake, MacPorts
SDL2; recording-enabled and recording-disabled build trees
Evidence-only SHA, if applicable: Documentation commit immediately following
the evaluated source commit
Task/spec version: `docs/recording-v1/milestones/rec15-displayed-contract.md`

No private ROM, media, absolute path, hash, screenshot or raw log is included
here.

## Scope and changes

REC15 adds the frontend-owned contract and SDL acquisition primitive needed by
the later Displayed recording session. It does not connect a Displayed frame
to the MKV writer.

Changed files:

* `sdl2/recording/recording_displayed_capture.h/.c` define and validate a
  borrowed top-down SDL RGBA32 frame, bounded byte ownership and RGB24
  normalization. The callback must copy before returning.
* `sdl2/scrnmng.h/.c` expose the capture lifecycle and error state, allocate
  one reusable readback buffer only while active, record the SDL renderer
  generation and drawable descriptor, and read the final target immediately
  before `SDL_RenderPresent`.
* `CMakeLists.txt` includes the contract implementation and focused test.
* `tests/frontend/recording/test_recording_displayed_capture.c` covers source
  pitch/size/origin validation and exact RGBA32-to-RGB24 conversion including
  row padding.
* `scrnmng_display_capture_selftest` contains a labelled fixture-only SDL
  render target with distinct guest, menu, OSD and status regions. It checks
  normalized bytes against an independently populated expectation, one frame
  token, paused UI-only redraw suppression, geometry stop and dummy rejection.

The frozen scope remains unchanged: Native and Displayed are the only sources;
both include enabled guest-area emulator OSD; Native remains the canonical
pre-CRT recording-owned raster; existing guest-only screenshots and QA captures
remain on their original paths; output format, guest-time accounting, C99
boundaries and private-data rules are untouched.

## Implementation findings and decisions

The existing SDL frame order is `sdl2/np2.c`'s
`scrnmng_present_begin()` -> `gui_render()` -> `scrnmng_present_end()`.
`scrnmng_present_end()` first draws the enabled SDL Video information and
Framebuffer information OSD through `scrnmng_draw_sdl_overlays`, then performs
the new Displayed readback, then calls `SDL_RenderPresent`. Therefore the seam
observes the final SDL client composition, including the visible guest,
letterbox/effect, guest-area OSD, menu and in-client GUI, and does not read the
guest texture directly. The existing screenshot helper remains unchanged and
continues to crop its menu strip according to its existing contract.

Admission uses `SDL_GetRendererOutputSize`, so the descriptor is the physical
drawable width and height rather than the requested window size. The reusable
buffer has a top-down `width * 4` pitch and a 256 MiB bound. The source pointer
is valid only during the callback; the normalization helper produces the
recording owner's RGB24 bytes.

The capture stores the `drawcount` as its frame token. A repeated token is a
host/UI-only redraw and does not cause a second readback or a timed recording
frame. A drawable-size change or SDL renderer-generation change fails closed
and releases the owned buffer. Native presenter activation, renderer loss,
readback failure, callback rejection, dummy driver and allocation failures
have explicit error values. When inactive, the added path performs no
readback and allocates nothing.

The fixture arms the internal seam only after constructing an SDL software
render target because the public start function correctly rejects the dummy
driver. It is explicitly a synthetic fixture, not Displayed platform
acceptance.

## Checks

| Check ID | Exact command/environment | Exit code / outcome | Evidence |
|---|---|---|---|
| REC15-BUILD-ON | `cmake --build build/macos-rec13-on -j4 --target vaeg_sdl2` | 0 / pass | Recording-enabled frontend links with the new seam |
| REC15-BUILD-OFF | `cmake --build build/macos-rec13-off -j4 --target vaeg_sdl2` | 0 / pass | Recording-disabled frontend links with the same SDL contract |
| REC15-CONTRACT-ON | `cmake --build build/macos-rec13-on -j4 --target vaeg_recording_displayed_capture_test` and focused CTest | 0 / pass | Exact normalization and negative contract checks |
| REC15-CONTRACT-OFF | `cmake --build build/macos-rec13-off -j4 --target vaeg_recording_displayed_capture_test` and focused CTest | 0 / pass | Contract is independent of FFmpeg/recording build option |
| REC15-REGRESSION-ON | `ctest --test-dir build/macos-rec13-on --output-on-failure -R 'vaeg_recording_displayed_capture|vaeg_recording_(c_contracts|osd_snapshot|native_composition|native_tap|clock|queue|native_session)'` | 0 / 8 of 8 pass | New readback contract plus affected recording suites |
| REC15-REGRESSION-OFF | `ctest --test-dir build/macos-rec13-off --output-on-failure -R 'vaeg_recording_displayed_capture|vaeg_recording_(c_contracts|osd_snapshot|native_composition|native_tap|clock|queue)'` | 0 / 7 of 7 pass | Recording-OFF and prior contracts |
| REC15-WERROR | `cmake --build build/macos-rec13-werror -j4 --target vaeg_recording_displayed_capture_test`; focused CTest | 0 / pass | New and test C units compile with Werror |
| REC15-SANITIZER | `cmake -S . -B build/macos-rec13-sanitize`; build the focused target; focused CTest | 0 / pass | ASan/UBSan contract fixture |
| REC15-FIXTURE | `SDL_VIDEODRIVER=dummy build/macos-rec13-on/sdl2/vaeg --selftest --no-cfg --no-bkupmem` | 0 / pass | Synthetic SDL target, token and fixed-descriptor checks; dummy status is explicit |
| REC15-REAL-SDL | `build/macos-rec13-on/sdl2/vaeg --selftest --no-cfg --no-bkupmem` without a forced video driver | 1 / unavailable | macOS UI-service connection became invalid before the run completed; no real-host Displayed evidence is claimed |
| REC15-REPO | `git diff --check` plus repository case/UTF-8/EOL and milestone-ID validators | 0 / pass for the evaluated source/docs changes | No source or public-document hygiene findings |

The existing full recording suites were also rerun through the focused ON/OFF
sets above. The Werror full frontend remains affected by the known unrelated
`io/np2sysp.c` unused `str_pentium` warning; the new focused target itself
passed with Werror. No Windows/Linux presenter, GPU backend, live Displayed
MKV, VLC playback, human demonstration or release validation was run.

## Integrity results

The synthetic target is 640x422 with a 640-byte-per-row RGBA32 readback and a
640x422 RGB24 normalized result. The independent expected buffer and callback
result compare equal for all 810,240 RGB bytes. One changed guest token yields
one accepted callback carrying the same token and descriptor generation; a
second final-client redraw with the same token yields no additional frame. A
window geometry change ends the capture with `geometry_changed` and releases
the buffer. No video or audio packet was produced by REC15, so MKV PTS,
sample counts and queue high-water values are not applicable here.

## Review and limitations

The readback allocation and SDL read are on the presentation/render-owner
thread, not the audio callback or guest/core lock. The callback receives only a
borrowed source and is responsible for copying; the current fixture copies to
RGB24 immediately. No second render pass, OS capture API, guest-texture crop or
timed frame source was introduced.

The missing real macOS SDL host run is a concrete evidence gap, not a passing
platform result. REC15 is therefore `IMPLEMENTED_UNVERIFIED`, despite the
contract, focused tests, SDL fixture and build checks passing. REC16 and the
other Displayed integrations remain blocked on a host with usable real SDL
renderer output unless an equivalent authorized platform evidence source is
provided.

## Gate and next task

Machine gate: IMPLEMENTED_UNVERIFIED. The deterministic contract and fixture
pass; the required real SDL renderer runtime was unavailable in this session.
Human gate: NOT_REQUIRED.
Push/remote state under repository policy: no push performed; local branch only.
Next dependency-ready REC task and host requirement: REC16/M100r17 after the
REC15 real-SDL evidence gap is closed; a host with a functioning SDL window and
renderer is required.
