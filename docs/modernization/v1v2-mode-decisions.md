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

# V1/V2 mode: implementation decisions register

This register lists every place where vaeg's V1/V2 mode follows a choice
that is not a direct statement of a VA manual or a measurement on a real
PC-88VA: derivations from ROM code, behaviour borrowed from the PC-8801
side, and plain implementation policy. It is the list to review at the
human gates and the list of things to check on a real machine. Hardware
model and milestone plan:
[`v1v2-mode-plan.md`](v1v2-mode-plan.md); CPU side:
[`upd9002-upd70008-mode.md`](upd9002-upd70008-mode.md).

## How to read it

Basis tags:

- `[VA-TM]` stated by the BNN technical manual or the electronic
  technical manual (`tekumani`).
- `[NEC-GIHO]` NEC Giho article on the PC-88VA (summarised, not quoted).
- `[UPD72022]` NEC µPD72022 data book (the generic part behind the VA
  TSP), as reconstructed in [`upd72022-tsp.md`](upd72022-tsp.md).
- `[ROM]` read from VA2 ROM code or traced while it runs.
- `[X88000]` PC-8801 behaviour as implemented by X88000 1.5.3 (public
  domain), used as a behavioural reference and re-implemented.
- `[DERIVED]` concluded from the above; not stated anywhere.
- `[POLICY]` an implementation choice with no hardware basis.
- `[MEAS]` measured on a real machine.

Status:

- **Approved** — accepted by the maintainer at the named gate.
- **Pending** — waiting for the named gate.
- **Open** — known gap or doubt; no decision yet.

"Real-machine check" says what would settle the point.

## 1. CPU and mode entry

| ID | Decision | Basis | Status | Where | Real-machine check |
|---|---|---|---|---|---|
| C1 | The alternate set (`AF'`…`HL'`) lives in machine state, survives CALLN/RETI, RETEM→BRKEM and adapter reset. | `[MEAS]` ALTPRB for the CALLN round trip; `[POLICY]` for the rest | Approved G103a | `cpu/upd9002/`, `cpu/upd9002_upd70008.cpp` | — |
| C2 | Hardware reset sets all alternate pairs to FFFFh. | `[MEAS]` third-party VA2 `mon` dump shows `A'F'`, `B'C'` all ones; `DE'`/`HL'` assumed the same | Approved G103c | same | cold power-on vs. warm reset, then V2 BASIC → `mon` → `x` |
| C3 | `BRKEM2` (`0F FE nn`) uses the same entry as `BRKEM`. | `[POLICY]`; ROM-less tests of both encodings | Approved G103b | `cpu/upd9002/` | — |
| C4 | No user-facing V1/V2 override; the ROM selects V1/V2 from the boot disk (SW7 option withdrawn). | `[ROM]` trace of the ROM's FDD selection | Approved G103b | — | — |
| C5 | IRET returning to compatible mode keeps F bits 1, 3, 5 as `(flag & 0FFFh) | F000h`. | `[MEAS]` probe results | Approved G103a | `cpu/upd9002/` | — |

## 2. Memory map and banking

| ID | Decision | Basis | Status | Where | Real-machine check |
|---|---|---|---|---|---|
| M1 | The 64 KiB 88-mode space is VA physical 10000h–1FFFFh, RAM-backed there. | `[NEC-GIHO]` memory figures | Approved G103b | `memoryva/memoryva.c` | — |
| M2 | The 1 KiB RAM window (ports 70h/78h) sits at 8000h–83FFh, backed at 18000h; its origin resets to 80h. | `[NEC-GIHO]` for the location; `[POLICY]` for the reset origin | Approved G103b | `io/memctrlva.c` | read 70h after power-on |
| M3 | RMODE (31h bit 2) maps only 6000h–7FFFh to the Debug 8800 bank (varom00 1E000h); 0000h–5FFFh stays N88-BASIC. | `[ROM]` MON keeps executing below 6000h after setting RMODE `[DERIVED]` | Approved G103c | `memoryva/memoryva.c` | dump 0000h–5FFFh with RMODE set |
| M4 | Extended RAM: E2h holds RE (bit 0) / WE (bit 4) and reads back inverted; E3h holds page (bits 3–2) and bank (bits 1–0). | Both manuals print the page/bank field under E2h and RE/WE under E3h; `[X88000]` and a PC-8801MA2 loader use the PC-8801 numbering, which the manuals' fields match with the port numbers exchanged `[DERIVED]` | Pending G103e | `io/memctrlva.c` | write E3h, read E2h and E3h |
| M5 | Page p, bank b of extended RAM is VA RAM `20000h + p·20000h + b·8000h`; page 3 exists only with the RAM board. | `[NEC-GIHO]`, BNN V1/V2 memory map | Pending G103e | `memoryva/memoryva.c` | write through ERAM, read the VA address in V3 |
| M6 | Dictionary ROM window: F0h selects a 16 KiB bank, F1h bit 0 clear maps it read-only at C000h–FFFFh. | Not in the VA manuals or X88000; a PC-8801MA2 loader checks for `44 10` at C000h, which is how the VA2 dictionary ROM image begins `[DERIVED]` | Pending G103e | `io/memctrlva.c`, `memoryva/memoryva.c` | read C000h after `OUT F0h,0 / OUT F1h,0` |
| M7 | When the dictionary window is mapped it takes precedence over GVRAM/TVRAM in C000h–FFFFh. | `[POLICY]` | Pending G103e | `memoryva/memoryva.c` | — |

