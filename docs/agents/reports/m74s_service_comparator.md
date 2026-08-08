# M74s service comparator and scratch residue provenance

## Identity

- **Proven repository fact.** Branch: `topic/m74-va1-basic-command-hang`.
- **Proven repository fact.** Starting SHA: `451dcf5bb031fcd0361b8fa737dca1fe05baa48a`.
- **Proven repository fact.** Final diagnostic source SHA before this report: `f0873e451a8b8952d54bfbca764339b6b68a26a0`.
- **Proven dynamic identity.** Final worker SHA-256: `a3af418b96911de373c61f7d4f42665b4846fea644af423100b4b1839c3629e8`.
- **Proven repository fact.** Runner: `tools/m74-diagnostics/run_basic_case.sh`; SHA-256 `38602ad31550b576e45168f8eff57cc83bc594efa047f077afc590e59df8c580`; runner change commit `56d04d08a095ac6422f1bd2fd8163227dfc911bf`.
- **Proven runtime identity.** BASIC 1.05 boot-only image SHA-256: `35c17df8b65f747b1d789200bf950f07c092ac791e29169bfd49a089893b7e4d`; VA ROM SHA-256: `656d81bc532ed0bb602d1eb7f03df27c74f841fb19c72ad07d47e79302ac814b`.
- **Proven runtime contract.** Model VA, trace-enabled native build, deterministic frame bound 1100, first prompt frame 720, selected command injection frame 840.
- **Rejected methodology.** Host elapsed time was not used as a guest-semantic verdict.

The reset-armed rows are a separate evidence class from the historical command-armed rows. Their raw totals are not merged.

## M74r gate-hypothesis closure

**Rejected hypothesis.** `H-[0374:0096]-CAUSES-FAILURE` remains **REJECTED**. M74r's disposable nonzero intervention was removed and is not a fix. It changed the `2730` branch but still returned through `0191`, reached `01E4`, and transferred to `34C0:0005`. M74s did not repeat or retain that probe.

## M74r object and scratch observations

**Proven dynamic observation.** For both failing commands, the source object at `2E8A:0236-023D` and its post-copy scratch image were all zero:

| Command | Source | Scratch before `2730` |
| --- | --- | --- |
| `A=1` | `00 00 00 00 00 00 00 00` | `00 00 00 00 00 00 00 00` |
| `PRINT 1` | `00 00 00 00 00 00 00 00` | `00 00 00 00 00 00 00 00` |

**Proven dynamic observation.** Before the failing copy, the scratch object was `00 00 00 00 24 16 32 46`. The exact writer of this residue is resolved below; it was not a prior `0180` invocation.

## Arming-window correction

**Proven diagnostic fact.** Historical command-local arming could not observe service activity before command injection. M74s added reset-armed fixed counters and snapshots at the existing reset, first-Ok, and selected-injection checkpoints. Reset arming changes the observation window, not guest configuration.

**Proven equivalence result.** With reset-armed extensions disabled, the final worker reached first Ok at frame 720 and produced TVRAM SHA-256 `c6ff29f6852e02e822ce3ea628817a0fbe15bbb832701d734b3d269ac43044b5`, exactly matching the accepted reference. With the extensions enabled, `PRINT 1` reproduced `391D=1 3983=0 3985=1 3988=1 34C0=0 002A=1 0180=1 01E4=1` and the inherited `0005,34C0` far frame.

## Reset-armed service counts

**Proven dynamic observation.** Primary RESET-ARMED `PRINT 1` row:

| Window | `0180` | `2730` | `2751` | `0191` | `01E4` |
| --- | ---: | ---: | ---: | ---: | ---: |
| reset to first Ok | 0 | 0 | 0 | 0 | 2 |
| first Ok to injection | 0 | 0 | 0 | 0 | 0 |
| after injection to frame 1100 | 1 | 1 | 0 | 1 | 1 |
| total | 1 | 1 | 0 | 1 | 3 |

`A=1` has the same service-window counts. `A%=1` has totals `0180=0 2730=0 2751=0 0191=0 01E4=2`; its two `01E4` events both precede first Ok. Therefore `01E4` is not a valid proxy for `0180` across startup.

## Pre-command 0180 invocation count

**Proven dynamic observation: PRECOMMAND-0180-ABSENT.** The exact pre-command count is `N=0` from reset through injection in the 1.05 `PRINT 1` session. `2730` is also zero in both pre-command windows.

**Rejected hypothesis.** `H-HIDDEN-PRECOMMAND-COMPARATOR` is **REJECTED** for this configuration. Command-time arming did hide two startup `01E4` events, but it did not hide an `0180/2730` invocation.

