# REC12 report: native guest-boundary video tap

Status: PASS (machine gate for the bounded REC12 tap scope)

Feature ID and mapped legal M ID: REC12 / M100r13

Assigned branch: `topic/m100r3-recording-clock`

Base SHA: `d819946436fe82b6c310f4b9cf8f19dedf82bef7`

Evaluated source SHA: `415ca5f114b27990daa04ce157f4f27616ac98b2`

Task/spec version: `docs/recording-v1/milestones/rec12-native-video-tap.md`

Evaluated environment: macOS arm64, AppleClang 21.0.0.21000101, CMake
3.31.12, Ninja 1.13.2, MacPorts SDL2 2.32.10.

## Scope and changes

* `machine/pccore.c` now distinguishes the actual `screendispva` guest
  boundary from `pccore_redraw`. A Native tap forces the existing canonical
  composition to run when normal host draw skipping would otherwise suppress
  it, but redraws do not become timed capture frames.
* The completed composition is copied from the existing guarded RGB565 shadow
  into recording-owned RGB24 storage using the REC09 conversion and is then
  composed with the REC08 OSD snapshot. The guest buffer is not modified.
* `recording_native_tap.c/.h` define a small C-compatible callback contract.
  Each callback receives the owned frame for the callback lifetime, its copied
  OSD snapshot, a monotonically checked sequence, exact guest-clock interval,
  and descriptor generation. Consumers must copy retained data before return.
* The tap rejects backwards/zero guest intervals, non-monotonic sequences,
  descriptor changes, invalid frame/snapshot identity and callback rejection;
  failures deactivate the tap and remain observable.
* `pccore_guest_clock()` exposes the current CPU guest-clock position in an
  extended monotonic 64-bit domain. The extension state is reset with the
  emulated machine reset and is not serialized into guest state.
* `scrnmng_capture_native_recording_frame_with_snapshot()` keeps the existing
  helper behavior while allowing the boundary producer to retain the exact
  OSD snapshot paired with the composed frame. The Native descriptor identity
  is derived from the canonical coded dimensions, source pitch and contract
  version.
* The new CTest covers interval partitioning, frame/OSD sequence association,
  consumer ownership-by-copy, frame skipping activation, descriptor-change
  rejection, invalid time, and callback failure. It is compiled in both
  recording ON and OFF configurations.

The tap is intentionally not connected to the REC07 writer or a public Start
Recording controller here. That is REC13 scope. No Displayed source, GPU
readback, second CRT pass, screenshot mode, or QA capture behavior was added.

## Guest-boundary data flow

```text
guest screendispva event
    -> drawscreenva(guest_boundary=true)
    -> canonical VA raster composition into existing shadow
    -> drawcount / pccore_guest_clock()
    -> owned RGB24 copy + matching REC08 OSD snapshot
    -> bounded C callback contract for the future coordinator
```

The normal `drawframe == false` path remains cheap when the tap is inactive.
When active, the existing composition is requested for every display event;
the implementation does not execute extra guest instructions and does not use
SDL Present or host wall time as the capture clock. `pccore_redraw()` calls the
same compositor with `guest_boundary=false`, so GUI/configuration redraws do
not add a movie interval.

## Checks

| Check | Exact command/environment | Result |
|---|---|---|
| REC12-TAP-ON | `cmake --build build/macos-rec07-on --target vaeg_recording_native_tap_test vaeg_sdl2 -j2` | Exit 0; frontend and tap test built |
| REC12-TAP-OFF | `cmake --build build/macos-rec07-off --target vaeg_recording_native_tap_test vaeg_sdl2 -j2` | Exit 0; frontend and tap test built |
| REC12-FOCUSED-ON | `ctest --test-dir build/macos-rec07-on --output-on-failure -R '^vaeg_recording_native_tap$'` | Exit 0; 1/1 passed |
| REC12-FOCUSED-OFF | Equivalent command in `build/macos-rec07-off` | Exit 0; 1/1 passed |
| REC12-WERROR | `cmake --build build/macos-rec08-werror --target vaeg_recording_native_tap_test -j2 && ctest --test-dir build/macos-rec08-werror --output-on-failure -R '^vaeg_recording_native_tap$'` | Exit 0; 1/1 passed |
| REC12-WERROR-PRODUCTION | Generated Werror commands for `machine/pccore.c`, `sdl2/scrnmng.c` and `sdl2/recording/recording_native_tap.c` from `ninja -t commands`, piped to `/bin/sh` | Exit 0; changed production C units compiled |
| REC12-SANITIZER | `cmake --build build/macos-rec08-sanitize --target vaeg_recording_native_tap_test -j2 && ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1 ctest --test-dir build/macos-rec08-sanitize --output-on-failure -R '^vaeg_recording_native_tap$'` | Exit 0; 1/1 passed |
| REC12-SUITE-ON | `ctest --test-dir build/macos-rec07-on --output-on-failure -L recording` | Exit 0; 13/13 passed |
| REC12-SUITE-OFF | `cmake --build build/macos-rec07-off --target vaeg_sdl2 vaeg_recording_native_tap_test -j2 && ctest --test-dir build/macos-rec07-off --output-on-failure -L recording` | Exit 0; 9/9 passed |
| REC12-ROMLESS-ON | `SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=invalid build/macos-rec07-on/sdl2/vaeg --selftest --no-cfg --no-bkupmem` | Exit 0; all selftests passed; expected invalid-audio warning |
| REC12-ROMLESS-OFF | Equivalent command in `build/macos-rec07-off/sdl2/vaeg` | Exit 0; all selftests passed |
| REC12-REPO | `git diff --check`, case/UTF-8/EOL validators and milestone-ID audit | Exit 0 |

The full Werror frontend target was not used as a REC12 pass criterion because
it remains affected by pre-existing warnings in unrelated
`io/np2sysp.c` and `sound/vermouth/midinst.c`; the changed production units
were compiled with the generated Werror commands above.

## Acceptance coverage and limitations

The focused tap test proves exact contiguous intervals (`100..120..150` in a
synthetic guest-clock domain), monotonic frame identity, copied pixel/OSD
association, skip activation, descriptor invalidation, and fail-closed
consumer errors. The production path uses the actual `screendispva` boundary,
canonical shadow copy and REC08 snapshot helper. Existing non-recording
screenshots, QA captures, redraws and presentation backends remain outside the
tap.

No live callback was connected to a recording session because that is REC13.
Therefore this report does not claim a complete MKV, synchronized audio,
public UI/CLI, VLC playback, Displayed capture, GPU readback, Windows/Linux
runtime, physical guest demonstration, human acceptance or release validation.
The tap's callback lifetime is synchronous by design; REC13 must copy or move
the frame before returning and apply its bounded queue/backpressure policy at a
safe guest boundary.

Machine gate: PASS.

Human/release gate: NOT_REQUIRED for this machine-only checkpoint; no human or
release validation is claimed.

Next dependency-ready task: REC13 / M100r14 after this Native tap checkpoint.

Push/remote state: local only; no push, merge or publication performed.

<!--
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
-->
