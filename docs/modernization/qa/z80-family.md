<!--
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
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

# QA of the Z80-compatible CPUs

Two hardware roles run the shared Z80 compatibility backend
(`cpu/z80_compat_cpu.*` over the vendored suzukiplan core). They are
different CPUs and are checked separately, so that a change for one role
cannot pass because the other role's expectations still hold.

| Role | Hardware | Core configuration | Behaviour today |
|---|---|---|---|
| uPD70008-compatible mode | the uPD9002 main CPU after `BRKEM` (V1/V2 mode, CP/MVA) | `Upd9002Upd70008ConfigureCore` (`cpu/upd9002_upd70008_core_config.cpp`) | `kUpd9002` flag profile, measured undefined-opcode rules, IVT reader for the undefined `ED` loads |
| FDC subsystem CPU | the NEC uPD780C running `vasubsys.rom` | `Upd780ConfigureCore` (`cpu/upd780/upd780_core_config.cpp`) | `kZilog` profile |

Production and the role checks call the same configuration functions. See
[the QA overview](README.md) for the three levels and the oracle order.

## 1. Oracles

| Role | Highest available oracle | Source |
|---|---|---|
| uPD70008-compatible mode | real PC-88VA2 outputs (level 1) | M101 ZEXDOC/ZEXALL and probes (`docs/agents/reports/m101_zexall_qa/pc88va2/`), M102 probes (`docs/agents/reports/m102_zexund_qa/run*_pc88va2/`) |
| uPD780C | none on real hardware; the Zilog Z80 (level 3) | ZEX's stock CRCs |

No real uPD780C has been measured. The FDC CPU's checks therefore prove
Zilog conformance of the configured backend, not uPD780C conformance. NEC
NMOS Z80 parts are reported to differ from Zilog in the undocumented X/Y
bits of `SCF`/`CCF`; whether the uPD780C does is unverified
(`cpu/z80_compat_cpu.h`).

## 2. External checks

| CTest | Role | Runs | Oracle | Judgement |
|---|---|---|---|---|
| `vaeg_upd70008_zexdoc`, `vaeg_upd70008_zexall` | uPD70008 | stock ZEXDOC/ZEXALL, 67 groups each | real VA2 group CRCs (`tests/z80_compat/upd9002_zex*_expected.txt`) | every group equal |
| `vaeg_upd70008_probes` | uPD70008 | M101/M102 probe programs | real VA2 output files | byte-exact, prefix, or per-record (below) |
| `vaeg_upd780_zexdoc`, `vaeg_upd780_zexall` | uPD780C | stock ZEXDOC/ZEXALL | stock Zilog CRCs | every group OK |
| `vaeg_upd780_probes` | uPD780C | ZEX13S, ZEXIY, ZEXUND, ZEXED | stock Zilog CRCs | every group OK |
| `vaeg_upd9002_brkem_upd70008` | uPD70008 adapter | synthetic: production bridge, alternate-register storage, compatible-mode CPU speed | expectations written from documentation and M103a records | fixed cases |
| `vaeg_romless_tests` (`vaeg --selftest`, uPD780 part) | uPD780C subsystem | synthetic: state bridge, clock and wait, HALT/IRQ state, IRQ acknowledge, sleep paths, FDD boundary, EI state, cross-wired 8255 ports | expectations written in `tests/upd780/subsystem_integration.cpp` | fixed cases |
| `vaeg_z80_compat_*` | shared backend | wrapper contracts, legacy contract, state format, differential traces against the bare core, bare-core ZEX | the bare core and the backend's own earlier output (level 4) | consistency only |

The role checks run in the `compat-conformance` job. The probe programs are
built with z80asm 1.8; without it the probe checks report a skip.

## 3. uPD70008-compatible mode: where it departs from Zilog, and what checks it

The mode is a Z80 superset (μPD780 instruction set plus `CALLN`/`RETEM`),
not an 8080 mode (`docs/modernization/upd9002-upd70008-mode.md` §2.4), but
its flag computation and undefined-opcode space differ from a Zilog Z80.
Rules R1–R19 are defined in `docs/modernization/uPD9002-zex-results.md` §4.

| Rule | Behaviour (summary) | Checked against real VA2 by |
|---|---|---|
| R1 | F bits 5/3 always read 0 and cannot be stored | ZEXALL CRCs; FLAGPRB P7 (exact) |
| R2–R3 | AND and BIT clear H | ZEXALL CRCs; FLAGPRB P1/P2 |
| R4 | RLCA/RRCA/RLA/RRA change only A and C | ZEXALL CRCs; FLAGPRB P3 |
| R5 | LDI/LDD block flags | ZEXALL CRCs; FLAGPRB P4 |
| R6–R7 | 16-bit ADD/ADC/SBC H rules | ZEXALL CRCs (ZEXDOC masks H in groups 1–4); FLAGPRB P5/P6 |
| R8–R11 | CPL/SCF/CCF/DAA | DAA/CPL/SCF/CCF exhaustive dumps (4 × 16384 records, exact), DAADUMP, ZEX13S |
| R12–R13 | `DD CB`/`FD CB` with a register operand: BIT tests the register, RES/SET modify only the register | CBPRB, all 1024 encodings × 2 input sets (runs 2 and 3, exact) |
| R14 | undefined `ED 00`–`3F`, `74`/`75`/`77` load a word from the native page 0 | EDPRB3 (one variable at a time), EDPRB4 (all 256 `C` values), ED00/E200 prefixes, e264 records |
| R15 | `ED 4C` duplicates `RETN` | ED00/E200 prefixes |
| R16 | `ED 64`/`65`, `6C`/`6D` RRD/RLD also write `(HL+1)` | S200 (exact), e264 records |
| R17–R18 | `ED 7C`–`A7` single and `AD`–`AF`/`B4`–`B7` repeated word transfers | S200, e264 records |
| R19 | `ED BC`–`EC` consume one operand byte with no visible effect | e264 records |
| IY halves | `FD 24/25/2C/2D` behave like the IX halves | ZEXIY (exact) |

