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

Status: **draft; awaiting maintainer approval**

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

## Scope

Two ZEX-derived CP/M programs, built like `ZEX13S` (file-output ZEX,
separate test table, `msbt` unchanged, descriptors copied from the stock
ones with only the named fields changed). Every group runs twice, with flag
mask D7h and FFh. Expected CRCs are those of the Zilog profile, computed on
the host.

### `ZEXIY.COM` → `ZEXIY.TXT` (O3)

| Group | Opcodes | Derived from | Change |
| --- | --- | --- | --- |
| `inc,dec ixh` | DD 24/25 | `incxh` | none |
| `inc,dec ixl` | DD 2C/2D | `incxl` | none |
| `inc,dec iyh` | FD 24/25 | `incyh` | prefix DDh → FDh |
| `inc,dec iyl` | FD 2C/2D | `incyl` | prefix DDh → FDh |

The IX groups are the reference for the IY groups under both masks.

### `ZEXUND.COM` → `ZEXUND.TXT` (other undocumented instructions)

| Group | Opcodes | Derived from | Change |
| --- | --- | --- | --- |
| `rot (ix,iy+d) -> r` | DD/FD CB d 00–3F | `rotxy` | fourth-byte counter 07h → all 64 opcodes |
| `bit n,(ix,iy+d) all` | DD/FD CB d 40–7F | `bitx` | fourth-byte counter covers all low bits |
| `res/set (ix,iy+d) -> r` | DD/FD CB d 80–FF | `srzx` | fourth-byte counter covers all 128 opcodes |
| `neg (all encodings)` | ED 44/4C/54/5C/64/6C/74/7C | `negop` | second-byte counter 38h |

The DDCB/FDCB groups include the register-copy forms (the result is also
written to B, C, D, E, H, L or A); the ZEX machine state covers those
registers.

### Excluded, with reasons

- `IN F,(C)`, `OUT (C),0` and other I/O instructions: they access real
  PC-88VA ports, which can have side effects and do not give repeatable
  input values.
- `ED ED` (CALLN) and `ED FD` (RETEM): µPD9002 mode-switch instructions.
- Other undefined ED opcodes and redundant DD/FD prefixes: not needed for
  O3, and an undefined opcode that traps would stop the run.
- `LD A,R`: R is not reproducible.

### Hang protection

An undocumented opcode that the µPD9002 does not implement could hang the
program before it closes its output file. The two programs are therefore
separate, `ZEXIY` is run first, and every result line is also shown on the
console so that a partial run can still be read from the screen.

## Work

1. Add `zexiy.patch` and `zexund.patch` (GPL-2.0-or-later, like
   `zex13s.patch`) and build `ZEXIY.COM` and `ZEXUND.COM` in `zexbuild.py`
   with the same `msbt` and prefix checks.
2. Compute the Zilog expected CRCs on the host and record them in the
   patches. Acceptance: under the Zilog profile both programs report OK for
   every group.
3. Record the uPD9002 host predictions. Extend `host_reference.py`,
   `compare.py` and the parsers with the two programs.
4. Add both programs to `cpmva-tools.d88` and `cpmva-zexall-test.d88`, and
   update `docs/cpmva-setup.md`.
5. Real-machine run (maintainer): `ZEXIY` then `ZEXUND` from a regenerated
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
