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

The uPD9002 V30-mode instruction set should be treated as NEC V30-like
with a VA-specific exception list. The working technical notes say VA V30
mode does not support `INS`, `EXT`, `OUTM`, or `INM`. They also state that
VA adds `BRKEM2 nn` (`0F FE nn`) for entry into uPD780/Z80-compatible
mode, and that VA does not use V30-compatible `BRKEM nn` (`0F FF nn`).

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

- By analogy with V50, `FFF0h-FFFFh` are expected to configure CPU pin
  functions, wait control, the internal DMA controller, interrupt
  controller, timer, and serial interface placement. The exact uPD9002
  mapping is still partly inferred.
- The settings above are documented for VA2 use; VA1 was explicitly not
  investigated in the referenced notes.
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

## I/O Trap Semantics

The I/O-trap mechanism is a uPD9002/VA compatibility feature, not a normal
PC-98 device. It raises a software-interrupt-style trap when selected
`IN` or `OUT` instructions execute. The documented purpose is V1/V2
compatibility: ports `50h-53h`, `60h-68h`, and `6Eh-6Fh` can be trapped
and emulated. V3 mode normally leaves this disabled, but software can
enable it through the control ports.

The trap control registers are:

```text
FFE0h/FFE2h  trap block 1 start/end
FFE4h/FFE6h  trap block 2 start/end
FFEFh        trap control
```

The trap-control byte enables IN traps, OUT traps, and byte-vs-word-port
matching. The range registers accept word accesses, but the high byte is
effectively zero; word-port trapping still matches on the low byte. Trap
handlers must therefore check the actual port and avoid accidentally
handling nearby ports that matched only by low byte.

The trap vectors are:

```text
7Ch  I/O trap for IN
7Dh  I/O trap for OUT
```

On trap entry, FLAGS, CS, and IP are saved, but the saved CS:IP points at
the trapping I/O instruction itself, not the next instruction. A correct
handler must decode the trapped instruction length and adjust the saved
return CS:IP before `IRET`; otherwise it will execute the same I/O
instruction again and loop forever. Trap entry also runs with interrupts
disabled. Handler code that performs I/O must disable trapping around its
own I/O accesses to avoid nesting.

The `FFE0h-FFFFh` control range itself must not be trapped. I/O traps are
software-handled and can slow trapped I/O by orders of magnitude, so
high-traffic ranges such as FDD or PC-Engine-heavy ports are especially
sensitive.

## Mode LEDs and V2S Area

The exact ROM routine that draws the visible `V2S` text has not been
identified in this note. However, the surrounding boot code clearly
touches the VA mode-switch and system-port state:

- `01C6h` writes `sysportva.modesw`, which is read back through
  `0150h/0151h`.
- `01CDh` and `01CFh` update `sysportva.c`; the emulator derives the
  V1/V2/V3 mode LEDs from bits 4..6 of that value and calls
  `sysmng_modeled()`.

The ROM-side evidence for this neighborhood is stronger than the display
string evidence. `VAROM1.ROM:0x12DC` writes a byte from `B000:1FC5` to
port `01C6h` during the early setup block. Later ROM code explicitly uses
port `01CFh` at `0x1459` and `0x2C70`; the `0x1459` routine also steps
down to port `01CDh` and writes the full system-port latch. The emulator
side then updates the visible mode LEDs from that latch.

No plain ASCII `V2S` string has been found in `VAROM1.ROM`, so the visible
`V2S` text is probably drawn through a ROM bitmap, font, or text routine
rather than as a direct ASCII literal. That keeps the exact text-drawing
site unresolved.

Therefore the `V2S` display and the V1/V2/V3 lamp state are expected to be
controlled by the ROM in this early V30-side setup or immediately after
the handoff described below. They should not be attributed to the FDD
subsystem Z80.

## Model and Configuration Caveat

The ROM observations above were made against a PC-88VA2 ROM image. Do not
generalize every visible boot symptom to every VA model yet.

