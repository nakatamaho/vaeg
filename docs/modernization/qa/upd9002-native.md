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

# QA of the uPD9002 native mode

The uPD9002 native mode runs a V30-family instruction set (`cpu/upd9002/`)
with NEC changes: four V30 instructions are deleted, `BRKEM`/`BRKFEM` enter
the uPD70008-compatible mode, and V50-style on-chip peripherals sit in the
system I/O area (`docs/modernization/upd9002-upd70008-mode.md` §1, §4.1–4.2,
§5). No uPD9002 single-instruction corpus exists, so the instruction-level
oracle is the closest available reference CPU, the NEC V20 (uPD70108). See
[the QA overview](README.md) for the three levels and the oracle order.

## 1. What the V20 corpus can and cannot be

SingleStepTests V20 `v1_native` (pinned in
`tests/ssts/v20_dataset_manifest.json`) records, per instruction, the CPU
state before and after one instruction on a V20. It is a level-3 oracle:

- where the uPD9002 and the V20 share an instruction, agreement is strong
  evidence;
- where the uPD9002 differs, the V20 result is wrong for vaeg and must not be
  counted as a pass or a failure;
- where the uPD9002 has something the V20 lacks, the corpus is silent.

The target decisions below come from the ROM-read monitor tables (M60b),
the semantics migration plan (`docs/agents/UPD9002_SEMANTICS_MIGRATION.md`)
and the G67 divergence registry, which is kept at commit
`884b7dca97b6576007baa235b81a2597fa5f0c22`
(`tests/ssts/divergence/g67/registry.json`).

## 2. The external check

`vaeg_sst_v20_ci` (job `upd9002-ssts`, tool `tools/qa/sst_v20.py`, baseline
`tests/ssts/v20/baseline_ci.json`):

| Item | Value |
|---|---|
| Selection | records with an empty initial prefetch queue, ascending upstream hash, first 500 per form (`ci` profile) |
| Executed by | `vaeg --upd9002-ssts-worker`, the production instruction core with a flat test memory and an I/O sink |
| Compared | all registers; FLAGS under the corpus flags mask; every RAM byte the record names; I/O reads and writes; termination (normal, halt, type-0 exception) |
| Judged | 170500 records in 341 forms, all passing |
| `target_absent` (not judged) | 4500 records: `0F31`, `0F33`, `0F39`, `0F3B` (INS/EXT), `6C`–`6F` (INM/OUTM), `63` |
| `excluded` (not judged) | 5000 records: `66`, `67`, `D8`–`DF` (upstream status `fpu`) |
| Failure rule | a judged record that fails is a regression (`SST_V20_REGRESSION`); there is no tolerated failure |

The full profile (all 1,562,502 empty-queue records) is supported by the tool
but is not run in CI and has no committed baseline.

## 3. What the V20 check covers

Every judged form is an instruction the uPD9002 is documented to have, with
the V20 as oracle:

- the 8086/80186 base set present on the V30, including the V30
  `REPC`/`REPNC` (`64`/`65`) string forms that M70 implemented with explicit
  authority (`A4`–`AF` with those prefixes);
- the V30 extensions present in the uPD9002 monitor table: `TEST1`/`NOT1`/
  `CLR1`/`SET1` (`0F10`–`0F1F`), `ADD4S`/`SUB4S`/`CMP4S` (`0F20`/`22`/`26`),
  `ROL4`/`ROR4` (`0F28`/`2A`);
- segment, `LOCK` and repeat prefixes inside those records;
- software interrupts, `INTO`/`BOUND`/divide exceptions and `HLT` as far as one
  instruction's termination shows them.

## 4. What the V20 check cannot see

| Area | Reason | Covered elsewhere |
|---|---|---|
| Undefined FLAGS bits | the corpus mask excludes them; an all-16-bit comparison is a diagnostic only (G67 `fingerprint`) | nowhere against hardware |
| Prefetch queue effects | half the corpus (1,562,498 records) starts with a non-empty queue; the worker cannot inject a queue | nowhere |
| Cycle counts, bus timing, wait states | not compared by design | nowhere |
| `0F31/33/39/3B`, `6C`–`6F`, `63` on the real uPD9002 | target-absent: the uPD9002 behaviour of these encodings is not recorded; vaeg still contains V20 `INS`/`OUTS` handlers for `6C`–`6F`, an open cleanup question (G67 `reserved_behavior.primary_6c_6f_cleanup`) | open |
| `66`/`67` (FPO2) and `D8`–`DF` (ESC) | upstream `fpu`; whether the uPD9002 silicon implements FPO2 is underdetermined (G67) | 8087 core: `vaeg_8087_core` (unit tests in `tools/8087/test_core.c`), not instruction-level hardware evidence |
| `BRKEM` (`0F FF imm8`), `BRKFEM`/`BRKEM2` (`0F FE imm8`) | no corpus records; BRKFEM semantics are not established (G67 zero-coverage records) | `vaeg_upd9002_brkem_upd70008` (synthetic) |
| `RETEM`, `CALLN` and the MD flag | compatible-mode side | see [z80-family.md](z80-family.md) |
| Prefix and restart behaviour after an interrupt or exception mid-instruction | one-instruction records cannot show it; hardware question in G67 | open |
| Hardware interrupts, NMI, the interrupt controller, DMA, timers, refresh | V50-style peripherals, not instruction semantics | local levels; `docs/modernization/upd9002-upd70008-mode.md` §5 |
| Memory mapping beyond the flat test memory (banks, VA memory control, segmented word access across mapped regions) | the worker uses flat memory | `vaeg_upd9002_m68_segmented_memory` (synthetic) |
| I/O trap subsystem | machine feature (§9 of the mode document) | local level A |
| Records past the first 500 per form | `ci` profile only | full profile, run locally on demand |

