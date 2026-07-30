# M81 - Clean up the remaining cpucva boundary

M81 cleans up the remaining `cpucva/` boundary after the FDC subsystem CPU has
moved to `cpu/upd780/`.

Predecessor: approved G80.

Branch: `topic/m81-cpucva-boundary-cleanup`

Commit prefix: `M81:`

Candidate gate: `G81`

Report: `docs/agents/reports/m81_cpucva_boundary_cleanup.md`

Do not start M82. Do not merge M81 to `main` before G81 approval. Do not
declare G81 passed.

## Scope

M81 must:

- audit remaining `cpucva/` contents;
- decide whether VA memory belongs under a future `memory/`, `va/`, or other
  active-tree location;
- keep `cpu/upd9002/` focused on instruction execution rather than platform
  memory mapping;
- preserve the M68 mapped-memory dispatcher boundary;
- move or retire `cpucva/` only when the active dependencies are closed.

## Non-goals

M81 must not change uPD9002 instruction semantics, FDC uPD780 behavior, I/O
dispatcher behavior, or state-save compatibility without a focused approval.

## Validation

Run repository checks, builds, native tests, M68/M69/M70/M71 protections,
save/load checks, and manual VA boot/application gates.