Maintainer hardware memory adds one important distinction: on an original
PC-88VA with no FDD inserted at power-on, the machine does not show the
same `V2S`-style path. It falls back to V1/V2 behavior, controlled by DIP
switch configuration. On PC-88VA2/3-class machines, the no-disk path is
reported to stop at a prompt equivalent to "insert a system disk" instead.

The setup UI is entered by holding the PC key during power-on or reset.
That is a configuration path, not necessarily the same path as the normal
FDD boot or the no-media fallback path.

The current note should therefore be read as a VA2-centered ROM trace with
known model-dependent behavior. Original PC-88VA ROM behavior may differ,
and PC-88VA3 behavior is still unverified here.

The ROM-visible model identification method reported for VA software is
to read the word at `F000:FFFE`:

```text
FFFFh  original PC-88VA
FFFEh  PC-88VA2 or PC-88VA3
FFFDh  original PC-88VA with the PC-88VA-91 version-up board
```

This marker explains why software can branch between the original VA,
VA2/VA3, and VA-91-upgraded environments without relying only on external
DIP-switch state. It also reinforces the caveat above: the current
disassembly notes are centered on a VA2 ROM, while original VA and VA-91
paths can legitimately differ.

## BRKEM2 Handoff Candidate

Before reaching the candidate handoff, the ROM installs interrupt vectors.
The relevant routine starts at `VAROM1.ROM:0x13ED` and is called at
`0x1364`. It first fills the interrupt-vector table with safe default
handlers, then applies a small override table at `VAROM1.ROM:0x0F5E`.
The important entries in that table are:

```text
7C -> F000:1944
7D -> F000:1944
7E -> F000:1920
90 -> 1000:0000
91 -> F000:24B0
95 -> 1000:E000
```

The `7Ch` and `7Dh` vectors match the technical-manual I/O-trap vectors
for IN and OUT traps. The `90h` vector is the important handoff clue:
the ROM explicitly prepares vector `90h` before executing `BRKEM2 90h`.
That makes `90h` look intentional, not an accidental immediate byte.

There is one branch before the handoff. After installing vectors, the ROM
tests port `000Dh` bit 2 through the helper at `0x13E6`:

```asm
1364: call 13edh       ; install vectors
1367: call 13e6h       ; read port 000Dh, mask 04h
136a: jz   13d2h       ; alternate path, no BRKEM2 in this block
```

The non-zero path enables I/O traps by calling `0x18E7`, which writes
`03h` to `FFEFh`, then runs the memory/display setup immediately before
the handoff. The zero path sets `[2F07h]=1`, writes `91h` to port `FFh`,
runs the shared later initialization, and jumps back into the ROM instead
of executing the `BRKEM2 90h` sequence in this block.

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
is `90h`. The related V30-compatible `BRKEM nn` encoding is `0F FF nn`,
but the VA notes state that PC-88VA uses `BRKEM2` instead of `BRKEM` for
this transition.

The static evidence is high that the bytes at `0x13B1` are intended as
`BRKEM2 90h`: the surrounding stream is valid 16-bit V30 setup code, the
bytes sit at an instruction boundary after `mov es,ax`, and `0F FE nn` is
the VA-specific handoff encoding. The vector setup above strengthens this:
the ROM has just installed vector `90h` as `1000:0000`, then executes
`BRKEM2 90h`. What is not yet proven by this note is that every boot
configuration reaches this exact instruction. The nearby `000Dh` branch
means a CS:IP trace is still the right way to prove execution for a
specific ROM/configuration pair.

The closest documented CPU-side analogue is the NEC V30 `BRKEM imm8`
instruction (`0F FF imm8`). `BRKEM` is not a normal software interrupt:
it switches the V30 from native mode into 8080 emulation mode. Its useful
behavioral model is:

```text
BRKEM imm8:
  save PSW
  switch the mode latch to emulation mode
  save PS
  save PC after the immediate byte
  load new PS:PC from IVT[imm8]
  flush prefetch and fetch the next instruction as emulation-mode code
```

