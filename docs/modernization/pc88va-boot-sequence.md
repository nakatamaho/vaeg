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

# PC-88VA Boot Sequence Notes

This note records the current boot-sequence understanding from the M9/M11
PC-88VA investigation. It is not a ROM listing. ROM images are not
distributed by this repository; the byte offsets below refer to the
maintainer's `VAROM1.ROM` used during debugging.

## Reset Entry

The VA path starts in V30/uPD9002 native mode. `pccore_reset()` forces
`CPU_TYPE = CPUTYPE_V30` for VA models, and the C CPU reset path
initializes:

```text
CS:IP = F000:FFF0
physical = 0xFFFF0
```

Relevant source paths:

- `pccore.c`: VA reset selects `CPUTYPE_V30` and later restores the reset
  vector.
- `i286c/i286c.c`: `i286c_initreg()` sets `CS=F000h`, `IP=FFF0h`.
- `i286x/i286x.cpp` and `i286x/v30patch.cpp`: legacy reference paths use
  the same reset vector.

For VA memory, physical `0xF0000-0xFFFFF` is ROM1. `biosva/biosva.c`
loads `VAROM1.ROM` into `rom1mem`, and `cpucva/memoryva.c` maps ROM1
reads through the current `memoryva.rom1_bank`. At reset the bank state is
zero, so `0xFFFF0` reads from `VAROM1.ROM` offset `0xFFF0`.

The observed bytes at `VAROM1.ROM:0xFFF0` are:

```text
EA 00 00 00 F0
```

In 16-bit V30/x86 terms this is:

```asm
jmp F000:0000
```

Therefore the first practical ROM entry after reset is `F000:0000`,
which corresponds to `VAROM1.ROM` offset `0x0000`.

## ROM1 Offset 0x0000

`VAROM1.ROM:0x0000` begins with a jump table. The first entry is:

```asm
0000: e9 b5 12    jmp 12b8h
```

The first reset path therefore becomes:

```text
F000:FFF0 -> F000:0000 -> F000:12B8
```

The remaining early table entries are near jumps to other ROM services.
They should be treated as a dispatch table, not as straight-line startup
code.

## Initialization Body at 0x12B8

The code at `VAROM1.ROM:0x12B8` performs early V30-side hardware setup.
The observed sequence begins:

```asm
12b8: mov ax,0040h
12bb: mov ds,ax
12bd: mov sp,0000h
12c0: call 1799h
```

`0x1799` sends a 14-byte parameter block through ports `0142h` and
`0146h`; these are TSP ports in `iova/tsp.c`. It then writes `0020h` to
port `0100h`, a VA video port handled by `iova/videova.c`. This is the
first visible display-system setup.

The code then sets `ES=B000h` and accesses the VA memory-control and
backup-RAM control ports:

- `0152h`: ROM bank control, implemented by `iova/memctrlva.c`
- `019Ah`: backup RAM write-protect clear, implemented by
  `iova/memctrlva.c`

It calls a routine around `0x23A1`, which checks and initializes data near
`B000:1FC0`. That area corresponds to VA backup RAM / memory-switch
state. The same early code later writes port `01C6h`; in the current
emulator this updates `sysportva.modesw` in `iova/sysportva.c`, and ports
`0150h/0151h` read that mode switch state.

## uPD9002 and Trap Setup

At `0x12F0`, the ROM walks a table at `VAROM1.ROM:0x0F20` and emits
groups of descending port writes. Parsed as `(count, start port, bytes)`,
the table contains:

```text
FFFE=11 FFFD=07 FFFC=01 FFFB=60 FFFA=88 FFF9=A0
FFF6=08 FFF5=80 FFF4=53
FFF2=08
FFF0=00
FFE7=00 FFE6=6F FFE5=00 FFE4=60 FFE3=00 FFE2=5B FFE1=00 FFE0=50
```

These values match the uPD9002/VA internal-control and I/O-trap area
described by the technical-manual notes discussed during debugging:

- `FFF0h` is the uPD9002 timing-control port in the current emulator
  (`iova/upd9002.c`).
- `FFE0h-FFE7h` are the two I/O-trap port-range registers.
- `FFEFh` is the I/O-trap control port.

This strongly suggests that the ROM is preparing the V1/V2 compatibility
I/O-trap environment before entering the compatibility path.

The next table starts at `VAROM1.ROM:0x0F43`:

```text
01A6=76
01A2=82
01A2=06
0188=11
018A=00
018A=80
018A=03
018A=7F
0160=01
```

The current emulator maps these ports as:

- `01A2h/01A6h`: PIT/timer ports in `io/pit.c`
- `0188h/018Ah`: VA PIC master ports in `io/pic.c`
- `0160h`: VA DMAC control in `io/dmac.c`

So this block initializes timers, interrupt control, and DMA state.

## Mode LEDs and V2S Area

The exact ROM routine that draws the visible `V2S` text has not been
identified in this note. However, the surrounding boot code clearly
touches the VA mode-switch and system-port state:

- `01C6h` writes `sysportva.modesw`, which is read back through
  `0150h/0151h`.
- `01CDh` and `01CFh` update `sysportva.c`; the emulator derives the
  V1/V2/V3 mode LEDs from bits 4..6 of that value and calls
  `sysmng_modeled()`.

Therefore the `V2S` display and the V1/V2/V3 lamp state are expected to be
controlled by the ROM in this early V30-side setup or immediately after
the handoff described below. They should not be attributed to the FDD
subsystem Z80.

## BRKEM2 Handoff Candidate

Near `VAROM1.ROM:0x13A8`, the ROM prepares segment state and then emits
the VA-specific mode-transition opcode:

```asm
13a8: mov ax,1000h
13ab: mov ds,ax
13ad: xor ax,ax
13af: mov es,ax
13b1: 0f fe 90    ; BRKEM2 90h
13b4: cli
```

General x86 disassemblers may decode `0F FE ...` as an MMX instruction
when they are not told about uPD9002. In PC-88VA/uPD9002 context, the
technical-manual correction is that `0F FE nn` is `BRKEM2 nn`, the
transition from V30 mode to uPD780/Z80-compatible mode. The `nn` byte here
is `90h`.

After the `BRKEM2` byte sequence, the following V30-side code disables the
I/O trap by writing `00h` to `FFEFh`, then continues with more bank and
control setup. That post-handoff path only makes sense after the
uPD780/Z80-compatible handler returns control to the V30 side.

Current emulator status:

- The FDD subsystem Z80 is implemented separately through
  `cpucva/z80c.cpp`, `iova/subsystem.cpp`, and `VASUBSYS.ROM`.
- The main CPU's uPD780/Z80-compatible execution mode required by
  `BRKEM2` is not implemented.
- `docs/agents/reports/m9_v30_map.md` records this as an explicit future
  implementation item. The `BRKEM2` path must not be wired to the FDD
  subsystem Z80.

## Working Boot Sequence Summary

The current working model is:

```text
1. Reset selects V30/uPD9002 native mode.
2. CPU starts at F000:FFF0.
3. ROM1 reset vector jumps to F000:0000.
4. ROM1 offset 0000h jumps to offset 12B8h.
5. V30 ROM code initializes display/TSP, video, ROM banks, backup RAM,
   uPD9002 control ports, I/O traps, PIT, PIC, and DMAC.
6. The ROM updates mode-switch / system-port state near the same phase;
   this is the likely neighborhood for V2S display and mode-lamp setup.
7. The ROM reaches `BRKEM2 90h` at offset 13B1h, which should enter the
   main CPU uPD780/Z80-compatible mode.
8. Returning from that compatibility path should resume the V30-side code
   after the BRKEM2 instruction.
```

The unresolved emulator gap is step 7: the project currently emulates the
FDD subsystem Z80, but not the main CPU uPD780/Z80-compatible mode entered
by `BRKEM2`.
