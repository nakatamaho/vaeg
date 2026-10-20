<!-- Copyright (c) 2026 Nakata Maho. SPDX-License-Identifier: BSD-2-Clause -->
# M100s10 — Windows/MinGW and Linux Displayed archive recording

## Authorization and scope

Maintainer request: implement Displayed video recording on Windows (MinGW) and
Linux, following the accepted G100s9. One milestone; no realtime H.264, host audio
clock adapter, decorated OS capture, or distribution approval is implied.

Preserve Native/Displayed FFV1/PCM/MKV guest-time semantics. Capture the actual
complete client after GUI composition, before presentation; do not run shaders
twice or replace output with the guest framebuffer. Reuse bounded storage, perform
no recording readback when disabled, and fail closed on acquisition errors.

## Work plan

- Audit the existing common SDL_Renderer and Windows D3D11 paths.
- Add Linux OpenGL final-client readback and the missing native GUI integration.
- Reuse Windows staging resources across equal-size captures.
- Verify recording ON/OFF, focused contract tests, Linux renderer/runtime checks
  where available, MinGW compilation/import audit and repository invariants.
- Keep platform runtime and private media checks explicitly NOT_RUN when unavailable.

## Gate

OPEN. Human gate: clean static MinGW handoff, Windows and Linux Displayed recording
with GUI/effects, decoded movie picture/audio/seek/EOF, resize/reset/stop and Native
regressions; standard guest boot/demo/OS and all required media/demo build checks.
No platform runtime success is inferred from a cross-build or mocked fixture.

## Status

Client implementation is present; gate remains OPEN. SDL_Renderer already had
shared Displayed readback. Windows D3D11 now reuses its size-matched staging
texture, releases it when capture is off, and clears completion before early
failure. Linux now creates a GL-capable native window, supports pass-through
without a shader runtime, composes the pinned unmodified ImGui OpenGL3 backend,
and forwards one final-client readback before swap. Readback restores pixel-pack
state and converts in place to top-down opaque RGBA. No extra render or allocation
is performed by the GL readback path. FFmpeg 6 compatibility failures found by the
Linux checks are corrected without weakening existing exact-image assertions.

## Local validation checkpoint

Builds use Debug, tests and subsystem trace ON. macOS ON validation uses system
FFmpeg, not approved static redistribution dependencies. Linux uses Ubuntu 24.04
arm64, system FFmpeg 6.1.1, Xvfb and Mesa software OpenGL; physical Linux GPU and
Wayland remain unverified. Commands and raw logs are retained in task-local
scratch/evidence, not shipped as guest/hardware evidence.

- `cmake --build build/m100s10-linux-on -j4`: exit 0, complete recording ON app.
- `ctest --test-dir build/m100s10-linux-on -L recording --output-on-failure`:
  exit 0 after the FFmpeg corrections; hardware H.264 probe SKIP and GL fixture
  default-disabled (run explicitly below), not hardware PASS.
- `LIBGL_ALWAYS_SOFTWARE=1 xvfb-run -a build/m100s10-linux-on/vaeg_gl_displayed_test`:
  exit 0. Actual GL/ImGui final-client rectangle and font-atlas upload, RGBA bytes,
  top-down rows, letterbox, capture OFF unchanged buffer, restored custom pack
  alignment/row length/skips, invalid capture geometry and invalid-frame errors.
- `cmake --build build/m100s10-linux-off -j4` and recording-label CTest: exit 0;
  16 tests pass (separate manually run GL fixture remains default-disabled).
- macOS recording OFF build/recording-label CTest: exit 0, 16 tests pass.
- macOS recording ON build/recording-label CTest: exit 0, 25 tests pass. This
  does not replace the earlier accepted physical Metal evidence or a static build.
- Linux production app, intentionally empty ROM directory, SDL audio dummy,
  `--record-source displayed --record-frames 4`: exit 0 for SDL_Renderer and for
  actual native OpenGL pass-through. The latter logs `backend=OpenGL
  state=pass-through` and `GUI renderer: native`; earlier SDL fallbacks are kept
  as failed-native-attempt evidence, not mislabeled native passes. `ffprobe` and
  `ffmpeg -v error -i <movie> -f null -` decode both resulting movies, exit 0.
  No real guest boot, sound device, shader runtime or playback acceptance implied.
- MinGW `x86_64-w64-mingw32-g++ -std=c++17 -Wall -Wextra -Werror` compile of
  D3D11 bridge: exit 0 with vendored headers marked `-isystem`. An initial
  all-`-I` attempt failed inherited vendor unused-parameter/function-cast warnings;
  those logs remain, and vendor source was not edited.

Implementation commit:
[43fdb709](https://github.com/nakatamaho/vaeg/commit/43fdb709aaf6fb02c7a231cf6cfa4b57f71b0647).

The clean MinGW helper now accepts `--recording`, explicitly building pinned
minimal LGPL-only static FFmpeg 8.1.2 (archive SHA-256
`464beb5e7bf0c311e68b45ae2f04e9cc2af88851abb4082231742a74d97b524c`).
Recording OFF remains its default. GPL/nonfree/version3/autodetect/network are
disabled; source archive, configuration, license and logs are retained outside
Git. Host report hashing/size uses Python rather than GNU-only `stat -c` so the
helper can finish on macOS. `bash -n` passes.

The first clean `97ae264959fed2dc13b0c35502ee3938bc9618a1 --recording`
helper run completed the pinned shader SDK and FFmpeg builds, then failed CMake
configuration: the cross toolchain overwrote `PKG_CONFIG_LIBDIR`, hiding the new
FFmpeg stage. Its logs/dependency outputs are retained, not rerun as a mystery
failure. The correction adds an explicit target-only dependency prefix to both
CMake's root paths and pkg-config, including propagation through compiler probes.
A fresh MinGW compiler/prefix/pkg-config/static-archive CMake probe passes; the
first probe also caught and corrected the compiler-probe propagation issue.

`--shader-sdk DIR` now permits read-only reuse of the completed dependency SDK;
the existing mandatory CMake source/archive/notices hashes still apply and no
application objects are reused. The next clean clone will use that audited SDK.
This does not turn the failed 97ae2649 helper into an executable handoff.

Additional checks: Linux GL bridge/presenter `-Wall -Wextra -Werror` compiles pass
with vendor headers treated as system headers. ASan/UBSan real GL fixture passes
(`ASAN_OPTIONS=detect_leaks=0`, Mesa/driver leak checking excluded;
`UBSAN_OPTIONS=halt_on_error=1`). Isolated Linux ON/OFF full `--selftest` both
exit 0. No hosted CI has been used as an iterative debugger.

Still required: clean pushed-commit fully static recording-ON MinGW artifact and
import audit, native Windows recording/playback, Linux shader/physical GPU and
Wayland coverage, standard boot/demo/OS and private media/PSD/DOSBox checks.
No human gate or legal/distribution readiness is asserted.