## 3. I/O trap

| ID | Decision | Basis | Status | Where | Real-machine check |
|---|---|---|---|---|---|
| T1 | Compatible-mode trap matching uses the low port byte; native matching uses byte or word ports per FFEFh bit 4; FFE0h–FFFFh are never trapped. | `[VA-TM]` registers; `[POLICY]` for compatible low-byte matching | Approved G103b | `cpu/upd9002_upd70008.cpp`, `cpu/upd9002/` | — |
| T2 | A trapped compatible instruction transfers to vector 7Ch/7Dh with the instruction's own IP; the ROM handler advances IP. | `[ROM]` handler at F000:1944 | Approved G103b | same | — |
| T3 | DD/FD-prefixed compatible I/O and trap timing are not modelled. | — | Open | — | — |

## 4. Interrupts, timers and keyboard

| ID | Decision | Basis | Status | Where | Real-machine check |
|---|---|---|---|---|---|
| I1 | 8214 mode at reset; port 158H selects 8259 mode until reset. | `[VA-TM]` §5.2.2; `[ROM]` V3 writes 158H, V1/V2 does not | Approved G103c | `io/pic.c` | — |
| I2 | 8214 acceptance and re-arm: accept only above the current status (or any level with XSGS), no further acceptance until E4h is written. | Intel 8214 / PC-8801 behaviour; the VA manual documents only the registers | Approved G103c | `io/pic.c` | — |
| I3 | General timer 2 (600 Hz) runs only while E6h bit 0 enables it; the phase of its first tick is arbitrary. | `[POLICY]` | Approved G103c | `io/pic.c` | — |
| I4 | Key-matrix releases are delayed until the key has been seen across two VRTC starts. | `[POLICY]` (one would do for N-88 BASIC; two is a margin) | Approved G103c | `io/serial.c` | — |

## 5. Text display (TSP 3301 emulation)

| ID | Decision | Basis | Status | Where | Real-machine check |
|---|---|---|---|---|---|
| X1 | The VA2 ROM writes split-screen table fields with the generic memory commands: 8Eh `DPLD` loads DPTR0 (19-bit byte address, three bytes), 8Fh `DPLD` loads DPTR1 (signed increment; the ROM sends `01 00 00`, +1), 97h `WDAT` with MOD = 11b writes each parameter and then adds DPTR1, until the next command. vaeg writes one byte per parameter at TVRAM byte DPTR0 and always advances by one; the DPTR1 value and `MASK` (89h) are not modelled. | `[UPD72022]` §10.6 (the commands are not in the BNN VA command list); `[ROM]` traces. M103c had inferred the 8Eh/97h roles and left 8Fh unexplained. One byte per parameter in byte-access mode is `[DERIVED]`. | Approved G103c (basis now documented) | `io/tsp.c` (its comment still calls 8Fh unknown) | — |
| X2 | 3301 attribute pairs follow X88000's transparent-mode rule (memory order, shifted when the first column is non-zero, colour/secret/blink/reverse carry across rows, lines reset per row, start white). Colour attribute mode is assumed. | `[X88000]` | Approved G103c | `vram/maketextva.c` | — |
| X3 | Byte-mode address: local L = start field / 2; TVRAM byte = `((L & F000h) << 1) | (L & 0FFFh)`. | `[VA-TM]` §8.2.1 tabulates 3000h–3FFFh and B000h–BFFFh; the rule fits both `[DERIVED]` (M103c used L + 3000h, which fits only the first) | Approved by the maintainer (2026-10-04, G103f review) | `vram/maketextva.c` | — |
| X4 | Rows of the emulated screen that start outside the usable byte-mode ranges are not displayed. After a 3301 RESET the ROM moves the screen to local 0800h, so the text stays blank while the 3301 display is stopped (GAME-A no longer shows a dashed line). | `[VA-TM]` §8.2.1 calls other ranges unusable; blanking them is `[POLICY]` (maintainer choice "a", 2026-10-04) | Pending G103f | `vram/maketextva.c` | photograph GAME-A after its opening |
| X5 | Not modelled: semigraphics, 40-column doubling, monochrome attribute mode, CURS-positioned cursor. | — | Open | — | — |

## 6. Graphics

