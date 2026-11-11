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

Gate build: `tools/release/build-mingw-static-container.sh --commit
d37b57540878810ef381d1e8d0a7b9e41c1d9be2` with `VAEG_BUILD_TRACK=m146` exited
0 in vaeg's dedicated Colima runtime. It displays `Rel.20261111 (m146,
d37b5754)`, keeps the local-only encoders OFF and imports only Windows system
DLLs. `vaeg.exe` SHA-256
`10831ea0d7295d4d3c2d00e57633d130d81b967e014c6bfb162172e0908c9e72`.

## GUI additions (maintainer request, 2026-11-11)

- [347ff133](https://github.com/nakatamaho/vaeg/commit/347ff13330e40365209e8dab24318cb92b117236): the EMS
  Board "Installed memory (MB)" and I/O Bank Memory "128KB banks" fields were
  a fixed 120 pixels while their +/- buttons follow the GUI font, leaving no
  room for the digits at larger scales. Their width now comes from the digit
  width, frame padding and both buttons. A disabled EMS board offers 1MB.
- [e6fc0851](https://github.com/nakatamaho/vaeg/commit/e6fc0851a39086bd149e4284c4d8205ce8b8a0b9): SASI1 and
  SASI2 access lamps on a second front-panel row under FD1, in the V1/V2/V3
  green, lit 120 ms after each sxsi drive 0x00/0x01 block access
  (`sysmng_hddaccess()` was an empty macro). The panel is 26 dots per 640.
- [621ddc41](https://github.com/nakatamaho/vaeg/commit/621ddc41ca8fa5ea90e174a48944610dd0cd2ae9): the layout
  and speed QA follow the 26-dot panel. The M143 DRAFT layout references no
  longer match; this is an intentional visual change.

Verified on virtual X11 (Linux arm64 container, 960x640, software renderer):
the EMS dialog shows "3" and "Capacity: 3MB" after enabling and two "+";
booting the VA2 utility SASI image lights SASI1 while the drivers load, and
PC-Engine reaches Ready. Selftest: panel height, SASI lamp timing and
lit/dim pixels; turning the SASI lamp colour to the idle colour fails
"simple panel SASI1 lamp is not lit green" (reverted). Native CTest 183/183;
`x11_matrix.py` and `runtime_speed_qa.py` selftests pass. Not covered:
Windows, Cocoa, physical GPU, HiDPI.

At 960 pixels wide (scale 2) the V1/V2/V3 lamps are hidden to keep the
REC/lock group, as before this change (main shows the same).

Gate build (replaces `d37b5754` for G146): `tools/release/build-mingw-static-container.sh
--commit 84981ff9c7d2c02ebc48a4c8419575b7e0bfe586` with `VAEG_BUILD_TRACK=m146`
exited 0 (the first attempt failed in apt on a local DNS outage; its log is
kept). It displays `Rel.20261112 (m146, 84981ff9)`, keeps the local-only
encoders OFF and imports only Windows system DLLs. `vaeg.exe` SHA-256
`ba447695ff6fba0af7ce5ce0922749240ef6b39c98037aaa43830186c15d73a0`.

## SQEMM98 size message (maintainer report, 2026-11-12)

The maintainer saw "EMS Memory: 31.0 MB" with a 13MB board.
[27572df2](https://github.com/nakatamaho/vaeg/commit/27572df269e0d35d59f8b5bae46f012cde8a5e05)
fixes the upstream two-digit print in the PC-88VA preparation; ledger entry
added. `tools/hardware/emsbank/build.sh` rebuilt with it: "13.0 MB" at 13,
"10.0 MB" at 10 (VA2, MS-DOS 4.0).

The 13MB ceiling (`EMSIO_MAX_MEGABYTES`) is inherited from NP2's `ExMemory`
limit (PC-98 extended memory below 16MB); it is not a PC-88VA EMS board
specification. No VA board capacity has been identified yet.

## 15MB EMS (maintainer request, 2026-11-12)

The maintainer asked for 15MB, the most the 4-bit target register selects
(targets 1-15; 0 is off).

- [1f82fac9](https://github.com/nakatamaho/vaeg/commit/1f82fac95768568109fc637239ad03816fe38256):
  `EMSIO_MAX_MEGABYTES` 15; the selftest sweeps all 960 pages; current docs
  updated (historical records keep 13MB).
- [2b9ee78e](https://github.com/nakatamaho/vaeg/commit/2b9ee78e3b55430a41e437bde546d0606eef1299):
  SQEMM98 probes targets 1-15 and sizes its page tables for 960 pages;
  `check-sqemm98.py` checks the init save/restore sequences and the call
  target instead of a fixed displacement (single-mutation negatives: a
  broken restore and an out-of-driver target both fail
  `SQEMM98_CHECK_CALLER_STATE`); bundle pin
  `a8ef06d29d4558292ae89b7a5c5f9394abfd6cf1260c1d3871d63e116972fc08`.

VA2, MS-DOS 4.0 `EMSBANK` disk: at `ExMemory=15` SQEMM98 tests all pages,
reports "Page Count: 0960" and "15.0 MB", and `EMSBANK` PASSes; 14MB gives
896 pages, "14.0 MB", PASS; 13MB unchanged. Native CTest 183/183.
