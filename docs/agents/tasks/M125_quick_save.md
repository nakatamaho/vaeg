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

# M125 — quick save on F12

Status: **G125 human gate passed on 2026-10-19; main integration pending**

## Request

The maintainer (2026-10-18): saving is needed in the busiest moments, where
menu, choice and click are too slow; F12 should save to the top slot and
move the earlier ones down. Shift+F12 should load.

## Decisions (discussed with the maintainer)

- A separate ring of five quick saves; the ten manual slots are untouched,
  so repeated F12 never pushes out a manual save.
- Write first, then rotate: a failed save leaves the five as they were.
- Key repeat and a second press within one second are ignored.
- A notice at the top right for 1.5 s; no menu, dialog or pause.
- Shift+F12 loads the newest. The state before a quick load is kept apart
  (`quickundo.sav`), not pushed into the ring, so pressing Shift+F12 twice
  does not swap between two states.

## Work

([0fb4fbf7](https://github.com/nakatamaho/vaeg/commit/0fb4fbf790a3b7aa4802900a3db9b17392b2dfd8))

- F12 assignment `クイックセーブ (Shift+F12 で読み込み)` (`F12_bind=9`),
  handled in `taskmng.c` like the screenshot and pause assignments.
- `quick1.sav`..`quick5.sav` (+ `.png`) in the software's state directory;
  `quick_rotate` renames 4→5 .. 1→2 and the new file to 1.
- 状態 > クイックセーブ: 今すぐ保存, the five with time and thumbnail (click to
  load), 読み込む前の状態に戻す.
- Selftests: `quick save` (seven saves leave s7..s3 with thumbnails, a failed
  save changes nothing, no sixth) and the F12 binding; four mutations
  caught.

## Verification

In vaeg (Linux, PC-Engine, F12 events sent to the event loop): seven F12
presses and a double press gave eight saves (the second of the pair
ignored), five kept, newest first; Shift+F12 loaded quick1 and wrote
`quickundo`; the notices showed. A save took 32-48 ms (state and
thumbnail). ctest 94/94.

## Gate G125 (human)

Standard gate, plus: with F12 set to クイックセーブ, F12 during play saves
without stopping, Shift+F12 returns to the newest save, and 状態 >
クイックセーブ lists them newest first.

## G125 passed

The maintainer stated **Gate passed; merge requested** on 2026-10-19.
Accepted candidate:
[b247425f](https://github.com/nakatamaho/vaeg/commit/b247425fa767a9e72825e452c54d431d2c09e1a3).
The fully static MinGW build used a new clean clone and the pinned Ubuntu
24.04 container; commit/date embedding and the Windows-system-DLL-only
import audit passed. Executable SHA-256:
`66b382783b6dfa577dfbc4c7801d78ad6f1529299bc2202289540520b16f304d`.

The accepted build predates later main changes (including M127/M128).
Integrate with the current main in a separate temporary worktree, preserving
those changes, and run integration checks before pushing the main merge.
