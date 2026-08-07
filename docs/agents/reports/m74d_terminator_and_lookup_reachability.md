# M74d: terminator and lookup reachability

## Scope and identity

G74 is not approved. This diagnostic-only pass starts at
`6bc415de9ce86eeed7b1baaacfebe10cd14e3de4` on branch
`topic/m74-va1-basic-command-hang`, with approved G73 predecessor
`766a132ff6d66e335fe9bb1d0082d777a4a8fe14`.

Mandatory run conditions used for the admissible attempt:

- `--model va`
- `-DVAEG_ENABLE_TESTS=ON`
- `-DVAEG_Z80_COMPAT_INTEGRATION_TRACE=ON`
- `VAEG_HEADLESS_MAX_FRAMES=5000`
- `VAEG_M74_CPU_TRACE_LIMIT=300000`
- boot-only images only

Trace worker SHA-256:
`0a0b7ac2674ae2441f59181c5e028e2b79139edf866ec377b78298e56fe019a1`.

## A0 result

The admissible 1.05 attempt reached the first BASIC prompt and injected `A=1`
at frame 840. It did not reach a second prompt before the deterministic absolute
frame bound at frame 5000. The trace was event-only with lifecycle and vector
watches disabled. No `m74-parser-entry`, `m74-thunk-retf`, or equivalent
`E000:391D` record was emitted before the run ended.

This is a bound-limited non-arrival, not a hang classification. The existing
M74 trace seam still incurs enough host overhead in other modes that a complete
control-transfer/lifecycle dump could not be completed within the interactive
run budget. That is an instrumentation limitation, not guest evidence.

The 1.00 and 1.10 runs were not completed to an admissible A0 trace result in
this pass. No claim is made about their failure shape.

| Claim | Verdict |
|---|---|
| D0a | UNDETERMINED: fixed-bound non-arrival, terminal chain not captured |
| D0b | UNDETERMINED: required 1.00/1.10 A0 evidence incomplete |
| D1 | UNDETERMINED |
| D2 | UNDETERMINED; no valid 391D-to-397A window |
| D3 | UNDETERMINED; no lookup count/AX evidence |
| D4 | UNDETERMINED; no lookup ABI control established |

## Negative results and gaps

No SCSI or HOSTFAT image was run. No production source, ROM, disk image, raw
trace, or generated binary was added to Git. No `INT 97h` execution count, no
per-probe branch classification, no `AL` at `3948`, and no RETF target was
obtained. The full terminator matrix was therefore not started, as required by
the A0 gate.

The next bounded boundary is to make the A0 trace seam cheap enough to retain
only the required control transfers and parser checkpoints, then rerun the
three boot-only images under the same explicit conditions. The current result
must not be promoted to a hang or to a zero-page continuation.

## Validation

- Trace-enabled configure/build: PASS.
- `git diff --check`: to be run after this report commit.
- `--selftest`, M68/M69/M70, and `ctest -L romless`: not run in this diagnostic
  continuation.
- G74 remains not approved.
