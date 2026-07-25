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

# μPD9002 μPD780 (Z80-Compatible) Mode

Reference for the PC-88VA main CPU's non-native execution mode.

Merged from `upd9002-z80-emulation.md` and the provenance-tagged hardware
reference. Supersedes both.

**Scope.** The μPD9002 main CPU: its two execution modes, the
instructions and signals that move between them, the PC-88VA firmware
conventions layered on top, and what a correct emulation must contain.

**Not in scope.** The FDD subsystem Z80. That is a separate physical
device with its own register file and ROM (`VASUBSYS.ROM`), reached in
vaeg through `cpucva/z80_core.cpp` and `iova/subsystem.cpp`. It has
nothing to do with μPD780 mode, and the two must not share code or
state. The general PC-88VA boot trace lives in
`pc88va-boot-sequence.md`; only the parts bearing on mode transition are
repeated here.

---

## 0. How to read this document

Every factual claim carries a provenance tag. The tags determine what may
be implemented as fact, what must sit behind a flip point, and what must
not be implemented at all. Prior versions of these notes mixed authority
levels, and every error found so far lived on exactly that boundary.

| Tag | Meaning | Implementation rule |
|---|---|---|
| `[V30-MAN]` | NEC V20/V30-family documentation. The μPD9002's native mode *is* a V30 and its emulation mode is the same mechanism, but these are **analogy sources**, not μPD9002 manuals. | Implement as specified; flag anywhere the Z80 superset could plausibly differ. |
| `[VA-TM]` | *PC-88VA Technical Manual*. Directly describes the two CPU modes, compatible-mode instruction coverage, `CALLN 91h`/`95h`, `RETEM`, and the I/O trap. | Implement as specified. |
| `[VA-TEKU]` | 「てくまに」 — the *88VA Technical Manual* compiled independently by members of the PC-VAN "88VA Users Club" SIG and circulated as `TEKUMANI.LZH`. **Not** the BNN book: separately authored, community-edited, and its own distribution page notes that errors have been reported in it. | Implement, but treat as weaker than `[VA-TM]`. Corroborate against the manual scan or the ROM wherever possible. |
| `[VA-WIKI]` | *Inside PC-88VA* wiki, §1.5 CPU. Documents what the てくまに omits and carries its errata. Its I/O-trap section is CoBit's original 1992 post reproduced verbatim; its instruction-set section cites *Micom* Aug 1987. | Implement; note that CoBit's own caveat (ROM analysis plus experiment, possibly incomplete) applies to §9. |
| `[ROM]` | Extracted from PC-88VA ROM images: opcode tables, string pools, and disassembly. | Reliable for what the firmware *does*. A disassembler table is **not** proof that silicon executes something. |
| `[SRC]` | Period source or binaries that shipped and worked: CPMVA (Makichan, 1989), 98IOE/IOTRAP (CoBit, 1992). | Reliable. |
| `[DERIVED]` | Logically forced by the above; the derivation is stated inline. | Implement with a citing comment. |
| `[UNKNOWN]` | Not determined by anything in hand. | Do not guess. Register in §14. |

### 0.1 Sources

**In hand.**

- NEC *16-bit V Series Instruction Manual*, `U11301EJ5V0UMJ1` —
  `BRKEM`/`CALLN` operation, flag tables, instruction classification,
  register and segment encodings.
  <https://datasheets.chipdb.org/NEC/V20-V30/U11301EJ5V0UMJ1.PDF>
- NEC *V20/V30 User's Manual* (Oct 1986), **Chapter 8 complete** — mode
  shifting, register and flag correspondence, segment usage, interrupt /
  `RESET` / `HALT` behaviour, nesting prohibition, `PS3` indication.
- *PC-88VA Technical Manual* — BNN, first edition 25 Jun 1987,
  ISBN 4-89369-024-8, ¥5500, long out of print. Page 12 gives the two CPU
  modes, compatible-mode instruction coverage, `CALLN 91h`, `CALLN 95h`
  and `RETEM`. OCR is noisy; typography and register notation are
  normalised against the page image.
  <https://archive.org/details/PC88VA/page/12/mode/2up>
- NEC *V40HL/V50HL* data sheet `U13225EJ4V0DS00` — family analogy for the
  on-chip peripheral model only.
  <https://datasheets.chipdb.org/NEC/V40-V50/>
- 「てくまに」 / `TEKUMANI.LZH` (249 KiB, LHA) — a PC-88VA technical
  manual written independently by members of the PC-VAN "88VA Users
  Club" SIG. Covers system overview, memory, I/O, display, hardware
  control and BIOS. **Distinct from the BNN manual above**, and the
  distribution page states that bugs have been reported in it — hence
  the separate, weaker tag.
  <http://www.iris.dti.ne.jp/~nano/88va/tekumani.html>
- *Inside PC-88VA* wiki (Shinra, PukiWiki, 2005–2011), **§1.5 CPU** —
  device overview, V30-mode instruction set (citing *Micom* Aug 1987),
  built-in peripheral control with the VA2 configuration bytes, and the
  I/O trap. The I/O-trap section reproduces CoBit's original post to the
  VA Club "PC実験室" board, #4016, 31 Mar 1992 — i.e. it is the primary
  source for everything in §9, including the sample program 98IOE.
  <http://www.pc88.gr.jp/inside88va/wiki/index.php?CPU>
- ROM images: `varom00.rom` (VA), `varom00_va2.rom` (VA2), `varom1.rom`,
  `varom08.rom`. See Appendix C.
- CPMVA 1.3 (Makichan, 1989) — pc88.gr.jp software library,
  <http://www.pc88.gr.jp/softlib/?action=list_file&anum=2&gnum=424> —
  `CPMVA.ASM`, `CPMVA.H`, `CPMBIOS.MAC`,
  `V30.MAC`, `EXIT.MAC`, `CHARDEV.ASM`, `CRTOUT.ASM`, `BLOCKDEV.ASM`,
  `MAKEFILE`, `CPMVA.DOC`; binaries `CPMVA.EXE`, `CPMBIOS.COM`,
  `EXIT.COM`, `DO.COM`, `FCONV.COM`.
- 98IOE 2.4 (CoBit, 1992) — the I/O-trap test referenced from the
  *Inside PC-88VA* wiki `CPU` page — `98IOE.ASM`, `IOTRAP.ASM`, `IOTRAP.INC`,
  `INIT.ASM`, `STD.INC`, `DEBUG_C.ASM`, `98IOE.DOC`, makefiles;
  binary `98IOE.COM`.
- MAME NEC V20/V30 core — implementation comparison point (§12).

**Not in hand.**

- A μPD9002 device manual. Nothing tagged `[VA-TM]` or `[VA-TEKU]`
  should be mistaken for one.
- `CPM.SYS` (CCP + BDOS + BIOS image that `CPMVA.EXE` loads).
- The V1/V2 sub-ROM / disk BIOS.
- Remaining PC-88VA ROM images beyond those listed.

---

# Part I — The CPU

## 1. Device overview

The μPD9002 is a PC-88VA-specific CPU with two execution modes.

| Mode | Role |
|---|---|
| **V30 mode** (native) | 16-bit execution; a V30 instruction set with VA-specific differences. |
| **μPD780 mode** (compatible) | μPD780/Z80-compatible execution, used by V1/V2 compatibility software. |

`[VA-TM]` `[VA-WIKI]` Both state that the relationship between the two
modes is the same as the V30's native and 8080-emulation modes, and that
the mode-transition method is likewise the same in principle. That is what licenses
the use of V30 `BRKEM`/`CALLN`/`RETEM` documentation as a behavioural
reference throughout this document.

`[UNKNOWN]` The designation "V52". Treat the μPD9002 as the VA's
V52-class CPU for lineage purposes; no source in hand confirms the name.

## 2. Mode model

### 2.1 The MD flag

`[V30-MAN]` Mode selection is controlled by the **MD flag, the most
significant bit of the PSW** — bit 15. `MD = 1` is native, `MD = 0` is
compatible mode.

Three consequences follow, and all three were missing from earlier
project documents:

1. `[DERIVED]` Mode is **architectural state carried in FLAGS**, not a
   side-channel implementation flag. Everything that saves or restores
   the PSW participates in mode switching: `PUSH PSW`/`POP PSW`,
   interrupt entry, `RETI`, `BRKEM`, `BRKEM2`, `CALLN`, `RETEM`.
2. `[DERIVED]` Mode transitions are structurally **identical to
   interrupt entry and return**.
3. `[DERIVED]` `PUSH PSW` in native mode pushes a word whose bit 15 is 1.
   An implementation that pushes 0 there is wrong in a guest-visible way,
   and visible to any FLAGS-comparing test oracle.

`[V30-MAN]` The instruction manual annotates `BRKEM`'s effect as
"MD ← 0: write enable status", and `POP PSW` does not modify the mode
flag outside a native call made from compatible mode. `[DERIVED]` So MD
is not an ordinary writable flag bit: there is a write-enable condition
around it. Model it as a latch with its own write rule, not as bit 15 of
a plain flags word (§12 for how MAME splits this into two fields).

### 2.2 Mode-transition instructions

Notation: `push(x)` is `SP ← SP − 2; (SS:SP) ← x`. `vec(n)` is the
real-mode interrupt vector table entry `n` — offset at physical `n×4`,
segment at `n×4+2`.

#### BRKEM imm8 — `0F FF imm8` — enter compatible mode

