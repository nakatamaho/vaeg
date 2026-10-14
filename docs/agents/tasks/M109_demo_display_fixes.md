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

# M109 - Demo fixes and 200-line display on the 24 kHz monitor

Status: **in progress**

Series: V3 series (maintainer request, 2026-10-14, after G106). Branch
`topic/m109-demo-display-fixes` off `main` at `d9a84316`. Commit prefix
`M109:`.

## Goal

Items carried over from M106 by the maintainer:

1. ZUNDAORB runs without an I/O bank memory board (the maintainer's
   PC-88VA2 has none; M106 found that the program stops when its BMS
   probe fails).
2. ZUNDAORB HUD: a space after the `DIST:` value, and `LOOK` and `RAD`
   moved right by one column accordingly.
3. Every pseudo-sprite demo can show up to 1024 balls.
4. Other demo fixes found on the PC-88VA2 with the self-booting SGP disk.
5. With the 24 kHz monitor setting, 640x200 and 320x200 modes are shown
   without scanlines (line-doubled), and ZUNDAORB uses such a mode too.
6. The kana and Caps Lock indicators at the bottom (front panel / status
   area).
7. A hardware check of the digital 8-colour mode (maintainer, on the
   PC-88VA2).

## Gate G109 (human)

Standard V3 gate (clean static MinGW build of the pushed commit), plus:
ZUNDAORB runs on the PC-88VA2 from the rebuilt disks; the pseudo-sprite
demos reach 1024 balls; 200-line modes show no scanlines at 24 kHz in vaeg;
the kana and Caps Lock indicators follow the keyboard; the digital
8-colour result on hardware is recorded whatever it is. Demo disks are
rebuilt and their payloads match a fresh build (AGENTS.md).

## Progress