The R14 checks use the IVT page reconstructed from real run 8, which the CP/M
runner passes to the role configuration. In the emulator the same opcodes
read the guest's live page 0; the two agree only while the guest has not
hooked those vectors (run 10 differed in three RAM-hooked vectors).

### What ZEXDOC/ZEXALL alone cannot establish

- A group CRC covers only the state space ZEX exercises; a match says nothing
  outside it (`uPD9002-zex-results.md` C5).
- ZEXDOC masks hide H in groups 1–4 and S, P/V in the BIT groups (C1).
- Groups 29–31 are degenerate for Y/X (C2); F never enters with bits 5/3 set,
  so storage of those bits is not exercised (C4).
- The `iyh`/`iyl` groups execute `DD`-prefixed opcodes; the real IY halves
  are not exercised (C3).
- No undocumented `DD CB`/`FD CB` register forms and no undefined `ED`
  opcodes are exercised.

Each of these gaps is closed by a probe in the table above, against real
VA2 output.

## 4. uPD70008-compatible mode: not checked by the external level

| Area | Why not | Where it is covered, if anywhere |
|---|---|---|
| Undefined `ED 54`/`55`, `AC`, `EE`–`FC` (leave compatible mode), `5C` (hang), `5D` (fault) | only symptoms were observed; vaeg does not emulate them | nowhere |
| `ED` word transfer with `BC = 0` on entry | not measured; vaeg copies 65536 words like `LDIR` | nowhere |
| `IM` duplicates (`ED 4E/66/6E/76/7E`) and `LD A,R` | excluded from the probes by design | nowhere against hardware |
| `IN`/`OUT` family | the CP/M runner rejects I/O; INPRB is real-machine only | local level A only |
| Mode transitions `BRKEM`/`BRKEM2`, `CALLN`, `RETEM`; native servicing of interrupts taken in compatible mode; `RETI` re-entry | adapter and native side, not the backend | `vaeg_upd9002_brkem_upd70008` (synthetic); local level A |
| The `CALLN`/`RETI` carry question (`upd9002-upd70008-mode.md` §3.2) | unresolved on hardware | open |
| HALT/standby rules across modes (§3.5) | needs interrupts from the machine | local level A |
| Z80 interrupt modes `IM 0/1/2` in compatible mode | `[UNKNOWN]` on hardware (§3.5) | open |
| Segment bases (code/data translation) and the live IVT | adapter glue | `vaeg_upd9002_brkem_upd70008`; local level A |
| Cycle counts and bus timing | no hardware timing record | nowhere; `docs/modernization/z80-compat-cycle-deltas.md` records backend-versus-bare-core deltas only |
| ZEXUND and ZEXED complete results | the real runs did not close their files (screen photographs only) | open |

## 5. uPD780C: checked and not checked

Checked externally: Zilog conformance of the configured backend over ZEXDOC,
ZEXALL, ZEX13S, ZEXIY, ZEXUND and ZEXED (every group OK), and synthetic
subsystem integration through `vaeg --selftest`.

Not checked externally, and why:

- Any real uPD780C behaviour: no measurement exists. Planned evidence would
  run the same probe programs on the FDC CPU of a real VA, which needs a
  harness in `vasubsys.rom`'s environment.
- The FDC firmware itself (`vasubsys.rom`) and its traffic with the uPD765
  and the main CPU: needs the NEC ROM, so local level A only.
- Interrupt acceptance and timing on the real board, `HALT`/sleep paths
  beyond the synthetic cases, and cycle counts.

## 6. Reading a result

- `vaeg_upd70008_*` passing means: for every listed program and record, the
  emulated uPD70008-compatible mode reproduces the real PC-88VA2 output. It
  does not cover section 4.
- `vaeg_upd780_*` passing means: the FDC CPU behaves as a Zilog Z80 on the
  exercised state space. It does not mean the uPD780C was verified.
- Swapping the two role manifests makes both probe checks fail, so the
  checks do distinguish the roles.

Update procedure and file formats: `tools/qa/z80_role_probes.py` (manifests
`tests/z80_compat/upd70008_probes.json`, `upd780_probes.json`) and
`tests/z80_compat/*_zex*_expected.txt`.
