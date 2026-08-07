# M74 VA1 N88 BASIC V3 command-hang investigation

## Status and fixed identities

M74 remains a human-review candidate. G74 is not approved by this report.
No production correction is claimed.

| Item | Value |
| --- | --- |
| Branch | `topic/m74-va1-basic-command-hang` |
| Starting SHA | `87629924538836fd2ab29c4c8337f6bd41aac523` |
| Approved G73 predecessor | `766a132ff6d66e335fe9bb1d0082d777a4a8fe14` |
| Task authority SHA | `976c33956d585560223561bf6694c6a26ee8cedd810cffed1b60a59189014ea1` |
| Evaluated SHA | `db6534d56cf72a7ed4c911f5bca6a39b24e0dfb1` before the report commit |
| Diagnostic commit SHA | `db6534d56cf72a7ed4c911f5bca6a39b24e0dfb1` |
| Production-fix SHA | None |

The worktree already contained uncommitted M74 diagnostic changes when this
continuation started. They were preserved. No ROM, disk image, or private
integration asset was added to the repository.

## Established evidence

The maintainer supplied this VA1 observation:

```text
348856 bytes free
Ok
PAINT(0,0),3
Ok
A=1
```

`PAINT(0,0),3` returns to `Ok`. The current clean BASIC matrix also confirms
that `A%=1`, `A!=1`, `A#=1`, and `A$=""` return to `Ok`, while unsuffixed
assignments such as `A=1`, `B=1`, `A=0`, and `A=-1` do not return to `Ok` in
the bounded runs. This keeps the failure specific to the unsuffixed/default
assignment path rather than generic graphics, FDC, or all scalar storage.

The CPU remains active. It does not enter a normal HALT wait. It eventually
executes a zero-filled region after an invalid far return and later reaches
the observed bytes around `34C0:4D6E`.

## Default-type behavior matrix

The following results are from clean headless VA1 sessions. A blank result
means that the syntax or the follow-up part of the compound test did not
reach a stable prompt in the available bounded run; it is not treated as a
success.

| Command | Echo | `Ok` after command | Result |
| --- | --- | --- | --- |
| `PAINT(0,0),3` | yes | yes | known-good graphics/direct-mode path |
| `PRINT 1` | yes | no in this run | incomplete literal path |
| `? 1` | yes | yes | prints `1`, then `Ok` |
| `A=1` | yes | no | failing default scalar path |
| `LET A=1` | yes | no | same assignment family does not complete |
| `A%=1` | yes | yes | integer scalar succeeds |
| `A!=1` | yes | yes | explicit single scalar succeeds |
| `A#=1` | yes | yes | double scalar succeeds |
| `A$=""` | yes | yes | string scalar succeeds |
| `B=1` | yes | no | not specific to the letter `A` |
| `A=0` | yes | no | not specific to the value `1` |
| `A=-1` | yes | no | negative literal also fails |
| `DEFINT A-Z` then `A=1` | yes | unresolved | compound test did not reach the second stable result |
| program assignment | yes | unresolved | program-entry test did not reach `RUN` completion |

The available VA2/VA3 runtime comparison did not reach the equivalent parser
checkpoint in its bounded window. Therefore no VA2/VA3 success or failure is
claimed for this exact path. The matrix establishes a common/default-type
assignment boundary, but not yet the responsible instruction or data value.

## Rejected and downgraded hypotheses

These hypotheses are not being reopened without contradictory evidence:

* Generic FDC wait: downgraded because `PAINT(0,0),3` completes.
* Generic graphics/LIO failure: downgraded for the same reason.
* FPU instruction execution failure: rejected on the failing `A=1` interval;
  no `D8h`--`DFh` opcode executes.
* BCD instruction failure: rejected on the failing interval; no `27h`, `2Fh`,
  `37h`, `3Fh`, `0F 20h`, `0F 22h`, or `0F 26h` opcode executes.
* Optimized direct word access: rejected as the immediate cause; disabling
  that diagnostic-build path still reaches the same bad far return.
* ROM-bank interpretation at `0152h/0153h`: rejected by the earlier bounded
  bank comparison.
* `RETF` implementation: rejected below by direct source and frame evidence.

The narrower FPU-capability hypothesis is not supported: no emulator FPU or
coprocessor-presence flag/test was found in the relevant source, and no such
test is present in the captured failing interval. It is therefore not a
production candidate.

