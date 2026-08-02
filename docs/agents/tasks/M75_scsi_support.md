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

The supplied PCPLUS/SCHD trace records no `0CC6h` access in the active
low-level path.  Keep the inherited handler as a compatibility mapping, but
classify it as unused by this guest path rather than as a required WD33C93
port.  It must not be treated as a PC-9801-55 specification port without
separate guest or primary-source evidence.

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
3. **M75c3 — transfer classification trace:** record the phase, protocol
   direction, AR `19h` read/write counts, host count, legacy `0CC6h` access
   count, source path, IRQ request/assertion counts, completion CSR, and the
   captured CDB bytes for each TRANSFER INFO.  This checkpoint is trace-only
   and distinguishes an active DATA IN path from the current still-COMMAND
   path before CDB execution is connected.

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

### M75d1 — CDB decode and next-phase derivation

M75d1 uses one shared phase contract for the internal phase, WD33C93 service
request (`88h` through `8Fh`), and transfer direction.  The target command
helper owns CDB execution and selects the next phase; the AR `19h` PIO pump
only consumes the host-programmed transfer count and never selects a phase.

The INQUIRY acceptance sequence is allocation-length aware:

```text
COMMAND 8Ah -> TC=6 -> AR19 write x6 (12 00 00 00 24 00) -> CSR=1Ah
DATA IN  89h -> TC=36 -> AR19 read x36 -> CSR=19h
```

When the target response is shorter than the allocation, the target must stop
at the real response length and report the early phase change:

```text
DATA IN  89h -> TC=36 -> AR19 read x32
short transfer -> CSR=4Bh, residual TC=4
STATUS   8Bh -> TC=1 -> AR19 read x1 -> CSR=1Bh
MESSAGE  8Fh -> TC=1 -> AR19 read x1 (00h) -> CSR=1Fh
BUS FREE 85h only when Control bit 3 permits it
```

When the allocation is shorter than the target response, TC reaches zero,
the unrequested suffix is discarded, and normal DATA IN completion `CSR=19h`
leads to STATUS. M75d1 is not complete until one of these two contract paths
is observed with zero AR19 writes during DATA IN and the correct residual TC.
The first integration run reaches the corrected
`8Bh` request after TUR but the supplied PCPLUS path stops while inspecting
the transfer-count registers; this remains a blocking evidence item for the
single G75 gate.  The `--scsitrace-limit N` option is the deterministic
transfer-count termination control for bounded diagnostic runs; wall-clock
timeouts remain safety bounds only.

### PCPLUS raw CSR acceptance set

The PCPLUS interrupt normalizer and dispatch table define the following
canonical raw CSR values for the active PCPLUS/SCHD path:

| raw CSR | normalized/status key | static destination |
|---|---:|---:|
| `11h` | `01h` | `186Ch` |
| `16h` | `06h` | `1884h` |
| `18h` | `08h` | `1893h` |
| `19h`-`1Bh` | `09h`-`0Bh` | `1818h` |
| `1Fh` | `0Fh` | `1818h` |
| `42h` | `02h` | `1878h` |
| `48h` | `08h` | `1893h` |
| `49h`-`4Fh` | `09h`-`0Fh` | `1818h` |
| `85h` | `15h`, dispatch key `10h` | `1935h` |
| `88h`-`8Fh` | `18h`-`1Fh`, dispatch key `11h` | `1972h` |

This is a host-contract set, not complete WD33C93 silicon coverage. VAEG
generated controller statuses must remain within this canonical set. The
disassembly does not prove that `00h`, `01h`, `10h`, `41h`, `43h`-`47h`,
`80h`-`84h`, `86h`, or `87h` are silently rejected; they also normalize into
dispatch-table entries and remain semantically unverified. Validators must
not classify those values as proven no-op or rejection cases without new
PCPLUS/SCHD evidence.

For the current integration blocker, `1C0Eh` is a `CS:[047Eh]` read followed
by return, not a branch to `1C32h`. The relevant trace-only path is:

```text
19BBh phase comparison
  -> 1B60h or 1BA1h
  -> 1C14h transfer setup
  -> 1C32h AR=18h <- 20h
```

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

### Current diagnostic checkpoint (2026-08-02)

The trace-enabled `linux-ci-clang` CMake configuration and `vaeg_sdl2` target
build both complete successfully. The resulting worker is
`build/linux-ci-clang/sdl2/vaeg` (SHA-256
`d69b11ad7b9bc3427042d808d3d06c4a3900e50a6427b15032f0f51b43c58836`).

A dummy-SDL run using the complete VA2 ROM directory,
`pcengine110-scsi-support.d88`, and a temporary 40 MB VHD target reaches the
real PCPLUS/SCSI path and records `CSR=11h`, `CSR=8Ah`, six AR19 CDB writes,
`CSR=1Ah`, and then `CSR=8Bh` on IRQ6. It records one transfer IRQ request
and one assertion, with no AR19 reads. An eight-second external safety bound
ends the run with status 137 before DATA IN, STATUS, or MESSAGE IN. This is
diagnostic evidence only: it reproduces the current handoff blocker and does
not satisfy the M75d1 golden sequence or the G75 gate.

## Closure

The final report must include the audited SCSI dependency graph, retained and
removed code paths if any, validation commands and exit statuses, manual gate
results, and a G75 human-review checklist.


## M75d1 addendum — target phase readiness

The handoff blocker is classified as a controller timing/state-boundary defect,
not an incorrect `8Bh` normalization. The next target phase was released when
AR17 consumed the previous CSR, while the PCPLUS foreground was still in its
completion path. The fix must keep CSR consumption and target phase readiness
as separate gates, hold DBR low until the controller processing event, and
avoid delaying same-phase PIO continuation. It must remain independent of
PCPLUS addresses, CDB order, filenames, and guest timing.

The corrected TUR evidence must show `8Ah`, six CDB writes, `1Ah`, deferred
`8Bh`, one STATUS byte `00h` with `1Bh`, one MESSAGE byte `00h` with `1Fh`,
and optional `85h` only when Control bit 3 enables ending disconnect. REQUEST
SENSE (`03h`) is part of the observed PCPLUS probe and therefore has a fixed
no-sense DATA IN response. The later INQUIRY (`12h`) remains a required
M75d1 evidence item and is not complete until its allocation-length contract
is observed: either the full DATA IN response with CSR `19h`, or a short
response with `0x48 | STATUS` and the correct residual TC.

The only terminal approval remains G75. This addendum does not approve G75 and
does not begin M76.


### M75d1 post-cursor and bounded-trace addendum

PCPLUS may issue one-byte `TRANSFER INFO` requests while a DATA IN response
still has bytes remaining.  The target DATA cursor must survive those request
boundaries and reset only at command entry or a real phase boundary.  The
controller must not make the first response byte repeat indefinitely.

For long diagnostic runs, `--scsitrace-no-guest` suppresses only the disabled-by-
default UPD9002 guest observation seam, and `--scsitrace-compact` suppresses
per-port records while retaining transfer results, DATA bytes, phase waits, and
warnings.  These are diagnostic-output controls, not guest or controller
behavior.  INQUIRY acceptance requires the normal-speed allocation/response
contract, including `4Bh` and residual-TC handling when the response is
shorter than the allocation.  Accelerated `--cpumult` runs are diagnostic
only and cannot satisfy that requirement.


The historical M75a worker digest in the earlier diagnostic checkpoint is
retained as provenance.  The current post-cursor worker digest is recorded in
the report addendum and must be used for any new evaluation.


## M75d1 follow-up: bus-free and multiplier evidence

