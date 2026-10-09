<!--
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
-->
# PCPLUS 1.08 disassembly notes

This note records a first-pass static disassembly of **PCPLUS.SYS v1.08**, the
PC-88VA PC-Engine service bundle distributed as PCPLUS. The binary is not stored
in this repository; the examined executable was built locally from the utility
media recipe that applies `pcp108p.lzh`'s `pcplus.bdf` update to the base
`pcp108.lzh` driver.

Examined artifact:

- `PCPLUS.SYS`, size `16384` bytes
- SHA-256: `f86d03201a2fa6c0dab13345df55f3bb929f41ec3c7c6d03efb4dbd7935f1b06`
- DOS character device name: `PCPLUS00`
- device header strategy offset `27B8h`, interrupt offset `27C3h`

## High-level role

PCPLUS is a PC-Engine device driver that installs a set of VA extension BIOSes
and services rather than a single block-device driver. Its user manual lists the
service bundle as:

- `$TIME`: time manager;
- `$INTTRG`: interval-trigger / VRTC hook manager;
- `$TDBios98`: PC-9801-compatible calendar/timer BIOS compatible with
  `INT 1Ch`;
- `$SCSIBIOS`: PC-9801-55/compatible SCSI interface BIOS for VA;
- `$SMM`: Sound Memory Manager;
- `$BProc`: background process manager.

The patched v1.08 banner embedded in the driver identifies these component
versions:

```text
PCPLUS.SYS v1.08
INTTRG   : v1.09
TIME     : v1.01
SCSIBios : v1.08
SMM      : v1.03
BProc    : v1.00
```

## DOS device-driver loading path

The first bytes are a normal DOS character-device header:

```text
+00  FFFF:FFFF next device pointer
+04  8000h     character-device attributes
+06  27B8h     strategy entry
+08  27C3h     interrupt entry
+0A  "PCPLUS00" device name
```

The strategy entry at `27B8h` only saves the DOS request-header pointer in
resident variables `00DCh:00DEh` and returns far. The interrupt entry at
`27C3h` switches `DS` to `CS`, reloads that request header, and dispatches on
request command byte `[ES:BX+02h]`.

Only command `00h` performs initialization. For that request it stores the
resident end pointer `CS:2820h` into the request header at offsets `0Eh/10h`.
After the initialization calls it writes status `0100h` to `[ES:BX+03h]` and
returns. Other request types skip the install body and still complete with
status `0100h`, so PCPLUS behaves as an always-resident service driver rather
than an active DOS character I/O endpoint.

Initialization prints the banner through PC-Engine console service `INT 83h`
with `AH=02h`, then installs the service vectors described below.

## Installed interrupt vectors

The initializer saves old vectors with DOS `INT 21h, AH=35h`, installs new
vectors with `AH=25h`, and also writes a few interrupt vectors directly into
the IVT for private service entry points.

| Vector | New handler | Saved old vector | Apparent component |
| ---: | ---: | ---: | --- |
| `CDh` | `0D1Ah` | not chained through DOS; direct IVT write | PCPLUS private service |
| `CEh` | `0D1Ah` | not chained through DOS; direct IVT write | PCPLUS private service |
| `CFh` | `0D1Ah` | not chained through DOS; direct IVT write | PCPLUS private service |
| `7Ah` | `117Ah` | `00F0h:00F2h` | INTTRG / interval services |
| `1Ch` | `11E2h` | `00F4h:00F6h` | PC-98 compatible timer BIOS path |
| `8Ch` | `1693h` | `0320h:0322h` | TIME / alarm path |
| `1Bh` | `1E00h` | `0486h:0488h` | SMM-related service hook |
| `86h` | `1E28h` | `048Eh:0490h` | SMM-related service hook |
| `CAh` | `2592h` | `0492h:0494h` | BProc/background service hook |
| `82h` | `2649h` | `0BC6h:0BC8h` | extension service hook |
| `96h` | `2692h` | `0BCAh:0BCCh` | extension service hook |
| `CBh` | `2752h` | `0BCEh:0BD0h` | extension service hook |

The vector table and handler grouping match the manual: PCPLUS is a collection
of resident software interrupt services. For an emulator, the main effect is
not DOS file I/O; it is the new interrupt-surface and the hardware I/O these
handlers perform.

## EMS use

PCPLUS optionally uses EMS. The user manual says some functions require EMS and
that PCPLUS should be loaded after the EMM stack. The initialization code
confirms this:

1. obtains the `INT 67h` vector with `INT 21h, AX=3567h`;
2. verifies the EMM signature `"EMMXXXX03"` at offset `000Ah` of the EMM entry
   segment;
3. calls `INT 67h, AH=46h` and requires an EMS 4.x-style version (`AL >= 40h`);
4. calls EMS function `58h` to enumerate mappable physical pages and records
   physical page-frame segments for entries 0 and 1 at resident offsets
   `00E2h` and `00E4h`;
