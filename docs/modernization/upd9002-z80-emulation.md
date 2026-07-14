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

## Reference Materials

This note uses these materials with different authority levels:

| Source | Role in this note | Link |
| ------ | ----------------- | ---- |
| NEC V-series instruction manual, `U11301EJ5V0UMJ1` | Primary public reference for V20/V30-family `BRKEM`, `RETEM`, and `CALLN` behavior. It is an analogy source for `BRKEM2`, not a uPD9002-specific manual. | <https://datasheets.chipdb.org/NEC/V20-V30/U11301EJ5V0UMJ1.PDF> |
| NEC V40HL/V50HL data sheet, `U13225EJ4V0DS00` | Public V40/V50-family analogy source for the on-chip peripheral model: V20/V30 software compatibility, uPD8080AF emulation, 1MB memory space, 64KB I/O space, and system-I/O allocation of DMA/ICU/TCU/SCU. uPD9002 is treated here as the PC-88VA V52-class CPU; this data sheet is only a public family analogy, not a uPD9002/V52 manual. | directory: <https://datasheets.chipdb.org/NEC/V40-V50/>; V40HL PDF: <https://datasheets.chipdb.org/NEC/V40-V50/NEC_uPD70208H.pdf>; V50HL PDF: <https://datasheets.chipdb.org/NEC/V40-V50/NEC_uPD70216H.pdf> |
| PC-88VA technical notes under `docs/tekumani/` | Primary local notes for PC-88VA/uPD9002 differences: unsupported V30-mode instructions, `BRKEM2` opcode, internal-control ports, and I/O trap behavior. | local repository material |
| *PC-88VA Technical Manual*, page 12 | Direct description of the two CPU modes, compatible-mode instruction coverage, `CALLN 91h`, `CALLN 95h`, and `RETEM`. The scan's OCR is noisy, so this note normalizes typography and register notation against the page image. | Internet Archive scan: <https://archive.org/details/PC88VA/page/12/mode/2up> |
| MAME NEC V20/V30 core | Implementation comparison point for how a mature emulator models V30 `BRKEM`, 8080-mode dispatch, `RETEM`, `CALLN`, and prefetch flushing. MAME does not implement VA `BRKEM2` as a uPD9002-compatible-mode entry. | source directory: <https://github.com/mamedev/mame/tree/master/src/devices/cpu/nec>; `nec.cpp`: <https://github.com/mamedev/mame/blob/master/src/devices/cpu/nec/nec.cpp>; `necpriv.ipp`: <https://github.com/mamedev/mame/blob/master/src/devices/cpu/nec/necpriv.ipp>; `necinstr.hxx`: <https://github.com/mamedev/mame/blob/master/src/devices/cpu/nec/necinstr.hxx>; `nec80inst.hxx`: <https://github.com/mamedev/mame/blob/master/src/devices/cpu/nec/nec80inst.hxx> |
| `VAROM1.ROM` disassembly observations | PC-88VA2 ROM-specific evidence for vector setup, `0F FE 90`, and the post-handoff V30 resume block. | maintainer-provided ROM, not distributed here |

## Dual-Mode CPU Model

The uPD9002 is a PC-88VA-specific CPU with two execution modes:

| Mode | Role |
| ---- | ---- |
| V30 mode | Native 16-bit execution, with a V30-like instruction set and VA-specific differences. |
| uPD780 mode | uPD780/Z80-compatible execution used by V1/V2 compatibility software. |

The technical manual explicitly compares this relationship to the V30 native
mode and 8080 emulation mode. It also says that the mode-transition method is
the same in principle. This strengthens the use of V30 `BRKEM`, `CALLN`, and
`RETEM` as behavioral references, while `BRKEM2` remains the uPD9002-specific
entry instruction.

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

The uPD9002 also integrates DMA-controller, interrupt-controller, and timer
functions analogous to the V50 family. This is direct support for treating
the high system-I/O setup performed by the VA ROM as CPU-internal peripheral
configuration, although the exact uPD9002 register map remains VA-specific.

This matters for disassembly. General x86 disassemblers may decode
`0F FE ...` as an MMX-family instruction when they are not told about
uPD9002. In PC-88VA/uPD9002 context, `0F FE nn` is `BRKEM2 nn`.

