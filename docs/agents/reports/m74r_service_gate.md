# M74r service-object field gate

## Identity

- **Proven dynamic observation.** Branch: `topic/m74-va1-basic-command-hang`.
- Starting source: `9da4b8f8f9eab47a0f6c4680b5c873b4023aabda`.
- Diagnostic source used for final normal rows: `46be1071d268f38e95fd56f88a41f836d07ab363`.
- Normal worker SHA-256: `621fca968e78a214dd90a80047a309f7bb84908b8e6b9b6ec86f088160de23ee`.
- Runner SHA-256: `1e1aa04b21219def77b10b8557e1e6dcebe785880dcd2be98725e2ae984ab565`.
- ROM identity: `varom00.rom`, SHA-256 `656d81bc532ed0bb602d1eb7f03df27c74f841fb19c72ad07d47e79302ac814b`.
- Runtime: VA model, trace-enabled build, deterministic frame bound, boot-only 1.05 image.
- Gate: G74 is **NOT APPROVED**.

## M74q closures

- **Proven static and dynamic fact.** The measured `34BD/391D` wrapper deliberately creates `[002A][0005][34C0]`; `3983` or `3988` consumes `002A`, leaving `[0005][34C0]` for the service.
- **Rejected hypothesis.** `3983` is not the failure boundary, `3985` does not imply completion, and `E000:34C0=0` is not contradictory.
- **Proven dynamic observation.** All five sampled continuation windows were zero and the below-free band was largely zero. The pre-populated-continuation model remains downgraded and is not the primary M74r line.
- **Proven static and dynamic fact.** The complete boot-only `19E3:C7EB` list has 42 records in a 168-byte source extent and no destination record for `1040:0AC3`.

## Correction: 0374:0096 is object field 7

**Proven static fact.** `0374:0096` is the last byte of the eight-byte object at `0374:008F-0096`. It is not modeled as an independent global gate. The normal copy at `E000:0200 REP MOVSW` is the expected writer from the caller object.

## E000:0180 object-copy contract

| Address | Bytes | Instruction | Architectural effect |
| --- | --- | --- | --- |
| `0180` | `FB` | `STI` | enables maskable interrupts |
| `0181` | `06` | `PUSH ES` | saves caller ES |
| `0182` | `1E` | `PUSH DS` | saves caller DS |
| `0183` | `57` | `PUSH DI` | saves source offset |
| `0184` | `BA 74 03` | `MOV DX,0374` | selects service segment |
| `0187` | `8E C2` | `MOV ES,DX` | ES becomes `0374` |
| `0189` | `E8 6B 00` | `CALL 01F7` | copies eight bytes from caller `DS:DI` |
| `018C` | `8E DA` | `MOV DS,DX` | DS becomes `0374` |
| `018E` | `E8 9F 25` | `CALL 2730` | consumes service object |
| `0191` | `EB 47` | `JMP 01DA` | common return epilogue |
| `01DA` | `5F` | `POP DI` | restores caller DI |
| `01DB` | `07` | `POP ES` | restores original DS into ES |
| `01DC` | `E8 2E 00` | `CALL 020D` | copies service object back to caller |
| `01DF` | `8C C2` | `MOV DX,ES` | stages original DS |
| `01E1` | `8E DA` | `MOV DS,DX` | restores DS |
| `01E3` | `07` | `POP ES` | restores original ES |
| `01E4` | `CB` | `RETF` | consumes inherited far frame |

`01F7` is `MOV SI,DI; MOV DI,008F; MOV CX,4; CLD; REP MOVSW; RET`. Thus the exact mapping is caller `DS:DI+[0..7]` to `0374:008F+[0..7]`.

## E000:2730 complete annotated disassembly

| Address | Raw bytes | Instruction | Stack/flags/memory role |
| --- | --- | --- | --- |
| `2730` | `C6 06 8E 00 00` | `MOV BYTE [008E],00` | clears auxiliary byte before the object; no stack change |
| `2735` | `F6 06 96 00 FF` | `TEST BYTE [0096],FF` | reads field 7, sets ZF/SF/PF, no write |
| `273A` | `74 14` | `JZ 2750` | zero path to local `RET` |
| `273C` | `9C` | `PUSHF` | saves flags made from the original field; SP -2 |
| `273D` | `80 26 96 00 7F` | `AND BYTE [0096],7F` | clears field-7 bit 7; updates flags |
| `2742` | `E8 0C 00` | `CALL 2751` | near call; balanced by a reachable `RET` |
| `2745` | `9D` | `POPF` | restores original field flags; SP +2 |
| `2746` | `79 05` | `JNS 274D` | tests original field bit 7 through saved SF |
| `2748` | `80 0E 96 00 80` | `OR BYTE [0096],80` | restores original sign bit when set |
| `274D` | `E8 E8 DB` | `CALL 0338` | runs follow-up numerical service |
| `2750` | `C3` | `RET` | returns to `E000:0191` |

