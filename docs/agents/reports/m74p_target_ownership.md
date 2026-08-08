# M74p target ownership

## Identity

| Item | Value |
| --- | --- |
| Branch | `topic/m74-va1-basic-command-hang` |
| Starting SHA | `152b4de802d6ceee4c39bdb5f6e34ab3b9d14306` |
| Expanded-capture source SHA | `e6fee9c5075cab723fb8db5215f8f20ed033f9c1` |
| Worker SHA-256 | `0e3dcfbe9f8dcd3281bc6a7c12b814ad515e77785808e40583f041987502e678` |
| Runner SHA-256 | `a537d6382ffaf18fee73e391ea53972afda82f07f03703281a3c7079c7f5e4d7` |
| ROM SHA-256 | `656d81bc532ed0bb602d1eb7f03df27c74f841fb19c72ad07d47e79302ac814b` |
| Model/image contract | explicit `--model va`, trace-enabled Release worker, deterministic guest-frame bounds, 1.05 boot-only image |
| Production fix | None |
| G74 | NOT APPROVED |

The relevant reviewable commits before this report are `a1ab97e`, `483b7a8`,
`057d2dc`, and `e6fee9c`. No ROM, disk image, worker binary, or raw runtime log
is tracked.

## Closed wrapper mechanism

**Proven static fact and proven dynamic observation.** M74o remains closed.
`PRINT 1` reaches `3988 RET` with `002A,0005,34C0`; `A=1` reaches `3983 RET`
with the same three words. Both RET instructions consume `002A`, leaving
`0005,34C0` for `002A -> 0180 -> 01E4 RETF -> 34C0:0005`.

- H1, `3983` is the failure boundary: **REJECTED**.
- H2, `3985` implies ordinary caller completion: **REJECTED AS A GENERAL STATEMENT**.
- H3, `E000:34C0=0` is contradictory: **REJECTED**.
- H4, positive and negative non-escape paths normalize the same continuation frame: **PROVEN**.

## H6 deferral-loop correction

**Proven implementation fact.** The existing command-local `01E4` one-shot
already read the RETF frame, computed its target, and read 16 target bytes.
M74p extends that same fixed event and bounded read mechanism. It does not add
an interrupt hook, instruction trace, lifecycle watch, EA-write watch, or
mapping override.

The later maintainer direction to identify BASIC's actual free-memory bounds is
also implemented as a disabled-by-default bounded fixed-address/string-event
one-shot. It records the three operands used by the verified ROM calculation;
it does not scan memory or write guest state.

## Existing 01E4 seam capabilities

The expanded record contains eight stack words, the target-relative 256-byte
block, the `34C0:0000` base block, four other continuation segment windows,
`memmode_va`, and the production low-memory mapping classification. All memory
bytes use the same bounded diagnostic read used by the earlier target capture.

## Expanded-capture equivalence recheck

C1 and C2 used the same executable SHA-256
`0e3dcfbe9f8dcd3281bc6a7c12b814ad515e77785808e40583f041987502e678`.
Only runtime switches differed.

| Check | Result |
| --- | --- |
| C1, expanded fields disabled, first prompt frame | `720` |
| C1 TVRAM SHA-256 | `c6ff29f6852e02e822ce3ea628817a0fbe15bbb832701d734b3d269ac43044b5` |
| Required historical SHA-256 | `c6ff29f6852e02e822ce3ea628817a0fbe15bbb832701d734b3d269ac43044b5` |
| Byte-identical match | **PASS** |
| C2 `PRINT 1` counters | `34BD=1 391D=1 3983=0 3985=1 3988=1 34C0=0 002A=1 0180=1 01E4=1` |
| C2 `3988` words | `002A,0005,34C0` |
| C2 RETF target | `34C0:0005` |
| Gate | **PASS** |

C1 log SHA-256 is
`fc83c1249362de6d2ae1b85e4675132ab3cff6c8517a5185ad0ee9740b30337f`.
C2 log SHA-256 is
`8aaac1f7191ce118e757f2e96eafe4bbdf55a3168b856b4c617243f4c10f972b`.
No wall-clock result is part of the comparison.

