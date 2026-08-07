# M74d diagnostic seam validation boundary

## Identity

Branch: topic/m74-va1-basic-command-hang. Starting SHA:
ac86f3386fe4463c714734285ef7fcbd0dbf149c. G74 is not approved. This is a
methodology correction only; no production correction is included.

## Established A0 baseline

The approved A0 report records worker SHA-256
34bfd64a9c61bff6f44ee7aab1081e5e858b42b2bd5b4ce1b70ead12bf38d9f2, first BASIC
Ok at frame 720, injection at frame 840, and the proven A=1 counter chain on
1.00, 1.05, and 1.10. That binary is no longer present in the available
worktrees or temporary build directories, so its exact command invocation and
source blob cannot be independently replayed in this correction.

The currently built binary is 4844fee9833ec1a05bd119588132f53ea93816084e93b025ba05213671d9d5a1.
Its source file digest is 33e97b4d1644c5cd95fecf22c198875f899a2f1d17c29deb2646bf9d0e62a789;
the committed source blob before the uncommitted fixed-address counter addition
is 5acfd18886c7bc9bec6ea63ccf537b2ea5a9e622.

## Termination-bound audit

| mechanism | location | bound | disposition |
|---|---|---|---|
| BASIC prompt timeout | sdl2/headless_input.c | guest frame count, 12000 default/configured | deterministic; can end before max frame bound by design |
| absolute headless bound | sdl2/headless_input.c | guest frame count, 20000 | deterministic |
| CTest TIMEOUT | CMakeLists.txt | host wall clock | test containment only, not used for guest classification |
| taskmng sleep/frame pacing | sdl2/taskmng.c | host pacing | not a diagnostic verdict |
| tool API wait | external command runner | host wall clock | emergency containment; it interrupted the attempted replay before guest bound |

No repository diagnostic path using steady_clock, alarm, SIGTERM, or SIGKILL was
found for the headless run. The external command runner can still stop an
interactive process before its guest frame bound; it therefore cannot classify
the interrupted replay.

## R1/R2/R3

| run | binary/seam | result | verdict |
|---|---|---|---|
| R1 | recorded A0 binary 34bfd64a... | binary unavailable for replay | unresolved; no environment conclusion |
| R2 | current binary, reachability environment disabled | external wait ended before first Ok; guest bound was not reached | unresolved; not a seam conclusion |
| R3 | current binary, fixed E000:3823 counter enabled | external wait ended before first Ok; guest bound was not reached | unresolved; not a seam conclusion |

The R2/R3 attempts used the same VA model, ROM root, 1.05 boot-only disk,
headless script, 20000-frame maximum, 12000-frame prompt timeout, and nowait
option. They did not produce a valid first-Ok comparison. Because R1 could not
be replayed and the external wait preempted R2/R3, it is not admissible to call
the difference environment, disabled-seam non-equivalence, enabled-seam
perturbation, seam cost, or guest regression.

## Diagnostic implementation decision

The added B0 measurement is only a fixed-address host counter at E000:3823;
it does not decode CD 97, hook interrupt dispatch, read the vector, or modify
CPU state. It is retained as uncommitted diagnostic work pending R1/R2/R3
validation. No session-wide INT 97 entry/exit implementation was added.

B0 advancement is stopped at the isolation gate. No INT 97 count, D2, D3, or
D4 claim is made from the interrupted runs.

## Static work boundary

The existing M74c evidence remains the admissible static result: E000:34BD
calls 391D, the return IP is 34C0, DX is 0005, and the terminal construction is
stable across the A0 versions. This does not substitute for the required B1
invocation/event measurement and does not establish lookup ABI semantics.

## Hashes

Boot-only disks:

- 1.00 bf551fc8d87f91072fefea94983a8477d7f84418bd73b24d5cf1dc6d94c09d4c
- 1.05 35c17df8b65f747b1d789200bf950f07c092ac791e29169bfd49a089893b7e4d
- 1.10 258d7d218289ab0437e8772aa50c86763fc904e024e36243823323cd86602275

ROM hashes were recorded from the maintainer-local ROM root and are retained
in the external evidence log; private ROM payloads are not copied into Git.

## Validation status

