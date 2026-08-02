# M75 SCSI support progress report

Status: implementation in progress; G75 is pending and has not been
self-approved.

## Starting point and reviewed history

M75 was restarted from the current `main` integration point:

```text
starting SHA: 766a132ff6d66e335fe9bb1d0082d777a4a8fe14
branch: topic/m75-scsi-support
task-authority: 869b59f211ed7b9f3e87f3627a5ff769fe7ec2ff
```

The historical M75 commits supplied for reference were inspected. The GUI
SCSI slots, image creation, command-line attachment, ROM detachment, and
documentation commits were reapplied as separate commits. The historical
lightweight trace commit `6e1b65d` was not copied: it depends on discarded
retired VA1 diagnostic investigation diagnostic interfaces and would have mixed an unrelated diagnostic seam
into the clean M75 branch.

## Implemented boundary

The board ROM remains detached by default. The active SCSI path is split into:

1. the existing PCPLUS software SCSIBIOS compatibility entry;
2. the C-Bus controller register/data path;
3. the existing SxSI image backend.

The controller now retains the target-controlled phase state after SELECT and
implements the WD33C93 register/PIO boundary through the COMMAND transfer
completion point. The active low-level AR=18h/19h path does not yet decode
the CDB or advance to DATA IN/OUT, STATUS, or MESSAGE IN. The VA I/O
registration includes the inherited `0CC6h` byte stream as a legacy
compatibility path; the SCSI55 document independently specifies `0CC0h`,
`0CC2h`, and `0CC4h`, while guest-level evidence for a separate `0CC6h`
hardware designation remains pending.

The transfer-length counter uses the pre-existing serialized `cmdpos` slot.
The removed board-ROM storage is represented by reserved padding of the same
size, so `_SCSIIO` retains its historical serialized size while no longer
owning or mapping board firmware bytes. This is a state-layout preservation,
not a new save-state field or compatibility format.

The default expansion interrupt selection is INT2/IRQ6, avoiding the SASI
INT3/IRQ9 collision described by the supplied PC-88VA documentation.

## M75a WD33C93 trace checkpoint

The M75a correction is that the active controller is a WD33C93-family host
interface.  The specification boundary is therefore the register and
interrupt contract at `0CC0h`/`0CC2h`, not an independently invented physical
REQ/ACK state machine.  The auxiliary status DBR bit is the PIO data-ready
handshake; CBSY, CIP, and INT are also host-visible status.  AR `19h` is a
fixed DATA window, while the CDB registers `03h`-`0Eh` and NEC extension
registers `30h`-`35h` remain addressable controller registers.

The event-driven status sequence required for the low-level SELECT path is:

```text
SELECT complete  11h
COMMAND request  8Ah
DATA request     89h/88h
STATUS request   8Bh
MESSAGE request  8Fh
disconnect       85h
```

The temporary delayed-`8Ah` experiment was discarded before M75a.  It was a
diagnostic probe only and is not part of the production state machine.

The trace-only implementation is commit `9deafb3` and adds the disabled-by-
default `--scsitrace` option.  It records AR selection, `0CC2h` register
accesses, `0CC4h` and `0CC6h` accesses, `CS:IP`, controller status/phase,
auxiliary status, and the configured SCSI IRQ assertion/EOI clear.  It does
not alter guest state or timing when the option is absent.

Trace command used:

```text
SDL_VIDEODRIVER=dummy gtimeout -k 1 8 build/linux-debug/sdl2/vaeg \
  --model va2 --roms docs/roms \
  --fdd1 pcengine110-scsi-support.d88 \
  --scsi1 /tmp/m75-schd-40mb-512.hdd --scsitrace --nowait --mute
```

Observed predecessor boundary:

```text
AR=15h/18h, SELECT without ATN
SCSI status 11h, phase COMMAND, IRQ6 asserted
PCPLUS reads auxiliary status and status register, then clears IRQ6 by EOI
no CDB DATA-window access follows before the run timeout
```

This proves the current stopping point without claiming that a delayed timer
is a valid fix.  The next M75 checkpoint must determine why the target phase
request is not being exposed through the WD33C93 command/DBR contract.

### Full boot-to-SELECT access audit

The trace was rerun from emulator startup through the first SELECT, without
an output window or event filter.  It captured all accesses made through the
registered `0CC0h`/`0CC2h` callbacks.  The observed AR sequence includes:

```text
33(read) 02(write/read) 17(read) 33(read) 30(read/write)
33(read) 00(write) 18(write) 33(read) 17(read) 16(read)
01(write) 02(write) 11(write) 15(write) 16(write)
33(read) 33(read) 15(write) 18(write) 33(read) 17(read) 16(read)
33(read)
```

The trace seam therefore does not filter out the extended AR range: AR `30h`
and AR `33h` are visible.  No access to AR `31h`, `32h`, `34h`, or `35h` was
made by this PCPLUS initialization path before SELECT.  AR `00h`, `01h`, and
`02h` were observed, so their absence from an earlier excerpt was a window
selection issue rather than an untraceable register path.

The first low-level command is:

```text
AR=15h <- 00h
AR=18h <- 07h
CSR=11h, phase=COMMAND, IRQ6 asserted
PCPLUS reads Aux Status, then AR=17h and AR=16h
EOI clears the emulated PIC request
```

No AR `19h` CDB byte, DBR polling sequence, or AR `12h`-`14h` transfer count
appears after this point.  This confirms that the missing boundary is between
the target's COMMAND-phase request and the WD33C93 host-visible event; it is
not yet evidence about DATA, STATUS, or MESSAGE IN handling.

