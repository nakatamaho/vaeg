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
# PCEPAT Rev.50916 disassembly notes

This note records a first-pass static disassembly of **PCEPAT.SYS Rev.50916**,
the PC-Engine patch driver for PC-88VA/VA2/VA3. The source archive is an
LHarc self-extracting `PCEPAT.COM`; the examined `PCEPAT.SYS` is the driver
payload extracted from that archive and used by the utility-media builder.

Examined artifacts:

- source archive `pcepat.com`, size `6656` bytes, SHA-256
  `59296bcb77b158ce072a7f62bdbdca420305fb43004f69845345efc73c276945`
- extracted/generated `PCEPAT.SYS`, size `6490` bytes, SHA-256
  `fc2bb1d785ac08765e70ac88f18e9cc417c8fcc02a5a8aa3a5a73b878dc5f94e`
- DOS character device name/signature: `%PCEPAT`
- device header strategy offset `1463h`, interrupt offset `1472h`

## High-level role

PCEPAT is a PC-Engine compatibility patch layer. Its manual says it must be
loaded early in `CONFIG.SYS`, before MSE:

```dos
DEVICE = PCEPAT.SYS
DEVICE = MSE312.SYS
```

The documented fixes target PC-Engine v1.05 on the original VA and v1.1 on
VA2/VA3/VA-91. They include:

- preserving the parent's attribute mode when starting `PCENGINE.COM` as a child;
- adding or fixing `PCENGINE /C` command execution and batch execution;
- allowing packed EXE/self-extracting EXE/Turbo Pascal 4+ programs to run;
- extending internal `CLS` and `BASIC` commands on PC-Engine v1.05;
- avoiding file-size zeroing during delete;
- fixing parts of the batch `FOR` command;
- adding the owner program path at the end of the environment block;
- correcting selected create-directory/create-file/temp-file/new-file paths;
- fixing an interrupt-disabled stack-switch race;
- repairing parent return from `SHELL`/`EXIT`;
- avoiding a BASIC startup hang when `ADVGBIOS.SYS` is absent;
- preserving registers for mouse BIOS `INT 33h, AH=00h` and communication BIOS
  `INT 8Ah, AH=08h`.

## Device-driver loading path

`PCEPAT.SYS` is a DOS character device. Its header starts with:

```text
+00  FFFF:FFFF next device pointer
+04  8000h     character-device attributes
+06  1463h     strategy entry
+08  1472h     interrupt entry
+0A  "%PCEPAT" device name/signature
```

The interrupt entry handles the DOS init request and selects one of several
installation profiles according to detected machine/PC-Engine state. The init
body finishes by setting the request status to `0100h`, like the other resident
service drivers in this utility-media stack.

The visible resident-size constants match the manual's three profiles:

```text
PC-88VA + PC-Engine v1.05        0B50h bytes
PC-88VA2/3 + PC-Engine v1.1      05E0h bytes
PC-88VA + PC-88VA-91 + v1.1      0580h bytes
```

## Resident signature and service chain

The driver name/signature `%PCEPAT` is intentionally exposed. MSE scans resident
service chains for this signature during its own install and warns if PCEPAT is
not present. This is why the utility-media recipe places PCEPAT before MSE.

The PCEPAT resident body also saves original vectors and far-entry bytes before
patching them. Most small resident handlers start by checking the function code
and tail-jump or far-call through saved original pointers when they do not own
the request.

## Installed hooks and patches

PCEPAT installs a mixture of IVT hooks and direct PC-Engine work-area patches.
The first-pass disassembly identifies these clear installation writes:

| Target | New target | Purpose from code/manual |
| --- | ---: | --- |
| `INT 33h` IVT entry (`0000:00CCh`) | `CS:0020h` | mouse BIOS wrapper; preserves registers for `AH=00h` |
| `INT 8Ah` IVT entry (`0000:0228h`) | `CS:0039h` | communication BIOS wrapper; preserves `DX` for `AH=08h` |
| `INT 21h` IVT entry (`0000:0084h`) | `CS:0059h` | DOS/PC-Engine function patch dispatcher |
| `INT 82h` IVT entry (`0000:0208h`) | `CS:0110h` | PC-Engine service patch |
| `INT 87h` IVT entry (`0000:021Ch`) | `CS:028Bh` | PC-Engine service patch |
| `1040:0025h` | far jump to `00A2h` | PC-Engine work-area patch |
| `1040:005Ch` | far jump to `022Eh` | PC-Engine work-area patch |
| `1040:001Bh` | far jump to `025Eh` | PC-Engine work-area patch |
| `1040:0048h` | far jump to `02A5h` | PC-Engine work-area patch |
| `1040:052Ah` | far jump to `02ACh` | PC-Engine work-area patch |
| `1040:05A7h` | far jump to `02DEh` | PC-Engine work-area patch |
| `1040:05B1h` | far jump to `02EDh` | PC-Engine work-area patch |
| `1040:0043h` | far jump to `0333h` | PC-Engine work-area patch |
| `1040:0066h` | far jump to `036Ah` | PC-Engine work-area patch |
| `1040:050Ch` | far jump to `0433h` | PC-Engine work-area patch |