## Bounded diagnostic configuration

The existing M74 trace additions are opt-in and bounded by:

```text
VAEG_M74_CPU_TRACE_LIMIT
VAEG_M74_CPU_TRACE_COMMAND
```

They retain 4096 instruction records, a bounded interrupt ring, stack windows,
control-transfer records, and watched writes for the continuation and target
regions. Headless input arms the trace immediately before the selected
command. The unrelated post-segment stack watcher was removed after it was
shown to produce only noise.

The principal failing trace was bounded at 1,127,000 instructions and has
SHA-256:

```text
0cf174f818bf2fec5d3d0ae2444578ea1d61d3e94ec679a0fe4f848fda8f2fe0
```

The successful explicit-single trace was bounded at 1,500,000 instructions
and has SHA-256:

```text
984f4e134090019b99747cb185282eab3906da709c4433216dd7cecd1be22cf9
```

The diagnostic worker used for the final local checks has SHA-256:

```text
338b052d2141977e96d1d94afefd154e54026b9439003a1af4d7ea58ce0fcdda
```

## Entry provenance for `34C0:4D6E`

The invalid sequence is not entered by an immediate far call. The complete
observed chain is:

```text
E000:34BA  CALL near 3705       ; pushes return IP 34BD
E000:34BD  CALL near 391D       ; pushes return IP 34C0
E000:3922  PUSH DX              ; DX = 0005
E000:3923  PUSH SI              ; SI = 002A
E000:3983  RET                   ; consumes 002A, enters E000:002A
E000:002A  JMP E000:0180
E000:01E4  RETF                  ; consumes IP=0005, CS=34C0
34C0:0005  zero-filled execution
34C0:4D6D  F0
34C0:4D6E  9A 09 E3 19 98       ; CALL FAR 9819:E309
9819:E309  zero-filled execution
```

The critical trace record is:

```text
m74-control-transfer seq=1113574 from=e000:01e4 to=34c0:0005
  ss=7fe0 sp=01f4 stack_phys=7fff4
  stack_before=0500c0342a00fd33
  post_ss=7fe0 post_sp=01f8 post_cs_base=34c00
```

The consumed words are therefore, in little-endian order:

```text
SS:SP       bytes        value
7FE0:01F4   05 00        IP = 0005
7FE0:01F6   C0 34        CS = 34C0
```

The first observed semantically unexpected continuation is this `RETF` to
`34C0:0005`; the `RETF` itself is architecturally valid. The later `9A` is an immediate far pointer and does not read
`9819:E309` from the stack. In the exact trace it is preceded by `F0` at
`34C0:4D6D`; the `9A` decoder then transfers to `9819:E309`. This later
transfer is a consequence of executing the unconstructed page, not proof of
a faulty far-call decoder.

## Stack-frame provenance

The surrounding watched writes show the frame construction:

| Sequence | Guest instruction | Physical stack write | Value |
| ---: | --- | ---: | --- |
| 1110689 | `E000:34BD CALL 391D` | `7FFF6` | return IP `34C0` |
| 1110732 | `E000:3922 PUSH DX` | `7FFF4` | `0005` |
| 1110733 | `E000:3923 PUSH SI` | `7FFF2` | `002A` |
| 1113539 | `E000:3983 RET` | consumes `7FFF2` | `002A` |
| 1113574 | `E000:01E4 RETF` | consumes `7FFF4/7FFF6` | `0005:34C0` |

`DX=0005` is already present before the final wrapper sequence, and `SI=002A`
is loaded by `MOV SI,002A` at `E000:33BC`. These are ordinary guest
instructions. The current evidence proves the producer instructions but not
the intended higher-level contract that should have populated or entered the
`34C0` continuation. The original `PUSH DX`/`PUSH SI` pair must not be
mistaken for CPU stack corruption.

## `RETF` semantic and source audit

The production implementation is:

```text
cpu/upd9002/upd9002_mn.c:2369-2375
  REGPOP0(UPD9002_IP)
  REGPOP0(UPD9002_CS)
  CS_BASE = SEGSELECT(UPD9002_CS)
```

