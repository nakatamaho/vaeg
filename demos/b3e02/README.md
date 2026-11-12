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

# PC-9801B3-E02 on the PC-88VA — probes (M148)

Probes for stages B0–B3 of
[`docs/agents/tasks/M148_b3e02_va_connection.md`](../../docs/agents/tasks/M148_b3e02_va_connection.md):
does a PC-9801B3-E02 (CL-GD5428, C bus) answer in a PC-88VA slot? MS-DOS
4.0 programs (msdos4-va.2), V3 mode. Synthetic data only.

| File | Stage | Board | Machine | What it does | Writes |
| --- | --- | --- | --- | --- | --- |
| `B3E0.COM` | B0 | absent (optionally present) | VA, VA2/VA3 | every B3-E02 port with six code patterns; B0000h (153h SMBC=0) and E0000h (152h bit 3) through the slot, four offsets, two patterns | 152h/153h for a few reads with interrupts off, restored and read back |
| `B3E1.COM` | B1 | present | VA | 0FA2h index 00h/01h/03h/04h, each read back; 0FA3h data; ID, window, relay/MMIO, DIP decoded | 0FA2h only; incoming index restored |
| `B3E2.COM` | B2 | present | VA | ports, CR27 (0B55h and 0D55h) and the SR06 lock before and after the wake-up `0904h=00h, 0902h=01h, 0904h=20h`; the wake-up undone | CRTC/SR indexes (restored), SR06 (left locked), 0902h/0904h; `/MMIO` and `/RELAY` also board reg 03h bit 0 / bit 1 before the wake-up, restored at the end |
| `B3E3.COM` | B3 | present | VA | after the wake-up and a CR27 check: Misc, Feature, Input Status 0, DAC state, SR00–1F, GR00–3F, CR00–3F with SR06 unlocked | as B3E2, with the same options (SR06 unlocked for the dump, relocked) |

None writes main RAM, the VRAM window or board reg 01h (window); only
`/RELAY` writes the relay bit. Ports print as `FF` (undriven), `D:xx` (six equal reads) or `F:` and
six bytes (the reads differ: floating bus). Every line goes to the screen
and is appended to `A:\B3LOG.TXT`; the header line names the build id,
DOS date and time and the machine from F000:FFFE.

Exit codes: 0 pass, 1 option, 2 machine, 3 no board, 4 log not written,
10h and up stage failures (B0: 10h driven port, 11h floating port, 12h
152h/153h not restored; B1: 10h ID, 11h index readback, 12h index not
restored; B2: 10h CR27, 11h SR06, 12h not restored; B3: 10h CR27).

## Build

```sh
TMPDIR=$HOME/work/pc88va/tmp demos/b3e02/build.sh "$HOME/work/pc88va/tmp/m148/media"
```

writes the four programs and `b3e02-msdos4.d88`, a vanilla copy of the
FreeDOS-88VA `msdos4-va.2` disk (from the shared cache, SHA-256 checked;
`--msdos4 IMAGE` overrides) with the programs and `B3E02.TXT` in its root.
The disk stays out of Git.

## Smoke run without hardware

`smoke.py` runs one program under Unicorn with a minimal DOS and a fake
board built on the task file's assumptions (not on measurements):

```sh
python3 demos/b3e02/smoke.py B3E2.COM /MMIO --board --expect 0
python3 demos/b3e02/smoke.py B3E2.COM --deadchip --expect 0x10
```

It checks control flow, printable output, the log copy and the cleanup
(152h/153h, board index, reg 03h, POS102, SR06, VGA indexes).
