<!--
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.
-->
# V1/V2 mode on the PC-88VA: hardware model and implementation plan

Companion to [`upd9002-upd70008-mode.md`](upd9002-upd70008-mode.md)
(the CPU) and ADR-0016 (the milestone series). Provenance tags are those
of §0 of the CPU document. This page answers one question: **what does
the VA do, in hardware and in firmware, to run PC-8801 software — and
which parts does vaeg still lack?**

## 1. The model in one paragraph

`[VA-TM]` V1/V2 mode is "a mode that emulates the 88M/F series: not only
the CPU but the memory configuration and the display system run in
emulation mode" (BNN technical manual, chapter 8). Three independent
switches make it: the CPU enters Z80 emulation with `BRKEM2 90h`
(§2.2.2, §8 of the CPU document); the **memory system** is switched to
the 88-mode map by port `153H` bit 6 = 0 (§8.2); and the **I/O trap** is
enabled around the session so that a small set of 8801 ports is emulated
by firmware (§9). Everything else — the 8801 I/O ports, the text and
graphics display, the FDD sub-system — is **VA hardware operating in a
compatibility configuration** that the firmware programs in V3 mode
before the handoff. vaeg has the compatible instruction adapter
(M76–M103a); M103b now decodes `BRKEM2` using the existing entry routine.
The rest of this page covers the machine side still needed to boot.

## 2. Memory: the 88-mode window

`[VA-TM]` ch. 8.1. With `153H` bit 6 = 0, physical `10000h`–`1FFFFh`
("88 MODE area") becomes the 8801's 64 KiB space; the Z80 runs with
`PS = DS0 = 1000h`. The window maps "part of the V3-mode main RAM, VRAM
and ROM" so that the 88M/F memory map appears there. `20000h`–`3FFFFh`
is addressable as PC-8801-02N-compatible extension RAM (ERAM pages).
CPU execution mode does not itself select the memory map: `153H` bit 6
is a separate control, written by native firmware before `BRKEM2`.
Mappings outside the window and native trap-handler accesses need to be
checked against the manual rather than inferred from CPU mode.

Inside the window the 8801's own banking applies, through ports the VA
implements **in hardware** (`[VA-TM]` port table, "V1/V2 モード専用"):

| Port | 8801 meaning | VA status |
|---|---|---|
| `31H` OUT | system port 1: `MMODE` RAM/ROM, `RMODE` N88/N80 ROM, 200/400 line, graphics enable | hardware |
| `32H` | `ROMSL` extension-ROM bank, `TMODE`, `GVAM`, `PMODE`, `AVC` | hardware |
| `34H`, `35H` OUT | GVRAM control (ALU / access mode) | hardware |
| `5CH`–`5FH` | GVRAM plane select into `C000h`–`FFFFh` | hardware |
| `70H`, `71H`, `78H` | text window offset, extension ROM, offset increment | hardware |
| `E2H`, `E3H` | extension RAM select | hardware |
| `E4H`, `E6H` OUT | 8214 interrupt level / mask | hardware |
| `E8H`–`EDH` | kanji ROM | hardware |

`[ROM]` The 8801 ROM payloads are already in the VA ROM set: N88-BASIC
(32 KiB), its four extension banks and Debug 8800 live in `varom1.rom`
at `0x10000` and `0x18000`–`0x1FFFF` and in `varom00.rom` (CPU document
§4.4–§4.6, Appendix C). `[UNKNOWN]` Which physical ROM/RAM pages the
window selects for each `31H`/`32H`/`71H` state is not written in the
sources in hand; it is what M103b must derive from the ROM's own window
setup (the handoff block §8, the shared initialiser `0x2210`) and from
8801 behaviour, and verify by running the ROM.

### 2.1 TVRAM in 88 mode

`[VA-TM]` ch. 8.2.1. 4 KiB of the V3 TVRAM — `A6000h`–`A6FFEh` — is
mapped to `1F000h`–`1FFFFh`. The TSP is put into byte-access mode and the
area is used in **µPD3301 VRAM format** (character codes and 8801
attribute codes); the TSP converts the attribute format automatically,
and the 8 text colours become colour codes 8–15. `A0000h`–`AFFFFh` is not
accessible as TVRAM in this mode and DMA cannot reach it. Not emulated by
the TSP: alternate-line display, block-cursor reverse, special control
characters and their interrupts, horizontal-counter clear, light pen,
attribute-less mode, non-transparent monochrome attribute mode. One of
the four split screens shows the 8801 text; its parameters must be set in
V3 mode beforehand (the firmware does this). 40-column mode doubles the
even characters and hides the odd ones; attribute columns are still
counted in 80.

