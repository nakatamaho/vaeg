# M83 - Final VA-only source-tree audit

M83 performs the final audit of the VA-only active source tree after the SCSI,
I/O, BIOS, FDC subsystem CPU, `cpucva`, and state-save cleanup milestones.

Predecessor: approved G82.

Branch: `topic/m83-final-va-only-source-tree-audit`

Commit prefix: `M83:`

Candidate gate: `G83`

Report: `docs/agents/reports/m83_final_va_only_source_tree_audit.md`

Do not start M84. Do not merge M83 to `main` before G83 approval. Do not
declare G83 passed.

## Scope

M83 must:

- prove the active tree is organized around PC-88VA, not general PC-98/98x1
  compatibility;
- list all retained legacy-looking files and the active VA dependency for
  each;
- verify that `cbus/` retains only VA-supported expansion-board paths or
  explicitly deferred evidence gaps;
- verify that `cpu/upd9002/` and `cpu/upd780/` have clear ownership;
- verify that state-save, HOSTFAT, SASI/SCSI, FDD, display, sound, keyboard,
  mouse, and manual runtime gates still pass.

## Non-goals

M83 must not implement new hardware behavior. Any remaining evidence gap must
be reported as backlog rather than silently removed.

## Validation

Run the full repository invariant checks, normal builds, native tests,
available platform builds, save/load checks, hosted CI, and the standard human
VA gate.
