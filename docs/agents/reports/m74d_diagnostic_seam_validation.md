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
