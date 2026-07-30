# M80 - Audit the FDC subsystem uPD780-compatible CPU

M80 audits the FDC subsystem CPU path and documents the replacement boundary
for `cpucva/z80_core.cpp` as a uPD780-compatible FDC subsystem CPU, not as a
generic Z80 core.

Predecessor: approved G79.

Branch: `topic/m80-upd780-subsystem-cpu-audit`

Commit prefix: `M80:`

Candidate gate: `G80`

Report: `docs/agents/reports/m80_upd780_subsystem_cpu_audit.md`

Do not start M81. Do not merge M80 to `main` before G80 approval. Do not
declare G80 passed.

## Scope

M80 must:

- audit `cpucva/z80_core.cpp`, `cpucva/z80_disasm.cpp`, legacy-state codec,
  `subsystem`, `subsystemif`, and `fdsubsys` consumers;
- identify the current FDC uPD780-compatible CPU contract;
- distinguish FDC subsystem uPD780 behavior from any future main-CPU
  emulation-mode work;
- define the exact files to move under `cpu/upd780/` in M81;
- preserve the suzukiplan-backed wrapper behavior.

## Non-goals

M80 must not move files or change production CPU behavior except for minimal
audit instrumentation disabled in production if absolutely required.

## Validation

Run Z80/uPD780 wrapper tests, FDC subsystem tests, repository checks, builds,
and manual FDD boot/access gates.
