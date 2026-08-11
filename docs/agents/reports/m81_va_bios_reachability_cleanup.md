<!--
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
--->

# M81: VA BIOS reachability audit

## Status

This is the M81 candidate report. The audit starts from the approved M80
checkpoint at [`c44569bd8c47c87c19c6e59bfb735ce7431102bd`](https://github.com/nakatamaho/vaeg/commit/c44569bd8c47c87c19c6e59bfb735ce7431102bd).
No BIOS handler was removed: the comparison identified entries that are not
listed as the common INT09h-INT1Fh services in Tekumani, but did not prove
that they are 98-only and unreachable in the active VA product. G81 remains
pending.

## What Tekumani documents

`docs/tekumani/600INDEX.TXT` and `2.TXT` describe the VA public BIOS family.
The documented primary vectors include INT80h FDD, INT81h HDD, INT82h
keyboard, INT83h text, INT84h sprite, INT87h extended graphics, INT88h
animation, INT89h printer, INT8Ah communication, INT8Bh sound, INT8Ch
calendar-clock, INT8Dh Japanese FEP, INT8Eh line editor, and INT8Fh
graphics-screen control. The vector map also lists INT92h font, INT93h
popup, INT94h screen editor, INT97h BASIC internal, INT98h editor, INT9Eh
monitor/debugger, INT9Fh shell, and the numeric services INT A0h-A8h and
INT B0h-B7h.

## Current BIOS entries not listed as those Tekumani VA vectors

The following are the requested not-listed entries. The right-hand column is
the closest documented VA service where one exists; it is not an assertion
that the implementations are ABI-compatible.

| Current entry | Current role | Closest Tekumani VA entry | M81 decision |
| --- | --- | --- | --- |
| `BIOSOFST_09` / `bios0x09` | Common keyboard service | INT82h keyboard | Retain; common compatibility path is active and the VA keyboard path is separate. |
| `BIOSOFST_0c` / `bios0x0c` | Common serial interrupt service | INT8Ah communication | Retain; not proven 98-only. |
| `BIOSOFST_12` / `bios0x12` | FDC service/result path | INT80h FDD | Retain; FDC is an active VA dependency. |
| `BIOSOFST_13` / `bios0x13` | FDC service/result path | INT80h FDD | Retain; FDC is an active VA dependency. |
| `BIOSOFST_18` / `bios0x18` | Common text/graphics service | INT83h, INT87h, INT8Fh and related services | Retain; active LIO and display helpers depend on this compatibility layer. |
| `BIOSOFST_19` / `bios0x19` | Common RS-232C service | INT8Ah communication | Retain; VA RS-232C has an active mapped path. |
| `BIOSOFST_CMT` / `bios0x1a_cmt` | Cassette subentry | No corresponding Tekumani public CMT BIOS entry found | Retain; absence from the manual is not an unreachable proof. |
| `BIOSOFST_PRT` / `bios0x1a_prt` | Common printer subentry | INT89h printer | Retain; VA printer-related ports and the common compatibility map both exist. |
| `BIOSOFST_1b` / `bios0x1b` | Common disk dispatch, including SASI/SCSI | INT80h FDD and INT81h HDD | Retain; active SASI/SCSI support and FDD paths use this boundary. |
| `BIOSOFST_1c` / `bios0x1c` | System and interval timer service | INT8Ch calendar-clock is related, but no INT1Ch counterpart is listed | Retain; no proof of VA unreachability. |
| `BIOSOFST_1f` / `bios0x1f` | Extension/memory-move service | No corresponding Tekumani public INT1Fh entry found | Retain; the service is reachable through the common BIOS dispatcher. |

Thus, in vector terms, the not-listed set is INT09h, INT0Ch, INT12h,
INT13h, INT18h, INT19h, the INT1Ah cassette/printer subentries, INT1Bh,
INT1Ch, and INT1Fh. The INT1Ah notation here follows the implementation
names `bios0x1a_cmt` and `bios0x1a_prt`; these are simulated common BIOS
subentries, not claims about a Tekumani VA vector number.

## Emulator-internal hooks also absent from the manual

These are not additional public BIOS APIs, but they are current emulator
interception points that do not appear in the Tekumani public vector list:

- simulated reset and boot entries at `FD80:0080` and `FD80:0084`;
- the FDD wait entry at `FD80:00B4`;
- bootstrap-load interception at physical `0xFFFE8` and `0xFFFEC`;
- LIO interception in `0xF9950`-`0xF9994`; and
- the ITF-bank early-return rule in `biosfunc()`.

Current VAEG additions also outside the Tekumani public BIOS description are
the M75 SCSI backend extensions and the read-only HOSTFAT driver path. HOSTFAT
uses its own VA port/driver protocol; its `INT83h` is a driver interrupt and
must not be confused with the Tekumani text BIOS INT83h.

## Reachability decision

`bios_initialize()` is part of the reset path for all active models, while
VA reset additionally initializes the VA-specific BIOS ROM set and VA I/O
map. The common and VA maps are selected at runtime, so a common BIOS helper
cannot be removed merely because its closest public documentation is in a
different VA vector family.

A read-only comparison of the available VA1 and VA2 ROM code also found call
encodings for the common keyboard, serial, FDC, text/common, RS-232C, cassette,
disk, timer, and extension services. This byte scan is lower-bound evidence
only; it is not being used as a complete control-flow proof. Together with
the active FDC, SASI/SCSI, RS-232C, display, and state-save dependencies, it
does not establish any safe 98-only deletion candidate.

Accordingly M81 makes no production-source deletion. Retaining the entries is
the evidence-backed result; removing them would be speculative and would
violate the task requirement to remove only proven 98-only inactive handlers.

## Source evidence

- [`bios/bios.h`](../../../bios/bios.h) defines the common offsets and roles.
- [`bios/bios.c`](../../../bios/bios.c) installs and dispatches the simulated
  BIOS, bootstrap hooks, wait hook, and LIO interception.
- [`pccore.c`](../../../pccore.c) invokes common BIOS initialization during reset.
- [`io/iocore.c`](../../../io/iocore.c) and [`io/np2vasup.c`](../../../io/np2vasup.c)
  show the runtime common/VA I/O-map boundary.
- [`io/fdc.c`](../../../io/fdc.c) contains the active VA FDC path.
- [`bios/bios1b.c`](../../../bios/bios1b.c) and [`bios/sxsibios.c`](../../../bios/sxsibios.c)
  contain the active disk/SASI/SCSI boundary.
- [`io/serial.c`](../../../io/serial.c) contains the VA keyboard and RS-232C
  bindings.
- [`bios/bios18.c`](../../../bios/bios18.c) contains the common display/text
  helpers used by the active LIO path.
- [`docs/tekumani/600INDEX.TXT`](../../tekumani/600INDEX.TXT) and
  [`docs/tekumani/2.TXT`](../../tekumani/2.TXT) are the read-only VA manual
  sources used for the vector comparison.

## Validation

Repository checks are run on this report-only candidate:

```text
python3 tools/repo/check_encoding.py
python3 tools/repo/check_eol.py
python3 tools/repo/check_case.py
python3 tools/qa/upd9002_rename.py
git diff --check
cmake --preset linux-debug
cmake --build --preset linux-debug --clean-first -j4
build/linux-debug/sdl2/vaeg --selftest
ctest --test-dir build/linux-debug --output-on-failure
```

The M81 candidate Linux debug build completed successfully and
`--selftest` ended with `selftest: all tests passed` and exit status 0.
`ctest` reported that no tests were found. M81 changes are documentation
and milestone-status metadata only; no production source or binary payload
was changed. A
macOS Cocoa VA smoke launch was attempted, but the headless environment
failed before guest execution with the platform appearance error
`SystemAppearance not found`; this is an environment limitation, not a BIOS
pass. The required G81 human gate remains open.