| ID | Decision | Basis | Status | Where | Real-machine check |
|---|---|---|---|---|---|
| G1 | Ports 34h and 35h read back the last value written. | The manuals list them as output only; `[ROM]` the text trap handler saves them with IN and restores them with OUT, which only works if they read back `[DERIVED]` | Pending G103d | `io/memctrlva.c` | `OUT 35h,95h : IN 35h` from V2 BASIC (`INP(&H35)`) |
| G2 | Port 31h bits PM00, GDEN0 and VW1 drive GrRes (102h) bit 0 and GrMode (100h) bits 15 and 1 in 88 mode. | `[VA-TM]` the bit names coincide; `[ROM]` sets 100h with GDEN0 clear and BASIC only writes 31h `[DERIVED]` | Pending G103d | `io/memctrlva.c` | — |
| G3 | 10Ch PLTM2 = 0 selects the palette mode from 32h PMODE and 31h PM00. | `[VA-TM]` | Pending G103d | `io/videova.c` | — |
| G4 | Multiplane 1 bit/pixel: the OR of the planes switched on in 110h is drawn in the text cell's colour (after reverse), in place of a separate graphics screen. | `[VA-TM]` ch. 4 §2.1; colour/reverse handling as `[X88000]` | Pending G103d | `vram/makegrphva.c`, `vram/scrndrawva.c` | — |
| G5 | With 110h 88MD set and 400 lines, plane 0 shows the upper and plane 1 the lower 200 lines, over twice the frame-buffer height. | `[ROM]` SCREEN 2 leaves the frame buffer at 200 lines and switches on planes 0 and 1; PC-8801 640×400 format `[DERIVED]` | Pending G103d | `vram/makegrphva.c` | — |
| G6 | In colour mode `COLOR f,b` leaves the background black: the ROM writes the backdrop, but graphics colour 0 is opaque. | `[ROM]` register values; matches `[X88000]` colour mode | Pending G103d | — | photograph `COLOR 7,4` in SCREEN 0 |
| G7 | Not modelled: 320-dot mode in 1 bit/pixel, GVRAM wait states. | — | Open | — | — |
| G8 | 110h bit 7 (G3MSK) clear keeps plane 3 out of 4 bit/pixel display. Bug fix, in the ledger. | `[VA-TM]` ch. 4 and §8.3.2 | Pending G103f | `vram/makegrphva.c` | — |

## 7. Character ROM

| ID | Decision | Basis | Status | Where | Real-machine check |
|---|---|---|---|---|---|
| K1 | Kanji ports E8h/E9h (level 1) and ECh/EDh (level 2): OUT writes a word address, IN even port = right half, odd port = left half. | `[VA-TM]` | Pending G103e | `io/cgromva.c` | — |
| K2 | The word address indexes the VA font image directly, with byte-offset bit 14 inverted from offset 8000h (kanji area). | `[ROM]` the extension-ROM BASIC computes `((jis1 & 1Fh) | (jis2 & 60h)) << 9 | (jis2 & 1Fh) << 4`; the VA image's JIS decoder uses the opposite sense of bit 14 `[DERIVED]` | Pending G103e | `io/cgromva.c` | kanji text in SR-DEMO and N88-日本語BASIC |
| K3 | Level-2 kanji in the 7xxxh JIS rows (stored differently in the VA image) are not converted. | — | Open | — | — |

## 8. Media

| ID | Decision | Basis | Status | Where | Real-machine check |
|---|---|---|---|---|---|
| D1 | D88 table entries at or beyond the first track (lowest entry 0–159 at or above 2A0h) are ignored. Bug fix, in the ledger. | Image format | Pending G103e | `fdd/fdd_d88.c` | — |
| D2 | `--fdd1-image N` / `--fdd2-image N` choose a disk inside a multi-image D88. The choice is not stored in the configuration or in state files; no GUI control. | `[POLICY]` | Pending G103f | `fdd/`, `sdl2/` | — |
| D3 | Formatting is refused unless the D88 file holds a single image. | `[POLICY]` (growing a track would move later images) | Approved by the maintainer (2026-10-04, G103f review) | `fdd/fdd_d88.c` | — |

## 9. Not yet covered

- V1 mode: no V1 media available; nothing V1-specific has been run.
- Sound under V1/V2: SR-DEMO drives the OPN through 44h/45h, but audible
  output has only the G103e listening check.
- DD/FD-prefixed trapped I/O and trap timing (T3); text and graphics
  features listed in X5 and G7; level-2 kanji 7xxxh rows (K3).

## 10. Real-machine checks, in order of value

1. `INP(&H34)`, `INP(&H35)` after `OUT` in V2 BASIC (G1).
2. GAME-A screen after its opening: are there stray text rows (X4)?
   (vaeg now shows none.)
3. `INP(&HE2)`, `INP(&HE3)` after writing E3h (M4).
4. `mon` → `x` after cold power-on and after warm reset (C2).
5. `COLOR 7,4` in SCREEN 0: black or green background (G6).