```
Executed : native mode
Operation:
    push(PSW)
    MD ← 0                       ; write-enable status
    push(PS)                     ; PS = code segment (x86 CS)
    PS ← vec(imm8).segment
    push(PC)                     ; PC after the 3-byte instruction
    PC ← vec(imm8).offset
    flush prefetch; fetch as compatible-mode code
```

`[V30-MAN]` Confirmed against the NEC instruction manual. Notes:

- Pushes go to the **native stack** (`SS:SP`), not the compatible-mode
  stack. `SP` is untouched by compatible mode (§3.4), so the native
  return context survives.
- **`BRKEM` does not clear IE or BRK**, unlike the `BRK` (INT)
  instruction. Interrupts remain enabled across the transition.
- The vector is an ordinary real-mode IVT entry. `[SRC]` CPMVA installs
  it with DOS `INT 21h AH=25h` and restores it on exit.

`[SRC]` **`BRKEM` works on the μPD9002.** CPMVA emits `0F FF E1` and runs
CP/M-80 on real hardware. Earlier literature claiming the μPD9002 has no
`BRKEM` is wrong.

#### BRKEM2 imm8 — `0F FE imm8` — the VA firmware's entry

```
Executed : native mode
Operation: presumed BRKEM-shaped; see below
```

`[ROM]` Present in the on-board monitor's opcode table in both ROM
images, immediately after `BRKEM`. **The two ROMs disagree on the
mnemonic:**

| ROM | Mnemonic for `0F FE` |
|---|---|
| `varom00.rom` (PC-88VA) | `BRKEM2` |
| `varom00_va2.rom` (PC-88VA2) | `BRKFEM` |

`[VA-WIKI]` §1.5.1 states it directly: `BRKEM2` was **added** as the
transition instruction into Z80 emulation mode, encoded as three bytes
`0F FE nn` where `nn` is an interrupt number, and the VA does **not use**
the V30-compatible `BRKEM` (`0F FF nn`).

`[DERIVED]` Read that wording carefully: "not used" is a statement about
the VA's own firmware, not about the silicon. `BRKEM` is present and
works — CPMVA uses it (§2.4). The wiki is silent on what `BRKEM` enters
on a μPD9002.

`[ROM]` Corroborated by firmware: `VAROM1.ROM:0x13B1` contains
`0F FE 90`, at a valid instruction boundary, immediately after the ROM
has installed vector `90h` (§8).

**The open question this creates.** `[DERIVED]` Both entries exist and
both work, and **both destinations run Z80 code** — `BRKEM` because
CPMVA's Z80 BIOS executes `JR` (§2.4), `BRKEM2` because V1/V2 mode runs
Debug 8800 (§4.4). The old hypothesis that one entry selects 8080 and the
other Z80 is therefore *not* supported. What distinguishes them is
`[UNKNOWN]`. The plausible candidates:

- `BRKEM2` additionally switches machine-level state — the V1/V2 I/O and
  memory maps — where `BRKEM` only switches the CPU decoder.
- They differ in the saved frame or in the mode-latch write rule.
- They differ in nothing architecturally, and the VA firmware simply
  uses the VA-specific opcode by convention.

`[UNKNOWN]` Until this is settled, implement `BRKEM2` as a documented
default with a single flip point (§14).

Note for tooling: a general x86 disassembler will decode `0F FE` as an
MMX instruction. In PC-88VA context it is `BRKEM2`.

#### CALLN imm8 — `ED ED imm8` — call a native routine

```
Executed : compatible mode
Operation:
    push(PSW)                    ; the pushed PSW has MD = 0
    MD ← 1
    push(PS)
    PS ← vec(imm8).segment
    push(PC)
    PC ← vec(imm8).offset
```

`[V30-MAN]` `[VA-TM]` Both confirm the encoding and the operation. The
called native routine **returns with `RETI`** (NEC's mnemonic for
`IRET`, opcode `CF`); restoring the pushed PSW restores `MD = 0` and
re-enters compatible mode.

`[SRC]` CPMVA's native handler is a `far` procedure ending in `iret`.
The byte sequence `ED ED E0` appears three times in the shipped
`CPMBIOS.COM` — `0FF34h`, `0FF3Fh`, `0FF82h` — matching `c$boot`,
`c$wboot` and `execbios` in `CPMBIOS.MAC`.

`[DERIVED]` `ED ED` and `ED FD` are both **undefined** `ED`-prefixed
opcodes on a real Z80: NEC placed these instructions in unused Z80
encoding space. This is also why neither can be a native-mode
instruction — `ED` in native mode is `IN AW, DW`, which NEC would not
have broken.

#### RETEM — `ED FD` — leave compatible mode

```
Executed : compatible mode          <-- note
Operation:
    PC  ← pop()
    PS  ← pop()
    PSW ← pop()                     ; restores MD = 1 → native mode
```

`[V30-MAN]` Chapter 8 states it outright: `RETEM` is used exclusively as
the return from compatible mode to native mode when `BRKEM` caused the
shift, **it is executed in compatible mode**, execution returns from the
`BRKEM` interrupt routine to the main routine, `PS`/`PC`/`PSW` are
restored, and the `MD = 1` that `BRKEM` pushed is restored.

`[SRC]` Independently proven by CPMVA. `EXIT.MAC` is a `.z80` source
whose terminating sequence is a bare `retem` — that is how CP/M returns
control to the PC-Engine monitor. `V30.MAC`, which defines the `calln`
and `retem` macros with `defb`, is included only by Z80-side sources;
the 8086-side `CPMVA.H` defines `brkem` with `db`. `CPMVA.DOC` records
the build: V30 side with OPTASM 1.0 + TLINK 2.0, Z80 side with M80/L80
or SLR Z80ASM + SLRNK.

Two structural points:

- `RETEM` is the **only** instruction that ends a `BRKEM` session and
  resumes the instruction after it. Any model in which `RETEM` executes
  in native mode leaves compatible mode with no exit.
- Because `RETEM` executes in compatible mode, **the native decode table
  needs no `ED FD` special case at all.**

`RETEM` has **one** return target. `[ROM]` The `1000:C003` destination
sometimes attributed to `RETEM` is not part of the instruction: it is an
explicit `jmp 1000h:c003h` in the native code that runs *after* `RETEM`
has returned and V3 re-initialisation has completed (§8).

### 2.3 Transition summary

`[V30-MAN]` Chapter 8 enumerates both directions exhaustively.

| Path | Encoding | Executes in | Ends in | Returned from by |
|---|---|---|---|---|
| `BRKEM imm8` | `0F FF imm8` | native | compatible | `RETEM` |
| `BRKEM2 imm8` | `0F FE imm8` | native | compatible | `RETEM` (presumed) |
| `CALLN imm8` | `ED ED imm8` | compatible | native | `RETI` (`CF`) |
| `RETEM` | `ED FD` | compatible | native | — |
| `RETI` | `CF` | native | compatible, if the popped `MD = 0` | — |
| `NMI` / `INT` | — | compatible | native | `RETI` (`CF`) |
| `RESET` | — | compatible | native | emulation in progress is aborted |

**Nesting is prohibited.** `[V30-MAN]` Inside a native routine entered by
`CALLN`, or by an `NMI`/`INT` taken from compatible mode, compatible mode
cannot be entered again with `BRKEM`; if attempted, `MD` does not work
normally and behaviour is undefined. There is exactly one level of
compatible-mode context.

`[DERIVED]` For an emulator this means the mode state needs no stack of
its own — the single `MD` latch plus the ordinary interrupt stack is the
entire model, and a nested `BRKEM` is an error to diagnose rather than a
case to support.

**External mode indication.** `[V30-MAN]` The `PS3` processor-status
output is high during a compatible-mode bus cycle and low in native mode.

`[UNKNOWN]` Whether the PC-88VA's "88-mode emulator" gate array uses
`PS3` to switch peripheral decoding between the V1/V2 and V3 I/O maps.
If it does, that is the hardware link between CPU mode and I/O map, and
it would be a strong candidate answer to the `BRKEM` vs `BRKEM2`
question in §2.2. The VA1 schematics are in hand; tracing the μPD9002
`PS3` pin is a cheap, high-value check.

### 2.4 Compatible mode is Z80, not 8080

`[V30-MAN]` On the V20/V30, code fetched after `BRKEM` is interpreted as
μPD8080AF instructions.

`[VA-TM]` The technical manual states that μPD780 mode supports the
complete documented μPD780 instruction set and many undefined
instructions, plus `CALLN` and `RETEM`.

`[SRC]` **Directly proven**, from the shipped `CPMBIOS.COM`:

- `CPMBIOS.MAC` is assembled with `.z80` and uses `JR` on the primary
  BIOS dispatch path: `c$const: ld a,2 / jr execbios`.
- `CPMBIOS.COM` maps to `0FA00h–0FFFFh` (1536 bytes, ending with the
  ASCII system ID `VA` at `0FFFEh`, exactly as the source's
  `.phase`/`defs`/`defb` directives specify). The entry jump table at
  `0FA00h` reads `C3 2F FF  C3 3A FF  C3 45 FF …`, and at `0FF45h` —
  the `const` entry — the bytes are `3E 02 18 …`: `LD A,2` then **`JR`**.
- `JR` (`18`), `JR cc` (`20`/`28`/`30`/`38`) and `DJNZ` (`10`) are
  Z80-only; on an 8080 those opcodes are undocumented NOPs. If the mode
  were 8080, `c$const` would fall through into `c$conin` and CP/M could
  not run. The binary contains 14 `18`, 16 `20`, 2 `28`, 22 `30`, 1 `38`
  and 5 `10` opcode bytes.

