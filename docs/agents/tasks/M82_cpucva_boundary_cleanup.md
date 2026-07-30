# M82 - Clean up the remaining cpucva boundary

M82 cleans up the remaining `cpucva/` boundary after the FDC subsystem CPU has
moved to `cpu/upd780/`.

Predecessor: approved G81.

Branch: `topic/m82-cpucva-boundary-cleanup`

Commit prefix: `M82:`

Candidate gate: `G82`

Report: `docs/agents/reports/m82_cpucva_boundary_cleanup.md`

Do not start M83. Do not merge M82 to `main` before G82 approval. Do not
declare G82 passed.

## Scope

M82 must:

- audit remaining `cpucva/` contents;
- decide whether VA memory belongs under a future `memory/`, `va/`, or other
  active-tree location;
- keep `cpu/upd9002/` focused on instruction execution rather than platform
  memory mapping;
- preserve the M68 mapped-memory dispatcher boundary;
- move or retire `cpucva/` only when the active dependencies are closed.

## Non-goals

M82 must not change uPD9002 instruction semantics, FDC uPD780 behavior, I/O
dispatcher behavior, or state-save compatibility without a focused approval.

## Validation

Run repository checks, builds, native tests, M68/M69/M70/M71 protections,
save/load checks, and manual VA boot/application gates.