The prior A0 commit recorded trace-enabled build PASS, selftest PASS (193
cases), M68/M69/M70 PASS (5/5), romless 69 passed / 2 pre-existing protected
deletion failures / 1 external skip, and git diff check PASS. The current
uncommitted fixed-address counter source builds successfully. A new full
validation run is deferred until R1/R2/R3 has a reproducible runner and exact
A0 worker artifact.

G74 remains not approved.


## Static analysis while runtime isolation is blocked

### S1: post-3985 neighborhood

The successful scanner return is a normal near return with CF=1. The caller
continues through the code following E000:34C0; the failing A=1 path instead
uses the 3983 RET, consumes the saved SI word as IP, and enters E000:002A.
The existing trace proves the resulting tail E000:002A -> E000:0180 -> E000:01E4
RETF. The 3985 path therefore returns to its caller; 3983 displaces that caller
continuation. No static evidence here establishes whether the displaced path
or the 3983 continuation is the intended BASIC semantic.

### S2: E000:34A0-34BD provenance

The real callsite is E000:34BD, encoded E8 5D 04, so the near CALL pushes
post-CALL IP 34C0. Existing trace evidence records E000:3922 PUSH DX with
DX=0005 and E000:3923 PUSH SI with SI=002A. SI=002A is loaded by E000:33BC;
DX=0005 is already present before the wrapper and is preserved through the
observed path. The final stack words are therefore guest-produced values, not
RETF or far-CALL decoder artifacts.

### S3: E000:33B0-33D0

The relevant static producer is E000:33BC MOV SI,002A. Its direct effect is a
literal SI value used by the later near RET path. The surrounding path preserves
DX and reaches E000:34BD. The existing static callsite census identifies five
real CALL 391D sites; only 34BD is dynamically exercised in the admissible A0
runs. No production interpretation of 002A as a module owner is made.

## Updated isolation conclusion

R1/R2/R3 remain unresolved. The exact A0 executable is unavailable; the clean
rebuild from ac86f33 produced e43d9af7d07de9ab48bfda0b86dada4e2085a5a83a1f5a234428a82e2ba0f1c2,
not the recorded 34bfd64a... worker. Attempts using the current worker were
interrupted by the external command wait before the guest deterministic bound;
this is not classified as a guest result. No evidence currently proves disabled
seam non-equivalence, enabled seam perturbation, seam cost, or a guest
regression.

B0 remains gated. The fixed-address E000:3823 counter is the only B0-specific
measurement code; no INT 97 vector/opcode hook was added. A future run must first
reproduce exact R1 or explain the build/invocation identity difference, then run
R2 and R3 under a runner that does not classify external wall-clock interruption
as guest termination.


## Historical versus reconstructed baseline

The historical exact worker remains unavailable. The clean reconstruction from
ac86f33 is N0, worker SHA-256
e43d9af7d07de9ab48bfda0b86dada4e2085a5a83a1f5a234428a82e2ba0f1c2. The current
diagnostic-capable worker is N1/N2, SHA-256
4844fee9833ec1a05bd119588132f53ea93816084e93b025ba05213671d9d5a1. N1 and N2
are the same executable; only runtime diagnostic environment differs.

The short deterministic prompt-only gate used VAEG_HEADLESS_MAX_FRAMES=900,
model va, the same ROM root, the corresponding boot-only disk, --nowait, and
BASIC / @prompt / @exit. It observed first Ok at frame 720 and exited at frame
840 for every N0/N1/N2 run on 1.00, 1.05, and 1.10. The N2 fixed-address
counter output before first Ok was 391D=0, 3983=0, 3985=0, 002A=0, 01E4=2,
INT97=0 on the measured runs; the extra 01E4 events are startup activity and
not the command-specific A0 arm.

The 1.05 short-gate TVRAM digest was identical for N0, N1, and N2:
c6ff29f6852e02e822ce3ea628817a0fbe15bbb832701d734b3d269ac43044b5.
The 1.00 and 1.10 short-gate runs recorded the same prompt signature and frame
result, but TVRAM files were not captured in that gate; that is an evidence gap,
not an inferred equality. No stable full guest-state digest is available.

This establishes a usable reconstructed behavioral baseline for the first-Ok
boundary. It does not recreate the historical executable bit-for-bit and does
not yet validate the longer A=1 diagnostic run.

## Symbolic stack ledger

