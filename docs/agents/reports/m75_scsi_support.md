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
```

## Remaining M75 gate

The branch must still be exercised with the user-supplied
`pcengine110-scsi-support.d88` and a disposable SCSI image. The maintainer
should confirm PCPLUS loads before SCHD, SCHD registration completes, SCFORM
can initialize the target, and a reboot can create/read/delete a test file.
SASI, HOSTFAT, and the existing non-SCSI disk paths must remain unchanged.

G75 remains a human gate. This report does not declare G75 passed and does
not start M76.