## V52 Lineage And V40/V50 Peripheral Analogy

For PC-88VA work, uPD9002 should be treated as the VA's V52-class CPU,
not as a literal V50. The V40/V50 data-sheet directory still matters
because it contains public V-series material for the same broad design
family. The V40HL and V50HL PDFs are the combined `uPD70208H, 70216H`
data sheet.

That data sheet is not a uPD9002/V52 manual. Use it only as supporting
evidence for the family-level model also described by the PC-88VA notes:
V30-like native execution plus CPU-internal peripherals mapped into a
system I/O area. Exact uPD9002/V52 register meanings remain VA-specific
and must come from ROM traces, technical notes, or future hardware
confirmation.

| Data-sheet point | Why it matters here |
| ---------------- | ------------------- |
| V40HL/V50HL are V20/V30 software-compatible processors. | This supports treating uPD9002/V52 native mode as V30-like before applying VA-specific differences. |
| The data sheet lists a uPD8080AF emulation function. | This makes the V30-family `BRKEM` analogy plausible, while still leaving `BRKEM2` as a VA/uPD9002-specific opcode. |
| The devices include on-chip CG, WCU, REFU, TCU, SCU, ICU, and DMAU blocks. | This matches the kind of CPU-internal peripheral block that the PC-88VA notes describe for uPD9002. |
| The devices expose a 1MB memory address space and 64KB I/O address space. | This matches the broad address-space assumptions used by the VA ROM setup code. |
| The data sheet describes internal peripherals as mapped into a system I/O area. | This supports the interpretation that VA ROM writes to `FFF0h-FFFFh` are CPU-internal control setup, but the exact uPD9002/V52 register meanings remain VA-specific and partly inferred. |

## uPD780-Compatible Mode And Native Services

The technical manual states that uPD780 mode supports the complete documented
uPD780 instruction set and many undefined instructions. It additionally
supports the mode-control instructions needed to call V30 code and return to
V30 mode:

```text
CALLN nn  ED ED nn
RETEM     ED FD
```

This compatibility is the architectural basis for running most software
written for earlier PC-8800-series machines. It is main-CPU execution, not an
invocation of the independent FDD subsystem Z80.

The VA provides native-service entry points so V1/V2-compatible code can
access V3 memory and I/O or call native V30 code.

### CALLN 91h: V3 Memory And I/O Access

`CALLN 91h` is encoded as `ED ED 91`. The caller selects the operation in
register `A`:

| A bit | Clear | Set |
| ----- | ----- | --- |
| 2 | Memory access | I/O access |
| 1 | Read/input | Write/output |
| 0 | Byte operation | Word operation |

The remaining register ABI is:

| Register | Meaning |
| -------- | ------- |
| `HL` | Segment for memory access; unused for I/O access. |
| `DE` | Memory offset or I/O port address. |
| `BC` | Value for write/output; result for read/input. |

The ROM vector table maps interrupt vector `91h` to `F000:24B0`, matching
this documented service number.

### CALLN 95h: User Native Routine

`CALLN 95h` is encoded as `ED ED 95`. It calls the user native routine at
`1000:E000`, a location corresponding to address `E000h` as seen from V1/V2
mode. Compatible-mode software can therefore prepare a native routine in
that shared location and invoke it with `CALLN 95h`.

The native routine returns to uPD780-compatible mode with `IRET`, not
`RETEM`. The ROM vector table maps vector `95h` directly to `1000:E000`,
which independently agrees with the documented ABI.

### RETEM: Return To V30 Mode

`RETEM` is encoded as `ED FD` and returns from uPD780-compatible mode to the
saved V30-mode context. The documented V1/V2-to-V3 transition performs V3
initialization after returning and then jumps to `1000:C003`.

This explains the otherwise unusual RAM-side target observed after the ROM's
`BRKEM2 90h` sequence. It confirms the return opcode and high-level path, but
does not by itself define the exact uPD9002 mode-latch write-protection,
prefetch, interrupt, or cycle-timing details.

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

## MAME V30 Implementation Comparison

