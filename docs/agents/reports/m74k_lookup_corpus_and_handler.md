# M74k lookup corpus and handler

## Identity

Branch: `topic/m74-va1-basic-command-hang`  \nStarting SHA: `57fcd7fddea9334c3e30b8271bd3327934eb6e8d`  \nRuntime image: `pcengine105-bootonly.d88`; explicit `--model va`; trace-enabled build; deterministic frame bounds; no SCSI/HOSTFAT. No production fix was made.

## Counter window and 01E4 discrepancy

The source contract is: `VAEG_M74_CPU_TRACE_COMMAND` matches the headless script command index; `upd9002_m74_trace_arm()` resets the counters immediately before the selected command injection; pre-arm events are discarded; counters then accumulate until shutdown. The summary is session-cumulative from that arm point.

The M74i and M74j A!=1 runs are not byte-identical invocations: M74i used the earlier worker `4844fee...` and a 1500-frame run; M74j used the rebuilt worker `efe02737...` and a 1100-frame run. Their scripts were separately recreated and no complete environment capture proves all other inputs identical. Therefore the `01E4=2` versus `01E4=0` disagreement is retained as a diagnostic-observable reproducibility gap, not guest perturbation. The M74k corpus does not use `01E4` for lookup classification.

The disabled-gate TVRAM digest is byte-identical to the established M74f 1.05 reference:

```text
M74f reference: c6ff29f6852e02e822ce3ea628817a0fbe15bbb832701d734b3d269ac43044b5
M74k disabled:  c6ff29f6852e02e822ce3ea628817a0fbe15bbb832701d734b3d269ac43044b5
```

## Isolated corpus

Each row was a fresh run with one injected command. Injection was frame 840; successful runs exited at frame 1080; terminal `A=1` was bounded at frame 1100/1300 depending on the run. The six counters are fixed-address events. `3835` is a shared target for prelookup exits as well as the FFFE route, so the direct equality `3823=3831+3835+3837` is not a valid global invariant. For lookup classification, `3821` counts entries that reached the INT97 instruction, and its outcomes are `3831`, `3835`, or `3837`.

| command | 3823 | 3831 | 3835 | 3837 | 3816 | 3818 | 3821 | 391D | terminal/Ok |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---|
| `A%=1` | 5 | 0 | 5 | 0 | 5 | 5 | 5 | 1 | Ok |
| `A$=""` | 5 | 0 | 5 | 0 | 5 | 5 | 5 | 1 | Ok |
| `A!=1` | 5 | 0 | 5 | 0 | 5 | 5 | 5 | 1 | Ok |
| `A=1` | 4 | 0 | 4 | 0 | 4 | 4 | 4 | 1 | terminal `3983` |
| `PAINT(0,0),3` | 1 | 1 | 4 | 0 | 5 | 1 | 1 | 1 | Ok |
| `PRINT(1)` | 1 | 1 | 3 | 0 | 4 | 1 | 1 | 1 | Ok |
| `? 1` | 0 | 0 | 2 | 0 | 2 | 0 | 0 | 1 | Ok |
| `LIST` | 1 | 1 | 0 | 0 | 1 | 1 | 1 | 1 | Ok |
| `NEW` | 1 | 1 | 0 | 0 | 1 | 1 | 1 | 1 | Ok |
| `CLS` | 1 | 1 | 0 | 0 | 1 | 1 | 1 | 1 | Ok |

The lookup-entry accounting is complete for every row: `3821` equals the sum of its mutually exclusive post-lookup outcomes (`3831 + (3835- prelookup-only events) + 3837`). The five rows with `3831>0` are the positive candidates: `PAINT(0,0),3`, `PRINT(1)`, `LIST`, `NEW`, and `CLS`.

Corpus totals: `3823=24`, positive `3831=5`, positive fraction `5/24` (20.8333%). No `3837` event occurred. Thus a positive AH=00-measured service outcome is observed in the corpus. This establishes that the service path is not universally negative, but the current fixed-address counters do not capture AX/name/selector state for the positive events.

The A=1 result remains `3823=4, 3831=0, 3835=4, 3837=0`, hence all four A=1 lookup outcomes are the FFFE branch by control-flow. The `A!=1` result is not evidence about `383A`, because it exits at the `!` escape.

## Dispatch and handler status

The emulator uses the normal guest software-interrupt path. `cpu/upd9002/upd9002_core.c:upd9002_intnum()` records the exception, pushes FLAGS/CS/IP, and loads the vector from `mem + vect*4`; it does not special-case `0x97`. The current CPU mode uses the real-mode vector base. Therefore INT 97h reads vector bytes at `0000:025C--025F`. The normal vector implementation is confirmed; no host-side INT97 interception exists.

The exact vector value consumed at reset, BASIC prompt, and command injection, the complete vector writer lifecycle, and the handler target disassembly were not captured in this pass. Existing `1040` census data does not identify the INT97 vector. This is a precise remaining gap, not evidence that the handler is absent.

The handler ABI is consequently not yet established. The ROM-side caller proves only that `AH=00`, `BX`, `CX`, and `DS:SI` are presented at `3823`, and that `3806` classifies the returned AX through `FFFE`/`FFFF` comparisons. The corpus proves a non-negative service outcome exists, but not its AX value or the table it searched.

## Continuation-segment dumps

The requested 256-byte dumps at reset, first prompt, and pre-injection for segments `34C0`, `43B5`, `49FC`, `75AB`, and `7F2D` were not captured. The exact blocker is that the current approved fixed-address seam has no segment-base dump event, and adding a new three-checkpoint memory snapshot would be a separate diagnostic seam requiring its own N0/N1/N2 equivalence run.

The known target remains `34C0:0005` = physical `34C05h`, ordinary VA RAM, zero/unwritten in existing lifecycle evidence. DX is not known for the other four callsites; no target offset is inferred.

## E4 status

The relevant `1040:0AC3` slot remains observed as `CB` in the available census. The installer source-record lists at `19E3:C7EB` and `1CC5:C6BB` were not fully decoded in this pass. E4 is therefore **BLOCKED: source-record decode incomplete; exact blocking field is the installer record format/conditional list, not an observed LIVE handler**.

## D3 and model interpretation

- A=1 tier 1 (`AX=FFFEh` four times): **PROVEN BY FIXED-ADDRESS CONTROL FLOW**.
- FFFE semantic meaning: **UNDETERMINED** at handler ABI level.
- Four selector namespace mapping: **UNDETERMINED**.
- Positive AH=00 outcome: **PROVEN OBSERVED** in five corpus runs, but the returned AX/name/selector are not captured.
- 383A aggregate interpretation: remains the leading model for the non-escape path; `A!=1` does not test it.
- First incorrect emulator-produced state: **None proven**.
- Production fix: **None**.

No handler-missing or empty-table conclusion is drawn. The positive corpus result specifically prevents treating all AH=00 calls as universally negative.

## Evidence and validation

Worker SHA: `efe027376f95645c274bfd3395ee9abdc507d2ea3c21bc52087ca5f1cd10d379`. Individual corpus log and TVRAM SHA-256 values are retained outside Git; the deterministic counter rows above are extracted from those logs. The disabled prompt-gate log/TVRAM was byte-checked against the M74f reference. The trace-enabled build completed successfully. `git diff --check` is required before commit. Repository invariant checks and the previously recorded selftest/M68-M70/romless results remain the applicable validation; hosted CI was not run.

Worktree retains only pre-existing untracked diagnostic backups and `tools/__pycache__`; no ROM, disk, raw trace, generated binary, or private asset was added to Git. G74 remains not approved.
