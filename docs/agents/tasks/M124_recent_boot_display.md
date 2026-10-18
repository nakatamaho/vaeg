<!--
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
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

# M124 — monitor and screen rotation in 最近の起動

Status: **implemented; G124 human gate pending**

## Request

The maintainer (2026-10-18, asked as "M122"; that number was taken by the
recent folders): 最近の起動 should also keep 24 kHz / 15 kHz and the 90°
screen rotation.

## Work

([00a5d1d9](https://github.com/nakatamaho/vaeg/commit/00a5d1d98e8255429b2814f84c5f202c4268a917))

- Each boot record keeps the monitor (`Monitor15kHz`) and the rotation
  (`GUI_rotate`) at the reset that records it, as a fourteenth
  `vaeg-history.txt` field `monitor,rotation`. Records from before M124
  read as unknown (`-1,-1`) and leave both settings as they are.
- Choosing a record sets the monitor before its reset and the rotation at
  once; the label adds `15 kHz` / `24 kHz` and `右 90°`.

## Verification

GUI harness (Linux, offscreen) with one new and one old record: the menu
shows `pce.d88  (VA2/VA3, 自動, 15 kHz, 右 90°)` and `pce.d88  (VA2/VA3,
自動)`; choosing the first writes `Monitor15kHz=true` and `GUI_rotate=1` and
turns the window; the old record is written back as `-1,-1`. ctest 94/94.

## Gate G124 (human)

Standard gate, plus: boot something at 15 kHz with the screen turned, boot
something else at 24 kHz unturned, then choose the first from 最近の起動:
it comes back at 15 kHz and turned.