## 5. Native checks inside vaeg

These run `vaeg` entry points with expectations written from the documented
semantics, ROM evidence, and the milestone records. They are synthetic
(level 2 or 4), not hardware measurements.

| CTest | Content |
|---|---|
| `vaeg_upd9002_flags_materialization` | FLAGS canonicalisation and guest-visible materialisation (M60a) |
| `vaeg_upd9002_iret_restoration` | IRET restoration (M60e) |
| `vaeg_upd9002_mov_imm_register` | C6/C7 register-form MOV immediate (M61) |
| `vaeg_upd9002_semantics_bundle` | consolidated semantic corrections (M62) |
| `vaeg_upd9002_m64_semantics` | DIV/IDIV and monitor-authorised `0F` support (M64) |
| `vaeg_upd9002_m65a_ff7` … `m65e_tail10` | FF /7, BOUND, F7 /2 NOT, FF /6 and ten residual cases (M65a–M65e) |
| `vaeg_upd9002_m68_segmented_memory` | mapped-memory dispatch for segmented word access (M68) |
| `vaeg_upd9002_m70_prefix_string` | REPC/REPNC with string instructions (M70) |
| `vaeg_upd9002_m96d_nop` | NOP handling (M96d) |
| `vaeg_upd9002_rep0f_diagnostic_stop` | the REP + `0F` diagnostic stop |
| `vaeg_upd9002_dispatch_normalization`, `_static`; `vaeg_upd9002_native_invariant`; `vaeg_upd9002_rep0f_transition_*`; `vaeg_upd9002_protected_deletion`; `vaeg_upd9002_rename` | dispatch-table structure and retired-code checks |
| `vaeg_upd9002_abi`, `_state_boundary`, `_state_payload_probe`, `_m66a_state_verify`, `_trace_equivalence` | state format, save/load boundary, deterministic traces |
| `vaeg_upd9002_ssts_worker_selftest` | the SST worker protocol itself |

## 6. Codes and baseline update

| Code | Meaning | Action |
|---|---|---|
| `SST_V20_REGRESSION` | a judged record that passes in the baseline fails now | fix the emulator |
| `SST_V20_BASELINE_STALE` | a record listed as failing now passes | run `update` and commit the baseline |
| `SST_V20_IDENTITY_MISMATCH` | dataset, profile, or selection rule differs | regenerate the baseline deliberately |
| `SST_V20_FORM_SET_MISMATCH` | a form is missing on one side | as above |
| `SST_V20_SELECTION_MISMATCH` | a form's selected count or status (`executed`, `excluded`, `target_absent`) differs | as above |
| `SST_V20_BASELINE_SCHEMA` | a document is malformed | fix the file |

```sh
python3 tools/qa/sst_v20.py ci --dataset-root "$V20" --worker build/linux-ci-gcc/sdl2/vaeg \
  --baseline tests/ssts/v20/baseline_ci.json --output /tmp/sst-v20-ci.json
python3 tools/qa/sst_v20.py update --result /tmp/sst-v20-ci.json \
  --baseline tests/ssts/v20/baseline_ci.json
```

The result file lists, per failing record, the outcome, mismatch kinds and
instruction bytes; it is a diagnostic and is not committed.

## 7. Reading a result

- `vaeg_sst_v20_ci` passing means: on the selected records of every form the
  uPD9002 is documented to share with the V20, the emulated native core
  reproduces the V20's architectural result. It is not hardware validation of
  the uPD9002.
- A change in a `target_absent` form is invisible to this check by design;
  deciding uPD9002 behaviour there needs real-machine evidence.
- Regenerate the baseline only for an intended change (`sst_v20.py update`),
  and only after explaining the change in the commit.