## Extended PRINT 1 01E4 capture

**Proven dynamic observation.** First `Ok` was frame 720, command-local arm and
injection were frame 840, and the required `E000:01E4` event was reached before
the deterministic frame-1100 bound.

```text
CS:IP before RETF = E000:01E4
SS:SP              = 7FE0:01F4
word 0             = 0005  (RETF IP)
word 1             = 34C0  (RETF CS)
word 2             = 002A
word 3             = 33FD
word 4             = 0000
word 5             = 0000
word 6             = 0000
word 7             = 0000
```

Words 2-7 remain unclassified. The separate pre-`3988` capture is
`SS:SP=7FE0:01F2`, words `002A,0005,34C0`.

## Continuation target 256-byte dump

**Proven dynamic observation.** `34C0:0005` is nominal address `34C05h`.
The complete 256-byte target-relative block is zero. Its SHA-256 is:

```text
5341e6b2646979a70e57653007a1f310169421ec9bdd9f1a5648f75ade005af1
```

The first 16 bytes are sixteen `00` bytes. The separately captured
`34C0:0000` base block is also all zero and has the same SHA-256.

## Other continuation segment windows

**Proven dynamic observation.** These are windows, not exact targets; their
callers' DX values remain unproven.

| Window | Nominal base | SHA-256 | Class | Byte `0005` | Exact target offset |
| --- | ---: | --- | --- | --- | --- |
| `43B5:0000` | `43B50h` | `5341e6b2646979a70e57653007a1f310169421ec9bdd9f1a5648f75ade005af1` | all zero | `00` | unknown |
| `49FC:0000` | `49FC0h` | same | all zero | `00` | unknown |
| `75AB:0000` | `75AB0h` | same | all zero | `00` | unknown |
| `7F2D:0000` | `7F2D0h` | same | all zero | `00` | unknown |

No observed continuation window contains pre-existing guest-visible code or
data at the terminal checkpoint. This weakens the general pre-populated-window
model; it does not prove what each unknown-DX caller intends.

## Terminal stack context

**Proven dynamic observation.** `PRINT 1` and `A=1` have the same residual
RETF frame and same target. The six lower words do not prove their own
semantics or identify the installer.

## Mapper/bank state at RETF

**Proven implementation fact and dynamic observation.** The captured production
input is `memmode_va=1`. For `34C05h`, no VA BMS bank, system-memory bank,
ROM bank, or VA91 selector participates. The static read table selects the
low-address VA path.

The complete VAEG route is:

```text
upd9002_memoryread(memmode_va=1)
 -> upd9002_memoryread_va(34C05h)
 -> membyte_read[3]
 -> i286_rd_va
 -> upd9002_memoryread(memmode_va=0)
 -> mem[34C05h]
```

`CPU_ADRSMASK` is also applied, but `34C05h` is already within the 20-bit
range.

## VAEG mapping decision for 34C05h

**Proven dynamic observation.** Mapper path `va-low-direct` selects ordinary
main `mem` backing at effective offset `34C05h`. The returned block is zero.
No diagnostic override is present.

## Continuation-window disposition

**Inference entailed by the measured bytes.** All five measured windows are
zero. There is no byte evidence that these segment bases are generally
pre-populated under this configuration. Target ownership must instead be
resolved against BASIC's allocated-memory boundary, installer state, or a
mapping contract.

## BASIC free-memory boundary

**Proven static fact.** The verified ROM prints `" bytes free"` through
`E000:F7B0`. Immediately before the numeric formatter, `E000:2D72-2D92`
performs:

```text
2D72  A1 1A 00       MOV AX,[001A]
2D75  2B 06 10 00    SUB AX,[0010]
2D79  8B D0          MOV DX,AX
2D7B  C1 C2 04       ROL DX,4
2D7E  81 E2 0F 00    AND DX,000F
2D82  C1 E0 04       SHL AX,4
2D85  8B C8          MOV CX,AX
2D87  A1 04 00       MOV AX,[0004]
2D8A  F7 D8          NEG AX
2D8C  48             DEC AX
2D8D  03 C1          ADD AX,CX
2D8F  83 D2 00       ADC DX,0000
2D92  E8 CB 2C       CALL 5A60
2DA3  BB 5D 2E       MOV BX,2E5D
2DA6  E8 07 CA       CALL F7B0
```