The observed `EOI` is an 8259-side request clear in the current emulator
trace.  M75b/c must separate that from the WD33C93 device-INT latch: reading
AR `17h` must consume the CSR, while EOI alone must not clear the device
event or overwrite a pending second event.

### Register values from the complete trace

The value-bearing trace fixes the initial contract inputs as follows:

| AR/port | access | value | interpretation at this checkpoint |
|---|---|---:|---|
| `33h` | read | `17h` | controller-provided RESET/INT/ID value |
| `02h` | write/read | `80h` | timeout value is written and read back |
| `17h` | read | `00h` | reset status is consumed |
| `0CC4h` | write | `02h` | DMER reset; DMA is explicitly disabled |
| `30h` | read/write | `00h`/`04h` | memory-bank register is probed and enabled |
| `00h` | write | `07h` | controller own ID |
| `18h` | write | `00h` | explicit controller RESET |
| `01h` | write | `08h` | PIO mode (`DmaModeSelect=000b`) with ending-disconnect policy set |
| `11h` | write | `00h` | synchronous-transfer setup |
| `15h` | write | `07h`, then `00h` | controller setup, then target ID 0 SELECT |
| `16h` | write | `00h` | source ID setup |
| `18h` | write | `07h` | SELECT without ATN |

The `18h <- 00h` RESET was present in the complete trace; it was omitted from
the earlier abbreviated report.  The `01h`, `02h`, `30h`, and `33h` values are
now recorded as evidence rather than inferred defaults.  The current emulator
maps the observed `33h=17h`/INT2 choice to VA IRQ6; the permanent
implementation must derive that mapping from one controller configuration
source instead of maintaining independent constants.

After the final EOI/IRQ clear, no further SCSI port access occurs before the
run timeout.  Two independent six-second runs produced 71 `scsitrace` records
each and byte-identical filtered output (`cmp` exit 0).  This makes the
M75b1 baseline deterministic and proves that the stop is an interrupt-wait
state, not a noisy polling loop.

## M75b1 single-depth CSR checkpoint

M75b1 adds a runtime-only CSR latch and one pending event slot without changing
the serialized `_SCSIIO` image. A new event is scheduled only when no CSR is
latched or in flight. While the CSR is unread, one successor is retained;
additional events are rejected at the admission boundary for the bus layer to
back-pressure rather than overwriting the visible CSR. Reading AR `17h`
consumes the latch and admits the single pending event. 8259 EOI handling is
not used to consume this latch.

M75b1 commits:

```text
821edef M75b1: add single-depth WD33C93 CSR latch
0fabeb3 M75b1: validate single-depth CSR latch
```

Validation:

```text
cmake --build build/linux-debug --target vaeg_sdl2 -j2       pass
python3 tools/qa/m75_scsi_controller.py --root .           pass
vaeg --selftest                                             pass
two filtered --scsitrace runs: 71 records each, cmp exit 0
M75a baseline vs M75b1 filtered trace: cmp exit 0
```

The bus-side phase back-pressure and guest-visible 17h/EOI semantics are not
yet claimed complete; phase back-pressure and the post-8Ah command/data
sequence remain the M75c implementation scope.

## M75b2 WD33C93 PIO register checkpoint

The WD33C93A primary data sheet defines indirect-register auto-increment with
three exceptions: Auxiliary Status, DATA, and COMMAND.  M75b2 applies those
exceptions to the active 0CC0h/0CC2h path.  AR `19h` is now a fixed DATA
window, and AR `18h` is a fixed COMMAND window.  DATA reads and writes use
the existing controller buffer positions and do not advance the selected AR.

The Auxiliary Status value is composed from the serialized controller status
plus the runtime CSR latch:

```text
bit 7 INT:  set while a CSR is latched; cleared only by reading AR=17h
bit 6 LCI:  retained stored status bit (currently zero unless raised later)
bit 5 BSY:  level-II controller operation is active
bit 4 CIP:  the last command is being interpreted
bit 1 PE:   parity-error state (currently zero)
bit 0 DBR:  PIO DATA window is ready
```

Reading 0CC0h no longer clears Auxiliary Status.  In particular, an 8259
EOI only clears the emulated PIC request; it does not consume the WD33C93 CSR
latch.  AR `17h` consumption is the device-side clear and admits the single
pending CSR event recorded by M75b1.

The observed `0CC4h <- 02h` is recorded as the DMER reset strobe.  M75 remains
PIO-only: TCIR, TCMR, TCMS, and DMES are not implemented, and a trace warning
marks any such unsupported strobe as `hardware-pending`.  No DMA path or DMA
channel is synthesized.

AR `30h` and `31h` retain the existing memory-bank/window state.  AR `32h`,
`34h`, and `35h` return the documented open/unsupported value (`FFh`) and
emit a `hardware-pending` trace warning; no speculative package or FIFO
behavior is added.  AR `33h` remains the read-only controller RESET/INT/ID
value used by the observed VA IRQ6 setup.

M75b2 commits:

```text
82c65ee M75b2: add WD33C93 register contract tests
7b5672b M75b2: implement WD33C93 PIO register windows
96419fa M75b2: validate WD33C93 register windows
```

Validation after M75b2:

```text
cmake --build build/linux-debug --target vaeg_sdl2 -j2       pass
python3 tools/qa/m75_scsi_controller.py --root .           pass
vaeg --selftest                                             pass
scsitrace boot through SELECT                                 pass
```

The post-SELECT trace still stops before AR `12h`-`14h`, AR `18h`=`20h`, and
AR `19h` CDB bytes.  Therefore M75b2 proves the register-window and
Auxiliary-Status boundary but does not claim the target phase request or
full PCPLUS/SCHD command progression.  Those remain M75c evidence.

