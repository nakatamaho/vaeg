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
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
-->

# NEON4 faithful PC-9801 to PC-88VA audit

This audit restarts the port from the current `main` baseline. The original
`demos/NEON4_1_0/` directory is local reference material and is not modified.
The earlier simplified port is not used as evidence or as an implementation
base.

## Source inventory

| Role | PC-98 source | Finding for VA port |
|---|---|---|
| Entry/options | `NEON4_16.ASM`, `COMMAND4_16.INC`, `OPTIONS4_16.INC` | Keep the eight-scene timeline, DOS command-line shape only where meaningful, and English status text; remove PC-98 guards |
| Frame scheduler | `FRAME_RENDER4_LOW.INC`, `AFS4_286.INC` | Replace IRQ2/INT 0Ah with VA TSP VBLANK polling; retain logical frame/scene timing |
| Scene selection | `SCENE4_256.INC` | Preserve the eight scene order and chapter frame lengths |
| Geometry | `GEOM4_LOW.INC`, `GEOM4_CORE.INC` | Preserve tetra/cube/octa/icosa/corona/panel/ribbon geometry; convert coordinates from 640x400 to 320x200 |
| PC-98 video | `VIDEO4_LOW.INC`, `VIDEO4_256.INC`, `VIDEO256_PACKED*.INC`, `BLIT4_256.INC` | Do not port GRCG/EGC/PEGC register code or bank windows |
| Text | `TEXT4_LOW.INC`, `TEXT4_256.INC` | Do not write PC-98 text VRAM; use DOS console text before entering graphics or defer VA text overlay |
| Sound | `OPNA.INC`, `OPL3.INC`, `MUSIC_*.INC` | Do not probe PC-98 ports; use VA Music BIOS only after the graphics path is stable |
| State/data | `DATA4.INC`, `VIDEO4_DATA.INC`, `LOW4_DATA.INC` | Copy only scene constants and palette intent; no PEGC page or PC-98 latch state |

## Hardware difference table

| Topic | PC-9801 evidence | PC-88VA evidence | Required conversion |
|---|---|---|---|
| 16-colour framebuffer | GRCG/EGC planar VRAM and A000/B000 windows (`docs/98io/io_disp.txt`, `io_egc.txt`) | Graphics BIOS `INT 8Fh`; SGP is single-plane (`docs/tekumani/606GRP.TXT`, `4.TXT`) | Use VA 320x200 4bpp G0/G1 and SGP descriptors; no plane registers |
| 256-colour path | PEGC MMIO at E0000h, bank/page selectors (`VIDEO4_256.INC`, `docs/98io/io_disp.txt`) | VA graphics BIOS has 8bpp modes, but this task has no verified SGP 8bpp page contract | Do not translate PEGC; keep 8bpp as a separately audited follow-up |
| Drawing | CPU writes plus GRCG/EGC logical operations | SGP command `LINE=0009h`, `CLS=000Ah`, `SET_WORK=0003h` | Generate main-RAM command lists; use LINE edges and CLS only |
| Fill | EGC/GRCG fills and raster spans | No verified SGP flood-fill in the port contract | Replace filled faces with ordered wireframe edges; do not fake pixels in CPU loops |
| Command memory | Device/window state in PC-98 VRAM | SGP list in main RAM, 58-byte work area, word command ports `0500h/0502h` | Emit `SET_WORK` first; use WORD I/O for command pointer |
| Page exchange | PC-98 page selectors and IRQ2 scheduler | VA G1 DSA1 word registers `022Eh/0230h`, TSP VBLANK status `0142h` | Render hidden G1 page, wait SGP then VBLANK, exchange DSA1 |
| Composition | PC-98 GDC/text/graphics priority | VA Compose IDs: text, sprite, G0, G1 (`606GRP.TXT`) | Compose G1 over G0; leave G0 black unless the original scene explicitly draws a carrier |
| Background | Original 16-colour build clears the page and draws scene-specific raster panels; it does not use a permanent checkerboard | VA G0 may be CPU-initialized for bring-up, but no checkerboard is part of NEON4 | Use black G0 and reproduce scene carrier/panel geometry as lines on G1 |
| VSync | IRQ2/INT 0Ah and port 0064h (`AFS4_286.INC`, `docs/98io/io_disp.txt`) | TSP VBLANK status | Poll VA status; no PIC/vector hook |
| Keyboard exit | Original main loop uses DOS `INT 21h`, AH=06h, DL=FFh and exits on available input | VA DOS path remains available to a COM | Use the same DOS polling path and require ASCII ESC (1Bh) |
| Sound | Direct OPN/OPNA/OPL3 probes and PC-98 board ports | Music BIOS `INT 8Bh`; AH=00 initialization required (`611MUSIC.TXT`) | Add optional BIOS adapter only after graphics; no direct port detection |