The exact boundary form is:

```text
lower = ([DS:0010] << 4) + [DS:0004] + 1
upper = ([DS:001A] << 4) + 10000h       ; exclusive, after [001A]:FFFF
free  = upper - lower
```

**Proven dynamic observation.** In the final 1.05 run:

```text
DS             = 2E8A
[DS:0004]      = 1207
[DS:0010]      = 403A
[DS:001A]      = 7FE0
lower          = 415A8h
upper          = 8FE00h
free           = 4E858h = 321624
TVRAM text     = "321624 bytes free"
```

The arithmetic result and guest text match exactly. Static initialization sets
`[001A]` from the detected top-memory segment minus `20h` at `2CB2-2CB5` and
sets `[0010]` after the resident layout at `2C8F-2C92`. Static candidates that
write `[0004]` are `E000:804B` and `E000:E992`; the final fixed-address run did
not dynamically identify which relocated/handler path supplied `1207`, so that
specific writer remains unproven.

For the maintainer-supplied `348856` observation, with the same established
`[001A]=7FE0` top:

```text
348856 decimal = 552B8h
upper           = 8FE00h
lower           = 8FE00h - 552B8h = 3AB48h
check           = 8FE00h - 3AB48h = 552B8h = 348856
```

Thus `34C05h` is below the free lower bound in both cases: by `C9A3h` in the
final current run and by `5F43h` in the supplied 348856 case. **This materially
narrows H6:** `34C0:0005` is not inside BASIC's reported free-byte interval; it
lies in the lower resident/allocated side of BASIC memory. This does not yet
identify the owning module or prove that the zero contents are wrong.

## Unique C0 34 ROM occurrence

**Proven static fact.** `C0 34` occurs once, at file `277D8h` (bank 2 offset
`77D8h`). It crosses fields in a regular variable-length table: `C0` is the
payload of the preceding record and `34 31` begins the next ordered key. It is
category 3, another structured datum, not immediate `34C0h`, not a far-pointer
segment, and not an instruction. No causal consumer or relevant-session code
execution is established.

## ROM cross-references around bank 2:77D8

The bounded neighborhood has repeated ordered keys and one/two-byte payloads.
No reachable instruction boundary enters at `77D8`, and no consumer promotes
the crossing bytes into a `34C0` value. H9 is therefore **REJECTED AS A
CAUSAL LITERAL LEAD** for this occurrence.

## Far-pointer 0005 static candidates

**Proven static census.** Raw `05 00` occurs 564 times, but exact
`05 00 C0 34` is absent. The corresponding four-byte combinations with
`43B5`, `49FC`, `75AB`, and `7F2D` are also absent. No raw hit was promoted
without alignment, table structure, xref, and consumer evidence.

## DX=0005 provenance

**Proven bounded static boundary.** No instruction in `E000:34A7-34BD` writes
DX. `E000:34A7` is the first proven live-in boundary; its caller is
`E000:33FA`. The first still-unresolved upstream producer lies before the
`E000:33F7` predecessor merge. The unique ROM `MOV DX,0005` at `E000:0D4E`
is unrelated because its routine saves and restores DX before returning.
This is a DX-ABI/live-in boundary result, not a claim that DX is defective.

## A%=1 escape-side dynamic confirmation

**Proven dynamic observation.** Source `e6fee9c`, current worker, frame bound
1100:

```text
first Ok=720  injection=840  second Ok=960  scripted exit=1080
34BD=1 391D=1 3983=0 3985=1 3988=1 34C0=1
002A=0 0180=0 01E4=0
3988 SS:SP=7FE0:01F6
3988 words=34C0,002A,33FD
```

Only the top word was predicted. It is `34C0`; the escape-side prediction
passes. Log SHA-256 is
`458d49047bee6014b6891903fbb90c23204b31af8fcd4c2fa731cacb4af478ba`.

