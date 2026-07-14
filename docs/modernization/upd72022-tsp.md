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
# NEC uPD72022 and the PC-88VA TSP

This note records the available NEC documentation for the uPD72022 and relates
it to vaeg's PC-88VA Text Sprite Processor (TSP) implementation. The uPD72022
is a display processor. It is unrelated to the PC-88VA3 2TD floppy controller.

## Terminology

NEC names the uPD72022 an **Intelligent Display Processor (IDP)**. The active
vaeg tree names the corresponding PC-88VA functional block the **Text Sprite
Processor (TSP)** in `iova/tsp.c` and `iova/tsp.h`.

These names describe substantially overlapping responsibilities:

- CRT raster and synchronization control;
- text and semigraphics display;
- text attributes and cursor control;
- sprite attributes, display, grouping, and collision status;
- display-memory addressing and host transfers.

The uPD72022 documentation is therefore a strong semantic reference for the
VA TSP. It must not yet be treated as proof that every PC-88VA model contains
an unmodified commercial uPD72022. Contemporary PC-88VA descriptions call its
VDP and IDP custom LSIs, and vaeg contains comments recording behavior that
differs from the uPD72022 data sheet. Board markings, an NEC VA schematic, or a
VA-specific parts list are still needed to establish the exact silicon
relationship.

## Primary Documentation

The best available source is NEC Electronics Inc., *Intelligent Peripheral
Devices (IPD) Data Book*, 1989-1990 edition, June 1989, Document No. 50051.
The complete uPD72022 chapter occupies printed pages 3-57 through 3-96.

