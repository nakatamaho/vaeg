# M80 - Move the FDC subsystem CPU to cpu/upd780

M80 creates `cpu/upd780/` and moves the FDC subsystem uPD780-compatible CPU
wrapper/backend there.

Predecessor: approved G79.

Branch: `topic/m80-move-upd780-subsystem-cpu`

Commit prefix: `M80:`

Candidate gate: `G80`

Report: `docs/agents/reports/m80_move_upd780_subsystem_cpu.md`

Do not start M81. Do not merge M80 to `main` before G80 approval. Do not
declare G80 passed.

## Scope

M80 owns the move from `cpucva/` to `cpu/upd780/` for the FDC subsystem CPU
files identified by M79.

Use rename-only commits for moves and separate reference-fixup commits.

The final tree should make the CPU roles explicit:

```text
cpu/upd9002/   main uPD9002 instruction core
cpu/upd780/    FDC subsystem uPD780-compatible CPU wrapper/backend
```

## Non-goals

M80 must not implement a new CPU core, change FDC semantics, move unrelated
VA memory code, or rename the FDC subsystem protocol beyond path-accurate
documentation.

## Validation

Run repository checks, builds, uPD780/Z80 wrapper tests, FDC subsystem tests,
save/load checks, and manual FDD boot/access gates.
