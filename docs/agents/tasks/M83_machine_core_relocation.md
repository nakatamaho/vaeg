# M83 - Move root machine-core sources under machine

M83 relocates active machine-core sources that still live in the repository
root into a dedicated `machine/` directory.

Predecessor: approved G82.

Branch: `topic/m83-machine-core-relocation`

Commit prefix: `M83:`

Candidate gate: `G83`

Report: `docs/agents/reports/m83_machine_core_relocation.md`

Do not start M84. Do not merge M83 to `main` before G83 approval. Do not
declare G83 passed.

## Scope

M83 owns source-tree layout only. It should move active root machine-core
files such as:

- `pccore.c` / `pccore.h`;
- `nevent.c` / `nevent.h`;
- `timing.c` / `timing.h`;
- `calendar.c` / `calendar.h`;
- `keystat.c` / `keystat.h` / `keystat.tbl`;
- `statsave.c` / `statsave.h` / `statsave.tbl`;
- `debugsub.c` / `debugsub.h`;
- `clockscale.h`.

Use rename-only commits for file moves and separate commits for CMake,
include-path, documentation, and validation fixups.

## Deferred files

Do not move these files in M83 unless a focused audit proves the move is safe
and the maintainer explicitly keeps them in scope:

- `common.h`, because it is a broad project-wide type and macro boundary;
- `np2ver.h`, because it is release/product identity used by frontend and
  packaging paths;
- `oprecord.c` / `oprecord.h`, because operation recording was explicitly
  deferred from M72 and needs a dedicated audit.

## Non-goals

M83 must not change machine behavior, state-save payloads, timing semantics,
keyboard behavior, event scheduling, or uPD9002 instruction semantics.

## Validation

Run repository invariant checks, normal builds, native tests, save/load
checks, M68-M72 protected checks where available, and the standard manual VA
gate.