## Ordinal-selected invocation records

**Bounded comparator result.** Not applicable: `N=0`, so there is no pre-command `0180` ordinal to select. No unbounded census was introduced.

M74s separately captured the first two pre-command calls of the shared `01F7` copy helper after fixed counters proved that this helper ran without `0180`. Those records are alternative scratch-writer evidence, not `0180` comparator records:

| Helper ordinal | Outer call return | Source | Source bytes | Scratch before | Scratch after |
| ---: | --- | --- | --- | --- | --- |
| 1 | `E000:0086` (`0083 CALL 01E5`) | `2E8A:0652` | `00 00 00 00 00 00 90 41` | all zero | `00 00 00 00 00 00 90 41` |
| 2 | `E000:006C` (`0069 CALL 01E5`) | `2E8A:0652` | `00 00 00 00 20 71 26 45` | `00 00 00 00 12 49 29 46` | `00 00 00 00 20 71 26 45` |

Both use `01E5 -> 01EC CALL 01F7`, with local return `01EF`. Neither produces the final `24 16 32 46` residue.

## Known-normal comparator disposition

**Bounded comparator result: NO-SAME-SERVICE-COMPARATOR.** No pre-command `0180` exists, so no known-normal invocation satisfying the M74s comparator contract is available. The two `01E5/01F7` records are other-service numerical copies and are not promoted to normal `0180/2730` comparators.

## Per-invocation far-frame comparison

**Bounded comparator result.** There are no pre-command `0180` far frames to compare. The failing command observations remain:

| Command | `0180` entry SP | IP word | CS word | RETF target |
| --- | --- | --- | --- | --- |
| `PRINT 1` | `7FE0:01F4` | `0005` | `34C0` | `34C0:0005` |
| `A=1` | `7FE0:01F4` | `0005` | `34C0` | `34C0:0005` |

## Per-invocation source object comparison

**Bounded comparator result.** No pre-command `0180` source object exists. The two alternative `01E5/01F7` source objects are nonzero packed numerical objects and differ from the failing all-zero `2E8A:0236` object, but they belong to a different service entry contract.

## Per-invocation scratch object comparison

**Bounded comparator result.** The two alternative copy-helper scratch results are `...9041` and `...20712645`; neither equals the first-Ok residue `...24163246`. The failing command copy overwrites the residue with eight zero bytes.

## Per-invocation 2730 branch

**Proven dynamic observation.** No pre-command `2730` branch exists. After injection, `A=1` and `PRINT 1` each take the field-zero path (`2751=0`) and return through `0191=1`.

## Per-invocation RETF target

**Proven dynamic observation.** No pre-command `0180` invocation supplies a comparator RETF target. The two startup `01E4` events are from other paths and were not attributed to `0180`. The failing invocation reaches `34C0:0005`, whose captured target bytes remain zero.

## 24 16 32 46 residue provenance

**Proven static fact.** The relevant ROM path is:

```text
12AC  BB AF 00       MOV BX,00AF
12AF  E8 1F 01       CALL 13D1
13D1  8C D9          MOV CX,DS
13D3  8E C1          MOV ES,CX
13D5  FC             CLD
13D6  8B F3          MOV SI,BX
13D8  BF 8E 00       MOV DI,008E
13DB  B9 04 00       MOV CX,0004
13DE  A4             MOVSB
13DF  F3 A5          REP MOVSW
13E1  C3             RET
```

Because entry is `13D1`, the preceding `13CE SUB BX,0008` is not executed. The operation copies nine bytes from `0374:00AF-00B7` to `0374:008E-0096`; destination bytes 1-8 are exactly scratch object `008F-0096`.

**Proven dynamic observation: RESIDUE-NOT-FROM-OBSERVED-0180.** Both pre-first-Ok executions captured:

```text
source 0374:00AF-00B7 = 00 00 00 00 00 24 16 32 46
return IP             = 12B2
object after copy      = 00 00 00 00 24 16 32 46
```

Thus `E000:13DE MOVSB; 13DF REP MOVSW`, reached from `12AF CALL 13D1`, is the exact direct writer of the residue at `0374:008F-0096`. It executes twice before first Ok. This is another numerical workspace copy, not an earlier `0180` object copy.

**Inference, not root cause.** `24 16 32 46` has the packed-numerical layout proven in M74r and is consistent with decimal digits `321624` plus low-seven-bit format value `46h`. M74s does not need that coincidence to identify the writer and does not claim the residue is a continuation comparator.

## DS:0236-023D static writer enumeration

**Proven static boundary.** `33F7 MOV DI,022E` selects the local buffer. `34A7-34B8` writes words ending at `0235` and reaches `34BD` with `DI=0236`; no instruction in this local path writes `0236-023D`.

