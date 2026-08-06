# M74 VA1 N88 BASIC V3 command-hang investigation

## Status and fixed identities

M74 remains a human-review candidate. G74 is not approved by this report.
No production correction is claimed.

| Item | Value |
| --- | --- |
| Branch | `topic/m74-va1-basic-command-hang` |
| Starting SHA | `61d142b5d4a20e07675d669f8c0be21facb0be3d` |
| Approved G73 predecessor | `766a132ff6d66e335fe9bb1d0082d777a4a8fe14` |
| Task authority SHA | `976c33956d585560223561bf6694c6a26ee8cedd810cffed1b60a59189014ea1` |
| Evaluated SHA | e49641b5c78bdacbec220bec74eb5a1e72cce014 |
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

The first architecturally invalid transition is this `RETF` to
`34C0:0005`. The later `9A` is an immediate far pointer and does not read
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
first proven-bad: E000:01E4 RETF consumes 0005:34C0
producer proven: E000:3922 PUSH DX and E000:34BD CALL 391D create the words
producer still unresolved: the higher-level reason the 34C0 continuation is
not populated or should not be entered
```

## Causal experiments

| Hypothesis | Controlled experiment | Result | Conclusion |
| --- | --- | --- | --- |
| optimized direct word access corrupts the frame | disable the optimized direct-word path in a disposable diagnostic build | same `34C0:0005` transition | rejected as immediate cause |
