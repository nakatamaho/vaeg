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

# WSR-E pseudo-sprite track

Test programs, and later a demo, for the Melco WSR-E (CL-GD5428) on the
PC-88VA family under MS-DOS 4.0. The plan, the log format and the stages
are in
[`docs/modernization/wsr-e-pseudo-sprite-plan.md`](../../docs/modernization/wsr-e-pseudo-sprite-plan.md).
Programs accumulate: every stage keeps the earlier ones.

## Programs

| File | Stage | Board | What it does |
| --- | --- | --- | --- |
| `W1PORT.COM` | W1 | not needed | R0 calibration; probes 00E0h/00E1h/00E8h and the WSR-E ports; port 152h read-back with ROM area 0 released to the slot for a moment. Writes no WSR-E port. |
| `W1PROBE.COM` | W1 | needed | wake-up as AGS_CLGD.COM, SR06 lock by byte and by word OUT, CR27, GR09/GR0A by byte and by word OUT, board id ports. Never writes 40E1h, so the monitor stays on the VA. |

Every program prints CSV lines (with `#` comment lines) to the screen and
appends them to `WSRLOG.TXT` in the current directory. The first fields
are the stage, the build id (8 hex digits of the source commit), the
machine from F000:FFFE, the VA display rate, the board and CR27. Timing
rows count completions in 600 VRTCs; R0 runs three times, about 30 s.

Exit codes: 0 pass, 1 a check failed, 2 cannot measure (no board for
W1PROBE, no VRTC interrupt, or log I/O failure), 3 stopped with ESC (later stages).

## Running on a machine

1. Boot the MS-DOS 4.0 disk in V3 mode; put the program disk in a drive
   that is not write-protected (the log is written there).
2. Without the board: `W1PORT`. Photograph the screen.
3. With the board: `W1PORT` again, then `W1PROBE`. Photograph each.
4. Copy `WSRLOG.TXT` off the disk. When a photograph and the log
   disagree, the log is right.

W1PORT writes port 152h with interrupts off for a few instructions and
restores it. W1PROBE restores the incoming 46E8h and GR09/GR0A values
and leaves SR06 locked. It never writes the relay port 40E1h.

## Build

```sh
NASM=nasm demos/wsr-e-pseudo-sprite/build.sh /tmp/wsre
NASM=nasm demos/wsr-e-pseudo-sprite/build.sh --msdos4 msdos4-pc88va-2hd.d88 /tmp/wsre
```

The second form also writes `/tmp/wsre/wsr-e-pseudo-sprite-msdos4.d88`, a
vanilla copy of the FreeDOS-88VA `msdos4-va.2` disk with the programs in
its root. `BUILD_ID` overrides the build id. Generated files stay out of
Git.

## Smoke run without hardware

```sh
pip install unicorn
python3 demos/wsr-e-pseudo-sprite/smoke.py /tmp/wsre/W1PORT.COM --float next
python3 demos/wsr-e-pseudo-sprite/smoke.py /tmp/wsre/W1PROBE.COM --board
```

`smoke.py` checks control flow, the CSV lines, the exit code and that the
IRQ2 vector, the PIC mask and port 152h are restored. Its port models are
guesses and its timings mean nothing; vaeg and the machines are the tests.
The script returns the guest exit code (including 2 for no board) and fails
on missing/invalid CSV or failed cleanup. `--log PATH` saves its synthetic
CSV output. The four required models are default, `--float self`,
`--float next`, and `--board`.

Host comparison: `python3 tools/wsre/compare_logs.py MACHINE_LOG VAEG_LOG`.
This compares row identities and metadata (excluding build id), reports each
check value, and reports vaeg/machine timing ratios and repeat variation.
`FF` and `FLOAT ...` probe results are the same undriven class. Only for
emulator fixture comparisons, `--ignore-r0-counts` suppresses R0 timing
judgment; that result is explicitly not a hardware timing PASS.
