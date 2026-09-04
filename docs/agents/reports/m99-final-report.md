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
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
-->
# M99 final report

Status: BLOCKED

M99a through M99z were executed on `topic/m99-native-crt-rebuild`. The
optional native CRT implementation, licensing boundary, package checks,
fallback path, and environment-independent QA are complete. The required
real-GPU lifecycle and 1920x1080 performance gates could not be demonstrated
from this macOS/Colima environment, so this report does not claim `DONE`.

## Commits, branch, and remote state

- Starting rewritten-main baseline: `0b6e14883035cb17073c5c24c8c9d4b5c22b9162`.
- The lease-protected rewrite of `main` succeeded before M99 work began; the
  rejected implementation is unreachable from ordinary repository-owned refs,
  and its old commit log/hashes are intentionally not reproduced here.
- During M99, `origin/main` advanced to `b25d151236ddfe093e4c161bbf00ce0b7d8d5e74`
  with one unrelated retained change. It was merged into the topic by
  `5d2e63b5bfa52f5e84609b36a02d4fd2a161db5e` without altering that change.
- Topic remote at the last push before this report commit:
  `origin/topic/m99-native-crt-rebuild` = `01d31199e1ee86a24b6395f1a687f189c0bafcc0`.
- The final report commit is the containing commit for this file; its exact
  full ID is the final `HEAD` shown by `git log -1` after this commit.
- No merge into `main`, release, binary publication, or remote-history rewrite
  was performed after the authorized main reconstruction.

## Sub-milestone results

| Milestone | Result | Evidence commit(s) |
| --- | --- | --- |
| M99a | PASS — history boundary recorded | `9bcc6866` |
| M99b | PASS — removal boundary verified | `c36d429e` |
| M99c | PASS — main reconstructed | `04d9fe24` |
| M99d | PASS — lease-protected main publication | `cc5aed3c` |
| M99e | PASS — clean baseline recorded | `09fbab2e` |
| M99f | PASS — frontend inventory | `13064088` |
| M99g | PASS — dependency and shader audit | `33fe683a`, `b07b9c50` |
| M99h | PASS — common contracts | `92b4ad5e` |
| M99i | PASS — raw capture boundary | `aac27e1c` |
| M99j | PASS — common pass-through tests | `20bb4897` |
| M99k | PASS — macOS/Metal build bridge | `007ba166` |
| M99l | PASS — Metal pass-through implementation | `bb90b1ef` |
| M99m | PASS — Metal filter chain | `0ca99abc` |
| M99n | PASS — Metal lifecycle handling | `544eea1a` |
| M99o | PASS — D3D11 backend | `a3f29cef` |
| M99p | PASS — D3D11 pass-through | `c68ed39c` |
| M99q | PASS — D3D11 librashader chain | `34cafc26` |
| M99r | PASS — D3D11 lifecycle handling | `4f5ee593` |
| M99s | PASS — Linux OpenGL backend | `fda01ee1` |
| M99t | PASS — OpenGL librashader chain | `971d32bc` |
| M99u | PASS — common presets and parameters | `ab9246b9` |
| M99v | PASS — settings UI | `a7587921` |
| M99w | PASS — fallback and headless behavior | `1f141693` |
| M99x | PASS — shader and release packaging | `cfdd41d7`, `2b2da357` |
| M99y | PARTIAL — automated QA passed; physical GPU evidence deferred | `12e33893`, `7fda0663`, `ed339397` |
| M99z | PASS — documentation, evidence, and SDL smoke correction assembled | `b07b9c50`, `265f1582`, `5d2e63b5`, `1bd5330f`, `01d31199` |

## Gate status

| Gate | Status | Evidence boundary |
| --- | --- | --- |
| G99-1 | PASS | Rewritten main was lease-published and later unrelated mainline work was retained; ordinary refs were checked. |
| G99-2 | PASS | Backend-neutral contract, core isolation, raw-capture tests, and static audits passed. |
| G99-3 | BLOCKED | No Windows host/D3D11 hardware was available for real lifecycle and performance evidence. |
| G99-4 | BLOCKED | Colima's virtual arm64 Linux container had no real OpenGL display/GPU; its software/virtual result is smoke evidence only. |
| G99-5 | BLOCKED | macOS feature-on build passed, but no usable Cocoa/Metal display was available. |
| G99-6 | PASS | Runtime-free and optional-runtime staged archives for all three platforms passed inspection. |
| G99-7 | BLOCKED | No representative real-hardware 60 Hz benchmark; hosted CI still has the independent macOS FetchContent failure. |

