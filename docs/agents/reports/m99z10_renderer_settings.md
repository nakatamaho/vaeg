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
