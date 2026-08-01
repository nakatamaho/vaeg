# M75 - SCSI support cleanup and validation

M75 is reserved for SCSI support cleanup, validation, and VA integration.

Predecessor: approved G74.

Branch: `topic/m75-scsi-support`

Commit prefix: `M75:`

Candidate gate: `G75`

Report: `docs/agents/reports/m75_scsi_support.md`

Do not start M76. Do not merge M75 to `main` before G75 approval. Do not
declare G75 passed.

## Scope

M75 owns the active SCSI support path after M72 folds `SUPPORT_SCSI` to the
enabled side.

The PCPLUS documentation identifies `$SCSIBIOS` as the software SCSI BIOS
service supplied by `PCPLUS.SYS`. The PC-88VA SCSI55 guidance permits the
interface-board firmware ROM to be disconnected without affecting operation.
M75 therefore keeps the board ROM disconnected by default and does not claim
the historical `DC000h-DCFFFh` board-ROM window or a substitute `D2000h`
window in the VA guest map.

The supplied `SCSI55.TXT` identifies only `0CC0h`, `0CC2h`, and `0CC4h` as
the board I/O addresses. The inherited NP2 `cbus/scsiio.c` `0CC6h`
byte-stream handler is not sufficient evidence for VA hardware behavior and
must remain unclaimed until PCPLUS/SCHD tracing or authoritative documentation
establishes it. The supplied `SETDMA.ASM` additionally proves that `0CCh` is
the software `$SCSIBIOS` interrupt: `SETDMA.COM` locates the `INT 0CCh`
handler, checks the `PCPLUS` signature at offset `000Ah`, and requests DMA
mode with `AX=82C0h`, `BL=01h`. It does not access `0CC6h` or program a DMA
channel. M75's default target is therefore the documented VA PIO path; DMA
and any `0CC6h` VA claim require separate evidence.

The supplied `SCHD.SYS`/`SCHD.DOC`/`SCHD.LOG`/`SCHD.TXT` are the PC-88VA DOS
block-driver evidence for this milestone. `PCPLUS.SYS` must precede `SCHD.SYS`;
`-I0..7` selects the target ID, while `-C`, `-S`, `-B`, and `-X` are driver
geometry, buffer, and removable-media options. A byte scan of the supplied
driver finds five `INT 0CCh` call sites and no `CD 1Bh` or literal direct
`0CC0h`-`0CC6h` port setup. This supports the documented PCPLUS software
SCSIBIOS boundary but does not prove that the legacy NP2 `0CC6h` handler is a
VA port. M75 must trace or otherwise validate the `INT 0CCh` path before
expanding the VA I/O map.

The standard local validation artifact names are:

- source disk: `pcengine110-bootonly.d88`;
- generated SCSI support disk: `pcengine110-scsi-support.d88`.

The source disk is user-supplied and remains outside Git. The generated disk
is also a local validation artifact and must not be committed.

M75 must:

- audit `cbus/scsiio.c`, `cbus/scsicmd.c`, BIOS SxSI helpers, SDL2 media UI,
  configuration, save-state entries, and ROM-less tests;
- confirm which PC-9801-55-compatible SCSI board behavior is active
  VA-supported behavior;
- validate against the documented PC-88VA SCSI support disk flow in
  `docs/modernization/scsi-support.md`, including a driver-installed boot disk
  when available;
- preserve SASI, HOSTFAT, and existing disk-image behavior;
- add or update focused validation for the active SCSI path where practical;
- document remaining hardware or guest-OS gaps.

The implementation checkpoints are:

1. expose the target-controlled SCSI phase and interrupt lifecycle after
   SELECT;
2. implement phase-aware TRANSFER INFO with REQ/ACK-sized completion;
3. cover STATUS and MESSAGE IN transitions and the PCPLUS software SCSIBIOS
   boundary;
4. implement the SCHD-observed TEST UNIT READY, INQUIRY, READ CAPACITY, and
   MODE SENSE commands against the existing SxSI image backend;
5. validate the support disk, SCFORM, and SCHD registration while retaining
   SASI and HOSTFAT behavior.

## Non-goals

M75 must not:

- move `iova/` sources;
- delete 98-only `io/` devices;
- redesign state-save format outside the SCSI evidence needed by this task;
- remove SASI or HOSTFAT;
- start VA-only source tree consolidation;
- claim complete physical WD33C93 or PC-9801-55 silicon behavior beyond the
  PCPLUS/SCHD contract evidenced here.

## Validation

Run the repository invariant checks, the normal CMake build, available native
tests, and focused SCSI/SASI/HOSTFAT smoke checks. Record unavailable platform
checks with exact blocker details.

## Closure

The final report must include the audited SCSI dependency graph, retained and
removed code paths if any, validation commands and exit statuses, manual gate
results, and a G75 human-review checklist.
