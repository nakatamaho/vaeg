# M87 - Final VA-only source-tree audit

M87 performs the final audit of the VA-only active source tree after the BASIC,
SCSI, uPD9002 emulation-mode authority, I/O, BIOS, FDC subsystem CPU,
`cpucva`, state-save, machine-core relocation, legacy tool/ROM regeneration,
and `lio/` disposition milestones.

Predecessor: approved G86.

Branch: `topic/m87-final-va-only-source-tree-audit`

Commit prefix: `M87:`

Candidate gate: `G87`

Report: `docs/agents/reports/m87_final_va_only_source_tree_audit.md`

Do not start M88. Do not merge M87 to `main` before G87 approval. Do not
declare G87 passed.

## Scope

M87 must:

- prove the active tree is organized around PC-88VA, not general PC-98/98x1
  compatibility;
- list all retained legacy-looking files and the active VA dependency for
  each;
- verify that `cbus/` retains only VA-supported expansion-board paths or
  explicitly deferred evidence gaps;
- verify that `cpu/upd9002/` and `cpu/upd780/` have clear ownership;
- verify that the M75 uPD9002 emulation-mode authority conclusion and the FDC
  subsystem uPD780-compatible CPU ownership remain distinct;
- verify that `machine/` has clear ownership for reset, events, timing,
  calendar, keyboard state, and state save/load;
- verify that the M86 `lio/` disposition is reflected in the active tree as
  either a justified retained compatibility path or a completed removal;
- verify that state-save, HOSTFAT, SASI/SCSI, FDD, display, sound, keyboard,
  mouse, and manual runtime gates still pass.

## Non-goals

M87 must not implement new hardware behavior. Any remaining evidence gap must
be reported as backlog rather than silently removed.

## Validation

Run the full repository invariant checks, normal builds, native tests,
available platform builds, save/load checks, hosted CI, and the standard human
VA gate.