The saved return address is therefore the native instruction immediately
after the three-byte `BRKEM`. Return from that mode is not a plain far
return; V30 has `RETEM`, and temporary native calls from 8080 mode use the
`CALLN`/`RETI` path. Correct emulation needs a mode latch, a
write-enable/write-disable style latch for the mode flag, and a prefetch
flush on every mode-changing control transfer. Treating `BRKEM` as a
plain interrupt or far call is insufficient.

Applying that as a working analogy, `BRKEM2 90h` is expected to be the
uPD9002-specific sibling:

```text
BRKEM2 90h:
  save native V30/uPD9002 state
  switch to uPD780/Z80-compatible mode
  save PC = 13B4h
  load PS:PC from IVT[90h] = 1000:0000
  flush prefetch and fetch the next instruction as compatible-mode code
```

This analogy is strong enough to guide emulator design, but it is not yet
a complete uPD9002 specification. The exact compatible-mode return opcode,
the exact mode-latch write-protection semantics, and the interrupt
interaction still need VA/uPD9002-specific confirmation. The important
engineering consequence is already clear: `BRKEM2` is a main-CPU mode
transition and must not be routed to the separate FDD subsystem Z80.

After the `BRKEM2` byte sequence, the following V30-side code disables the
I/O trap by writing `00h` to `FFEFh`, then continues with more bank and
control setup. That post-handoff path only makes sense after the
uPD780/Z80-compatible handler returns control to the V30 side.

The return model is therefore an inference from control flow and the
uPD9002 mode-transition definition. The post-`BRKEM2` bytes disassemble as
ordinary V30 code beginning at `0x13B4`:

```asm
13b4: cli
13b5: call 18eeh    ; writes 00h to FFEFh, disabling I/O trap
13bd: mov dx,0152h
13c0: in ax,dx
13c1: or ax,4000h
13c4: out dx,ax
13cd: jmp 1000h:c003h
```

That makes sense only if the compatibility-mode handler eventually returns
to the V30 stream after the three-byte `BRKEM2 90h` instruction. The exact
return instruction or ROM handler that performs this return has not yet
been identified.

The far jump at `0x13CD` is also important:

```asm
13cd: jmp 1000h:c003h
```

This target is physical address `0x1C003`, not `VAROM1.ROM` offset
`0xC003`. It is therefore RAM-side code from the CPU's point of view. The
static ROM search found the same byte pattern only once in `VAROM1.ROM`
and found no direct `mov si,C000h` / `mov di,C000h` copy loop in this ROM
region. The working hypothesis is that the `BRKEM2 90h` compatibility path
or another RAM setup routine prepares the `1000:C003` code before the V30
side jumps there. That is another reason the missing main-CPU uPD780/Z80
mode matters: it may be responsible for preparing the next-stage RAM code.

## Post-BRKEM2 V30 Resume

The shared post-handoff initializer starts at `0x2210`; both the BRKEM2
resume path and the alternate `0x13D2` path call it after setting
`SS=3000h`.

`0x2210` is a dispatcher:

```asm
2210: call 2220h
2213: call 0e84h
2216: call 2233h
2219: call 240bh
221c: call 2252h
221f: ret
```

The subroutines perform these visible tasks:

- `0x2220`: emits a 16-entry port table. The parsed table writes
  `01CDh=B8h`, then initializes slave/master PIC ports
  `0184h/0186h` and `0188h/018Ah`; it also writes `0158h=00h`.
- `0x0E84`: installs 51 interrupt vectors from the table at
  `0x0DD9`, including BIOS/service vectors `80h`, `81h`, `88h`,
  `89h`, `8Ah`, `92h`, `B8h`, `B9h`, and others.
- `0x2233`: clears low memory under segment `0040h` and initializes
  several work bytes used by the later BIOS path.
- `0x240B`: temporarily maps/opens the `B000:1FC0` backup-memory area,
  checks ports `01E0h` and `01F0h`, updates flags/checksum bytes in the
  backup area, then restores the ROM bank and backup write-protect state.
