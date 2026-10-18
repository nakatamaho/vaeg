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

# M127 — restore the letter-key keypad overlay

Status: implementation in progress; G127 human gate pending.

## Request and history

After M126, the maintainer requested the old YUI-based keypad mapping as an
item immediately below Key > かな入力. Follow the commit history first.
M125 remains on hold; this task starts from main without its quick-save code.

- Original feature: M14, `136c4b5ef2632ec57886b7d5238962a8f909664d`.
- M106, `a55c447ab833b83fc99018507ebaa0bdb5ab7769`, replaced the letters
  with cursor keys at the maintainer's request.
- M110 integrated that cursor overlay into カーソル割り当て; the old
  letter-keypad setting must not repurpose the current cursor setting.

## Scope

- Restore Y/U/I = 7/8/9, H/J/K = 4/5/6, N/M/comma = 1/2/3, period = 0.
- Independent persisted boolean `keyboard_letter_keypad`, off by default.
- A checked menu item directly below かな入力, with a mapping tooltip.
- Preserve cursor assignment, JIS/US/custom layouts, and normal guest
  make/break injection. Overlay precedes Roman-Kana and US punctuation.
- Release held overlay keys when disabling, resetting or losing focus.
- No emulator-core, save-state format, private media or vendored changes.

## Verification and G127

Run GCC and ASan/UBSan builds, CTest, smoke/selftest, focused mapping and
routing tests with mutations, repository invariant/format checks, and media
build regression from a clean checkout. Keep generated/private outputs out
of Git. Reuse unchanged expensive demo results only with explicit input
identity proof. Push the topic branch after local validation is final and
produce the clean static MinGW handoff.

Human gate: standard V3/demo/OS check, plus toggle the new item, check the
letter keypad in a guest, toggle off and check normal typing, restart and
check persistence. Cursor assignment must remain independent. Stop at G127;
do not merge or begin the next milestone before maintainer approval.