and `REGPOP0` is defined in `cpu/upd9002/upd9002_ops.mcr` as a canonical
mapped word read followed by `SP += 2`. The trace matches this exactly:
`SS` stays `7FE0`, `SP` advances from `01F4` to `01F8`, `CS_BASE` becomes
`34C00`, and the target is `34C0:0005`. No `MOV SS,DX` occurs here; the
instruction sequence at `E000:01DF` is `MOV DX,ES`, `MOV DS,DX`, `POP ES`,
`RETF`. The earlier `MOV SS` interpretation is retracted.

No CPU `RETF` or far-call production change is justified.

## Memory classification of the invalid target

The nominal pre-mapping addresses are:

```text
34C0:4D6E -> 3996Eh
9819:E309 -> A6499h
```

The `34C0` page is ordinary VA RAM below the ROM windows. The complete
monitored continuation page remains zero at the loader boundary and at the
failure boundary. No canonical memory write, segmented write, DMA copy, or
watched direct-copy write created `F0 9A 09 E3 19 98` during the failing
command. The target page at `A6499h` is also zero-filled in the captured
execution.

Instruction fetches use the canonical `upd9002_memoryread()` route. The
watched bytes are therefore not evidence of an alternate flat instruction
fetch path, and no VA1 mapper correction is proven.

The important distinction is:

```text
34C0:4D6E contains the bytes before the late fetch
and is not written by the failing command;
the invalidity is that execution entered an unconstructed 34C0 page.
```

## A=1 versus A!=1 differential disposition

The screen matrix proves the behavioral difference: `A=1` does not return to
`Ok`, while `A!=1` does. The explicit-single trace reaches a successful
completion path in its 1,500,000-instruction bound and does not reach the
`E000:34BD`/`E000:3922`/`E000:01E4` bad chain in that bound. Because the two
paths have different BASIC dispatch lengths and the bounded ring retains only
the tail, an exact last-common architectural instruction and first differing
register are not yet proven. The report therefore does not claim that the
default-type table, a variable descriptor, or a particular conversion value
is the root cause.

The current strongest boundary is:

```text
last proven-good: the guest's normal explicit-type completion path
first observed bad continuation: E000:01E4 RETF consumes 0005:34C0
producer proven: E000:3922 PUSH DX and E000:34BD CALL 391D create the words
producer still unresolved: the higher-level reason the 34C0 continuation is
not populated or should not be entered
```

## Causal experiments

| Hypothesis | Controlled experiment | Result | Conclusion |
| --- | --- | --- | --- |
| optimized direct word access corrupts the frame | disable the optimized direct-word path in a disposable diagnostic build | same `34C0:0005` transition | rejected as immediate cause |
| `RETF` pops the wrong order or segment | compare source macro, source implementation, and complete frame trace | order, SP, SS, and CS base all match | rejected |
| FPU opcode is failing | record every `D8h`--`DFh` opcode on the failing interval | none observed | rejected |
| BCD opcode is failing | record the complete BCD opcode set on the failing interval | none observed | rejected |
| continuation bytes are overwritten during `A=1` | watch source and target physical ranges | no writes; bytes remain zero | not a write-corruption defect |
| FPU-presence flag selects the bad path | search and trace capability tests between the paths | no emulator capability test or flag source found | not supported |

These experiments are diagnostic only. No production behavior was changed.

## First incorrect state and root-cause status

The first observed bad continuation is execution of a far return frame that
points at `34C0:0005`, followed by execution of an unconstructed zero page.
This is not yet a proven CPU architectural error: the CPU implementation
consumes the frame correctly.

The causal chain is currently bounded as:

```text
A=1 selects the unsuffixed/default assignment path
        -> guest execution reaches the wrapper sequence
        -> E000:34BD creates return IP 34C0
        -> E000:3922 supplies DX=0005 above that return word
        -> E000:01E4 correctly executes RETF to 34C0:0005
        -> 34C0 continuation memory is zero/unconstructed
        -> sequential zero execution reaches F0 9A ...
        -> immediate CALL FAR transfers to 9819:E309
        -> zero-filled execution follows
```

The first line after the assignment boundary is still a guest-state or
runtime-image contract question: whether the `34C0` continuation should have
been populated, whether the wrapper should have been entered, or whether a
prior default-type dispatch value selected the wrong path. No safe production
fix can be selected from the present evidence.

## Regression and production correction