- `0x2252`: runs the larger BIOS/device initialization sequence. It calls
  many service routines through the early ROM jump table, sends TSP
  commands through `0x18C8`, adjusts PIC masks, executes BIOS interrupts
  such as `INT 80h`, `INT 81h`, `INT 89h`, and `INT B9h`, and finally
  calls far ROM service `F000:9400`.

The `0x2252` path is conditional on `[2F07h]`. The alternate branch at
`0x13D2` sets `[2F07h]=1`; the BRKEM2 path leaves it clear. When clear,
`0x2252` executes the broader interrupt/service initialization before the
later `1000:C003` jump.

Current emulator status:

- The FDD subsystem Z80 is implemented separately through
  `cpucva/z80c.cpp`, `iova/subsystem.cpp`, and `VASUBSYS.ROM`.
- The main CPU's uPD780/Z80-compatible execution mode required by
  `BRKEM2` is not implemented.
- `docs/agents/reports/m9_v30_map.md` records this as an explicit future
  implementation item. The `BRKEM2` path must not be wired to the FDD
  subsystem Z80.
- A future implementation should model `BRKEM2` like a mode-changing
  control transfer, not like an ordinary software interrupt: save the
  post-immediate native return address, switch the decoder, flush
  prefetch, and keep the main-CPU compatible-mode state separate from the
  FDD subsystem CPU state.

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
7. The ROM installs vector `90h -> 1000:0000`, enables I/O traps, and
   reaches `BRKEM2 90h` at offset 13B1h on the non-zero `000Dh & 04h`
   path.
8. Returning from that compatibility path resumes V30 code at `13B4h`,
   the instruction immediately after `BRKEM2`, then disables I/O traps,
   updates bank/control state, and calls the shared initializer at
   `2210h`.
9. The shared initializer programs system/PIC ports, installs BIOS
   interrupt vectors, updates low-memory and backup-memory state, runs
   BIOS/device initialization, and returns.
10. The BRKEM2 path then jumps to RAM at `1000:C003`; the producer of
   that RAM code remains to be proven, with the BRKEM2 handler the leading
   hypothesis.
```

The unresolved emulator gap is step 7: the project currently emulates the
FDD subsystem Z80, but not the main CPU uPD780/Z80-compatible mode entered
by `BRKEM2`.

## Confidence

| Step | Confidence | Reason |
| ---- | ---------- | ------ |
| 1-5 | High | These are direct reset-vector, memory-map, ROM-byte, and I/O-port observations, and they line up with current source bindings. |
| 6 | Medium | The ROM definitely updates the mode-switch/system-port neighborhood, and the emulator definitely derives LEDs from `sysportva.c`. The exact `V2S` drawing routine is still unidentified. |
| 7 | High for opcode identity; medium-high for runtime execution | The ROM explicitly installs vector `90h -> 1000:0000` before executing `0F FE 90` at a valid instruction boundary. The V30 `BRKEM` analogy strongly supports interpreting this as a mode-changing control transfer that saves the post-immediate return address. The remaining uncertainty is path coverage: the nearby `000Dh` branch can skip this block. |
| 8 | Medium-high for the V30 resume block | The code immediately after `BRKEM2` is coherent V30 code: disable I/O trap, send TSP/control commands, set bank state, set `SS=3000h`, and call `2210h`. By analogy with V30 `BRKEM`, `13B4h` is the expected saved return PC. The exact return-from-uPD780/Z80 opcode and latch mechanics have not been located yet. |
| 9 | Medium-high | `2210h` and its callees are statically visible and line up with emulator port bindings and interrupt-vector setup. Semantic labels for every BIOS service call are not complete. |
| 10 | Medium | `1000:C003` is clearly a RAM target, not ROM1 offset `C003h`. The static ROM search did not find a direct copy loop in this region, so the BRKEM2/vector-90 path is the leading hypothesis for preparing it, but this still needs execution tracing. |