The complete `2751` branch structure is:

- `2751-276D` performs two comparisons and selects `2775`, `2826`, or `2849`.
- `2775-2825` executes the main numerical path and ends in `2825 RET`.
- `2826-2848` compares field 7 with `38h`, conditionally performs a shorter numerical path, and ends in `2848 RET`.
- `2849-287B` performs the third numerical path, sets field-7 bit 7 at `286D`, and ends in `287B RET`.

Every statically resolved `2751` destination returns to `2745`; none consumes or rewrites the inherited `0180` far frame.

`0338` calls `147F`, saves SP in `DS:[0117]`, calls `03C2`, then transfers to `123C`. The cleanup at `1257` restores SP from `[0117]`, calls `14B9` to restore the saved numerical workspace, and executes `RET`. `03C2` calls the installed `0374:000A` slot, then either returns directly at `0402` or uses a `123C/1242/124E` cleanup return. At the measured first-Ok state, `0374:000A` is the installed `CB` far-return stub. The disposable run also dynamically traversed `03C2` and reached `0191`, resolving the runtime-indirect return for this configuration.

## E000:2730 zero branch

**Proven static and dynamic fact.** With field 7 equal to `00`, `273A JZ` is taken to `2750 RET`. Final normal rows have `2751=0`, `0338=0`, `03C2=0`, and `0191=1` for both commands.

## E000:2730 non-zero branch

**Proven static fact.** A nonzero field clears bit 7, calls `2751`, restores the original sign bit when required, calls `0338`, and then executes `2750 RET`.

**Causal intervention result.** Forced field `01` produced `2751=1`, `0338=1`, `03C2=1`, and `0191=1` for both `A=1` and `PRINT 1`. The branch was consumed and returned normally.

## Does the non-zero branch return to 0180?

**NONZERO-RETURNS.** It returns to `E000:0191`. No first non-returning transfer exists in the tested path. The original `0180` call return and inherited `[0005][34C0]` frame remain in place. The nonzero path then executes `020D` and `01E4` just like the zero path.

## 0374:008F-0096 field access table

| Object offset | Scratch address | Proven use |
| ---: | --- | --- |
| `0-6` | `008F-0095` | copied by `01F7/020D`; manipulated as packed decimal payload by the `03C2` and `2751` numerical routines |
| `7` | `0096` | zero/nonzero gate at `2735`; bit 7 saved/cleared/restored as a sign bit; lower bits consumed as numerical format/scale information |

**Object semantics partial, proven from ROM consumers.** `0870` isolates bit 7 as sign and accepts low-seven-bit format values `41h-45h`; `08EE` constructs field 7 as `40h + decimal digit count`, ORed with the sign bit. The eight-byte object is therefore a packed numerical representation. A complete name for every payload nibble is not established, but field 7 is not an unrelated global flag.

## Placeholder/fallback-frame hypothesis

**Rejected hypothesis.** The nonzero path does not escape the `0180` service. It returns and reaches the same `01E4 RETF`. The inherited `[0005][34C0]` words are not a zero-only fallback frame selected by `2730`.

## A=1 source object

**Proven dynamic observation.** At O1, `DS:DI=2E8A:0236`; bytes are `00 00 00 00 00 00 00 00`; field 7 is `00`. Entry stack is `SS:SP=7FE0:01F4`, with `0005,34C0` as the inherited far frame.

## A=1 scratch object

**Proven dynamic observation.** At O2 before `2730`, `0374:008F-0096` is `00 00 00 00 00 00 00 00`; source and scratch are byte-identical.

## PRINT 1 source object

**Proven dynamic observation.** At O1, `DS:DI=2E8A:0236`; bytes are `00 00 00 00 00 00 00 00`; field 7 is `00`.

## PRINT 1 scratch object

**Proven dynamic observation.** At O2, `0374:008F-0096` is `00 00 00 00 00 00 00 00`; source and scratch are byte-identical.

## Source-to-scratch byte comparison

| Offset | `A=1` source | `A=1` scratch | `PRINT 1` source | `PRINT 1` scratch |
| ---: | ---: | ---: | ---: | ---: |
| `0` | `00` | `00` | `00` | `00` |
| `1` | `00` | `00` | `00` | `00` |
| `2` | `00` | `00` | `00` | `00` |
| `3` | `00` | `00` | `00` | `00` |
| `4` | `00` | `00` | `00` | `00` |
| `5` | `00` | `00` | `00` | `00` |
| `6` | `00` | `00` | `00` | `00` |
| `7` | `00` | `00` | `00` | `00` |

