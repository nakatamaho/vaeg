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

# M146 — EMS page frame and the system memory bank

Status: **implementation in progress; G146 human gate pending.**

Branch `milestone/m146-ems-bank`; commit prefix `M146:`. (M145 is the paused
`milestone/m145-hyperframe-probe`.)

## Request

The maintainer (2026-11-11): EMS memory use on the PC-88VA, reading the BNN
technical manual; bank switching does not work properly.

## Sources

- `[BNN]` PC-88VA technical manual (BNN), private copy, chapter 3: V3 memory
  layout, system memory area `A0000h-DFFFFh` banked by the memory map
  register (port 153h bits 3-0, SMBC: 0 bus slot, 1 TVRAM, 4 GVRAM,
  8/9 kanji ROM #1/#2, C dictionary ROM; reserved banks may be used by the
  bus slot); RAM bank port 1D0h for `80000h-9FFFFh`.
- `[EMMVA]` EMM adapter for the PC-88VA, EMMVA01/EMMVA02 1.5 (freeware,
  cached `emmva15a.lzh`): EMMVA01 sets SMBC to 0 before the EMM is loaded;
  EMMVA02 hooks INT 67h, selects SMBC 0 for each call, leaves it at 0 after
  the mapping functions (44h, 4Eh, 4Fh, 50h, 55h-57h, 5Bh) and restores the
  caller's bank otherwise. Read from a disassembly in task scratch; no
  disassembly is published.
- `[CODE]` vaeg: `memoryva.c` `ems_page_frame_active()` (M90) mapped the
  frame at `C0000h-CFFFFh` whenever an EMS target was selected (port 08E9h),
  independently of SMBC.

## Plan

1. A free MS-DOS 4.0 probe, `EMSBANK.COM`, with the EMS stack on the
   msdos4-va.2 disk: marker in the frame with SMBC 0, then GVRAM and kanji
   ROM #1 at the same address, then the frame again.
2. If vaeg fails it, restrict the frame to the bus slot bank and add a
   selftest with a single-mutation negative.
3. Check that the EMS stack and RDEMS still work (VA, VA2).

## Findings and change

- `[MEAS]` vaeg before the change, `EMSBANK` (EMMVA01 + SQEMM98 + EMMVA02,
  `ExMemory=1`, VA and VA2, PC-Engine 1.1 and MS-DOS 4.0 disks): `bus` and
  `bus-again` hold the marker, and so do `gvram-before` and `kanji`: FAIL.
  EMMVA02 leaves SMBC 0 after AH=44h (`153h-after-map=40`).
- [df87b814](https://github.com/nakatamaho/vaeg/commit/df87b814b1bbd3cfddcf737a9082a9e05114e0c3):
  `ems_page_frame_active()` also requires `sysm_bank() == 0` (the bus slot,
  or the DMA bank during DMA). Ledger entry added.
- [e60e0a85](https://github.com/nakatamaho/vaeg/commit/e60e0a854b51eb1171ada84d0acf93933505a15c):
  `tools/hardware/emsbank/` (probe, MS-DOS 4.0 disk builder, README).
- Not changed: the I/O bank memory at `80000h-9FFFFh` (port 1D0h, BMS),
  which follows the manual already; the V1/V2 memory map.

## Local verification

- `EMSBANK` after the change: PASS on VA and VA2 (both disks); `gvram-before`
  is GVRAM, `kanji` is kanji ROM data, the marker survives. The previous
  build FAILs the same disks.
- Utility FDD, `ExMemory=13`, VA and VA2: SQEMM98 completes, RDEMS `C:`;
  `copy a:config.sys c:m146.txt`, `type` and `dir c:` work.
- `tools/hardware/emsbank/build.sh` exit 0 (DOSBox Open Watcom backend);
  `SQEMM98.SYS` SHA-256 `eb3d443d7c12b6eb204e03a7ebac4b68a69d4690d0899ca93a37ad7a546d4930`
  equals M90's; `EMSBANK.COM` SHA-256
  `19d28db027d0cc02a257af99abebe7e8506ff3faaa59e88798d1108ee70f8e9c`.
- `--selftest` passes; with the bank condition removed it fails with
  "a non-bus-slot bank read the EMS page frame" (reverted). Native CTest
  183/183.

## Gate G146 (human)

Standard V3 gate (clean static MinGW build, V3 boot, bundled VA demo, an OS
and simple operations), plus: EMS Board enabled, boot the utility FDD and use
the RDEMS `C:` drive; boot the `EMSBANK` disk and run `EMSBANK` (PASS). On
hardware with an EMS board, `EMSBANK` is the comparison.
