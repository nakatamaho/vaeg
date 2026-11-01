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

# WSR-E pseudo-sprite track: plan and specification

Melco WSR-E (Cirrus Logic CL-GD5428, 1 MiB DRAM, C-bus) on the PC-88VA
family, driven from MS-DOS 4.0, and the same board in vaeg.

This file is a design document, not a task. Work starts only from a task
file the maintainer assigns (`docs/agents/tasks/M*.md`, first one: W1),
under `AGENTS.md`. When this file and a task file disagree, the task file
wins; record the disagreement in the task file.

Evidence tags used below: `[DOC]` a manual or data book, `[MEASURED]` a
photograph or log from a PC-88VA/VA2, `[OTHER-BOARD]` shipped software
for another board with the same port map, `[EMULATOR]` NP2kai's reverse-
engineered WAB model, `[INFERRED]` reasoning only, `[OPEN]` not known.
Nothing about the WSR-E on a PC-88VA is `[MEASURED]` yet.

## 1. Goal and deliverables

Extend `demos/sgp-pseudo-sprite/65536` (`SGP655S.COM`) to a 65,536-colour
pseudo-sprite demo that draws with the WSR-E's BitBLT engine, and make
vaeg run it the way the machines do.

- Final program: `WSR655S.COM`. Two 320x200 RGB565 pages on the WSR-E,
  24x24 HSV spheres drawn with transparent screen-to-screen BLTs, a HUD
  with the machine name, FPS and ball count.
- Test programs: one or more per stage W1-W5, kept and rebuilt after
  later stages (they accumulate; nothing is deleted).
- Distribution: every COM on an MS-DOS 4.0 disk built from FreeDOS-88VA
  `msdos4-va.2`, `wsr-e-pseudo-sprite-msdos4.d88`.
- vaeg: at each stage, the WSR-E functions that stage uses are added to
  vaeg, and the same COM is compared between vaeg and the machines.
  Pass/fail is decided on the machines; vaeg is tuned to their numbers.

## 2. Target environment

| Item | Value |
| --- | --- |
| Machines | PC-88VA, VA2, VA3, VA with PC-88VA-91, V3 mode |
| Reference machines for gates | PC-88VA and PC-88VA2 (both photographed) |
| CPU | uPD9002, 80186 instructions only: assemble with `cpu 186`; never emit the `0Fh 8xh` near conditional jump |
| Board | Melco WSR-E (CL-GD5428, 1 MiB). PCM part unused |
| Monitor | one that shows the WSR-E output (31 kHz class); port 40E1h's relay selects WSR-E or VA video |
| OS | MS-DOS 4.0, FreeDOS-88VA `msdos4-va.2` disk |
| Memory | the demo takes a 64 KiB work segment after the COM; with less memory it says so and exits |
| Verification | photographs and logs from the PC-88VA and VA2 are the reference; vaeg runs the same COM |

## 3. Relation to SGP655S

Ball images, motion and controls are taken over unchanged; only the
drawing engine and display path change. The HUD is new.

| Item | SGP655S.COM (existing) | WSR655S.COM (new) |
| --- | --- | --- |
| Engine | VA SGP | GD5428 BitBLT engine |
| Screen | G0 320x200x16 bpp, two pages, FB0 DSA flip | 320x200 RGB565, two pages, CRTC start-address flip |
| Output | VA video | WSR-E output, selected by the 40E1h relay |
| Ball images | 24x24, 16 HSV hues (`orb_hsv16_24.inc`) in main RAM | same file, copied to WSR-E VRAM at start through the E window |
| Transparency | BITBLT mode 0105h | GD5428 16-bpp transparent BLT; two-pass mask if it does not work (decided in W3) |
| Clear | SGP CLS | fill BLT of the hidden page |
| Ball count | 1-1024, UP/DOWN or `+`/`-` | same |
| Motion | deterministic records, 16 directions (`sprite_records_128_random16.inc`, generated beyond 128) | same table and generator |
| Grid | SPACE: white 16-px grid with SGP LINE | SPACE: 16-px grid with 1-px fill BLTs |
| HUD | none | machine, FPS, balls at the upper right |
| Issue | one command list started at once | per ball: write BLT registers, wait for completion (the 5428 has no queued start) |
| Vertical sync | TSP status 0142h bit 6 | WSR-E status 5AE0h (VGA 3DAh) |
| FPS time base | calendar BIOS INT 8Ch function 02h | HUD: same; measuring mode `/B`: VRTC counts |
| Exit | ESC, counters printed | same; also restores the relay, port 152h and the board |