No production regression test or production correction was added because the
first incorrect higher-level producer is not proven. The bounded M74 CPU trace
is the diagnostic regression artifact. Adding a `RETF`, `F0`, `9A`, mapping,
FPU, or BCD correction would be speculative and is explicitly out of scope.

## Validation

| Command | Result |
| --- | --- |
| `cmake --build /private/tmp/m74-trace-build --target vaeg_sdl2 -j 4` | passed; linker emitted existing duplicate-library/alignment warnings |
| `vaeg --selftest` | all selftests passed; CoreAudio emitted an existing host warning |
| `vaeg --upd9002-m68-segmented-memory` | passed |
| `vaeg --idp-m69-status-composition` | passed |
| `vaeg --upd9002-m70-prefix-string` | passed |
| `git diff --check` | passed |
| `ctest -L romless` with isolated Git environment | 68 passed, 2 failed, 1 skipped; only protected-deletion tests failed because the pre-existing M74 branch changes `cpu/upd9002/upd9002_ops.mcr`; no new diagnostic file was the cause |

The CTest failures are not presented as passing. Several repository checks
invoke Git without the isolated configuration needed by this sandbox, and the
protected-artifact checks intentionally reject the trace-only edits to CPU
sources. The focused production build and uPD9002 M68--M70 checks pass.

## Changed-file and scope audit

The diagnostic-only working changes are limited to:

```text
cpu/upd9002/memory.c
cpu/upd9002/upd9002_core.c
cpu/upd9002/upd9002_ops.mcr
cpu/upd9002/upd9002_trace.c
cpu/upd9002/upd9002_trace.h
sdl2/headless_input.c
sdl2/np2.c
docs/agents/reports/m74_va1_basic_command_hang.md
```

They add opt-in, bounded instruction/control-transfer/stack/memory-write
diagnostics and this report. They do not modify ROMs, disk images, CPU
instruction semantics, FDC behavior, timing, memory mapping policy, state
serialization, or later milestone work. The current continuation does not
modify `cpu/upd9002/upd9002_ops.mcr`; the protected-deletion failure is an\existing branch-state condition from earlier M74 work.

## Remaining risks and next boundary

The remaining ranked hypotheses are:

1. A guest-side default-type/variable-dispatch value selects the wrapper path
   without constructing the continuation page.
2. The PC-Engine runtime image or its loader contract expects a RAM/table or
   continuation content not supplied by the current integration setup.
3. A VA1-specific resident/BIOS state transition enters the wrapper with the
   wrong continuation contract.

The next investigation must compare the full semantic event stream for
`A=1` and `A!=1` before `E000:34BD`, including default-type state, variable
lookup/allocation, and continuation ownership. It should also verify the
runtime image/loader contract for `34C0` without changing CPU control-flow
semantics.


## E000:391D static contract

The complete routine from `E000:391D` through `E000:3988` is a parser/helper
routine, not a CPU far-return primitive. Its relevant normal success path is:

```text
391D  CALL FAR 1040:0AC3
3922  PUSH DX
3923  PUSH SI
3924  ... parser/helper loop ...
3973  PUSH SI
3976  CALL 383A
3979  PUSH DS
397A  JC 3984
397C  POP CX
397D  POP BX
397E  POP AX
397F  CLC
3980  MOV AX,0081
3983  RET
```

On the observed `A=1` path, `SI=002A` is pushed by `3923`, later survives
as the word consumed by the intermediate near `RET`, and sends execution to
`E000:002A`. `DX=0005` is pushed above the near-CALL return word and survives
as the low word consumed by the final `RETF`. The near call at `E000:34BD`
pushes `34C0`; that word remains below `0005` until `E000:01E4` consumes it
as the final CS. The observed stack contract is therefore:

```text
before CALL 391D:       caller stack
CALL 391D:              [34C0]
PUSH DX:                [0005, 34C0]
PUSH SI:                [002A, 0005, 34C0]
intermediate RET:       consumes 002A, leaves [0005, 34C0]
final RETF:             IP=0005, CS=34C0
```

This is a deliberate stack construction in the observed guest code. It does
not prove that the resulting continuation is validly populated, nor that the
`A=1` caller should have selected this success path.

## E000:391D call-site cross-reference

A real instruction-boundary scan of all eight relevant ROM banks found five
near-call references to `E000:391D`. No data-only byte matches are included.

