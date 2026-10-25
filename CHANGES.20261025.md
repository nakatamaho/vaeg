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

# 88VA Eternal Grafx Rel.20261025

Changes since Rel.20261017, through M137. This release focuses on CPU and
SGP timing, the differences between VA and PC-8801 compatibility displays,
movie recording, and everyday save-state and input controls.

<div align="center">
<table align="center" width="100%">
<tr>
<td align="center" width="50%"><img width="100%" src="https://github.com/nakatamaho/vaeg/releases/download/rel-20261025/vaeg-20261025-192336-0000017427-000.png" alt="PC-8801mkIISR demonstration title booted as a VA"><br><em>VA boot: PC-8801mkIISR demonstration, with the VA's blank key blocks</em></td>
<td align="center" width="50%"><img width="100%" src="https://github.com/nakatamaho/vaeg/releases/download/rel-20261025/vaeg-20261025-192522-0000123417-000.png" alt="PC-8801mkIISR demonstration line drawing booted as a VA"><br><em>VA boot: black lines on white</em></td>
</tr>
<tr>
<td align="center" width="50%"><img width="100%" src="https://github.com/nakatamaho/vaeg/releases/download/rel-20261025/vaeg-20261025-203526-0000090924-000.png" alt="PC-8801mkIISR demonstration title booted as a PC-8801mkIISR"><br><em>PC-8801mkIISR boot: the same demonstration, without the blank blocks</em></td>
<td align="center" width="50%"><img width="100%" src="https://github.com/nakatamaho/vaeg/releases/download/rel-20261025/vaeg-20261025-203829-0000274378-000.png" alt="PC-8801mkIISR demonstration line drawing booted as a PC-8801mkIISR"><br><em>PC-8801mkIISR boot: white lines on black</em></td>
</tr>
</table>
</div>

The maintainer confirms that these VA and PC-8801mkIISR display behaviours
match the respective real machines. The pictures are emulator screenshots,
not photographs of the hardware.

## CPU and SGP timing

- Native V30-compatible instructions now use the uPD70116 clock counts in
  place of inherited 80286 counts. The model includes segment and REP
  prefixes, a six-byte prefetch queue, extra bus cycles for odd-address
  words, programmed WCU memory waits, and GVRAM access waits. CPU-loop
  benchmarks are within 0.95-1.09 of the measured VA and VA2 results;
  this is not a claim of cycle accuracy for every program.
- The uPD9002's uPD70008-compatible mode now charges per-instruction costs
  calibrated with Z80TIME 2 on the VA and VA2, rather than assuming that
  every instruction takes the Z80's cycles. Measured classes include loads,
  arithmetic, exchanges, branches, calls, stack operations and block copies.
  Memory waits account for word-wide accesses and instruction fetches.
  Unmeasured classes use a related measured class or the core's cycle count.
- BRKEM and CALLN cost 47 clocks; RETEM costs 37. Compatible-mode timing
  rows in Z80TIME 2 agree within 0.98-1.03 of the hardware measurements;
  the unchanged native memory-reference row is 0.95.
- H/S now selects the V1/V2 compatible-mode instruction rate as well as
  the `IN 6Eh` identification bit, including initial boot and reset on
  both VA models. CPU clock multiplier and CPU speed settings also apply
  to compatible instructions. Native bus waits are not multiplied again,
  and an S selection does not slow native V3 BRKEM code.
- SGP setup, BITBLT and CLS costs are calibrated per model against
  SGPBENCH. Undefined negative source pitches reproduce the observed
  first-line-then-garbled behaviour instead of being treated as valid
  transfers.
- CPU/SGP contention is modelled as a time share while the SGP is busy,
  with separate VA and VA2 values, replacing a fixed cost per SGP access.
  Source-built timing probes are under `demos/z80-timing` and
  `demos/sgp-contention`.

## V1/V2 text and monochrome graphics

