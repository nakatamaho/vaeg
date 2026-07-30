# M77 - Audit and remove 98-only io implementations

M77 audits and removes 98-only `io/` implementations after the VA dispatcher
has been consolidated.

Predecessor: approved G76.

Branch: `topic/m77-98-only-io-cleanup`

Commit prefix: `M77:`

Candidate gate: `G77`

Report: `docs/agents/reports/m77_98_only_io_cleanup.md`

Do not start M78. Do not merge M77 to `main` before G77 approval. Do not
declare G77 passed.

## Scope

M77 may remove only implementations proven unreachable or irrelevant to the
active VA product.

Initial audit candidates include:

- `epsonio`;
- `emsio`;
- `printif`;
- `nmiio`;
- `necio`;
- `artic`, only if its callback and state-save role are proven inactive.

`fdd320` is not an M77 default deletion target. It must be audited separately
because 5-inch 2D behavior may still be relevant to the PC-88 side of the VA
environment.

## Required retained boundaries

Do not remove:

- `cbus/` boards that can be attached to the VA;
- SASI, SCSI, HOSTFAT, BMS, FDC, DMA, PIC, PIT, keyboard, mouse, sound, or
  display paths;
- shared state/helper code such as `sysport`, `upd4990`, `crtc`, `cgrom`,
  `gdc`, `egc`, or `mouseif` unless a focused audit first replaces the active
  dependency.

## Validation

Run repository checks, builds, native tests, save/load tests, and manual VA
device gates. Any state-save section removal requires explicit compatibility
evidence.