5. allocates one EMS page with `INT 67h, AH=43h, BX=1`, storing the handle at
   `00EAh`;
6. saves/restores the page map around a temporary map of logical page 0 to
   physical page 0;
7. clears the selected 16 KiB EMS page-frame window.

This EMS path is optional: if the signature, version, page-frame enumeration,
or allocation fails, the initializer carries on and later skips EMS-dependent
features. In the utility-media recipe, PCPLUS is deliberately loaded after the
EMMVA/SQEMM98/EMMVA stack so this path can succeed when the EMS board is
configured.

## INTTRG / VRTC trigger service

The handler installed on `INT 7Ah` and the chained `INT 1Ch` path maintain two
small trigger tables in the resident image. The VRTC path first calls the
previous timer handler, then uses a private resident stack at `CS:0320h` while
it walks these tables.

Visible table behaviour:

- fixed-size tables with 16 entries;
- 8-byte records for scheduled triggers;
- flags for active/expired entries;
- periodic counters for 1, 10, 20, 30, and 60 tick-like intervals;
- far callback pointers invoked under interrupt context when a trigger expires.

This matches the manual's `$INTTRG` description: PCPLUS centralizes VRTC hook
management so multiple resident programs can safely share periodic callbacks.
For VAEG, accurate `INT 1Ch` cadence and safe nested interrupt behaviour matter
more than the exact human-facing API names.

## SCSI BIOS path

The SCSI initializer probes a PC-9801-55-compatible board at the expected ports:

```text
0CC0h  command/status style port
0CC2h  data/auxiliary port
0CC4h  control port
```

It also installs a temporary interrupt gate through IVT entry `00CCh` and
programs board-related vectors after probing. The bundled `scsi55.txt` says the
board should remain at ports `0CC0h`, `0CC2h`, and `0CC4h`; its firmware ROM is
normally at `0DC000h-0DCFFFh` and should not overlap an EMS page frame. The
same note says VA users should choose interrupt vectors that do not collide
with sound/mouse uses, and that the SCSI BIOS normally uses programmed I/O;
DMA is optional and must avoid the already scarce VA DMA channels.

The code reads and writes the `0CC0h/0CC2h/0CC4h` ports directly, performs board
status checks, and uses `INT CCh` internally while waiting for completion. A VA
emulator that does not emulate a PC-9801-55-compatible board should let this
probe fail cleanly rather than fabricate a partially working controller.

## Sound Memory Manager and background hooks

The SMM-related initialization installs `INT 1Bh` and `INT 86h`, and the binary
contains module labels such as `CTM:`, `DMA:`, `PCM:`, and `BGP:`. The included
source for `SMSTAT.COM` and `SMM.H` belongs to the same package and reports
Sound Memory Manager state. The disassembly shows that these handlers keep
resident tables and dispatch through software interrupts rather than through
DOS device I/O.

The BProc/background-process hooks install `INT CAh`, `INT 82h`, `INT 96h`, and
`INT CBh`. The first-pass pass has not fully labelled their function matrices,
but the structure is consistent with PCPLUS exposing independent service
families through software interrupts.

## Emulator implementation implications

For VAEG and PC-88VA/PC-98 compatibility work, PCPLUS stresses these areas:

1. **PC-Engine device-driver ABI**: DOS must call the strategy and interrupt
   entries with a correct request header. PCPLUS relies on the init request to
   set the resident end pointer and does not act like a normal character stream.
2. **Software interrupt surface**: after install, programs may call `INT 7Ah`,
   `1Ch`, `8Ch`, `1Bh`, `86h`, `CAh`, `82h`, `96h`, `CBh`, or the private
   `CDh/CEh/CFh` services. Snapshots and resets should preserve or clear these
   vectors consistently.
3. **EMS page-frame correctness**: PCPLUS uses EMS function `58h` to discover
   physical page-frame segments and maps/clears an EMS page. Page-frame
   selection must reflect the EMM driver configuration, especially the
   `C000h/C400h/C800h/CC00h` VA recipe.
4. **SCSI-board non-presence must be safe**: the driver probes real hardware
   ports `0CC0h/0CC2h/0CC4h`. If no 55-compatible board exists, reads should
   make the probe fail instead of hanging forever.
5. **Timer callback timing**: `$INTTRG` callbacks run from the timer/VRTC path
   on a resident stack. Bad interrupt re-entry or incorrect `INT 1Ch` cadence
   can break resident animations, alarms, or background services.

## Current confidence and open points

The device header, install flow, vector installation, EMS probe/allocation, and
SCSI port use are directly supported by static disassembly. The exact public
API matrices for each software interrupt still need a deeper pass with the
original individual specifications or with runtime traces from clients such as
`SMSTAT.COM`, `SETDMA.COM`, and SCSI device drivers.