- Hiding text no longer clears the attribute colours used by text-merged
  monochrome graphics (PR #56, shitamo). Graphics remain visible when
  software hides the characters through port 53h.
- A VA/VA2 boot retains the text cells' background blocks when characters
  are hidden, in all V1/V2 graphics modes. A PC-8801/8001 model instead
  hides the characters and their backgrounds as that machine does. This
  accounts for the white key blocks on the VA demonstration title above;
  they are not a rendering defect to remove.
- TSP SYNC whole-screen reverse is implemented. VA/VA2 apply it in all
  graphics modes; PC-8801 models ignore it with monochrome graphics, while
  PC-8001 models always apply it. The demonstration's line drawing is
  therefore black on white on a VA and white on black on a PC-8801mkIISR.
- Corrected the VA text-bank/display-source selection in H and S modes.
  CPU bank switching no longer makes unrelated main RAM appear as text
  over graphics, fixing severe display corruption when starting software.
- Compatible-mode instruction operands are fetched through the code
  segment, not the data segment, including prefixed instructions.

## Movie recording and screenshots

- Recording is enabled in the public build configuration on Linux,
  Windows and macOS. The lossless archive is Matroska (`.mkv`), with
  FFV1 video and PCM audio, timed by the guest clock rather than the
  host's recording speed.
- Select Native for the guest picture, or Displayed for the actual
  composed client, including the supported display effects and overlays.
  The selection is remembered across restarts. A recording toggle and
  REC elapsed-time indicator accompany the settings window.
- Native recording splits into new files on supported video-mode,
  geometry, rotation and reset changes. A state load stops the recording
  rather than splicing a discontinuous guest timeline into the archive.
- Optional post-stop MP4 compression runs in the background, preserves
  the original MKV, and shows progress, completion, errors and cancellation.
  Saved MKVs can also be selected for conversion after a restart.
  This is conversion after recording, not realtime H.264 capture.
- Windows uses Media Foundation and macOS uses AVFoundation/VideoToolbox
  for H.264/AAC MP4. No H.264/AAC encoder implementation is bundled.
  Linux public builds provide MKV recording, **not MP4 compression**;
  Linux's separately opted-in local encoders are never part of a release.
- Guest screenshots now follow the selected 90-degree display rotation,
  including the unprocessed/graphics-analysis screenshot path.

See the [recording guide](https://github.com/nakatamaho/vaeg/tree/rel-20261025/docs/recording-v1)
and [MP4 guide](https://github.com/nakatamaho/vaeg/blob/rel-20261025/docs/recording-v1/mp4-export.md).
Platform/backend runtime coverage is described in those guides; enabling
recording does not imply validation on every GPU or display backend.

## Frontend, saves and input

- A separately switchable machine-information bar shows the formal model,
  live mode and configured clocks, with guest-draw FPS and optional FRAME.
  The information menu controls it independently of the front-panel lamps;
  tooltips explain the clocks and show details in narrow windows. Clock
  labels are configured rates, not measured instruction throughput.
- F12 can quick-save to five rotating generations; Shift+F12 quick-loads
  the newest. Quick saves are separate from manual slots, with thumbnails
  and a dedicated menu. The state before a quick load is retained separately
  so loading does not overwrite the quick-save history.
- Recent boots remember the monitor setting and the window size, renderer,
  scaling and rotation in use when a session ends. A missing secondary
  disk or tape no longer prevents restoring the available boot media.
- Fixed second-disk assignment when opening a two-disk archive for FDD2.
  The FDD picker also shows compound `.d88.xz` suffixes correctly.
- The independent YUI/HJK/NM,. letter-keypad switch is available below
  Key > かな入力. Pasted characters are paced for the live S-mode keyboard
  scan so repeated characters are not dropped.
- The controller port is empty (`なし`) by default. Select a joystick or
  mouse explicitly before using it, including cursor-key joystick/mouse
  assignments. The new `ControllerPort` setting replaces `Mouse_VA`, so
  existing configurations also start with an empty port until explicitly
  configured. Cursor-key assignments fall back to ordinary keys when the
  corresponding device is not connected.

## 8087, demos and media tools

- On x86 hosts the optional host-x87 path handles FPTAN, FPATAN, F2XM1,
  FYL2X and FYL2XP1; `NDP8087HostFPU` selects it. Corrected the SoftFloat
  precision-control mapping and the FPATAN/F2XM1 series used by the
  portable path. A BASIC test is supplied in `demos/fpu-test`.
- Utility HDD generation includes the requested Softlib programs and
  `FPUTEST.BAS`/`MANDEL.BAS`. The SASI workflow can build its CP/M images
  from cached archives when images are not supplied.
- Media tools share a checksum-checked download cache, accept cached
  package names regardless of case, and keep CP/M installation scratch
  separate from the cache. CP/M archive extraction handles case-insensitive
  filesystems.

## Remaining limitations

- The compatible-mode forced-slice-exit I/O accounting issue in PR #57
  is **not fixed in this release**. Its integration is reserved for a
  separate follow-up; the M135 timing calibration does not close it.
- Calibration uses specific measured workloads. It does not establish
  exact timings for every instruction, sprite workload or interrupt path.
- ROMs and private system disks are not included. Supply the appropriate
  ROMs and media separately; PC-8801/8001 models still require user-supplied
  ROMs. The screenshots do not add the demonstration program to the package.
