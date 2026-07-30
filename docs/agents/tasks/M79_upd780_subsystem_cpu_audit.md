# M79 - Audit the FDC subsystem uPD780-compatible CPU

M79 audits the FDC subsystem CPU path and documents the replacement boundary
for `cpucva/z80_core.cpp` as a uPD780-compatible FDC subsystem CPU, not as a
generic Z80 core.

Predecessor: approved G78.

Branch: `topic/m79-upd780-subsystem-cpu-audit`

Commit prefix: `M79:`

Candidate gate: `G79`

Report: `docs/agents/reports/m79_upd780_subsystem_cpu_audit.md`

Do not start M80. Do not merge M79 to `main` before G79 approval. Do not
declare G79 passed.

## Scope

M79 must:

- audit `cpucva/z80_core.cpp`, `cpucva/z80_disasm.cpp`, legacy-state codec,
  `subsystem`, `subsystemif`, and `fdsubsys` consumers;
- identify the current FDC uPD780-compatible CPU contract;
- distinguish FDC subsystem uPD780 behavior from any future main-CPU
  emulation-mode work;
- define the exact files to move under `cpu/upd780/` in M80;
- preserve the suzukiplan-backed wrapper behavior.

## Non-goals

M79 must not move files or change production CPU behavior except for minimal
audit instrumentation disabled in production if absolutely required.

## Validation

Run Z80/uPD780 wrapper tests, FDC subsystem tests, repository checks, builds,
and manual FDD boot/access gates.
