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

The supplied `SCSI55.TXT` identifies `0CC0h`, `0CC2h`, and `0CC4h` as the
documented board I/O addresses. The inherited `0CC6h` byte stream is retained
as the controller data leg in M75, but its independent hardware designation
remains unclaimed until PCPLUS/SCHD tracing or authoritative documentation
establishes it. The supplied `SETDMA.ASM` additionally proves that `0CCh` is
the software `$SCSIBIOS` interrupt: `SETDMA.COM` locates the `INT 0CCh`
handler, checks the `PCPLUS` signature at offset `000Ah`, and requests DMA
mode with `AX=82C0h`, `BL=01h`. It does not access `0CC6h` or program a DMA
channel. M75's default target is therefore the documented VA PIO path; the
`0CC6h` handler is a compatibility implementation rather than a new hardware
claim.

The supplied `SCHD.SYS`/`SCHD.DOC`/`SCHD.LOG`/`SCHD.TXT` are the PC-88VA DOS
block-driver evidence for this milestone. `PCPLUS.SYS` must precede `SCHD.SYS`;
`-I0..7` selects the target ID, while `-C`, `-S`, `-B`, and `-X` are driver
geometry, buffer, and removable-media options. A byte scan of the supplied
driver finds five `INT 0CCh` call sites and no `CD 1Bh` or literal direct
`0CC0h`-`0CC6h` port setup. This supports the documented PCPLUS software
SCSIBIOS boundary; M75's `0CC6h` registration is therefore kept as a
compatibility data path and remains subject to guest-level validation.

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

## WD33C93 host-contract checkpoints

M75 treats the PC-9801-55-compatible controller as a WD33C93-family
register interface, not as a freely designed SCSI state machine.  The
controller's host-visible contract is the authority for this milestone.

The host selects a register through `0CC0h` and accesses it through `0CC2h`.
`0CC0h` reads the auxiliary status; the documented PIO handshake is the
DBR bit, together with the controller-busy and command-in-progress bits.
Register `19h` is the fixed DATA window.  The CDB registers (`03h` through
`0Eh`) retain the normal address progression, and the NEC extension range
`30h` through `35h` must be audited before the phase engine is changed.

The required low-level sequence is event-driven:

```text
SELECT -> SCSI status 11h -> COMMAND request 8Ah
TRANSFER INFO/CDB -> DATA request 89h or 88h
DATA complete -> STATUS request 8Bh
STATUS complete -> MESSAGE IN request 8Fh
MESSAGE complete -> disconnect 85h
```

The `8Ah` request must be derived from the target/controller phase event.  A
wall-clock delay that happens to expose `8Ah` is not an acceptable
implementation.  The trace-only M75a checkpoint must record every `0CC0h`,
`0CC2h`, `0CC4h`, and `0CC6h` access with AR, value, `CS:IP`, auxiliary
status, SCSI status, phase, and interrupt assertion/clear events.

The observed PCPLUS path is specifically the low-level `07h Select without
ATN` followed by `20h Transfer Info` path.  It does not send MESSAGE OUT or
IDENTIFY.  The target LUN must therefore be obtained from the incoming CDB
where the protocol requires it; an unconditional IDENTIFY or intermediate
disconnect is not permitted.  `85h` is reserved for command completion and
bus-free handling unless the Control register explicitly authorizes another
disconnect interrupt.

The controller status register is a depth-one latch.  A second target event
must remain pending while the current status is unread.  Reading `17h`
consumes the latched status and permits the next pending event to assert INT.
An 8259 EOI is not the WD33C93 device-INT clear operation.  M75b/c tests must
cover ordered delivery of two events and must prove that an unread status is
not overwritten or cleared by EOI.

SELECT must return `11h` only for a configured target ID.  Other IDs return
`42h` (select/reselect timeout), with the timeout policy derived from AR
`02h` rather than treating every ID as present.

The source of the inherited `0CC6h` handler remains unresolved.  It must not
be treated as a PC-9801-55 specification port without guest or primary-source
evidence; if no evidence is found it is a pending/open-bus compatibility
question, not a reason to alter the documented `0CC0h`/`0CC2h`/`0CC4h` path.

The M75b1 register-value evidence must retain these observed inputs:

```text
AR=00h <- 07h
AR=01h <- 08h
AR=02h <- 80h, readback 80h
AR=11h <- 00h
AR=15h <- 07h, later 00h
AR=16h <- 00h
AR=18h <- 00h (RESET), later 07h (SELECT)
AR=30h read 00h, write 04h
AR=33h read 17h
0CC4h <- 02h (DMER reset; PIO selected)
```

