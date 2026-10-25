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

# M136 — Controller port empty by default

Status: **implemented; G136 human gate pending.**

Branch `milestone/m136-controller-none`; commit prefix `M136:`.

## Request

The maintainer (2026-10-25): make "nothing connected" (なし) the default of
the controller port. Testament (V2) had ignored the keypad, the letter
keypad and the cursor-key keypad because Key > コントローラポート was set to
マウス: the game reads the port (OPN 44h/45h) and an idle mouse reads as
every direction held.

## Changes

- [db0d78cb](https://github.com/nakatamaho/vaeg/commit/db0d78cba992c6a434c05b491f1728c581189afa):
  - `MOUSEIFVA_NONE`: the port reads as released lines (data 0Fh, buttons
    03h) and ignores the strobe; it is the default.
  - Configuration key `ControllerPort` (0 joystick, 1 mouse, 2 none)
    replaces `Mouse_VA`, so existing configurations also start empty; an
    invalid value falls back to none.
  - `--controller none|joystick|mouse`; Key > コントローラポート gains なし.
  - カーソル割り当て ジョイスティック/マウス drives that device only while it
    is on the port; otherwise the cursor keys stay keys (before, they were
    taken from the keyboard and lost when the other device or none was
    selected).
  - Selftest `controller port` (empty port released; cursor up reaches the
    joystick only with the joystick on the port; one mutation caught);
    `cursor keys as mouse` now puts the mouse on the port.
- [31385bb4](https://github.com/nakatamaho/vaeg/commit/31385bb4e897c9345860a7d72ac4d3e4a8f93ffa):
  README.

## Gate G136 (human)

Standard V3 gate, plus: a fresh and an existing configuration start with
コントローラポート なし; Testament's keypad works with なし; selecting
ジョイスティック or マウス still works for software that uses them.

## Local verification

- `--selftest`: exit 0 (`controller port ok`, `cursor keys as mouse ok`).
  Restoring the old cursor-key diversion fails with "cursor up drove an
  absent joystick".
- `--controller bogus` reports "accepts only none, joystick or mouse".
