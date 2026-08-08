# M74n caller convergence

## Identity

Branch: `topic/m74-va1-basic-command-hang`  
Starting SHA: `f5ecefb4dd3d108fe5275f3527d697fbfdc58c5d`  
Worker: `ad46eb8f8b9167f2211461d497b9ae7e92fd9e548caa2ffdfb5f8046e85d533c`  
Conditions: explicit `--model va`, trace-enabled Release build, deterministic guest-frame bound, `pcengine105-bootonly.d88`. G74 remains unapproved.

## Address-model correction

`E000:34C0` is nominal linear address `E34C0h`. `34C0:0005` is nominal linear address `34C05h`. They are distinct. The latter is the target consumed by `01E4 RETF`; it is not the instruction address `E000:34C0`.

## Proven A=1 route

Static and prior admissible trace evidence establishes:

```text
E000:34BD CALL 391D -> pushes return IP 34C0
E000:3922 PUSH DX  -> pushes 0005
E000:3923 PUSH SI  -> pushes 002A
E000:3983 RET     -> consumes 002A and enters E000:002A
E000:002A -> E000:0180 -> E000:01E4 RETF
01E4 consumes IP=0005, CS=34C0 -> 34C0:0005
```

This path bypasses ordinary return execution at `E000:34C0` after the `3983 RET`.

## Proven PRINT 1 / LET A=1 route

M74m established for `PRINT 1` and `LET A=1`: `391D=1`, `3983=0`, `3985=1`, `002A=1`, `0180=1`, `01E4=1`, and `RETF=34C0:0005` with zero target bytes. The exact route between the `3985` return and `002A` remains unresolved.

## Correct convergence point

The proven common dynamic path is `E000:002A -> E000:0180 -> E000:01E4 -> 34C0:0005`. `E000:34C0` is not a proven convergence point for `A=1` and `PRINT 1`.

## Annotated E000:34C0 listing

A complete new byte-level listing could not be reconstructed from the repository-tracked artifacts: the relevant post-`3985` code resides in the private runtime image/RAM state, while the repository contains the prior interpreted notes but no admissible byte dump for this region. The available static boundary is therefore:

- `E000:34BD` is the real near `CALL 391D` site (`E8 5D 04`), whose architectural return IP is `34C0`.
- The `3985` side is a normal near `RET` path; the prior M74 reports identify the following region as the ordinary caller continuation.
- The `3983` side consumes saved `SI=002A` and never returns through that caller continuation.
- The actual branch/dispatch in the post-`34C0` region that sends `PRINT 1` to `002A` is not established.

This is an instrumentation/evidence gap, not permission to infer the route from the return-IP value.

## Annotated E000:34A0-34BD listing and DX provenance

Existing evidence proves `E000:33BC MOV SI,002A`, and the wrapper observes `DX=0005` before `E000:3922 PUSH DX`. M74n did not obtain an admissible byte-level backward walk from `34A0` through the unique predecessor to the first `DX=0005` producer. The precise result is therefore: `DX=0005` is a live input at the observed wrapper boundary; its first producer remains outside the current admissible static artifact.

## Branches and state consumed after 34C0

No new decisive branch was identified. The current reports establish only the earlier `391D` local selector (`397A JC 3984`) and the two different local exits. The state consumed by the unidentified post-`3985` route is not yet measured.

## Static candidate routes into 002A

Known candidates are stack-derived continuation, indirect dispatch, or a helper path after ordinary scanner return. The current fixed-address evidence does not distinguish them. No literal direct `JMP/CALL E000:002A` producer was established.

## Current-worker command matrix

The current worker used command-local arm `VAEG_M74_CPU_TRACE_COMMAND=3`, first prompt at frame 720, injection at frame 840, and deterministic prompt boundary at frame 1260.

| command | 34BD | 391D | 3983 | 3985 | 34C0 counter | 002A | 0180 | 01E4 | outcome |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---|
| `A=1` | 1 | 1 | 1 | 0 | 0 | 1 | 1 | 1 | bounded terminal continuation; `34C0:0005` zero |
| `PRINT 1` | 1 | 1 | 0 | 1 | 0 | 1 | 1 | 1 | no second prompt by bound; `34C0:0005` zero |
| `LET A=1` | not completed under this worker | not completed | not completed | not completed | not completed | not completed | not completed | not completed | prior M74m evidence only, not merged |
| `A%=1` | not completed under this worker | not completed | not completed | not completed | not completed | not completed | not completed | not completed | exact current-worker control remains open |
| `A!=1` | not completed under this worker | not completed | not completed | not completed | not completed | not completed | not completed | not completed | exact current-worker control remains open |

