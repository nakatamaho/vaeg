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

# CPU conformance checks

Three independent checks cover the three CPU roles. Each role has its own
expectation data, so a deliberate change in one role never moves the
expectations of another.

| Role | Check | Oracle | Expectation data | CTest |
|---|---|---|---|---|
| uPD9002 native mode | SingleStepTests V20 `v1_native`, one instruction per record | V20 hardware captures (upstream corpus) | `tests/ssts/v20/baseline_ci.json` | `vaeg_sst_v20_ci` |
| uPD9002 uPD70008-compatible mode (BRKEM) | ZEXDOC and ZEXALL | real PC-88VA2 group CRCs | `tests/z80_compat/upd9002_zex{doc,all}_expected.txt` | `vaeg_upd70008_zexdoc`, `vaeg_upd70008_zexall` |
| | M101/M102 probe programs | real PC-88VA2 output files | `tests/z80_compat/upd70008_probes.json` | `vaeg_upd70008_probes` |
| FDC subsystem uPD780C | ZEXDOC and ZEXALL | stock Zilog Z80 CRCs carried by ZEX | `tests/z80_compat/upd780_zex{doc,all}_expected.txt` | `vaeg_upd780_zexdoc`, `vaeg_upd780_zexall` |
| | ZEX13S, ZEXIY, ZEXUND, ZEXED | stock Zilog Z80 CRCs | `tests/z80_compat/upd780_probes.json` | `vaeg_upd780_probes` |

All three are ROM-less and run in hosted CI: the V20 check in the
`upd9002-ssts` job, the ZEX and probe role checks in the `compat-conformance`
job, which installs z80asm 1.8 for the probe programs. Without z80asm the
probe checks report a skip.

## uPD9002 native mode: SST V20

`tools/qa/sst_v20.py` selects, per opcode form, the records with an empty
initial prefetch queue in ascending upstream-hash order (the `ci` profile
keeps the first 500 per form; `full` keeps all). Forms whose upstream
metadata status is `fpu` or `undefined` are listed as excluded and not
executed. Every other selected record is executed by
`vaeg --upd9002-ssts-worker`; it passes when registers, FLAGS under the
upstream flags mask, watched RAM, I/O, and termination match.

The baseline records, per form, the upstream hashes of the records that fail
today. It holds no commit, milestone, or history identity; the only identity
it carries is the corpus `dataset_id`, the profile, and the selection rule.

`check` reports stable codes:

| Code | Meaning | Action |
|---|---|---|
| `SST_V20_REGRESSION` | a record that passes in the baseline fails now | fix the emulator |
| `SST_V20_BASELINE_STALE` | a record listed as failing now passes | run `update` and commit the baseline |
| `SST_V20_IDENTITY_MISMATCH` | dataset, profile, or selection rule differs | regenerate the baseline deliberately |
| `SST_V20_FORM_SET_MISMATCH` | a form is missing on one side | as above |
| `SST_V20_SELECTION_MISMATCH` | a form's selected count or status differs | as above |
| `SST_V20_BASELINE_SCHEMA` | a document is malformed | fix the file |

Local use, with the corpus acquired and verified as described in
`tests/ssts/README.md`:

```sh
python3 tools/qa/sst_v20.py ci --dataset-root "$V20" --worker build/linux-ci-gcc/sdl2/vaeg \
  --baseline tests/ssts/v20/baseline_ci.json --output /tmp/sst-v20-ci.json
# after an intended improvement:
python3 tools/qa/sst_v20.py update --result /tmp/sst-v20-ci.json \
  --baseline tests/ssts/v20/baseline_ci.json
```

The result file also lists, per failing record, the outcome, mismatch kinds,
and instruction bytes; it is a diagnostic and is not committed.

Baseline at creation (`ci` profile): 360 forms, 175000 executed records,
171883 pass, 3117 fail, 5000 excluded. The failures are in nine forms:
`0F31`, `0F33`, `0F39`, `0F3B`, and `63` (all 500 records each) and
`6C`-`6F` (INS/OUTS; 118, 121, 132, and 246 records). These are the forms the
previous G65m scoreboard classified as known target gaps and did not execute;
the new check therefore also guards the 1383 INS/OUTS records that pass.

## uPD70008-compatible mode and uPD780C: ZEX per role

