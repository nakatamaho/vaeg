# M74o dispatch mechanism

## Identity

Branch: `topic/m74-va1-basic-command-hang`  
Starting SHA: `a39110b36bf9924c9d33c8a3c88d6b6c2b6d21fa`  
Ending commit is recorded below. Runtime used explicit `--model va`, trace-enabled Release build, deterministic guest-frame bounds, and `pcengine105-bootonly.d88`. Worker after the M74o seam extension: `35ebbc5e9e2a7819ae8fb5ea4f047306204318cd1ba9f63a99f088d34ce0eecd`.

Verified VA ROM identity used for static bytes: `varom00.rom`, SHA-256 `656d81bc532ed0bb602d1eb7f03df27c74f841fb19c72ad07d47e79302ac814b`. The ROM payload remains outside Git.

## Superseded M74n interpretation

The M74n statement that `E000:34C0` counter zero was contradictory is superseded. The word `34C0` is pushed by the near `CALL` but, on non-escape paths, remains below `002A` and `0005` until `01E4 RETF` consumes it as `CS`. `E000:34C0` executes only on the balanced ordinary-return path.

## Verified ROM instruction listing

The verified VA ROM bytes establish the relevant tail:

```text
34BD CALL 391D       ; near return IP = 34C0
3922 PUSH DX
3923 PUSH SI
3948/394C/3950/3954/3958 escape checks -> 3985
395C POP BX
395D PUSH BX
395E PUSH CX
3973 PUSH SI
3976 CALL 383A
3979 PUSH DS
397A JC 3984
397C POP CX
397D POP BX
397E POP AX
397F CLC
3980 MOV AX,0081
3983 RET
3984 POP AX
3985 POP SI
3986 POP DX
3987 STC
3988 RET
```

The bytes and stack effects are architectural facts. This report does not assign higher-level semantic names to the routine.

## Common 34BD/391D wrapper stack

Top of stack first:

```text
CALL 34BD -> [34C0]
PUSH DX   -> [0005,34C0]
PUSH SI   -> [002A,0005,34C0]
```

## Case A — escape terminator

For `A%=1` and other measured explicit escape forms, the tail restores the wrapper frame and `3988 RET` consumes `34C0`, returning to `E000:34C0`. This does not by itself guarantee whole-command completion, but current escape controls complete under their admissible runs.

## Case B — non-escape positive

For `PRINT 1`, `3973`/`3979` add the classifier frame. The positive path reaches `3984`/`3985`; `3986 POP DX` consumes the saved `CX`, leaving `002A` above `0005,34C0`. `3988 RET` therefore consumes `002A` and enters `E000:002A`.

## Case C — non-escape all-negative

For `A=1`, the negative epilogue pops `CX`, `BX`, and `AX`, leaving `002A,0005,34C0`; `3983 RET` consumes `002A` and enters `E000:002A`.

## Stack-equivalence proof for B and C

Both non-escape epilogues present the same pre-dispatch words:

```text
[002A, 0005, 34C0]
```

They differ only in the RET instruction: `PRINT 1` uses `3988`, while `A=1` uses `3983`. Both leave `[0005,34C0]` for `002A -> 0180 -> 01E4`.

## Correct meaning of E000:34C0 counter

For Case A, `34C0` is consumed as near IP and the instruction executes. For Cases B/C, it is preserved as the later far-return segment and `E000:34C0` need not execute. The prior `34C0=0` observation is therefore expected on non-escape dispatch paths.

## M74o-A three-run confirmation

| command | 34BD | 391D | 3983 | 3985 | 3988 | 34C0 | 002A | 0180 | 01E4 | result |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|
| `A%=1` | not admissible in this pass | | | | | | | | | external run preemption |
| `PRINT 1` | 1 | 1 | 0 | 1 | 1 | 0 | 1 | 1 | 1 | admissible short-bound capture |
| `A=1` | prior admissible A0 evidence | 1 | 1 | 0 | 0 | 0 | 1 | 1 | 1 | prior A0 terminal capture; new run externally preempted |

`PRINT 1` one-shot immediately before `3988` captured:

```text
SS:SP = 7FE0:01F2
words = 002A,0005,34C0
```

Its `01E4` capture was `34C0:0005` with 16 zero bytes. The current worker log SHA is `53a5b017ab2b62f3d155f00e074ad71bf6582472442db839832f6f8f4db88e69`.

## Wrapper accounting invariants

For the statically bounded `34BD -> 391D` wrapper, `3983 + 3988 = 391D` is the applicable per-invocation accounting relation. It is not promoted as a command-global invariant when a command has multiple invocations or other producers.

## Current-worker corpus