## 4. Directory and build

```text
demos/wsr-e-pseudo-sprite/
  README.md
  build.sh            [--msdos4 IMAGE] [OUTDIR]
  wsre.inc            ports, model id, VRTC timing, CSV log, port probe
  smoke.py            developer smoke run under Unicorn (no hardware)
  gen_data.py         HUD glyph tiles and test patterns (from W5)
  tests/
    w1port.asm        -> W1PORT.COM
    w1probe.asm       -> W1PROBE.COM
    w2win.asm         -> W2WIN.COM
    w3blt.asm         -> W3BLT.COM
    w4bw.asm          -> W4BW.COM
    w5mode.asm        -> W5MODE.COM
  wsr655.asm          -> WSR655S.COM (W6), WSR655A.COM ... (W7), WSR655T.COM (W8)
```

```sh
NASM=nasm demos/wsr-e-pseudo-sprite/build.sh /tmp/wsre
NASM=nasm demos/wsr-e-pseudo-sprite/build.sh --msdos4 msdos4-pc88va-2hd.d88 /tmp/wsre
```

- `build.sh` builds every program that exists at that point.
- Ball images and the motion table are not copied: assemble with
  `-I ../sgp-pseudo-sprite/65536/` and include `orb_hsv16_24.inc` and
  `sprite_records_128_random16.inc` directly.
- `--msdos4` uses `tools/pc88va/pcengine_disk.py` `vanilla` and `install`
  (as `demos/tsp-sprites/build.sh`) to make
  `OUTDIR/wsr-e-pseudo-sprite-msdos4.d88`; COMs go to the root.
- `build.sh` embeds the 8-hex-digit `git rev-parse --short=8 HEAD` as the
  build id (`nogit000` outside a checkout).
- 8.3 names: tests `W<stage><content>`, demo `WSR655` plus one letter.
- Generated COMs and D88s stay out of Git, except a distributable
  `.d88.xz` under `demos/disks/` if the maintainer asks for one.
- Every file carries the BSD 2-clause header of the existing demos.
  Tracked paths are lowercase (`CONVENTIONS.md`).

## 5. Common rules for every program

- Machine: the ROM word at F000:FFFE (FFFFh `VA`, FFFEh `VA2/3`, FFFDh
  `VA+91`, other values `?` and the hex word), first on screen and in the
  log (`AGENTS.md`, "Measurements on the machines").
- Board: `yes` or `none` from the SR06 probe, and the CR27 chip id, as
  read. W1PORT does not touch the board and reports `-` for both. The
  board id ports (51E1h, 59E1h, 5BE1h) are logged as check rows.
- Build id and display rate: the build id above, and the VA display rate
  (15 or 24 kHz, port 1CBh bit 3 set at 24.8 kHz). The VRTC period
  depends on the rate, so every timing row is recorded with it.
- Output: W1-W4 do not use the WSR-E display; they print to the VA
  screen (DOS console). From W5 on, results are drawn on the WSR-E
  screen and a summary is printed to DOS on exit.
- Log: every result line is also appended to `WSRLOG.TXT` in the current
  directory (section 7).
- Exit codes: 0 pass, 1 a result differs from the expected one, 2 cannot
  measure (no board, not enough memory), 3 stopped with ESC.
- Interrupts: VRTCs are counted on IRQ2 (vector 0Ah, PIC 188h/18Ah), as
  SGPCONT does. During a timing row every IRQ except IRQ2 is masked and
  the mask is restored after the row. While port 152h releases the E
  area to the slot, interrupts are off, for at most 1 KiB of transfer at
  a time (so no VRTC is lost).
- Cleanup: on every exit path restore port 152h, 40E1h (output to the
  VA), 46E8h, SR06 (extensions locked), the interrupt vector and mask,
  and the VA screen mode.
- HUD (W5 on): upper right of the WSR-E screen, `MACHINE FPSnn.n Bnnnn`,
  glyphs from VRAM-resident tiles placed with transparent BLTs; FPS
  updated once a second from the calendar BIOS.