A complete repository-wide static writer count cannot be stated. The exact blocker is indirect aliasing through numerical/BASIC routines that use runtime `DS:DI`, `ES:DI`, `MOV [DI]`, `STOS`, and variable-range `REP MOVS`. Without the prohibited address-range/EA write seam, those operations cannot all be proven disjoint from physical `2EAD6-2EADD`. Therefore the complete writer census remains blocked by specifically named indirect write aliases; no false completeness claim is made.

For the scratch page, a finite 31-address set of direct/fixed-destination candidates was counted. Before first Ok in the primary `PRINT 1` run, only `0200` (2), `1309` (1), `1312` (1), and `13DE` (2) fired. The bounded `13DE` capture resolves the final residue writer.

## DS:0236-023D candidate writer runtime comparison

**Proven dynamic observation.** At the wrapper boundary, both paths execute each local setup address exactly once:

| Address | `A=1` | `A%=1` |
| --- | ---: | ---: |
| `33F7` | 1 | 1 |
| `34A7` | 1 | 1 |
| `34B0` | 1 | 1 |
| `34B4` | 1 | 1 |
| `34B8` | 1 | 1 |
| `34BD` | 1 | 1 |

At `34BD`, both capture `DS:DI=2E8A:0236` and bytes `00 00 00 00 00 00 00 00`.

## A=1 versus A%=1 upstream object preparation

**Proven dynamic observation.** No source-object divergence exists at the measured wrapper boundary:

| Command | `DS:DI` | Object | Wrapper outcome |
| --- | --- | --- | --- |
| `A=1` | `2E8A:0236` | all zero | `3983 -> 002A -> 0180 -> 01E4` |
| `A%=1` | `2E8A:0236` | all zero | `3988 -> E000:34C0`; no `0180` |

**Rejected hypothesis within this boundary.** `H-SOURCE-OBJECT-DIVERGENCE` is rejected at `34BD`: successful `A%=1` and failing `A=1` present the same eight source bytes. This does not prove that all-zero is correct for every downstream service; the escape path does not consume it through `0180`.

## 0374 page timeline

**Proven dynamic observation.** Final 1.05 page captures:

| Checkpoint | SHA-256 | Object `008F-0096` |
| --- | --- | --- |
| reset | `5341e6b2646979a70e57653007a1f310169421ec9bdd9f1a5648f75ade005af1` | all zero |
| first Ok | `8a38b2c9bcd2ebccab8bf57d9537532c773c0dedf213828dc2c79b97bcc5b659` | `00 00 00 00 24 16 32 46` |
| failing pre-copy | same as first Ok | `00 00 00 00 24 16 32 46` |
| failing pre-2730 | `3babb5db3f7b4beaef81b0114ef52928a42d1baf4d05828d7d1f5f4b81356d18` | all zero |
| post-2730 | same as pre-2730 | all zero |
| pre-01E4 | same as pre-2730 | all zero |

Selected pre-command `0180` pre/post pages are not applicable because `N=0`. Page hashes delimit changes; writer identity comes from the fixed-address and bounded `13DE` capture, not from hash differences alone.

## Boot-only E4 closure

**Proven static closure.** E4 remains **CLOSED** for the current boot-only configuration. The complete `19E3:C7EB` list contains 42 records over 168 bytes and no source record for destination `1040:0AC3`. This is not an installer skip of a present record. `1CC5:C6BB` remains outside current boot-only scope.

## Pre-populated continuation-model disposition

**Working hypothesis disposition: NOT PRIMARY.** Five sampled continuation windows are zero; the below-free band is largely zero; no direct `34C0` work-area anchor was found; measured mapping selection matches the documented low-memory resource; and M74s finds no normal pre-command `0180` comparator that points back to target ownership.

## Hypothesis table

| Hypothesis | M74s status | Evidence |
| --- | --- | --- |
| `[0374:0096]=00` causes failure | REJECTED | M74r intervention changed branch but not terminal path |
| hidden pre-command `0180` comparator | REJECTED | reset-to-injection `0180=0` |
| residue from prior `0180` | REJECTED | exact writer is `12AF -> 13D1 -> 13DE/13DF` |
| all-zero source object causes failure | UNPROVEN, weakened | successful `A%=1` also has all-zero object at `34BD` |
| successful/failing paths prepare different `0236-023D` bytes | REJECTED at measured wrapper boundary | both byte-identical at `34BD` |
| continuation target ownership is primary | DEPRIORITIZED | no comparator redirects analysis there |

## First incorrect emulator-produced state

