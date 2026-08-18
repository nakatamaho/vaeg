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

# NEON4 PC-9801 to PC-88VA port audit

## Scope and evidence

`demos/NEON4_1_0/` is an existing PC-9801-oriented NASM demo. It is kept as
the regression/reference source; the port uses a separate VA-specific source
so that the original GRCG, EGC, and PEGC implementations remain available for
comparison. The source directory and the two document collections used below
are maintainer-provided local material and are not changed by this task.

Primary local evidence:

| Evidence | Use |
|---|---|
| `docs/98io/io_disp.txt` | PC-9801 text/graphic GDC, plane selection, GRCG mode, 256-colour-related model notes, VSYNC trigger |
| `docs/98io/io_egc.txt` | PC-9801 EGC register map and mandatory word accesses |
| `docs/98io/io_music.txt`, `docs/98io/io_sound.txt` | PC-9801-14 and PC-98 FM/OPNA port families |
| `docs/98io/io_mem.txt` | PC-9801 bank windows and EMS/VRAM mappings |
| `docs/tekumani/4.TXT` | PC-88VA SGP address space, command words, descriptors, LINE, CLS, page operation |
| `docs/tekumani/606GRP.TXT` | PC-88VA graphics BIOS mode, buffers, windows, composition, palette, scroll |
| `docs/tekumani/607ADVG.TXT` | PC-88VA extended graphics BIOS SETFRAME, VIEW, CLS, LINE, POLYLINE, PAGE |
| `docs/tekumani/611MUSIC.TXT` | PC-88VA Music BIOS, INT 8Bh, OPNA initialization and play modes |
| `docs/modernization/upd92017-sgp.md` | Tracked reconstruction of the VA SGP interface and VAEG implementation boundary |
| `demos/NEON4_1_0/*.INC` | Exact PC-98 assumptions in the original program; source evidence, not VA hardware evidence |

The Japanese text was decoded as CP932 for inspection only. The source
documents above remain byte-for-byte untouched.

## Original NEON4 inventory

| Original component | Location | PC-98 assumption | Port disposition |
|---|---|---|---|
| 16-colour renderer | `VIDEO4_LOW.INC`, `GEOM4_LOW.INC` | GRCG at ports `007Ch/007Eh`, graphics mode through `INT 18h`, planar VRAM | Replace with VA SGP command lists and VA graphics setup |
| EGC fast path | `VIDEO4_LOW.INC`, `AFS4_286.INC` | EGC word registers `04A0h-04AEh`, EGC mode selected through `006Ah` | Do not port; retain only as historical reference |
| 256-colour renderer | `VIDEO4_256.INC`, `VIDEO256_PACKED*.INC`, `BLIT4_256.INC` | PC-9821 PEGC MMIO at `E0000h`, bank selectors, PEGC packed/plane modes | Replace with one VA 4-bpp SGP path first; a VA 8-bpp follow-up is separate |
| Text overlay | `TEXT4_LOW.INC`, `TEXT4_256.INC` | PC-98 text VRAM segments and 80x25 layout | Use VA text BIOS or a minimal DOS status line; never write PC-98 text segments |
| VSYNC scheduler | `AFS4_286.INC`, `DATA4.INC` | PC-98 IRQ2 / `INT 0Ah`, PIC masks, port `0064h` trigger | Use VA TSP VBLANK status and the tested VA page exchange; no `INT 0Ah` hook |
| Sound detection | `OPNA.INC`, `OPL3.INC` | Candidate port probing (`0088h`, `0188h`, `0288h`, `0388h`) and PC-98 OPL3-SA ports | Use VA Music BIOS `INT 8Bh`; direct probing and OPL3 are out of the first port |
| Geometry/scene data | `GEOM4_CORE.INC`, `GEOM4_256.INC`, `SCENE4_256.INC` | Arithmetic and object data are CPU-side and mostly portable | Reuse data and projection math after removing renderer-specific fields |
| Music data | `MUSIC_*.INC`, `.S98`, `.mid` | Arrangement is authored for OPN/OPNA/OPL3 variants | Reuse musical intent; adapt playback to VA Music BIOS and document unsupported voices |
| DOS shell/guards | `BREAKGUARD.INC`, `FAULTGUARD.INC`, `COMMAND4_*.INC` | PC-98 STOP/INT 06h and IRQ ownership | Keep only DOS-safe ESC handling in the VA program |

## PC-9801 versus PC-88VA differences