The register progression is intentional and tested: reading AR `17h`
increments the selected address to AR `18h`, while a subsequent write lands
on the fixed COMMAND window.  AR `12h`-`14h` therefore accept three
successive transfer-count bytes; AR `18h` and AR `19h` do not advance.  AR
values `1Ah`-`2Fh` are held and produce a `hardware-pending` warning rather
than an invented wrap or increment rule.

The existing `scsiioint()` path gates delivery to the VA IRQ line on
`membank bit 2` (IRE1).  The CSR latch is still updated internally when IRE1
is clear, but no PIC request is asserted; this keeps device state and the
system interrupt wire separate.  AR `31h` remains the implemented memory
window register.  Auxiliary Status LCI (bit 6) and PE (bit 1) are defined but
currently return zero; neither speculative command-ignore nor parity behavior
is claimed by M75b2.

The M75c work is split without creating additional human gates:

```text
M75c1: expose the target COMMAND-phase Service Required event (8Ah) only.
       DoD ends after AR=12h-14h and AR=18h <- 20h are observed.

M75c2: implement Transfer Info PIO byte pumping through fixed AR=19h.
       DoD observes AR=19h bytes and CSR=1Ah; later phases are not yet active.
```

The 8Ah event must be back-pressured behind the unread 11h CSR rather than
generated in the same simulation call.  CDB decoding must wait until the
host-programmed transfer count reaches zero; no command-group length is
allowed to substitute for that observed count.

## M75c1 Service Required checkpoint

M75c1 now records a successful SELECT as two distinct controller events.  The
first event exposes CSR `11h` and latches a pending COMMAND-phase request.
That request is not generated in the same call.  Only after the host reads
AR `17h` (which advances AR to `18h`) is CSR `8Ah` scheduled and delivered.
This is the single-depth latch/back-pressure boundary required by the
WD33C93 initiator contract.

The deterministic guest trace reaches:

```text
CSR 11h read
CSR 8Ah delivered and read
AR 12h <- 00h, AR 13h <- 00h, AR 14h <- 06h
AR 18h <- 20h
M75c1 holds Transfer Info at COMMAND phase
```

The transfer count (`000006h` in this first observed command) is recorded as
evidence only; M75c2 must consume the host-programmed count rather than
assuming a six-byte CDB.  The predecessor's immediate CDB-copy path is
explicitly disabled at this checkpoint, so no `8Bh`, STATUS transition, or
AR `19h` access is produced by M75c1.

M75c1 commits:

```text
8df28c7 M75c1: add deferred command-request tests
99222f4 M75c1: protect Transfer Info boundary
9b4376d M75c1: defer command phase service request
```

The run is intentionally terminated by the bounded wall-clock harness after
the AR `18h` boundary (`exit=124`); the trace prefix through that boundary is
the acceptance evidence.  M75c2 must replace this bounded stop with a
deterministic transfer-count termination.

## M75c2 PIO CDB transfer checkpoint

M75c2 replaces the former immediate CDB copy for the low-level COMMAND phase.
When the host writes `AR=18h <- 20h`, the three transfer-count registers are
read as one 24-bit value and become the only active byte count.  Each AR `19h`
write is accepted only while DBR is set, is appended to the CDB buffer, and
leaves AR at `19h`.  The opcode/group is not decoded early and the count is
not inferred from the first byte.

When the host count reaches zero, the controller emits CSR `1Ah` (successful
Transfer Info completion in the COMMAND phase).  M75c2 does not yet advance
to DATA IN/OUT, STATUS, or MESSAGE IN and does not claim CDB command
execution.  A zero transfer count remains `hardware-pending` and is not
treated as a successful transfer.

The first deterministic PCPLUS trace now contains:

```text
CSR 8Ah
AR 12h/13h/14h <- 00h/00h/06h
AR 18h <- 20h
AR 19h <- six CDB bytes
CSR 1Ah
```

The same trace shows later host-programmed counts such as `24h`, `0Ah`, and
`08h`; this is why a fixed six-byte CDB assumption would be incorrect. The
M75c3 trace-only classification below establishes whether these are DATA IN
transfers or are incorrectly being consumed by the COMMAND path.

M75c2 commits:

```text
d04747f M75c2: add PIO transfer boundary tests
acf588f M75c2: pump PIO CDB bytes through DATA window
bffa7cf M75c2: validate PIO CDB completion
```

M75c3 commits:

```text
da3f469 M75c3: trace transfer phase classification
4047f58 M75c3: validate transfer phase tracing
1260764 M75c3: document transfer phase classification
```

## Existing command-helper coverage (not active low-level AR=19h execution)

| CDB | Existing `scsicmd_cmd()` helper | Active low-level path |
|---|---|---|
| `00h` TEST UNIT READY | successful status when a SCSI image is mounted | not reached from M75c2 |
| `12h` INQUIRY | fixed direct-access HDD identification, allocation-length bounded | not reached from M75c2 |
| `25h` READ CAPACITY (10) | big-endian last LBA and logical block length | not reached from M75c2 |
| `1Ah` MODE SENSE (6) | direct-access header and one block descriptor, allocation-length bounded | not reached from M75c2 |

The helper table is retained for the existing BIOS compatibility path. The
active low-level controller path currently leaves `scsiio.phase` at COMMAND,
does not call `scsicmd_cmd()`, and reports CSR `1Ah` after every host-counted
AR=19h transfer. DATA IN/STATUS/MESSAGE behavior is therefore not claimed.

## M75c3 transfer-phase classification checkpoint