## Known-normal service comparator, if available

**Proven bounded negative observation.** The three-record pre-command comparator captured no `E000:0180` invocation before command arming in either clean 1.05 run. A known-normal same-service comparator is therefore unavailable. The scratch object already present at first Ok is `00 00 00 00 24 16 32 46`, but no bounded evidence proves that it was consumed by the same `0180/2730` service, so it is not promoted as a normal invocation comparator.

## Field-7 source provenance

**Proven static boundary.** `33F7 MOV DI,022E` selects the fixed work buffer. `34A7-34B8` writes only through `DS:0235`: words at `022E`, `0230`, and `0234`, while explicitly skipping `0232-0233`. It reaches `34BD CALL 391D` with `DI=0236`. No instruction in this local producer writes `DS:0236-023D`.

**FIELD7-INHERITED.** Field 7 is already present at `DS:023D` at the `E000:33F7 -> 34A7` work-buffer boundary. The first still-unresolved upstream producer is the earlier BASIC work-area initialization/clear path that established `DS:0236-023D=00` before this command. The report does not misidentify `01F7` as that producer; `01F7` is only the source-to-scratch copy.

## Independent writers of scratch field 7

The finite writer set relevant to the `2730` nonzero call tree is:

| Address | Instruction | Role | Normal `A=1` / `PRINT 1` count |
| --- | --- | --- | ---: |
| `0200` | `REP MOVSW` | expected full-object copy, including field 7 | `01F7_count=1` |
| `273D` | `AND BYTE [0096],7F` | clear sign before `2751` | `0 / 0` |
| `2748` | `OR BYTE [0096],80` | restore negative sign | `0 / 0` |
| `286D` | `OR BYTE [0096],80` | conditional numerical branch update | `0 / 0` |
| `03FD` | `MOV [DI],AL`, with `DI=0096` on that path | numerical status update | `0 / 0` |
| `1309` | `MOV [0096],DL` | cleanup zero writer | `0 / 0` |
| `1350` | `OR [0096],DL` | cleanup/status writer | `0 / 0` |

**Proven dynamic observation.** The expected copy executes once per `01F7` invocation. All six independent candidate counters are zero on the normal zero-field runs. The disposable nonzero CFG necessarily executes `273D`; its retained scratch field is `01` at post-`2730` and pre-`01E4`.

## 0374:0000-00FF checkpoint dumps

| Checkpoint | SHA-256 | Nonzero bytes | Object `008F-0096` |
| --- | --- | ---: | --- |
| D0 reset | `5341e6b2646979a70e57653007a1f310169421ec9bdd9f1a5648f75ade005af1` | 0 | `0000000000000000` |
| D1 first Ok | `8a38b2c9bcd2ebccab8bf57d9537532c773c0dedf213828dc2c79b97bcc5b659` | 139 | `0000000024163246` |
| D2 pre-copy | same as D1 | 139 | `0000000024163246` |
| D3 pre-2730 | `3babb5db3f7b4beaef81b0114ef52928a42d1baf4d05828d7d1f5f4b81356d18` | 135 | `0000000000000000` |
| D4 post-2730 | same as D3 | 135 | `0000000000000000` |
| D5 pre-01E4 | same as D3 | 135 | `0000000000000000` |

D0 raw page:

```text
0000: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
0010: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
0020: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
0030: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
0040: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
0050: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
0060: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
0070: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
0080: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
0090: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
00A0: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
00B0: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
00C0: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
00D0: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
00E0: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
00F0: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
```

D1 and D2 raw page (byte-identical):

```text
0000: CB CB CB CB CB CB CB CB CB CB CB CB CB CB CB CB
0010: CB CB CB CB CB CB CB CB CB CB CB CB CB CB CB CB
0020: CB CB CB CB CB CB CB CB CB CB CB CB CB CB CB CB
0030: CB CB CB CB CB CB CB CB CB CB CB CB CB CB CB CB
0040: CB CB CB CB CB CB CB CB CB CB CB CB CB CB CB CB
0050: CB CB CB CB CB CB CB CB CB CB CB CB CB CB CB CB
0060: CB CB CB CB CB CB CB CB CB CB CB CB CB CB 8A 2E
0070: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
0080: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
0090: 00 00 00 24 16 32 46 00 00 00 00 00 00 00 00 00
00A0: 00 00 00 12 49 29 46 00 00 00 00 00 00 00 00 00
00B0: 00 00 00 00 24 16 32 46 00 80 01 00 00 00 00 00
00C0: 00 00 00 70 02 00 00 00 00 00 00 00 00 60 03 00
00D0: 00 00 00 00 00 00 00 50 04 00 00 00 00 00 00 00
00E0: 00 40 05 00 00 00 00 00 00 00 00 30 06 00 00 00
00F0: 00 00 00 00 00 20 07 00 00 00 00 00 00 00 00 10
```

