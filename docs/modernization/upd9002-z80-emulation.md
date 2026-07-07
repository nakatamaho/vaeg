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
# uPD9002 Z80-Compatible Emulation Notes

This note separates the uPD9002 compatible-mode questions from the general
PC-88VA boot-sequence trace. The boot-sequence overview lives in
`pc88va-boot-sequence.md`.

The scope here is the main CPU's uPD780/Z80-compatible mode entered by the
PC-88VA `BRKEM2` instruction. This is not the FDD subsystem Z80. The FDD
subsystem CPU is a separate device implemented through `cpucva/z80c.cpp`,
`iova/subsystem.cpp`, and `VASUBSYS.ROM`.

## V30-Mode Instruction Set

The uPD9002 V30-mode instruction set should be treated as NEC V30-like
with a VA-specific exception list. The working technical notes say VA V30
mode does not support:

```text
INS
EXT
OUTM
INM
```

The same notes state that VA adds `BRKEM2 nn` (`0F FE nn`) for entry into
uPD780/Z80-compatible mode, and that VA does not use V30-compatible
`BRKEM nn` (`0F FF nn`).

This matters for disassembly. General x86 disassemblers may decode
`0F FE ...` as an MMX-family instruction when they are not told about
uPD9002. In PC-88VA/uPD9002 context, `0F FE nn` is `BRKEM2 nn`.

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
disabled.

Handler code that performs I/O must disable trapping around its own I/O
accesses to avoid nesting. The `FFE0h-FFFFh` control range itself must not
be trapped. I/O traps are software-handled and can slow trapped I/O by
orders of magnitude, so high-traffic ranges such as FDD or
PC-Engine-heavy ports are especially sensitive.

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

The `7Ch` and `7Dh` vectors match the I/O-trap vectors. The `90h` vector
is the important handoff clue: the ROM explicitly prepares vector `90h`
before executing `BRKEM2 90h`. That makes `90h` look intentional, not an
accidental immediate byte.

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

The static evidence is high that the bytes at `0x13B1` are intended as
`BRKEM2 90h`: the surrounding stream is valid 16-bit V30 setup code, the
bytes sit at an instruction boundary after `mov es,ax`, and `0F FE nn` is
the VA-specific handoff encoding. The vector setup above strengthens this:
the ROM has just installed vector `90h` as `1000:0000`, then executes
`BRKEM2 90h`.

What is not yet proven by this note is that every boot configuration
reaches this exact instruction. The nearby `000Dh` branch means a CS:IP
trace is still the right way to prove execution for a specific
ROM/configuration pair.

## V30 BRKEM Analogy

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
interaction still need VA/uPD9002-specific confirmation.

The important engineering consequence is already clear: `BRKEM2` is a
main-CPU mode transition and must not be routed to the separate FDD
subsystem Z80.

## V30 Resume After BRKEM2

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

That makes sense only if the compatible-mode handler eventually returns
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
side jumps there. That is another reason the missing main-CPU
uPD780/Z80-compatible mode matters: it may be responsible for preparing
the next-stage RAM code.

## Shared Native Initializer

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

## Emulator Status

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

## Confidence

| Item | Confidence | Reason |
| ---- | ---------- | ------ |
| Opcode identity | High | The ROM explicitly installs vector `90h -> 1000:0000` before executing `0F FE 90` at a valid instruction boundary, and the technical notes define `0F FE nn` as `BRKEM2 nn`. |
| Runtime path coverage | Medium-high | The observed path can reach the opcode, but the nearby `000Dh` branch can skip the block for some configurations. |
| V30 resume block | Medium-high | The code immediately after `BRKEM2` is coherent V30 code: disable I/O trap, send TSP/control commands, set bank state, set `SS=3000h`, and call `2210h`. By analogy with V30 `BRKEM`, `13B4h` is the expected saved return PC. |
| Compatible-mode return mechanics | Medium | The exact return-from-uPD780/Z80 opcode and latch mechanics have not been located yet. |
| RAM target preparation | Medium | `1000:C003` is clearly a RAM target, not ROM1 offset `C003h`. The static ROM search did not find a direct copy loop in this region, so the BRKEM2/vector-90 path is the leading hypothesis for preparing it, but this still needs execution tracing. |
