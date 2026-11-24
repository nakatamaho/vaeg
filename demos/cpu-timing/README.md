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

# CPUALIGN: main-CPU loops at even and odd addresses

`CPUALIGN.COM` (`cpualign.asm`, M165) measures the same main-CPU loops
twice, once with the loop's first instruction at an even address and once
at an odd one, so that vaeg's timing of a branch to an odd address can be
told apart from the cost of the memory accesses. It was written after
SGPMEM (`demos/sgp-contention`) found vaeg's main RAM byte-read loop about
4 clocks an iteration fast on both machines, a loop that happened to start
at an odd address (see
[va-bus-timing.md](../../docs/modernization/va-bus-timing.md)).

An MS-DOS 4.0 program for a PC-88VA, PC-88VA2 or PC-88VA3 (and vaeg): type
`CPUALIGN`; it runs about a minute at 15 kHz or 24.8 kHz, leaves the
screen mode alone, prints the table through DOS and writes it to
`CPUALIGN.TXT`. The first line names the machine (F000:FFFE) and the
display frequency.

Each row repeats a chunk for 120 VRTCs with only the VRTC interrupt
enabled and counts the chunks: `EVEN`, `ODD`, and their ratio. A chunk is
64 iterations of a loop (the body, `DEC CX`, `JNZ`; DS = ES = the row's
memory) or 64 straight instructions:

| Row | Body | Memory |
|---|---|---|
| `REG ADD` | `ADD AX,BX`, `INC DI` | none |
| `RAM READ B ES:` | `MOV AL,ES:[DI]`, `INC DI` | main RAM (CS + 1000h) |
| `RAM READ B` / `READ W` | `MOV AL,[DI]` / `MOV AX,[DI]` | main RAM |
| `RAM WRITE B` / `WRITE W` | `MOV [DI],AL` / `MOV [DI],AX` | main RAM |
| `RAM LODSW AND OR` | `LODSW`, `AND AL,ES:[DI]`, `OR AL,AH`, `STOSB` (CPUBENCH's `LOOP`) | main RAM |
| `GVRAM ...` | the same | GVRAM at A0000h (port 153h 54h, single plane) |
| `TVRAM ...` | the same | TVRAM offset 7000h (port 153h bank 1) |
| `64 MOV AX,IMM` | 64 x `MOV AX,imm16` (3 bytes) | none |
| `64 MOV RAM,IMM` / `GVRAM,IMM` | 64 x `MOV WORD [DI+d16],imm16` (6 bytes, CPUBENCH's `COMPLD`) | main RAM / GVRAM |

The straight rows' `ODD` places the first instruction at an odd address.

```sh
demos/cpu-timing/build.sh [--msdos4 msdos4-pc88va-2hd.d88] [OUTDIR]
```

With `--msdos4` (the FreeDOS-88VA msdos4-va.2 disk) the script also writes
`cpu-timing-msdos4.d88`, a vanilla MS-DOS 4.0 copy with the program.

## On the machines (2026-11-23)

PC-88VA and PC-88VA2/VA3, 15 kHz (24.8 kHz gives the same ratios, every
count 1.09 times higher):

```text
CPUALIGN  PC-88VA  15 KHZ  CHUNKS IN 120 VRTC
LOOP (64 TIMES)     EVEN   ODD  ODD/EVEN
REG ADD             9781  8695  0.888
RAM READ B ES:      6858  6138  0.895
RAM READ B          6909  6886  0.996
RAM READ W          6138  6138  1.000
RAM WRITE B         7559  7084  0.937
RAM WRITE W         6670  6324  0.948
RAM LODSW AND OR    4408  4299  0.975
GVRAM READ B        6138  6137  0.999
GVRAM WRITE B       8085  7082  0.875
GVRAM WRITE W       7057  6321  0.895
GVRAM LODSW AND OR  3752  3749  0.999
TVRAM READ B        5097  5033  0.987
TVRAM WRITE B       7265  6341  0.872
64 MOV AX,IMM      26921 26794  0.995
64 MOV RAM,IMM     11196 12340  1.102
64 MOV GVRAM,IMM   10696 11727  1.096
```

```text
CPUALIGN  PC-88VA2/VA3  15 KHZ  CHUNKS IN 120 VRTC
LOOP (64 TIMES)     EVEN   ODD  ODD/EVEN
REG ADD             9834  8748  0.889
RAM READ B ES:      6911  6210  0.898
RAM READ B          6963  6930  0.995
RAM READ W          6197  6183  0.997
RAM WRITE B         7640  7159  0.937
RAM WRITE W         6719  6347  0.944
RAM LODSW AND OR    4411  4346  0.985
GVRAM READ B        6215  6182  0.994
GVRAM WRITE B       8147  7152  0.877
GVRAM WRITE W       7072  6345  0.897
GVRAM LODSW AND OR  3779  3777  0.999
TVRAM READ B        5118  5070  0.990
TVRAM WRITE B       7325  6384  0.871
64 MOV AX,IMM      27076 26966  0.995
64 MOV RAM,IMM     11276 12423  1.101
64 MOV GVRAM,IMM   10763 11809  1.097
```

What they show (the two models within 1%):

- An odd loop address costs 3-5 clocks an iteration in loops that do not
  read memory early (register, byte and word writes), and almost nothing
  in loops that read memory without a segment prefix (main RAM, GVRAM,
  TVRAM): the wait for the data hides the late instruction bytes. With
  an `ES:` prefix the read loop pays 4 clocks at an odd address.
- A main RAM read without a prefix costs as much as one with it at an even
  address (the prefix is free there).
- A GVRAM byte write is faster than a main RAM one (29.9 against 31.9
  clocks an iteration at an even address).
- Straight 6-byte `MOV`s are 10% faster starting at an odd address (the
  first 5 bytes of each instruction arrive in fewer bus cycles).

## vaeg (M165), 15 kHz

The PC-88VA and the PC-88VA2 give the same table:

```text
CPUALIGN  PC-88VA  15 KHZ  CHUNKS IN 120 VRTC
LOOP (64 TIMES)     EVEN   ODD  ODD/EVEN
REG ADD            10032  8636  0.860
RAM READ B ES:      6906  6918  1.001
RAM READ B          7786  6918  0.888
RAM READ W          7107  6546  0.921
RAM WRITE B         8318  7335  0.881
RAM WRITE W         7107  6727  0.946
RAM LODSW AND OR    5150  4850  0.941
GVRAM READ B        6362  5770  0.906
GVRAM WRITE B       8043  7120  0.885
GVRAM WRITE W       6906  6546  0.947
GVRAM LODSW AND OR  3910  3734  0.954
TVRAM READ B        5044  4665  0.924
TVRAM WRITE B       7318  6546  0.894
64 MOV AX,IMM      24926 24846  0.996
64 MOV RAM,IMM     11984 11937  0.996
64 MOV GVRAM,IMM   11422 11379  0.996
```

vaeg's odd-address cost is the same 3-4 clocks in every loop, and its
even-address read loops are 3-5 clocks fast; across the 32 counts it is
7.2% off (root mean square, at most 16.8%). See
[va-bus-timing.md](../../docs/modernization/va-bus-timing.md#open-questions)
for the corrections tried and why none was kept.