| Caller | CALL bytes | post-CALL IP | observed setup/context | predicted thunk target | target status |
| --- | --- | ---: | --- | --- | --- |
| `E000:34BD` | `E8 5D 04` | `34C0` | `CALL 3705`; parser/error continuation | `34C0:DX` | observed `34C0:0005`, page zero |
| `E000:43B2` | `E8 68 F5` | `43B5` | after `CALL 3ED2`; error retry path | `43B5:DX` | not dynamically exercised |
| `E000:49F9` | `E8 21 EF` | `49FC` | after `CALL 3ED2`; error retry path | `49FC:DX` | not dynamically exercised |
| `E000:75A8` | `E8 72 C3` | `75AB` | after `CALL 3ED2`; error retry path | `75AB:DX` | not dynamically exercised |
| `E000:7F2A` | `E8 F0 B9` | `7F2D` | after `CALL 3753`; parser path | `7F2D:DX` | not dynamically exercised |

The static scan supports a generic parser/error thunk interpretation. It does
not support treating `34BD` as a BASIC-specific hard-coded entry.

## Successful thunk invocations

The trace-only entry hook recorded six invocations across six bounded command
runs: two `A=1` runs, two `A!=1` runs, one `? 1` run, and one additional
`A=1` writer-watch run. Every invocation entered at `E000:34BD`; no alternate
static callsite was dynamically observed.

The fully instrumented `A=1` invocation was:

```text
m74-thunk-entry seq=1110791 caller=e000:34bd return_ip=34c0
  dx=0005 si=002a ss=7fe0 sp=01f6 stack_phys=7fff6
  stack=c0342a00fd330000
m74-thunk-stack-step seq=1110793 ip=3922 sp=01f6 post_sp=01f4
  post=e000:3923 stack_after=0500c0342a00fd33
m74-thunk-stack-step seq=1110794 ip=3923 sp=01f4 post_sp=01f2
  post=e000:3924 stack_after=2a000500c0342a00
m74-thunk-stack-step seq=1113600 ip=3983 sp=01f2 post_sp=01f4
  post=e000:002a stack_before=2a000500c0342a00
m74-thunk-helper-return seq=1113600 path=success post=e000:002a
m74-thunk-retf seq=1113635 entry_seq=1110791 from=e000:01e4 to=34c0:0005
  target_phys=34c05 target_bytes=00000000000000000000000000000000
```

The omitted intermediate parser pushes are present in the same trace: at
`3973` the stack includes `0001,002A,0005,34C0`, and at `3979` it includes
`E72A,2B00,0100,2A00`; they are consumed before the final trampoline return.
The exact first producer of the intermediate parser-local word is not needed
to establish the final `DX`/return-IP contract and remains outside the proven
root-cause boundary.

The `A!=1` comparison entered the same thunk with the same `DX=0005` and
`SI=002A`, but returned through the failure path:

```text
m74-thunk-helper-return seq=1198929 path=failure post=e000:34c0
  post_sp=01f8 flags=0245
```

It did not reach `3973`, `3983`, or the final `RETF`. This is evidence that
the divergence occurs in the helper/parser success decision before the bad
continuation, not in the CPU's final far-return operation.

## 34C0 lifetime snapshots

The lifecycle watcher sampled the complete logical 64 KiB segment and separately
watched the translated continuation page. The exact available samples are:

| Stage | Trace label | Segment FNV-1a64 | nonzero bytes | `34C0:0005` |
| --- | --- | --- | ---: | --- |
| T0 reset | `reset` | `eb05052ea5b62325` | 0 | all zero |
| T1 after boot/BASIC entry, before first command | `headless-before-command` | `13cbad5468436587` | 85 | all zero |
| T2 immediately before BASIC launch | not separately emitted | unavailable | unavailable | not separately emitted |
| T3 after first stable prompt wait | `headless-before-wait` | `f9df67237b51f885` | 88 | all zero |
| T4 immediately before `A=1` | `before-391d` | `f9df67237b51f885` | 88 | all zero |
| T5 immediately before final `RETF` | `before-01e4` | `f9df67237b51f885` | 88 | all zero |
| T6 after entering `34C0:0005` | post-RETF lifecycle sample | `f9df67237b51f885` | 88 | all zero |
| T7 after late zero-filled execution | final `headless-before-wait` | `f9df67237b51f885` | 88 | all zero |