## A=1 current-worker reference

**Proven dynamic observation.** Source `e6fee9c`, same worker/runner, frame
bound 1100:

```text
first Ok=720  injection=840  second Ok=no
34BD=1 391D=1 3983=1 3985=0 3988=0 34C0=0
002A=1 0180=1 01E4=1
3983 SS:SP=7FE0:01F2
3983 words=002A,0005,34C0
RETF target=34C0:0005; target bytes all zero
```

The deterministic frame bound ended after the required event; no external
containment classified the run. Log SHA-256 is
`f873d57ccc0f9c68d5f0b8acfcdd7a3d783be2dac4a02f566311dd69c0be525d`.
H5 is **PROVEN FOR THE MEASURED `34BD/391D` WRAPPER PATHS**.

## PC-88VA hardware mapping at 0x34C00

**Repository-authority result.** `docs/modernization/upd9002-upd70008-mode.md`,
using its cited PC-88VA technical-manual and V30-manual authorities, places
`00000h-3FFFFh` in VA1's 256 KiB base/main RAM lower partition. VA1's WMB is
`53h`; VA1/VA2 wait-control differences do not change ownership of `34C05h`.
The circuit report independently labels this lower block standard RAM.

The terminal dynamic VAEG state is `memmode_va=1`. No additional production
bank state selects a different resource for this low address.

## Hardware versus VAEG mapping comparison

**MAPPING-MATCH at the resource level.** Documented hardware resource:
standard/main RAM. VAEG selected resource: ordinary main `mem` backing.
Therefore H7, a wrong resource mapping at `34C05h`, is **REJECTED/DOWNGRADED**.
The technical authority does not specify the expected byte contents of this
BASIC-resident location, so it cannot prove an initialization defect.

## E4 dynamic source/destination differential

**E4-PARTIAL.** Existing bounded destination evidence is complete for the
166-slot table: reset stubs are compared with first-prompt state, and live slot
writes are associated with `19E3:C7EB MOVSW`; relevant `1040:0AC3` remains a
stub with no first-live writer. M74p did not capture `DS:SI` and bounded source
bytes at `19E3:C7EB`. Consequently the remaining exact ambiguity is whether a
source record for the relevant slot is absent or present but conditionally
skipped. No SCSI/HOSTFAT runtime image was used.

## Tracked reproducer identity

The tracked runner is
[`tools/m74-diagnostics/run_basic_case.sh`](../../../tools/m74-diagnostics/run_basic_case.sh).
It accepts worker, ROM root, boot disk, BASIC command, model, guest frame
bound, diagnostic switches, and output paths. It emits repository SHA, worker
SHA, runner SHA, authorized ROM identity, model, command, guest bound,
diagnostic modes, working directory, and emulator exit status. External asset
paths and the private boot-image digest are not recorded in this tracked
report.

## Validation restoration

| Command/check | Environment | Result |
| --- | --- | --- |
| `python3 tools/repo/check_encoding.py` | Git config isolated | PASS, exit 0 |
| `python3 tools/repo/check_eol.py` | Git config isolated | PASS, exit 0 |
| `python3 tools/repo/check_case.py` | Git config isolated | PASS, exit 0, 0 findings |
| `git diff --check` | Git config isolated | PASS, exit 0 |
| `python3 tools/qa/milestone_ids.py --root . --selftest --discover --audit` | Git config isolated | PASS, exit 0, 48 selftests |
| `cmake --build build/linux-release -j2` | native Release, tests enabled | PASS, exit 0 |
| `build/linux-release/sdl2/vaeg --selftest` | current worker | PASS, exit 0 |
| targeted ROM-less plus M68/M69/M70 CTest regex | Git config isolated | PASS, 6/6 |
| `ctest --test-dir build/linux-release -LE external --output-on-failure` | Git config isolated | FAIL, 92 ordinary PASS, 5 dependency SKIP, 2 FAIL |
| `cmake --preset mingw-cross` | cross compiler available | PASS |
| `CCACHE_DISABLE=1 cmake --build --preset mingw-cross -j2` | process-local ccache disable | PASS, 430/430 |

