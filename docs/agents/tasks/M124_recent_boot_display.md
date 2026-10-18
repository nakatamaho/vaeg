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

Status: **complete; G124 passed on 2026-10-18**

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

## Follow-up: two-disk archives (maintainer, 2026-10-18)

The display part worked. A ZIP holding disks A and B came back as A, A:
both drives record the archive, and opening an archive for FDD2 took its
first disk. An archive of two or more disks opened for FDD2 now takes its
second disk ([e2ba66dc](https://github.com/nakatamaho/vaeg/commit/e2ba66dcde74b35d2644fb270a35ad86d83cdcbd));
checked with the real archive in a LibArchive build (ledger entry).

## Follow-up: the display a session ends with (maintainer, 2026-10-18)

The maintainer asked to keep also the window size at exit, 描画方式 and
拡大方法. These were already kept in `vaeg.cfg` across a restart (checked:
`GUI_win_cx/cy`, `GUI_scaling`, `NativeCRT`); the request applies to 最近の起動.
Each recent boot now also holds the rotation, renderer (SDL, CRT,
librashader unfiltered), scaling, window scale and window size its session
ended with ([7a310a08](https://github.com/nakatamaho/vaeg/commit/7a310a08b21e04a46b3e7c3693066fd0936dd459)). They
are written when the session ends: at the next reset, before another
recent boot is restored, and at exit (`gui_session_end`). The fourteenth
field becomes `monitor,rotation,renderer,scaling,scale,width,height`; the
two-value form of the earlier M124 build still reads. The monitor stays
the one the session started with, and the display fields no longer make two
records of the same media differ.

Verified with the GUI harness: a session changed to 整数倍, 2倍 and then
quit wrote `0,0,0,3,2,1280,822`; a new start from default settings that
chose the record came back at `GUI_scaling=3`, `GUI_scale=2` and a 1280x850
window; with librashader (unfiltered) chosen the record held renderer 2 and
restored `NativeCRT=true`, `NativeCRTFilter=false`.

## Local CI QA at `9af8cb4e` (2026-10-18)

- Repository checks: encoding, EOL, path case, Markdown comments, milestone
  IDs, clang-format of the changed sources: no findings.
- `linux-ci-gcc`: build, smoke, CTest 94/94 (four external-fixture skips),
  selftest; no new warnings in the changed files.
- `linux-ci-asan` (ASan/UBSan): CTest 94/94, no sanitizer reports.
- `linux-ci-clang` in the pinned Ubuntu 24.04 container with the CI job's
  packages: build, smoke, CTest 94/94.
- Media build regression (clean clone): demo disks, SGP boot disk,
  all-demos and all-demos-msdos4 identical to the committed images; the
  utility disk (4096 bytes free) and both SASI images build.
- Clean-checkout static MinGW build: `vaeg.exe` SHA-256
  `849a7e6da82b7dd275f0273fb5b9333a8e15381f02884cd6ff0ac7b31d2bcf8b`, Windows
  system DLL imports only.

## Gate G124 (human)

Standard gate, plus: boot something at 15 kHz with the screen turned, boot
something else at 24 kHz unturned, then choose the first from 最近の起動:
it comes back at 15 kHz and turned, with the window size, 描画方式 and
拡大方法 it had when that session ended; a two-disk ZIP comes back as A and B.

### G124

- Passed on 2026-10-18 (maintainer) with the Windows build
  `849a7e6da82b...cf8b` (SHA-256, static MinGW) from
  [9af8cb4e](https://github.com/nakatamaho/vaeg/commit/9af8cb4ea808f617251cf8ec72812ded3af1197d).