T2 was not separately timestamped by the existing headless lifecycle hook;
T1 is the first post-boot/pre-command sample and the subsequent samples are
explicitly bounded. This is recorded as an instrumentation gap, not filled
with an inferred digest. The `A!=1` run likewise retained a zero continuation
page; its segment digest at the final pre-command boundary was
`682a5dd0c2e806b5` with 92 nonzero bytes.

## 34C0 nonzero-range map

At T1 the nonzero ranges in logical segment `34C0` were:

```text
4d5c-4d60, 4d68-4d6b, 4d6d-4d79, 4d7b-4d7d,
b3a4-b3a5, b3a7-b3a8, b3aa, b3ac-b3ae, b3b0-b3b1,
b3b4-b3b5, b3b7-b3bc, b3be, b3c0, b3c2-b3c4,
b3c6-b3ca, b3cc, b3ce-b3d0, b3d2-b3d4, b3d6-b3da,
b3de-b3df, b3e2, b3e4, b3e6-b3e7, b3e9-b3ea,
b3ec, b3ee, b3f1-b3f5, b3f7-b3fb.
```

The later sample also contains `516a-516b,516d`. The bytes at `4D6D` are
`F0 9A 09 E3 19 98`, but offset `0005` is not in any nonzero range. Thus
`34C0:0005` and `34C0:4D6D` are distinct evidence: one is an unpopulated
continuation page and the other is pre-existing work-area content encountered
only after the bad entry.

## 34C0 first-writer provenance

A bounded instruction-boundary watch and host-write watch covered the physical
range for `34C0:0000`--`34C0:00FF` from reset through the available boot
lifecycle. It recorded no write to physical `34C00`--`34CFF`, including the
continuation offset `34C05`, from reset through the failure. The target range
therefore remains zero rather than being cleared during `A=1`.

The same watcher recorded writes in the distinct physical range corresponding
to the later `4D6D` bytes, from guest BIOS/work-area instructions including
`F000` and `E000` paths. Those writes explain the nonzero late bytes but do
not populate `34C0:0005`. No DMA, disk transfer, host initialization, or
bulk-copy writer to the `34C0` page was observed.

## Continuation ownership

No production source, loader table, overlay descriptor, resident-module table,
or ROM-to-RAM copy descriptor in the repository identifies `34C0` as an owned
runtime module. The PC-Engine boot documentation identifies other staged
runtime locations and explicitly leaves the complete `PCENGINE.SYS` load/
relocation path unresolved; it does not establish `34C0` ownership. The current
answer is therefore `unknown/unresolved`, not “BASIC core” and not “CPU RAM
that should be pre-filled.”

## Expected-image/source search

The local reference media was searched for address/table evidence and for a
candidate block whose loader destination is `34C0`. No source descriptor or
verified expected continuation image was found. Private reference asset names,
paths, and digests are intentionally omitted from this repository report under
the integration-asset policy. No ROM or disk bytes were copied into Git.

No known-good repository revision with a reproducible VA1 `A=1` -> `Ok` result
was identified. Consequently no historical build comparison or blind bisect
was performed.

## A=1 versus A!=1 pre-thunk divergence

The two traces converge at the thunk entry:

```text
A=1:   E000:34BD -> 391D, DX=0005, SI=002A -> helper success -> 3983 -> 002A -> RETF
A!=1:  E000:34BD -> 391D, DX=0005, SI=002A -> helper failure -> E000:34C0
```

The first proven semantic difference is the helper/parser success decision
inside `E000:391D`, before the `3973`/`3976` success-only path. The current
bounded traces do not identify the exact table index, variable descriptor, or
source value that makes that decision differ. Therefore default-type dispatch
remains a ranked hypothesis, not a proven producer defect.

## DEFINT/DEFSNG/DEFDBL results

The robust prompt-aware headless script was used so a stale initial `Ok` could
not be mistaken for completion. `DEFINT A-Z` was echoed, but no cleared-and-
reappeared `Ok` prompt was observed within the bounded wait; `A=1` was not
injected after it. The decoded screen ended with:

```text
           88-                  v3.0
Copyright (C) 1987 NEC Corporation
336520 bytes free
Ok
DEFINT A-Z
```

The `DEFSNG A-Z` and `DEFDBL A-Z` runs likewise injected the declaration after
the initial prompt but did not produce a second verified prompt before the
bounded runs ended. No `A=1` result is claimed for either. These are unresolved
observations, not evidence that the declarations themselves are incorrect.