## Faithful scene mapping

The original scene timeline is eight chapters:

1. SIGNAL SEED — raster carrier plus tetra seed.
2. FACET ASSEMBLY — rotating solid and far/near wire cage.
3. MATERIAL ASSEMBLY — multiple solids and satellites.
4. MORPH GATE — changing convex geometry.
5. RASTER TRANSFER — moving carrier panels.
6. SURFACE WAVE — ribbons and FM visualizer intent.
7. GRID ARRIVAL — perspective floor/grid.
8. SOLID FINALE — corona and central solid/shutter.

The VA first implementation will preserve this ordering, scene durations,
palette families, relative positions, and motion phases. Because SGP has no
verified flood-fill primitive in the allowed interface, face interiors are
represented by painter-ordered LINE edges. This is a deliberate wireframe
conversion, not a claim that the VA SGP filled polygons like the PC-98 EGC.
Coordinates are scaled from the authored 640x400 logical space to 320x200 by
dividing both coordinates and extents by two. The permanent background is
black; a scene's carrier, grid, or panel is drawn only when that scene calls
for it.

## Adversarial review

| Proposal | Attack | Resolution |
|---|---|---|
| Copy the previous VA demo and rename it | It has a non-original checkerboard and a keyboard path not demonstrated for this program | Start from `origin/main`; recreate the plan and use DOS INT 21h AH=06h |
| Translate PEGC addresses to VA addresses | Matching numeric windows do not prove equivalent hardware | Delete PEGC assumptions; use only `INT 8Fh`, tested G1 DSA, and SGP documentation |
| Keep filled polygons by CPU rasterization | Violates the SGP-only rendering goal and changes the performance problem | Use SGP LINE outlines; document fill loss explicitly |
| Use a PC-98 IRQ2 handler for smooth timing | It changes VA PIC ownership and can hang or steal interrupts | Use TSP VBLANK status polling |
| Treat a black page as a missing background | The original background is scene-specific, not checkerboard | Draw carrier/grid/panel lines only in the corresponding scene routines |
| Assume INT 82h keyboard semantics | A vector/function mismatch can make ESC ineffective | Use the original DOS `INT 21h/AH=06h/DL=FFh` path and test AL=1Bh |
| Claim original music is ported by copying S98/MID data | VA Music BIOS queue format and OPNA availability differ | Keep music out of the graphics gate until AH=00/1Dh behavior is proven |
| Call a VAEG screenshot hardware validation | Emulator timing and ROM coverage are not real-board evidence | Report VAEG launch and human hardware gate separately |

## Open questions

- Exact VA Music BIOS queue placement and rhythm initialization require a
  separate runtime probe; graphics must not depend on it.
- SGP LINE direction names conflict between generic and LINE-specific VAEG
  constants; asymmetric edges in all four directions must be checked.
- 8bpp/640x400 fidelity requires a separate framebuffer-capacity audit.

## Implemented VA path

The implementation is `demos/neon4-va/neonva.asm` and builds as the DOS 8.3
file `NEONVA.COM`. It uses the verified 320x200 single-plane 4bpp mode,
Graphic 0 at SGP `200000h`, and two Graphic 1 pages at SGP `220000h` and
`227d00h`. The display page is selected with DSA1 `022eh/0230h` only after
TSP status-port `0142h` reaches VBLANK. SGP command pointers at `0500h/0502h`
are written as WORDs, and every command list starts with `SET_WORK` for the
58-byte work area in main RAM.

Each frame clears the hidden G1 page with `CLS`, emits the scene's ordered
wireframe records through `SET_COLOR` plus `LINE`, waits for SGP completion,
waits for VBLANK, and exchanges DSA1. The G0 black initialization is also an
SGP `CLS` command targeting `200000h`; no CPU pixel loop is used for animated
geometry. DOS `INT 21h/AH=06h/DL=FFh` polls the console and accepts only ASCII
ESC (`1bh`) as the exit key.

The first VAEG bring-up exposed two implementation hazards that are now fixed:

1. Scene routines initially used `PUSHA/POPA`. That restored `DI`, the SGP
   command-list write cursor, and silently discarded every scene record. Scene
   routines now preserve caller registers individually while retaining `DI`.
2. A large CPU `REP STOSW` clear of the VA G0 window delayed the guest DOS
   command path under the current VAEG memory timing. G0 initialization now
   uses the same mandatory-`SET_WORK` SGP `CLS` primitive as the draw pages.

The source was rerun in VAEG after both corrections. A fresh disposable disk
showed the scene-specific carrier rectangle and tetrahedral seed without a
checkerboard; SGP trace output showed `SET_WORK`, `CLS`, multiple `LINE`
records, and `END` for both hidden pages. These are VAEG observations, not a
real-hardware performance claim. Manual ESC and physical PC-88VA validation
remain the final human gate.
