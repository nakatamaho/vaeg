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

# Z80TIME 2: compatible mode, instruction by instruction

`Z80TIME.COM` (`z80time.asm`) is an MS-DOS program for a PC-88VA or PC-88VA2
(and vaeg). It measures the uPD9002's compatible (uPD70008, Z80) mode one
instruction at a time, to calibrate vaeg's compatible-mode timing (M135,
issue #62). It contains no data of any other program. The font is synthetic
(`gen_font.py`, from `demos/sgp-bench`), and the screen set-up is that of
`demos/sgp-contention`.

Each row counts the blocks done in N VRTCs: by default 1200 (20 s a row,
about 17 minutes in all), or `Z80TIME 60` for a quick run. More is faster.

- **Compatible-mode block:** entered by `BRKEM`, left by `RETEM`. It sets
  `EI`, HL = IX = 0100h, DE = 0200h, SP = 0E00h, `XOR A` (Z set) and B = 0,
  then makes 256 passes of eight copies of one instruction and `DJNZ`.
  - One instruction takes `(N x 133460 / count - N x 133460 / BASE) / 2048`
    clocks (7.9872 MHz, 133,460 clocks a VRTC at 15.98 kHz).
  - The "NOT" rows are conditional instructions not taken (Z set).
  - `PUSH+POP` and `CALL+RET` are pairs: the figure is the pair's.
- **`LDIR BYTE`:** copies 2048 bytes once a block. A byte takes
  `N x 133460 / (count x 2048)`, the block's set-up included.
- **`BRKEM+RETEM`:** 2048 round trips from a native `LOOP` a block, each to
  a `RETEM`.
- **N rows:** native V30 code, 256 passes of eight instructions and
  `DEC CX`, `JNZ`, for comparison with M130.
- **Placement:** the code runs at 20000h; the CODE HI rows run a copy at
  70000h. HI data is at 72800h, LO data at 30000h. The PC-88VA inserts one
  wait in main RAM from 40000h; the PC-88VA2 inserts none.
- The native VRTC handler counts VRTCs and runs during the blocks.

| Row | Instruction |
|---|---|
| 1 | BASE DJNZ |
| 2 | NOP |
| 3 | LD A,B |
| 4 | LD A,N |
| 5 | LD DE,NN |
| 6 | INC A |
| 7 | INC DE |
| 8 | ADD A,C |
| 9 | ADD A,N |
| 10 | ADD HL,DE |
| 11 | AND N |
| 12 | RLCA |
| 13 | DAA |
| 14 | EX DE,HL |
| 15 | EXX |
| 16 | LD A,(HL) HI |
| 17 | LD A,(HL) LO |
| 18 | LD (HL),A HI |
| 19 | LD (HL),A LO |
| 20 | LD A,(DE) HI |
| 21 | INC (HL) HI |
| 22 | LD A,(NN) HI |
| 23 | LD (NN),A HI |
| 24 | LD HL,(NN) HI |
| 25 | LD (NN),HL HI |
| 26 | PUSH+POP HI |
| 27 | CALL+RET HI |
| 28 | JR TAKEN |
| 29 | JR NZ NOT |
| 30 | JP NN |
| 31 | JP NZ NOT |
| 32 | RET NZ NOT |
| 33 | LD A,(IX+D) HI |
| 34 | LD (IX+D),A HI |
| 35 | INC (IX+D) HI |
| 36 | ADD IX,DE |
| 37 | LD IX,NN |
| 38 | BIT 0,A |
| 39 | SRL A |
| 40 | SET 0,(HL) HI |
| 41 | RL (IX+D) HI |
| 42 | NEG |
| 43 | LDIR BYTE HI |
| 44 | CALLN+IRET |
| 45 | BRKEM+RETEM |
| 46 | NOP CODE HI |
| 47 | LD A,(HL) CODE HI |
| 48 | N BASE DEC JNZ |
| 49 | N NOP |
| 50 | N MOV AL,(BX) HI |

The counts are also stored at GVRAM 30000h (D000:0000h, not shown): `Z8T2`,
the number of rows (16-bit), N (16-bit), then the counts (32-bit). In vaeg,
`capture ID gvram` after `wait-pc 2000:03E8` (the program's end) gives them
at offset 30000h of the dump.

The program needs a 15 kHz screen and memory up to segment 7400h. It copies
itself to 2000:0000 and 7000:0000 and does not return: restart the machine
to leave.

```sh
./build.sh [--msdos4 msdos4-pc88va-2hd.d88] [OUTDIR]
```

With `--msdos4` (the FreeDOS-88VA msdos4-va.2 disk) the script also writes
`z80time-msdos4.d88`, a vanilla MS-DOS 4.0 copy with the program; type
`Z80TIME`.

## Z80TIME 1 on the machines (2026-10-25)

The first version (branch `bench/z80time`, issue #62) measured single
instructions with one copy a pass. Compatible mode took about 1.35 times
vaeg's Z80 counts for branches, memory accesses and CALL/RET, but less for
ALU and IX-indexed instructions; native code agreed within 0.90-1.05. The
figures are in the issue.