M75c3 adds trace-only accounting; it does not change guest behavior. Each
`18h <- 20h` TRANSFER INFO records the controller phase, direction, host
transfer count, source path, the number of AR=19h accesses, the number of
legacy 0CC6h data-port accesses, and the resulting CSR. A bounded run with
the PC-Engine 1.1 SCSI support disk produced:

```text
exit=124 (bounded wall-clock stop)
phase=1Ah (COMMAND) for every observed transfer
direction=host-to-spc for every observed transfer
source=m75c2-ar19-pio for every observed transfer
tc=000006 -> ar19_accesses=6,  csr=1Ah
tc=000024 -> ar19_accesses=36, csr=1Ah
tc=00000A -> ar19_accesses=10, csr=1Ah
tc=000008 -> ar19_accesses=8,  csr=1Ah
data_port_accesses=0 in the classified records
```

The direction and interrupt columns are now explicit. For the same records,
all AR=19h accesses were writes (`ar19_reads=0`, `ar19_writes=TC`), and each
transfer produced exactly one CSR event request and one VA IRQ6 assertion:

```text
tc=000024 ar19_reads=0 ar19_writes=36 irq_requests=1 irq_assertions=1
tc=000008 ar19_reads=0 ar19_writes=8  irq_requests=1 irq_assertions=1
```

There were no per-byte IRQs; the single assertion occurred at transfer
completion. This is the concrete M75c3 load test for the depth-one CSR latch
and back-pressure boundary.

The captured CDB bytes resolve the command sequence without inference:

```text
tc=000006 cdb=12 00 00 00 24 00
tc=000024 cdb=00 00 00 00 00 00 00 00 00 00 00 00
tc=00000a cdb=25 00 00 00 00 00 00 00 00 00
tc=000008 cdb=00 00 00 00 00 00 00 00
tc=000006 cdb=1a 00 04 00 24 00
```

Thus `12h` with allocation length `24h` and `25h` followed by length `08h`
are confirmed. The `24h` and `08h` records themselves contain response
bytes treated as COMMAND data, proving the (b) result: the low-level path has
not yet transitioned to DATA IN.

The `24h` and `08h` values are therefore not evidence that DATA IN is already
implemented in the current branch. They are host-programmed counts consumed
by the still-COMMAND M75c2 path. Their lengths are consistent with an
INQUIRY response and READ CAPACITY response in the guest command sequence,
but the active controller has not decoded those CDBs or generated the DATA
IN/STATUS/MESSAGE phases. This is a demonstrated contract gap, not a reason
to weaken the M75c2 scope statement.

The trace also proves that the 0CC6h legacy data path is not the source of
these records. It is classified as unused by the supplied PCPLUS/SCHD path,
while the compatibility mapping remains retained for now. The next
implementation checkpoint must connect completed
COMMAND CDBs to the existing command helper only through a general phase
transition, then verify DATA IN with distinct CSR `19h`, followed by STATUS
and MESSAGE IN transfers with TC=1.

## Validation performed

The following checks passed on the current branch:

```text
cmake --preset linux-debug
cmake --build build/linux-debug --target vaeg_sdl2 -j2                  pass
cmake -S . -B build/m75-tests -DCMAKE_BUILD_TYPE=Debug \
  -DVAEG_ENABLE_TESTS=ON -DVAEG_Z80_INTEGRATION_TRACE=ON               pass
cmake --build build/m75-tests --target vaeg_sdl2 -j2                   pass
python3 tools/qa/m75_scsi_controller.py --root .                      pass
ctest --test-dir build/m75-tests -R vaeg_m75_scsi_controller \
  --output-on-failure                                                  pass
python3 tools/qa/m75_scsi_controller.py --root .                      pass
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy vaeg --selftest            pass
vaeg --model va2 --roms docs/roms --scsi1 /tmp/m75-headless40/scsi40.hdd \
  --smoke --nowait --mute                                               pass
SDL_VIDEODRIVER=dummy gtimeout -k 2 12 build/linux-debug/sdl2/vaeg \
  --model va2 --roms /Users/maho/vaeg/docs/roms \
  --fdd1 /Users/maho/vaeg/pcengine110-scsi-support.d88 \
  --scsi1 /tmp/m75-scsi40.hdd --scsitrace --nowait --mute              exit=124;
                                                                          required M75c3
                                                                          transfer records present
```

The ROM override was added so validation can use the maintained local ROM
directory without copying or modifying ROM payloads. The smoke run verified
the complete VA2 ROM set and accepted the SCSI VHD image. The full guest
PCPLUS/SCHD registration path, SCFORM, and file operations remain the M75
manual validation gate and have not yet been claimed as passing.

## Commits on the current branch

```text
869b59f M75: define SCSI support cleanup and validation
26a140a M75: add SCSI HDD menu slots
99a1b1b M75: add SCSI image creation menu
d468011 M75: add command-line SCSI attachment
d3ccebe M75: detach SCSI board ROM by default
cce1d4b M75: document PCPLUS software SCSIBIOS path
1c8d82a M75: document SCSI port and SCSIBIOS evidence
2f07711 M75: document SCHD driver evidence
32942be M75: document SCSI GUI mounting
f341f85 M75: document SCSI image creation
c246e71 M75: document command-line SCSI attachment
93f7e79 M75: implement SCSI command phases and media geometry
859dd72 M75: validate SCSI controller phase contract
7166336 M75: signal SCSI phase completion interrupts
1a82af6 M75: add explicit ROM directory override
9d6ecdb M75: document explicit ROM directory validation
54f65c7 M75: preserve SCSI state layout for transfer length
d2001aa M75: align SCSI validator with state layout
602f6ab M75: document SCSI phase implementation progress
82c65ee M75b2: add WD33C93 register contract tests
7b5672b M75b2: implement WD33C93 PIO register windows
96419fa M75b2: validate WD33C93 register windows
```

