# M77 - Consolidate the VA I/O dispatcher

M77 makes the VA I/O dispatcher the canonical active I/O dispatcher and
removes the unnecessary `iocore` / `iocoreva` split where evidence permits.

Predecessor: approved G76.

Branch: `topic/m77-va-io-dispatcher-consolidation`

Commit prefix: `M77:`

Candidate gate: `G77`

Report: `docs/agents/reports/m77_va_io_dispatcher_consolidation.md`

Do not start M78. Do not merge M77 to `main` before G77 approval. Do not
declare G77 passed.

## Scope

M77 owns dispatcher structure only:

- audit `iomode_va`, `iocore_*`, and the moved VA port table;
- make the active VA I/O routing canonical;
- preserve all active VA devices, C-bus board routes, HOSTFAT, SASI, SCSI,
  keyboard, mouse, sound, display, FDC, DMA, PIC, PIT, and state behavior;
- remove duplicate dispatcher structure only when behavior-neutral.

## Non-goals

M77 must not delete 98-only device implementations merely because they remain
compiled. That cleanup is M78.

## Validation

Run focused I/O dispatcher tests if available, repository checks, normal
builds, native tests, save/load checks, and manual VA boot/device validation.
