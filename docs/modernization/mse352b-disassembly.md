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
# MSE 3.52B disassembly notes

This note records a first-pass static disassembly of **MSE 3.52B**, the
PC-Engine resident emulator that runs MS-DOS/PC-98-style applications on
PC-88VA. The binary is not stored in this repository; the examined executable
was built locally from the archived `mse352a.lzh` plus `mse352bf.wup` recipe
used by the utility-media builder.

Examined artifact:

- `MSE352B.COM`, size `21027` bytes
- SHA-256: `794375496c62bf8f508ccbf57c8ceeb2ab439606d31638eea397eaac6bd3e68a`
- disassembled as a 16-bit `.COM` image loaded at `0100h`

## High-level role

MSE is a DOS resident compatibility layer, not a full machine emulator. It
hooks DOS/PC-Engine/BIOS entry points, rewrites selected vectors and BIOS work
entries, and emulates enough MS-DOS/PC-98 behaviour for DOS tools to run under
PC-Engine on PC-88VA. Its own manual describes it as software for running
MS-DOS applications on 88VA.

Important options observed in both the manual and code:

- `/Vn.nn` sets the DOS version reported by MSE.
- `/A` enables Alias support and stores alias data in BMS.
- `/B` swaps part of MSE's code/data into BMS.
- `/X` additionally places a small code-swap block in an XMS-provided UMB; the
  code requires `/B` to have been selected first.
- `/K` samples the keyboard while installing: `q` aborts installation, and `b`
  disables BMS usage.

## Startup and resident/unresident flow

The `.COM` starts at `0104h` and jumps to the installer at `0141h`. The first
major check uses `INT 21h, AH=35h, AL=DCh` to fetch vector `INT DCh`; MSE then
compares the target data at `ES:000A` with the resident signature
`"%MSE.SYS"`.

If an existing MSE resident is found, the code enters an uninstall path:

- validates the embedded version word at resident offset `0035h` (`0352h` for
  3.52);
- restores vectors from the resident copy back into the interrupt vector table;
- releases BMS swap allocations with BMSDR `INT E8h, AH=22h` when present;
- releases the XMS/UMB allocation through the XMS entry obtained from
  `INT 2Fh, AX=4310h`;
- releases the DOS memory block with `INT 21h, AH=49h`, then exits.

If no resident is found, the installer patches its PSP/MCB context, initializes
internal segment constants, installs hooks, optionally allocates BMS/XMS-backed
swap blocks, and finally stays resident with `INT 21h, AX=3100h`.

## Installed hooks

The installer saves old vectors and writes new far pointers into the interrupt
vector table. Clearly visible vector writes include:

| IVT offset | Interrupt | New handler | Notes |
| --- | ---: | ---: | --- |
| `0028h` | `INT 0Ah` | `2C91h` | saved at resident `2CB6h` |
| `0084h` | `INT 21h` | `2458h` | DOS API emulation/trapping |
| `00B8h` | `INT 2Eh` | `2CBAh` | command-shell / `COMSPEC` handling |
| `00A4h` | `INT 29h` | `3025h` | fast console output path |
| `0370h` | `INT DCh` | `0A0Ch` | MSE resident signature/service vector |
| `0060h` | `INT 18h` | `2F6Ah` | BIOS/service compatibility path |
| `0068h` | `INT 1Ah` | `0A11h` | BIOS timer/clock compatibility path |

It also patches PC-Engine/BIOS work entries in segment `1040h`, replacing
several 5-byte entry points with far jumps (`EAh`) to resident handlers. The
original pointers/bytes are preserved for uninstall.

The installer scans for PCEPAT by following an existing vector chain and
looking for the signature `"%PCEPAT "`. If absent, it prints the warning
`PCEPAT.SYS is not installed.` but continues.

## BMS and XMS usage

MSE relies on **BMSDR** through `INT E8h`, matching the BMS Driver 1.50
interface:

- `AH=FFh`: resident/status check; success returns `AX=BA09h`, BMSDR version in
  `BX`, revision in `CX`, and the bank-select I/O port in `DX`.
- `AH=21h`: allocate BMS as memory. Input `DX=paragraphs`; output `AL=bank`,
  `BX=segment`.
- `AH=22h`: free allocated BMS memory. Input `AL=bank`, `DX=segment`.

At install, MSE stores the BMS port returned in `DX` into many runtime stubs.
Whenever it copies or uses a BMS-resident block, it selects the bank by
`OUT DX,AL`, accesses the returned segment, and then deselects with `AL=0`.
For VAEG this means both the BMS port (`01D0h` on the native VA recipe, or the
compatibility port when configured) and the `80000h-9FFFFh` BMS aperture must be
coherent during install and later resident calls.

### `/A`: Alias buffer in BMS

