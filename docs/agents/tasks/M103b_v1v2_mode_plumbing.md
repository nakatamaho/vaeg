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

# M103b - V1/V2 mode plumbing: BRKEM2, boot select, 88-mode window, I/O trap

Status: **research in progress; G103b pending**

Series: V1/V2 mode (ADR-0016). Branch `topic/m103b-v1v2-plumbing` off the
integration branch `topic/v1v2-mode`. Following the maintainer's M103b
request, these branches were created at `33f268cc5487e4feffdde75bcb495c4657cdcf94`
on top of the pending M103a branch, not `main`. This does not mark G103a
passed or authorize a main merge; reconcile the dependency before merging.
Commit prefix `M103b:`. Plan and hardware model:
[`docs/modernization/v1v2-mode-plan.md`](../../modernization/v1v2-mode-plan.md).

## Goal

Make the VA2 boot ROM take its own V1/V2 path end to end: boot selection,
display/memory setup in V3, `153H` bit 6 ← 0, I/O trap on, `BRKEM2 90h`,
and N88-BASIC running from `1000:0000` inside the 88-mode window until it
waits for the keyboard. No display or keyboard work in this milestone; the
gate is trace-based. Follow the real machine's automatic FDD selection;
there is no user-facing V1/V2 boot override. The BRKEM2 decoder does not
select the memory map or force a boot path. Verify unchanged V3 disk
boot and automatic selection of a compatible disk at the human gate.

## Scope

1. **`BRKEM2 imm8` (`0F FE nn`)** in `cpu/upd9002/`: enter the
   uPD70008-compatible mode through vector `nn` exactly as `BRKEM` does
   (ADR/CPU document §2.2.2: no architectural difference has been found;
   the machine-level switch is the `153H` write four instructions earlier).
   Keep one implementation and record the entry encoding in a trace
   event for `brkem2-target` (§15.2) should a difference appear later.
   Decode in the existing `0F` dispatcher; self-test in the M76 harness.
   There is no native instruction disassembler in the active tree (the
   uPD780 disassembler is unrelated); no disassembler entry is added.
2. **Boot selection.** Preserve the existing active-low PC-key input
   (`000Dh` bit 2) in `io/serial.c`. Port `40h` bit 3 is SW7: zero tries
   intelligent-FDD boot; one skips it. The diagnostic trace with SW7=1
   and the PC key unpressed reaches `BRKEM2` (plan §5), but bypasses the
   disk-identification path and is not an acceptance test. Keep SW7=0
   and trace the FDD command/result branches through `1F90h`. Do not
   replace automatic media selection with a GUI/INI mode selector.
3. **`153H` bit 6 — system memory mode.** `io/memctrlva.c`: store the bit,
   read it back, and switch `memoryva` between the V3 map and the 88-mode
   map for physical `10000h`–`1FFFFh` (and `20000h`–`3FFFFh` as ERAM
   pages). Save state.
4. **88-mode window.** In `memoryva/`, a window model for `10000h`–`1FFFFh`
   driven by the hardware banking ports (audit and extend existing
   handlers rather than attaching over them): `31H` OUT (`MMODE`, `RMODE`, `VW1`, `GDENO`, `PM00`), `32H`
   (`ROMSL`, `TMODE`, `GVAM`, `PMODE`, `AVC`), `5CH`–`5FH` (plane select
   into `C000h`–`FFFFh`), `70H`/`71H`/`78H`, `E2H`/`E3H`, with N88-BASIC
   and its extension banks taken from the existing VA ROM images
   (`varom1.rom` `0x10000` and `0x18000`–`0x1FFFF`; CPU document §4.6 and
   Appendix C). **Derive the exact page selection from the ROM**: trace
   the V3-side setup in the handoff block and the shared initialiser
   (`0x13A8`/`0x2210`, §8) and the first instructions N88-BASIC executes,
   and record the derivation with `[ROM]`/`[DERIVED]` tags in
   `v1v2-mode-plan.md` §2. TVRAM (`1F000h`) and GVRAM (`1C000h`) backing
   are stubs in this milestone (plain RAM that M103c/M103d replace), but
   the port-side state (`5CH`–`5FH`, `34H`/`35H`, `TMODE`) is kept.
5. **I/O trap hardware** (§9): registers `FFE0h`–`FFE7h` (byte and word
   writes) and `FFEFh`; matching on the low byte in word mode; in both
   CPU modes, a trapped `IN`/`OUT` performs no access and raises native
   interrupt `7Ch`/`7Dh` with `CS:IP` at the I/O instruction and the
   interrupt flag cleared, exactly the frame 98IOE and the ROM handler
   expect (§9.2). Ports `FFE0h`–`FFFFh` never trap. The compatible-mode
   adapter's `In`/`Out` must route through the same check before
   `iocore_inp8`/`iocore_out8`. Unit test with a tiny native handler that
   advances the saved IP; adapter self-test with a Z80 `OUT (50h),A`.
6. **8214 (`E4H`/`E6H`) and kanji ROM (`E8H`–`EDH`) ports**: attach as
   state-holding stubs so the ROM's initialisation does not fault; their
   semantics belong to M103c.
7. **Trace.** Extend the compat trace with the mode-entry encoding, the
   `153H` state and trap events, so the gate can be checked from a log.

Out of scope: text/graphics rendering, keyboard matrix, interrupt
delivery into Z80 code (M103c), FDD under V1/V2 (M103d), V1 mode.

