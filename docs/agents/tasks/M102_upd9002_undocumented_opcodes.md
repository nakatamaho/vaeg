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

# M102 - µPD9002 undocumented-opcode coverage

Status: **in progress (scope A approved on 2026-09-30)**

Predecessor: G101 passed; M101 merged to `main` at
`a651ae761fb08619d0ade5cf596c356c0a1b34c7`.

Branch: `topic/m102-upd9002-undocumented-opcodes`

Commit prefix: `M102:`

Evidence document: `docs/modernization/uPD9002-zex-results.md`

## Background

M101 established R1–R11 for the µPD9002 Z80 emulation mode from ZEXDOC,
ZEXALL, direct probes and exhaustive dumps on a real PC-88VA2. Open item O3
remains: the ZEX groups labelled `<inc,dec> iyh` and `<inc,dec> iyl` vary IY
but execute DD-prefixed opcodes (DD 24/25, DD 2C/2D), so INC/DEC IYH/IYL
(FD 24/25/2C/2D) was never executed on the machine.

ZEX also leaves several other undocumented Z80 instructions untested. The
real-machine run costs the maintainer a disk round trip, so M102 covers O3
and those instructions in one run.

Two defects found while preparing M102 are part of its scope (maintainer
decision A, 2026-09-30):

- **ZEX descriptor bug.** Besides the DD/FD mix-up, the stock
  `<inc,dec> ixh/ixl/iyh/iyl` counters vary the wrong field: the IX groups
  vary IY and the IY groups vary the memory operand. All four groups
  therefore increment and decrement one fixed value, which explains
  `uPD9002-zex-results.md` §6 C2.
- **Null opcode handlers in the vendored core.** suzukiplan/z80 leaves 196
  ED entries and 90 DD and 90 FD entries of its opcode tables empty. With
  exceptions it throws; vaeg builds with `Z80_NO_EXCEPTION`, where executing
  such an opcode calls a null function pointer and crashes the emulator
  (verified for ED 00, ED 4C and ED A4). A Zilog Z80 treats undefined ED
  opcodes as two-byte NOPs, the NEG/RETN/IM duplicates as the base
  instruction, and a DD/FD prefix before a non-index instruction as ignored.

## Scope

Three ZEX-derived CP/M programs, built like `ZEX13S` (file-output ZEX,
separate test table, `msbt` unchanged, descriptors copied from the stock
ones with only the named fields changed). Every group runs twice, with flag
mask D7h and FFh. Expected CRCs are those of the Zilog profile, computed on
the host.

### `ZEXIY.COM` → `ZEXIY.TXT` (O3)

| Group | Opcodes | Derived from | Change |
| --- | --- | --- | --- |
| `inc,dec ixh` | DD 24/25 | `incxh` | counter on IX (stock: IY) |
| `inc,dec ixl` | DD 2C/2D | `incxl` | counter on IX (stock: IY) |
| `inc,dec iyh` | FD 24/25 | `incyh` | prefix DDh → FDh; counter on IY (stock: memory operand) |
| `inc,dec iyl` | FD 2C/2D | `incyl` | prefix DDh → FDh; counter on IY (stock: memory operand) |

The IX groups are the reference for the IY groups under both masks.

### `ZEXUND.COM` → `ZEXUND.TXT` (other undocumented instructions)

| Group | Opcodes | Derived from | Change |
| --- | --- | --- | --- |
| `rot (ix,iy+d) -> r` | DD/FD CB d 00–3F | `rotxy` | fourth-byte counter 07h → all 64 opcodes |
| `bit n,(ix,iy+d) all` | DD/FD CB d 40–7F | `bitx` | fourth-byte counter covers all low bits |
| `res/set (ix,iy+d) -> r` | DD/FD CB d 80–FF | `srzx` | fourth-byte counter covers all 128 opcodes |
| `neg (all encodings)` | ED 44/4C/54/5C/64/6C/74/7C | `negop` | second-byte counter 38h |
| `dd/fd inc,dec r` | DD/FD 04/05/0C/0D/14/15/1C/1D/3C/3D | `inca` etc. | redundant prefix before INC/DEC B, C, D, E, A |

### `ZEXED.COM` → `ZEXED.TXT` (undefined ED opcodes)

