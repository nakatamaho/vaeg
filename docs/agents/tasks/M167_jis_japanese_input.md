<!--
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
-->

# M167 — JIS keys for guest Japanese input

**Status:** Planned only. No implementation or G167 result. The maintainer
has authorized implementation and candidate verification while G166 is pending;
see the M167-only exception in [AGENTS.md](../../../AGENTS.md). G167 still
requires maintainer acceptance.

## Goal and scope

Expose the PC-88VA keys needed for guest-side Japanese input through VAEG's
existing JIS keyboard layout:

| Host JIS key | Emulated PC-88VA key |
|---|---|
| 無変換 (Muhenkan) | 決定 |
| 変換 (Henkan) | 変換 |
| 右 Alt (Right Alt) | 全角 |

Follow [ADR-0008](../DECISIONS/ADR-0008-keyboard-mapping.md): map SDL scancodes
through the normal guest key down/up path. Confirm the guest key identities
against the PC-88VA keyboard mapping rather than substituting Return or a
host IME action. Preserve the existing US layout and custom bindings; verify
that consuming Right Alt in JIS mode does not leak an Alt modifier to the
guest. Host IME integration and new US/F12 assignment options are out of
scope.

## Checks and G167

- Add focused automated coverage for JIS key press/release, focus-loss cleanup,
  and unchanged US and custom mappings.
- Run the applicable CMake build, CTest, repository checks and headless smoke.
- Provide the clean static MinGW handoff for the standard human gate. Verify
  with a physical JIS keyboard that the three guest keys support entering,
  converting and confirming Japanese text in a guest application. Keep
  G167 pending until the maintainer accepts it.
