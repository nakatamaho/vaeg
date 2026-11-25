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

# M170 — Right Ctrl is 全角 in the US layout

**Status:** G170 human gate passed on 2026-11-25 at [`413889d3`](https://github.com/nakatamaho/vaeg/commit/413889d3042964bad9f3367e5fcb64cceb86ec57) (maintainer: Windows host with the Japanese IME, US layout, Right Ctrl reaches the guest as 全角).
Merged to `main` with `--no-ff` at [`7cfbe700`](https://github.com/nakatamaho/vaeg/commit/7cfbe70034e7e882256afcd61d531fc82a761f95).
Assigned 2026-11-25 by the maintainer from the FreeDOS-88VA MS-DOS work;
branch `milestone/m170-us-zenkaku` from `main` (`d79a931f`).

## Why

On a Windows host with the Japanese IME installed, the US layout has no way
to send the PC-88VA 全角 (FULLWIDTH, `5Bh`) key, which switches Japanese
input on the guest (ROM JFP and the MS-DOS 4.0 project FEP). Alt+` is taken
by the Windows IME before VAEG sees it. The maintainer chose host **Right
Ctrl** for 全角 in the US layout: it is unbound in VAEG today (guest CTRL is
Left Ctrl only), Windows and its IME do not use it alone, most laptops
have it, and it sits at the right end of the bottom row like 全角 on the VA.

| Host US key | Emulated PC-88VA key |
|---|---|
| Left Ctrl | CTRL (unchanged) |
| Left Alt | GRPH (unchanged) |
| Right Alt | カナ (unchanged) |
| Application | 変換 (unchanged) |
| F11 | 決定 (unchanged) |
| **Right Ctrl** | **全角 (new)** |

## Scope

- US preset only: bind `SDL_SCANCODE_RCTRL` to `KBDROLE_ZENKAKU` (`5Bh`)
  through the normal guest key down/up path (ADR-0008). JIS and custom
  presets keep their bindings; custom maps may still rebind 全角.
- Right Ctrl must not also act as guest CTRL, and must not leave a CTRL or
  全角 bit stuck on focus loss (reuse M167's focus-release handling).
- Check where VAEG reads `KMOD_CTRL`/`KMOD_RCTRL` for host shortcuts or
  chords (GUI, F12 actions, keytop chords): Right Ctrl + key must not
  trigger a host action or a guest Ctrl chord in the US layout. Report any
  conflict instead of silently changing other bindings.
- Update the keyboard binding UI/docs/help text that list US keys.
- Out of scope: host IME pass-through, Ctrl+Application alias, JIS changes,
  the pre-existing US bare-grave `^` issue (record it as a follow-up note).

## Checks and G170

- Automated: extend the M167-style event-queue test (`vaeg_jis_japanese_keys`
  or a sibling) for US Right Ctrl make/break → `5Bh`, no CTRL bit, focus
  loss cleanup, JIS and custom unchanged; one mutation that drops the
  binding must fail.
- Guest check (vaeg VA and VA2, MS-DOS 4.0 Japanese disk
  `msdos4-va.3` from https://github.com/FreeDOS-88VA/MS-DOS/releases):
  US layout, `JFP ON` then Right Ctrl switches to kana input (romaji →
  かな), Right Ctrl again back; `JFP ROM` likewise toggles the ROM JFP.
- Applicable CMake build, CTest, repository checks, headless smoke; the
  standard human gate with the clean static MinGW handoff (absolute path
  of `vaeg.exe`). The maintainer tests on Windows with the Japanese IME
  active: Right Ctrl reaches the guest.
- Do not merge; push the branch and report SHAs.

## Shortcut audit and follow-up

The US preset masks only the Right Ctrl modifier bit before the frontend
routes SDL key events. This avoids conflicts with Ctrl+V, Ctrl+Shift+C,
and ImGui's Ctrl handling without changing Left Ctrl or other layouts.
F12 quick save/load checks Shift, not Ctrl; US keytop chords likewise
check Shift only. Right Ctrl does not disable independently assigned host
keys. The existing F11 fast-forward action takes precedence over the US
preset's F11 → 決定 guest binding; this conflict is unchanged. The
pre-existing US bare-grave `^` translation is a separate follow-up.
