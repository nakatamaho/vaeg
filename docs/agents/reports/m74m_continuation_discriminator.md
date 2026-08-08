# M74m continuation discriminator

## Identity and evidence hashes

Branch: `topic/m74-va1-basic-command-hang`  
Starting SHA: `0a5d270d9a3734e19eb644277a1eb98318aa7d0d`  
Runtime: explicit `--model va`, trace-enabled build, deterministic guest-frame bound, `pcengine105-bootonly.d88`. Runtime work used the boot-only image and ROM set already used by M74l; private payloads remain outside Git. Worker: `bb664d9c0787e25c73d9a9b452123644dc60c2d01fde35f108ba48ee66aabda6` (Release build). Source was the working tree at the starting SHA plus the diagnostic-only fixed-address counters in this commit. Historical malformed M74l worker value is not reused.

## Superseded model

`3983` is rejected as the failure boundary: `PRINT 1` reached `3985` and still reached `002A`/`01E4`. `3985` is rejected as implying command completion. The local `391D` classifier interpretation remains: positive aggregate reaches the `3985` side and all-negative aggregate reaches the `3983` side. That local result does not classify whole-command completion.

## Current measured discriminator

| command | 3983 | 3985 | 002A | 0180 | 01E4 | outcome |
|---|---:|---:|---:|---:|---:|---|
| `A=1` | 1 | 0 | 1 | 1 | 1 | terminal continuation, target `34C0:0005` |
| `PRINT 1` | 0 | 1 | 1 | 1 | 1 | no second prompt by frame 1260 |
| `LET A=1` | 0 | 1 | 1 | 1 | 1 | no second prompt by frame 1260 |

These measurements support, but do not prove universally, `002A` as the strongest current discriminator. Entry into `002A` itself is not proven defective.

## PRINT 1 command-local RETF capture

`PRINT 1` was armed at command 3 after first prompt at frame 720 and injection at frame 840. The one-shot `01E4` capture reported `RETF IP=0005`, `CS=34C0`, target `34C0:0005`, and 16 zero bytes. This is the same target as `A=1`. The run was bounded at frame 1300/1260 prompt wait and is not by itself a permanent-hang proof; the longer-bound confirmation remains open.

## LET A=1 and DEFINT A-Z

`LET A=1` produced the same fixed counters and the same `34C0:0005` zero target. `DEFINT A-Z` has an earlier M74l classifier result (`391D=1, 3976=1, 3823=1, 3831=1, 3985=1`) but a fresh M74m command-local RETF result was not obtained: the diagnostic run was externally terminated before the first prompt/command window. It is therefore not promoted as new M74m evidence.

The stateful `DEFINT A-Z` then `A=1` test was not completed.

## PRINT 1 route into 002A

The fixed-address producer census for `PRINT 1` was:

| address | count |
|---|---:|
| `34BD` | 1 |
| `43B2` | 0 |
| `49F9` | 0 |
| `75A8` | 0 |
| `7F2A` | 0 |
| `0021,0024,0027,002D,0030,0033,0036` | 0 each |
| `002A` | 1 |
| `0180` | 1 |

Thus the measured `PRINT 1` route used `34BD` and reached `002A` directly in the armed window. No sibling `39AD` counter was added, and no alternate producer is inferred. `002A == 0180 == 01E4` for this run, supporting one complete service traversal.

## Full corpus and counterexamples

The full M74m corpus rerun was not completed. The existing M74k/M74l rows remain historical evidence, not silently merged with this worker identity. No counterexample to the currently supported `002A` discriminator was found in the new rows, but universality is unproven.

## Positive lookup captures

No new register capture was added. D3 tier 1 remains the existing control-flow deduction for `A=1`; D3 tiers 2 and 3 remain dependent on the handler ABI/name/selector capture.

## Evidence identity

New log hashes:

- `PRINT 1`: `ebde9b9a9add0ca4f1ab1609d0a4fc38d548b36146db6e165747d6cef774c048`
- `A=1`: `b05ae9075bbee04a9886ca529c33ba56d07cb7237e0ce30c8402e5b4b8478ad9`
- `LET A=1`: `eb6bd1b15cf12c4daec8403d9ec13c65fdd8bc04dadd05d600e2b35e960038e1`
- `DEFINT A-Z` incomplete run: `480668abd1a1447210cf59ff45083d303c9245d4a681eb8614586a507eff21c0`

The exact script used for the completed runs was the five-line script `BASIC`, `@prompt`, command, `@prompt`, `@exit`; its raw temporary artifact is outside Git. A committed script/config identity is still required for future reproducibility.

## Five continuation-segment snapshot status

Deferred. No bounded memory-snapshot seam was added in M74m, so no snapshot evidence is admissible. The exact blocker is runtime budget and the absence of an already equivalence-gated snapshot artifact, not a claim that the task is redundant.

## 0x34C00 hardware mapping status

The observed VAEG target is ordinary mapped address `34C05h` with zero fetched bytes. The authoritative VA1 hardware mapping and terminal bank/register state were not measured here; no mapping defect is asserted.

## E4 status

The installer source-record lists for `19E3:C7EB` and `1CC5:C6BB` were not completely decoded in this pass. E4 is specifically blocked by incomplete source-record format/control-flow decoding.

## Hypothesis table

- H-3983: **REJECTED** as the whole-command failure boundary; `PRINT 1` is the counterexample.
- H-3985: **REJECTED** as a completion guarantee; `PRINT 1` reaches it without a prompt in the measured bound.
- H-002A-discriminator: **SUPPORTED BY CURRENT MEASURED SET**, not universal proof.
- H-continuation-cause: **UNPROVEN**.
- H-shared-frame-protocol: **SUPPORTED for measured `PRINT 1` and `A=1`**, but broader generalization is unproven.

## First incorrect emulator-produced state

None proven. The observed zero target and frame are guest-visible evidence, but the expected hardware mapping/initialization and terminal mapper state are not established.

## Production fix

None. No production source correction was made.

## Validation

- Release trace-enabled build: PASS; worker SHA above.
- `git diff --check`: PASS.
- `check_encoding.py`: PASS.
- `check_eol.py` and `check_case.py`: not run successfully in this pass because their Git subprocess inherited the sandboxed global Git configuration; this is a tooling-environment blocker, not a source verdict.
- Selftest, ROM-less tests, M68/M69/M70 protections: not run in this pass.
- Hosted CI: NOT RUN.

## Worktree status

Committed changes are the diagnostic-only fixed-address counters and this report. Pre-existing untracked files remain: `cpu/upd9002/upd9002_trace.c.orig`, `cpu/upd9002/upd9002_trace.c.rej.orig`, and `tools/__pycache__/`. No ROM, disk, raw trace, generated binary, or private asset was staged.

## G74 status

NOT APPROVED.