## Remaining M75 gate

The branch must still be exercised with the user-supplied
`pcengine110-scsi-support.d88` and a disposable SCSI image. The maintainer
should confirm PCPLUS loads before SCHD, SCHD registration completes, SCFORM
can initialize the target, and a reboot can create/read/delete a test file.
SASI, HOSTFAT, and the existing non-SCSI disk paths must remain unchanged.

G75 remains a human gate. This report does not declare G75 passed and does
not start M76.

## M75d1 phase-contract checkpoint

The M75c3 trace classified the apparent 36-byte and 8-byte DATA transfers as
host-to-controller writes while the controller still reported COMMAND
(`1Ah`).  The captured CDBs were `12 00 00 00 24 00` and `25 00 00 00 00
00 00 00 00 00`, so the host is following the reported phase rather than
inventing a direction.  M75d1 therefore makes the target's next phase the
single source of the service-request code, direction, and transfer pump
selection.

The phase contract table is:

| internal phase | service request | direction |
|---|---:|---|
| DATA OUT (`18h`) | `88h` | host to controller |
| DATA IN (`19h`) | `89h` | controller to host |
| COMMAND (`1Ah`) | `8Ah` | host to controller |
| STATUS (`1Bh`) | `8Bh` | controller to host |
| INFORMATION OUT (`1Ch`) | `8Ch` | host to controller |
| INFORMATION IN (`1Dh`) | `8Dh` | controller to host |
| MESSAGE OUT (`1Eh`) | `8Eh` | host to controller |
| MESSAGE IN (`1Fh`) | `8Fh` | controller to host |

The table is shared by the phase decoder and transfer trace; no second
direction switch is used.  CDB completion now invokes the existing target
command helper, records the next service request behind the CSR latch, and
keeps the pump phase-neutral.  DATA IN, STATUS, and MESSAGE IN transfers use
the same fixed AR `19h` PIO path, with completion CSRs `19h`, `1Bh`, and
`1Fh` respectively.  A command-complete message byte is `00h`; bus-free is
`85h` only when Control bit 3 permits the ending-disconnect interrupt.

The implementation checkpoint is not yet a terminal acceptance.  The first
guest run reaches the corrected `8Bh` STATUS request after TUR, but the
PCPLUS handler does not yet issue the expected one-byte STATUS Transfer Info
sequence; it reads the transfer-count bytes and stops.  This is recorded as
an M75d1 integration blocker, not as a passing DATA/STATUS result.  The
phase table and the AR19 byte-pump path are covered by the static validator,
but the required INQUIRY DATA IN golden (`CSR=19h`, AR19 read x36) has not
yet been observed.

The deterministic trace option is now `--scsitrace-limit N`.  It requests
termination after N completed transfer records (the run-loop observes the
request at a frame boundary, so a final frame may contain additional records).
The old wall-clock timeout remains useful as a safety bound only; it is not
evidence of a completed phase sequence.

M75d1 local result:

```text
phase-contract validator: pass (all 8 phases)
Linux debug SDL2 build: pass
focused M75 CTest: pass
bounded PCPLUS/SCHD run: blocked after TUR STATUS request
INQUIRY DATA IN golden: not observed
G75: not eligible
```

#### Evidence reconciliation

An earlier intermediate handoff incorrectly described the current branch as
having no production changes after the M75c3 evidence and therefore mixed two
different execution states. The branch advanced through the following
commits before the current documentation checkpoint:

```text
00e1dba M75d1: derive SCSI phases from decoded commands
5abba93 M75d1: validate the shared SCSI phase table
c526872 M75d1: document phase integration blocker
```

The first two commits are production/validation changes, not merely report
edits. They connect COMMAND completion to `scsicmd_command()`, derive the
next target phase from the shared contract, and queue the next service
request behind the CSR latch. Consequently, the post-TUR `8Bh` records belong
to the post-`00e1dba` implementation state; the earlier M75c3 records, which
showed only COMMAND/`1Ah`, remain valid predecessor evidence.

The post-TUR diagnostic records are exact at the register boundary:

```text
event: CSR=8Bh, internal phase=1Bh, IRQ6 asserted
AR=17h read: 8Bh
AR=16h read: 00h
AR=13h read: 00h
AR=14h read: 00h
AR=18h write: absent
```

A separate diagnostic changed only the phase-transfer setup count to one;
that run returned `AR=13h=00h`, `AR=14h=01h` and still produced no
`AR=18h <- 20h`. The experiment was reverted and is not part of the current
source tree. It therefore does not establish that a count value alone is
the missing contract.

The `AR=16h` value is not an unobserved reset default in this path: the
complete initialization sequence writes `AR=16h <- 00h` before SELECT, and
the handler reads it after each service request. PCPLUS's status handler
tests the upper `ER/ES` bits of this register; the observed zero follows its
normal path and is not evidence that the service request was rejected.

The remaining blocker is therefore the handoff from the interrupt/status
handler to the PCPLUS phase dispatcher, not proof that `8Bh` was absent. The
next diagnostic must monitor the internal status byte written by the handler
and the first subsequent dispatcher read, while preserving the exact AR13,
AR14, and AR16 values above. No DATA IN, STATUS Transfer Info, or MESSAGE IN
acceptance is claimed until that handoff is observed.

## Static PCPLUS CSR acceptance set and handoff narrowing