`vaeg_z80_compat_role_zex_runner` runs ZEX on the shared backend built with
the production compile definitions. With `--role upd70008` it applies
`Upd9002Upd70008ConfigureCore` (`cpu/upd9002_upd70008_core_config.cpp`),
which sets the uPD9002 flag profile and the reader of the native-side page 0
(the x86 IVT) that the undefined ED 00-3F/74/75/77 read; with
`--role upd780` it applies `Upd780ConfigureCore`
(`cpu/upd780/upd780_core_config.cpp`). Production calls the same functions:
the uPD9002 adapter when it initializes its core, and the subsystem when it
creates its `UPD780C`. A future role-specific change therefore reaches the
role check without editing the test.

The runner compares every ZEX group with the role's expectation file
(`<group> OK` or `<group> ERROR <expected-crc> <found-crc>`). The uPD70008
file holds the real PC-88VA2 results recorded in
`docs/modernization/uPD9002-zex-results.md`. The uPD780C file expects every
group to match the stock Z80 CRC; no real uPD780C result exists yet, so this
records the configured Zilog behaviour, not verified hardware behaviour.

## Probe programs per role

`tools/qa/z80_role_probes.py` builds the M101/M102 CP/M programs from tracked
sources with z80asm 1.8 and runs them with `vaeg_cpm_runner --role <role>`,
which applies the same role configuration as production. Each manifest entry
names a program, one output file, and a judgement:

| Mode | Meaning |
|---|---|
| `exact` | the output equals a recorded file byte for byte |
| `prefix` | a recorded file that stopped early is a prefix of the output |
| `zex_all_ok` | a ZEX-style output completed with the given number of groups, all OK |

Stable codes: `Z80_ROLE_EXACT_MISMATCH`, `Z80_ROLE_PREFIX_MISMATCH`,
`Z80_ROLE_ZEX_GROUP`, `Z80_ROLE_OUTPUT_MISSING`, `Z80_ROLE_RUN_FAILED`,
`Z80_ROLE_MANIFEST`.

uPD70008-compatible mode, against real PC-88VA2 files from
`docs/agents/reports/m101_zexall_qa` and `m102_zexund_qa`:

| Program | Output | Judgement |
|---|---|---|
| `DAADUMP` | `DAA.BIN`, `CPL.BIN`, `SCF.BIN`, `CCF.BIN`, `DAADUMP.TXT` | exact |
| `FLAGPRB` | `FLAGPRB.TXT` | exact |
| `ZEX13S` | `ZEX13S.TXT` | exact |
| `ZEXIY` | `ZEXIY.TXT` | exact |
| `CBPRB` | `CBPRB.TXT` (runs 2 and 3) | exact |
| `EDPRB2S` | `S200.TXT` (runs 4 and 5) | exact |
| `EDPRB3` | `EDPRB3.TXT` (runs 7 and 8) | exact |
| `EDPRB4` | `EDPRB4.TXT` (run 8; the 256-point IVT sweep) | exact |
| `EDPRB` | `ED00.TXT` (runs 2, 3, 5) | prefix |
| `EDPRB2` | `E200.TXT` (runs 4 and 5) | prefix |

The runner supplies the native-side page reconstructed from run 8, so the
undefined-ED results match the real machine. Not checked: the empty and
restarted per-opcode files of runs 3 and 5 (written after the unsandboxed
`EDPRB` had corrupted the program), `INPRB` (real-machine I/O), and
`ZEXUND`/`ZEXED`, whose real-machine results exist only as screen
photographs. `EDPRB` stops early on the host as it did on the real machine
and is marked `may_fail`.

FDC uPD780C: `ZEX13S` (10 groups), `ZEXIY` (8), `ZEXUND` (12), and `ZEXED`
(24) must complete with every group OK, that is, the stock Zilog CRCs.

Swapping the roles makes both checks fail, so the two manifests discriminate
the roles.

## Coverage limit

The role checks exercise instruction semantics through each role's core
configuration. Mode transitions (BRKEM, RETEM, CALLN), alternate-register
storage across the modes (ALTPRB), 8214-mode interrupts, and the CPU speed in
the compatible mode belong to the adapter. They are covered by the synthetic
`vaeg_upd9002_brkem_upd70008` and by local ROM-tier runs, not by these
checks.
