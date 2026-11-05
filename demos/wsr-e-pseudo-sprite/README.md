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
TASK_SCRATCH="$HOME/work/pc88va/tmp/m143-wsre-w1"
NASM=nasm demos/wsr-e-pseudo-sprite/build.sh "$TASK_SCRATCH/media"
NASM=nasm demos/wsr-e-pseudo-sprite/build.sh --msdos4 "$MSDOS4_SOURCE" "$TASK_SCRATCH/media"
```

Bind `MSDOS4_SOURCE` to the pinned system disk before using the second form.
It also writes `$TASK_SCRATCH/media/wsr-e-pseudo-sprite-msdos4.d88`, a
vanilla copy of the FreeDOS-88VA `msdos4-va.2` disk with the programs in
its root. `BUILD_ID` overrides the build id. Generated files stay out of
Git.

## Smoke run without hardware

```sh
python3 -m venv "$TASK_SCRATCH/venv"
"$TASK_SCRATCH/venv/bin/pip" install --cache-dir "$TASK_SCRATCH/pip-cache" unicorn==2.1.4
"$TASK_SCRATCH/venv/bin/python" demos/wsr-e-pseudo-sprite/smoke.py "$TASK_SCRATCH/media/W1PORT.COM" --float next
"$TASK_SCRATCH/venv/bin/python" demos/wsr-e-pseudo-sprite/smoke.py "$TASK_SCRATCH/media/W1PROBE.COM" --board
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

`tests/wsre/expected/*.csv` are synthetic VA2/V3/15kHz contracts, not
hardware logs. Their CR27, board-read and word-alias values remain `[OPEN]`.
They can be regenerated out of tree with:

```sh
python3 tools/wsre/expected_logs.py "$TASK_SCRATCH/expected"
python3 tools/wsre/test_compare_logs.py
python3 tools/wsre/compare_logs.py tests/wsre/expected/w1probe-on.csv VAEG_LOG --ignore-r0-counts
```

R0 counts in these fixtures are placeholders. Their E8h expectation is still
undriven; it is deliberately not fitted to vaeg's existing V3 Kanji-port
binding. See the [M143 task](../../docs/agents/tasks/M143_wsre_w1_port_probe.md)
for the unresolved local failures and gate status.

## W1SCAN: rotary address-setting probe

The supplied WSR-E setting table defines seven legal settings: 0, 1, 2,
3, 5, 6 and 7; 4 is forbidden. The low even byte is E0h + 2*setting,
with the corresponding odd byte one higher. Setup stays at 46E8h.
For example, the SR index/data pair is 44E0h/45E0h at setting 0 and
44EEh/45EEh at setting 7. The board photo confirms CL-GD5428; the rotary
pointer's setting must not be inferred just from the fixed printed zero.

Build the separate scanner disk (the output directory must be new):

```sh
python3 tools/wsre/build_scan.py --source "$MSDOS4_SOURCE" --output-dir "$TASK_SCRATCH/scan-disk"
"$TASK_SCRATCH/venv/bin/python" tools/wsre/test_scan.py
```

The builder validates the pinned MS-DOS image, boot sector and all original
system/license files, and reads back the exact added payload. It leaves the
original AUTOEXEC unchanged. At the DOS prompt, `SCANHELP` displays usage.
No hardware probe runs automatically. Use V3 on VA/VA2 with VA video and
no other board claiming the candidate ports. Do not use this against
Hyper Frame+. New results append to **WSRSCAN.TXT**, not WSRLOG.TXT.

- `W1SCAN`: read-only data-port survey of all seven settings. This does
  not detect an uninitialized board and is not a board PASS.
- `W1SCAN /P`: tests SR06 at all seven candidates, requiring **12/0F twice**.
  Does not wake the board. Only candidate SR index/data ports are written.
- `W1SCAN /W 7`: explicitly applies the **provisional, GA-98NB-derived**
  wake-up at setting 7, then tests only that candidate. Substitute another
  legal setting only deliberately; this is not automatic write scanning.
  It writes common 46E8h, selected misc/status and SR06 ports. It can change
  clock/polarity/setup. This wake-up is still unverified for WSR-E.

No mode writes relay, GR banks, CRTC or VRAM, changes port 152h or hooks
interrupts. SR06's prior unlocked state and readable index are restored;
otherwise SR06 is locked and an unreadable index cannot be restored.
For `/W`, prior misc/setup reads of FFh are **unknown** and are **not**
written back: setup may remain 08h and misc E3h until reset/power-off.
The log explicitly marks that limitation; this is not full restoration.