## Implementation and ownership

The emulation framebuffer remains in the SDL2 shadow buffer and is submitted
as `VAEG_FRAME_INPUT`. Raw guest-frame and TVRAM capture remain upstream of all
GPU processing. A native presenter owns exactly one platform path:

- macOS: `MetalPresenter` and `CAMetalLayer`/Metal bridge;
- Windows: `D3D11Presenter` and D3D11/DXGI bridge bound to the SDL window;
- Linux: `GLPresenter` and an OpenGL 3.3 core bridge.

The common contract carries borrowed pixels, format, pitch, dimensions,
aspect, frame number, and timing only. It carries no platform backend type.
Preset parsing and metadata enumeration occur at initialization, while GPU
resources are recreated only for size/context/device recovery. Filter failure
first falls back to native pass-through; native device/resource failure
recreates the existing SDL renderer from the same shadow framebuffer.

Headless dummy video bypasses native presentation. Native rendered capture is
explicitly unavailable and reports that fact; deterministic `--screenshot`
and TVRAM capture remain the QA path.

## Dependency, binary, and license provenance

The pinned dependency is official `librashader-v0.12.0`, peeled commit
`87e8a97b50516d997defeaa168173dcd185d4022`, source archive SHA-256
`4bf8cf2489d00848dcabbf2163204093776082da4217d5a5db45e4cbf335cedf`.
The C API is 5 and C ABI is 2. VAEG uses the official C API and dynamic loader;
it does not statically link librashader.

The tracked header SHA-256 values are:

```text
librashader.h    5d478897c391af3f60015810b67785ae1a286d262a845485276e36ded9f21e62
librashader_ld.h bcffcbc854afb287c9f935c1a0e3b569f5e6775ef85914bd7f7025ad1f6bde33
```

VAeg remains BSD-2-Clause. librashader implementation files are MPL-2.0 and
the upstream C headers/loader are MIT as recorded in ADR-0014. The single
default `crt-lottes-fast.slang` shader is recorded as Unlicense/public-domain
provenance, with no include, LUT, or secondary-pass dependency. CRT-Geom,
CRT-Royale, Mega Bezel, the complete `slang-shaders` tree, GPL material, and
unknown-license shader content are not tracked or shipped.

The staged shader closure is:

```text
assets/shaders/crt/vaeg_crt_default.slangp
assets/shaders/crt/shaders/crt-lottes-fast.slang
assets/shaders/crt/licenses/crt-default-license.txt
assets/shaders/crt/licenses/crt-default-provenance.md
licenses/librashader-MPL-2.0.txt
licenses/librashader-headers-MIT.txt
licenses/THIRD_PARTY_NOTICES.md
```

Package runtime inputs are optional and platform-specific. The audited staged
runtime hashes are recorded in the M99x report and ADR-0014; no runtime binary
is committed.

## Build and test evidence

Clean-baseline evidence is recorded in M99e: the old SDL renderer built, 83 of
84 registered CTest cases passed, one external SSTS test was skipped, and the
ROM-less smoke invocation used for the baseline completed in that local setup.

M99y ran the feature-on Linux build in Colima/Docker with the trace build
option, then:

```sh
cmake -S /src -B /tmp/vaeg-m99-linux -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug -DVAEG_ENABLE_TESTS=ON \
  -DVAEG_Z80_COMPAT_INTEGRATION_TRACE=ON \
  -DVAEG_ENABLE_LIBRASHADER=ON -DVAEG_ENABLE_ARCHIVE_DROP=OFF
cmake --build /tmp/vaeg-m99-linux --parallel 2
ctest --test-dir /tmp/vaeg-m99-linux --output-on-failure \
  -R '^(vaeg_librashader_|vaeg_romless_tests$)'
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
  /tmp/vaeg-m99-linux/sdl2/vaeg --selftest
```

Results: feature-on Linux build PASS, focused M99/ROM-less CTest 7/7 PASS,
and full executable selftest exit 0. macOS feature-on build and focused
fallback/lifecycle tests passed. Feature-off full app/selftest also passed.
The source archive checker passed after the explicit librashader allow-list
correction; package staging passed for Linux, macOS, and Windows with and
without the optional runtime, including a deliberate wrong-platform negative
case.