Let S0 be SP immediately before E000:34BD CALL 391D. The admissible A0 trace
has SS=7FE0 and SP=01F6 on entry to E000:391D, so S0=01F8. The stack ledger is:

| Step | Instruction | SP after | Top words / provenance |
|---|---|---:|---|
| 1 | before CALL at 34BD | 01F8 | caller frame |
| 2 | near CALL 391D | 01F6 | 34C0, produced by the near return address |
| 3 | 3922 PUSH DX | 01F4 | 0005, then 34C0; DX was 0005 |
| 4 | 3923 PUSH SI | 01F2 | 002A, 0005, 34C0; SI was 002A |
| 5 | 3983 RET | 01F4 | consumes 002A as near IP; 0005, 34C0 remain |
| 6 | 002A near JMP 0180 | 01F4 | no stack change |
| 7 | 0180 prologue | 01EE | pushes ES, DS, DI; nested CALLs are balanced |
| 8 | 01DA-01E3 epilogue | 01F4 | pops DI, saved DS/ES and restores the original depth |
| 9 | 01E4 RETF | 01F8 | consumes IP=0005 at 01F4 and CS=34C0 at 01F6 |

The bounded 002A-to-01E4 disassembly is:

- 002A: E9 53 01, a near JMP to 0180;
- 0180: STI, PUSH ES, PUSH DS, PUSH DI, setup/call sequence, then a common
  epilogue at 01DA which restores all three saved words;
- 01E4: CB RETF.

No operation in this path reorders the two words left by 3983. Therefore 34C0
becomes CS: the exact producer is the near CALL return address at 34BD, and the
exact consumer is the CS pop performed by RETF at 01E4. This proves the observed
stack contract; it does not prove that selecting 3983 for A=1 is semantically
intended.

## 3983 versus 3985 selector

The local tail has two selector classes. Terminators %, !, #, $, and ( branch
from 3948-395A to 3985, restoring SI and DX and returning CF=1. For a non-escape
terminator the path reaches 3973/3976 and calls 383A. At 397A, JC 3984 consumes
the helper carry result: carry set takes the balanced 3985-side return path;
carry clear reaches 397C-3983, which consumes the remaining parser words and
returns through the saved SI as a near IP. The immediate selector is therefore
3948-395A for escape forms, or the CF produced by 383A for the non-escape form.
The ultimate data source of that CF is not proven in this track; it is the next
runtime B0/C investigation boundary.

Existing admissible comparison remains: A=1 enters 34BD and reaches 3983;
A%=1 enters the same generic callsite and takes the percent escape to 3985 on
both observed scanner invocations. No new runtime data is synthesized for the
second invocation's caller.

## Revised causal chain and remaining gate

Proven chain:

34BD near CALL creates return IP 34C0 -> wrapper saves DX=0005 and SI=002A ->
3983 RET consumes 002A -> 002A JMPs to 0180 -> the balanced 0180 path restores
stack depth -> 01E4 RETF consumes IP=0005 and CS=34C0 -> execution enters
34C0:0005, whose observed bytes are zero.

The first still-unproven link is why the A=1 parser state selects the 3983
continuation rather than returning through the normal 3985/caller path. No
production correction is authorized by this static proof.

Track A short prompt gate: PASS for N0, N1, and N2 on all three boot-only
versions. Full A=1 N0/N1/N2 equivalence and B0 E000:3823 counting remain open.


## M74f: full A=1 N0/N1/N2 equivalence

The exact historical A0 executable remains unavailable. The comparison below therefore tests the reconstructed behavioral baseline, not binary identity. The common deterministic run contract was `--model va`, the trace-enabled build, `VAEG_HEADLESS_MAX_FRAMES=1500`, `VAEG_HEADLESS_PROMPT_TIMEOUT_FRAMES=300`, the existing `A=1` script, and `--nowait`. First `Ok` was observed at frame 720, input was injected at frame 840, and the second-prompt boundary was frame 1260. The nonzero process exit is the deliberate prompt-timeout result after the terminal signature; it is not a wall-clock classification.