Groups of undefined ED opcodes, each expected to act as a two-byte NOP on a
Zilog Z80: ED 00–3F, ED 77/7F, ED 80–9F, ED A4–A7/AC–AF/B4–B7/BC–BF and
ED C0–FF without ED ED (CALLN) and ED FD (RETEM). This program runs last;
an opcode that traps on the µPD9002 would stop it.

The DDCB/FDCB groups include the register-copy forms (the result is also
written to B, C, D, E, H, L or A); the ZEX machine state covers those
registers.

### Excluded, with reasons

- `IN F,(C)`, `OUT (C),0` and other I/O instructions: they access real
  PC-88VA ports, which can have side effects and do not give repeatable
  input values.
- `ED ED` (CALLN) and `ED FD` (RETEM): µPD9002 mode-switch instructions.
- RETN and IM duplicates (ED 55/5D/65/6D/75/7D, ED 4E/66/6E/76/7E): they
  change the return address or the interrupt mode, which the ZEX harness
  cannot contain.
- `LD A,R`: R is not reproducible.

### Hang protection

An undocumented opcode that the µPD9002 does not implement could hang the
program before it closes its output file. The programs are therefore
separate and run in order `ZEXIY`, `ZEXUND`, `ZEXED`, and every result line
is also shown on the console so that a partial run can still be read from
the screen.

## Work

0. Core fix, as a downstream patch applied through the provenance procedure
   and offered upstream as a separate pull request: fill every empty ED, DD
   and FD entry with the Zilog behavior (undefined ED = two-byte NOP;
   NEG/RETN/IM duplicates = base instruction; DD/FD before a non-index
   opcode = prefix ignored, which also covers repeated prefixes). Test all
   256 opcodes of each table in the production configuration, and record
   the crash in `docs/modernization/bug-fixes.md`. Until real-machine data
   exists, the `Upd9002` profile uses the same behavior.
1. Add `zexiy.patch`, `zexund.patch` and `zexed.patch` (GPL-2.0-or-later,
   like `zex13s.patch`) and build `ZEXIY.COM`, `ZEXUND.COM` and `ZEXED.COM`
   in `zexbuild.py` with the same `msbt` and prefix checks.
2. Compute the Zilog expected CRCs on the host and record them in the
   patches. Acceptance: under the Zilog profile both programs report OK for
   every group.
3. Record the uPD9002 host predictions. Extend `host_reference.py`,
   `compare.py` and the parsers with the three programs.
4. Add both programs to `cpmva-tools.d88` and `cpmva-zexall-test.d88`, and
   update `docs/cpmva-setup.md`.
5. Real-machine run (maintainer): `ZEXIY`, `ZEXUND`, then `ZEXED` from a regenerated
   `cpmva-zexall-test.d88` on a PC-88VA2, V3 mode through BRKEM as in M101.
6. After the real outputs arrive:
   - commit them under `docs/agents/reports/m102_zexund_qa/` in the M101
     layout;
   - if every group matches the uPD9002 host prediction, record the results
     and close O3;
   - otherwise derive rules only from the evidence, add them to the
     `Upd9002` profile with rule ids from R12 on, extend the upstream pull
     request, and require byte-identical host reproduction of both output
     files. If the evidence does not determine a rule, stop and report.
7. Update `docs/modernization/uPD9002-zex-results.md` (§6 C3, §7 O3, new
   results section).

## Machine checks

- Repository checks, `linux-ci-gcc` build and `ctest`.
- `host_reference.py` with the new programs (Zilog and uPD9002).
- `check_zex_archive.py --mode source` with the new patches approved.

## Gate

G102 is a human gate:

1. The standard gate with a fully static MinGW build of the candidate.
2. The real-machine run of step 5 and the resulting step 6 outcome.

### G102 progress

- 2026-09-30, emulator run (supplementary): the maintainer ran `ZEXIY`,
  `ZEXUND` and `ZEXED` in vaeg from the generated `cpmva-zexall-test.d88`
  (disk after the run:
  `144e0a8f957359f7908a2b3638cd84cad8a08607ec8e7171eef1e1dae6e0ee62`).
  All three output files are byte-identical to the host uPD9002 reference
  of the same programs, and the emulator no longer stops on the undefined
  ED opcodes, the NEG duplicates or the redundant prefixes.
- Real-machine run (item 2) pending.