The hosted run for the earlier M99y tip is
[Actions run 33864023294](https://github.com/nakatamaho/vaeg/actions/runs/33864023294)
and completed with failure. The failure set was separately diagnosed:

- Linux gcc/clang/ASAN and Windows smoke jobs exited 1 after explicitly
  reporting ROM-less mode. The ROM-less return value in `sdl2/np2.c` is
  intentional: it keeps screen-uniform detection disabled. The actual
  startup failure was an inverted `SUCCESS == 0` test in
  `sdl2/scrnmng.c`, which treated successful SDL resource creation as a
  failure; M99z corrects both affected startup/fallback checks.
- The standalone archive check rejected `external/librashader` until the
  allow-list fix in `265f1582`.
- macOS FetchContent configuration failed inside hosted CMake with missing
  internal CMake variables; this is a host/toolchain configuration failure
  after checkout and before compilation.

The subsequent topic run for the corrected tip was
[Actions run 33867116878](https://github.com/nakatamaho/vaeg/actions/runs/33867116878).
It completed with the following result: repo invariants, guest-driver,
Ubuntu gcc/clang/ASAN, SST, standalone compatibility, and Windows MinGW
build/smoke/unit-test/release-artifact jobs passed. The only failure was
macOS FetchContent configure, with CMake reporting missing
`CMAKE_OBJCXX_COMPILE_OBJECT`, `CMAKE_OBJCXX_ARCHIVE_CREATE`, and related
internal variables before compilation. Hosted CI is not used as real-GPU
evidence.

## Performance and manual evidence

No acceptable benchmark numbers were obtained. Average, p95, maximum
presentation time, GPU resource growth, and 60 Hz sustain at 1920x1080 remain
unmeasured on representative real Windows, Linux, and macOS hardware. No
fidelity or shader policy was silently changed to manufacture a performance
result.

Deferred manual action: on one real host for each platform, run the default
CRT preset and native pass-through through resize, HiDPI/Retina, fullscreen,
minimize/restore, toggle, context/device/drawable recovery, and a sustained
1920x1080 60 Hz workload. Record hardware, driver/OS, preset identity, average,
p95, maximum frame/presentation time, and bounded-resource observations. This
is the smallest remaining action to convert G99-3/G99-4/G99-5/G99-7 from
BLOCKED to PASS.

## Final status

The implementation and all safe environment-independent M99 work are
complete. Overall status is **BLOCKED**, not DONE, because required physical
GPU lifecycle/performance evidence is unavailable and the hosted compatibility
workflow still has the independent macOS FetchContent/toolchain failure. The
SDL smoke startup condition has been corrected locally and passed in the
subsequent hosted Linux and Windows smoke jobs.

Working-tree status captured during report assembly:

```text
?? va2bkupmem.dat
```

The untracked backup file is pre-existing topic-worktree state and was not
modified, staged, or deleted.

## M99z1 Windows black-screen follow-up

A real Windows MinGW run after M99z displayed the menu and continued to
produce emulated audio and floppy-drive sounds, but no guest video. Its
startup and scale-change diagnostics showed valid window and drawable sizes
while the guest viewport remained `0,0 0x0`. The same result with the SDL-only
build excluded librashader loading and shader compilation as the immediate
cause.

The demonstrated defect was a second set of inverted status checks in
`sdl2/scrnmng.c`. `scrnmng_get_drawable_size()` returns the repository status
value `SUCCESS == 0`; four callers incorrectly used logical negation. This
made every successful drawable query fail viewport calculation and also
affected menu high-DPI placement, pointer mapping, and native-presenter
resize/presentation. Commit
[`fa2a3d78`](https://github.com/nakatamaho/vaeg/commit/fa2a3d78749d84fc08d90ee749bc32f10b2f08fa)
uses explicit `!= SUCCESS` checks, rejects an invalid startup viewport, and
adds the `vaeg_sdl_startup_viewport` ROM-less CTest.

Local verification at that commit:

```text
macOS MacPorts build (librashader off): PASS
macOS CTest: 90 executed PASS; 1 external SSTS case SKIP
dummy SDL smoke: guest=0,22 640x400
MinGW SDL-only console build: PASS
MinGW librashader-enabled D3D11 console build: PASS
encoding, EOL, and path-case checks: PASS
```

The repository-wide clang-format checker still reports formatting debt in
unchanged pre-existing lines; only the modified line ranges were formatted
with `clang-format-mp-22`. A physical Windows rerun with the corrected binary
is still required before claiming that the observed black screen is cleared,
and native D3D11/CRT lifecycle and performance evidence remains part of the
existing G99-3/G99-7 blocker.