Additional direct patches follow in the same installer family. The pattern is
consistent: copy the original byte/far pointer into resident storage, then write
an `EAh` far jump or IVT far pointer to the corresponding PCEPAT wrapper.

## ROM calls and PC-Engine dependence

PCEPAT is tightly coupled to PC-Engine ROM services. Several resident paths push
a continuation address and then far-jump to routines in the active ROM window,
for example:

```text
E000:0AFDh
E000:04DCh
E000:4359h
E000:432Eh
E000:0FFFh
E000:1084h
E000:11ECh
E000:0FE2h
E000:0819h
```

This is not standalone DOS code. It assumes the PC-Engine ROM/work area is
present, the current ROM-bank state is compatible with the expected service,
and segment `1040h` contains the PC-Engine work structures. For emulation this
means PCEPAT is a useful stress test for VA ROM banking and PC-Engine work-area
fidelity.

## INT 21h and DOS compatibility patches

The `INT 21h` wrapper handles several documented PC-Engine/DOS fixes:

- intercepts process termination `AH=4Ch` to clean user traps and restore shell
  work-area state before chaining;
- uses `AH=48h`, `49h`, and `4Ah` memory calls while adjusting child-process
  and environment memory layout;
- uses custom `AH=7Fh` and `AX=44E5h` PC-Engine extensions to save and restore
  text/attribute/graphic mode state around patched commands;
- recognizes `PCENGINE /C`, internal command text, and command strings copied
  from the environment/command tail;
- prepares owner-path data at the end of environment blocks.

The code also contains explicit command handling for the documented internal
command extensions: `CLS` accepts modes 1, 2, and 3, and `BASIC /G` suppresses
some graphics-screen clearing.

## ADVGBIOS and BASIC startup

The resident strings contain:

```text
Warning : ADVGBIOS.SYS is not installed.
ADVGBIOS
```

The installer/patch path checks for ADVGBIOS and prints this warning if absent.
The manual's item 13 says PCEPAT fixes a hang when starting BASIC without
`ADVGBIOS.SYS`; the disassembly shows this is handled by a mix of PC-Engine
work-area checks and calls through `E000:` ROM service entry points.

## Display and system-memory handling

Several paths manipulate VA display/system-memory mapping directly:

- reads/writes port `0153h`, masking the low system-memory select bits and
  temporarily selecting TVRAM (`... | 01h`);
- touches `A000:` memory while clearing screen/control data;
- uses `B000:` memory and segment `0040h` common-area state to compute display
  attributes;
- calls PC-Engine BIOS services via `INT 83h`, `INT 8Dh`, and `INT 94h`.

This matches the documented fixes for attribute mode inheritance, CLS/BASIC
screen behaviour, and v1.05 display-state bugs.

## Emulator implementation implications

For VAEG and PC-88VA compatibility work, PCEPAT stresses these areas:

1. **Load order matters**: MSE expects PCEPAT to already be resident and checks
   for `%PCEPAT`. The generated utility media should keep `PCEPAT.SYS` before
   `JFPPAT.SYS`, `TSCLVA.SYS`, and `MSE352B.COM`.
2. **PC-Engine ROM bank/window fidelity**: PCEPAT jumps into `E000:` ROM
   service entries and assumes the PC-Engine ROM/work-area state matches the
   model/version it detected.
3. **Segment `1040h` work area fidelity**: direct patching and state reads from
   `1040:` are central to the driver.
4. **IVT and BIOS-wrapper semantics**: `INT 21h`, `INT 33h`, `INT 8Ah`, `INT
   82h`, and `INT 87h` hooks must be preserved through TSR loading and must
   chain correctly.
5. **Display mapping via port `0153h`**: temporary TVRAM selection and restore
   must not corrupt the VA system-memory map.
6. **ADVGBIOS absence should be safe**: the warning path is expected; missing
   ADVGBIOS should not hang BASIC startup when PCEPAT is installed.

## Current confidence and open points

The device header, major installed hooks, PC-Engine work-area patches, ROM
far-call dependence, ADVGBIOS warning, and key DOS/display patch paths are
directly supported by static disassembly and the bundled manual. The exact
function matrix behind every work-area patch still needs a deeper pass with
runtime traces on PC-Engine v1.05 and v1.1 and comparison against the VA, VA2/3,
and VA-91 resident-size profiles.