For the record, the argument used in earlier project documents — that
`ED`-prefixed `CALLN`/`RETEM` proves Z80 — is **invalid**. Those are
`ED ED`/`ED FD` on the plain V20/V30 too, where the mode is 8080. The
`_IX equ si` / `_IY equ di` aliases in `CPMVA.H` are also not proof:
CPMVA never uses them.

## 3. Programming model in compatible mode

### 3.1 Registers

`[V30-MAN]` From Figure 8-1: the low 8 bits of `AW` serve as the
accumulator, and both halves of `BW`, `CW` and `DW` serve as the six
general-purpose registers. `SP` is the stack pointer in native mode;
**`BP` acts as the stack pointer in compatible mode.** The pairings are
`AW`→`AC`, `BW`→`H`,`L` = `HL`, `CW`→`B`,`C` = `BC`, `DW`→`D`,`E` = `DE`,
`BP`→`SP`, `PS`→code segment, `DS0`→data segment, `PC`→`PC`,
`PSW`→flags.

`[SRC]` The Z80-side view, from `CPMVA.H` — the same mapping extended
with the Z80-only index registers:

```
Z80    V30 (NEC / x86)         Z80    V30
A   ↔  AL                      BC  ↔  CW / CX
B   ↔  CH                      DE  ↔  DW / DX
C   ↔  CL                      HL  ↔  BW / BX
D   ↔  DH                      IX  ↔  IX / SI   [UNKNOWN — §14]
E   ↔  DL                      IY  ↔  IY / DI   [UNKNOWN — §14]
H   ↔  BH                      SP  ↔  BP        <- BP, not SP
L   ↔  BL                      PC  ↔  PC / IP
```

The `A`, `B`/`C`, `D`/`E`, `H`/`L`, `BC`, `DE`, `HL` and `SP` rows are
exercised by working CPMVA code: the native BIOS handler reads the Z80
function number as `AL`, returns file handles in `CX`, lengths in `DX`,
and reads buffer pointers from `BX`.

`[V30-MAN]` **`AH`, `SP`, `IX`, `IY` and all four segment registers
(`PS`, `SS`, `DS0`, `DS1`) are not addressable from compatible mode**,
and the manual gives the rationale for the split stack pointers: keeping
`SP` and `BP` independent prevents misuse of one mode's stack pointer
from destroying the other's.

`[DERIVED]` This is a **point of divergence for the μPD9002**. A Z80 has
`IX` and `IY`; on the V20/V30 they are explicitly unreachable from
emulation mode. The μPD9002 must therefore have changed this, and *how*
is precisely the open `ix-iy-share` question: it could expose the
existing `IX`/`IY` register-file entries — which is what `CPMVA.H`'s
`_IX equ si` / `_IY equ di` assumes — or it could have added separate
storage.

`[DERIVED]` The V30 register file has eight 16-bit registers. The Z80
main set plus `IX`/`IY` plus the compatible-mode `SP` consumes all of
them except `AH` and the native `SP`. That leaves at most three bytes of
visible register file, which cannot hold the eight bytes of
`AF'/BC'/DE'/HL'`. Either the alternate set has storage invisible to
native mode, or it does not exist.

### 3.2 Flags

`[V30-MAN]` The low 8 bits of the PSW **are** the compatible-mode flag
register. There is **no flag conversion at a mode transition**; the same
physical bits are read under a different interpretation. Figure 8-2 gives
both layouts and they are the same byte:

```
bit          9    8    7    6    5    4    3    2    1    0
V30  PSW:    IE   BRK  S    Z    0    AC   0    P     1    CY
8080 Flag:             S    Z    0    AC   0    P     1    C
```

This is not a coincidence of the V-series: the 8086 inherited its low
flag byte from the 8080 in both bit position and meaning.

`[DERIVED]` For the Z80 superset the alignment still holds, with the
three bits the 8080 leaves constant carrying the Z80-only flags:

```
bit          7    6    5    4    3    2     1    0
V30  PSW:    S    Z    0    AC   0    P     1    CY
Z80  F  :    S    Z    F5   H    F3   P/V   N    C
```

`S`, `Z`, `H`↔`AC` and `C`↔`CY` occupy identical positions; `F5`, `F3`
and `N` land on PSW bits 5, 3 and 1, which the 8080/V30 side holds at
constant `0`, `0`, `1`. The Z80's `P/V` shares bit 2 with the V30's `P`,
but the two differ in *meaning* for arithmetic operations (overflow vs
parity).

**Implementation consequence** `[DERIVED]`: the difference between the
modes is in **how flag values are computed**, not where they are stored.
A design that keeps a separate Z80 `F` and converts it at each transition
is modelling the hardware incorrectly, and the error is guest-visible.

`[ROM]` Corroboration from an unrelated direction: Debug 8800's flag
notation table (§4.4) encodes the Z80 `F` layout — including `N` at
bit 1 and undefined bits at 3 and 5 — exactly as above.

**Anomaly, unresolved.** `[SRC]` CPMVA's extended BIOS (functions
`80H`–`84H`) documents a carry return convention: `CPMVA.DOC` specifies
`Cy=0` for success and `Cy=1` for error, and `CPMVA.ASM` implements it
(`stc` on the `ExExec` error path; `xor _A,_A`, which also clears CY, on
success paths). But those handlers are reached through `CALLN` and return
through `iret`, and `RETI` restores the PSW that `CALLN` pushed — which
should discard the handler's carry. Either the μPD9002's `CALLN`/`RETI`
pair has flag semantics that are not a plain PSW restore, or the
convention never worked and callers relied on the error code in `A`
(also documented, and sufficient). **This is the cheapest experiment that
settles the flag model** — see §16.7.

### 3.3 Address space and segments

`[V30-MAN]` In compatible mode, **the segment base for memory operands,
including the stack, is determined by `DS0`** (the x86 `DS`), whose value
the program sets before entering. Instruction fetch uses `PS` (the x86
`CS`). The segment registers are not visible to compatible-mode code.

```
code   fetch  :  physical = (PS  << 4) + PC
data   access :  physical = (DS0 << 4) + addr16
stack  access :  physical = (DS0 << 4) + BP     <- DS0, not SS
```

The 64 KiB compatible-mode space can therefore sit anywhere in the 1 MiB
physical space. Two instances are documented:

`[ROM]` **V1/V2 mode is at physical `0x10000`.** Immediately before the
firmware handoff (§8) the ROM sets `DS = 1000h`, and vector `90h` is
installed as `1000:0000`, so `PS = DS0 = 1000h`. This is what makes
`1000:E000` "address `E000h` as seen from V1/V2 mode" and `1000:C003`
"`C003h` in V1/V2 space" — both are ordinary offsets in that window, not
special addresses.

`[SRC]` **CPMVA places its window wherever DOS allocated it.**
`CPMVA.ASM` declares a paragraph-aligned `emu_seg`, enlarges its DOS
memory block by `1000h` paragraphs ("64KB for Z80 emulation area"),
points the `BRKEM` vector at `emu_seg:0FA00h`, and sets `DS = emu_seg`
immediately before `brkem`. `SS` is never touched — it remains the
`.MODEL small` DGROUP stack — and this is correct precisely because
compatible-mode stack accesses use `DS0`.

`[V30-MAN]` Bus behaviour is unchanged by compatible mode: memory and I/O
cycles are identical to native mode, and the bus-hold and `HALT` standby
functions work as they do natively. `BUSLOCK` and `POLL` are
**unavailable** in compatible mode.

`[UNKNOWN]` How Z80 `IN`/`OUT` port numbers map to the V30 I/O space. The
bus cycles being identical suggests a direct mapping of the 8-bit (or
`BC`-supplied 16-bit) port number, but nothing in hand states it.

### 3.4 Stacks

Two distinct stacks; do not confuse them.

| Stack | Pointer | Segment | Used by |
|---|---|---|---|
| native | `SP` | `SS` | `BRKEM`, `BRKEM2`, `CALLN`, `RETEM`, `RETI`, interrupt entry |
| compatible | `BP` | `DS0` | Z80 `PUSH`/`POP`/`CALL`/`RET`/`RST` |

`[SRC]` `CPMBIOS.MAC` sets the compatible-mode stack with
`ld sp,CCPTOP` (`0E400h`), which lands in `BP`. `CPMVA.ASM`'s native
handler freely uses `push si`/`pop si` on the native stack while
compatible-mode code is suspended.

`[DERIVED]` Getting this backwards produces a "boots, then desyncs"
failure rather than a clean one. Verify it first.

### 3.5 Interrupts, RESET and HALT

`[V30-MAN]` A maskable interrupt or NMI taken in compatible mode is
**serviced in native mode**, exactly as it would be natively; the handler
returns with `RETI`, which restores the pushed PSW and re-enters
compatible mode.

`[DERIVED]` This is the `CALLN` mechanism again: the interrupt pushes the
compatible-mode PSW (`MD = 0`), hardware sets `MD = 1`, `RETI` undoes
both. No separate compatible-mode interrupt path is needed for the
V30-documented behaviour.

`[V30-MAN]` `RESET` is different: it resets the CPU exactly as in native
mode and **the emulation in progress is aborted** — there is no return.

`[V30-MAN]` `HALT`/standby has its own rules:

- If standby was entered *from compatible mode*, an `INT` — even with
  interrupts disabled — resumes **in compatible mode** at the instruction
  after `HALT`.
- `RESET` or `NMI` instead, or `INT` while interrupts are enabled, exits
  standby into **native mode**; from there the handler can return to
  compatible mode with `RETI`.
- If standby was entered from native mode, `RESET`, `NMI` or `INT`
  resumes native mode regardless of the interrupt-enable state.

`[UNKNOWN]` Whether the μPD780 superset adds Z80-style interrupt handling
on top — `IM 0`/`IM 1`/`IM 2`, the `I` register, `IFF1`/`IFF2`, `EI`/`DI`
semantics, `RETN`. CPMVA uses only synchronous `CALLN`. The V1/V2 BASIC
ROM does contain an `IM 1`-style vector at Z80 `0038h` (Appendix C),
which is suggestive but proves nothing about how the interrupt arrives.

This is a different question from the PC-8801-side and FDD-side Z80
interrupt work. Do not reuse conclusions across them.

## 4. Instruction set

### 4.1 The `0F` extension map

`[ROM]` The on-board monitor carries a table of
`{mask, opcode, operand-class}` triples for the `0F` extensions, with a
parallel mnemonic string pool (ASCII, bit 7 set on the final character).
Both ROMs contain the same 14 entries:

```
FF 20 00  ADD4S      FF 20 01  ADD4S      F6 10 03  TEST1
FF 22 00  SUB4S      FF 22 01  SUB4S      F6 16 03  NOT1
FF 26 00  CMP4S      FF 26 01  CMP4S      F6 12 03  CLR1
FF 28 02  ROL4       FF FF 04  BRKEM      F6 14 03  SET1
FF 2A 02  ROR4       FF FE 04  BRKEM2 (VA) / BRKFEM (VA2)
```

The `F6` mask covers the four encodings of each bit instruction
(`CL`/`imm` × byte/word), matching the documented V30 layout `0F 10`–`0F 1F`.
**`INS` and `EXT` are absent.**

### 4.2 The four deleted instructions

`[VA-WIKI]` The V30-mode instruction set is the V30's, minus four
instructions: `INS`, `EXT`, `OUTM`, `INM`. The wiki cites *Micom*,
August 1987 for this.

`[V30-MAN]` What those four actually are — and this was wrong in every
earlier project document:

| NEC mnemonic | Intel mnemonic | Encoding | Function |
|---|---|---|---|
| `INS` | — | `0F 31`, `0F 39` | **bit field insert** (`AW` → bit field at `DS1:IY`) |
| `EXT` | — | `0F 33`, `0F 3B` | **bit field extract** (bit field at `DS0:IX` → `AW`) |
| `INM` | `INSB`/`INSW` | `6C`, `6D` | primitive block input |
| `OUTM` | `OUTSB`/`OUTSW` | `6E`, `6F` | primitive block output |

NEC's classification places `EXT`/`INS` under *bit field manipulation*
and `INM`/`OUTM` under *primitive I/O*. **Only two of the four are block
I/O.** Any reasoning that treats all four as block I/O — in particular
the suggestion that the I/O trap exists to emulate them in software —
loses half its basis and should be withdrawn. The naming collision is the
likely origin of the error: NEC's `INS` is not Intel's `INS`.

`[ROM]` Consistent with removal: `INS`/`EXT` are absent from the `0F`
table. **Not** consistent, and worth stating plainly: `INM` and `OUTM`
(and their Intel-notation twins `INS`/`OUTS`) *are* present in the
monitor's mnemonic pool. A disassembler table is not an execution table,
so this neither proves nor disproves the deletion — but earlier notes
claiming they are absent from the string pool are wrong.

`[DERIVED]` Consequence for any V20-based test oracle: `6C`–`6F` and
`0F 31/33/39/3B` are expected divergences on a μPD9002 target, not
defects to fix.

### 4.3 What compatible mode decodes

`[VA-TM]` The complete documented μPD780 instruction set plus many
undefined instructions, plus `CALLN` and `RETEM`.

`[SRC]` Independently proven present: the 8080 subset used by CP/M, plus
`JR`, `JR cc`, `DJNZ` (§2.4).

`[ROM]` `LDIR` (`ED B0`) appears in live V1/V2-mode code (§4.4).

`[UNKNOWN]` The undocumented `DD`/`FD` half-index-register opcodes
(`IXH`/`IXL`/`IYH`/`IYL`) and the rest of the undocumented space. Neither
Debug 8800 nor N88-BASIC has any reason to use them.

`[ROM]` The **V3-mode PC-Engine monitor** carries no Z80 mnemonics in
either ROM image; its assembler and disassembler are NEC-native only.
That is a fact about the V3 tooling, not the silicon. **Debug 8800 is a
different matter** — see §4.4.

### 4.4 Debug 8800 — a corpus of compatible-mode Z80 code

`[ROM]` `varom00.rom` also contains **Debug 8800 version 1.0**,
banner-dated 11 Dec 1981, copyright NEC — a PC-8801-era Z80 monitor, six
years older than the PC-88VA, carried for V1/V2 compatibility. Banner at
ROM offset `0x1F058`.

The surrounding region is unambiguously Z80. Disassembling from
`0x1EE5F`:

```
21 FA F1      LD   HL,0F1FAh
01 03 00      LD   BC,0003h
11 C9 ED      LD   DE,0EDC9h
ED B0         LDIR
F3            DI
ED 7B F7 F1   LD   SP,(0F1F7h)
2A F5 F1      LD   HL,(0F1F5h)
3A F9 F1      LD   A,(0F1F9h)
D3 70         OUT  (70h),A
C3 8B 7E      JP   7E8Bh
```

Three data structures in this region matter:

- **Command dispatch table**, `0x1E0F5`: the letters `ALTRWVBM` then
  `EIOGFXDS` — sixteen command letters — then 16-bit little-endian
  handler addresses (`6135h`, `61A3h`, `6254h`, `63D8h`, `6406h`,
  `6491h`, `6479h`, `64D8h`, `746Fh`, `64A6h`, `695Eh`, `6806h`,
  `6754h`, …).
- **Flag notation table**, `0x1E3C8`: `PM-Z---H--OE-N-C` — eight
  two-character pairs, one per flag bit, matching the monitor's own help
  text (`Sign[p or m]`, `Zero[z or -]`, `Undef[-]`, `Half[h or -]`,
  `Undef[-]`, `Parity[o or e]`, `Sub[n or -]`, `Carry[c or -]`). The
  **Z80** `F` layout, matching §3.2.
- **Register name table**, `0x1E318`: `IY`, `IX`, `H'`, `D'`, `B'`, `F'`,
  `A'`, `H`, `D`, `B`, `F`, `A`. The `x` command displays and edits **the
  alternate register set and the index registers**.
- **Mnemonic pool**, `0x1EB00`–`0x1ED00`, high-bit-terminated: it carries
  **both notations**. Zilog — `LD`, `JP`, `JR`, `DJNZ`, `CALL`, `RET`,
  `NOP`, `PUSH`, `POP`, `EX`. Intel 8080 — `MOV`, `MVI`, `LXI`, `JMP`,
  `STA`, `LDA`, `XCHG`, `XTHL`, `SPHL`, `PCHL`, `RST`, `DAD`, `RLC`.
  So its `a` and `l` commands handle Z80 source directly; the tests in
  §16 can be typed as mnemonics rather than hexadecimal.

`[DERIVED]` The handler addresses, the internal `JP 7E8Bh` and the banner
offset are mutually consistent with *Z80 address = ROM offset − `0x18000`*
for this region, placing Debug 8800 at Z80 `6000h`–`7FFFh` — the PC-8801
banked extension-ROM window. The bank's entry shim confirms both the
mapping and the mechanism:

```
0x1E000:  44 42            "DB"  (bank signature)
          F3               DI
          C3 31 7A         JP   7A31h
          ED 73 F7 F1      LD   (0F1F7h),SP
          DB 70            IN   A,(70h)          ; save bank-select state
          32 F9 F1         LD   (0F1F9h),A
          CD 00 7B         CALL 7B00h
          21 F4 71         LD   HL,71F4h
          01 79 00         LD   BC,0079h
          ED B0            LDIR
```

with the exit path above restoring `SP` and the bank register. Port `70h`
is the PC-8801 extended-ROM bank select.

**Why this matters.** The PC-88VA has no second main-CPU Z80, and
**Debug 8800 has been run in V1/V2 mode on a real PC-88VA**. V1/V2-mode
Z80 code therefore executes on the μPD9002 in compatible mode, and the
whole of Debug 8800 and the N88-BASIC ROM (Appendix C) is
proven-executable compatible-mode code. In particular:

- `[DERIVED]` `LDIR` appears in live code, so the `ED` block instructions
  are implemented.
- `[DERIVED]` The `x` command reads and writes `A'/F'/B'/D'/H'`. There is
  no way to reach the alternate registers except by executing
  `EX AF,AF'` and `EXX`, so if `x` works the alternate set exists.
- `[DERIVED]` The same command handles `IX`/`IY`, requiring the `DD`/`FD`
  prefix spaces.

§16.0 turns these into a ten-minute confirmation.

## 5. On-chip peripherals

`[VA-TEKU]` The μPD9002 integrates DMA-controller, interrupt-controller
and timer functions analogous to the V50 family.

`[V30-MAN]` Family analogy only, from the V40HL/V50HL data sheet
(`uPD70208H`/`70216H`):