## Known-good revision comparison

No earlier revision was demonstrated to pass the required external-asset
criterion `VA1 N88 BASIC: A=1 returns to Ok`. The historical report and current
branch provide diagnostic evidence but not a deterministic known-good endpoint.
A source-level history comparison would therefore not distinguish an emulator
regression from an unchanged integration/runtime-image contract.

## Thunk hypothesis disposition

```text
E000:391D thunk hypothesis: PROVEN for the observed invocation contract.
```

The proof consists of the complete static stack path, six dynamic entries at
the same generic callsite, the successful `A=1` invocation showing
`CALL-return-IP=34C0`, `DX=0005`, and final `RETF 0005:34C0`, and the matching
`A!=1` failure path before the final `RETF`. The result proves that the RETF
transition is intentional guest stack construction and is architecturally
correct in this execution. It does not prove that `34C0:0005` is supposed to
contain executable continuation bytes or that `A=1` should select this path.

## Updated first incorrect operation

The first incorrect operation is not proven. The first observed bad
continuation is `E000:01E4 RETF -> 34C0:0005`; the earliest unresolved producer
boundary is the helper/parser success decision inside `E000:391D` or the
missing runtime initialization/loader operation that would own `34C0:0005`.
No production source operation can safely be named until one of those two
boundaries is resolved.

## Updated causal chain

The strongest evidence-supported chain is:

```text
A=1 selects a helper/parser success path
        -> E000:34BD CALL 391D intentionally contributes CS=34C0
        -> DX=0005 is intentionally retained as the final IP
        -> E000:01E4 correctly executes RETF to 34C0:0005
        -> 34C0:0005 is zero from reset through failure
        -> execution falls through unrelated pre-existing 34C0 data
        -> F0 9A ... performs its encoded immediate far CALL
        -> zero-filled execution follows at 9819:E309
```

The causal chain stops at the unresolved question whether the success-path
selection is wrong or the continuation owner/loader is missing. A speculative
CPU, RETF, LOCK, FPU, BCD, or mapper correction is not justified.

## Next production boundary

The next bounded task is to identify the helper/parser decision value and its
producer on the `A=1` versus `A!=1` paths, then independently identify the
owner and expected source of `34C0:0005`. Only a concrete failure at one of
those production operations may receive a focused regression or correction.
No regression and no production fix were added in this continuation.

## Diagnostic artifacts and source scope

The current trace-only continuation changes are in:

```text
cpu/upd9002/memory.c
cpu/upd9002/upd9002_core.c
cpu/upd9002/upd9002_trace.c
cpu/upd9002/upd9002_trace.h
sdl2/headless_input.c
sdl2/headless_input.h
```

They are disabled unless the M74 diagnostic controls are armed, do not alter
CPU behavior, and do not embed ROM/disk data. Diagnostic artifact hashes are:

```text
worker: 7e47c4da86a50e5013d481e186909e032bf37fd73632541730cffe390b5fdf98
A=1 stack trace: b4b9eb8633870670fce7c441d3593f3229db069e6028ddf8bde1b5061a2f8d97
A!=1 stack trace: 8f297ecc9809f0d8c59496e8d635c7e58aa896736759e4c0bec7d98b081708c6
writer-watch trace: 6ecabd583bd94fe91b155e1650b7f83cc3c6ad6cdcaea59e068eec2bfefce1df
DEFINT screen dump: 832c47900a1dac1d80287c8a9675fe08b6a6bd175b6030ff4c93f31dd74486cd
```

The raw traces and screen dump remain outside Git. The report records their
hashes only as stable evidence identifiers.

## Human G74 checklist

- [x] Reproduced the VA1 BASIC default-assignment failure.
- [x] Captured bounded control-flow and stack provenance.
- [x] Distinguished immediate far pointer `9A` from stack-derived `RETF`.
- [x] Rejected generic FDC, graphics/LIO, FPU-opcode, BCD, direct-word, ROM-bank, and `RETF` explanations as immediate causes.
- [x] Preserved the existing worktree changes and did not modify ROM/disk assets.
- [ ] Prove the higher-level producer/default-type or continuation ownership defect.
- [ ] Add a focused regression for that proven defect.
- [ ] Implement and validate the smallest production correction.
