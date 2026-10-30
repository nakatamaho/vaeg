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

# M142 — TVRAM VB instruction-cost and SYNC follow-up

Issue [#75](https://github.com/nakatamaho/vaeg/issues/75) remains **OPEN**;
correction **NOT_IMPLEMENTED**, G142 **NOT_READY**. This follows the
[ten returned DONE screens and matched software results](m142_tvram_vb_hardware.md).
No additional hardware run or photograph is requested. The work below
identifies software-side thresholds and an alternative SYNC condition, not
the physical limiter or a justified timing correction.

## Evaluated worker and execution contract

- Evaluated executable source:
  [30bceba24c102eb3ee60bdfcf90e0e4b93836b3c](https://github.com/nakatamaho/vaeg/commit/30bceba24c102eb3ee60bdfcf90e0e4b93836b3c).
- Worker SHA-256:
  `8ee71b64e92175b4e965902b4f29bb7112d3318a949ec6f997524a57ca7358be`.
  It is the unchanged worker used for the earlier ten software observations.
  Repository HEAD during these runs was
  `1a69d88f52c17189083938324801ed525ba20ba8`; its changes after the evaluated
  executable source are documentation only.
- Native macOS, SDL dummy video/audio, nominal standard 8 MHz, 15 kHz,
  `clk_mult=2`, `CPU_Speed=100`, `Monitor15kHz=true`, `DIPswtch=3e 73 7b`,
  `sgp_mode=0`, `sgp_mult=1`, `ExMemory=0`. Recording ON, local encoders OFF,
  shaders OFF. No Windows, physical presenter or absolute-time acceptance.
- The final diagnostic disk is copied into each isolated case; HOME/config
  are isolated and every other FDD/SASI/SCSI slot is explicitly empty. The
  COMs are unmodified. NASM listings were generated out of tree and each
  reassembled COM checked byte-for-byte against the existing kit payload.
  The demo itself relocates to segment `2000h`; no relocation wrapper or
  patched guest/emulator was needed.
- Existing `--debug-script` / `--debug-output-dir` facilities collect bounded
  `upd9002-trace-v1` traces and register snapshots. Six Q runs and three SYNC/S
  control runs complete their intended captures, worker and runner exits 0.
  They produce thirteen traces: six Q and three S traces of 40000 dispatches
  each, plus four SYNC traces of 5000 dispatches each. These are diagnostic
  executions, **not nine additional DONE-screen contracts or gate passes**.

The launch flags are the earlier [software command contract](m142_tvram_vb_hardware.md#fresh-unchanged-worker-software-comparison),
with `--headless-input-script` / scheduled screenshot replaced by:

```sh
--debug-script "$CASE/debug.txt" --debug-output-dir "$CASE/capture"
```

Q scripts type the matching `<PROGRAM> 1` at frame 180, wait for the first
`2000:t_tv_vb` entry, capture registers, trace 40000 dispatches and exit.
Variants receive no SPACE. Stock advances four screens at frames 600, 800,
1000 and 1200. Debug `input-line . .` preserves the internal SPACE; dots and
Return have no benchmark action. Each Q trace contains three fully bracketed
low-to-high-to-low polling windows, not a complete 300-VRTC measurement.

Control scripts trace the first `send_sync`, then the first
`t_vb_permille`, and capture AX immediately before that function's final RET
for both S and X. In the selected-SYNC stock control, S is pressed on screen
1 at frame 600, its `send_sync` is traced, and the four SPACE advances are at
800, 1000, 1200 and 1400. Instruction addresses come from the byte-identical
assembly listings; the return site is independently bound to its unique
`XOR DX,DX; MOV BX,60; DIV BX; RET` encoding.

Exact commands, source/program/worker/ROM/disk/runner/reducer identities,
registers, traces, output hashes and reduction are retained outside Git.
The one-off reduction schema is `m142-tvram-vb-instruction-cost-reduction-v1`.
No private captures, traces, filenames, paths or asset hashes are published.

## Q: measured software thresholds, not a continuous 1.45 ratio

The following values are **model-reported instruction costs**, not physical
CPU measurements. Relative coordinates sum dispatched instruction costs and
place a poll at the beginning of its IN instruction; they do not locate an
exact bus edge. IRQ entry can occur outside trace begin/end records, so no
invented IRQ-entry cost is added. Each retained copy window is checked to
contain no IRQ; mapping disables interrupts before these copies.

| Model | Program | B/chunk | Complete REP costs observed | Chunks in the three sampled windows | First window: last high post-copy poll / first low poll, relative to first detected high |
|---|---|---:|---|---|---|
| VA2 | TSPSPR | 128 | 1410, 1423, 1436 | 3, 3, 3 | 2942 / 4444 |
| VA2 | V032A | 32 | 359, 372 | 7, 8, 7 | 2579 / 3004 |
| VA2 | V032E | 32 | 359, 372 | 7, 7, 7 | 2579 / 3004 |
| VA2 | V128A | 128 | 1410, 1423 | 3, 3, 3 | 2942 / 4431 |
| VA2 | V256E | 256 | 2807, 2820, 2833 | 2, 2, 2 | 2863 / 5762 |
| VA | V128A | 128 | 1474, 1487, 1500 | 8, 8, 8 | 10861 / 12414 |

The 128-byte second post-copy poll remains high at offsets **2942–2968**
in these samples, authorizing a third whole copy. The 256-byte first
post-copy poll remains high at **2863–2876**, authorizing a second whole
copy. The final copies end well after those polls. This directly confirms
the software counting mechanism: `vb_chunks` is incremented before polling
VB again; an entire counted chunk may extend beyond VB.

For reference, applying unchanged `tsp_updateclock()` to the traced 224-line
vector at this configuration gives 267 lines/frame, 888 pixel clocks/line,
integer line rate 15979, frame cost 133460, and nominal status-VB scheduling
cost **2999**. The current VA hold uses `floor(133460 / 267) * 23 = 11477`.
These are **emulator calculations**, not measured hardware periods. Poll
latency, event dispatch and instruction quantization prevent treating the
nominal 2999 as an independently observed edge in the table.

The extra 128/256-byte copies are authorized within roughly 140 modeled
clocks of that nominal short-window length. This replaces the earlier toy
arithmetic with a bound from actual unchanged-worker execution: a large
chunk-count ratio need not mean a comparably large continuous transfer-rate
error. It does **not** choose a shorter VB, slower copy or shifted edge as
the correction. In particular, do not change six programmed blanking lines
to five or fit a fractional line from the Q counts.

The 32-byte A sample also crosses a threshold (7, 8, 7 versus E's 7, 7, 7).
Three windows do not replace the existing full-run 2124/2104 readouts or
establish exhaustive A/E phase independence. Both stock and V128A start with
three chunks in these samples; that does not explain or overwrite their
different full-run 875/901 totals.

### REP redispatch is part of the measured emulator cost

The complete-copy costs obey, in all six traces:

```text
cost = words * (8 + model_tvram_word_wait) + 13 * REP_dispatch_count
model_tvram_word_wait = 12 (VA2), 13 (VA)
```

For example, VA2 64-word copies take ten or eleven dispatches, giving
`1280 + 130 = 1410` or `1280 + 143 = 1423`. A 16-word copy takes three or
four dispatches, giving 359 or 372. These equations describe the current
implementation, not a newly measured hardware cost.

[`upd9002_rp.c`](../../../cpu/upd9002/upd9002_rp.c) charges startup 11 on each
entry, yields when its remaining clock budget expires, and rewinds IP to
redispatch REP. [`upd9002_mn.c`](../../../cpu/upd9002/upd9002_mn.c) charges the
prefix's two clocks again. Thus a copy's charged cost depends on how many
scheduler boundaries it crosses, not just its byte count. This is a concrete
software confounder to preserve in analysis, **not proof of #75's physical
cause or an independently verified CPU-timing defect**. Merely removing
those repeated charges would speed up the copies, not supply evidence for
the required hardware limiter. No REP, scheduler or TVRAM wait was changed.

## SYNC: an alternative condition for the earlier VA S=86

The traces record the actual program-generated SYNC command/parameter writes,
not a label inferred from a filename:

```text
224-line: c1 57 1c 00 9f 00 10 0f 25 00 e0 00 02 04
200-line: c1 57 1c 00 9f 00 10 0f 25 00 c8 00 0f 08
```

| Software control | First programmed SYNC | Selection before screen 5 | S return AX | X return AX |
|---|---|---|---:|---:|
| VA2 V128A | 224-line | None | 21 | 87 |
| VA V128A | 224-line | None | 85 | 87 |
| VA2 stock TSPSPR | 224-line | Press S; traced switch to 200-line | 87 | 87 |

The VA and VA2 unselected controls send **identical 224-line parameters**,
yet differ in S because the current emulator applies the VA-specific hold.
This confirms the software-model difference; it does not discover a hardware
SYNC vector from a photograph.

The selected control reaches approximately 8.6% for **both S and X without
any VA-specific hold**, on the VA2 model. Source inspection explains why:
`frame()` permits S on the ordinary screens, `sync200` persists into
`tvram_screen`, and the first-phase rows use the already selected SYNC.
The second phase switches only if `sync200` was zero. DONE retains neither
that initial selection nor the preceding key history.

Consequently, an earlier first-phase result near 86 can arise from a prior
200-line selection; it does not by itself establish that VA VB ignores SYNC.
This is a demonstrated **alternative software condition**, not evidence that
S was actually pressed in the historical VA run. The software result is
87/87, not an exact reproduction of every historical hardware value. Retain
the historical VA S=86 and returned VA S=20/X=86 as separate corpora; do not
retroactively relabel their unknown run conditions or declare the conflict
resolved. The generic TSP status description associates VB with BBR/BBL/VS,
but that alone does not settle VA board behavior.

S itself is not a duration measurement: its code divides high-poll count by
all-poll count, does arithmetic between sampling intervals, rounds each
frame to an integer permille and then averages. In the VA2 trace, the first
two completed divisions use **100/4449 → 22** and **97/4446 → 21**; the
60-frame function returns 21. Neither S=21 nor hardware S=20 specifies an
exact edge, clock frequency or wait-state correction.

## Failed attempt, checks and next decision

One earlier selected-SYNC control ends at frame limit 12000 with worker exit
0 but **missing the S trace/result**; its runner exits 1 with
`AssertionError: TRACE_NOT_CREATED`. Its `input-line  .` lost the leading
SPACE in `debug_remainder()`, so the four screen advances never occurred.
The corrected case uses internal SPACE (`input-line . .`) and reaches both
return snapshots. The incomplete run is retained as incomplete, not counted
among the nine successful captures and not diagnosed as a stale disk or a
hardware interaction problem. An initial ad-hoc reduction also failed on an
IRQ event outside begin/end; the final reduction handles and retains those
events without inventing elapsed IRQ clocks.

All nine successful runs exit 0; trace/listing/payload identities and the
numerical tables are checked locally. Repository encoding/EOL/case/Markdown,
milestone selftest/audit and whitespace checks exit 0. No new CTest suite,
GUI/media regression, binary build, hosted CI, Windows execution, merge or
human acceptance is claimed. The unchanged Windows handoff remains valid
for its previously stated scope; this report does not relabel its source.

Next, distinguish status-window/poll timing from transfer/arbitration costs
using the established threshold locations and G1/long-blank controls. The
prior-200-line-selection alternative must be retained when evaluating the
VA hold, but is not permission to delete it speculatively. Until a mechanism
predicts the existing hardware controls without fitting Q, leave #75's
cause OPEN and correction NOT_IMPLEMENTED.
