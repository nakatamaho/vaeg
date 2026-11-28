<!--
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
-->

# M171 — SASI multiboot candidate

- Earlier candidate: [`b518ef1c`](https://github.com/nakatamaho/vaeg/commit/b518ef1ceb948479620d95d81d5cd7cffabcbae8).
- [Task and gate](../tasks/M171_sasi_multiboot_disk.md)
- [English and Japanese instructions](../../modernization/pc88va-sasi-multiboot.md)

The builder constructs an empty NEC FAT12 volume. The guest's `SYS`
installs and captures both PC-Engine systems. The host checks FAT chains,
validates the captures against the installed files and stored IPLs, and
publishes only after checks finish. Existing utility disk builders and
`pcengine_disk.py` are unchanged. A frame-boundary TVRAM checkpoint in the
SDL2 debug harness lets the builder check the menu and default boot in one
no-input run. Startup insertion preserves the FAT root end marker, and
final validation reparses the bytes that will be published. The archived
reference tier is unaffected.

## Source and ROM-free checks

- `python3 -B tools/pc88va/test_sasi_multiboot_disk.py -v`: 9 passed.
  Single-mutation negatives check `D5_FILE_MISSING`, `D5_STATE_CRC`,
  `FAT_COPIES`, and `GUEST_TIMEOUT_MENU` against passing fixtures. The tests
  also cover all four FAT12 geometries, startup names, stale root entries,
  the CRC vector and no-clobber output.
- `git diff --check`, `check_case.py`, `check_encoding.py --expect utf8`,
  `check_eol.py`, `check_md_comments.py`: passed.
- CMake Release build and focused CTest (`vaeg_m74_debug_harness`,
  `vaeg_m152_debug_script_parser`, `vaeg_romless_tests`): passed.
- Canonical clang-format check: blocked; `clang-format-mp-22` unavailable.

## G171 follow-up: FAT timestamps and menu labels

- Fixing commits: [`c48d4004`](https://github.com/nakatamaho/vaeg/commit/c48d400432863e600b9297ac8ed2c8238b84f55e)
  (timestamps) and [`ee1037e4`](https://github.com/nakatamaho/vaeg/commit/ee1037e442d487d7762d4d9b90f08423c3395508)
  (labels).
- Host FAT insertion now requires a valid write timestamp. It preserves
  the source directory fields for DOS utilities, uses local modification
  times for supplied startup files and timestamps generated startup files
  at build start. The final check reparses the HDI and compares the
  builder-written entries with their expected timestamps. Guest `SYS`
  still owns system-file installation and capture metadata.
- The defaults are `PC-Engine 1.05 (VA)` and `PC-Engine 1.1 (VA2/3)`.
  The builder requires MS-DOS 4.0 VA with D5.1 or later, checks the
  version 3 label table and requires the full labels in the menu.
- All 15 synthetic checks passed, including exact timestamp round trips,
  zero-date rejection, an older menu format, an older label prompt and a
  truncated label. Repository validators and the three focused CTests
  passed. The utility disk builders are unchanged.
- D5.1 follow-up checklist (VA 5 MB and VA2 40 MB): confirm the three
  full labels in the boot menu and `SYS C: /L`, select each entry and
  check its prompt, and compare the dates shown by `DIR C:` for
  `SYS.COM`, `CHKDSK.COM` and `AUTOEXEC.BAT` with their FAT directory
  timestamps. G171 human acceptance remains pending.

## Guest-check contract for the earlier candidate

| Model | SASI sizes for G171 | Checkpoints |
|---|---|---|
| VA | 5 and 40 MB | VA banner, keys 1/2 to `Ready`, 3 to `C>`, timeout retains selection, `SYS C: /L` lists both captures, clean `CHKDSK C:` |
| VA2 | 5 and 40 MB | VA2/VA3/VA-91 banner, keys 1/2 to `Ready`, 3 to `C>`, timeout retains selection, `SYS C: /L` lists both captures, clean `CHKDSK C:` |

Headless guest checks passed on VA and VA2 at 5 and 40 MB, and both standard SASI utility HDIs matched the baseline byte-for-byte. Input and output identities are not published here (statement approved by the maintainer on 2026-11-27). Automated
guest checks use headless text VRAM, not the actual SDL presenter.

**NOT RUN:** G171 human acceptance, physical hardware, separate VA3 and VA-91
coverage, non-Linux host coverage, full media/demo regression, and actual
presenter/screen automation. No merge or release acceptance is implied.
