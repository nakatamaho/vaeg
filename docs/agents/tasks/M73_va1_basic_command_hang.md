# M73 - VA1 N88 BASIC V3 command hang investigation

M73 investigates and, if evidence permits, corrects the inherited VA1 N88
BASIC V3 command hang before the later I/O, BIOS, and source-tree
reorganization milestones.

Predecessor: approved G72.

Branch: `topic/m73-va1-basic-command-hang`

Commit prefix: `M73:`

Candidate gate: `G73`

Report: `docs/agents/reports/m73_va1_basic_command_hang.md`

Do not start M74. Do not merge M73 to `main` before G73 approval. Do not
declare G73 passed.

## Scope

M73 owns only the already documented VA1 N88 BASIC V3 command hang.

Required reproductions include:

- boot VA1 mode;
- enter V3 BASIC with `BASIC`;
- run `FILES`, `LIST`, `RUN`, and `BEEP` where applicable;
- compare VA1 behavior against VA2/VA3 behavior where the same ROM/software
  path is available;
- record whether the guest is halted or executing a repeated path.

M73 must capture enough evidence to distinguish at least:

- FDC Sense Interrupt Status polling or another FDC wait condition;
- BIOS entry or BIOS simulation hooks;
- VA1/VA2 memory-map differences;
- text-display, TVRAM, or TSP paths;
- LIO/BASIC compatibility hooks, without deleting `lio/`.

## Non-goals

M73 must not:

- implement SCSI support;
- move `iova/*`;
- consolidate the I/O dispatcher;
- remove `lio/`;
- remove 98-only I/O or BIOS code;
- change unrelated CPU instruction semantics;
- change state-save format unless a directly proven BASIC fix requires a
  separately documented compatibility decision.

If the root cause is outside M73's safe correction scope, M73 may close with a
bounded reproducer, trace evidence, and a follow-on task recommendation instead
of a production fix.

## Validation

Run normal builds, native tests, repository invariant checks, focused BASIC
manual/runtime checks, and any focused trace or ROM-less test added for the
observed wait condition.

The human gate must include VA1 BASIC command testing and the standard VA
runtime smoke checks.
