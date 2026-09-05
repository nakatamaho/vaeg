<!--
 * Copyright (c) 2026 Nakata Maho
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
-->


# M99z10 — Renderer-specific screen settings

Starting commit: `c09d135f8a2f98ee1249cb06826e8e3a92339a17`.
Branch: `topic/m99-native-crt-rebuild`; unrelated main changes preserved.

## Changes

- Replace the CRT shader parent menu with direct SDL/librashader selection.
- Show SDL Effect only for actual SDL ownership; show Shader settings only
  for native ownership. Move preset path, reload, parameters and reset into
  a separate closable window. Shared scaling/aspect/window controls remain.
- Keep existing configuration and parameter persistence across renderer changes.
- Show actual renderer/filter status, including native pass-through, and
  retain creation failure details after the failed presenter is destroyed.
- No screenshot semantics, raw captures, guest timing, GPU algorithms, vendor
  dependencies or binary assets changed.

## Verification

Commands (from the task checkout):

```sh
CCACHE_DISABLE=1 cmake --build --preset mingw-cross -j4
CCACHE_DISABLE=1 cmake --build build/macos-ci -j4
CCACHE_DISABLE=1 cmake --build build/macos-macports -j4
ctest --test-dir build/macos-ci --output-on-failure -R '^(vaeg_librashader_|vaeg_romless_tests$|vaeg_sdl_startup_viewport$)'
ctest --test-dir build/macos-macports --output-on-failure -R '^(vaeg_librashader_|vaeg_romless_tests$|vaeg_sdl_startup_viewport$)'
python3 tools/repo/check_encoding.py --expect utf8
python3 tools/repo/check_eol.py --enforce
python3 tools/repo/check_case.py
git diff --check
```

Builds PASS. Both focused suites: 10/10 PASS. Repository checks: zero findings.
The controller test now injects one initialization failure into the passing
fake presenter and checks exact retained diagnostic text, then verifies a
successful retry clears it. Existing parameter persistence, capture boundary
and SDL window-rebind tests remain enabled.

Windows menu interaction/visual verification is deferred to the maintainer;
local cross-compilation and mock tests are not physical GPU evidence.
macOS/Linux native GUI integration remains outside this follow-up.

## Handoff

