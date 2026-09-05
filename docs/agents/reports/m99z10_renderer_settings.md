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