D3, D4 and D5 raw page (byte-identical):

```text
0000: CB CB CB CB CB CB CB CB CB CB CB CB CB CB CB CB
0010: CB CB CB CB CB CB CB CB CB CB CB CB CB CB CB CB
0020: CB CB CB CB CB CB CB CB CB CB CB CB CB CB CB CB
0030: CB CB CB CB CB CB CB CB CB CB CB CB CB CB CB CB
0040: CB CB CB CB CB CB CB CB CB CB CB CB CB CB CB CB
0050: CB CB CB CB CB CB CB CB CB CB CB CB CB CB CB CB
0060: CB CB CB CB CB CB CB CB CB CB CB CB CB CB 8A 2E
0070: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
0080: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
0090: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
00A0: 00 00 00 12 49 29 46 00 00 00 00 00 00 00 00 00
00B0: 00 00 00 00 24 16 32 46 00 80 01 00 00 00 00 00
00C0: 00 00 00 70 02 00 00 00 00 00 00 00 00 60 03 00
00D0: 00 00 00 00 00 00 00 50 04 00 00 00 00 00 00 00
00E0: 00 40 05 00 00 00 00 00 00 00 00 30 06 00 00 00
00F0: 00 00 00 00 00 20 07 00 00 00 00 00 00 00 00 10
```

The D2-to-D3 change is the normal eight-byte object copy plus the `2730` auxiliary clear. D3=D4=D5 proves that the observed zero path does not change the page after its gate.

## Disposable non-zero intervention

- **Causal intervention result.** Source SHA before the temporary line: `093edaf4bca4371d41839f8871f572c71271a6e9`.
- Disposable worker SHA-256: `3dc53efdf531f17bb496a8df26a0d51298c0ececab21d1cb0b6e6fc076079aff`.
- Patch SHA-256: `5257a993786b359dcad04a8c95ff224db0a7277d3d63284db9862c1b68ede6cd`.
- Exact temporary operation: after normal O2 capture and before executing `E000:2730`, write `01` to `(DS_BASE+0096h)`.
- The change was uncommitted, removed after the runs, and the normal worker was rebuilt.

| Command | `2751` | `0338` | `03C2` | `0191` | `01E4` | second Ok | Deterministic result |
| --- | ---: | ---: | ---: | ---: | ---: | --- | --- |
| `A=1` | 1 | 1 | 1 | 1 | 1 | no by frame 1100 | same `34C0:0005` zero-target path |
| `PRINT 1` | 1 | 1 | 1 | 1 | 1 | no by frame 1100 | same `34C0:0005` zero-target path |

The post-`2730` scratch object is `0000000000000001` for both runs, proving the forced byte survived the nonzero path.

## Disposable-probe causal interpretation

**PROBE-SAME-FAILURE, override verified.** The intervention changed the consumed branch and activated all three dormant routines, but the nonzero branch returned to `0180`, reached `01E4`, and transferred to the same target. Therefore field 7 controls substantive numerical work inside `2730`; it does not select an escape from the terminal continuation path in this configuration. The intervention does not prove zero incorrect. It instead rejects field 7 as sufficient to explain the terminal control transfer.

## Boot-only E4 closure

**CLOSED — SOURCE RECORD ABSENT.** The complete `19E3:C7EB` boot-only installer list has 42 records over 168 bytes. No record installs `1040:0AC3`; this installer did not skip a present `0AC3` record. `1CC5:C6BB` is outside the executing boot-only configuration and remains out of M74r scope.

## Pre-populated continuation hypothesis disposition

**NOT PRIMARY.** Negative evidence is: all five sampled windows are zero, the below-free band is largely zero, work-area pages have no direct `34C0` anchor, and measured mapper resource selection matches the documented low-memory path. M74r adds that both zero and nonzero `2730` processing return to the same inherited far frame, but it does not provide positive evidence that `34C0:0005` must have been pre-populated.

## Hypothesis table