| Data-sheet point | Why it matters |
|---|---|
| V40HL/V50HL are V20/V30 software-compatible. | Supports treating μPD9002 native mode as V30-like before applying VA differences. |
| They list a μPD8080AF emulation function. | Makes the `BRKEM` analogy plausible while leaving `BRKEM2` VA-specific. |
| On-chip CG, WCU, REFU, TCU, SCU, ICU, DMAU blocks. | Matches the kind of block the VA notes describe for μPD9002. |
| 1 MiB memory space, 64 KiB I/O space. | Matches the address-space assumptions in the VA ROM setup code. |
| Internal peripherals mapped into a system I/O area. | Supports reading VA ROM writes to `FFF0h`–`FFFFh` as CPU-internal control setup. |

`[DERIVED]` The I/O trap control ports at `FFE0h`–`FFEFh` (§9) sit in the
same high I/O region. Treat `FFE0h`–`FFFFh` as μPD9002-internal.

`[VA-WIKI]` §1.5 gives the μPD9002's own list, "as in the V50":
DMA controller, interrupt controller, timer, serial interface; a
**programmable wait control unit** that inserts 0–3 wait clocks into
memory and I/O cycles; a clock generator; and a refresh unit. Plus the
I/O trap (§9).

`[VA-WIKI]` On the V50, `FFF0h`–`FFFFh` selects CPU pin functions, wait
control, and the I/O addresses of the DMA controller, interrupt
controller, timer and serial interface. The μPD9002 is presumed the same.
The **PC-88VA2 configuration**, as written by its firmware, is:

```
port  value        port  value        port  value
FFFE   11          FFF6   08          FFF2   08
FFFD   07          FFF5   80          FFF0   00
FFFC   01          FFF4   53
FFFB   60
FFFA   88
FFF9   A0
```

(VA1 not investigated.) `[UNKNOWN]` What each byte means; the V50 data
sheet is the obvious cross-reference and has not yet been applied.

---

# Part II — PC-88VA firmware conventions

Everything in Part I is CPU behaviour. Everything here is convention
built on top of it by the VA firmware, and belongs in emulated firmware
rather than in the CPU model.

## 6. Interrupt vector table

`[ROM]` The vector installer at `VAROM1.ROM:0x13ED`, called from
`0x1364`, fills the IVT with safe defaults and then applies an override
table at `0x0F5E`. The entries that matter here:

```
7C -> F000:1944      I/O trap, IN
7D -> F000:1944      I/O trap, OUT   (same handler as 7C)
7E -> F000:1920
90 -> 1000:0000      BRKEM2 target — V1/V2 entry
91 -> F000:24B0      CALLN 91h service
95 -> 1000:E000      CALLN 95h service
```

Three independent confirmations fall out of this table:

- `[DERIVED]` `7Ch` and `7Dh` point at **one shared handler**, exactly as
  98IOE's own installer does (§9.5). The handler distinguishes IN from
  OUT by decoding the trapped opcode, not by which vector fired.
- `[DERIVED]` Vector `90h` = `1000:0000` is the V1/V2 code entry, and it
  fixes `PS = 1000h` for compatible mode.
- `[DERIVED]` **`CALLN 91h` and `95h` are ordinary IVT entries.** They
  are *not* hardcoded CPU behaviour. `CALLN imm8` remains uniformly "call
  `vec(imm8)`", and the "fixed services" are firmware conventions. This
  is the single most important structural correction in this document:
  an implementation that special-cases `91h`/`95h` inside the CPU is
  modelling the wrong layer.

## 7. CALLN native services

`[VA-TM]` The VA provides native-service entry points so V1/V2-compatible
code can reach V3 memory and I/O or call native V30 code.

### 7.1 CALLN 91h — V3 memory and I/O access

Encoded `ED ED 91`. The caller selects the operation in `A`:

| `A` bit | Clear | Set |
|---|---|---|
| 2 | memory access | I/O access |
| 1 | read / input | write / output |
| 0 | byte | word |

| Register | Meaning |
|---|---|
| `HL` | segment for memory access; unused for I/O |
| `DE` | memory offset, or I/O port address |
| `BC` | value for write/output; result for read/input |

`[ROM]` Vector `91h` → `F000:24B0`, agreeing with the documented service
number.

### 7.2 CALLN 95h — user native routine

Encoded `ED ED 95`. Calls the user native routine at `1000:E000` — that
is, offset `E000h` in the V1/V2 window (§3.3). Compatible-mode software
prepares a native routine there and invokes it.

The routine returns to compatible mode with **`IRET`, not `RETEM`**
(§2.2). `[ROM]` Vector `95h` → `1000:E000`, independently agreeing with
the documented ABI.

## 8. The V1/V2 handoff in VAROM1.ROM

`[ROM]` One branch precedes the handoff. After installing vectors, the
ROM tests port `000Dh` bit 2 through the helper at `0x13E6`:

```asm
1364: call 13edh       ; install vectors
1367: call 13e6h       ; read port 000Dh, mask 04h
136a: jz   13d2h       ; alternate path — no BRKEM2 in this block
```

The non-zero path enables I/O traps by calling `0x18E7`, which writes
`03h` to `FFEFh` (IN + OUT traps, byte-port matching), runs memory and
display setup, and reaches the handoff. The zero path sets `[2F07h]=1`,
writes `91h` to port `FFh`, runs the shared initialisation, and jumps
back into the ROM without executing `BRKEM2`.

The handoff itself:

```asm
13a8: mov ax,1000h
13ab: mov ds,ax        ; DS0 = 1000h — the V1/V2 window
13ad: xor ax,ax
13af: mov es,ax
13b1: 0f fe 90         ; BRKEM2 90h  ->  PS:PC = 1000:0000
13b4: cli              ; <- RETEM returns here
```

`[DERIVED]` Evidence that `0x13B1` is intended as `BRKEM2 90h` is
strong: the surrounding stream is valid 16-bit V30 setup code, the bytes
sit at an instruction boundary after `mov es,ax`, `0F FE nn` is the
VA-specific encoding, and the ROM has just installed vector `90h` as
`1000:0000`. `[UNKNOWN]` Whether every boot configuration reaches this
instruction — the `000Dh` branch can skip the block, so a CS:IP trace is
still the way to prove execution for a given ROM/configuration pair.

**The resume block.** `[ROM]` The bytes after the handoff disassemble as
ordinary V30 code:

```asm
13b4: cli
13b5: call 18eeh       ; writes 00h to FFEFh — disable I/O trap
13bd: mov dx,0152h
13c0: in  ax,dx
13c1: or  ax,4000h
13c4: out dx,ax
13cd: jmp 1000h:c003h
```

`[DERIVED]` This only makes sense if compatible-mode code eventually
executes `RETEM` and returns to `0x13B4`. The trap-enable at `0x18E7`
before the handoff and the trap-disable at `0x18EE` after it bracket the
compatible-mode session exactly — **direct firmware evidence that the I/O
trap is the V1/V2 I/O compatibility mechanism** (§9).

`[DERIVED]` The far jump target `1000:C003` is physical `0x1C003` — an
offset in the V1/V2 window, i.e. RAM from the CPU's point of view, not
`VAROM1.ROM` offset `0xC003`. This is the post-V3-initialisation
destination the technical manual describes, and it is reached by an
explicit `jmp`, **not** by any special `RETEM` behaviour.

`[UNKNOWN]` What prepares the code at `1000:C003`. The static ROM search
found the byte pattern only once in `VAROM1.ROM` and no direct copy loop
in this region. The leading hypothesis is that the compatible-mode path
entered by `BRKEM2 90h` prepares it — another reason the missing
compatible-mode implementation matters.

`[ROM]` The shared post-handoff initialiser at `0x2210` and its
subroutines (`0x2220` port table, `0x0E84` 51-vector install, `0x2233`
low-memory clear, `0x240B` backup-memory area, `0x2252` BIOS/device
init, conditional on `[2F07h]`) are boot-sequence material; see
`pc88va-boot-sequence.md`.

## 9. I/O trap subsystem

`[VA-WIKI]` A μPD9002/VA compatibility feature, not a PC-98 device.
Ordinary V-series software interrupts come from `INT`/`INTO` or a `DIV`
error; on the VA, `IN` and `OUT` can raise one too.

**The trap exists for V1/V2 mode**, to emulate a subset of the I/O ports
— `50h`–`53h`, `60h`–`68h`, `6Eh`–`6Fh`. V3 mode leaves it disabled, but
software can enable it through the control ports, which is exactly what
the firmware does around the V1/V2 handoff (§8). CoBit's own assessment
is that the facility is weak, existing essentially to patch V1/V2 I/O.