The `/A` path sets a flag at `4D38h` and calls the routine at `4DC9h`. That
routine:

1. verifies BMS is available (`word [22E7h] != 0`);
2. requests a BMS memory block with `INT E8h, AH=21h`;
3. records the returned bank at `22D9h` and segment at `22F5h`/`07C4h`;
4. selects the bank through the BMS port;
5. copies an alias-manager header from `4079h` for `00EBh` bytes;
6. copies custom alias data from `527Eh` for the length stored at `527Ch`;
7. deselects the bank and reports success.

The code refuses `/A` if no alias data has been embedded by `MSECUST`; this
matches the manual and the existing release note that stock `MSE352B.COM` has
no generic alias payload.

### `/B`: code swap in BMS

The `/B` path sets a flag at `4D39h` and calls the routine at `4BD0h`. That
routine is the main BMS code-swap allocator:

1. computes the required size from `0BD5h + 009Ch` bytes, rounded to paragraphs;
2. calls `INT E8h, AH=21h` to allocate BMS memory;
3. records the returned bank/segment at multiple resident call sites;
4. patches many internal offsets so later handlers call the swapped copy;
5. selects the allocated BMS bank with `OUT DX,AL`;
6. copies a `009Ch`-byte swap header from `51E0h` and `0BC6h` bytes of code from
   `34C8h` to the BMS segment;
7. deselects bank `0` and returns success.

This explains why `/B` is sensitive to BMS emulation correctness: install-time
success is not enough. Later interrupt handlers execute through patched stubs
whose target bytes live behind a selected BMS bank.

### `/X`: XMS/UMB code swap

The `/X` path only runs after `/B` is selected. It probes XMS via
`INT 2Fh, AX=4300h` and obtains the XMS entry point with `AX=4310h`. The code
then calls the XMS entry with `AH=10h`, which is the XMS **Request UMB**
function, not a normal extended-memory-block allocation. It asks for enough
paragraphs to hold `042Bh` bytes.

If XMS returns a UMB segment high enough for the intended high-memory swap, MSE
copies `042Bh` bytes from `309Dh` to that segment and patches two call-site
pointers (`021Bh`, `483Bh`) to use the copied block. If the XMS probe or UMB
request fails, MSE prints an XMS warning and continues without that swap.

## INT 21h behaviour visible from strings/code

The resident DOS handler contains explicit diagnostics and compatibility
handling around memory and process APIs. One visible string is:

```text
Illegal Int 21h AX=49h, You can't release environment memory!
```

This is consistent with a guard in the DOS emulation layer: MSE traps selected
DOS memory-management calls and prevents programs from freeing the environment
block incorrectly. Other visible strings and code paths show support for:

- `COMSPEC=` lookup for `INT 2Eh` shell execution;
- `TMP=` lookup and a temporary file name `PCE$$$$$.BAT` for pipe/batch support;
- output/input redirection parsing (`>`, `>>`, `2>`, `2>>`, `<`), with
  `Redirect ERROR !!` diagnostics;
- version-dependent expansion of DOS functions mentioned in the manual
  (`1Fh`, `32h`, `52h`, `60h`) when `/V` is set to DOS 3.1 or later.

## Emulator implementation implications

For PC-88VA/PC-98/MS-DOS emulation work, MSE stresses these areas:

1. **BMSDR contract**: `INT E8h` functions `FFh`, `21h`, and `22h` must agree
   with the BMS port and aperture mapping. `/B` stores executable resident code
   in BMS, so stale bank selection or wrong aperture aliasing will crash later.
2. **Native VA BMS port**: the modern utility recipe uses BMSDRVA with `/P`, so
   the port is expected to be `01D0h` rather than only the older `00ECh` path.
3. **Vector and BIOS-work patching**: MSE writes the real IVT and selected
   PC-Engine work entries. Snapshot/restore, reset, and TSR removal paths must
   preserve ordinary real-mode vector semantics.
4. **XMS is UMB-facing here**: `/X` uses the XMS UMB request (`AH=10h`) for a
   small high-memory code block. An emulator that implements only EMB-oriented
   XMS calls will make `/X` fail gracefully, but should not corrupt MSE state.
5. **PCEPAT ordering matters**: MSE only warns if PCEPAT is absent, but the
   documentation and utility-media recipe place `PCEPAT.SYS` before MSE because
   MSE assumes the PC-Engine patch layer's services and vectors are already in
   their final form.

## Current confidence and open points

The install, uninstall, hook, BMS, and XMS paths above are directly supported by
static disassembly. The full `INT 21h` emulation matrix is broader than this
first pass; a complete map should label each handler table and compare runtime
traces from representative DOS/PC-98 programs. The exact cause of the known
`/B` interaction with the expanded utility-media recipe should be debugged with
BMS bank-select traces around the `4BD0h` copy and later calls through patched
stubs.
