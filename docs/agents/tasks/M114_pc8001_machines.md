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

# M114 — PC-8001 machines

Status: **implemented; G114 human gate pending**

## Request

Run the PC-8001, PC-8001mkII and PC-8001mkIISR from the ROMs found by M112
under `extra/<model>/`, as M113 does for the PC-8801 family (maintainer,
2026-10-16).

## Decisions

- [POLICY] As in M113 (approach A): a PC-8001 model runs in the N mode of
  the VA2's V1/V2 mode with its own N-BASIC ROM in place of a PC-8801's
  `n80.rom` (`n80` for the PC-8001, `n80_2` for the mkII, `n80_2sr` for the
  mkIISR). The VA firmware still starts the machine and the VA's N88-BASIC
  stays; the VA ROM set remains required.
- [DERIVED] A 24 KiB PC-8001 ROM leaves 6000h-7FFFh, the expansion socket,
  reading FFh.
- [DERIVED] The PC-8001 BASICs program the CRTC for 10-raster rows at 200
  lines, a 15 kHz monitor (uPD3301 reset parameters CE 93 69 BE 13, against
  CE 93 73 38 13, 20-raster rows, from the PC-8801 N-BASIC). On a 24 kHz VA
  the text is shown at half height, so these machines run with SW1 at
  15 kHz (`videova.crtmode`), for the session, after `videova_reset()`.
- [DERIVED] Port 31h has its PC-8001mkII meaning (X88000 `WriteIO_31_N80`):
  bit 1 selects all RAM and bit 3 enables graphics; bits 0, 2 and 4 are the
  mkII's bank and graphics bits, not the PC-8801's 200-line, N/N88 ROM and
  colour bits. vaeg keeps N-BASIC and 200 lines (bits 0 and 2 set) and
  passes bits 1 and 3. A PC-8001 never writes port 31h, so N mode starts at
  200 lines. Without this the mkII ROM's write of port 31h selected the
  VA's N88-BASIC.
- [POLICY] The PC-8001 family has no FDD inside: the external unit runs
  `disk.rom` when one is given, as for the PC-8801. 8x8 characters come
  from `font.rom`.
- [POLICY] Only N mode is offered under 起動 for these models.

Not modelled (recorded, not part of G114): the mkII's own graphics modes
(N80 mode graphics through ports 31h, 33h, 35h, 53h, 5Ch-5Fh) and its
expansion ROM bank at 6000h; the mkIISR's N80SR mode (`n80_3`,
`font80sr`, `fonthira`). The mkII and mkIISR start the N-80 BASIC of their
`n80_2`/`n80_2sr` ROM.

## Implementation

- `sdl2/extrarom.c`: PC-8001 profiles (`n_role`), `apply_pc8001()`.
- `io/memctrlva.c`: port 31h on a PC-8001 model; 200 lines at N-mode entry.
- `memoryva`: `memoryva_pc8001`, cleared at every `romva_initialize()`.
- GUI: N only for these models.
- Documentation: [`pc8801-machines.md`](../../modernization/pc8801-machines.md).

## Verification

- Selftest `PC-8001 machine` (synthetic ROM files): 24 KiB N-BASIC with an
  empty socket, `font.rom`, N mode, 15 kHz, no FDD unit, port 31h, state
  cleared at reset; model profiles in `PC-8801 machine`. Mutation check: 7
  of 7 mutants killed.
- With ROM sets matching MAME's `pc8001`, `pc8001mk2` and `pc8001mk2sr`:
  "NEC PC-8001 BASIC Ver 1.1", "NEC N-80 BASIC Ver 1.0" and "Ver 1.2"
  reach `Ok` at full row height, and `print 1+2` answers 3; from the
  command line and from 起動.
- Media build regression at `3d4908e1` (M115, which contains this
  branch), clean clone, shared cache: `build-utility-disk.sh` (PC-Engine
  1.1) exit 0, 4096 bytes free, boots in vaeg; `build-sasi-utility-disks.sh`
  exit 0, each image boots alone on its model with `\BIN\V480PAT.COM`; the
  five demo `build-d88.sh`, `build-from-psd.sh`, the SGP boot disk,
  all-demos and all-demos-msdos4 exit 0, every file in every regenerated
  image identical to the committed images (directory dates only differ;
  the SGP boot and all-demos-msdos4 `.xz` byte-identical).
- Clean-checkout MinGW build of `3d4908e1`: `vaeg.exe` SHA-256
  `100a54a7357e8b31f1e20e2d06e1f63c3a1123732114564e1ab1320efcb9737a`,
  imports Windows system DLLs only; ctest 92 of 92.