- Data: synthetic only (`orb_hsv16_24.inc`, glyphs and test patterns from
  `gen_data.py`); no ROM or game data.

## 6. Milestones

Eight stages. Each closes with photographs and logs from the PC-88VA and
VA2. vaeg adds the stage's functions and matches the same COM's numbers.
Machine measurements follow the previous stage's pass; vaeg work may run
ahead. W3 (does the 16-bpp transparent BLT work) fixes W6's drawing
method; W4 (effective bandwidth) fixes W8's background configuration.

| Stage | Program | What it establishes | Pass condition | vaeg side |
| --- | --- | --- | --- | --- |
| W1 | W1PORT.COM | without the board: whether the VA drives low bytes E0h/E1h/E8h (reads compared with different trailing code), port 152h read-back, R0 calibration | on VA and VA2 the three ports are not driven; 152h reads back what was written | nothing new (an unconnected port reads FFh, which counts as not driven) |
| W1 | W1PROBE.COM | with the board: wake-up, SR06 12h/0Fh, CR27, word OUT written and byte IN read back, board id ports | SR06 toggles by byte and by word; word-written GR09/GR0A read back | board skeleton, port map, SR06 lock, CR27, word writes, id values |
| W2 | W2WIN.COM | E window through 152h; pattern over the whole 1 MiB, read back; window transfer times; finally 10 s with interrupts allowed while the window is open | no error on VA and VA2; times recorded | 152h banks 8-15 to the WSR-E window, bank registers, window waits |
| W3 | W3BLT.COM | 16-bpp screen-to-screen BLT (copy, ROPs, transparency, fill, colour expansion), results read through the window, CRC compared | which BLTs work on the 5428 is known; CRCs recorded | BLT engine (only what worked on the machines), GR31 |
| W4 | W4BW.COM | BLT and I/O time table (section 7), display off and on | every row recorded on both machines | BLT duration, board I/O waits, display contention; vaeg/machine 0.9-1.1 |
| W5 | W5MODE.COM | 320x200 RGB565 with double scan, relay, start-address flip, WSR-E vertical sync period, HUD | correct picture on the WSR-E monitor; period recorded | GD5428 display, relay, vertical sync |
| W6 | WSR655S.COM | the demo (clear + balls, double buffer, HUD); `/B` frames per ball count | `/B` table recorded on both machines; ESC returns to DOS | nothing new; `/B` table vaeg/machine 0.9-1.1 |
| W7 | WSR655A.COM ... | optimised variants (word OUT, skipped unchanged high bytes, game logic under background BLTs) | `/B` above W6 | nothing new |
| W8 | WSR655T.COM | backgrounds (one and two scrolling layers) | 30 fps and 60 fps configurations chosen from the `/B` table | nothing new |

Each stage's vaeg work is a vaeg milestone (an M number, a task file, a
human gate), numbered when it starts. Gate photographs: PC-88VA and
PC-88VA2, each showing machine, build id and display rate.

## 7. Measurement and logging

All times are counted in VA VRTCs and recorded as raw numbers (count and
VRTCs). Conversion to clocks is done afterwards on the host.

- Method: one row per item. Count how many times the item completes in
  600 VRTCs (about 10 s), as SGPCONT does (more is faster). If a count is
  below 1000, make the unit smaller so the resolution is 0.1 % or better.
  The count includes the call and loop overhead of the measuring loop.
- Start: counting starts at a VRTC edge; one VRTC between rows.
- Calibration row R0, first in every COM: `LOOP` 1024 times in main RAM.
  M130 measured 18.1 clocks a pass on both machines, which checks both
  the CPU speed and the VRTC period.
- Conversion: on the VA2, 1 VRTC = 133,460 clocks (M130: 30 VRTCs =
  4,003,800 clocks; the display rate of that run was not recorded). For
  other machines and rates, derive it from R0 and record it with the
  results.
- Repetition: each timing row runs 3 times; all three are recorded. If
  max-min exceeds 1 %, the host script marks the row.
- Photographs and log: the screen shows every row, readable in one
  photograph. When the photograph and the log disagree, the log wins.
  vaeg runs the same COM and its log is compared field by field.

Log format (`WSRLOG.TXT`, one item per line, comma separated, a header
line when the file is created; lines starting with `#` are comments):

