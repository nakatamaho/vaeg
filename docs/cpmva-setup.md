<!--
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
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
# CP/MVA setup guide

This guide explains how to prepare and run CP/MVA on VAEG. The procedure was
validated with the M76 uPD70008-compatible Z80 emulation path and a generated
CP/M tools disk that reaches the CP/M `A>` prompt.

## Requirements

- A VAEG build with the M76 changes.
- A user-owned, FAT-formatted PC-Engine boot D88. The input image must be
  obtained lawfully; VAEG does not redistribute PC-Engine ROMs or guest disks.
- Python 3.10 or newer.
- `z80asm` 1.8 in `PATH` or selected with `VAEG_Z80ASM`.
- `lha` or `unar` for the CPMVA archive.

The installer downloads and verifies the locked CPMVA, CP/M, game, and BDS C
sources. It does not execute downloaded DOS or CP/M programs on the host.

## Build the disks

From the VAEG checkout, run:

```sh
python3 tools/cpmva/install_cpmva.py \
    --boot-disk /path/to/pcengine-boot.d88 \
    --output-dir /path/to/cpmva-ready \
    --accept-cpm-license \
    --accept-cpmva-license \
    --accept-games-license \
    --accept-bdsc-license
```

The original boot disk is never modified. The output directory contains:

```text
pcengine-boot-cpmva.d88  PC-Engine boot disk copy with CPMVA files
cpmva-tools.d88         CP/M tools and games disk
cpmva-source.d88        source and documentation disk
cpmva-dev.d88           BDS C development disk
cpmva-build-manifest.json
cpmva-install-report.txt
```

The installer builds the 64K CP/M 2.2 CCP/BDOS from the locked public source,
combines it with the CPMVA `CPMBIOS.COM`, and writes the resulting `CPM.SYS`.
It does not use NEC `MOVCPM5.COM`, `DDT.COM`, `SAVE`, M80, L80, or MASM.

For an offline installation, populate the cache during an online run and add
`--offline`. Local archives can be supplied with `--cpmva-archive`,
`--cpm22-archive`, `--vt100-games-archive`, and `--bdsc-archive`; every local
archive is still checked against the lock file. See
[`tools/cpmva/README.md`](../tools/cpmva/README.md) for source-lock and cache
details.

## Run CP/MVA in VAEG

1. Start VAEG and load `pcengine-boot-cpmva.d88` as the PC-Engine floppy
   boot disk.
2. Boot the guest and run `CPMVA.BAT`, or type `CPMVA` at the DOS prompt.
3. When CP/MVA displays `Set CP/M diskette on drive FD1: and hit any key`,
   replace FD1 with `cpmva-tools.d88`.
4. Press a key. CP/MVA loads the CCP, BDOS, and BIOS from the tools disk and
   should display the CP/M `A>` prompt.
5. Run `DIR` to verify the tools disk, then run `EXIT` to return to
   PC-Engine.
6. Swap FD1 to `cpmva-source.d88` or `cpmva-dev.d88` when source files or BDS C
   tools are needed.

The CP/MVA disk images generated together by the installer must be used as a
set. Do not manually rewrap them as another D88 geometry; the tested VAEG
layout preserves the CP/M directory and sector mapping expected by the
PC-88VA BIOS.

## Headless runs

The SDL2 frontend accepts the existing headless input script format. A disk
can be swapped after CP/MVA has displayed its prompt with `@fdd1`:

```text
CPMVA
@wait 1200
@fdd1 /path/to/cpmva-tools.d88
@wait 1200
@enter
@wait 600
DIR
@wait 600
EXIT
```

Pass the script with `--headless-input-script /path/to/script.txt`. This is
useful for repeatable smoke tests, but it does not replace the interactive
instruction to change FD1 in a normal session.

## Troubleshooting

- If CP/M stops before `A>`, confirm that the boot disk is the generated
  `pcengine-boot-cpmva.d88` and that FD1 was changed to the matching
  `cpmva-tools.d88` only after the CP/MVA disk prompt.
- If `DIR` reports `Bdos Err on A: Bad Sector` or shows no file names,
  regenerate the disks with the same installer version and use the generated
  pair without a manual D88 conversion.
- If `EXIT` is reported as unknown, verify that `EXIT.COM` is visible in
  `DIR` and that the tools disk is still mounted in FD1.
- Existing output files require `--force`; `--dry-run` and `--verify-only`
  do not replace output files.

CP/M, CPMVA, the included games, BDS C, and their documentation retain their
original licenses. The installer records their provenance and does not assign
VAEG's BSD-2-Clause license to those materials or to generated disk images.