### 2.2 GVRAM in 88 mode

`[VA-TM]` ch. 8.3. Multi-plane mode, screen 0 only, 640 × 200 (15 kHz) or
640 × 400 (24.8 kHz). Each plane is 16 KiB at `1C000h`–`1FFFFh`,
selected by `OUT 5CH`/`5DH`/`5EH` and backed by V3 planes 0–2 at
`A4000h`–`A7FFFh`; plane 3 is unused and must be masked by the screen
switch in 4-bit pixel mode. Frame buffer 0 only; no split, scrolling
allowed. Palette 0 emulates the M/F palette, palette 1 is the 8-colour
direct table; **M/F-format palette writes and the background-colour
register are emulated by the I/O trap** because the register formats
differ (ch. 8.4.2). Composition: backdrop (mode 0), graphics screen 0,
text; sprites unused but not disabled by hardware.

## 3. I/O: hardware versus trap

`[VA-WIKI]`/`[ROM]` (CPU document §9) The trap windows are `50h`–`5Bh`
and `60h`–`6Fh`, programmed at reset, enabled (`FFEFh ← 03h`) just before
`BRKEM2` and disabled after `RETEM`. The handler acts on `50h`–`53h`
(µPD3301 CRTC, background/border, screen overlay), `60h`–`68h` (µPD8257
DMAC, which the 8801 uses to feed the CRTC) and `6Eh`–`6Fh`. Everything
else in the 8801 port space is hardware:

| 8801 ports | Device | In vaeg today |
|---|---|---|
| `00h`–`0Eh` | keyboard matrix scan (88M/F compatible interface) | present (`io/serial.c`); V1/V2 guest input remains to be verified |
| `10h` | calendar/printer strobe | present |
| `20h`, `21h` | µPD8251 serial | present |
| `30h`, `31h` IN | DIP switches / system port | present (31h IN only) |
| `31h` OUT, `32h`, `34h`, `35h` | memory mode, extension ROM, GVRAM control | compatibility banking missing; existing port handlers require individual audit |
| `40h` | system port: strobe, VRTC, CMT, beep | present |
| `44h`–`47h` | YM2203/2608 OPN | present |
| `50h`–`53h`, `60h`–`68h`, `6Eh`, `6Fh` | trapped (firmware) | **trap hardware missing** |
| `5Ch`–`5Fh`, `70h`, `71h`, `78h`, `E2h`, `E3h` | banking | **missing** |
| `E4h`, `E6h` | 8214 interrupt controller | **missing** |
| `E8h`–`EDh` | kanji ROM | **missing** |
| `FCh`–`FFh` | µPD8255 to the FDD sub-CPU | present (`io/subsystem.cpp`, `UPD780C`) |

`[DERIVED]` The FDD side needs no new device: the VA's disk sub-system
*is* an 8801-style sub-CPU with its own `disk.rom` (§4.6), and vaeg runs
it already for V3.

## 4. Interrupts in compatible mode

`[V30-MAN]` An interrupt taken in Z80 emulation is serviced in native
mode through the IVT and returns with `RETI` (§3.5). `[DERIVED]` 8801
software expects Z80 `IM 2` vectors from the 8214 (`E4h`/`E6h`), so the
firmware's native handlers must deliver the interrupt into the Z80 code
by rewriting the saved frame (push the Z80 return address on the `BP`
stack, set `PC` from the 8801 vector table). `[UNKNOWN]` How exactly the
VA2 ROM does this — which native vectors, which table, how `E4h`/`E6h`
feed the ICU — is the largest open research item of the series and the
first task of M103c; it is ROM analysis, not hardware measurement.

## 5. Boot path

`[ROM]`/`[VA-TM]` §8: `000Dh` bit 2 is the active-low **PC key**, not
an independent memory-mode switch. `io/serial.c` already binds keyboard
rows `00h`–`0Eh`; scan code `7Ah` maps to row `0Dh`, bit 2. An unpressed
key reads as one. The VA2 ROM then takes the candidate V1/V2 path at
`F000:136Ch`, but that alone does not guarantee reaching `BRKEM2`.

The next decision is **SW7**, port `0040h` bit 3. The manual defines
zero as boot from the intelligent FDD, and one as do not boot from it.
`io/sysportva.c::sysp_i040()` defaults to zero for this bit. With
zero the ROM calls `1F90h` to try the FDD boot path; with one it skips
that call. Successful IPL loading and subsequent disk-dependent paths
must not be conflated with a direct CPU-mode switch.

A local vaeg diagnostic run with SW7 temporarily forced to one and the
PC key unpressed observed this VA2 ROM sequence (emulator observation,
**not real-machine measurement**):