| image | worker | first Ok / injection / boundary | A0 vector | terminal event | TVRAM SHA-256 |
|---|---|---|---|---|---|
| 1.00 | N0 | 720 / 840 / 1260 | 391D=1, 3983=1, 3985=0, 002A=1, 01E4=1 | `34C0:0005`, 16 zero bytes | `55ec29212748fd80ad638d2980e271c790ae988116f72713a9c8917167339536` |
| 1.00 | N1 disabled | 720 / 840 / 1260 | same terminal vector; diagnostics emit no summary | same | same |
| 1.00 | N2 enabled | 720 / 840 / 1260 | same, plus `E000:3823=4` | same | same |
| 1.05 | N0 | 720 / 840 / 1260 | 391D=1, 3983=1, 3985=0, 002A=1, 01E4=1 | `34C0:0005`, 16 zero bytes | `126908bee355934c5e357d1b5f7d210ca9d1ecb3d2ff25ca0cef02c3c4b5c5bc` |
| 1.05 | N1 disabled | 720 / 840 / 1260 | same terminal vector; diagnostics emit no summary | same | same |
| 1.05 | N2 enabled | 720 / 840 / 1260 | same, plus `E000:3823=4` | same | same |
| 1.10 | N0 | 720 / 840 / 1260 | 391D=1, 3983=1, 3985=0, 002A=1, 01E4=1 | `34C0:0005`, 16 zero bytes | `f4cb0a0753a0e12079b9efbc6844fa9109da0d8de26a142844ee7686f2f75c60` |
| 1.10 | N1 disabled | 720 / 840 / 1260 | same terminal vector; diagnostics emit no summary | same | same |
| 1.10 | N2 enabled | 720 / 840 / 1260 | same, plus `E000:3823=4` | same | same |

N0 is worker `e43d9af7d07de9ab48bfda0b86dada4e2085a5a83a1f5a234428a82e2ba0f1c2`. N1 and N2 are the same worker, `4844fee9833ec1a05bd119588132f53ea93816084e93b025ba05213671d9d5a1`; only the runtime diagnostic enable switch differs. The historical vector is reproduced exactly for all three images. N1 and N2 match N0 on every available deterministic observable. No guest-state digest exists, so that field remains an instrumentation gap rather than an invented prerequisite. The external runner did not preempt any of these short deterministic runs.

## B0 gate and fixed-address measurement

The full A=1 comparison opens B0: N0 reproduces the historical behavior, N1 matches N0, N2 matches N1, and no wall-clock containment classified a run. The B0 mechanism is only the existing fixed-address counter at `E000:3823` (`CD 97`); it does not decode the opcode or hook INT 97 dispatch.

| image | `E000:3823` executions | first/last frame or sequence |
|---|---:|---|
| 1.00 | 4 | not captured by the counter seam |
| 1.05 | 4 | not captured by the counter seam |
| 1.10 | 4 | not captured by the counter seam |

The missing first/last frame and guest-sequence fields are explicit gaps. The four executions are established by the fixed-address count and are sufficient for the B0 count; no general INT97 instrumentation was introduced.

## Selector provenance and current boundary

The final local selector is `E000:397A JC 3984`. The branch consumes CF from the preceding `E000:3976 CALL 383A` path for non-escape terminators; a clear carry reaches `397C`/`3983`, while carry set reaches the balanced `3985` side. The escape terminators `%`, `!`, `#`, `$`, and `(` bypass that helper and branch directly from `3948`--`395A` to `3985`. Thus the exact selector instruction and its immediate producer are proven, but the ultimate source of the helper CF is not yet isolated. B0 proves that all four lookup probe sites execute on A=1; it does not by itself establish their AX values or ABI meaning.

The admissible A=1 versus typed-assignment comparison is: A=1 enters the generic `34BD` callsite, reaches the non-escape helper, and selects `3983`; `A%=1` reaches the same scanner callsite and takes the `%` escape to `3985` on both scanner invocations. A fixed-address count for A!=1 was not added in this run, so an exact first divergent architectural register state is not claimed.

The stack contract remains: `34BD CALL` produces `34C0`, `3922` saves `0005`, `3923` saves `002A`, `3983 RET` consumes `002A`, and `01E4 RETF` consumes `0005`/`34C0`. This is a downstream, guest-ROM control-flow consequence, not a proven emulator defect. No first incorrect emulator-produced state is known; production fix: none. The first still-unproven link is the helper's ultimate CF/data provenance and why A=1 selects the non-escape continuation.