Implementation: [11bafaf5](https://github.com/nakatamaho/vaeg/commit/11bafaf5ede49c4bae1df7fc8316e754eb85e571).
Reconfigured/rebuilt MinGW after committing to embed that build identity.
Package: `build/mingw-cross/vaeg-m99z10-windows-x86_64.zip`, including the
unchanged runtime DLL, shaders, licenses and updated user guide.
Directory and ZIP package validators PASS; staged EXE matches the build via `cmp`.
ZIP SHA-256: `e469c2327c281cb7bf8c9e394b62ea87eca7c1eee95a292c38642c9668594fad`.
Final feature-on/off focused tests passed 10/10 each (8.49 s / 7.70 s).
Removed an extra report EOF blank line detected by the staged diff checker.

## M99z13 — Purpose-oriented renderer menu

Starting commit: `076e9eb4aa7b3b3d73e545de447992ec121b1301`.
The maintainer approved `画面 > 描画方式 > 標準（SDL） / CRT効果（librashader）`.
SDL shows `エフェクト` directly beneath the renderer group; native CRT shows
`CRT設定…`. The settings window keeps its existing ImGui identity. Hide normal
SDL/filtered status text in the menu, but retain failures, native pass-through
and restart-required notices. Screenshots remain the first two menu entries.
No configuration, renderer lifecycle, capture, shader or guest behavior changes.

Validation: the MinGW and macOS feature-on build commands listed above PASS;
the macOS feature-on focused ctest command passes 10/10 (6.07 s). Encoding,
EOL and case checks report zero; diff check clean. Formatting was limited to
the changed renderer-selection block. Windows manual menu verification is
deferred; no new physical GPU evidence is claimed.

Implementation: [e595f00e](https://github.com/nakatamaho/vaeg/commit/e595f00e7f2db7a12d34bb69f13bd0e886f1d7c3).
MinGW rebuilt with committed identity; staged EXE matches via `cmp`.
Package: `build/mingw-cross/vaeg-m99z13-windows-x86_64.zip` (DLL/assets/notices
included); package validator PASS.
ZIP SHA-256: `027f74e5e0cb75932530967c511f3516433c9d979ca04af13b4db894658daccd`.

## M99z14 — Exclusive fullscreen auto-hide menu

Starting commit: `c84c5491d95571bbdbe45930a4bc634f25b33d10`.
Exclusive fullscreen starts with a hidden menu. A 3-DPI-scaled-logical-pixel
top-edge hover reveals it; pointer-over-bar or popup/item interaction retains
it, otherwise it hides after 500 ms. Windowed mode remains always visible.
Relative guest mouse capture must first be released with the existing toggle.
The reveal area blocks guest mouse input to avoid click-through.

Fullscreen reserves zero menu inset regardless of visibility. SDL and native
CRT share this viewport policy, so menu visibility cannot resize or shift the
guest image. SDL information overlays now draw before ImGui (at most once per
frame), with a present-end fallback for non-GUI callers. This keeps a revealed
menu above information panels, matching native CRT composition. Screenshot
capture still omits GUI and now retains the entire fullscreen drawable area.

Focused tests exercise hidden entry, reveal, 499/500 ms timeout boundary,
popup/item retention, bar hover, mode exit/reentry, tick wrap, zero-inset crop,
and idempotent SDL overlay composition. The builds and focused ctest commands
listed above are reused. No GPU code, guest timing, raw golden capture or
configuration format changes. Real Windows exclusive-mode hover/input and
SDL/CRT screenshot checks remain deferred to the maintainer.

Results: MinGW and macOS feature-on/off builds PASS; focused suites 10/10
PASS each (6.27 s / 5.69 s). Encoding/EOL/case checks zero; diff check clean.

Implementation: [8fa87aea](https://github.com/nakatamaho/vaeg/commit/8fa87aea2711f91b2889cf8e5dac1243291ae3ba).
MinGW rebuilt with committed identity; staged EXE matches via `cmp`.
Package: `build/mingw-cross/vaeg-m99z14-windows-x86_64.zip` (DLL/assets/notices
included); package validator PASS.
ZIP SHA-256: `608c9f2df23adb13e6c33916dcf3b811be0b8c10b7b902f67fa2dbe4dfb3cfa0`.

## M99z15 — Discoverable fullscreen toggle

Starting commit: `711bfc183880f246a3c7aa760a5162dec471279f`.
The maintainer confirmed Windowed restoration works; the provisional exit-path
changes were withdrawn without committing. This follow-up changes only GUI
discoverability: replace Windowed/Exclusive fullscreen with one checked
`全画面表示` item; show `画面上端でメニュー表示` for up to three seconds on
entry while the menu is hidden; widen the reveal zone from 3 to 12 DPI-scaled
logical pixels. No return button or Esc shortcut is added. The hint takes no
input and is skipped by the existing screenshot GUI-suppression path.

The existing menu-policy selftest additionally verifies the hint's 2999/3000 ms
boundary and immediate disappearance on leaving fullscreen. The previous
build/test/check commands apply. Windows physical UI verification is deferred.

Results: MinGW and macOS feature-on/off builds PASS; focused suites 10/10
PASS each (6.17 s / 5.84 s). Encoding/EOL/case checks zero; diff check clean.

Implementation: [db8c2a57](https://github.com/nakatamaho/vaeg/commit/db8c2a57b319369f8fe2a9edca7de250d2a94131).
MinGW rebuilt with committed identity; staged EXE matches via `cmp`.
Package: `build/mingw-cross/vaeg-m99z15-windows-x86_64.zip` (DLL/assets/notices
included); package validator PASS.
ZIP SHA-256: `5ea7f01ea36a1f6ae6c0e3a43c21edb4f788f6e2ac8656661ee91da77b549251`.

## M99z16 — Fit startup splash to drawable size

Starting commit: `5bcd66b9b91ca64d7e87341be556bcc29bb87c75`.
The SDL startup splash now fits the actual renderer output dimensions instead
of using the configured integer guest scale. Aspect ratio is preserved,
remaining margins are centered, and small outputs can downscale safely.
The embedded bitmap and nearest-neighbor sampling remain unchanged. This
does not add a splash to native-only startup paths that lack an SDL renderer.

Verification: `CCACHE_DISABLE=1 cmake --build --preset mingw-cross -j4` and
`CCACHE_DISABLE=1 cmake --build build/macos-ci -j4` PASS.
`ctest --test-dir build/macos-ci --output-on-failure -R '^(vaeg_romless_tests$|vaeg_sdl_startup_viewport$)'`
passes 2/2 (5.08 s), including new fit geometry checks for 640x422, 1280x844,
1920x1080, downscaling and zero output rejection. Encoding/EOL/case checks
zero; diff check clean. Physical Windows splash appearance remains untested.

## M99z17 — Two-thirds splash and native presentation

Starting commit: `8301422e62b38733b372a1d5e0acd61d7081416d`.
Fit the splash inside a centered box occupying two thirds of each drawable
dimension, preserving aspect ratio. Native startup previously returned early
because `splash_show` required an SDL renderer. It now converts the embedded
bitmap to ARGB8888 and submits it through the active native presenter with a
temporary splash viewport and filter disabled, then restores the filter and
guest viewport. The guest shadow and raw capture paths are not used or changed.
No vendor/backend code or bitmap payload is modified.

MinGW and macOS feature-on builds PASS using the commands above. The focused
macOS ctest suite passes 10/10 (6.33 s), including updated two-thirds geometry
checks and existing controller/filter/viewport tests. Encoding/EOL/case checks
zero; diff check clean. Actual native splash visibility remains untested locally;
the geometry and controller tests are not physical GPU evidence.

Implementation: [74e32878](https://github.com/nakatamaho/vaeg/commit/74e32878f70cb8809ab6306dd676927ad6225a8a).
MinGW rebuilt with committed identity; EXE-only handoff at
`build/mingw-cross/vaeg-m99z17-windows-x86_64/vaeg.exe` matches the build via `cmp`.

## M99z18 — CRT screen size, 80–120 percent

Starting commit: `908b0a7076992767b84773f197d59f830517d6e9` on
`topic/m99-native-crt-rebuild`. Scope: bundled default preset only.
The new independently authored BSD-2-Clause shader adds `VAEG_SCREEN_SIZE`
(80–120, default 100, step 1) through the existing runtime parameter UI and
persistence. Older parameter files leave the new value at its default.
No frontend, emulator, vendor shader, or capture implementation is changed.

Data flow: unchanged guest texture -> centered size pass at source resolution
-> unchanged audited Lottes CRT pass -> existing overlays/display capture.
At 80%, the pre-CRT image occupies the central 80% of each dimension, leaving
10% black on each edge. At 120%, it is enlarged and clipped. Curvature may
reshape this border. Normal displayed screenshots include the result;
unprocessed screenshots and canonical QA captures do not acquire this effect.
Custom presets remain untouched. The additional pass requires updating assets,
not just the EXE. Package staging and validation pin the new shader and preset.
The complete new BSD notice is embedded in the shader; upstream provenance and
the original permissive CRT shader bytes remain intact.

Local commands and results:

```sh
CCACHE_DISABLE=1 cmake --build --preset mingw-cross -j4
CCACHE_DISABLE=1 cmake --build build/macos-ci -j4
CCACHE_DISABLE=1 cmake --build build/macos-macports -j4
ctest --test-dir build/macos-ci --output-on-failure -R '^(vaeg_librashader_|vaeg_romless_tests$|vaeg_sdl_startup_viewport$)'
ctest --test-dir build/macos-macports --output-on-failure -R '^(vaeg_librashader_|vaeg_romless_tests$|vaeg_sdl_startup_viewport$)'
python3 tests/frontend/librashader/test_screen_size.py
python3 tools/repo/check_encoding.py --expect utf8
python3 tools/repo/check_eol.py --enforce
python3 tools/repo/check_case.py
git diff --check
```

Builds PASS. macOS feature-on focused suite: 11/11 PASS (6.39 s).
macOS feature-off focused suite: 11/11 PASS (6.05 s).
Coordinate/preset tests: 3 PASS, including exact rational pixel-center checks
at 640, 400, 1920 and 1080 pixels for 80/100/120%. These are explicitly static
and reference-math evidence, not GPU readback. Repository checks: zero findings.
Existing macOS duplicate-library/alignment and `maketextva.c` warnings, SDL
CMake deprecation and MinGW libarchive cross-check warnings remain unrelated.

Both shader stages additionally compiled successfully to SPIR-V using Debian
glslang-tools 12.0.0-2 in an ephemeral Colima container:

```sh
docker run --rm --mount type=bind,source="$PWD",target=/src,readonly debian:bookworm-slim sh -c 'apt-get update -qq && apt-get install -y -qq python3 glslang-tools && python3 /src/tests/frontend/librashader/test_screen_size.py --glslang /usr/bin/glslangValidator'
```

Container image digest:
`sha256:88200866dfff7ea7f5cbcb6ec7c8a701889efe6fe859fe64d6990e4b07ea4171`.
This does not prove librashader reflection, D3D11 compilation, or physical GPU
appearance. Windows visual/screenshot checks at 80/100/120%, parameter reload,
and GPU performance remain deferred. The added source-sized intermediate pass
has not been GPU-benchmarked; no performance gate is claimed.

Implementation: [fc470537](https://github.com/nakatamaho/vaeg/commit/fc470537f0bb018c6d8d0d5ec5a92b17ea65dcab).
MinGW was reconfigured and rebuilt after this commit to embed its identity.
Handoff: `build/mingw-cross/vaeg-m99z18-windows-x86_64.zip`, containing the EXE,
unchanged optional DLL, updated assets and notices. Directory and ZIP validation
PASS with `python3 tools/release/check-librashader-package.py --input <path>
--platform windows`; copied EXE matches the build with `cmp`.
EXE SHA-256: `10565bd5b1ba90a87397a588071bd04e3c426dc4470789efe49cd3622619bf48`.
DLL SHA-256: `1890f647c7fbe52d4cc591526db24367caca284996855c4565c6003c7e46f8cc`.

## M99z19 — Expand the FDD browser list with its window

Starting commit: `810fc808e900072109fc86bd533c3667e0a643cd`.
`draw_fdd_browser()` already uses a resizable ImGui window with first-use-only
initial dimensions. Vendored ImGui enables edge resizing by default and the
SDL platform backend advertises mouse cursor support. The inner list, however,
was fixed at 230 pixels high. It now consumes the available vertical space,
reserving the path field, Mount/Cancel row and wrapped status text using the
current font/style metrics. Small windows retain a three-row minimum and the
existing parent scrolling. No HDD browser or disk mounting code is changed.

MinGW and macOS feature-on builds PASS with the M99z18 commands above.
The initial build caught the repository's `max` macro; using `(std::max)`
resolved that collision before the successful builds. The same focused macOS
suite passes 11/11; encoding/EOL/case and diff checks are clean. Only the edited
GUI lines were formatted with `clang-format-mp-22`. These tests do not exercise
physical mouse dragging. Manual check: drag a side/corner of "Mount FDD image
or archive", verify the list expands, and confirm Mount/Cancel remain usable
at normal and high DPI. Windows interaction evidence remains deferred.

## M99z20 — librashader version header regression

The maintainer reported "shader parameter enumeration failed" after M99z18.
Reproduced against official librashader 0.12.0 macOS arm64, without a GPU:
`PreprocessError(MissingVersionHeader)`. The new shader's BSD comment preceded
its version directive. Moving `#version 450` to the first line resolves actual
runtime enumeration: nine parameters, including size metadata 100/80/120/1.
The license notice is unchanged; its position follows the required directive.
The standalone glslang test used in M99z18 did not enforce this librashader
preprocessor requirement and was insufficient. The new optional runtime test
checks both the working preset and a passing temporary fixture mutated only
by prepending a comment, asserting the exact MissingVersionHeader category.
The GUI now retains the runtime's detailed error string for future failures.

```sh
python3 tests/frontend/librashader/test_screen_size.py --runtime build/m99z20-runtime-check/librashader.dylib
CCACHE_DISABLE=1 cmake --build --preset mingw-cross -j4
CCACHE_DISABLE=1 cmake --build build/macos-ci -j4
ctest --test-dir build/macos-ci --output-on-failure -R '^(vaeg_librashader_|vaeg_romless_tests$|vaeg_sdl_startup_viewport$)'
```

Official release archive `librashader-aarch64-macos-v0.12.0-optimized.zip`
SHA-256: `49808004a4904f6a99e0231092dcfdfe52b7b61f68430a4c9f1e165749c4c90e`.
Downloaded with `gh release download librashader-v0.12.0 --repo
SnowflakePowered/librashader --pattern librashader-aarch64-macos-v0.12.0-optimized.zip
--dir build/m99z20-runtime-check`, and verified against the release asset digest.
No dependency version, shader math, or emulation/capture boundary changes.
Windows GPU appearance and mouse dragging remain unverified locally. The
handoff also includes the separate M99z19 FDD list resize commit.
Builds PASS; focused suite 11/11 PASS (7.31 s), runtime positive/negative checks
PASS, repository encoding/EOL/case checks zero and diff check clean.
Test runtime dylib SHA-256:
`1dbecea0c165fd0fddc2407ed4b8872f9f73f4fd2c3689a80ffc24c87e3fda2a`.

Fix: [0730d507](https://github.com/nakatamaho/vaeg/commit/0730d507e44a4e5fc4eb4d827763b5b93cbdabcf).
FDD resizing: [74644d99](https://github.com/nakatamaho/vaeg/commit/74644d996045fd60192e0b6058c4fa95d3cc8835).
Reconfigured and rebuilt MinGW with the committed fix identity. Handoff:
`build/mingw-cross/vaeg-m99z20-windows-x86_64.zip`. Both directory and ZIP pass
the package validator; copied EXE matches the build artifact using `cmp`.
EXE SHA-256: `15fdbc2e1c0a1578ad3eef8fcd8c789b658d06b331e56adbac161502e380c82a`.
Replace the package assets as well as the EXE: the shader file fixes CRT,
while the executable contains the FDD layout and improved error diagnostic.

## M99z21 — Requested CRT defaults and parameter name

Starting commit: `6e89523278bf67c23972047b742ff87b9bd4478f`.
Rename the owned shader parameter to `SCREEN_SIZE`, default 96.50%, range
80–120%, step 0.01%, displayed with two decimal places. Set `CURVATURE=0.030`
in the VAeg-authored preset without modifying the audited upstream shader.
The actual pinned runtime enumerates the preset override as the parameter's
initial value, so the existing reset path uses 0.030 without special-case code.
Saved settings retain precedence; use "Reset parameters" for the new defaults.
The retired parameter name is ignored by the existing unknown-key policy.

Validation uses the same M99z20 build, ctest and runtime commands above.
MinGW and macOS feature-on builds PASS. The actual 0.12.0 runtime returns
SCREEN_SIZE 96.5/80/120/0.01 and CURVATURE initial 0.030 (float32 precision).
The runtime negative test for MissingVersionHeader also PASS. Package hashes
are updated. No guest/capture changes or physical Windows GPU claims.
Focused ctest: 11/11 PASS (7.95 s). Encoding/EOL/case: zero; diff check clean.
