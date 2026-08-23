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

# GLASS ORBIT GA-3 palette and color-bar proof

GA-3 proves that VAEG can visibly distinguish all sixteen palette-set-0
entries in the already approved G0 640 by 200 single-plane 4bpp mode. It is a
diagnostic CPU-rendered color-bar stage. It does not start SGP, configure a
second framebuffer, define a final GLASS face palette, or make a physical
PC-88VA conformance claim.

## BIOS palette contract

The raw `GLASSG3.BIN` image first invokes the GA-2 `$ScnMode` entry contract:

```text
INT 8Fh: AH=00h, BX=A002h, CX=0004h, DX=0000h
```

It then invokes `$PalCtl` with `AH=09h, AL=00h` to select palette mode 0 and
uses `$SetPal` sixteen times:

```text
INT 8Fh: AH=08h, AL=palette index 0..15, CX=palette value
```

Every BIOS call must return `AX=0`. The diagnostic table intentionally uses
the manual's documented palette-mode-0 reset values:

| Index | Value | Intended diagnostic colour |
|---:|---:|---|
| 0 | `0000h` | black |
| 1 | `001Fh` | bright blue |
| 2 | `03E0h` | bright red |
| 3 | `03FFh` | bright magenta |
| 4 | `FC00h` | bright green |
| 5 | `FC1Fh` | bright cyan |
| 6 | `FFE0h` | bright yellow |
| 7 | `FFFFh` | white |
| 8 | `7DEFh` | grey |
| 9 | `0015h` | dark blue |
| 10 | `02A0h` | dark red |
| 11 | `02B5h` | dark magenta |
| 12 | `AC00h` | dark green |
| 13 | `AC15h` | dark cyan |
| 14 | `AEA0h` | dark yellow |
| 15 | `AEB5h` | light grey |

Evidence: `[VA-TEKU:606GRP.TXT sections 6.6.2-6.6.3, functions 0, 8, 9]`.
The local Technical Manual is maintainer-local and is cited here, never from a
source comment.

This is a diagnostic profile only. The later GLASS face indices 8 through 13
will be replaced by visually tuned 75-percent-brightness face colours under
the approved rendering design. GA-3 deliberately does not prejudge those
values.

## Bar geometry

The CPU uses the already proven single-plane GVRAM mapping:

```text
OUT 0153h <- 54h
OUT 0580h <- 10h
ES = A000h
```

Each logical line contains sixteen adjacent 40-pixel bars. A bar is ten packed
words, with each word repeating the one palette index in all four nibbles:

```text
bar 0: 0000h    bar 1: 1111h    ...    bar 15: FFFFh
```

The 16 bars fill all 640 pixels in each of the 200 logical rows. `$Compose`
with `AH=03h, AL=0, CX=0003h` then selects G0 only, and `$ScnDsp` with
`AH=0Bh, AL=1` enables graphic display. No graphics BIOS drawing primitive is
used.

## VAEG capture and acceptance

`build-ga3.sh` creates `GLASSG3.BIN` and its local PC-Engine loader
`GLASSP3.COM`. `build-ga3-bootable-d88.sh` and `run-vaeg-ga3.sh` make a local
bootable test disk from a caller-provided PC-Engine template. The generated
disk, ROMs, logs, and screen images are private integration artifacts and are
not committed.

The M74 script waits for `2000:0100` and captures the renderer. The checker
fails closed unless it observes:

1. the `4743h` success marker and the approved segment / stack boundary;
2. a 640-pixel viewport with a 400-row composition canvas;
3. 200 logical rows, each made of sixteen uniform 40-pixel bars;
4. sixteen distinct sampled RGB values, one for each bar; and
5. 200 black intervening rows, making the selected 200-line output boundary
   explicit in VAEG's 640 by 400 composition canvas.

The resulting image must also be opened and visually checked: it must show
sixteen vertical coloured bands, not merely non-black framebuffer activity.
The alternating black rows are a VAEG observation for this 200-line capture,
not a claim about real-hardware display timing.

`tools/repo/find_unreferenced.py --report` lists the two GA-3 assembly sources
because an explicit local NASM script, not a production CMake root, builds
them. `build-ga3.sh` and the local loader own those sources; they are not
deletion candidates.
