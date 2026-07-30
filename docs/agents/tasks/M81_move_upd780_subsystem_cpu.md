# M81 - Move the FDC subsystem CPU to cpu/upd780

M81 creates `cpu/upd780/` and moves the FDC subsystem uPD780-compatible CPU
wrapper/backend there.

Predecessor: approved G80.

Branch: `topic/m81-move-upd780-subsystem-cpu`

Commit prefix: `M81:`

Candidate gate: `G81`

Report: `docs/agents/reports/m81_move_upd780_subsystem_cpu.md`

Do not start M82. Do not merge M81 to `main` before G81 approval. Do not
declare G81 passed.

## Scope

M81 owns the move from `cpucva/` to `cpu/upd780/` for the FDC subsystem CPU
files identified by M80.

Use rename-only commits for moves and separate reference-fixup commits.

The final tree should make the CPU roles explicit:

```text
cpu/upd9002/   main uPD9002 instruction core
cpu/upd780/    FDC subsystem uPD780-compatible CPU wrapper/backend
```

## Non-goals

M81 must not implement a new CPU core, change FDC semantics, move unrelated
VA memory code, or rename the FDC subsystem protocol beyond path-accurate
documentation.

## Validation

Run repository checks, builds, uPD780/Z80 wrapper tests, FDC subsystem tests,
save/load checks, and manual FDD boot/access gates.
