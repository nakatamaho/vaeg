<!--
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.
-->
# M22 - SDL2 disk-image drag and drop

Status: implementation complete; G22 pending

Branch: `topic/m22-disk-drop`

Gate: G22 human media-drop and guest-regression gate

Depends on: G21 passed.

## Scope

- Collect one SDL drop operation from `SDL_DROPBEGIN` through
  `SDL_DROPCOMPLETE` even while Dear ImGui is open.
- Accept direct `.d88`, `.88d`, `.d98`, `.98d`, `.fdi`, `.xdf`, `.hdm`,
  `.dup`, `.2hd`, and `.tfd` images supported by the active FDD loader.
- Read `.zip`, `.7z`, and `.lzh` archives through LibArchive without shelling
  out to host commands.
- Sort all direct and extracted candidates by case-insensitive basename.
  Mount the first candidate as FDD1 and the second as FDD2; report the count
  of additional ignored candidates.
- Reuse `diskdrv_setfdd()` and the existing delayed FDD insertion path.
- Persist extracted images under the platform user-state `archive-drop/`
  directory and save mounted paths through `FDD1FILE` / `FDD2FILE`.
- Remove only managed extracted images that are no longer referenced by
  either configured drive.
- Show mount and error feedback at the bottom of the FDD menu.

## Archive dependency

`VAEG_ENABLE_ARCHIVE_DROP` defaults to ON. Distribution builds fetch pinned
zlib, XZ Utils/liblzma, and LibArchive releases and link them statically.
Development builds prefer a system LibArchive. If it is unavailable, direct
image drop remains active and archive drop reports that it is unavailable.
`VAEG_FETCH_LIBARCHIVE=ON` selects the pinned static stack explicitly.
The resulting `vaeg.exe` must not import their DLLs. Exact sources and hashes
are recorded in `../DECISIONS/ADR-0010-archive-drop.md`.

| Preset family | Acquisition | Linkage |
|---|---|---|
| Windows MinGW native/cross/CI | pinned FetchContent | static |
| macOS release/CI | pinned FetchContent | static |
| macOS MacPorts development/ASan | system LibArchive | package-defined |
| Linux | system LibArchive | package-defined |

## Extraction policy

- Reject absolute paths, drive-rooted paths, empty components, and `..`.
- Reject symbolic and hard links.
- Extract only supported disk-image entries; never materialize unrelated
  archive content.
- Limit archives to 4096 entries and 256 disk candidates.
- Limit one extracted image to 64 MiB and one drop batch to 128 MiB.
- Put every image in its own generated subdirectory under the managed
  user-state `archive-drop/` root, retaining only the safe basename from the
  archive entry.
- Delete incomplete output after any error.
- Open Windows archive paths through LibArchive's wide-character API after
  converting the SDL UTF-8 path with `std::filesystem::u8path()`.

Archive and direct image drops both persist through the existing `FDD1FILE`
and `FDD2FILE` settings. Reset and application restart therefore restore the
same mounted images. Eject clears the corresponding setting. A later drop
replaces only the drives for which it supplies candidates: a one-image drop
replaces FDD1 and leaves FDD2 unchanged. Managed extracted images are pruned
only after neither drive references their private image directory.

## Automated checks

- ROM-less selftest covers extension classification, unsafe-path rejection,
  basename ordering, and, in a LibArchive-enabled build, generated deflate ZIP
  and LZMA/LZMA2 7z extraction. LZH remains a manual test because LibArchive
  reads but does not write that format.
- Linux builds cover both LibArchive-found and LibArchive-missing policy when
  those environments are available.
- `mingw-cross` must build all archive dependencies statically and retain only
  Windows system DLL imports.
- Repository encoding, EOL, case, diff, and frozen-tier checks remain clean.

## Agent verification (2026-07-13)

- `cmake --preset linux-debug` and
  `cmake --build --preset linux-debug --target vaeg_sdl2` passed with no
  system LibArchive present; direct image drop remained compiled in.
- A fresh Linux build with `VAEG_FETCH_LIBARCHIVE=ON` passed. Its ROM-less
  selftest generated and extracted deflate ZIP and 7z archives, rejected a
  traversal entry, and verified reference-aware managed-storage pruning.
- ROM-less `--selftest` and `--smoke` passed in both the normal Linux build
  and the fetched-static archive build.
- `cmake --preset mingw-cross` and
  `cmake --build --preset mingw-cross --target vaeg_sdl2` passed. PE import
  inspection showed only Windows system DLLs; LibArchive, zlib, liblzma,
  SDL2, libgcc, libstdc++, and winpthread are not runtime DLL dependencies.
- The macOS release/CI presets select the same pinned static archive stack.
  They were not built on this Linux host because no Darwin SDK is available.
- `check_encoding.py`, `check_eol.py`, `check_case.py`, `git diff --check`,
  and the frozen-tier path diff passed. This repository currently defines no
  CTest preset.

## G22 manual gate

- Drop one direct D88 and confirm FDD1 insertion.
- Drop two direct images in reverse filename order and confirm sorted
  FDD1/FDD2 assignment.
- Drop three images and confirm the ignored count.
- Repeat the one-image and multi-image checks for ZIP, 7z, and LZH.
- Confirm nested archive paths work and traversal/link archives are rejected.
- Confirm an archive with no supported image produces a visible message.
- Open an ImGui dialog and confirm disk drop still reaches the emulator.
- Reset after an archive mount and confirm the extracted image remains usable.
- Restart and confirm archive-mounted FDD1/FDD2 images are restored.
- Eject one archive image and confirm an image still mounted in the other
  drive remains usable.
- Drop a different one-image archive and confirm FDD1 is replaced while FDD2
  and its managed extraction remain intact.
- Eject or replace the last reference and confirm the unreferenced managed
  extraction is removed.
- Confirm manual FDD Open/Eject, V3 boot, VA demo, and OS operations still work.