```csv
stage,build,machine,khz,board,cr27,row,param,run,count,vrtc,result
# W1PORT VA2/3 FFFE 1a2b3c4d 15kHz
W1,1a2b3c4d,VA2/3,15,-,--,R0,LOOP1024,1,4321,600,
W1,1a2b3c4d,VA2/3,15,-,--,P,00E0,,,,FLOAT A=55B3 B=AAB1 C=33B4
W1,1a2b3c4d,VA2/3,15,-,--,B152,SLOT,,,,R=08 E=08 PASS
```

- Timing rows fill `run`, `count` and `vrtc`; `result` is empty.
- Check rows leave `run`, `count`, `vrtc` empty and put a token without
  commas in `result`: `R=xx E=xx PASS|FAIL` (read, expected), a value,
  a probe class, or for `SUM` `PASS` / `FAIL n` / `NO-BOARD`.
- Port probe: three code patterns (A, B, C) differ in the byte before
  the `IN` and the bytes after it, and each runs with the `IN` at an even
  and at an odd address (six reads, printed as even/odd pairs). Classes:
  `DRIVEN xx` (six equal reads, not FFh), `FF` (six FFh: pulled up, or
  vaeg's unconnected port) and `FLOAT A=.. B=.. C=..` (reads differ, so
  the bus kept code bytes). `FF` and `FLOAT` both mean "not driven".
- `demos/wsr-e-pseudo-sprite/smoke.py` runs a COM under Unicorn with a
  minimal DOS and fake ports (FF, two floating-bus models, a fake board)
  to check control flow, CSV output and cleanup before hardware runs.

W4BW.COM rows (W2 and W5 reuse some of them):

| Row | Item | Unit |
| --- | --- | --- |
| R0 | `LOOP` in main RAM | 1024 passes |
| I1 | `OUT DX,AL` to 4EE0h | 256 |
| I2 | `OUT DX,AX` to 4EE0h | 256 |
| I3 | `IN AL,DX` from 4FE0h | 256 |
| M0 | port 152h switch and back, CLI/STI only | 1 |
| M1 | `REP STOSW` into the E window | 1 KiB |
| M2 | `REP MOVSW` main RAM to the E window | 1 KiB |
| M3 | `REP MOVSW` E window to main RAM | 1 KiB |
| S0 | the 7 BLT register writes only (no start) | 1 |
| S1 | 24x24 16-bpp copy, including the completion wait | 1 |
| S2 | 24x24 16-bpp transparent (the method that worked in W3) | 1 |
| F1 | 320x200 16-bpp copy | 1 |
| F2 | 320x200 16-bpp transparent | 1 |
| F3 | 320x200 fill (the method that worked in W3) | 1 |
| D1-D4 | 24 lines high, 8/24/64/320 pixels wide, copy (width sweep) | 1 |