Provenance note: everything in this section traces to CoBit's post to the
VA Club "PC実験室" board (#4016, 31 Mar 1992), reproduced in the *Inside
PC-88VA* wiki. **The register names (`_IOTrap1S` and so on) are CoBit's
own coinage, not NEC's**, and CoBit states plainly that the whole
description comes from ROM analysis and experiment and may contain
errors or omissions. The 98IOE sources in hand are his sample program
for it, so §9 has one author, not two independent ones.

### 9.1 Control ports

```
FFE0h   trap block 1, start port number     word OUT
FFE2h   trap block 1, end   port number     word OUT
FFE4h   trap block 2, start port number     word OUT
FFE6h   trap block 2, end   port number     word OUT
FFEFh   control                             byte OUT
          bit 0   1 = IN traps enabled
          bit 1   1 = OUT traps enabled
          bits 2-3  reserved
          bit 4   0 = byte-port trapping, 1 = word-port trapping
```

`[SRC]` Bit assignments from the `RecIOTrapC` record in `IOTRAP.INC`;
access widths from `_setraport` (word) and `iotrap_on`/`iotrap_off`
(byte). Range registers accept word writes but the high byte is
effectively zero.

Interrupt vectors: `7Ch` for IN traps, `7Dh` for OUT traps.

`[VA-WIKI]` The I/O-trap ports occupy `FFE0h`–`FFE7h` plus `FFEFh`, and
**ports `FFE0h`–`FFFFh` must not themselves be trapped.**

### 9.2 Trap semantics

`[SRC]`

- Normal interrupt stack frame — `FLAGS`, `CS`, `IP` — and **`CS:IP`
  points at the I/O instruction itself**, not the following instruction.
  `IOTRAP.ASM` relies on this: `les si,[bp].$IP` then reads the opcode
  from `es:[si]`.
- **The handler must advance the saved `IP` by the instruction length
  before `IRET`**, or the same instruction traps forever. 98IOE decodes
  the opcode itself: one `inc [bp].$IP` for the `DX` form (1 byte), two
  for the `imm8` form (2 bytes).
- Opcode bit layout used for that decode (`RecOp`): bit 0 = word/byte,
  bit 1 = out/in, bit 3 = port-in-`DX` / port-in-`imm8`. Matches the x86
  `E4`–`E7` / `EC`–`EF` encodings.
- **Entered with interrupts disabled.** `IOTRAP.ASM` executes `sti`
  immediately after entry, with a comment that this must be removed if
  I/O inside hardware interrupt handlers is also to be emulated.
- The trapped instruction does **not** perform the access; the handler
  does.
- The handler must disable trapping (`FFEFh ← 0`) before its own I/O and
  re-enable before `IRET`.
- The IN result is returned by patching the saved accumulator in the
  handler's `pusha` image. Flags are not returned.

`[DERIVED]` One incidental consequence of the deleted `INM`/`OUTM`
(§4.2): with no block I/O instructions, every trappable I/O instruction
is a plain `IN`/`OUT` with no memory operand and therefore no segment
override, so "the pushed address is the start of the instruction" is
unambiguous.

### 9.3 Word-port matching

`[SRC]` **Word-port trapping matches on the low byte only.** 98IOE
demonstrates it directly: it programs block 1 as `0037h`–`0038h` with
word-port trapping and successfully traps port `1038h`. Handlers must
therefore re-check the full 16-bit port number — trapping `52h` also
traps `152h`.

### 9.4 Performance

`[VA-WIKI]` Trapped I/O instructions run **tens to hundreds of times
slower**, because the emulation is entirely in software. The worked
example is instructive: word-trapping a range containing port `52h` also
traps port `152h` (§9.3), which the PC-Engine subsystem uses constantly,
and the slowdown becomes obvious to the eye. FDD ports are the same.
A handler must also disable trapping before any `STI`, not just around
its own I/O.

### 9.5 Installation sequence

`[SRC]` From `INIT.ASM`:

```
1.  FFEFh ← 0                       ; disable trapping first
2.  hook INT 7Ch -> trap_entry      ; save the old vectors
    hook INT 7Dh -> trap_entry      ; SAME handler for both
3.  FFE0h ← block-1 start port      ; word OUT
    FFE2h ← block-1 end   port
    FFE4h ← block-2 start port
    FFE6h ← block-2 end   port
4.  FFEFh ← control value           ; byte OUT; enables trapping
```

An unused trap block is parked on a harmless port — 98IOE's
`_setraport` macro defaults an omitted block to `0Fh`–`0Fh`. Removal
restores the two vectors and frees the resident block.

### 9.6 What 98IOE actually is

Worth stating because earlier project notes described it incorrectly.
98IOE is a **PC-9801 I/O emulator demonstration** for the PC-88VA. Its
entire emulated functionality is: `OUT 37h,AL` with `AL=6` produces a
beep (forwarded to VA system port `01CFh`), and `IN AX,1038h` returns
`7777h`. It traps `0037h`–`0038h` and `000Fh`. It is a reference
*implementation* of the mechanism, not an instance of V1/V2 emulation.

## 10. CPMVA — a worked V3-side example

`[SRC]` The complete verified transition sequence of a real program.
Useful as an acceptance trace, and the counterpart to §8: `BRKEM` from
V3-mode software rather than `BRKEM2` from firmware.

```
native (V30 mode, MS-DOS on PC-88VA V3):
  CPMVA.EXE starts, DS = DGROUP
  SETBLOCK to (emu_seg + 1000h) - PSP          ; reserve the 64 KiB window
  read CPM.SYS into a buffer in DGROUP
  save vectors 0E0h and 0E1h (INT 21h AH=35h)
  install vector 0E0h -> cs:nBIOSentry         ; far, native
  install vector 0E1h -> emu_seg:0FA00h        ; the Z80 BIOS entry
  copy the BIOS image to emu_seg:0FA00h
  DS = emu_seg, ES = DGROUP
  brkem 0E1h                                   ; 0F FF E1

compatible (Z80):
  PS = DS0 = emu_seg, PC = 0FA00h
  c$boot:  ld sp,CCPTOP    ; -> BP = 0E400h
           ld a,0
           calln 0E0h      ; ED ED E0
             -> native nBIOSentry: reads AL = 0, prints banner, copies
                CCP+BDOS into emu_seg, patches the warm-boot and BDOS
                entry stubs, returns with iret
           jp CCPTOP       ; enter CCP
  ... every BIOS call is  ld a,<fn> / calln 0E0h / ret ...

exit (EXIT.COM, Z80):
  check system ID 'VA' at 0FFFEh
  retem                    ; ED FD -> native, at the instruction after brkem

native:
  restore vectors 0E0h/0E1h, INT 21h AH=4Ch
```

`nativeBios` (`0E0h`, the `CALLN` target) and `emVector` (`0E1h`, the
`BRKEM` target) are **two different vectors**. Earlier project notes
recorded them as one shared vector; that is wrong and not even
self-consistent, since the same address cannot be both a native routine
and Z80 code.

---

# Part III — Implementation notes

## 11. What the model must contain

`[DERIVED]` from Parts I and II. This is the minimum for a correct
implementation; it is not a design document.

1. **A mode latch, not a boolean.** `MD` is PSW bit 15 with a
   write-enable rule (§2.1). Keep the decode selector and the latch state
   separable; a single "currently Z80" flag is too weak for correct
   return and interrupt behaviour.
2. **Mode transitions are interrupt-shaped.** `BRKEM`/`BRKEM2`/`CALLN`
   push `PSW`, `PS`, `PC` to the native stack; `RETEM`/`RETI` pop them.
   No separate transition-context object, and no return-context stack of
   its own — nesting is prohibited (§2.3), so there is exactly one level.
3. **Two decode tables, switched wholesale.** The encoding spaces are
   disjoint (one Z80, one x86); there is no shared front end to build.
   `ED FD` and `ED ED` belong to the **compatible-mode** table only; the
   native table needs no `ED` special case.
4. **One register file.** Compatible mode reads and writes the same
   storage under the aliases in §3.1. No separate Z80 register struct.
   `SP` maps to the V30 `BP` slot — do not reflexively use the x86 `SP`.
5. **One flag byte.** Do not convert at transitions. Make flag
   *computation* mode-aware: bit 2 is `P` natively and `P/V` in
   compatible mode, and bits 1/3/5 carry `N`/`F3`/`F5` (§3.2).
6. **Segment selection by mode.** Compatible-mode code fetch uses `PS`;
   **all** data and stack accesses use `DS0` (§3.3).
7. **Prefetch flush on every mode-changing control transfer** —
   `BRKEM`, `BRKEM2`, `CALLN`, `RETEM`, `RETI`, and interrupt entry from
   compatible mode.
8. **Firmware conventions stay in firmware.** `CALLN 91h`/`95h`, the
   `1000:` window placement, and `1000:C003` are IVT and BIOS
   conventions (§6, §7, §8) — not CPU behaviour. Do not special-case
   them in the instruction core.
9. **Never route any of this to the FDD subsystem Z80.** Separate
   device, separate state, no shared code without an explicit later
   decision.
10. **Separate functional correctness from cycle accuracy.** Do not copy
    timing from another emulator.

## 12. MAME V20/V30 comparison

MAME already models the standard V30 emulation path and is a useful
implementation reference — but it does **not** implement `BRKEM2` as a
compatible-mode entry, so it is not a drop-in answer.

| Topic | MAME V30 behaviour | Implication here |
|---|---|---|
| Instruction family | Documents `BRKEM`, `RETEM`, `CALLN` for V20/V30/V40/V50. | Supports the analogy; does not prove the `BRKEM2` frame or latch rules. |
| Native entry opcode | Dispatches `0F FF nn` to `BRKEM` and calls a common break-to-emulation helper. | A `BRKEM2` case is needed in addition, not instead. |
| Mode state | Keeps a decode selector (`m_MF`) **and** a separate latch-like state (`m_em`) affecting whether restored flags may leave emulation mode. | Matches the write-enable rule in §2.1. Two fields, not one. |
| Entry stack frame | `nec_brk()` clears the native flag, pushes flags/`PS`/post-immediate `IP`, loads `PS:IP` from `IVT[nn]`. | The working `BRKEM2 90h` model: save `IP = 13B4h`, load `PS:IP` from `vec(90h) = 1000:0000`. |
| Decoder switch | Chooses the native table when the mode flag is set, the 8080 table when clear. | The main CPU needs its own compatible-mode decoder. |
| Return | `RETEM` lives in the 8080-mode table as `ED FD`; `CALLN` as `ED ED nn`. | Confirms the table placement in §11.3. |
| Prefetch | Control-transfer macro clears prefetch state on `BRKEM`, `RETEM`, calls, jumps, returns. | §11.7. |
| Timing | Assigns a concrete cycle cost to `BRKEM`; source comments treat some prefetch details as approximate. | §11.10. |

## 13. Current vaeg status

- The FDD subsystem Z80 is implemented separately — historically
  `cpucva/z80c.cpp`, currently `cpucva/z80_core.cpp`, with
  `iova/subsystem.cpp` and `VASUBSYS.ROM`.
- **The main CPU's compatible mode is not implemented.**
  `docs/agents/reports/m9_v30_map.md` records it as an explicit future
  item.
- `BRKEM`/`BRKEM2` must be modelled as mode-changing control transfers,
  not ordinary software interrupts.

---

# Part IV — Open questions and tests

## 14. Open questions

### 14.1 Resolvable from documents not yet obtained

| id | question | where the answer is |
|---|---|---|
| `doc-9002-man` | A device-level μPD9002 specification: exact saved frame, mode-latch write protection, prefetch, interrupt interaction, cycle timing | a μPD9002 manual, if one exists |
| `doc-brkem2` | What distinguishes `BRKEM2` from `BRKEM` | μPD9002 manual, or the `PS3` trace in §14.2 |
| `doc-c003` | What prepares the code at `1000:C003` | execution trace, or the V1/V2 sub-ROM |
| `doc-boot-cover` | Whether every boot configuration reaches `0x13B1` | CS:IP trace per ROM/configuration |

### 14.2 Requires hardware or the schematics

| id | question | default to assume | test |
|---|---|---|---|
| `ps3-decode` | Does the "88-mode emulator" gate array switch the I/O map on `PS3`? | unknown | trace the μPD9002 `PS3` pin on the VA1 schematics — cheapest high-value check available |
| `brkem2-target` | What mode/state does `0F FE imm8` establish? | trap and halt | §16.1 |
| `alt-regs` | Do `AF'/BC'/DE'/HL'` exist? | probably yes (§4.4) | §16.0 |
| `ix-iy-share` | Do `IX`/`IY` physically share `SI`/`DI`? | shared | §16.3 |
| `ed-block` | Do `LDIR`/`CPIR`/`INIR`/`OTIR` exist? | probably yes (§4.4) | §16.0 |
| `cb-dd-fd` | `CB`/`DD`/`FD` prefix spaces; half-index registers | `DD`/`FD` probably yes; `CB` and half-index unknown | §16.0, §16.4 |
| `ir-regs` | Do `I`/`R` exist? What does `LD A,R` return? | unknown | §16.5 |
| `undoc-flags` | Do `F3`/`F5` behave as on a real Z80? | unknown | §16.6 |
| `flag-callback` | Are flags set by a `CALLN`-invoked native routine visible after `RETI`? | no (plain PSW restore) | §16.7 |
| `z80-int-model` | Does the superset add `IM 0/1/2`, `I`, `IFF1/2`, `RETN`? | V30 behaviour only (§3.5) | hardware |
| `io-port-map` | How do Z80 `IN`/`OUT` ports reach the V30 I/O space? | direct | hardware |
| `iotrap-exempt` | Are `FFE0h`–`FFFFh` exempt from trapping? | exempt | program a range covering `FFEFh`, observe |

### 14.3 Resolved

| id | resolution |
|---|---|
| `retem-mode` | `RETEM` executes in compatible mode — §2.2 |
| `flag-layout` | Low PSW byte is the compatible-mode flag register — §3.2 |
| `emu-int` | Interrupt / `RESET` / `HALT` behaviour — §3.5 |
| `iotrap-init` | Vector and control-port installation order — §9.5 |
| `va-window` | V1/V2 window is at physical `0x10000`; `DS0 = 1000h` — §3.3, §8 |
| `calln-services` | `91h`/`95h` are IVT entries, not CPU behaviour — §6 |
| `retem-c003` | `1000:C003` is reached by an explicit `jmp` after `RETEM` — §8 |
| `v1v2-on-9002` | V1/V2 mode executes on the μPD9002 in compatible mode — §4.4 |

## 15. Confidence summary

| Item | Confidence | Reason |
|---|---|---|
| `BRKEM`/`CALLN`/`RETEM` semantics | High | NEC manuals plus a working shipped program for each |
| Compatible mode is Z80 | High | `JR` in the shipped `CPMBIOS.COM`; Debug 8800 runs in V1/V2 |
| Flag sharing | High | Figure 8-2, plus Debug 8800's own flag table |
| Address space model | High | Chapter 8 plus two independent worked instances |
| `BRKEM2` opcode identity | High | ROM installs `vec(90h)` then executes `0F FE 90` at an instruction boundary; VA notes define the encoding |
| `BRKEM2` semantics | Medium | Frame and latch rules are analogy from `BRKEM`, not confirmed |
| Runtime path coverage of `0x13B1` | Medium-high | The `000Dh` branch can skip the block |
| I/O trap mechanics | High | Full working reference implementation with source |
| I/O trap port list | Medium | `[VA-TEKU]`; the ROM's bracketing use is consistent but does not enumerate ports |
| `1000:C003` preparation | Medium | Target confirmed; no copy loop found; needs a trace |
| On-chip peripheral map | Low | Family analogy only |

## 16. Real-hardware test procedures

Two on-board instruments, dividing the work:

- **V3-mode PC-Engine monitor** (`varom00.rom`). Assembler and
  disassembler for NEC native mnemonics including `BRKEM` and
  `BRKEM2`/`BRKFEM`. Use it for the native side: vectors, issuing the
  mode-entry instruction, reading registers after `RETEM`. No Z80
  mnemonics, so compatible-mode payloads entered through it must be
  hexadecimal.
- **Debug 8800** — **this is the monitor `MON` gives you in V1/V2 mode.**
  It prints no banner, so it looks like "the usual monitor"; the
  `Debug 8800 version 1.0` string and the `Peeping Tom` message next to
  it are embedded text aimed at whoever dumps the ROM, not startup
  output. Four independent checks identify it (§16.-1). A Z80 monitor with an
  assembler (`a`), disassembler (`l`), memory dump/fill/move/edit, `g`
  with two breakpoints, port `i`/`o`, and an `x` command covering the
  full Z80 register set **including `IX`, `IY` and the alternates**.
  Where a test is about Z80 semantics rather than mode transition, this
  is the better instrument.

### 16.-1 Confirming that the V1/V2 `MON` monitor is Debug 8800

`[ROM]` `[DERIVED]` The identification rests on four things:

1. The bank at `varom00.rom:0x1E000` opens with the ASCII signature
   `"DB"` and maps to Z80 `6000h`–`7FFFh` — the PC-8801 N88
   extended-ROM bank window — and its entry shim saves and restores
   port `70h`, the bank-select register (§4.4). That is precisely a ROM
   bank entered from N88-BASIC.
2. The memory-test messages (`Testing TEXT RAM.`,
   `Testing Graphics RAM (Blue).`, …) live inside that bank at
   `0x1F3E7`. If you have seen those from the V1/V2 monitor, you have
   been running this code.
3. The command help at `0x1F4BB` is in the same bank.
4. The dispatch table at `0x1E0F5` gives exactly sixteen commands:
   `A L T R W V B M E I O G F X D S`.

At the prompt, three quick discriminators:

- `x` — should display `IX`, `IY` and the primed registers
  `A' F' B' D' H'` alongside the main set.
- `b` / `bh` — radix select, **hexadecimal or octal**. The octal option
  is a 1981 giveaway.
- `t` — memory test, printing the strings above.

If those match, no further setup is needed: V1/V2-mode code is already
compatible-mode code, so **no `BRKEM` or `BRKEM2` is required** to run
the tests below. You are inside the mode under test.

### 16.0 Z80 feature coverage — `alt-regs`, `ed-block`, `cb-dd-fd`

Debug 8800 does most of this for free:

1. Enter Debug 8800 in V1/V2 mode and issue `x`. If `IX`, `IY` and the
   primed registers display and edit correctly, `alt-regs` and the
   `DD`/`FD` part of `cb-dd-fd` are answered — the monitor cannot reach
   the alternate set except by executing `EX AF,AF'` and `EXX`.
2. `l` a region known to contain `ED B0` — Z80 `6E5Fh` in the Debug 8800
   bank — and single-step it with `g`. That answers `ed-block`.
3. Assemble `SLA A` with `a` and step it. That answers the `CB` part.

### 16.1 `0F FE` target — `brkem2-target`

Do **not** use `LDIR` as the discriminator; a negative result would be
ambiguous. Use a Z80-only, single-opcode instruction outside the `ED`
space. On an 8080, `10h`, `18h`, `20h`, `28h`, `30h`, `38h` are all
undocumented NOPs:

```
DJNZ:   06 02        LD B,2
        10 FE        DJNZ $          ; Z80: loops once; 8080: NOP NOP
JR:     18 01        JR +1           ; Z80: skips the next byte
        3E FF        (skipped)       ; 8080: falls through, A = FFh
        76           HALT
```

Set up a `0F FE` vector pointing at the payload, execute
`0F FE <vec>`, inspect. Repeat with `0F FF` as a control.

Then the more interesting half: check whether `BRKEM2` also changed
machine state that `BRKEM` does not — read a V1/V2-only port before and
after each entry, and watch `PS3` if a probe is available.

### 16.2–16.6 Z80 feature payloads

```
16.2  alt-regs      3E AA / 08 / 3E 55 / 08 / 76        ; A back to AAh?
                    01 34 12 / D9 / 01 78 56 / D9 / 76  ; BC back to 1234h?
16.3  ix-iy-share   DD 21 CD AB / 76   then RETEM and read SI natively
                    FD 21 .. .. / 76   then read DI
16.4  cb-dd-fd      3E 01 / CB 27 / 76                  ; A = 2 if SLA works
16.5  ir-regs       ED 5F / 76         ; LD A,R, twice with differing gaps
                    ED 47 / ED 57      ; LD I,A / LD A,I round-trip
16.6  undoc-flags   3E 28 / C6 00 / F5 / E1 / 76        ; F -> L, check bits 3,5
```

Test `CB` before `DD`/`FD`, since the index prefixes act on `CB` too.

### 16.7 Flag propagation across `CALLN`/`RETI` — `flag-callback`

The most valuable single test: it settles the flag model and explains or
refutes the CPMVA anomaly in §3.2.

```
native routine at vector n:
        F9           STC
        CF           RETI

compatible-mode caller:
        A7           AND A            ; clears CY
        ED ED nn     CALLN n
        38 xx        JR C,<set>       ; did the carry survive?
```

Report the answer into a register, `RETEM`, read it natively.

---

# Appendices

## Appendix A. Corrections to earlier project documents

Recorded so the errors are not reintroduced.

1. **`RETEM` executes in native mode; `ED FD` must be added to the native
   decode table.** Wrong on both counts. → §2.2, §11.3.
2. **A `CALLN`-invoked native routine returns via `RETEM`.** It returns
   via `RETI`/`IRET`. → §2.2.
3. **Flags must be converted at each transition.** The low PSW byte *is*
   the compatible-mode flag register. → §3.2.
4. **The MD flag.** Absent from earlier documents entirely. → §2.1.
5. **The `ED` prefix proves the mode is Z80.** Invalid — those are the
   encodings on the plain V20/V30 too. The real proof is `JR` in the
   shipped CPMVA binary. → §2.4.
6. **Vector `0E0h` serves as both the `BRKEM` and `CALLN` entry.** They
   are `0E1h` and `0E0h`. → §10.
7. **The four deleted instructions are all block I/O.** `INS` and `EXT`
   are bit field insert/extract. → §4.2.
8. **`INM`/`OUTM` are absent from the ROM string pool.** They are
   present, in both notations. → §4.2.
9. **The mnemonic for `0F FE` is `BRKFEM`.** That is the VA2 ROM's name;
   the VA ROM calls it `BRKEM2`. → §2.2.
10. **The I/O trap emulates V1/V2 ports, as 98IOE demonstrates.** 98IOE
    emulates PC-9801 ports; the V1/V2 port list is from the VA notes and
    the ROM's bracketing use, not from 98IOE. → §9, §9.6.
11. **The address space model was missing.** Data *and stack* use `DS0`;
    code uses `PS`. → §3.3.
12. **Interrupt behaviour in compatible mode was listed as unknown.** The
    V30-documented behaviour is a documented default. → §3.5.
13. **"The VA ROM contains no Z80 mnemonics."** True of the V3-mode
    monitor only; `varom00.rom` also carries Debug 8800. → §4.4.
14. **`ED ED` occurrences in the ROM images are candidate `CALLN`
    sites.** All are false positives — `LD DE,0EDC9h` + `LDIR` at
    `varom00.rom:0x1EE67`, `CALL 0EDEDh` at `varom1.rom:0x1831C` and
    `0x1AB90`, and a doubled-byte data table in an x86 region. The real
    `CALLN` services are IVT entries. → §6.
15. **`CALLN 91h`/`95h` are fixed CPU services requiring dispatch on the
    immediate.** They are ordinary IVT entries; `CALLN` is uniformly
    "call `vec(imm8)`". → §6, §11.8.
16. **`RETEM` has a context-dependent return target of `1000:C003`.** It
    has one return target; `1000:C003` is reached by an explicit `jmp`
    in the native resume block. → §2.2, §8.
17. **`BRKEM` does not exist on the μPD9002.** It does — CPMVA uses it.
    The VA *firmware* uses `BRKEM2` instead. → §2.2.
18. **"Neither ROM contains Z80 mnemonics."** Wrong as stated. The
    earlier scan searched plain ASCII and a mnemonic list that omitted
    the relevant entries. Debug 8800's mnemonic pool is
    high-bit-terminated and carries **both** Zilog and Intel 8080
    notations. Only the V3-mode monitor is NEC-native-only. → §4.3, §4.4.
19. **The I/O-trap port list rests on the てくまに.** It does not — it is
    CoBit's, from his 1992 post, along with the rest of §9. That also
    means §9 has a single author and his own error caveat, rather than
    two independent sources. → §9.
20. **Debug 8800 must be reached somehow.** It is the monitor `MON`
    already gives you in V1/V2 mode; it simply prints no banner.
    → §16.-1.

## Appendix B. NEC / Intel mnemonic correspondence

`[ROM]` The V3-mode monitor carries two parallel mnemonic tables.
Extracted in table order.

```
NEC        Intel            NEC        Intel           NEC        Intel
ADDC       ADC              MOVBK      MOVS            BR         JMP
SUBC       SBB              CMPBK      CMPS            BRK        INT
MUL        IMUL             LDM        LODS            BRKV       INTO
MULU       MUL              STM        STOS            RETI       IRET
DIV        IDIV             CMPM       SCAS            DBNZ       LOOP
DIVU       DIV              INM        INS             DBNZE      LOOPE
ADJBA      AAA              OUTM       OUTS            DBNZNE     LOOPNE
ADJBS      AAS              PREPARE    ENTER           BCWZ       JCXZ
ADJ4A      DAA              DISPOSE    LEAVE           POLL       WAIT
ADJ4S      DAS              LDEA       LEA             BUSLOCK    LOCK
CVTBW      CBW              TRANS      XLAT            HALT       HLT
CVTWL      CWD              XCH        XCHG            NOP        NOP
CVTBD      AAM              SHRA       SAR             CHKIND     BOUND
CVTDB      AAD              ROLC       RCL             DS0/DS1    DS/ES
                            RORC       RCR             PS         CS
```

Registers: `AW BW CW DW` = `AX BX CX DX`; `IX IY` = `SI DI`;
`PS DS0 SS DS1` = `CS DS SS ES`; `PC` = `IP`; `PSW` = `FLAGS`.
Flags: `CY V P S Z AC DIR IE BRK MD` = `CF OF PF SF ZF AF DF IF TF` plus
the mode flag.

## Appendix C. ROM image map

`[ROM]` From 8 KiB block opcode-profile classification, banner strings
and internal address cross-checks. **Nothing is compressed**; every image
found so far is stored plain.

### `varom1.rom` — 128 KiB

| Offset | Size | Content |
|---|---|---|
| `0x00000`–`0x0FFFF` | 64 KiB | x86 (V3-side firmware). Opens with an `E9 disp16` near-jump table. Contains the vector installer and the `BRKEM2` handoff (§6, §8). |
| `0x10000`–`0x17FFF` | 32 KiB | **Z80. NEC N-88 BASIC Version 1.9**, banner at `0x179C7`, "Copyright (C) 1981 by Microsoft". Maps to Z80 `0000h`–`7FFFh`. |
| `0x18000`–`0x1FFFF` | 32 KiB | **Z80. NEC N-88 BASIC Version 2.4**, banner at `0x1CF97`. BASIC error strings at `0x1C000`; `C3 nnnn` entry-vector table at `0x1E000`. |

Verification: `varom1.rom:0x10000` reproduces a `0000h`–`00FFh` dump
taken from a real PC-8801 running N88-BASIC 1.93 byte for byte —
`F3 31 A0 E1` (`DI` / `LD SP,0E1A0h`), `C3 E5 3B` at `0004h`,
`C3 69 E6` at `0038h` (the `IM 1` vector).

### `varom00.rom` — 512 KiB

Predominantly x86 V3 firmware. Exactly one Z80 region:

| Offset | Size | Content |
|---|---|---|
| `0x1E000`–`0x1FFFF` | 8 KiB | **Z80. Debug 8800 v1.0** (§4.4). Maps to Z80 `6000h`–`7FFFh`, the PC-8801 N88 extended-ROM bank window. |

The V3-mode PC-Engine monitor, with its NEC-mnemonic assembler and
disassembler tables, is at `0x5CC80`–`0x5D1A0`.

### `varom00_va2.rom` — 512 KiB

The PC-88VA2 counterpart. Same structure; the `0F` extension table is at
`0x66AAF` and names `0F FE` as `BRKFEM`.

### `varom08.rom` — 128 KiB

Entirely `0xFF`. Unpopulated.

### Not located in any supplied image

- Any genuine `CALLN` instruction (all `ED ED` hits are false positives).
- The V1/V2 sub-ROM / disk BIOS.
- Whatever prepares `1000:C003`.
