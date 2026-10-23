<!--
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
-->

# M131 — N88 CPU-bank and displayed-text probe

Source-only diagnostic for comparing VA/VA2 text display with CPU TMODE banking.
**Hardware behavior is not established by emulator output.** The maintainer
reports that all four V1S/V1H/V2S/V2H selections are available on VA/VA2; record
both the selected mode and the measured IN 31h value, rather than assuming that
menu labels determine the guest-visible bits.

## Build

Use the repository's cached z80asm 1.8 (see [CP/M tool bootstrap](../../cpmva/README.md)).
No new assembler dependency is downloaded by this builder.

```sh
python3 tools/pc88va/m131-tvram/build.py \
  --assembler "$VAEG_Z80ASM" --output-dir "$PRIVATE/probe-build"
VAEG_Z80ASM="$VAEG_Z80ASM" python3 tools/pc88va/m131-tvram/test_build.py
```

Output must be a new directory outside the repository. It contains:

- `probe.bin`: machine code loaded at B000h;
- `m131tv.bas`: ASCII N88-BASIC program, including license and DATA loader;
- `build.json`: source/binary/loader digests, byte count and loader checksum.

No ROM, BASIC system file, boot sector or disk image is distributed here.
The optional private handoff uses a **copy** of the maintainer-supplied non-Japanese
V1/V2-compatible N88-BASIC system disk. Its D88 header specifies **2HD**, not 2D.
Do not relabel it as 2D or alter the original. No native-2D distribution is
claimed; that would require separate compatible system media and verification.

## Preparation and precautions

Use a disposable system-disk copy, with no valuable files. The BASIC source can
be entered through the emulator's headless input, then saved with
`SAVE "M131TV",A`. Once prepared, write-protect the test disk for hardware runs.
D88 is a container: use a suitable floppy-image writer, not a raw `dd` of its
header and sectors. For a source-only transfer, import the ASCII BASIC file using
an appropriate N88 file-transfer tool. Merely copying it to an unrelated FAT
filesystem does not install it on an N88 disk.

The probe requires ordinary N88 disk BASIC, no resident extensions, and the
standard 80-column, 25-row screen at F3C8h with transparent attributes. It issues
`CLEAR ,&HAFFF`, `SCREEN 1`, `WIDTH 80,25`, `CONSOLE 0,25,0,1` (colour
attributes) and `COLOR 7,0,0`; any existing BASIC
variables/program work state must be considered disposable. It reserves
B000h-DFFFh for code, private stack and two 4-KiB backup buffers. Run only from
BASIC with main RAM selected, not a GVRAM, dictionary or extended-memory window.

The machine routine does no disk I/O, makes no BASIC/ROM calls, and leaves
CRTC/DMA programming unchanged. It disables maskable interrupts while the test
patterns occupy F000h banks. BASIC keys such as STOP are therefore **not** an
abort mechanism during the two display phases. Advance using **SPACE** twice.
Do not reset or power off midway if you want the restoration report. If the
machine hangs, reset/reboot from the disposable disk; no disk operation is in
progress in the routine. This is a diagnostic, not a guarantee against hangs
on an untested machine/configuration.

## Run each case independently

For each of VA and VA2, boot the same prepared disk copy in each requested mode:
V1S, V1H, V2S and V2H. Start from a fresh boot, not an inherited emulator state.

```text
LOAD "M131TV"
RUN
```

1. At **PHASE 0**, photograph the complete display. The CPU has TMODE clear.
2. Press and release **SPACE**. At **PHASE 1**, photograph the display again.
   The CPU now has TMODE set.
3. Press and release **SPACE** again. The routine restores both 4-KiB banks,
   verifies them byte-for-byte, restores the original port 32h, registers,
   stack and interrupt-enable state, then returns to BASIC.
4. Photograph the BASIC result and confirm `RESTORE=1`. If not, stop testing
   that session and reset. Confirm BASIC still answers `PRINT 1+1` with `2`.

The first displayed line identifies **where the CPU wrote the pattern**:

- `A: WRITTEN THROUGH CPU BANK 0 (TMODE CLEAR)`;
- `B: WRITTEN THROUGH CPU BANK 1 (TMODE SET)`.

These labels deliberately do **not** call either bank fast RAM: whether the
banks are distinct, and which one is displayed, are the observations being
measured. The phase label is written to both banks so it remains readable with
either display source. Transient output during pattern setup is not a result;
record only the stable SPACE-waiting phases.

After restoration BASIC reports:

- `PORT31`: entry IN 31h (bit 7: V1 identification; bit 6: H identification);
- `ORIGINAL32`: entry port 32h;
- `CPU P0` / `P1`: first character read by the CPU at F3C8h in each phase;
- `PORT32 P0` / `P1`: port readback in the two phases;
- `RESTORE`: 1 only if both full-bank comparisons pass.

ASCII `41h` is A, `42h` is B. CPU `42h/42h` means these two bank selections
aliased at the tested byte after B was written; `41h/42h` means they were
separate at that byte. Neither observation alone establishes the display source.

## Record observations, then compare policy

| Model | Requested mode | IN31 | Original32 | P0/P1 port32 | CPU P0/P1 | Display P0/P1 | Restore | PRINT |
|---|---|---|---|---|---|---|---|---|
| VA | V1S | | | | | | | |
| VA | V1H | | | | | | | |
| VA | V2S | | | | | | | |
| VA | V2H | | | | | | | |
| VA2 | V1S | | | | | | | |
| VA2 | V1H | | | | | | | |
| VA2 | V2S | | | | | | | |
| VA2 | V2H | | | | | | | |

The X88000-aligned **prediction**, not a hardware result, is A/A displayed with
CPU reads 41h/42h in V1H/V2, and B/B displayed with CPU reads 42h/42h in V1S.
The V2S observation is particularly important: do not decide it from its label
or from agreement with the emulator we just changed. Record unexpected output,
actual IN 31h and return behavior without trying to make it match the prediction.

Keep raw hardware photographs, media and logs outside Git. Publish only an
independently written result summary with neutral fixture identifiers and the
source/build identity. This probe is not the full M131 human gate.