The complete requested corpus was not admissibly completed within the external execution window. The only new complete current-worker row is `PRINT 1`, above. Existing rows from prior worker identities are not merged. No counterexample to the static wrapper rule was observed in the new row.

## Scope of the escape/non-escape rule

The rule is proven only for the measured `E000:34BD -> 391D` wrapper family. Commands not shown to invoke `34BD` are outside scope.

## Continuation target ownership question

The static stack derivation establishes guest ownership of the *frame construction*: the guest ROM deliberately leaves `[0005,34C0]` for `01E4 RETF`. It does not establish whether the target `34C0:0005` should contain code, RAM, an overlay, or another resource.

## Snapshot-seam equivalence

Not performed. No new snapshot seam was added in this pass. Therefore no snapshot-derived claim is admitted.

## Five continuation-segment snapshots

Deferred. Exact blocker: the separate side-effect-free snapshot seam and its S1/S2 equivalence gate were not implemented; no bytes are claimed for `34C0`, `43B5`, `49FC`, `75AB`, or `7F2D` in this report.

## PC-88VA 0x34C00 hardware mapping

No authoritative PC-88VA mapping rule plus terminal mapper-register capture was completed in this pass. VAEG observed target `34C0:0005` as zero-filled ordinary mapped memory in prior evidence. Hardware-vs-VAEG comparison remains unresolved; no mapper defect is asserted.

## Terminal mapper/bank state

Not captured at `01E4` in M74o. This is the exact missing state required before declaring a mapping mismatch.

## E4 installer decode

BLOCKED. The source-record format/list around `19E3:C7EB` and `1CC5:C6BB` was not completely decoded in this pass. The exact blocker is incomplete record-format/conditional-list decoding.

## Tracked headless reproducer

Added `tools/m74-diagnostics/run_basic_case.sh`. It accepts worker, ROM root, disk, command, frame bound, script path, and output path; emits worker/disk hashes and run identity; uses explicit VA model and deterministic frame bounds; and keeps payloads external. Its source is in this commit. Future rows should use its script SHA and identity output.

## Validation restoration

- `cmake --build build/linux-release --target vaeg -j2`: PASS.
- `git diff --check`: PASS.
- `check_encoding.py` with `GIT_CONFIG_GLOBAL=/dev/null GIT_CONFIG_SYSTEM=/dev/null`: PASS in prior M74o preparation.
- `check_eol.py`, `check_case.py`: not run in this pass; process-local Git isolation was not re-run after the source/report edits.
- Selftest: NOT RUN.
- ROM-less suite: NOT RUN.
- M68/M69/M70: NOT RUN.
- MinGW/cross-build: NOT RUN.
- Hosted CI: NOT RUN.

No persistent Git configuration was modified. External wall-clock preemption affected the incomplete `A%=1` and `A=1` reruns; those partial runs are not guest verdicts.

## Hypothesis table

- H1, `3983` is the failure boundary: **REJECTED**; `PRINT 1` uses `3988` and reaches the same service.
- H2, `3985` alone implies caller return: **REJECTED AS GENERAL STATEMENT**; `3988` consumes the current top stack word.
- H3, non-escape `34C0=0` is contradictory: **REJECTED**.
- H4, positive and negative non-escape paths share the continuation frame: **PROVEN STATICALLY; DYNAMICALLY CONFIRMED for PRINT 1 and existing A=1 stack evidence**.
- H5, escape versus non-escape selects ordinary return versus continuation dispatch in this wrapper: **STRONGLY SUPPORTED; full current-worker falsification corpus incomplete**.
- H6, target ownership/mapping/initialization is the remaining causal boundary: **PRIMARY UNRESOLVED BOUNDARY**.
- H7, VAEG maps `0x34C00` incorrectly: **UNRESOLVED**.
- H8, continuation infrastructure was never installed: **UNRESOLVED**.

## First incorrect emulator-produced state

None proven. The stack frame is guest-produced and the CPU consumes it according to the established RET/RETF semantics. The target contract and hardware mapping state remain unproven.

## Production fix

None.

## Remaining gaps

Current-worker `A%=1`/`A=1` captures, full corpus, snapshot equivalence and bytes, authoritative terminal mapping/register state, E4 decode, and full validation remain open. Each is named rather than collapsed into a generic unknown.

## Changed files

- `cpu/upd9002/upd9002_trace.c`
- `tools/m74-diagnostics/run_basic_case.sh`
- `docs/agents/reports/m74o_dispatch_mechanism.md`

## Worktree status

Pre-existing untracked files remain `cpu/upd9002/upd9002_trace.c.orig`, `cpu/upd9002/upd9002_trace.c.rej.orig`, and `tools/__pycache__/`. No ROM, disk, generated worker, raw log, or private asset was staged.

## Hosted CI status

NOT RUN.

## G74 status

NOT APPROVED.