Results: `MATCH n` means that candidate passed the repeated SR06 signature,
not complete board validation. `MULTIPLE` means more than one candidate
responded; `NONE_INCONCLUSIVE` cannot distinguish absent/disabled hardware,
a different initialization requirement or another mismatch. Exit codes:
0 read-only completed or one match, 1 multiple matches, 2 no match,
3 argument/target/log error. The program does not measure R0 timing.
The original log comparator does not parse W1SCAN sessions.

The Unicorn controls cover all seven settings, read-only/no-OUT, no board,
constant false signatures, multiple responders, single-candidate wake,
wrong wake address, cleanup, bad arguments/target and log errors. They
are synthetic contracts, not physical-board or timing verification.

## W1INIT: separated enable and DOS detection experiments

W1INIT is a **new** cpu-186 program; W1SCAN and its existing disks remain
unchanged. It uses byte I/O, not 386 instructions, a CPU capability test or
PC-98 BIOS calls. This isolates software prerequisites, but does not prove
that a VA slot supplies every bus condition required by WSR-E.

```sh
python3 tools/wsre/build_scan.py --probe init --source "$MSDOS4_SOURCE" --output-dir "$TASK_SCRATCH/init-disk"
"$TASK_SCRATCH/venv/bin/python" tools/wsre/test_init.py
```

The separate image is `wsr-e-init-probe-msdos4.d88`; `INITHELP` shows usage.
Original system files/AUTOEXEC are preserved, and **no probe auto-runs**.
The added files are W1INIT.COM, INITHELP.TXT/BAT and INITLIC.TXT. Results
append to **WSRINIT.TXT**, separate from both earlier probe logs.

- `W1INIT`: read-only SR-data survey of all seven legal candidates.
- `W1INIT /E 7`: selected-address enable experiment. It reads CR27 before
  wake and, if FF, uses the Windows-reference sequence, including **01h
  twice to 43EEh**, E3h misc and 00h status. A non-FF pre-read skips wake,
  as in the inspected local driver routine; this is not proof of identity.
  It then reads CR27 again and observes SR06 separately. No board/relay
  control write is made by `/E`. Another legal setting can replace 7.
- `W1INIT /D 7`: **writes board/relay control** at the selected address,
  reading its prior value, writing 7Ah, and requiring a 7Ah readback before
  proceeding. On acceptance it records pre-CR27, uses the DOS-reference
  setup/misc sequence ending in EFh, records post-CR27, then observes SR06.
  It does not insert the `/E` enable/status writes into the DOS sequence.
- `W1INIT /D ALL`: explicitly opt into that DOS-style address enumeration
  for 0/1/2/3/5/6/7. Each candidate is cleaned up as far as possible before
  the next; setting 4 is never selected. A rejected handshake skips VGA
  wake and indexed-register writes for that candidate.

**Use `/D` only with known exclusive ownership of all requested candidate
ports. It can switch video. Do not use it for Hyper Frame+, other boards,
or PC-98 machines.** Prefer `/E 7` first with VA video and unchanged rotary
position. Compare `/E` and `/D` in separate power-on sessions; their residual
state can otherwise affect each other. A single candidate may also be
specified for `/D` instead of enumerating all. Do not force a board into a
slot or change switches while powered.

`CHIP_CANDIDATE n` means post-CR27 masked by FC is 98, **not** a complete
board PASS. SR06's repeated 12/0F observation is independent, not the gate
for reaching CR27 or accepting that candidate. A constant CR27 response
alone is not proof of a working chip. `MULTIPLE` requires alias/conflict
investigation; `NONE_INCONCLUSIVE` is not evidence of a faulty/absent board
or CPU-based rejection. The log retains the gate, pre/post ID and SR data
for diagnosis. Exits: 0 read-only completed or one chip candidate, 1 multiple,
2 none, 3 argument/target/log error.

Cleanup is **partial**, not a state-save guarantee. SR06 is restored to
prior unlocked state when known, otherwise locked; readable SR/CR indices
are restored only within the interpreted ranges. Saved setup/misc FF values
are never written back. `/E` restores enable only for prior 00h/01h and does
not restore its status write; `/D` restores control only for prior 7Ah/7Bh.
Otherwise enable may remain 01h, control may remain 7Ah, and setup/misc may
remain changed. All restoration messages are **attempts based on prior
reads**, not verified post-restoration readbacks. No VRAM, GR-bank, CRTC-data,
word-I/O, timing or IRQ-hook test is performed; misc can change clock/polarity.

The control suite checks exact separate I/O sequences, all seven settings,
no implicit relay writes, rejected gates, CR27/SR06 independence, fake
partial/multiple/absent responses, interpretable versus unknown cleanup,
every log-write failure point, argument/target guards and the executed
instruction/DOS-interrupt subset. The fake wake requirements are deliberate
fixtures, **not an emulator or hardware specification**.
