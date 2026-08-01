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
M74 diagnostic interfaces and would have mixed an unrelated diagnostic seam
into the clean M75 branch.

## Implemented boundary

The board ROM remains detached by default. The active SCSI path is split into:

1. the existing PCPLUS software SCSIBIOS compatibility entry;
2. the C-Bus controller register/data path;
3. the existing SxSI image backend.

The controller now retains target-controlled phase state after SELECT. A
TRANSFER INFO command consumes the current CDB, exposes data-in/data-out
completion, then advances through STATUS and MESSAGE IN before disconnecting.
The VA I/O registration includes the inherited `0CC6h` byte stream as the
data leg of this phase engine. This is retained as a compatibility path; the
SCSI55 document independently specifies `0CC0h`, `0CC2h`, and `0CC4h`, while
guest-level evidence for a separate `0CC6h` hardware designation remains
pending.

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
       DoD observes AR=19h CDB bytes, CSR=1Ah, and the next phase request.
```

The 8Ah event must be back-pressured behind the unread 11h CSR rather than
generated in the same simulation call.  CDB decoding must wait until the
host-programmed transfer count reaches zero; no command-group length is
allowed to substitute for that observed count.

## CDB coverage

| CDB | Current behavior | Data source |
|---|---|---|
| `00h` TEST UNIT READY | successful status when a SCSI image is mounted | SxSI presence |
| `12h` INQUIRY | fixed direct-access HDD identification, allocation-length bounded | controller response buffer |
| `25h` READ CAPACITY (10) | big-endian last LBA and logical block length | SxSI totals and sector size |
| `1Ah` MODE SENSE (6) | direct-access header and one block descriptor, allocation-length bounded | SxSI totals and sector size |

Data transfer positions are reset at each data phase. Completing the final
byte raises the phase-completion request, so the next controller observation
sees STATUS rather than stale DATA IN. RESET clears phase and transfer state.

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
