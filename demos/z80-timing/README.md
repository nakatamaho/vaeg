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

# Z80TIME: compatible mode against native code

`Z80TIME.COM` (`z80time.asm`), an MS-DOS program for a PC-88VA or
PC-88VA2 (and vaeg), measures the uPD9002's compatible (uPD70008, Z80)
mode against native V30 code: single Z80 instructions in a loop, LDIR,
the CALLN round trip to native code, BRKEM-RETEM, and the same work in
V30 code. It contains no data of any other program; the font is
synthetic (`gen_font.py`, from vaeg's `demos/sgp-bench`), the screen
set-up is vaeg's `demos/sgp-contention`.

Each row counts the blocks done in N VRTCs (default 3000, about 50 s a
row, 15 minutes in all; `Z80TIME 60` for a quick run). More is faster.

| Row | Block |
|---|---|
| A | compatible mode: 1024 passes of `DJNZ` (with `DEC C`, `JR NZ` every 256) |
| B, C | the same with `LD A,(HL)` / `LD (HL),A`, data at 72800h ("HI") |
| D, E | the same, data at 30000h ("LO") |
| F | `ADD A,B` |
| G | `LD A,(IX+5)`, HI |
| H | `CALL` to a `RET`, stack HI |
| I | `LDIR` of 1024 bytes, HI |
| J | `CALLN` to a native `IRET` |
| K | 1024 `BRKEM` from a native `LOOP`, each to a `RETEM` |
| L | native: 1024 passes of `DEC CX`, `JNZ` |
| M, N | the same with `MOV AL,[BX]` / `MOV [BX],AL`, HI |
| O | `MOV AL,[BX]`, LO |
| P | `ADD AL,BL` |
| Q | `CALL` to a `RET` |
| R | `REP MOVSB` of 1024 bytes, HI |

A compatible-mode block (rows A-J) is entered by `BRKEM` and left by
`RETEM`, with `EI` first; the native VRTC handler (it counts VRTCs) runs
during it. The program runs at 20000h (below 40000h, where the PC-88VA
inserts one wait in main RAM and the PC-88VA2 none); HI data is above
that boundary, LO data below.

Clocks a pass: `N x 133460 / (count x 1024)` (7.9872 MHz, 59.85 VRTCs a
second at 15.98 kHz), less row A's (compatible mode) or row L's (native)
for the loop alone. Row K: `N x 133460 / (count x 1024)` is one `BRKEM`,
`RETEM` and `LOOP`.

The counts are also stored at GVRAM 30000h (D000:0000h, not shown):
`Z80T`, 18 counts (32-bit), N (16-bit). In vaeg, `capture ID gvram` after
`wait-pc 2000:03D2` (the program's end) gives them at offset 30000h of
the dump.

The program needs a 15 kHz screen and memory up to segment 7400h, copies
itself to 2000:0000 and does not return: restart the machine to leave.

```sh
./build.sh [--msdos4 msdos4-pc88va-2hd.d88] [OUTDIR]
```

With `--msdos4` (the FreeDOS-88VA msdos4-va.2 disk) the script also
writes `z80time-msdos4.d88`, a vanilla MS-DOS 4.0 copy with the program
(vaeg's `tools/pc88va/pcengine_disk.py`, or `PCENGINE_DISK`); type
`Z80TIME`.

## vaeg (main 4c2354cf), N = 60

| Row | VA | VA2 |
|---|---:|---:|
| A | 582 | 582 |
| B | 365 | 383 |
| C | 365 | 383 |
| D | 383 | 383 |
| E | 383 | 383 |
| F | 449 | 449 |
| G | 234 | 241 |
| H | 176 | 194 |
| I | 335 | 366 |
| J | 100 | 100 |
| K | 95 | 95 |
| L | 408 | 408 |
| M | 278 | 288 |
| N | 260 | 268 |
| O | 288 | 288 |
| P | 353 | 353 |
| Q | 147 | 147 |
| R | 690 | 858 |
