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

# Melco WGN-V on the PC-88VA — probes (M148)

Probes for the WGN-V track of
[`docs/agents/tasks/M148_b3e02_va_connection.md`](../../docs/agents/tasks/M148_b3e02_va_connection.md)
(section 9): does a Melco WGN-V2/V4 (S3 Trio64V+, C bus) work in a
PC-88VA slot through the "E bank" window at E0000h? MS-DOS 4.0 programs
(msdos4-va.2), V3 mode. Synthetic data only. They use the common code of
`../b3e02/b3e02.inc` and log to `A:\WVLOG.TXT`.

| File | Stage | Board | Machine | What it does | Writes |
| --- | --- | --- | --- | --- | --- |
| `WV0.COM` | V0 | absent or present | VA, VA2 | 51E1h, 59E1h, 53E0h–55E1h at every board offset n = 0–0Eh (six code patterns each), and eight bytes of the E0000h window (152h bit 3, interrupts off) | 152h for the window reads, restored and read back |
| `WV1.COM` | V1 | present | VA, VA2 | the Windows 95 driver's E-bank set-up (without its port 6Ah write), S3 id CR2E/CR2D, CR30/2F/36/31/35/51/58–5A/6A/40/11, and a VRAM pattern in the window that must survive a switch to the register view and back | board ports; window registers (Misc, CR38/39 unlocked then locked, CR40/11/35, SR08/SR11); four VRAM bytes. The board stays set up: power-cycle afterwards |

Exit codes: 0 pass, 1 option, 3 no board, 4 log not written; WV1: 10h
no Trio64V+ in the window, 11h VRAM did not read back; WV0: 12h 152h not
restored.

## Build

```sh
TMPDIR=$HOME/work/pc88va/tmp demos/wgnv/build.sh "$HOME/work/pc88va/tmp/m148/wgnv-media"
```

writes `WV0.COM`, `WV1.COM` and `wgnv-msdos4.d88` (a vanilla copy of the
msdos4-va.2 disk with the programs and `WGNV.TXT`). The disk stays out
of Git.

## Smoke run without hardware

`smoke.py` uses the DOS of `../b3e02/smoke.py` with a fake board built on
the task file's reading of the driver (not on measurements):

```sh
python3 demos/wgnv/smoke.py WV1.COM --board --expect 0
python3 demos/wgnv/smoke.py WV1.COM --deadchip --expect 0x10
```
