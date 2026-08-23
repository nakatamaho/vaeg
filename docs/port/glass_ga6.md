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

# GLASS ORBIT GA-6 source-window page exchange

## Scope

GA-6 verifies the planned G0 double-buffer geometry in VAEG without assigning
a physical timing guarantee. One FB0 source is defined as 640 by 400 packed
4bpp, then a 640 by 200 G0 window selects either of its two source halves.
The two page images are deliberately distinct:

| logical page | source Y | SGP base | packed word | palette result |
| --- | ---: | ---: | ---: | --- |
| A | 0 | `200000h` | `1111h` | default palette index 1, blue |
| B | 200 | `20FA00h` | `2222h` | default palette index 2, red |

Each half contains `7D00h` words: `640 * 200 / 4`. The SGP clear list for
each is the GA-5-proven `SET_WORK`, `SET_COLOR`, `CLS`, `END` sequence.

## PC-88VA Graphics BIOS ownership

The raw payload calls these PC-88VA Graphics BIOS functions through `INT 8Fh`:

| function | inputs | use in GA-6 |
| --- | --- | --- |
| `$ScnMode` | `AH=00h`, `BX=A002h`, `CX=0004h` | Enter single-plane G0 640x200 packed 4bpp mode. |
| `$ScnDsp` | `AH=0Bh`, `AL=0/1` | Hide graphics while the source is defined, enable it after page A is selected, and disable it on ESC. |
| `$ResPal` | `AH=0Ah` | Restore the documented standard palette. |
| `$DefBuf` | `AH=01h`, `AL=0`, `CX=1`, `ES:DI -> (4,640,400)` | Define FB0's full two-page source. |
| `$DefWin` | `AH=02h`, `AL=0`, `CX=1`, `ES:DI -> (0,0,200,0,0)` | Define one 200-line G0 window. |
| `$Compose` | `AH=03h`, `AL=0`, `CX=0003h` | Show G0 only. |
| `$RollTo` | `AH=05h`, `AL=0`, `CL=0`, `BX=0`, `DX=0/200` | Select page A or B inside FB0. |

The local PC-88VA Graphics BIOS reference specifies that `$ScnMode` resets
buffer/window/composition definitions, `$DefBuf` supplies pixel size/width/
height, `$DefWin` supplies FB number/window geometry/source offset, and
`$RollTo` sets an absolute source display origin. `docs/port/va_video_contract.md`
derives the coupled FB0 `OFY`/`DSA` consequence. GA-6 therefore makes no raw
DSA-only write: every page selection occurs by `$RollTo` after TSP status
`0142h` bit 6 has observed display-to-vblank transition.

Evidence: `[VA-TEKU:606GRP.TXT sections 6.6.2-6.6.3, functions 0-5 and 11]`,
`[VA-TEKU:4.TXT section 4.4.5]`, and the tracked
[VA video contract](va_video_contract.md). The local technical manual is not
cited from source comments.

## ESC exit

The normal `GLASSP6.COM` loop uses the PC-88VA Keyboard BIOS rather than DOS
input:

| function | inputs | result used |
| --- | --- | --- |
| `$PrmSnsK` | `INT 82h`, `AH=0Ah` | Carry means no pending primitive key. |
| `$PrmGetK` | `INT 82h`, `AH=09h` | The existing VA demo convention identifies ESC with returned `AH=00h`. |

On ESC the payload invokes Graphics BIOS `$ScnDsp(AL=0)` and restores the
loader's original stack continuation. It then returns to the invoking local
COM command path without executing a DOS interrupt from the bare payload.
This is a loader ABI for the local validation image, not a PC-88VA firmware
claim.

Evidence: `[VA-TEKU:603KEYB.TXT section 6.3.1]` identifies primitive sense
and get-key functions 0Ah and 09h; the returned-ESC convention is an existing
verified VAEG demo convention and remains `hardware_pending` until a real VA
comparison.

## Deterministic VAEG captures

`GLASSP6.COM` alternates source page A and B once per observed VBlank. It is
the human-facing program. For deterministic capture, the build also supplies:

| program | action |
| --- | --- |
| `GLASSP6A.COM` | initialize both pages and leave page A selected at the GA-6 checkpoint |
| `GLASSP6B.COM` | initialize both pages and leave page B selected at the GA-6 checkpoint |

The host runner starts a fresh local boot for each program and checks its
captured output. The validator requires the entire 640 by 200 visible region
to have the selected colour, verifies VAEG's 200-line composition separators,
and rejects a mixed, stale, or partial page. Distinct red and blue samples are
also required.

```text
demos/va/glass-orbit/run-vaeg-ga6.sh \
  SOURCE_BOOTABLE_2HD.d88 VAEG ROM_DIRECTORY OUTPUT_DIRECTORY
```

The image generated by `build-ga6-bootable-d88.sh` is bootable only because it
uses a maintainer-local PC-Engine template. It is an untracked local test
artifact and must not be committed.

## Boundary

Passing the GA-6 host check means that VAEG's documented Graphics BIOS
source-window path displays the two expected 640 by 200 source regions
without a visible mixture. It does not establish PC-88VA cycle timing,
tear-free hardware behavior, or real-hardware conformance. Those remain
`hardware_pending`.
