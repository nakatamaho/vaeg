# M78 - Consolidate the VA I/O dispatcher

M78 makes the VA I/O dispatcher the canonical active I/O dispatcher and
removes the unnecessary `iocore` / `iocoreva` split where evidence permits.

Predecessor: approved G77.

Branch: `topic/m78-va-io-dispatcher-consolidation`

Commit prefix: `M78:`

Candidate gate: `G78`

Report: `docs/agents/reports/m78_va_io_dispatcher_consolidation.md`

Do not start M79. Do not merge M78 to `main` before G78 approval. Do not
declare G78 passed.

## Scope

M78 owns dispatcher structure only:

- audit `iomode_va`, `iocore_*`, and the moved VA port table;
- make the active VA I/O routing canonical;
- preserve all active VA devices, C-bus board routes, HOSTFAT, SASI, SCSI,
  keyboard, mouse, sound, display, FDC, DMA, PIC, PIT, and state behavior;
- remove duplicate dispatcher structure only when behavior-neutral.

## Non-goals

M78 must not delete 98-only device implementations merely because they remain
compiled. That cleanup is M79.

## Validation

Run focused I/O dispatcher tests if available, repository checks, normal
builds, native tests, save/load checks, and manual VA boot/device validation.
