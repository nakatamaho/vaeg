<!--
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.
-->

# GLASS ORBIT GA-2 packed-4bpp CPU-fill proof

GA-2 proves only the first visible GLASS graphics boundary on VAEG:

```text
Graphics BIOS mode entry
  -> documented CPU GVRAM mapping
  -> 640 x 200 packed-4bpp CPU fill
  -> captured renderer validation
```

It does not submit an SGP command, define a second framebuffer, change the
display start address, poll vertical blank, or make a real-PC-88VA performance
or conformance claim.

## Graphics BIOS contract

The raw `GLASSG2.BIN` payload runs at the approved `2000:0000` boundary and
uses only the following Graphics BIOS calls through `INT 8Fh`:

| Function | Inputs | Purpose | Result required |
|---|---|---|---|
| `$ScnMode` | `AH=00h`, `BX=A002h`, `CX=0004h`, `DX=0000h` | G0 640 x 200, single-plane, graphics enabled, 4bpp | `AX=0` |
| `$ResPal` | `AH=0Ah` | Restore the documented default palette | `AX=0` |
| `$Compose` | `AH=03h`, `AL=0`, `CX=0003h` | Show G0 as the only compositor input | `AX=0` |
| `$ScnDsp` | `AH=0Bh`, `AL=1` | Enable graphics display | `AX=0` |

`BX=A002h` is derived from `$ScnMode`: bit 15 selects single-plane, bit 13
enables graphics, bit 3 clear selects 640 dots for G0, and bits 1:0 value `10b`
selects 200 lines. `CL=4` selects a four-bit G0 pixel size. `$ScnMode` resets
framebuffer, window, and composition defaults, so GA-2 relies on its
documented default FB0: 640 by 200, four bits per pixel, beginning at
`A000:0000`.

Evidence: `[VA-TEKU:606GRP.TXT sections 6.6.2-6.6.3, functions 0, 3, 10, 11]`.
The local Technical Manual is maintainer-local; it is cited here, not from
source comments.

## CPU write proof

After Graphics BIOS mode entry, GA-2 uses the established single-plane CPU
mapping sequence:

```text
OUT 0153h <- 54h
OUT 0580h <- 10h
ES = A000h
REP STOSW, CX = 7D00h, AX = 5555h
```

`0x7d00` words are exactly `640 * 200 / 4`, because four 4bpp pixels occupy a
word. `0x5555` therefore fills every pixel with palette index 5. The first
four bytes are then overwritten with `12h 34h 56h 78h`, whose documented
high-nibble-first interpretation is the pixel sequence `1,2,3,4,5,6,7,8`.

The mapping values are established by the existing direct-GVRAM visual probes,
not inferred from a PC-98 GRCG interface:
`[SRC:qa/sgp/scan-left-right/src/scanlr.asm:32-45,125-188]` and
`[SRC:demos/sgp-wireframe/sgp_wireframe.asm]`. The packed layout follows
`[VA-TEKU:4.TXT sections 4.4.3, 4.4.6]`.

## Local VAEG proof

`build-ga2.sh` builds two local artifacts:

```text
GLASSG2.BIN  raw payload, copied to 2000:0000
GLASSP2.COM  local PC-Engine loader only
```

`build-ga2-bootable-d88.sh` and `run-vaeg-ga2.sh` create a private bootable
test disk from a caller-supplied PC-Engine template. The image, ROMs, logs,
and captures are local integration artifacts and are never committed.

`tools/repo/find_unreferenced.py --report` lists the two GA-2 assembly sources
because they are intentionally built by an explicit local NASM script rather
than a production CMake root. Their owners are `build-ga2.sh` and the local
loader path above; they are not deletion candidates.

`glass_orbit_ga2.debug` waits for the fixed `2000:0100` idle location and
captures registers plus the rendered screen. `tools/verify-ga2-capture.py`
fails closed unless all of these hold:

1. the raw payload has its `4742h` success marker and the approved segment /
   stack boundary;
2. a 640-pixel rendered viewport with at least 400 guest rows is captured;
3. each of the 200 logical G0 rows is a complete palette-index-5 fill except
   for the seven non-cyan top-left probe pixels, while the 200 intervening
   rows in VAEG's 640 by 400 composition canvas remain black; and
4. the first eight pixels visibly classify as the documented default palette
   sequence 1 through 8.

The image is also visually inspected before a GA-2 PASS claim. This validates
VAEG's rendering path only. Exact nibble behavior, palette output, and mode
entry on a physical PC-88VA remain `hardware_pending`.

The alternating rendered rows are a VAEG observation for this selected
200-line mode, not a claim about physical display timing. They make the
logical 200-row bounds directly measurable in the 640 by 400 capture canvas.

## Lifecycle restriction

GA-2 ends in its fixed idle loop because it is a capture-only proof. It does
not return to PC-Engine or attempt to restore a previous display state. The
approved design remains that a later interactive GLASS stage will use the same
Graphics BIOS ownership for both mode entry and leaving graphics mode; GA-2
does not define that exit contract.
