# M76 - Consolidate the VA I/O dispatcher

M76 makes the VA I/O dispatcher the canonical active I/O dispatcher and
removes the unnecessary `iocore` / `iocoreva` split where evidence permits.

Predecessor: approved G75.

Branch: `topic/m76-va-io-dispatcher-consolidation`

Commit prefix: `M76:`

Candidate gate: `G76`

Report: `docs/agents/reports/m76_va_io_dispatcher_consolidation.md`

Do not start M77. Do not merge M76 to `main` before G76 approval. Do not
declare G76 passed.

## Scope

M76 owns dispatcher structure only:

- audit `iomode_va`, `iocore_*`, and the moved VA port table;
- make the active VA I/O routing canonical;
- preserve all active VA devices, C-bus board routes, HOSTFAT, SASI, SCSI,
  keyboard, mouse, sound, display, FDC, DMA, PIC, PIT, and state behavior;
- remove duplicate dispatcher structure only when behavior-neutral.

## Non-goals

M76 must not delete 98-only device implementations merely because they remain
compiled. That cleanup is M77.

## Validation

Run focused I/O dispatcher tests if available, repository checks, normal
builds, native tests, save/load checks, and manual VA boot/device validation.