The two full-CTest failures are
`vaeg_upd9002_protected_deletion` and its selftest: the manifest expects
`cpu/upd9002/upd9002_ops.mcr` SHA-256
`dbfcc5b3ce7d3f0b4df493cd494b7fe297aa932e231904ddeb4b59411cd73183`,
while the current tracked file is
`73c75f7a82706487b51a66e30718d6daef21caa9f73458cd3d538a059fe4d089`.
This protection-manifest failure predates the M74p diagnostic files and is not
reported as PASS. The five skips are the dependency's unavailable shell/data
cases (`test_suffix.sh`, three generated-compress cases, and `test_files.sh`).

Without isolation, Git fails with:

```text
fatal: unable to access '/Users/maho/.gitconfig': Operation not permitted
```

All Git-dependent checks used process-local
`GIT_CONFIG_GLOBAL=/dev/null GIT_CONFIG_SYSTEM=/dev/null`. The first MinGW
build attempt likewise failed because ccache could not create a temporary file
under the sandboxed user cache; `CCACHE_DISABLE=1` made the rerun pass. No
persistent Git or ccache configuration was modified.

## External-containment disposition

No final M74p run was externally preempted. C1 and `A%=1` exited through their
scripted guest events. `PRINT 1` and `A=1` reached the required command-local
RETF evidence before the deterministic frame-1100 boundary; that boundary,
not host wall time, ended the runs. Their event evidence is admissible, while
no later prompt is claimed.

## Hypothesis table

| Hypothesis | Status | Evidence |
| --- | --- | --- |
| H1 `3983` is failure boundary | REJECTED | `PRINT 1` uses `3988` and reaches same continuation |
| H2 `3985` implies caller completion | REJECTED generally | `3988` consumes current stack top |
| H3 `34C0=0` is contradictory | REJECTED | non-escape frame preserves it as later CS |
| H4 positive/negative non-escape frame equivalence | PROVEN | identical `002A,0005,34C0` |
| H5 escape vs non-escape wrapper outcome | PROVEN for measured paths | current-worker `A%=1` and `A=1` |
| H6 ownership of `34C0:0005` | MATERIALLY NARROWED, unresolved | below BASIC free lower bound, ordinary main RAM, zero |
| H7 wrong VAEG resource mapping | REJECTED/DOWNGRADED at `34C05h` | hardware and VAEG both select main RAM |
| H8 continuation state never initialized | UNRESOLVED | zero bytes do not prove expected contents |
| H9 unique ROM `C0 34` is causal | REJECTED as literal lead | structured table boundary, no consumer |

## First incorrect emulator-produced state

None proven. VAEG exposes the hardware-authorized resource class, and no
authority yet proves the expected bytes at this BASIC-resident address.

## Production fix

None. Production fix SHA: None.

## Remaining gaps

1. Identify the concrete owner/initializer for the allocated BASIC region
   containing `34C05h`; the exact missing evidence is the writer/installer
   contract for that address, not the free-memory size.
2. Capture the actual `[DS:0004]` writer path for the supplied `348856` state;
   current static candidates are `E000:804B` and `E000:E992`.
3. Complete E4 by pairing `19E3:C7EB` source `DS:SI` bytes with the already
   captured destination table, distinguishing absent from conditionally skipped.
4. Repair or formally update the pre-existing protected-deletion manifest in
   its own authorized milestone; M74p does not alter it.

## Changed files

- `cpu/upd9002/upd9002_trace.c`
- `tools/m74-diagnostics/run_basic_case.sh`
- `docs/agents/reports/m74p_target_ownership.md`

## Worktree status

The only preserved untracked material is
`cpu/upd9002/upd9002_trace.c.orig`,
`cpu/upd9002/upd9002_trace.c.rej.orig`, and `tools/__pycache__/`.
No private asset or generated binary is staged.

## Hosted CI status

NOT RUN. Local full CTest is not completely green, so hosted CI was not used.

## G74 status

NOT APPROVED.
