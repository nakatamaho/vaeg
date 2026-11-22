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

# FDD subsystem as a coprocessor

`FDDSUB.COM` (`fddsub.asm`), an MS-DOS program for a PC-88VA or PC-88VA2
(and vaeg), measures the FDD subsystem (a uPD780C at 4 MHz with its own
RAM, joined to the main CPU by a pair of 8255s) used as a coprocessor:
how fast bytes pass through the subsystem monitor's commands and through
a program of its own run in the subsystem's RAM, and how fast that CPU
runs. Synthetic data only; the subsystem program is assembled by
`gen_data.py`. The protocol is vaeg's
[`docs/bios/fdc_monitor.md`](../../docs/bios/fdc_monitor.md) (M140):
8255 mode 91h on both sides, DAV/RFD/DAC handshake, monitor commands 0Ch
(store), 0Bh (return) and 0Dh (execute).

Each row is measured in turn, every pass (`FDDSUB P`, 8 passes by
default, about 7 minutes), with mean, minimum, maximum and standard
deviation; a value is the blocks done in 300 VRTCs unless said otherwise:

| Row | Work |
|---|---|
| A, M | CPU loop (ADD, DEC, JNZ): the clock's yardstick, first and last |
| B, C | monitor commands 0Ch and 0Bh: 256 bytes to and from 6000h |
| D, E | the program at 4000h (stored by 0Ch, started by 0Dh): 256 bytes to it (and its answer), 256 bytes back |
| F | a round trip: one byte out, one back |
| G | a frame's exchange: 64 bytes out, 32 back |
| H-J | D, E and G two-phase: the sender puts a byte out and inverts its DAV, the receiver reads it and inverts its DAC (the command byte four-phase) |
| K | the subsystem counts 256-step DJNZ loops while the main CPU waits 300 VRTCs (at 4 MHz a loop is about 3360 states) |
| L | bytes of E and I different from those D and H sent (0 is right) |

The title line ends with the machine, from the ROM's word at F000:FFFE:
VA (FFFFh), VA2/3 (FFFEh), VA+91 (FFFDh, the PC-88VA-91 board), or ? and
the word.

After each pass the program returns to the monitor (`JP 002Ah`). The
program needs a 15 kHz screen and memory up to segment 3000h, copies
itself to 2000:0000 and does not return: restart the machine to leave. A
subsystem that does not answer stops it with a message.

```sh
demos/fdd-subsystem/build.sh [--msdos4 msdos4-pc88va-2hd.d88] [OUTDIR]
```

With `--msdos4` (the FreeDOS-88VA msdos4-va.2 disk) the script also writes
`fdd-subsystem-msdos4.d88`, a vanilla MS-DOS 4.0 copy with the program;
type `FDDSUB`.

## The machines and vaeg (2026-10-28)

Means of 8 passes, photographed on a PC-88VA and a PC-88VA2 (SD at most
2.9); vaeg (main 719b4d4a, the same on both models) with 2 passes:

| Row | VA | VA2 | vaeg | vaeg / machines |
|---|---:|---:|---:|---:|
| A CPU loop | 1827 | 1843 | 1768 | 0.96-0.97 |
| B monitor write | 195 | 194 | 194 | 1.00 |
| C monitor read | 228 | 228 | 228 | 1.00 |
| D own write | 284 | 284 | 292 | 1.03 |
| E own read | 275 | 273 | 290 | 1.05-1.06 |
| F round trip | 35246 | 35343 | 38664 | 1.09-1.10 |
| G frame, four-phase | 805 | 808 | 841 | 1.04 |
| H two-phase write | 683 | 683 | 743 | 1.09 |
| I two-phase read | 646 | 662 | 719 | 1.09-1.11 |
| J frame, two-phase | 1755 | 1766 | 1922 | 1.09-1.10 |
| K subsystem loops | 5938 | 5929 | 5930 | 1.00 |
| L read errors | 0 | 0 | 0 | |

- The subsystem CPU runs at vaeg's speed (K), and the monitor's own
  transfers match (B, C).
- The rows the main CPU paces with tight polling loops run 3-11 % fast in
  vaeg, while its CPU loop (A, M) runs 3-4 % slow: vaeg's main-CPU cost of
  a loop of 8255 accesses at FCh-FFh is low.
- A 64 + 32-byte exchange costs the main CPU about 22800 clocks two-phase
  and 49700 four-phase on the machines: too much to hand a 60 Hz program's
  work to the subsystem a frame.