The complete M75a trace also contains a reset command and must not be
described as a SELECT-only initialization.  A repeated run is accepted only
when the filtered `scsitrace` records are byte-identical and the no-access
interval after EOI is explicitly recorded.

M75c is complete only when the post-`8Ah` trace contains the controller's
transfer count writes at AR `12h`-`14h`, AR `18h`=`20h`, and the actual CDB
source (PIO DATA-window/DBR or an explicitly evidenced DMA path), followed by
CSR `1Ah` and a second interrupt.  PCPLUS merely progressing is not a
semantic acceptance criterion.

Before M75c implementation, the following M75b2 invariants are mandatory:

- reading AR `17h` advances AR to `18h`; the next write reaches COMMAND;
- three writes selected at AR `12h` reach `12h`, `13h`, and `14h`;
- repeated writes at AR `18h` remain COMMAND writes;
- repeated writes at AR `19h` remain DATA-window writes;
- AR `1Ah`-`2Fh` is held and reported as undefined/hardware-pending, with no
  invented wrap behavior;
- IRQ delivery is gated by `30h` IRE1 (membank bit 2), while the device CSR
  latch remains independent of the PIC wire;
- AR `31h` is the implemented memory-window register;
- Auxiliary Status LCI bit 6 and PE bit 1 are defined as zero/unmodeled until
  direct PCPLUS/SCHD evidence requires more behavior.

M75c is internally divided into three implementation checkpoints, but they
share the single terminal G75 gate:

1. **M75c1 — Service Required generation:** expose target COMMAND-phase
   `8Ah` only after the unread `11h` CSR has been consumed, then stop after
   the host writes AR `12h`-`14h` and AR `18h`=`20h`.  The transfer count is
   evidence, not a hard-coded CDB length.  M75c1 is implemented and its
   bounded trace reaches this exact stop.
2. **M75c2 — Transfer Info PIO:** pump bytes through fixed AR `19h` using
   DBR until the host-programmed transfer count reaches zero, then expose
   CSR `1Ah`.  Decode the CDB only after the transfer count completes.
   M75c2 reaches the AR `19h` CDB transfer and CSR `1Ah` boundary, but does
   not execute the decoded CDB or advance DATA/STATUS/MESSAGE phases.
3. **M75c3 — transfer classification trace:** record the phase, direction,
   host count, AR `19h` access count, legacy `0CC6h` access count, source
   path, and completion CSR for each TRANSFER INFO.  This checkpoint is
   trace-only and distinguishes an active DATA IN path from the current
   still-COMMAND path before CDB execution is connected.

None of these checkpoints is independently approvable or a new milestone
gate.

M75 does not claim that the physical REQ/ACK wires must be exposed as guest
ports.  The controller must instead reproduce the WD33C93 register and
interrupt contract that PCPLUS/SCHD observe.  NP2's simplified SCSI model is
not a specification source for this checkpoint.

The WD33C93A data sheet is the primary register authority for this milestone:

`http://www.bitsavers.org/components/westernDigital/WD33C93A_Data_Sheet_and_Application_Notes_Nov1990.pdf`

It specifies that indirect-register address auto-increment excludes the
Auxiliary Status, DATA, and COMMAND registers.  It also specifies that
Control DMA mode `000b` is polled I/O, with the host polling DBR before each
DATA access.  M75b2 implements only this PIO register boundary.  The 0CC4h
DMER reset observed from PCPLUS is evidence that DMA is disabled; TCIR, TCMR,
TCMS, and DMES remain hardware-pending and must not be emulated speculatively.

M75b2 also requires:

- AR `19h` DATA and AR `18h` COMMAND fixed-window accesses with no AR advance;
- Auxiliary Status composition with INT, LCI, BSY, CIP, PE, and DBR bits;
- AR `17h` as the only device-side CSR-latch consume operation;
- 8259 EOI not consuming or overwriting a WD33C93 CSR;
- AR `32h`, `34h`, and `35h` to remain explicit unsupported/hardware-pending
  accesses until PCPLUS/SCHD or board documentation proves their behavior;
- no DMA transfer path, DMA-channel synthesis, or 0CC6h hardware claim.

M75b1, M75b2, and M75c1-c3 are implementation checkpoints, not separate
human gates.
The single terminal G75 review remains the only approval boundary.  M75c must
still demonstrate the post-8Ah transfer-count writes, AR `18h`=`20h`, actual
AR `19h` CDB transfer, CSR `1Ah`, and the subsequent phase sequence.

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