The PCPLUS interrupt path was rechecked against the normalized status
dispatch table. Raw `8Ah` and `8Bh` both normalize to service-request dispatch
key `11h` and enter `1972h`. The successful `8Ah` path therefore proves that
the common interrupt/status handoff is alive. The next diagnostic boundary is
the phase comparison at `19BBh`, followed by the `1B60h`/`1BA1h` choice,
`1C14h` transfer setup, and the `1C32h` `AR=18h <- 20h` command.

The apparent `1C0Eh` branch is rejected by the disassembly. `1C0Eh` reads
`CS:[047Eh]` and returns; it does not branch to `1C32h`. `1C05h` calls
`1C95h` to read AR `13h`/`14h`, adjusts the residual count, reads `047Eh`, and
returns. `1C14h` is a separate function called by `1B60h` and `1BA1h`; its
`1C32h` path emits the Transfer Info command. Seeing AR `13h`/`14h` after
`8Bh` therefore proves entry into the residual-count path, but does not by
itself prove that `1C32h` was reached.

The canonical raw CSR values accepted by the PCPLUS contract and their static
dispatch destinations are:

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

This table is the M75 reference for which controller-generated statuses may
be emitted by the active PCPLUS/SCHD path. It is a host-contract table, not
a claim of complete WD33C93 silicon coverage.

The disassembly does not support labeling `00h`, `01h`, `10h`, `41h`,
`43h`-`47h`, `80h`-`84h`, `86h`, or `87h` as silently rejected: those values
also normalize into dispatch-table entries. Their semantic meaning remains
unverified. The validator must therefore enforce that VAEG-generated
statuses stay within the canonical set above, while treating other
dispatchable values as unverified rather than as proven no-op or rejection
cases.

No production behavior was changed by this extraction. The remaining
trace-only observation points are `19BBh` (comparison value and result), the
selected `1B60h`/`1BA1h` path, `1C14h` entry/exit, `1C32h` command emission,
and all relevant `047Eh` reads/writes.

## Latest headless integration check

After the diagnostic trace additions, the M75 branch was configured and built
with the trace-enabled `linux-ci-clang` preset. The build completed with exit
status 0 and produced:

```text
build/linux-ci-clang/sdl2/vaeg
SHA-256 d69b11ad7b9bc3427042d808d3d06c4a3900e50a6427b15032f0f51b43c58836
```

The headless run used the complete VA2 ROM directory and the user-provided
PC-Engine 1.1 SCSI support disk. A temporary 40 MB VHD-format target was
created outside the repository at `/private/tmp/m75-scsi-40mb.hdd` solely for
this check:

```text
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \\
  build/linux-ci-clang/sdl2/vaeg \\
  --model va2 \\
  --roms /Users/maho/vaeg/docs/roms \\
  --fdd1 /Users/maho/vaeg/pcengine110-scsi-support.d88 \\
  --scsi1 /private/tmp/m75-scsi-40mb.hdd \\
  --scsi2 none --scsi3 none --scsi4 none \\
  --scsitrace --scsitrace-limit 2 --nowait --mute
```

The executable started with the complete VA2 ROM set and reached the active
PCPLUS/SCSI path. The observed sequence was:

```text
SELECT             CSR=11h, IRQ6
COMMAND request    CSR=8Ah, IRQ6
Transfer Info      TC=6, AR=19h write x6
CDB completion     CSR=1Ah
next service       CSR=8Bh, IRQ6
```

The transfer accounting was:

```text
AR19 accesses       6
AR19 reads          0
AR19 writes         6
transfer IRQs       requested=1, asserted=1
```

The run did not reach DATA IN (`CSR=19h`), STATUS, or MESSAGE IN. It was
terminated by the external eight-second safety timeout with exit status 137;
this is a bounded diagnostic stop, not an emulator crash or a passing result.
The current M75 blocker is therefore reproduced: CDB PIO and the subsequent
`8Bh` service request are observable, but the PCPLUS phase handoff does not
yet emit the next `AR=18h <- 20h` Transfer Info command. G75 remains
ineligible.


## M75d1 target-phase readiness correction (current implementation)

The previous handoff diagnosis is now resolved at the controller boundary. The
raw `8Bh` value was normalized correctly by PCPLUS, but the controller exposed
the next target phase as soon as the previous CSR was consumed. The foreground
was still completing the preceding transfer, so the `8Bh` interrupt was picked
up by the main event pump (`1742h`/`1747h`) instead of the transfer wait path.
`1747h` clears memory `CS:[047Eh]` from `BBh` to `3Bh` and `1791h` branches
using the copied value; it does not reread the cleared byte.

The production correction is general and has no PCPLUS-address or CDB shortcut:

1. A completed transfer records the target's next phase as a pending event.
2. The host-visible `CSR` latch is still consumed only by an `AR=17h` read.
3. When the host issues `AR=18h <- 20h` for that pending phase, DBR is held
   low and a controller processing event is scheduled.
4. The event then exposes the service request. The event quantum is the
   existing 100-clock PIO controller event quantum; it is not a guest-tuned
   timer or an injected CSR.
5. If the target remains in the same phase, no processing event is inserted;
   the next PIO byte remains available. A phase transition alone requires the
   processing event.

The disassembly establishes that the command/status foreground wait is
`1B67h`/`1B73h`; `1CBDh` is a separate helper used by another transfer path and
is not claimed as the observed wait point. The corrected trace is:

```text
CDB 00 00 00 00 00 00
  CSR=1Ah, AR19 writes=6
  target-phase-wait phase=1Bh, TC=010000 (DBR held low)
  CSR=8Bh delivered at guest IP=1B67h
  AR17=8Bh, AR16=00h, AR13=00h, AR14=00h
  CS:[047Eh] after normalization/consumption = 3Bh
  1BA1h -> 1C14h -> 1C32h, AR18=20h
  AR19 read=00h, CSR=1Bh
  target-phase-wait phase=1Fh
  CSR=8Fh delivered at guest IP=1BB7h
  AR19 read=00h, CSR=1Fh
```

The same run then reaches the PCPLUS REQUEST SENSE command (`03 00 00 00
0D 00`). Its fixed no-sense response begins with `70h`, and the first DATA IN
transfer is observed as `CSR=19h`, AR19 read, data `70h`. The bounded trace did
not reach the later INQUIRY CDB (`12h`) before the safety limit; therefore the
INQUIRY 36-byte golden remains unclaimed. This is an evidence boundary, not a
claim that INQUIRY is unsupported.

Commits for this checkpoint are:

```text
dffa008 M75d1: add phase handoff regression checks
1cd3edb M75d1: gate target phase requests behind PIO readiness
e56a16e M75d1: implement request sense data phase
efb19cb M75d1: use controller phase processing quantum
07c1074 M75d1: record PIO data bytes in SCSI trace
0527db6 M75d1: validate PIO byte evidence
9e6919f M75d1: keep same-phase PIO requests ready
```

Validation after the correction:

```text
python3 tools/qa/m75_scsi_controller.py --root .       PASS
cmake --build build/linux-ci-clang --target vaeg_sdl2 -j2  PASS
cmake -S . -B build/m75-tests -DCMAKE_BUILD_TYPE=Debug \
  -DVAEG_ENABLE_TESTS=ON -DVAEG_Z80_INTEGRATION_TRACE=ON  PASS
cmake --build build/m75-tests --target vaeg_sdl2 -j2      PASS
ctest --test-dir build/m75-tests -R vaeg_m75_scsi_controller \
  --output-on-failure                                      PASS
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
  build/linux-ci-clang/sdl2/vaeg --selftest                PASS
```

The current Linux worker digest after the trace-byte rebuild is
`ae3930bd2738b7685621b5722a5e6699990fa75abaf1881cef82267b693f7f5e`.
The real-ROM bounded run with `--scsitrace-limit 6` exited 0 at the diagnostic
completion limit and observed TUR STATUS/MESSAGE plus REQUEST SENSE DATA IN;
the longer `--scsitrace-limit 20` run was externally bounded with exit 124
before INQUIRY. Neither result is a G75 approval. Manual SCFORM, SCHD
registration, file operations, SASI, HOSTFAT, and non-SCSI regression gates
remain outstanding.


## M75d1 post-cursor evidence (current implementation)

The first corrected real-ROM run exposed a second general PIO state defect.  The
PCPLUS path programs `TC=1` for each byte of a DATA IN response.  Resetting
`rddatpos` at every `TRANSFER INFO` request therefore returned the first byte
(`70h`) repeatedly for REQUEST SENSE.  Commit `4ab457b` preserves the target
DATA cursor across repeated requests and resets it only when a new command or a
new phase starts.  The subsequent phase-boundary reset is committed as
`dafeae0`.

The low-overhead diagnostic options are now explicit and disabled by default:
`--scsitrace-no-guest` keeps SCSI register/transfer evidence without the
UPD9002 guest observation seam, and `--scsitrace-compact` keeps only transfer,
data-byte, phase-wait, and warning records.  These options do not change SCSI
state or guest timing; they only reduce diagnostic output overhead.

With the current worker (`ae3930bd2738b7685621b5722a5e6699990fa75abaf1881cef82267b693f7f5e`),
normal-speed compact tracing records the corrected cursor sequence:

```text
REQUEST SENSE DATA IN: data=70 index=0, CSR=19h
REQUEST SENSE DATA IN: data=00 index=1, CSR=19h
```

The 30-second bounded run still ended with the external safety status 124
before the full REQUEST SENSE allocation and later INQUIRY command.  A longer
normal-speed compact run produced no additional DATA bytes before it was
stopped; it likewise did not reach INQUIRY.  Diagnostic `--cpumult 8/32` runs
reach INQUIRY and later CDBs, but change emulated timing and produce incomplete
DATA transfers; they are not counted as M75 acceptance evidence.

Current local regression checks:

```text
python3 tools/qa/m75_scsi_controller.py --root .                       PASS
cmake --build build/linux-ci-clang --target vaeg_sdl2 -j2             PASS
cmake --build build/m75-tests --target vaeg_sdl2 -j2                  PASS
ctest --test-dir build/m75-tests -R vaeg_m75_scsi_controller            PASS
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
  build/linux-ci-clang/sdl2/vaeg --selftest                            PASS
```

Session-only smoke checks using the maintained ROM directory passed for the
non-SCSI path, a SCSI image, and an empty HOSTFAT snapshot.  SASI could not be
run because every available temporary HDD image was rejected by the existing
validator with `unsupported format or geometry`; no SASI result is claimed.
SCFORM, SCHD registration, reboot, and file create/read/delete remain manual
PC-Engine guest checks and were not run by this headless session.

The 36-byte INQUIRY golden sequence has not yet been observed at normal
emulation speed.  M75d1 and G75 therefore remain open; no G75 approval is
claimed.


## M75d1 bus-free completion and CPU-multiplier evidence (2026-08-02)

