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

# M99z8 — Native-to-SDL frontend stall candidate

Starting commit: `42a9afd4a34461f6d8533851e6b80276eb1e1d77`.
Branch: `topic/m99-native-crt-rebuild`.
Status: candidate; affected Windows machine confirmation required.

The maintainer clarified that selecting librashader then SDL leaves audio
playing but freezes menu interaction; exiting and relaunching VAEG restores
operation. This is not evidence of a guest CPU/pacing stall. A provisional
host-pacing change was discarded before committing.

## Correction boundary

The former transition replaced GPU/ImGui resources on the same SDL window
and HWND. The pinned ImGui SDL shutdown also does not explicitly release
mouse capture. These are observed lifecycle properties, not proof of the
driver call where the maintainer's process stops.

On Windows, native-to-SDL selection and native failure fallback now recreate
only the host window after native teardown, before creating the SDL renderer.
Window position/size, hidden/maximized/fullscreen state and icon are retained.
The guest framebuffer and CPU/device/media state are not reset or reallocated.
The old window is not reused across native DXGI and SDL presentation ownership.

GUI shutdown explicitly releases popup mouse capture. Event processing ignores
focus/size notifications for a retired window ID. Transition-only log messages
identify GUI release, native release, window detachment, SDL creation, GUI
initialization and completion. If it still stalls, the last emitted stage is
needed to locate the blocking call; the actual Windows root cause remains open.

## Verification

- `CCACHE_DISABLE=1 cmake --build --preset mingw-cross -j4`
- `CCACHE_DISABLE=1 cmake --build build/macos-ci -j4`
- `CCACHE_DISABLE=1 cmake --build build/macos-macports -j4`
- `ctest --test-dir build/macos-ci --output-on-failure -R '^(vaeg_librashader_|vaeg_romless_tests$|vaeg_sdl_startup_viewport$)'`
- Same ctest expression for `build/macos-macports`.
- `python3 tools/repo/check_encoding.py --expect utf8`
- `python3 tools/repo/check_eol.py --enforce`
- `python3 tools/repo/check_case.py`
- `git diff --check`

The ROM-less selftest exercises the actual replacement helper ten times:
window ID must change, 640x400 dimensions and hidden state must remain, and
each new window must support SDL software rendering with a red-pixel readback.
Dummy SDL is portable lifecycle evidence, not Windows D3D11/D3D9 driver evidence.

Results: MinGW build PASS; macOS feature-on/off builds PASS; both focused
ctest runs 10/10 PASS including the ten-cycle window/software-readback test.
Repository checks: zero encoding/EOL/case findings; diff check clean.

No Windows/Wine execution is available locally. Physical menu switching and
fullscreen/focus behavior must be retested. No GPU/performance success claimed.
Main checkout and its unrelated changes are untouched. No dependency/version
or shader payload changes; the existing runtime prerequisites still apply.