## Deliverables

- Code as above with save-state coverage (new fields versioned per M85).
- `docs/modernization/v1v2-mode-plan.md` §2 window map derived from the
  ROM; CPU document §15.2 `bootsel-000d` and `brkem2-target` updated with
  what the trace shows.
- Tests: `BRKEM2` self-test, trap unit/self-tests, window-map unit tests
  against the derived table, ROM-driven automatic disk-selection trace
  test (romful, skipped without the private ROMs and media).
- Ledger entries for concrete defect corrections under AGENTS.md, including
  V1/V2 defects; feature-only additions do not require entries.

## Gate G103b (human)

Standard gate for V3 (clean build, V3 boot, VA demo, OS boot) unchanged,
plus, with the VA2 ROM set and compatible boot media, without overrides:

- the trace shows the ROM reading `000Dh` with the PC key unpressed,
  taking the `0x136C` path, reading SW7=0 at `40h` and executing the FDD
  boot call; record the media-dependent branch and return to the handoff.
  The reset/handoff sequence must show programming `FFE0h`–`FFE7h`, writing `FFEFh ← 03h`, clearing `153H`
  bit 6, executing `BRKEM2 90h`, and Z80 execution starting at
  `1000:0000`;
- N88-BASIC's initialisation runs inside the window (ROM-bank switching
  through `31H`/`32H`/`71H` visible in the trace) and reaches a stable
  keyboard-scan loop on ports `00h`–`0Eh` (which read as no key);
- no trap fires outside `50h`–`5Bh`/`60h`–`6Fh`, and every trap that
  fires returns through the ROM handler with the IP advanced.

## Implementation progress

- `0F FE imm8` now shares the existing `BRKEM` entry routine; this is an
  implementation policy, not a measured proof of complete architectural
  equivalence. No memory-mode bit is changed by the instruction.
- `compat-entry` CPU trace events record the second opcode byte in
  `address` (`FE` or `FF`) and the vector in `value`. No persistent CPU
  field or save-state format change is needed for this diagnostic.
- The production-adapter test runs both entry encodings, checks the
  six-byte native return frame and preserved DS/SS, then exercises
  compatible instructions, CALLN, nested native interrupt/IRET, RETEM,
  and compatible-state restore. This is ROM-less testing, not G103b.
- The SW7 GUI/INI option from `a4d85e64835ab0274731682c92332bb0bab4d368`
  is withdrawn following maintainer clarification: normal hardware selects
  V1/V2 automatically from the FDD. The forced-SW7 trace only isolated
  entry decoding. Memory mapping and I/O trapping remain pending.
- Bounded traces now confirm automatic FDD selection for synthetic 256-
  and 512-byte-sector 2D media and maintainer-provided BASIC media (plan
  §5.2). BASIC reaches BRKEM2 without SW7 changes, but then fetches zero
  bytes instead of BASIC ROM. All instrumented runs timed out after
  capturing this prefix; G103b has not passed. The next implementation
  target is the 88-mode memory decoder, not a boot-selection override.
- The memory-mode latch has now been recovered from the saved work,
  without the withdrawn SW7 override. Port `153h` bit 6 latches and reads
  back; reset selects V3 (`41h`). The production I/O regression covers
  byte writes, the ROM's word read/write at `152h`, and reset after an
  88-mode request. The initial latch-only step is now extended with the
  partial ROM overlay described below.
- New optional binary state section `MEM88MODE`, version 0, stores one
  byte (0=V3, 1=88-mode request). The old `MEMORYVA` structure and its
  reserved bytes remain untouched. State loading calls `iocore_reset`
  before section loading, so an old state without `MEM88MODE` keeps V3.
  Complete old/new state-file round trips remain unverified; current
  tests cover the I/O/reset path, not the full loader.
- Initial N88 ROM overlay is implemented for reads at physical
  `10000h`–`17FFFh`, selected by the mode latch and port 31h bits 1/2.
  Optional `MEM88SYS` saves the port 31h latch. Synthetic tests cover
  both word boundaries, MMODE switching, and RAM writes under ROM.
  Physical RAM backing is provisional; monitor ROM, extension banks,
  text windows, ERAM and video mappings remain pending (plan §2.0).
  An automatic-FDD BASIC run now executes the ROM entry instructions,
  including `LD SP,E1A0h` and `JP 3BE5h`; 600-frame capture exits 0 but
  does not establish a BASIC prompt. Full state-file testing is pending.
- Normal CPU/DMA accesses already pass through the byte/word VA decoder
  in `memoryva/memoryva.c`; only the isolated SST flat-memory seam
  bypasses it. Add the window there, including word-boundary handling,
  rather than introducing a second decoder in the compatible adapter.
- Local build and all 106 CTest entries complete without failures (one
  external SST skipped). Human boot acceptance is not inferred from
  these tests. The original named stash remains as a recovery copy.

## Research notes for the implementer

- Hardware-versus-trap port table and the TVRAM/GVRAM mapping: plan §2–§3
  (BNN manual ch. 8; CPU document §9).
- The ROM's V1/V2 path and resume block are disassembled in CPU document
  §8/§8.2 (VA2 offsets; VA1 counterparts given).
- X88000 is the reference for what 8801 software expects from `31H`,
  `32H`, `5CH`–`5FH`, `70H`/`71H`, `E2H`/`E3H` (ADR-0016: behaviour only,
  derivations recorded).