| Hypothesis | Status after M74r | Evidence |
| --- | --- | --- |
| Pre-populated continuation | DOWNGRADED / NOT PRIMARY | no positive measured contents or owner |
| Mapping defect | DOWNGRADED / rejected at measured resource selection | M74p mapping comparison |
| `0374:0096` is a standalone global gate | REJECTED | it is object field 7 |
| Object field 7 selects numerical work/no-work | PROVEN | exact `TEST/JZ`; disposable branch activation |
| Object field 7 selects escape versus `01E4` | REJECTED | both branches return to `0191` and reach `01E4` |
| Zero is architecturally incorrect | UNPROVEN; weakened as causal explanation | zero is a numerical zero representation; forcing nonzero preserves failure |
| Zero path consumes a fallback frame while nonzero escapes | REJECTED | NONZERO-RETURNS |
| Boot-only installer skipped a present `0AC3` record | REJECTED | complete list lacks the record |

## First incorrect emulator-produced state

**None exposed.** The diagnostic intervention changed guest state deliberately and is not evidence of an emulator-produced error. No independently justified expected field value differs from the normal worker's value.

## Root cause status

**UNRESOLVED.** M74r sharply removes the `2730` zero/no-op branch as the explanation for entering `01E4`. Both field classes return through the same continuation contract. The exact first incorrect emulator-produced architectural state remains unidentified.

## Production fix

None. No production source was changed and no force, special case, or workaround was retained.

## Validation

Process-local Git isolation was `GIT_CONFIG_GLOBAL=/dev/null GIT_CONFIG_SYSTEM=/dev/null`; no persistent Git configuration was modified.

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
| non-external CTest partitions `1-30`, `31-45`, `46-60`, `61-80`, remaining named tests | 92 PASS, 2 FAIL, 5 SKIP |
| `cmake --preset mingw-cross` | PASS, exit 0 |
| `CCACHE_DISABLE=1 cmake --build --preset mingw-cross -j2` | PASS, exit 0, no work required |

The two failures are the pre-existing protected-deletion pair `vaeg_upd9002_protected_deletion` and its selftest. Both report expected `cpu/upd9002/upd9002_ops.mcr` SHA-256 `dbfcc5b3ce7d3f0b4df493cd494b7fe297aa932e231904ddeb4b59411cd73183`, actual `73c75f7a82706487b51a66e30718d6daef21caa9f73458cd3d538a059fe4d089`. The five dependency tests `test_suffix.sh`, three generated-compress tests, and `test_files.sh` are SKIP, not PASS.

The exact full-suite commands were `ctest --test-dir build/linux-release -LE external -I 1,30 -j4 --output-on-failure` and the same command for ranges `31,45`, `46,60`, and `61,80`, followed by `-R '^(vaeg_m75_transfer_info_compiled|test_)' -j4`. Partitioning changes only host scheduling/containment, not guest-semantic criteria.

## Runtime identities

Final normal rows use source `46be1071d268f38e95fd56f88a41f836d07ab363`, worker `621fca...`, runner `1e1aa...`, model VA, frame bound 1100, command-local arm command 3, reachability enabled, and allocation capture enabled. First Ok is frame 720; injection is frame 840. Both required service events occur before the deterministic bound. No external host containment preempted a run; exit 1 is the requested guest-frame-bound result and is not interpreted as a host timeout.

The disabled equivalence run reaches first Ok at frame 720 and has TVRAM SHA-256 `c6ff29f6852e02e822ce3ea628817a0fbe15bbb832701d734b3d269ac43044b5`, matching the accepted reference. Enabled rows reproduce the established wrapper vectors and far frame.

## Remaining gaps

1. The exact earlier BASIC initialization instruction or bulk-clear operation that first establishes `DS:0236-023D=00` has not been identified; the proven boundary is `E000:33F7 -> 34A7`.
2. No pre-command known-normal invocation of the same `0180/2730` service occurred in the bounded three-record comparator, so there is no same-service normal object/branch pair.
3. M74r proves that both `2730` field classes return to the same continuation; it does not independently establish the intended ownership contract for the eventual far target.
4. Exact DX offsets for the four non-`34BD` wrapper families remain unmeasured and low priority.

## Changed files

- `cpu/upd9002/upd9002_trace.c`: bounded source/scratch/page snapshots, bounded startup comparator, post-`2730` return capture, and finite relevant field-writer counters.
- `docs/agents/reports/m74r_service_gate.md`: this evidence report.

## Worktree status

The final tracked changes are the two bounded diagnostic commits plus this report commit. Pre-existing `cpu/upd9002/upd9002_trace.c.orig`, `cpu/upd9002/upd9002_trace.c.rej.orig`, and `tools/__pycache__/` remain untracked and untouched. No disposable force remains in the tracked or working source.

## Hosted CI status

NOT RUN.

## G74 status

**NOT APPROVED.** M74r does not self-approve G74.