- [Bitsavers scan](https://www.bitsavers.org/components/nec/_dataBooks/1989_NEC_Intelligent_Peripheral_Devices_Data_Book.pdf)
- [Internet Archive mirror with OCR text](https://archive.org/details/bitsavers_necdataBootPeripheralDevicesDataBook_33483764)
- [Searchable Bitsavers mirror](https://ftpmirror.your.org/pub/misc/bitsavers/pdf/nec/_dataBooks/1989_NEC_Intelligent_Peripheral_Devices_Data_Book.pdf)
- [Standalone data-sheet index](https://datasheet4u.com/datasheet/NEC-Electronics/UPD72022-521631)

The Internet Archive item supplies the original 570-page PDF and generated OCR
text. The scan is preferable to third-party summaries because it includes the
pinout, command parameters, timing diagrams, electrical characteristics, and
package drawings.

A contemporary Japanese design article also exists:

- I. Kozono et al., "Intelligent display processor uPD72022," *NEC Technical
  Journal*, Vol. 39, No. 10, October 1986, pp. 64-69.

The article's bibliographic record is preserved in the non-patent citations of
[WO2008076950A3](https://patents.google.com/patent/WO2008076950A3/en). A freely
accessible scan of the article was not located during this investigation.

No uPD72022 file is present in the
[ChipDB NEC index](https://datasheets.chipdb.org/NEC/) that hosts the V20/V30
and V40/V50 manuals. For this device, Bitsavers is the more complete source.

## Published Device Capabilities

The data book gives the following headline capabilities:

| Property | uPD72022 specification |
|---|---|
| Display modes | Text, semigraphics, graphics |
| Display split | Four horizontal screen areas |
| Scrolling | Smooth horizontal and vertical scrolling |
| Sprites | Attribute-based sprite display, grouping, collision and overflow status |
| Colors | 16-color output, 4 bits per dot |
| Attributes | Up to seven attributes |
| Maximum horizontal resolution | 640 dots |
| Maximum vertical resolution | 512 dots |
| Maximum dot rate | 22MHz |
| Addressable display memory | Up to 256K x 16-bit words |
| Memory support | DRAM refresh and optional dual-port RAM |
| Host transfers | Programmed I/O and DMA |
| Synchronization | Programmable CRT timing, external horizontal/vertical sync |
| Commands | 22 display, memory, and sprite commands |
| Process and supply | CMOS, single +5V supply |
| Packages | 80-pin plastic miniflat or 68-pin PLCC |

The maximum addressable display memory is 512KiB. This is a device limit, not a
statement that every PC-88VA exposes 512KiB of text/sprite RAM. The documented
VA configuration is 64KiB of TVRAM on VA and 256KiB on VA2/VA3; see
[`virtual-machine-architecture.md`](virtual-machine-architecture.md).

## Internal Architecture

The data book divides the uPD72022 into three units:

1. **Host processor interface unit**: exchanges commands, parameters, data,
   and status; controls the asynchronous bus, DMA, and interrupts.
2. **Control processor unit**: decodes commands and operates on display data,
   display addresses, and screen-control state.
3. **Display control unit**: produces display-memory addresses, pixel output,
   CRT synchronization, blanking, and internal display timing.

This separation is useful for future vaeg work. Command decoding, display
memory operations, and raster scheduling should not be collapsed into one
immediate register-write side effect when BUSY, DMA, interrupt, or command
completion timing is guest-visible.

## Command Set

The complete documented command set is:

| Category | Command | Code | Purpose |
|---|---|---:|---|
| Initialization | `SYNC` | `10h` | Select mode and define scan timing |
| Display | `DSPON` | `12h` | Enable the display controller |
| Display | `DSPOFF` | `13h` | Disable the display controller |
| Display | `DSPDEF` | `14h` | Define screen layout and display format |
| Display | `CURDEF` | `15h` | Define cursor format |
| Display | `ACTSCR` | `16h` | Select the active screen area |
| Display | `LPNR` | `1Ah` | Read the light-pen position |
| Display | `CURS` | `1Eh` | Move the cursor |
| Sprite | `SPRRD` | `80h` | Read sprite attributes |
| Sprite | `SPROV` | `81h` | Read collision and line-overflow status |
| Sprite | `SPRON` | `82h` | Enable sprite display and set its base/options |
| Sprite | `SPROFF` | `83h` | Disable sprite display |
| Sprite | `SPRWR` | `84h` | Write sprite attributes |
| Sprite | `SPRSW` | `85h` | Enable or disable one sprite |
| Memory | `EXIT` | `88h` | Terminate a display-memory operation |
| Memory | `MASK` | `89h` | Set the display-memory write mask |
| Memory | `DPRD` | `8Ah` | Read the display-memory data pointer |
| Memory | `DPLD` | `8Eh`/`8Fh` | Load a display-memory address or offset |
| Memory | `RDAT` | `90h`-`93h` | Read display memory through the host interface |
| Memory | `WDAT` | `94h`-`97h` | Write display memory through the host interface |
| Memory | `BLKTOT` | `99h`-`9Bh` | Transfer display memory to the host using DMA |
| Memory | `BLKTIN` | `9Dh`-`9Fh` | Transfer host data to display memory using DMA |

The multiple command codes select transfer width or data-pointer update
behavior. Their bit-level meanings and parameter formats should be taken from
the command descriptions on printed pages 3-78 through 3-88 rather than
inferred from this summary.

## PC-88VA Relationship

Contemporary PC-88VA descriptions identify two display processors:

- a VDP for graphics operations;
- an IDP for text, display control, and sprites.

A surviving summary of the April 1987 *Monthly ASCII* PC-88VA article describes
the IDP as providing Japanese text and text-side sprites. A later October 1987
summary attributes screen splitting, smooth scrolling, display composition,
and up to 32 sprites to the combined VDP/IDP display architecture:

- [April 1987 PC-88VA article summary](https://eccmemberblog2.seesaa.net/article/2021-09-23.html)
- [October 1987 PC-88VA article summary](https://eccmemberblog2.seesaa.net/article/2022-01-21.html)

The published uPD72022 capabilities closely match the TSP command vocabulary
used by the VA ROM and current vaeg implementation. This is strong functional
evidence. It is not sufficient physical evidence to claim that the VA's custom
IDP and retail uPD72022 are electrically identical.

## Current vaeg Implementation

### Ownership and rendering path

The active implementation is divided as follows:

| File | Current responsibility |
|---|---|
| [`iova/tsp.c`](../../iova/tsp.c) | Command/parameter ports, TSP state, SYNC decoding, raster timing |
| [`iova/tsp.h`](../../iova/tsp.h) | TSP state shared with the display and renderer paths |
| [`vramva/maketextva.c`](../../vramva/maketextva.c) | Text and attribute rendering from `textmem` |
| [`vramva/makesprva.c`](../../vramva/makesprva.c) | Sprite table interpretation and sprite rendering |
| [`pccore.c`](../../pccore.c) | Display/VBlank event scheduling and blink progression |
| [`iova/sysportva.c`](../../iova/sysportva.c) | System-port view of the display synchronization state |

The guest-facing TSP and the host renderer are separate. SDL2 effects and
window scaling must not alter TSP timing or guest display state.

### VA I/O mapping

`tsp_bind()` currently exposes:

| VA port | Direction | Current use |
|---:|---|---|
| `0142h` | Read | Status, including BUSY/VBlank approximation |
| `0142h` | Write | Command byte |
| `0143h` | Read | Unimplemented; returns `FFh` |
| `0146h` | Write | Command parameter or sprite-attribute data |

These are PC-88VA board-level I/O assignments. They are not package pin
numbers or universal uPD72022 host addresses. The VA ROM boot sequence writes
commands and parameters through this interface as described in
[`pc88va-boot-sequence.md`](pc88va-boot-sequence.md).

### Implemented command coverage

The command decoder currently provides functional handling for:

- `SYNC` (`10h`);
- `DSPON` (`12h`);
- `DSPDEF` (`14h`);
- `CURDEF` (`15h`);
- `SPRON` (`82h`);
- `SPRWR`-like sequential sprite-table writes under the local name `SPRDEF`
  (`84h`);
- `EXIT` (`88h`).

`DSPOFF` and `SPROFF` have constants but no command cases. All other commands
fall through the unknown-command path. The implementation therefore does not
yet model active-screen selection, cursor movement, light-pen reads, sprite
status/read/switch operations, display-memory programmed I/O, DMA, masking,
or controller interrupts.

The 22-command data-book chapter is useful not only as background but as a
concrete completeness checklist for the TSP core.

### Timing model

`tsp_updateclock()` derives horizontal and vertical periods from the 14-byte
`SYNC` parameter block. It uses VA-specific dot-clock values for the supported
24.8kHz, 15.73kHz, and 15.98kHz modes, then schedules display and VBlank events
through `pccore.c`.

The implementation deliberately contains VA-specific behavior. Its horizontal
width calculation states that left and right border fields appear on real
hardware not to receive the `+1` treatment described by the uPD72022 data
sheet. This is direct evidence that the data book is a reference rather than a
drop-in executable specification for every VA detail.

Other timing comments identify unresolved approximations:

- cursor/text blink timing is adjusted because the documented value appears
  too slow when used directly;
- the original VA can initialize a 200-line SYNC block while operating in a
  24kHz state, requiring a system-port VBlank compatibility adjustment;
- one 15kHz dot-clock value is tuned toward observed real-machine timing.

These behaviors should be preserved until replaced by better measurements,
not mechanically overwritten with generic uPD72022 equations.

## Audit Findings for Future Work

The source and data book expose several areas that need focused tests before
the TSP implementation is expanded:

1. Verify command and parameter buffering against ROM traces for all supported
   commands.
2. Verify `0142h` BUSY and VBlank status independently, including operator
   precedence in the current status expression.
3. Determine the meaning of the unimplemented `0143h` read path from the VA
   technical manual or ROM reads.
4. Distinguish VA-specific `SPRDEF` behavior from the documented `SPRWR`
   command and its sprite-number/attribute-number parameters.
5. Implement `DSPOFF` and `SPROFF` before relying on display-disable behavior.
6. Add `ACTSCR`, cursor, sprite status, and collision/overflow behavior based
   on software that exercises them.
7. Keep display-memory DMA and interrupt support separate from immediate text
   rendering so command completion remains observable.
8. Compare VA, VA2, and VA3 TVRAM size and address decoding without assuming
   the uPD72022 maximum memory size is fully installed.

The active code also deserves ROM-less unit coverage for command parameter
counts, status transitions, sprite-table bounds, malformed command streams,
and timing arithmetic. No implementation changes are made by this document.

## Verification Sources Needed

The following evidence would resolve the remaining architectural uncertainty:

- readable VA/VA2/VA3 motherboard photographs showing the display LSI marks;
- NEC schematics or parts lists identifying the TSP/IDP package;
- the original 1986 NEC Technical Journal article;
- VA technical-manual pages defining ports `0142h`, `0143h`, and `0146h`;
- logic-analyzer traces for command writes, BUSY, VBlank, interrupt, and DMA;
- software that exercises all sprite collision, overflow, active-screen, and
  display-memory transfer commands;
- model-by-model tests of text/sprite RAM wrapping and open-bus behavior.

Until that evidence is available, the 1989 data book should be cited as the
primary semantic reference for the VA TSP while VA-specific observed behavior
retains priority where the two differ.
