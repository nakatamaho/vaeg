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

# Main CPU and SGP timing on the PC-88VA and PC-88VA2

How vaeg times the main CPU's memory accesses, the SGP's commands and the
two working at once, why, and what is still open. The figures come from
programs in `demos/` run on a PC-88VA and a PC-88VA2/VA3 by the
maintainer (M128, M130, M141, M165); this page collects what the task
files and the [bug-fix ledger](bug-fixes.md) record one milestone at a
time.

## Measurement programs

| Program | Directory | Measures |
|---|---|---|
| SGPCONT | [`demos/sgp-contention`](../../demos/sgp-contention/README.md) | 18 rows: CPU loops in main RAM, G0, G1 (read, write, AND-OR, REP STOSW, REP MOVSW); SGP lists; both at once; clearing by CPU and SGP (M130) |
| SGPMEM | `demos/sgp-contention` | one 64-iteration loop each in main RAM below 40000h, main RAM at 60000h, GVRAM and TVRAM (read, byte write, word write) and a register loop; alone and while the SGP draws from main RAM or from GVRAM (M165) |
| SGPBENCH, CPUBENCH, MIXBENCH, DBLBENCH | [`demos/sprite-bench`](../../demos/sprite-bench/README.md) | sprites a VRTC: SGP from main RAM/GVRAM; compiled and loop CPU sprites; both shared; double-buffered frames (M165) |
| TSPSPR | `demos/tsp-sprites` | TVRAM word accesses (M141) |

All are MS-DOS 4.0 programs (msdos4-va.2) that name the machine on
screen. The machines' tables are in the programs' READMEs and in
[`demos/sprite-bench/results.md`](../../demos/sprite-bench/results.md).

## Main CPU

Documented waits (PC-88VA technical manual, section 3.5): standard main
RAM none, extended main RAM 1, bank RAM 1, TVRAM at least 2 (usually
7-13), GVRAM at least 1, kanji ROM at least 2, dictionary ROM 1, main ROM
none, extension ROM 1; V1/V2 mode at least 1 everywhere.

vaeg's model (`cpu/upd9002/`, `memoryva/`):

- **Clock counts** of the uPD70116 (V30) user's manual for a prefetched
  instruction, byte or even-address word; segment and REP prefixes 2; a
  word at an odd address takes a second bus cycle (M130).
- **Prefetch queue**: 6 bytes; the BIU fetches an aligned word every 4
  clocks while the bus is free; a taken transfer empties it, and its count
  covers the target's first word; an instruction whose bytes have not
  arrived stalls; the instruction's data cycles and their waits hold the
  bus (M130).
- **Straight-line fetch** (M165): an instruction word the CPU waits for,
  from main RAM, takes 5 clocks instead of 4 from the third word after a
  transfer on. Compiled sprites (long runs of 6-byte `MOV [DI+d16],imm16`)
  take about 21 clocks a word on both machines, which the 4-clock fetch
  made 17.6; short loops, which refill after every branch, are not
  slowed (SGPMEM's register loop: 27.4-27.6 clocks an iteration on the
  machines, 27.6 in vaeg).
- **Wait control unit** (FFF4h-FFF6h): the programmed waits by partition;
  PC-88VA: 1 in 40000h-DFFFFh, PC-88VA2: none (M130). SGPMEM: the PC-88VA's
  main RAM at 60000h reads 1% slower than below 40000h.