The MESSAGE IN completion path must expose ending disconnect without requiring
a further `TRANSFER INFO` command.  For Control `08h`, the required terminal
status is `85h`; when ending disconnect is disabled, the corresponding bus-free
status is `80h`.  The implementation must keep the pending bus-free status
ready when `CSR=1Fh` is consumed, while retaining the processing gate for all
data-bearing phase changes.

PIO byte access remains synchronous with each AR19 access.  The byte helpers
must not schedule events or manipulate the CPU remaining-clock budget.  The
`--cpumult` diagnostic runs are not acceptance evidence until a normal-speed
36-byte INQUIRY record is observed.  Any accelerated partial-transfer symptom
must be investigated without adding a guest-tuned delay or a hard-coded CDB
path.

A clean build at predecessor `11cb0026ad646cff16237adef95e324fcedd40d9`
reproduced the same SASI unsupported-format/geometry rejection as the current
branch; this is not an M75 regression.  The terminal manual requirements remain
SCFORM initialization, SCHD registration, reboot, file create/read/delete,
and normal-speed INQUIRY DATA IN.


### M75d1 timing-unit audit and cpumult disposition

The CPU multiplier is a diagnostic timing mode, not an acceptance-speed
control.  Before treating accelerated partial transfers as a defect, audit
all SCSI delays in source:

- phase readiness and AR18=`20h` startup use
  `nevent_set(..., NEVENT_ABSOLUTE)` with emulated CPU-clock values;
- interrupt delivery uses the same emulated-clock event queue;
- AR19 PIO byte reads and writes remain synchronous to the individual I/O
  access;
- guest instruction work is scaled by `UPD9002_WORKCLOCK`, while VA I/O
  keeps the standard bus clock.

If this audit remains true, `--cpumult` partial DATA IN is classified as a
guest timing-assumption violation, not a production defect.  Do not add a
multiplier-specific delay or PCPLUS shortcut.  Normal-speed release runs with
compact/no-guest tracing and a generous external safety bound are required for
INQUIRY acceptance.

The focused validator must also protect the interval from AR18=`20h` receipt
to the first DBR assertion: DBR is held low while the target-processing event
is pending, and that event is expressed in emulated CPU clocks.

The normal-speed INQUIRY allocation/response sequence remains unobserved and
is still a G75 blocker.  The current source response table is intentionally
32 bytes; byte4 is `1Bh` (32 minus five), and the controller must therefore
exercise the short-transfer `4Bh`/residual-TC contract for a 36-byte request.
No ANSI level, vendor string, or padding change is authorized without guest
branch evidence.


### M75d1 post-short-transfer run requirement

The short-transfer implementation is source-validated, but guest acceptance
requires a normal-speed run to observe either the full DATA IN completion or
`4Bh` with the residual count.  A bounded full trace reached TUR BUS FREE and
then a second SELECT/COMMAND request before its safety bound; it did not yet
reach the second CDB transfer.  Accelerated CPU multiplier runs may be used
only to inspect later CDB order and never satisfy the terminal G75 evidence.
SCHD registration, SCFORM, reboot, and file operations remain manual
acceptance requirements.


## M75d1 continuation — MODE SENSE geometry

The SCHD probe uses MODE SENSE(6) CDB `1A 00 04 00 24 00`.  The controller
must decode page `04h`, not silently return only the mode header and block
descriptor.  Commit
[03d4cd7](https://github.com/nakatamaho/vaeg/commit/03d4cd76541a3058cf32b0c239b499e0c0431627)
implements the empty page-00 response, page-04 rigid-disk geometry response,
page `3Fh` as the page-00 plus page-04 all-pages request, DBD-dependent
response layout, allocation-bounded transfer, and CHECK CONDITION/ILLEGAL REQUEST
for unsupported pages or
contradictory mounted geometry.  All counts and lengths derive from
`SXSIDEV`; fixed geometry values are prohibited.

The correction is source-validated and the accelerated diagnostic sequence
reaches MODE SENSE as DATA IN.  Normal-speed SCHD registration remains a
required evidence item and this continuation does not alter the G75 human
gate.