| Address in segment F000h | Action |
|---|---|
| `13EAh` | keyboard read returns `FFh` at port `000Dh` |
| `136Ah`–`1375h` | PC-key branch and SW7 branch skip the FDD call |
| `1375h` → `18E7h` | write `03h` to `FFEFh`, enabling the I/O trap |
| `1378h`–`138Ch` | probe RAM at physical `80000h`; conditionally write `15Ah` |
| `138Dh`–`139Dh` | clear bit 1 at offsets `0121h`, `0129h`, `0131h` in segment A000h |
| `13A0h`–`13A7h` | read word at port `152h`, clear bit 14, write it back (153h bit 6) |
| `13A8h`–`13AFh` | set DS to `1000h` and ES to zero |
| `13B1h` | execute `0F FE 90`, the unsupported `BRKEM2 90h` entry |

The pre-implementation diagnostic reports `reserved 0f fe` at `13B2h`, the position of the
second opcode byte, not the instruction start. It does **not** establish
successful compatible-mode entry or BASIC execution. The temporary SW7
force and diagnostic prints are not production changes. A boot option
must preserve the existing PC-key matrix and default SW7=0 behavior;
forcing `000Dh` bit 2 alone is insufficient.

M103b now exposes SW7 through `v1v2_boot = true` in the `[NekoProjectII]`
INI section and the experimental GUI boot menu. It is off by default
and changes only SW7, not the PC key. The DIP input is live host
configuration, not part of a guest save-state section; rerun boot by
resetting after changing it. It skips intelligent-FDD boot, so it is
not yet the policy for booting a V2 BASIC disk.

With this setting and the new BRKEM2 decoder, a local VA2 emulator run
enters `1000:0000` with saved native frame `13B4/F000/F044`. It then reads
`00h` instructions instead of BASIC ROM because the 88-mode window is
still missing. The 600-frame smoke check fails with a uniform screen;
this is not a successful BASIC boot. The same smoke check with default
configuration exits successfully without a compatible-mode entry.

## 6. What vaeg has and lacks

| Piece | Status |
|---|---|
| Z80 emulation mode (uPD70008-compatible adapter, R1–R19, CALLN/RETEM, live IVT, alternate set) | done (M76–M103a) |
| `BRKEM2` (`0F FE nn`) | implemented with shared BRKEM entry policy; both encodings pass ROM-less round-trip tests; real-machine equivalence unmeasured |
| `153H` bit 6 memory mode | ignored (`io/memctrlva.c` reads back `0x40` always) |
| 88-mode window at `10000h`–`1FFFFh` with 8801 banking ports | missing |
| TVRAM `1F000h` mapping, TSP byte mode and 3301 attribute conversion | missing |
| GVRAM plane select `5Ch`–`5Fh` into `1C000h` | missing |
| I/O trap (`FFE0h`–`FFEFh`, vectors `7Ch`/`7Dh`, §9.2 semantics) | missing |
| keyboard matrix interface `00h`–`0Eh` | present; V1/V2 guest validation pending |
| 8214 `E4h`/`E6h`, kanji ROM `E8h`–`EDh` | missing |
| boot inputs: `000Dh` bit 2 / PC key, `40h` bit 3 / SW7 | PC key present; experimental INI/GUI SW7 option added, default zero |
| FDD sub-CPU, OPN, 8251, printer, system ports `30h`/`40h` | present |

## 7. Milestones (ADR-0016; V2 first)

| Milestone | Scope | Observable step |
|---|---|---|
| M103b | `BRKEM2`; boot selection; `153H` bit 6 and the 88-mode window with the hardware banking ports; I/O trap hardware; ROM-driven derivation of the window map | the VA2 ROM takes the V1/V2 path, hands off with `BRKEM2 90h`, N88-BASIC initialises and reaches its keyboard wait (trace-verified; no display yet) |
| M103c | interrupt delivery into Z80 code (ROM analysis), keyboard matrix, TVRAM 88-mode mapping and TSP emulation-mode text rendering | `Ok` prompt displayed, typed characters echoed |
| M103d | GVRAM plane mapping and 88-mode graphics (palette modes, 200/400 lines, backdrop), FDD path under V1/V2, remaining ports (`E8h`–`EDh`, `34h`/`35h`) | **N88-DISK BASIC V2** boots from a disk image and operates; graphics statements draw |
| M103e | timing and compatibility work against the **PC-8801mkIISR DEMO**; sound; V1 mode | the DEMO runs; series gate |

X88000 (public domain) is the behavioural reference for the 8801 side of
each row — port semantics, µPD3301 and attribute behaviour, keyboard
matrix layout, GVRAM ALU — never a source of the VA-side mechanism.