| Concern | PC-9801 evidence | PC-88VA evidence | Consequence for NEON4 |
|---|---|---|---|
| Graphics ownership | GDC drives graphics VRAM; GRCG/EGC are selected through PC-98 ports (`io_disp.txt`, `io_egc.txt`) | Graphics BIOS `INT 8Fh` owns G0/G1 mode, buffers, windows, composition (`606GRP.TXT`) | Remove PC-98 mode/plane programming; initialize a documented VA mode |
| Drawing engine | GRCG/EGC perform CPU/VRAM logical operations; EGC registers are word-only (`io_egc.txt`) | SGP is a separate command processor and works only in single-plane mode (`4.TXT`, `upd92017-sgp.md`) | Build a main-RAM word command list; no CPU pixel loops and no GRCG/EGC ports |
| 16-colour format | PC-98 planar VRAM, with plane select at model-dependent ports | VA single-plane 4 bpp; SGP pixel mode `1`, GVRAM at SGP `200000h-23FFFFh` | Use 4-bpp packed descriptors and VA pitch, not four PC-98 planes |
| 256-colour format | NEON source uses PEGC `E0000h` MMIO and 256 KiB pages; this is not a VA interface | VA graphics BIOS supports 8-bit buffers in single-plane mode, but SGP/VAEG original-VA limits must be verified separately | Do not translate PEGC registers; first deliver a verified 4-bpp VA port |
| Framebuffer address | PC-98 source assumes A0000h/B0000h windows and bank switching | VA SGP address map has main RAM `000000h-09FFFFh`, GVRAM `200000h-23FFFFh` (`4.TXT`) | Use SGP physical addresses and existing VA page constants only |
| Page exchange | PC-98 display/draw plane selection and custom IRQ scheduler | VA graphics BIOS defines buffers/windows; tested VA demos use G1 DSA1 word registers at `022Eh/0230h` with TSP VBLANK status | Keep page ownership in VA terms and flip only at VBLANK |
| Composition | PC-98 text/GDC/graphics priority is machine/model-specific | VA `Compose` encodes text, sprite, G0, G1; sprite is always above text (`606GRP.TXT`) | Explicitly request G1 over G0; do not assume PC-98 layer order |
| Scroll | PC-98 source has no VA buffer/window semantics | VA `Roll`/`RollTo` and extended `VIEW` operate on VA framebuffer windows; wrap rules are documented | Treat scrolling as a later VA window feature, not a copied PC-98 port |
| Line drawing | PC-98 GDC/EGC paths are used by the source | VA SGP `LINE` command `0009h` draws the destination block diagonal (`4.TXT`, `upd92017-sgp.md`) | Map NEON wire/edge primitives to SGP LINE records |
| Fill/clear | PC-98 GRCG/EGC fills and PEGC colour expansion | VA SGP `CLS` `000Ah`, `PATBLT` `0008h`, and extended graphics `CLS` are documented | Clear hidden G1 with SGP; do not copy EGC ROP setup |
| Command storage | PC-98 source directly writes device registers/VRAM | VA SGP command/parameters are little-endian words in main RAM, with a 58-byte work area (`4.TXT`) | Keep command lists in the COM's main RAM and initialize `SET_WORK` before drawing |
| SGP register width | Not applicable to the PC-98 GRCG/PEGC paths | VA SGP command pointer ports `0500h/0502h` are word ports; start/status is at `0506h` (`upd92017-sgp.md`) | Emit word I/O only for `0500h` and `0502h`; never split to byte writes |
| Vertical sync | PC-98 VSYNC trigger writes port `0064h` and services IRQ2/`INT 0Ah` (`io_disp.txt`) | VA TSP status exposes VBLANK; SGP completion is independent of display timing | Poll the tested VA status path; do not install a PC-98 PIC/IRQ hook |
| Sound chip/API | PC-98 FM ports at `0188h-018Eh`, model-dependent remapping; PC-98-14 has unrelated TMS3631 ports (`io_sound.txt`, `io_music.txt`) | VA Music BIOS vector is `INT 8Bh`; initialization is mandatory and OPNA-only features require OPNA (`611MUSIC.TXT`) | Use BIOS playback; no PC-98 port probing or OPL3 dependency in the first port |
| CPU assumptions | NEON uses `cpu 386`, PC-98 STOP/invalid-opcode guard, 286 variant | VA guest is 16-bit real mode with uPD9002 semantics; existing guest demos avoid unsupported branch encodings | Keep 16-bit NASM and short/near control flow that VAEG executes; remove PC-98 exception tricks |
| Text | PC-98 source writes `TEXT_CHAR_SEG`/`TEXT_ATTR_SEG` directly | VA text has its own BIOS/framebuffer and 40/80-column modes (`604TEXT.TXT`) | Status text is optional and must use VA text services or be omitted in first graphics gate |

## Conversion investigation

The viable first architecture is a VA-specific 320x200, 16-colour scene:

1. Initialize G0/G1 through the tested VA graphics BIOS path (`INT 8Fh`), set
   palette and composition, and draw a patterned G0 once through the VA CPU
   aperture.