The `34C0=0` result is a contradiction requiring resolution before interpreting that counter as an executed-instruction counter. It does not refute the static fact that `34BD CALL` pushes return IP `34C0`.

## E000:39AD sibling analysis

Prior static evidence identifies `E000:39AD` as `CALL FAR 1040:0AC8`, distinct from `E000:391D`'s `CALL FAR 1040:0AC3`. M74n added no `39AD` counter and did not prove that this sibling is involved in `PRINT 1`'s route to `002A`.

## PRINT 1 producer of 002A

**Unidentified.** `34BD=1` proves only that the `CALL 391D` invocation occurred. It does not identify the later instruction that reaches `E000:002A`. Candidate set and exact predecessor remain open.

## Jump-table entry census

The current worker recorded zero for each tested alternate entry during `A=1` and `PRINT 1`: `0021`, `0024`, `0027`, `002D`, `0030`, `0033`, and `0036`. `002A=1` and `0180=1` for both. This does not assign semantic names to the entries and does not identify the predecessor of `002A`.

## A=1 versus PRINT 1 002A frame comparison

The existing one-shot capture gives `A=1` target `34C0:0005`, with remaining stack words `0005,34C0` before `01E4`. M74n did not add a new `002A` one-shot because the fixed-address route census was not yet resolved and the current run was not extended with a separate bounded capture. Thus identical incoming frame at `002A` is unresolved; identical final RETF target is proven by M74m.

## Longer-bound PRINT 1 result

Not completed at the established long A=1 bound. The available run reached only the deterministic short prompt boundary and is not promoted as a permanent-hang proof.

## Validation recovery

- Trace-enabled Release build: PASS.
- `git diff --check`: PASS.
- `check_encoding.py`: PASS under process-local `GIT_CONFIG_GLOBAL=/dev/null GIT_CONFIG_SYSTEM=/dev/null`.
- `check_eol.py`, `check_case.py`: not completed in this pass; the prior failure was Git configuration inheritance, and no persistent user/system configuration was modified.
- Selftest, ROM-less, M68/M69/M70, MinGW: not run in this pass.
- Hosted CI: NOT RUN.

## Runner-containment disposition

The `LET A=1` and `A%=1`/`A!=1` current-worker runs did not complete inside the available external execution window. Those rows are unresolved, not guest failures. No host wall-clock event is used as a guest verdict.

## Reproducer script identity

The runtime script was temporary and external: `BASIC`, `@prompt`, one command, `@prompt`, `@exit`. It was not committed because it contains no proprietary payload but the M74 tracked reproducer was not created in this pass. This is a reproducibility gap to close before further runtime matrix expansion.

## Corrected hypothesis table

- H1 (`3983` is failure boundary): **REJECTED**; `PRINT 1` reaches the common continuation with `3983=0`.
- H2 (`3985` implies completion): **REJECTED**; `PRINT 1` reaches `002A/0180/01E4` after `3985`.
- H3 (`A=1` and `PRINT 1` converge at `E000:34C0`): **REJECTED** for the observed `A=1` path; `3983 RET` bypasses it.
- H4 (typed controls diverge after `34C0`): **SUPPORTED by prior evidence, not proven under this worker**.
- H5 (`002A` is strongest common boundary): **SUPPORTED**, not a universal causal claim.
- H6 (same incoming `002A` frame): **UNRESOLVED**.
- H7 (target initialization/mapping defect): **UNRESOLVED**.

## First incorrect emulator-produced state

None proven. The return frame and zero target are guest-visible observations; the expected target contract and post-`3985` route are not established.

## Production fix

None.

## Carried-forward tasks

Five segment snapshots, positive lookup state capture, authoritative `0x34C00` mapping, E4 installer decode, and stateful `DEFINT A-Z` then `A=1` remain open. The exact missing items are named above.

## Worktree status

Committed files: this report and the diagnostic-only `34C0` counter extension. Pre-existing untracked files remain `cpu/upd9002/upd9002_trace.c.orig`, `cpu/upd9002/upd9002_trace.c.rej.orig`, and `tools/__pycache__/`. No ROM, disk, raw trace, generated binary, or private asset was staged.

## Hosted CI status

NOT RUN.

## G74 status

NOT APPROVED.
