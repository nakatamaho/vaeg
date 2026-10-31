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

## First follow-up: failed attempt and checks

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

The next step at this checkpoint was to distinguish status-window/poll timing
from transfer/arbitration costs with G1/long-blank controls. The following
second follow-up executes that step without changing the worker or guest.

## G1 and long-blank controls

Repository HEAD for this second campaign is
[dfb03fe7db63bf57c8d11b2c4b2e6518d155d119](https://github.com/nakatamaho/vaeg/commit/dfb03fe7db63bf57c8d11b2c4b2e6518d155d119).
The evaluated source, worker digest and isolated 8 MHz/15 kHz execution
contract above are unchanged. Three new VA2 runs use **V032E, V128A and
V256E**, with no SPACE/S selection. Each script captures the first
`t_g1_vb` entry and 40000 R dispatches, then the next `t_tv_vb` entry (W,
after the program selects 200-line SYNC) and 40000 W dispatches. All three
worker/runner exits are 0. Six new traces contain **18 fully bracketed
windows**; these are not full R/W DONE-screen readouts. The matching three
Q traces are reused, not rerun, giving 27 bracketed windows in this comparison.

| Program | B/chunk | Q chunks in three windows (reused) | R/G1 chunks in three windows | R complete REP costs | W/long-blank chunks in three windows | W complete REP costs |
|---|---:|---|---|---|---|---|
| V032E | 32 | 7, 7, 7 | 13, 13, 13 | 172, 187 | 27, 27, 27 | 359, 372, 385 |
| V128A | 128 | 3, 3, 3 | 5, 5, 5 | 649, 664, 679 | 8, 8, 8 | 1410, 1423, 1436, 1449 |
| V256E | 256 | 2, 2, 2 | 3, 3, 3 | 1285, 1300, 1315 | 4, 4, 4 | 2807, 2820, 2833 |

These cost lists restrict copies to fully bracketed windows. Each W trace
also starts with an unbracketed high interval: E waits for low, whereas A can
accept a remaining tail. Partial intervals and their observed copies remain
in the raw evidence and analyzer output, not silently promoted to complete
windows or removed from the guest's count. Every retained window is IRQ-free,
with IF clear during REP. Entry low-to-high polls are **28 modeled clocks**
apart in all 27 windows; this brackets observation, not the physical VB edge.

W TVRAM costs still obey the earlier `20 * words + 13 * dispatches` equation.
They do not reveal a different long-blank word wait. Different scheduler
budgets can add dispatches, so equal bytes do not imply an identical charged
cost on every entry. R copies are substantially cheaper, but that difference
alone is not a newly measured physical TVRAM limiter.

### A common-cutoff counterexample, with explicit assumptions

A frozen-timeline test asks whether one earlier end cutoff, **keeping the
observed copy/poll costs and rising-edge phase unchanged**, can reduce
256-byte Q to one chunk while preserving the matched G1 control. It must not
be confused with running a modified scheduler or measuring a hardware edge.

- V256E Q's first post-copy high poll is at **2863**. To make it low and
  count only one chunk, the common cutoff can be at most **2863 + 28 = 2891**,
  conservatively allowing the whole entry-poll bracket.
- V128A R's fourth post-copy high poll in its first window is at **2926**.
  It authorizes the fifth chunk. Preserving that five-chunk window requires
  a cutoff **greater than 2926**, even with zero entry-detection latency.
- Those necessary intervals do not overlap: there is a **35-clock gap**.
  The unchanged full-run R=1505 already agrees with hardware; losing R copies
  is not an acceptable correction merely because Q improves.

This rejects the simple common cutoff **in this frozen-cost/phase contract**.
It does not prove that every rescheduled or phase-dependent VB model fails:
changing event placement can change REP budgets and costs, and the hardware
screens provide aggregate counts, not per-window traces. Nor does it prove a
specific arbitration mechanism or authorize TVRAM-only extra waits. Any
proposed mechanism must predict R as well as Q and W, not just fit Q's ratio.

### Ordinary-row controls from actual register returns

One additional V128A/VA2 run executes all A–X measurements with argument 1
and no S/SPACE selection. It captures AX immediately before `measure`'s
source/listing-bound final RET for the 22 ordinary rows, and before the
unique final divide/RET of `t_vb_permille` for S/X. Worker/runner exits are 0;
all **24 intended return snapshots** exist. This is a register-observation
contract, not another screenshot/presenter pass or an inference from exit 0.
The same kit payload is again checked byte-identical after reassembly.

Hardware below is the existing neutral receipt 09, not a new hardware run.
The fresh software run's Q is **902**; the earlier matched DONE screen's
**901** remains unchanged. They are distinct runs with different scripted
entry/capture histories; a one-count difference is not attributed to a cause
without a control. The R/S/W/X values agree with that earlier software row.

| Row | Hardware | Fresh vaeg AX | Row | Hardware | Fresh vaeg AX |
|---|---:|---:|---|---:|---:|
| A | 1841 | 1767 | M | 4281 | 4408 |
| B | 8831 | 8569 | N | 3365 | 3454 |
| C | 1232 | 1215 | O | 3347 | 3454 |
| D | 7676 | 7649 | P | 3364 | 3454 |
| E | 1196 | 1246 | Q | 602 | 902 |
| F | 1253 | 1246 | R | 1505 | 1505 |
| G | 1220 | 1234 | S | 20 | 21 |
| H | 1197 | 1234 | T | 10825 | 10751 |
| I | 3368 | 3454 | U | 3282 | 3363 |
| J | 3359 | 3454 | V | 1311 | 1335 |
| K | 3359 | 3403 | W | 2407 | 2407 |
| L | 3348 | 3398 | X | 86 | 87 |

In particular, ordinary word transfers differ by only a few percent, not Q's
roughly 1.5 factor. A/B are lower in software while most TVRAM word rows are
higher. A single global clock/throughput rescaling cannot be justified as a
Q correction from these controls. These numbers also do not independently
calibrate hardware MHz, per-word waits or bus arbitration.

### Reusable content analysis and verification

[`trace.py`](../../../tools/hardware/tvram_vb/trace.py) now exposes the content
reduction independently of runner, private bindings and Git topology. It
checks record ordering, complete REP/CX/DI/IP progress, IRQ-free copy windows
and poll/copy counts. It preserves external events without invented IRQ
charges, reports partials explicitly and emits machine-readable errors.
It does not certify protected history, evidence-commit scope, hardware timing
or a milestone gate. Summaries and all private bindings stay outside Git.

- `python3 tools/hardware/tvram_vb/test_build.py`: exit 0, **6 tests**,
  including 12 new exact-code content negatives and two new exact-code CLI
  negatives, each starting from a passing fixture with one mutation.
- `ctest --test-dir "$SCRATCH/native" -R '^vaeg_tvram_vb_builder$'
  --output-on-failure`: exit 0, **1/1**; it runs the current Python tests,
  without rebuilding or relabeling the existing native executable.
- The analyzer runs on all nine comparison traces (three reused Q, six new
  R/W), each with 40000 dispatches and three complete windows. Input/output
  and analyzer digests, exact commands, return-snapshot bindings and the
  conservative cutoff calculation are retained locally.
- An initial synthetic test exposed a draft analyzer's premature poll-count
  check on an unbracketed leading interval (`M142_TVVB_TRACE_POLL_COUNT`).
  The failed test log is retained; the analyzer now reports that interval as
  partial and validates poll counts only for fully bracketed windows. This
  is not a failed hardware run or a discarded guest result.

No emulator/COM, TVRAM wait, REP implementation, VB phase/length or VA hold
changes. No new worker build, full suite/media campaign, hosted CI, Windows
execution, merge or gate acceptance. The earlier VA S=20 versus S=86 conflict
and prior-200-line alternative remain unresolved. **#75 OPEN; correction
NOT_IMPLEMENTED; G142 NOT_READY.** No additional hardware work is requested.

## REP alignment accounting

A later source/content reduction reuses exactly these nine Q/R/W traces;
there is no new guest execution or hardware measurement. Across all three
chunk sizes, **1422 REP dispatches / 223 observed copy starts** have the same
residuals after subtracting the modeled per-word cost. Seven of the starts
belong to the previously disclosed leading partial W interval; they are not
promoted into the 216 copies in fully bracketed windows.

| Row | REP IP | Word cost | First-dispatch residual | Continuation residual |
|---|---|---:|---:|---:|
| Q/W TVRAM | `1604h` (even) | 20 | 13 | 13 |
| R GVRAM | `162fh` (odd) | 9 | 13 | 15 |

Thus, for these VA2 traces with `n` words and `d` dispatches, full-copy costs
are `20*n + 13*d` for Q/W and **`9*n + 15*d - 2`** for R. The latter explains
all previously reported R costs: for example, 64 words in six entries cost
`576 + 90 - 2 = 664`. This does not infer a new hardware wait from a ratio.

Source accounting separates three effects:

- `upd9002_rep_movsw()` charges 11 on each entry; `_repe()` charges the prefix
  2 again. Its word loop charges 8; single-plane GVRAM writes add 1, whereas
  VA2 TVRAM words add 12. The GVRAM five-clock write-spacing floor adds no
  extra term in these copies: the ordinary eight-clock body already exceeds
  that spacing. It is not the source of the 11-clock word-cost difference.
- A budget yield rewinds IP. `upd9002_queue_account()` treats that as a
  nonsequential transfer and flushes the modeled queue. At the odd R address,
  the flushed queue initially has one byte. The second byte of `F3 A5` arrives
  four clocks later; prefix accounting supplies two, leaving a two-clock
  fetch stall on every continuation. Initial copies enter with prefetched
  bytes and show no such extra stall. Even-aligned Q/W show no extra stall.
- Different memory costs also exhaust different scheduler budgets and yield
  different dispatch counts. Neither an equal chunk size nor an unchanged
  instruction encoding implies equal total modeled REP overhead.

This explains the **software** R/Q residual difference and strengthens the
need to distinguish scheduler/prefetch behavior from physical arbitration.
It does not identify the hardware limiter, authorize removing prefix/queue
costs, or supersede the frozen-cost/phase cutoff counterexample. An altered
scheduler model needs new execution evidence rather than recalculated old
windows. The returned VA S=20 / historical S=86 conflict remains unresolved.
An exploratory reduction initially expected CX on `begin` records; the trace
provides it on preceding `end` records. That analysis failure is retained and
did not launch or invalidate a guest run. **#75 remains OPEN; no timing-core,
wait, VB or VA-hold correction is implemented.**

## Event-budget and bus-wait exclusion follow-up

After the handoff, the maintainer prioritizes #75 again and defers the Python
BUPDATE replacement. A read-only reduction reuses the same nine source-bound
VA2 Q/R/W traces above. It verifies their digests against the original run
records before parsing them with `tools/hardware/tvram_vb/trace.py`; no guest,
CPU build, hardware capture or modified timing model is run. The local adapter
and its JSON/log are retained under task scratch `tvram-vb-arbitration-r1/`.

The question is narrower than the earlier common-cutoff test: **did an expired
scheduler budget leave a stale VB=1 visible at the particular poll authorizing
the extra Q copy?** `machine/pccore.c` dispatches native instructions while
`CPU_REMCLOCK > 0`, then calls `nevent_progress()`. The trace records remaining
budget at dispatch end. Every observed `IN AL,DX` at `0142h` in these nine traces
costs 10 clocks (instruction 8 plus I/O bus 2). A positive end budget therefore
rules out crossing the currently scheduled next event during that dispatch;
a negative value alone does not identify which event was crossed.

The following table selects the **last high poll in each of the first three
fully bracketed windows**, not every later/partial high interval in the trace.
Offsets remain sums of dispatch costs measured at instruction beginnings,
not physical I/O timestamps. Budget is signed clocks remaining after dispatch.

| Chunk bytes | Row | Last-high offsets, three windows | Remaining budgets, three windows |
|---|---|---|---|
| 32 | Q | 2579 / 2553 / 2566 | 124 / 38 / 65 |
| 32 | R | 2939 / 2939 / 2939 | 48 / 24 / 12 |
| 32 | W | 11183 / 11235 / 11235 | 48 / 118 / 80 |
| 128 | Q | 2942 / 2968 / 2942 | **21 / 20 / 35** |
| 128 | R | 2926 / 2866 / 2881 | 47 / 46 / 73 |
| 128 | W | 10426 / 10426 / 10387 | 57 / 47 / 48 |
| 256 | Q | 2863 / 2876 / 2876 | **125 / -1 / 55** |
| 256 | R | 2715 / 2730 / 2700 | -1 / 39 / 112 |
| 256 | W | 8648 / 8661 / 8648 | 50 / 13 / 2 |

Thus all three 128-byte extra-copy decisions, and two of the three 256-byte
ones, occur with positive budget. **A poll that merely runs past the pending
scheduler deadline cannot explain their extra copies.** The remaining
256-byte Q poll crosses a deadline by one clock, but the trace does not name
that pending event; it is not evidence that VB expiry itself was delayed.
Do not promote this exclusion into a proof that every scheduler phase or the
hardware VB duration is correct.

A second source audit rules out a simple missing-device-wait bookkeeping
claim. `tvramw_wt()` subtracts the model's TVRAM wait from `CPU_REMCLOCK`, and
`upd9002_memorywrite_w()` brackets the mapped VA write and adds that decrement
to `upd9002_bus_waits`. `upd9002_queue_account()` uses that value when holding
the prefetch bus. The read wrapper does the corresponding accounting. The
absence of a direct `upd9002_bus_waits` assignment inside `tvramw_wt()` is not
a missing wait: adding it there would double-account the wrapper's increment.
This establishes the code path, not cycle-exact physical arbitration.

The earlier read-only VALT audit also remains scoped: three retained VA2
state profiles have text control `03h` (VALT zero/disabled). The documented
per-horizontal-period quota cannot be assumed enabled in those software
profiles. Neither those states nor the photographs establish an independently
measured hardware bus-grant schedule. Fixed per-word waits approximate an
average; whether phase-dependent access latency explains Q while preserving
R/W and ordinary transfers still needs evidence. **No fitted wait, speculative
quota, changed blanking duration or VA-hold deletion is introduced. #75 remains
OPEN; correction NOT_IMPLEMENTED; G142 NOT_READY.**