- Display: S1, S2 and F1-F3 run with the display off (SR01 bit 5) and on
  (W2's 640x480x8 mode); W5 repeats them in the 320x200x16 mode.
- Demo measurement (`WSR655S /B`): balls 16, 64, 128, 256, 512, 1024,
  each for 600 VRTCs; record completed frames and missed vertical syncs.

## 8. vaeg in parallel

vaeg gets the WSR-E as a new C-bus board, one stage at a time. The target,
as in M130 and M141, is vaeg/machine within 0.9-1.1 for the same COM.

| Stage | Added to vaeg | Place (proposal) | Compared |
| --- | --- | --- | --- |
| W1 (W1PORT) | nothing | - | log identical (`FF`/`FLOAT` both "not driven", board `none`) |
| W1 (W1PROBE) | board present/absent setting; port map 4nE0h, 46E8h, 40E1h/42E1h and id ports; SR/GR/CRTC/attribute/DAC register files; SR06 lock; CR27 | `cbus/wsre.c` (board), `vram/gd5428.c` (chip) | W1PROBE log identical |
| W2 | when port 152h's low nibble is 1xxx, E0000-E7FFFh goes to the WSR-E window (before `varom08.rom` when the board is present); GR0B/GR09/GR0A banking; window wait states | `io/memctrlva.c`, `memoryva/` | no errors; M rows 0.9-1.1 |
| W3 | BLT: 16-bpp copy, ROPs, transparency, fill, colour expansion, GR31. A method that failed on the machines also fails in vaeg | `vram/gd5428.c` | CRCs identical |
| W4 | BLT duration (start latency + bytes / rate + display fetch share) through nevent; GR31 busy; board I/O waits | `vram/gd5428.c`, `machine/nevent.c` | every row 0.9-1.1 |
| W5 | display (CRTC, double scan, HDR RGB565, start address), relay switching the shown picture, vertical sync period | `vram/gd5428.c`, `sdl2/` | screen against the photographs; period |
| W6-W8 | nothing | - | `/B` table 0.9-1.1 |

- Word writes: vaeg's iocore splits a word OUT into byte writes to port
  and port+1. The WSR-E must take writes to 44E1h, 4EE1h and 54E1h as the
  data of the paired index register. Whether to add a 16-bit handler to
  iocore instead is decided from W1PROBE's machine result.
- Unconnected reads: the machines keep the last bus value, vaeg returns
  FFh. W1PORT treats both as "not driven".
- Implementation: NP2kai `wab/cirrus_vga.c` (from QEMU, MIT) models a
  5446 and would run 5446-only features. Do not port it. Write the subset
  this track uses as a 5428 and match the machines; reading it for
  reference is fine.
- Settings and states: the board is a setting, absent by default. When
  absent nothing changes, including the state file; when present a new
  state section is saved.
- Records: each vaeg task file keeps a "vaeg divided by the photographs"
  table in the M141 form. Raw photographs, hardware logs and private asset
  identities remain outside Git in task-specific scratch under `~/work`,
  as required by AGENTS.md; publish independently written analysis only.

## 9. Hardware facts and their evidence

| Item | Content | Evidence |
| --- | --- | --- |
| WSR-E | Melco C-bus board, CL-GD5428, 1 MiB, PCM sound; Windows 95 driver shared with WAB-S, WAB-1000, WSR-G | `[DOC]` pc-9800.net, Buffalo driver page |
| E area release | port 152h bit 3 = 1 sends E0000-EFFFFh to the expansion slot | `[DOC]` PC-88VA Technical Manual 3.3 |
| Port 152h | readable (the ROM reads it as a word with `IN AX,DX`); INT 80h/81h/83h, INT 87h's extended driver and INT 94h save it and restore the saved value | `[DOC]` ROM disassembly |
| VA I/O blocks | 1000h-FEFFh is the user area for expansion boards | `[DOC]` Technical Manual 2.2 |
| Low-byte partners | E0h/E1h unused by the VA; E2h/E3h, E4h/E6h, E8h/E9h/ECh/EDh are V1/V2-mode only | `[DOC]` Technical Manual; current vaeg E8h decode differs (W1 finding below) |
| Current vaeg V3 E8h | Compatible Kanji reads are registered without a V3 guard; W1PORT reports a driven low-byte port with the new board disabled | `[CODE]` existing `io/cgromva.c`; local guest observation, not hardware measurement. See the W1 task's open scope conflict. |
| Port map | VGA 3C0h+n at 4nE0h; Melco and I-O DATA share it | `[OTHER-BOARD]` AGS_CLGD.COM, `[EMULATOR]` NP2kai |
| E window | E0000-E7FFFh (32 KiB), banked by GR09/GR0A | `[OTHER-BOARD]` AGS_CLGD.COM draws at E000h, `[EMULATOR]` |
| Word OUT | `OUT DX,AX` to 4nE0h with even n writes index (AL) and data (AH) | `[OTHER-BOARD]` AGS_CLGD.COM |
| Wake-up | 46E8h=18h, 42E0h=01h, 46E8h=08h | `[OTHER-BOARD]` AGS_CLGD.COM |
| 40E1h | B3h output to the board, 00h back to the VA (GA-98NB values) | `[OTHER-BOARD]`; meaning on the Melco board `[OPEN]` |
| 16-bpp transparency | source-colour transparency without colour expansion on the 5428 | `[OPEN]` (documented for the 5446) |
| 320x200x16 bpp | not a standard mode; VCLK, double scan, HDR | `[OPEN]` |
| VA slot | WAIT line, 16-bit I/O and memory cycles (BHE), A20-A23 | `[OPEN]` (schematic p.260) |
| WSR-E PCM ports | whether any falls in 0000h-0FFFh | `[OPEN]` |
| Effective VRAM bandwidth | full-screen and 24x24 BLTs | `[OPEN]` (W4) |

## 10. Memory path

The only path from the uPD9002 (20-bit addresses) to WSR-E VRAM is the
32 KiB window at E0000h. The F window (F00000h and up) is out of reach
and is not used.

- Port 152h: a low nibble of 1xxx releases E0000-EFFFFh to the slot. The
  low three bits are expected not to matter to the WSR-E `[INFERRED]`.
  Leave the high nibble (F area) alone.
- Banks: GR0B bit 0 = 1 selects two banks (E0000-E3FFFh by GR09,
  E4000-E7FFFh by GR0A); bit 5 = 1 makes the granularity 16 KiB.
  AGS_CLGD.COM uses GR0B = 2Dh.
- Window enable: in NP2kai 46E8h bit 3 enables the window decode
  `[EMULATOR]`.
- Interrupts: off while the window is used, in sections of at most one
  bank. The BIOS (INT 80h and others) saves and restores port 152h, so a
  call keeps the setting.
- Speed: `REP MOVSW` tops out near 1 MB/s (unmeasured, less with C-bus
  waits). CPU transfers are for loading images at start only.

## 11. I/O port map

All WSR-E VGA ports are 4nE0h, inside the VA user area. In V3 mode no VA
device sits on their low bytes (E0h/E1h/E8h).

| VGA port | WSR-E port | Use | Read every frame |
| --- | --- | --- | --- |
| 3C0h/3C1h | 40E0h/41E0h | attribute controller | no |
| 3C2h | 42E0h | misc output (wake-up write in setup mode) | no |
| 3C4h/3C5h | 44E0h/45E0h | sequencer (SR) | no |
| 3C6h | 46E0h | pixel mask, hidden DAC register (HDR) | no |
| 3C8h/3C9h | 48E0h/49E0h | DAC | no |
| 3CEh/3CFh | 4EE0h/4FE0h | graphics controller (GR, BLT registers) | yes (GR31 completion) |
| 3D4h/3D5h | 54E0h/55E0h | CRTC | no |
| 3DAh | 5AE0h | input status (vertical sync) | yes (page flip) |
| 46E8h | 46E8h | setup / enable | no |
| board | 40E1h, 42E1h, 51E1h/59E1h/5BE1h | relay, F window, ids (NP2kai's reading) | no |

- Word writes: 44E0h, 4EE0h and 54E0h take `OUT DX,AX`, provided the VA
  slot passes 16-bit I/O cycles.
- DX switching: index and data ports differ only in DH (44h/45h,
  4Eh/4Fh), so `INC DH` / `DEC DH` switches them.
- V1/V2 modes are not supported (E8h kanji ROM and others are live there).

## 12. Board initialisation

Taken from AGS_CLGD.COM (Mugen Houyou, PC-98 version, 1995-05-25), written
for the I-O DATA GA-98NB (GD5434) with the same port map. That program uses
32-bit instructions and does not run on the VA; port the sequence.

```text
; --- wake-up + probe ---
out 46E8h, 18h        ; setup mode + enable
out 42E0h, 01h        ; wake (written while in setup mode)
out 46E8h, 08h        ; leave setup, subsystem enabled
out 42E0h, E3h        ; misc output
out 5AE0h, 00h        ; feature control
out 44E0h, 1206h      ; word out: SR06 = 12h (unlock extensions)
in  al, 45E0h         ; expect 12h; other values must read back 0Fh

; --- mode set: 640x480x8bpp (GD5434 values) ---
SR (word out to 44E0h): 00=02 00=03 01=01 02=FF 03=00 04=0E 07=01
    0B=4A 1B=2B         ; VCLK0 ~25.2 MHz (14.318*74/(21*2))
    1F=1C               ; MCLK ~50.1 MHz
    0F=3D 16=71 17=35   ; DRAM ctl / FIFO / config: GD5434-specific
CRTC (word out to 54E0h): 11=00 (unlock), 00=5F 01=4F 02=50 03=82 04=54 05=80
    06=0B 07=3E 09=40 10=EA 11=0C 12=DF 13=50 15=E7 16=04 17=E3 18=FF 1B=22
GR (word out to 4EE0h): 05=40 06=05 07=0F 08=FF 09=00 0A=00 0B=2D ... 31=04 (BLT reset)
ATTR: check CR24 bit 7 (flip-flop), reset via 41E0h/40E0h,
    then 10=41 11=00 12=0F 13=00 14=00, index|20h to enable
DAC: 46E0h<-FFh, 4x in 46E0h, out 46E0h,20h (HDR), out 46E0h,FFh

; --- output switch ---
out 40E1h, B3h        ; accelerator output on (bit 0 = relay)
out 40E1h, 00h        ; on exit

; --- VRAM access ---
; ES = E000h; bank select: out 4EE0h, (bank << 8) | 09h   ; GR0A for the second half
```

For the 5428:

- SR0F (DRAM control) and SR17 differ from the 5434. SR17 = 35h sets bit 2,
  which on the 5434 enables MMIO at B8000h. Take these from the 5428 TRM.
- SR07 = 01h, SR0B/SR1B (VCLK), SR1F (MCLK) and GR0B = 2Dh are expected to
  carry over `[INFERRED]`.
- 16 bpp needs the HDR value, the CRTC offset (CR13), 320x200 timing and
  double scan, all from the 5428 TRM.
- 40E1h values are the GA-98NB's; their meaning on the Melco board is open.
- DRAM refresh: the GD542x refreshes VRAM with the CRTC scan, so set a mode
  (start the CRTC) before using the window or BLTs. From W2 on, set
  640x480x8 first with the output left on the VA. NP2kai treats 40E1h bit 1
  as "DRAM refresh" `[EMULATOR]`.
- 320x200x16: follow VGA 320x200 (640x400 scan, 31.5 kHz, double scan).
  VCLK, dot clock division and HDR for 16 bpp are fixed in W5.

## 13. Drawing and BLT

A frame runs BLTs in series: clear (or background), balls, HUD, page flip.
The 5428 has no queued start (AutoStart), so each BLT is set up only after
the previous one completes.

1. Clear the hidden page (320x200; a fill BLT writes 125 KiB). W8 copies a
   background instead (250 KiB per layer).
2. W8 two-layer: the front layer with a transparent BLT (250 KiB full, or
   in proportion to coverage when only needed rectangles are drawn).
3. 24x24 balls with transparent BLTs (2.25 KiB each, read + write).
4. HUD glyphs with transparent BLTs.
5. Wait for vertical sync, switch the CRTC start address.

Per ball, write source address (GR2C-GR2E), destination address
(GR28-GR2A) and start (GR31): 7 registers, 7 word OUTs. Width, height,
pitches, mode, ROP and transparent colour stay set while the size is
unchanged.

| Register | Content | Written |
| --- | --- | --- |
| GR20/21, GR22/23 | width, height | when the size changes |
| GR24/25, GR26/27 | destination and source pitch (independent) | at start |
| GR28-GR2A | destination address | every BLT |
| GR2C-GR2E | source address | every BLT (unchanged high bytes may be skipped) |
| GR30 | BLT mode (bit 4 = 16 bpp, transparency enable) | at start |
| GR31 | start / status (bit 0), BLT reset (bit 2) | every BLT |
| GR32 | ROP | at start |
| GR34/35 | transparent colour (16-bpp behaviour on the 5428 `[OPEN]`) | at start |

- Completion: `OUT 4EE0h,31h`, then `IN 4FE0h`, bit 0. Byte reads.
- Issue cost: assumed about 350 clocks (44 us) per BLT with byte OUTs.
  For 24x24 the issue costs more than the transfer, so word OUTs and
  skipped high bytes matter (W7).
- Game logic: the CPU is free during clear and background BLTs; put the
  logic there. During balls, issue and transfer alternate and little can
  overlap.
- Loading: at start, send the 16 ball images (1,152 bytes each) and the HUD
  glyphs to VRAM through the E window. No CPU pixel transfers while running.
- Map updates (W8): tiles resident in VRAM, placed with screen-to-screen BLTs.
- If transparency does not work: a two-pass method (colour-expand a 1-bpp
  mask to clear, then OR), about 4.5 KiB and 2 BLTs per ball.

## 14. Performance model

Model: frame time = bytes / effective bandwidth + BLTs x 44 us. Ball 2.25
KiB read+write, clear 125 KiB written, a full background layer 250 KiB.
Upper bounds; small BLTs run below the full-screen bandwidth (W4 D rows).

| Configuration | 40 MiB/s | 60 MiB/s | 80 MiB/s |
| --- | --- | --- | --- |
| Clear only (W6), balls at 60 fps | 137 | 181 | 211 |
| One background layer (W8), balls at 60 fps | 106 | 156 | 190 |
| Two background layers (W8), balls at 60 fps | 44 | 105 | 147 |
| Two background layers (W8), balls at 30 fps | 213 | 312 | 381 |

- Background alone with two layers: 12.3 / 8.2 / 6.2 ms at 40 / 60 / 80 MiB/s.
- Issue cost: 200 clocks a BLT instead of 350 raises clear-only at
  40 MiB/s from 137 to 170 balls; for 24x24 the issue matters more than
  the bandwidth.
- Game logic hides only under clear and background BLTs.
- Dirty rectangles (restore + draw, 4.5 KiB and 2 BLTs a ball) break even
  with a full redraw near 56 balls; not worth it with many balls.

## 15. VRAM layout

| Use | Size | Bytes |
| --- | --- | --- |
| Display pages (2) | 320x200x2x2 | 250 KiB |
| Background A (W8) | 512x256x2 | 256 KiB |
| Background B (W8 two-layer) | 512x256x2 | 256 KiB |
| Ball images | 24x24x2x16 | 18 KiB |
| HUD glyphs | set by gen_data.py | a few KiB |
| Free | | about 240 KiB |

- W6 (no background) leaves more than 750 KiB.
- Ball images packed at a 48-byte source pitch (source and destination
  pitches are independent).
- Pages flip by start address; align page 1 to 4 bytes.
- A 512x256 background used as a ring needs up to 4 BLTs per layer when
  the view wraps.
- Larger maps: expand only the area near the screen from VRAM tiles.

## 16. Open questions

1. Does the 5428 do 16-bpp source-colour transparency without colour
   expansion (W3)? If not, two passes, and 60 fps with two backgrounds in
   W8 is out.
2. Does the VA slot honour WAIT, pass 16-bit I/O and memory cycles, and
   what are A20-A23 (W1PROBE, W2, schematic p.260)?
3. 320x200x16 mode values: VCLK, double scan, HDR (W5).
4. Do any WSR-E PCM ports fall in 0000h-0FFFh; can the PCM part be
   disabled by DIP switch?
5. Melco values of 40E1h and the id ports 51E1h/59E1h/5BE1h (W1PROBE).
6. May interrupts stay enabled while port 152h releases the E area (W2's
   last test)? If yes, the window needs no CLI.
7. Clocks per VRTC on each machine and at 24.8 kHz (W1PORT R0).

## 17. Sources

Primary (local):

- PC-88VA Technical Manual 2.2 (I/O port map), 3.3 (ROM area bank switching).
- AGS_CLGD.COM (Mugen Houyou PC-98, DISK01, 1995-05-25), disassembled;
  ALICESOFT ARCHIVES, http://retropc.net/alice/ (freely distributable).
- VA ROM and getromva.exe (VAEGTOOL070422.LZH), compared.

Web:

- [pc-9800.net C-bus graphics accelerators](https://www.pc-9800.net/db2/ga_cbus.htm)
- [Buffalo Windows 95 display driver utility](https://www.buffalo.jp/support/download/detail/?dl_contents_id=61292)
- [NP2kai (wab/cirrus_vga.c, cirrus_vga_extern.h)](https://github.com/AZO234/NP2kai)
- [XFree86 3.3.6 Cirrus: supported chipsets](https://xfree86.org/3.3.6/cirrus1.html)
- [XFree86 3.3.6 Cirrus: dot clock and BitBLT performance](https://xfree86.org/3.3.6/cirrus4.html)
- [CL-GD5446 Technical Reference Manual](https://theretroweb.com/chip/documentation/cl-gd5446technical-6456556f945a1573066314.pdf)
- [CL-GD5446 Preliminary Data Book](https://theretroweb.com/chip/documentation/cl-gd5446-smol-645654db1ae03286068933.pdf)

vaeg:

- `AGENTS.md` (measurement software rules, demos and disks)
- `demos/sgp-pseudo-sprite/65536`, `demos/sgp-contention` (600-VRTC rows),
  `demos/tsp-sprites` (machine id, `--msdos4` build)
- `docs/agents/tasks/M130_v30_timing.md` (clocks per VRTC, R0 reference),
  `docs/agents/tasks/M141_tsp_sprite_timing.md` (ratio table, gate form)
