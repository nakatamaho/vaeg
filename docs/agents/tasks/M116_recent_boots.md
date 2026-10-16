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

# M116 — 最近の起動 that does not start, and the host pacing label

Status: **complete; G116 passed on 2026-10-16; merged to `main` at [5f6b033](https://github.com/nakatamaho/vaeg/commit/5f6b0331abf37b2106c6f700568a214e1bc02b35)**

## Request

Maintainer report (2026-10-16, Windows build `57b838d8…`): entries under
模擬 -> 最近の起動 are greyed out or do nothing when chosen; they should
start. Boots of `.zip`, `.d88.xz` and plain `.d88` disks, and of PC-8801
models, are affected. Also: show the host pacing setting in Japanese.

## Findings

- Not reproduced with plain `.d88` entries on Linux: the entry mounts its
  disk and resets.
- [DERIVED] A greyed entry: a disk extracted from an archive is mounted
  from vaeg's archive cache. When it was not opened in this session (for
  example restored from the configuration at start-up), 最近の起動
  recorded the extracted copy, which the cache later removes, so the entry
  points at a missing file.
- [DEMONSTRATED] "Nothing happens": when an entry's disk could not be
  mounted, the entry still reset the machine, into the disk mounted before,
  without a message (reproduced with an archive on a build without
  LibArchive). Whether this is the maintainer's case is not confirmed.

## Changes

- `sdl2/dropmedia.cpp`: an extracted image keeps the path of its archive
  (`.source-archive` beside `.source-directory`); `dropmedia_source_archive`.
- `sdl2/gui/gui.cpp`: 最近の起動 records the archive for an extracted image;
  a greyed entry shows the missing files on hover; when an entry's disk
  cannot be mounted it does not reset and shows the reason at the top of
  the menu (and on stderr).
- 詳細設定: "ホストの待ち時間 / 1 フレームごとの最小間隔 (ms)" with a note
  that 0 disables it.

## Verification

- dropmedia selftest (LibArchive builds): the source archive of a zip/7z and
  of a `.d88.xz` image is read back.
- GUI (harness): a missing disk greys its entry and the tooltip names it;
  a failed mount leaves the running machine and shows the reason; a plain
  `.d88` entry starts while another disk is mounted.
- To confirm on Windows at G116: the maintainer's greyed and inactive
  entries; after re-recording, archive entries start.
- Clean-checkout MinGW build of `fa1b9989` (with LibArchive): `vaeg.exe`
  SHA-256 `4a7b8b2fbc90ba48fcb2748aa73600e07366d8220d70e9523720c06fac63a79c`;
  ctest 92 of 92. No media builder changed.

### G116

- Passed on 2026-10-16 (maintainer), with the Windows build
  `4a7b8b2fbc90...a79c` (SHA-256, static MinGW) from
  [fa1b9989](https://github.com/nakatamaho/vaeg/commit/fa1b9989ec5888acb6fe8ffbb2c63bd29093a024).