2. Keep two 320x200 4-bpp G1 pages. Build an SGP list in main RAM containing
   `SET_WORK`, `SET_COLOR`, `CLS`, ordered `SET_DESTINATION`/`LINE` records, and
   `END`. The scene is a reduced NEON geometric-solid chapter, not a PEGC
   emulation layer.
3. Use the documented VA SGP word ports (`0500h`, `0502h`, `0506h`) and restore
   the tested VA GVRAM write mode before each kick. Poll SGP busy to completion,
   then wait for the VA VBLANK transition and exchange the hidden G1 page.
4. Update object phase and projection on the CPU while the previous frame is
   displayed. No CPU loop writes sprite/line pixels; all line pixels are SGP
   work.
5. Add ESC through the VA keyboard BIOS. Sound is a separate adapter using
   `INT 8Bh`; it must initialize its queue/work area and can be disabled if the
   running ROM does not expose Music BIOS support.

The first implementation intentionally does not claim that every original
NEON4 scene, PEGC packed mode, OPL3 arrangement, or PC-98 text overlay is
ported. Those are separate follow-up work once the VA graphics path is
launchable and visually correct.

## Adversarial review

| Proposed claim or action | Adversarial objection | Evidence/response | Required mitigation |
|---|---|---|---|
| “PEGC can be translated to VA by changing the MMIO segment” | VA has no documented PEGC bank/register contract; a renamed address would be invented hardware | `VIDEO4_256.INC` is source evidence only; VA authority is `4.TXT`/`606GRP.TXT` | Delete PEGC backend from the port; use SGP descriptors |
| “The existing PC-98 VSYNC IRQ can be reused” | `INT 0Ah` and PIC ownership are PC-98-specific and can steal VA interrupts | `io_disp.txt` documents PC-98 IRQ2; VA docs expose TSP/VBLANK separately | Use VA TSP status and page registers already exercised by SGP demo |
| “A line command is enough without SET_WORK” | VA SGP work memory is required before drawing; uninitialized state may appear to work in the emulator | `4.TXT` explicitly requires `SET_WORK`; prior SGP bug ledger records this hazard | Emit `SET_WORK` in every initial/diagnostic list and keep the 58-byte area stable |
| “A byte write is fine for every SGP port” | VA command-address ports are word ports and real hardware can hang on byte access | `upd92017-sgp.md` and existing bug ledger classify `0500h/0502h` as word-only | Use `OUT DX,AX` for command address; use byte access only where the interface explicitly says byte |
| “The original 640x400 256-colour scene is the natural first target” | VA 8-bpp buffer layout, page capacity, and SGP/VAEG profile need independent proof; the source is PEGC-specific | `606GRP.TXT` allows 8-bit buffers, but `4.TXT`/VAEG SGP scope must still be honoured | Start with 320x200 4-bpp; track 8-bpp as a later design |
| “Use VA graphics BIOS PAGE as a black box” | The page BIOS may clear or own buffers differently than the direct tested G1 DSA path | `607ADVG.TXT` documents PAGE, while current SGP demo proves DSA1 path | First port uses the known VA SGP page exchange; compare PAGE only after a separate test |
| “Use direct OPNA writes from the PC-98 module” | Same YM name does not imply same ports or BIOS ownership; direct writes bypass VA Music BIOS work | `611MUSIC.TXT` requires initialization and names `INT 8Bh` | Implement a BIOS adapter with a no-sound fallback; do not probe `0188h` blindly |
| “Preserve all original fault guards” | STOP/invalid-opcode and DOS/PIC restoration code is coupled to PC-98 behavior | `BREAKGUARD.INC` and `FAULTGUARD.INC` explicitly name PC-98 vectors | Keep only simple DOS-safe ESC handling in the VA port |
| “A successful VAEG run proves real VA hardware” | VAEG SGP timing and BIOS coverage are not a physical-board test | `upd92017-sgp.md` marks unresolved semantics and model-dependent timing | Report VAEG launch separately from hardware validation; do not claim hardware performance |
| “Commit the original untracked source directory as part of the port” | It would mix a large reference import with the new implementation and may include binaries | Current tree shows `demos/NEON4_1_0/` untracked with COM/MID/S98 payloads | Preserve the reference directory; add only a small lower-case VA port directory and docs |

## Open questions and gates

- Exact VA Music BIOS availability in each ROM set must be checked at runtime;
  graphics must remain usable with sound disabled.
- SGP LINE direction bits have a documented conflict in the tracked
  reconstruction. The port must use one asymmetric-line self-test before
  relying on all octants.
- The original NEON4 640x400/256-colour artistic renderer needs a separate VA
  framebuffer and palette design; it is not silently represented by the first
  4-bpp port.
- A human gate is required after a clean VAEG build, launch, visual check,
  ESC exit, and (if available) VA hardware check. No screenshot or emulator
  run alone is a hardware PASS.