- **GVRAM**: a read waits 7 clocks; a write waits 1, and back-to-back
  writes are spaced to 5 clocks (REP STOSW 6.5 clocks a word) (M130).
  M165 first gave GVRAM word writes a 6.5-clock bus hold for the compiled
  sprites ([e369c6cb](https://github.com/nakatamaho/vaeg/commit/e369c6cb7bb6edfc74b170f4635483cb8657928e));
  SGPMEM then showed a GVRAM word-write loop as fast as a main RAM one, and
  the hold was replaced by the straight-line fetch above
  ([775e2f08](https://github.com/nakatamaho/vaeg/commit/775e2f08ba4f502b7af921bca712c53bdd6d2ed6)).
- **TVRAM**: a word access waits 13 clocks (PC-88VA) or 12 (PC-88VA2)
  (M141); a byte read 17 and a byte write 4 (M165,
  [2411c127](https://github.com/nakatamaho/vaeg/commit/2411c1279f69a16fcec3c1ea09787b2a481f099f)).
  SGPMEM's TVRAM loops against its main RAM loops on both machines: 52.1-52.6
  against 38.8-39.3 clocks a byte read, 37.5-37.7 against 33.7-34.0 a byte
  write. The difference is 13.3 clocks for reads, but vaeg's main RAM read
  loop is itself about 4 clocks fast (see the open questions), so the read
  wait is 17 to match the TVRAM loop.

## SGP

- **Clock**: 4 MHz on the PC-88VA, 8 MHz on the PC-88VA2 (`io/sgp.c`).
- **Command costs** (M128): setup, BITBLT/PATBLT words and lines, CLS
  words, the nominal costs scaled per model to SGPBENCH 2's microseconds.
- **Per source word** (M165,
  [b00b3d05](https://github.com/nakatamaho/vaeg/commit/b00b3d05a28be4e365ba6f093899284a188f00ee)):
  a BITBLT is charged by the source words it reads, not by the destination
  words it writes. A 16x16 sprite at any start dot (SGPBENCH) costs the
  same on the machines as one at dot 0 (SGPMEM): 4646 and 4662 CPU clocks
  on the PC-88VA. vaeg's extra destination word made SGPBENCH 5% slow and
  SGPMEM 4% fast; the word charge moved to the source and the BITBLT scale
  rose 4.6% (PC-88VA 432 to 452/256, PC-88VA2 497 to 519/256). A 1-bpp
  source expanded to colour is still charged per destination word.
- **Source in GVRAM** (M165,
  [00004337](https://github.com/nakatamaho/vaeg/commit/00004337370d7132554a5c401407c2524e338a12)):
  6 SGP clocks a word less than main RAM on the PC-88VA2 and 1.5 more on
  the PC-88VA. SGPBENCH's GVRAM/RAM ratios: PC-88VA2 1.04-1.15, PC-88VA
  0.94-1.00. SGPMEM's lists alone: PC-88VA 286 (main RAM) and 284 (GVRAM),
  PC-88VA2 454 and 489.

## The CPU while the SGP draws

The CPU loses a share of the time the SGP is busy (M130,
`sgp_cpu_share` in `io/sgp.c`): each CPU instruction executed meanwhile is
stretched by share / (1 - share), and the BIU fetches nothing during the
stretch (M165,
[27dfccda](https://github.com/nakatamaho/vaeg/commit/27dfccdaa67119d82336e5391624c38c942f3476):
without it, bus-bound code got the delay back in prefetched words). The
SGP is not slowed by the CPU (within 1-2% on the machines).

The share depends on where the SGP's BITBLT source is (M165,
[3e2023f6](https://github.com/nakatamaho/vaeg/commit/3e2023f684ea8ed85ee860fa336f922606422b43)):

| SGP source | PC-88VA share | CPU keeps (machine / vaeg) | PC-88VA2 share | CPU keeps (machine / vaeg) |
|---|---:|---|---:|---|
| main RAM | 80/256 | 0.696 / 0.695 | 142/256 | 0.460 / 0.449 |
| GVRAM | 30/256 | 0.870 / 0.867 | 55/256 | 0.747 / 0.737 |

(SGPMEM's register loop, 300 VRTCs.) The SGP still fetches its commands
from main RAM; the rest of the CPU's loss is the SGP's source reads. The
memory the CPU itself uses matters less (next section) and is not modelled.

## SGPMEM on the machines and in vaeg

Chunks of 64 iterations in 300 VRTCs alone, and the ratio kept while the
SGP repeats 30 16x16 sprites from main RAM / from GVRAM. Machines measured
2026-11-23; vaeg after M165.

PC-88VA:

| Loop | Alone (machine / vaeg) | SGP from RAM (machine / vaeg) | SGP from GVRAM (machine / vaeg) |
|---|---|---|---|
| register | 22653 / 22507 | 0.696 / 0.695 | 0.870 / 0.867 |
| RAM30 read | 15900 / 17927 | 0.784 / 0.695 | 0.910 / 0.865 |
| RAM30 write | 18383 / 18973 | 0.813 / 0.695 | 0.892 / 0.865 |
| RAM30 word write | 15529 / 16500 | 0.737 / 0.695 | 0.882 / 0.865 |
| RAM60 read | 15756 / 17427 | 0.770 / 0.696 | 0.906 / 0.863 |
| RAM60 write | 18383 / 18414 | 0.804 / 0.692 | 0.889 / 0.867 |
| RAM60 word write | 16321 / 16075 | 0.702 / 0.696 | 0.866 / 0.865 |
| GVRAM read | 14308 / 14929 | 0.788 / 0.694 | 0.900 / 0.865 |
| GVRAM write | 18383 / 18414 | 0.804 / 0.692 | 0.889 / 0.868 |
| GVRAM word write | 16321 / 16075 | 0.702 / 0.696 | 0.868 / 0.865 |
| TVRAM read | 11874 / 12051 | 0.834 / 0.693 | 0.919 / 0.862 |
| TVRAM write | 16571 / 16918 | 0.727 / 0.695 | 0.860 / 0.866 |
| TVRAM word write | 12342 / 12284 | 0.710 / 0.696 | 0.871 / 0.866 |
| SGP lists alone | 286, 284 / 290, 284 | | |

PC-88VA2/VA3:

| Loop | Alone (machine / vaeg) | SGP from RAM (machine / vaeg) | SGP from GVRAM (machine / vaeg) |
|---|---|---|---|
| register | 22835 / 22507 | 0.460 / 0.449 | 0.747 / 0.737 |
| RAM30 read | 16097 / 17927 | 0.567 / 0.455 | 0.776 / 0.734 |
| RAM30 write | 18560 / 18973 | 0.493 / 0.455 | 0.757 / 0.738 |
| RAM30 word write | 15752 / 16500 | 0.496 / 0.453 | 0.751 / 0.734 |
| GVRAM read | 14361 / 14929 | 0.597 / 0.456 | 0.771 / 0.737 |
| GVRAM write | 18516 / 18414 | 0.489 / 0.449 | 0.756 / 0.735 |
| GVRAM word write | 16442 / 16075 | 0.468 / 0.453 | 0.751 / 0.731 |
| TVRAM read | 11985 / 12051 | 0.649 / 0.453 | 0.782 / 0.730 |
| TVRAM write | 16676 / 16918 | 0.473 / 0.456 | 0.728 / 0.738 |
| TVRAM word write | 12433 / 12530 | 0.526 / 0.463 | 0.754 / 0.733 |
| SGP lists alone | 454, 489 / 461, 493 | | |

(PC-88VA2's RAM60 rows equal its RAM30 rows on the machine and in vaeg.)

## The sprite benchmarks after M165

vaeg against the machines, 15 kHz, 320x200 (ratios vaeg / machine):

| Program | PC-88VA | PC-88VA2 |
|---|---|---|
| SGPBENCH 16 colours (main RAM, GVRAM) | 0.94-1.02 | 0.98-1.01 |
| SGPBENCH 256 and 65536 colours | 0.91-0.98 | 0.97-1.02 |
| CPUBENCH `COMPLD` 16 and 256 colours | 0.97-1.04 | 0.97-1.03 |
| CPUBENCH `COMPLD` 65536 colours | 1.07-1.10 | 1.06-1.07 |
| CPUBENCH `LOOP` | 1.08-1.10 | 1.07-1.10 |
| MIXBENCH, all shares | 0.95-1.10 | 0.95-1.05 |
| DBLBENCH (counts of 2 and more) | 0.93-1.17 | 0.90-1.11 |

(Values under 2 sprites are left out: their rounding alone is 25% or
more. DBLBENCH's largest differences are `MIX` at 256 colours.)

## Open questions

- **Main RAM byte reads**: SGPMEM's read loop in main RAM takes 38.8-39.3
  clocks an iteration on the machines, 34.9 in vaeg (the M130 calibration
  loop with the same instructions measured 32.9 and is matched). The read
  loop starts at an odd address (the byte-write loop, at an even one,
  matches within 3%), so the cost of a branch to an odd address may be
  what is missing. A benchmark with each loop at an even and an odd
  address would separate the two. CPUBENCH's `LOOP` rows (reads from
  main RAM and GVRAM) are 7-10% fast for the same reason or a related one.
- **Reads lose less to the SGP than writes**: on both machines a CPU
  loop that reads (main RAM, GVRAM, TVRAM) keeps 0.08-0.19 more of its
  speed while the SGP draws from main RAM than a writing or register
  loop; vaeg charges every loop the same share.
- **GVRAM reads**: SGPMEM's GVRAM read loop is 4% fast in vaeg, while
  SGPCONT's G0 read row is 7% slow (and its G1 row 4% fast): the loops
  differ in alignment, so the read wait stays 7 until the alignment is
  measured.
- **SGPCONT P** (G1 writes while the SGP draws): 0.89 (PC-88VA) and
  0.92 (PC-88VA2) of the machines, from 0.95 and 0.99 before the BIU
  stall; the other contention rows are within 0.95-1.04.
- **65536-colour compiled sprites** remain 6-10% fast.
- **HLT while waiting for the SGP**: if halting the CPU (no instruction
  fetches) lets the SGP run faster on the machines, vaeg does not show it;
  not measured.