The full normal-speed trace now includes the terminal bus-free event.  After
MESSAGE IN DATA `00h` and completion `CSR=1Fh`, the controller raises
`CSR=85h` with `phase=00h`; PCPLUS reads `AR=17h -> 85h` and `AR=16h -> 00h`.
This was corrected by [bc29d9e](https://github.com/nakatamaho/vaeg/commit/bc29d9e7cecc426c4da22cbc628ab95f8c7efe8f).
The correction is general: bus-free has no following `TRANSFER INFO`, so the
pending ending-disconnect status is marked target-ready when MESSAGE IN
completes.  Data-bearing phase changes retain the controller processing
quantum and DBR gate.

The resulting TUR sequence is now:

```text
COMMAND request  8Ah; AR19 write x6; completion 1Ah
STATUS request   8Bh; AR19 read x1 = 00h; completion 1Bh
MESSAGE request  8Fh; AR19 read x1 = 00h; completion 1Fh
BUS FREE         85h; AR17 read = 85h; AR16 read = 00h
```

The same trace proceeds to a later SELECT/COMMAND request.  It does not yet
provide a normal-speed 36-byte INQUIRY DATA IN record before the bounded run
ends, so G75 remains open.  No `CSR=19h` INQUIRY record is claimed here.

### CPU multiplier diagnostic

The `--cpumult 8` and `--cpumult 32` runs reach later CDBs, including the
INQUIRY CDB `12 00 00 00 24 00`, but the guest requests only partial DATA IN
transfers before issuing later commands.  These accelerated traces are not
acceptance evidence.  Source inspection confirms that the PIO byte pump is
synchronous: `scsiio_data_read()` and `scsiio_data_write()` contain no event
scheduling or direct `CPU_REMCLOCK` manipulation.  Only phase readiness and
interrupt delivery use `nevent_set()`, whose absolute clock is expressed in
emulated CPU-clock units.  The focused validator now rejects any future
asynchronous byte-pump regression in [0c80c44](https://github.com/nakatamaho/vaeg/commit/0c80c447b6b655b81b3d08e5b67c8a1457d5be91).

The accelerated guest-level ordering mismatch is classified as an expected
CPU/device timing-ratio observation; no unsupported production timing
workaround was added.  Normal-speed evidence is the only evidence eligible
for the INQUIRY golden.

### CPU multiplier timing-unit closure (2026-08-02)

The accelerated `--cpumult 8` and `--cpumult 32` runs are closed as a
timing-mode observation, not as a demonstrated SCSI defect.  The unit audit
covered all three relevant boundaries:

- `scsiio_schedule_transfer_phase()` and the AR18=`20h` TRANSFER INFO path
  schedule `SCSI_TARGET_PROCESSING_CLOCKS` with `nevent_set(...,
  NEVENT_ABSOLUTE)`.
- `scsiintr_immediate()` and `scsiintr()` also schedule their delivery in
  the same emulated CPU-clock domain.
- `nevent_set()` converts absolute event clocks against `CPU_REMCLOCK`; it
  does not use wall-clock seconds or a guest-instruction counter.
- `scsiio_data_read()` and `scsiio_data_write()` perform one synchronous
  byte operation per AR19 access and neither schedule an event nor modify
  `CPU_REMCLOCK`.
- `UPD9002_WORKCLOCK()` applies the requested CPU multiplier to guest
  instruction consumption, while `iocoreva` retains the standard bus access
  clock.  Therefore `--cpumult 8/32` intentionally changes the guest
  CPU/device time ratio.

A finite guest polling budget can consequently expire before a device event
that is still pending in emulated time.  The partial accelerated DATA IN
traces are therefore not acceptance evidence and do not authorize a
CPU-multiplier-specific delay or guest shortcut.  The synchronous-PIO
validator was extended in [df2981e](https://github.com/nakatamaho/vaeg/commit/df2981e)
to require both the AR18-to-first-DBR target-processing event and the DBR-low
interval in the TRANSFER INFO path.

This closes the cpumult item as `guest timing assumption violation, not a
production defect`.  Normal-speed execution remains the only valid source for
the 36-byte INQUIRY golden.  The current release worker still has not produced
a normal-speed `CSR=19h` / AR19-read-x36 record within the available bounded
runs, so that evidence and all manual SCFORM/SCHD/file-operation checks remain
open.

The source pre-check also records that the current `hdd_inquiry` table is a
32-byte response while the observed CDB requests allocation length `24h`
(36 bytes).  No ANSI level, vendor string, or padding change is made from
this observation alone; the payload contract must be resolved and tested when
the normal-speed INQUIRY path is reached.

### SASI predecessor comparison

The same temporary SASI image that is rejected by the current branch was tested
with a clean binary built at the synchronized M75 predecessor
`11cb0026ad646cff16237adef95e324fcedd40d9`.  Both runs return exit status 1
with `unsupported format or geometry`.  This rejection predates the M75d1
controller changes and is not counted as an M75 regression.  No SASI pass is
claimed.

### Current evidence digest

The evaluated Linux SDL2 worker after the bus-free correction is:

```text
build/linux-ci-clang/sdl2/vaeg
SHA-256: 706b1d6a7bebc2f7a2270e49e0e55c4d79e458515d01fc4c7bc183b9af534d8a
```

The focused checks are:

```text
python3 tools/qa/m75_scsi_controller.py --root .             PASS
cmake --build build/linux-ci-clang --target vaeg_sdl2 -j2    PASS
```

The existing debug CTest, SDL selftest, repository invariant checks, SCSI
smoke, and HOSTFAT smoke remain green as recorded above.  The predecessor SASI
comparison is recorded as `exit=1` with the same pre-existing geometry error.
SCFORM, SCHD registration/reboot, create/read/delete file operations, and the
36-byte normal-speed INQUIRY golden remain manual or unobserved.  G75 is not
self-approved.