**None exposed.** All observations are guest state and control flow. No independent authority establishes that the all-zero source object, residue copy, or continuation frame differs from real hardware at a first producer boundary.

## Root cause status

**UNRESOLVED.** M74s closes the hidden-comparator question and identifies the residue writer, but neither result exposes the first incorrect emulator-produced architectural state.

## Production fix

None. Only diagnostic and documentation files changed. No command, address, object field, or continuation target is special-cased in production code.

## Validation

Git-dependent checks used process-local `GIT_CONFIG_GLOBAL=/dev/null GIT_CONFIG_SYSTEM=/dev/null`; persistent user/global/system Git configuration was not modified.

| Command | Result |
| --- | --- |
| `python3 tools/repo/check_encoding.py` | PASS, exit 0 |
| `python3 tools/repo/check_eol.py` | PASS, exit 0 |
| `python3 tools/repo/check_case.py` | PASS, exit 0, 0 findings |
| `git diff --check` | PASS, exit 0 |
| `python3 tools/qa/milestone_ids.py --root . --selftest --discover --audit` | PASS, exit 0, 48 selftests |
| `cmake --build build/linux-release -j2` | PASS, exit 0, trace-enabled native build |
| `build/linux-release/sdl2/vaeg --selftest` | PASS, exit 0 |
| targeted ROM-less/M68/M69/M70 CTest regex | PASS, 6/6 |
| non-external CTest partitions | 92 PASS, 2 FAIL, 5 SKIP |
| `cmake --preset mingw-cross` | PASS, exit 0 |
| `CCACHE_DISABLE=1 cmake --build --preset mingw-cross -j2` | PASS, exit 0, no work required |

The two failures are the pre-existing protected-deletion pair `vaeg_upd9002_protected_deletion` and its selftest: expected `cpu/upd9002/upd9002_ops.mcr` SHA-256 `dbfcc5b3ce7d3f0b4df493cd494b7fe297aa932e231904ddeb4b59411cd73183`, actual `73c75f7a82706487b51a66e30718d6daef21caa9f73458cd3d538a059fe4d089`. The five dependency tests `test_suffix.sh`, three generated-compress tests, and `test_files.sh` are SKIP, not PASS.

The partition commands were `ctest --test-dir build/linux-release -LE external -I 1,30 -j4 --output-on-failure`, then ranges `31,45`, `46,60`, `61,80`, followed by `-R '^(vaeg_m75_transfer_info_compiled|test_)' -j4`. All Git-using partitions were run with process-local Git isolation.

## Runtime identities

Final rows use source `f0873e451a8b8952d54bfbca764339b6b68a26a0`, worker `a3af418b96911de373c61f7d4f42665b4846fea644af423100b4b1839c3629e8`, runner SHA `38602ad31550b576e45168f8eff57cc83bc594efa047f077afc590e59df8c580`, BASIC 1.05 boot-only media, model VA, frame bound 1100, reset arming enabled, command-local reachability arm command 3, and allocation capture enabled.

## External containment

**Proven runtime disposition.** No run was externally preempted. `PRINT 1` and `A=1` reached their required command-local `0180/2730/01E4` events, then ended at the requested guest frame 1100; their verdicts are admissible for those events but are not normal-completion verdicts. `A%=1` reached a second `Ok` at frame 960 and requested normal runner exit at frame 1080.

## Remaining gaps

1. The first upstream routine that constructs numerical workspace `0374:00AF-00B7` remains descriptive, although its exact copy into scratch is proven.
2. A complete writer census for physical `2EAD6-2EADD` remains blocked by computed `DS/ES:DI` aliases and variable-range string operations under the no-EA-watch constraint.
3. No known-normal pre-command invocation of the same `0180/2730` service exists in this boot-only session.
4. The first incorrect emulator-produced architectural state remains unidentified.

## Changed files

- `cpu/upd9002/upd9002_trace.c`: reset-window service counters, bounded helper/copy captures, finite scratch-writer counters, and wrapper-source capture.
- `tools/m74-diagnostics/run_basic_case.sh`: tracked reset-arm switch and run identity.
- `docs/agents/reports/m74s_service_comparator.md`: this report.

## Worktree status

The tracked changes are the completed diagnostic commits plus this report. Pre-existing `cpu/upd9002/upd9002_trace.c.orig`, `cpu/upd9002/upd9002_trace.c.rej.orig`, and `tools/__pycache__/` remain untracked and untouched. No disposable M74r probe behavior is present.

## Hosted CI status

**NOT RUN.** Local validation was completed; hosted CI was not invoked.

## G74 status

**NOT APPROVED.** M74s does not self-approve G74.
