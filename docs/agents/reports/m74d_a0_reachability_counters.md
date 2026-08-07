# M74d-A0: reachability counters

## Scope

Diagnostic-only M74d-A0. G74 is not approved and no production correction is included.

- Branch: topic/m74-va1-basic-command-hang
- Starting SHA: 178bbab6df620b08773780e795059db01a0a762c
- Approved G73 predecessor: 766a132ff6d66e335fe9bb1d0082d777a4a8fe14
- Required model: --model va
- Build flags: VAEG_ENABLE_TESTS=ON, VAEG_Z80_COMPAT_INTEGRATION_TRACE=ON
- Images: boot-only 1.00, 1.05, 1.10 only
- Configured frame ceiling: 20,000
- First BASIC prompt: frame 720
- Command injection: frame 840
- Failing-session prompt timeout: frame 12,960
- Positive-control completion: frame 1,320

## Instrumentation recovery

The rollback history retained the diagnostic commits. Recovered commits were:
9ffaace (explicit VA model), 2846a3d and 596c4af (image differential
diagnostics), and 4e22803 (deterministic headless frame bound). The current
branch also contains 178bbab. M74 diagnostic tooling should be kept on a
long-lived tool/m74-diagnostics branch and rebased into report branches;
repeated report rollbacks otherwise discard measurement tooling.

The new seam counts only address-triggered events at E000:391D, 3983, 3985,
002A, and 01E4, with one-shot register/stack/RETF captures. Lifecycle, vector,
and EA-write watches were disabled. The captured build still used the legacy
trace configuration path with a one-record CPU trace allocation; it emitted no
per-instruction trace dump. Pure reachability-only startup and a frame callback
were not completed. This is a tooling gap, not guest evidence.

## A0.1 input delivery

Both inputs were echoed after the first Ok. A%=1 reached a second prompt and
? A% printed 1; A=1 remained echoed without a second prompt.

| Image | Input | First Ok | Inject | Second Ok/end | Echo |
|---|---|---:|---:|---:|---|
| 1.00 | A=1 | 720 | 840 | none; timeout 12960 | yes |
| 1.05 | A=1 | 720 | 840 | none; timeout 12960 | yes |
| 1.10 | A=1 | 720 | 840 | none; timeout 12960 | yes |
| 1.00 | A%=1 | 720 | 840 | 960 | yes |
| 1.05 | A%=1 | 720 | 840 | 960 | yes |
| 1.10 | A%=1 | 720 | 840 | 960 | yes |

A0.0: PROVEN.

## A0.2 counters

The result is identical for all three ROM versions.

| Image | Input | 391D | 3983 | 3985 | 002A | 01E4 |
|---|---|---:|---:|---:|---:|---:|
| 1.00 | A=1 | 1 | 1 | 0 | 1 | 1 |
| 1.05 | A=1 | 1 | 1 | 0 | 1 | 1 |
| 1.10 | A=1 | 1 | 1 | 0 | 1 | 1 |
| 1.00 | A%=1 | 2 | 0 | 2 | 0 | 0 |
| 1.05 | A%=1 | 2 | 0 | 2 | 0 | 0 |
| 1.10 | A%=1 | 2 | 0 | 2 | 0 | 0 |

The A=1 first entry capture was DX=0005 SI=002A, with DS 27F4 (1.00),
2E8A (1.05), and 2AE7 (1.10), and caller E000:34C0. The 3983 stack words
were 002A,0005.

First-hit frame numbers for the five CPU counters were not emitted by this
seam. The exact gap is a missing callback from the headless frame loop into
the counter state; the command, prompt, and timeout frames above are recorded
by the headless harness. No frame value is inferred.

## A0.3 terminal target

    E000:391D -> E000:3983 -> E000:002A -> E000:01E4 RETF
    RETF frame: IP=0005 CS=34C0
    result: 34C0:0005
    first 16 bytes: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00

The end state is executing a zero-filled page. This describes the observed
post-RETF state; it does not by itself identify the root cause.

- D0a: PROVEN -- 1.05 reproduces the complete chain.
- D0b: PROVEN -- 1.00 and 1.10 reproduce the same chain and bytes.

## Version comparison

No control-flow difference was observed. The only recorded entry-state
difference is BASIC work-area DS: 1.00=27F4, 1.05=2E8A, 1.10=2AE7.
The counter sequence, stack frame, RETF target, and target bytes are identical.

## Negative results and scope

No SCSI or HOSTFAT image was run. No terminator matrix, INT 97h semantics,
lookup-table analysis, or continuation-ownership analysis was started. No ROM,
disk image, raw trace, or generated binary was added to Git.

## Hashes

Worker:
34bfd64a9c61bff6f44ee7aab1081e5e858b42b2bd5b4ce1b70ead12bf38d9f2

Boot-only disk identities:

- 1.00: bf551fc8d87f91072fefea94983a8477d7f84418bd73b24d5cf1dc6d94c09d4c
- 1.05: 35c17df8b65f747b1d789200bf950f07c092ac791e29169bfd49a089893b7e4d
- 1.10: 258d7d218289ab0437e8772aa50c86763fc904e024e36243823323cd86602275

TVRAM evidence:

- 1.05 A=1: a753dfd442ff76d8c077b4e65d784755035002ba88c0f47460ff2e4eab2a8634
- 1.05 A%=1: f1879d7a85207a1411342eb23efb6fbe8445667da60fdbc4d5657a6165c84e2a
- 1.00 A=1: 55ec29212748fd80ad638d2980e271c790ae988116f72713a9c8917167339536
- 1.10 A=1: f4cb0a0753a0e12079b9efbc6844fa9109da0d8de26a142844ee7686f2f75c60

## Validation

- Trace-enabled configure/build: PASS.
- vaeg --selftest: PASS; 193 manifest cases.
- M68/M69/M70 focused tests: PASS, 5/5.
- ctest -L romless: 69 passed, 2 failed, 1 skipped external.
- The two failures are the pre-existing protected-deletion checks for
  cpu/upd9002/upd9002_ops.mcr: expected digest dbfcc5b..., actual branch
  digest 73c75f7.... This task did not modify that file.
- git diff --check: PASS.

G74 remains not approved.