MAME is useful because it already models the standard NEC V30-compatible
emulation path. It should not be treated as a drop-in answer for PC-88VA,
because VA uses `BRKEM2` (`0F FE nn`) rather than V30 `BRKEM`
(`0F FF nn`) for the main-CPU uPD780/Z80-compatible transition.

| Topic | MAME V30 behavior | PC-88VA/uPD9002 implication |
| ----- | ----------------- | --------------------------- |
| Instruction family | MAME documents V20/V30/V40/V50 as having dedicated emulation instructions: `BRKEM`, `RETEM`, and `CALLN`. | This supports using V30 `BRKEM` as the closest public analogy, but it does not prove the VA-specific `BRKEM2` entry frame or latch rules. |
| Native entry opcode | MAME dispatches `0F FF nn` to `BRKEM` and calls its common break-to-emulation helper. | VA technical notes say the PC-88VA transition is `0F FE nn`; `0F FF nn` is not the VA path. A vaeg implementation needs a new `BRKEM2` case, not only MAME-style `BRKEM`. |
| Mode state | MAME keeps a native/emulation decode selector (`m_MF`) and a separate latch-like state (`m_em`) that affects whether restored flags can leave emulation mode. | vaeg should also keep compatible-mode state separate from normal V30 flags. A single boolean "currently Z80" flag would be too weak for correct return/interrupt behavior. |
| Entry stack frame | MAME's `nec_brk()` clears the native-mode flag, pushes flags, `PS`, and the post-immediate `IP`, then loads the new `PS:IP` from `IVT[nn]`. | The working `BRKEM2 90h` model should save return `IP=13B4h` and load `PS:IP` from vector `90h = 1000:0000`, unless uPD9002-specific evidence proves otherwise. |
| Decoder switch | MAME's execution loop chooses the native instruction table when the mode flag is set and the 8080 table when it is clear. | The main CPU needs its own compatible-mode decoder. Reusing the FDD subsystem Z80 instance would be architecturally wrong. |
| Return from emulation | MAME implements `RETEM` in the 8080-mode table through an `ED FD` sequence that restores `IP`, `PS`, and flags and returns to native decode. `CALLN` is handled through `ED ED nn`. | The PC-88VA technical manual confirms these uPD9002 encodings. MAME remains an implementation analogy for the detailed frame, latch, and prefetch behavior rather than proof of those details. |
| Prefetch and control transfer | MAME's control-transfer macro clears the prefetch state on `BRKEM`, `RETEM`, calls, jumps, and returns. | `BRKEM2` should flush any native prefetch state before fetching compatible-mode code, and should flush again when returning to V30 mode. |
| Timing | MAME assigns a concrete cycle cost to V30 `BRKEM`, while its source comments still treat some V30 prefetch details as approximate. | For vaeg, functional correctness of the mode transition should be separated from future cycle-accuracy work. Timing must not be copied blindly from MAME for uPD9002. |

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

This analogy is strong enough to guide emulator design, but it is not a
complete uPD9002 specification. The technical manual confirms `RETEM` as
`ED FD` and `CALLN nn` as `ED ED nn`; the exact saved frame, mode-latch
write-protection semantics, prefetch behavior, cycle timing, and interrupt
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
to the V30 stream after the three-byte `BRKEM2 90h` instruction. The
technical manual identifies `RETEM` (`ED FD`) as that architectural return
instruction. The exact compatible-mode ROM location that executes it has
not yet been identified.

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
| Compatible-mode return opcode | High | The PC-88VA technical manual defines `RETEM` as `ED FD` and `CALLN nn` as `ED ED nn`. |
| Compatible-mode frame and latch mechanics | Medium | The manual excerpt establishes the transitions but does not fully specify the saved frame, mode-latch write protection, prefetch, interrupt interaction, or cycle timing. |
| RAM target preparation | Medium | `1000:C003` is clearly a RAM target, not ROM1 offset `C003h`, and the manual confirms it as the post-V3-initialization destination. The static ROM search did not find a direct copy loop in this region, so the BRKEM2/vector-90 path remains the leading hypothesis for preparing it, but this still needs execution tracing. |
