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

# M143 — WSR-E stage W1: the VA side of the port path and the board probe

Status: **IN_PROGRESS; G143 NOT_READY.** Assigned explicitly as M143 WSR-E
W1. Base: `origin/main` at `950efef393196497ed2a9f1c017a3f0ae30f5323`.
The separate runtime-speed branch is not included and its human acceptance
does not accept this WSR-E task. W2 remains out of scope.

The first commit preserves all delivered drafts unchanged:
[2ad997fc421a05d1f2b3cb259d762fbd49665a40](https://github.com/nakatamaho/vaeg/commit/2ad997fc421a05d1f2b3cb259d762fbd49665a40).

Branch `milestone/m143-wsre-w1`; commit prefix `M143:`.

Plan: [`docs/modernization/wsr-e-pseudo-sprite-plan.md`](../../modernization/wsr-e-pseudo-sprite-plan.md),
stage W1. Read sections 5, 7, 8, 9 and 11 before starting.

## Request

The maintainer (2026-11-02): extend `demos/sgp-pseudo-sprite/65536` to the
Melco WSR-E (CL-GD5428) on the PC-88VA; collect test programs in
`demos/wsr-e-pseudo-sprite`; show machine, FPS and ball count as the other
demos do; MS-DOS 4.0; split into milestones; record timing benchmarks
exactly; implement the board in vaeg in parallel.

W1 answers two questions before any VRAM or BLT work:

1. Does the VA drive the low bytes of the WSR-E ports (E0h, E1h, E8h) in
   V3 mode, and does port 152h read back what is written?
2. Is a GD542x answering at 4nE0h through the VA slot, and do word OUTs
   reach it as index + data?

## Known before the stage

- `[DOC]` VA I/O 1000h-FEFFh is the user area for expansion boards; E0h
  and E1h are unused; E2h-E9h, ECh, EDh are V1/V2 only (Technical Manual
  2.2).
- `[DOC]` Port 152h bit 3 = 1 releases E0000-EFFFFh to the slot (3.3);
  the ROM reads 152h as a word.
- `[OTHER-BOARD]` AGS_CLGD.COM (GA-98NB, same port map): wake-up
  46E8h=18h, 42E0h=01h, 46E8h=08h, 42E0h=E3h, 5AE0h=00h; SR06 probe by
  `OUT 44E0h,AX` (1206h) and `IN 45E0h` = 12h; word OUTs to 44E0h, 4EE0h,
  54E0h; VRAM at E000h through GR09/GR0A.
- `[MEASURED]` (getromva comparison, 2026-11-01) an undriven VA address
  reads back code bytes near the reading instruction (`AA 0E`).
- `[CODE]` vaeg's iocore decodes all 16 address bits and returns FFh for an
  unattached port; `iocore_out16` writes port and port+1 as two bytes.
- `[CODE]` vaeg's port 152h (`io/memctrlva.c`) keeps `rom0_bank`; banks
  8-9 come from `varom08.rom` (all FFh: an empty slot).

## Scope

### A. Programs (`demos/wsr-e-pseudo-sprite/`)

Already drafted (assemble, and pass `smoke.py` in every port model):
`build.sh`, `wsre.inc`, `tests/w1port.asm`, `tests/w1probe.asm`,
`smoke.py`, `README.md`. Review them against the plan, then:

1. Keep the CSV format of plan section 7 exactly; it is the interface
   between the machines, vaeg and the host scripts.
2. Add `tools/wsre/compare_logs.py` (or a place the maintainer prefers):
   reads two `WSRLOG.TXT` files and prints, per row, the machine value,
   the vaeg value and the ratio for timing rows; for check rows, equal or
   not, with `FF` and `FLOAT ...` treated as the same class.
3. Add the programs to the MS-DOS 4.0 all-demos disk only if the
   maintainer asks; otherwise `build.sh --msdos4` makes a separate disk.

### B. vaeg

1. Setting: `np2cfg.wsreenable` (default 0), INI key `WSRE`, as
   `mpuenable` in `machine/pccore.*` and `sdl2/ini.c`. A GUI switch only
   if the maintainer asks.
2. `cbus/wsre.c`, `cbus/wsre.h`, registered in `cbus/cbuscore.c`
   (reset/bind only when enabled):
   - ports 40E0h + n*100h (n = 0..F) as VGA 3C0h + n; 54E0h/55E0h CRTC;
     5AE0h status; 46E8h setup/enable (keep the value, readable);
   - board ports 40E1h, 42E1h, 51E1h, 59E1h, 5BE1h: keep written values;
     reads return what W1PROBE photographs show (until then FFh, marked
     `[OPEN]` in the code);
   - word writes: because `iocore_out16` splits them, a write to 44E1h,
     4EE1h or 54E1h is the data of the paired index register (45E0h,
     4FE0h, 55E0h). If W1PROBE shows the machine behaves otherwise,
     follow the machine;
   - nothing in the memory map yet (W2).
3. `vram/gd5428.c`, `vram/gd5428.h`: register files SR 00-1F, GR 00-3F,
   CRTC 00-3F, attribute 00-14 (with the index/data flip-flop), DAC
   index/data, pixel mask; SR06 reads 12h after 12h is written and 0Fh
   otherwise; CR27 = the value W1PROBE photographs show (98h is assumed
   for the 5428 until then, `[OPEN]`); extended registers read as written.
   No display, no BLT.
4. State: a new section `WSRE` saved only when the board is enabled; a
   state without it loads with the board absent. Nothing changes when the
   board is disabled.
5. Tests: unit tests for SR06, the word split, GR09/GR0A read-back, CR27;
   a headless run of W1PORT and W1PROBE (board off and on) whose logs are
   compared with checked-in expected logs, ignoring the build id and R0
   counts.

Out of scope: the E window (W2), BLT (W3), timing of board I/O (W4),
display and relay (W5).

## Expected results

| Row | VA | VA2 | vaeg |
| --- | --- | --- | --- |
| W1PORT P 00E0, 00E1, 00E8 (no board) | [OPEN] FF or FLOAT expected | [OPEN] FF or FLOAT expected | FF required; E8h finding below |
| W1PORT B152 SAME, SLOT, RESTORE | [OPEN] PASS expected | [OPEN] PASS expected | PASS |
| W1PORT R0 counts | [OPEN] logs needed | [OPEN] logs needed | 0.9-1.1 of the machine (M130 calibrated) |
| W1PROBE SR06B 12 / 00 | [OPEN] 12 / 0F expected | [OPEN] 12 / 0F expected | 12 / 0F |
| W1PROBE SR06W 12 / 00 | [OPEN] | [OPEN] | provisional index/data alias; await logs |
| W1PROBE CR27 | [OPEN] | [OPEN] | [OPEN] 98h provisional |
| W1PROBE GR09B/GR0AB, GR09W/GR0AW | [OPEN] | [OPEN] | provisional read-back; await logs |
| W1PROBE P 40E1h ... 5BE1h | [OPEN] | [OPEN] | [OPEN] FFh until hardware logs |

Fill the VA and VA2 columns from the logs; a row that differs from the
expectation is a finding, not a failure of the task: record it here and
in the plan's section 9, then make vaeg follow it.

## Gate G143 (human)

Standard V3 gate, plus:

- W1PORT without the board, W1PORT with the board, and W1PROBE, on the
  PC-88VA and the PC-88VA2: photographs showing machine, build id and
  display rate, and `WSRLOG.TXT` from the disk;
- vaeg with the board off runs W1PORT and W1PROBE (exit 0 and exit 2);
  with the board on, both logs match the machines' logs row by row
  (`compare_logs.py`), R0 within 0.9-1.1;
- vaeg with the board off behaves as before M143: demos and media
  regression of `AGENTS.md`, state files of M142 still load.

## Procedure on the machines (for the maintainer)

1. Build: `NASM=nasm demos/wsr-e-pseudo-sprite/build.sh --msdos4
   msdos4-pc88va-2hd.d88 OUTDIR`, write the D88 to a floppy.
2. Boot it in V3 mode (the log is written to the current drive; the disk
   must not be write-protected).
3. Without the board: `W1PORT`; photograph.
4. With the board: `W1PORT`, then `W1PROBE`; photograph each.
5. Return `WSRLOG.TXT` (or the whole disk image).

W1PORT writes port 152h with interrupts off for a few instructions. W1PROBE
writes 46E8h, 42E0h, 5AE0h, SR06, GR09 and GR0A, never 40E1h, so the
monitor stays on the VA.

## Local verification

Evaluated application/program source:
[26b60fcee5b74261b373951bd5b8b3fc60713c18](https://github.com/nakatamaho/vaeg/commit/26b60fcee5b74261b373951bd5b8b3fc60713c18).
Earlier development results remain in task scratch and Git history; they are
not relabeled as this candidate's results. No hosted-CI result is claimed.

### Builds and control tests

- `NASM=nasm demos/wsr-e-pseudo-sprite/build.sh --msdos4 "$MSDOS4_SOURCE"
  "$TASK_SCRATCH/media-26b60fce"`: exit **0**, NASM **3.01**, pinned
  FreeDOS-88VA `msdos4-va.2` input. Both sources retain `cpu 186`.
- `"$SMOKE_PYTHON" demos/wsr-e-pseudo-sprite/smoke.py "$COM" [MODEL]
  --log "$LOG"`: Unicorn **2.1.4** in an isolated venv. Default,
  `--float self`, `--float next`, `--board` return **0/0/0/0** for W1PORT
  and **2/2/2/0** for W1PROBE. All eight final runs meet the required
  statuses and CSV/cleanup controls. These are synthetic control-flow
  results, not timing or hardware measurements.
- `python3 tools/wsre/test_compare_logs.py`: exit **0**, **8 tests**,
  including generated-versus-checked expected logs and exact-code negatives.
- Native Darwin arm64 Debug: recording ON, isolated static FFV1/PCM SDK,
  bundled local MP4 encoders OFF, librashader OFF, integration tracing ON.
  `cmake --build "$NATIVE_BUILD" --parallel 4`: exit **0**.
  `ctest --test-dir "$NATIVE_BUILD" --output-on-failure -j4`: exit **0**,
  **176 registered: 160 PASS / 16 SKIP / 0 FAIL**, 215.21 seconds.
  Native worker SHA-256:
  `bfa629a5b1eceac5140abf510db56a5c1db4ff0b6cc5d6ecc7e186b55b4b0b85`.
  CTest log SHA-256:
  `f1c6a70ba1759af596ba3277b7911d2ec76206a6cffc5ab5196e5883c8ec08e9`.
- The first full run had one canonical-trace failure from the new fixture's
  extra I/O. [63b796aa](https://github.com/nakatamaho/vaeg/commit/63b796aa74a0531f20719d5540aa3f9400d3ae24)
  isolates it in `vaeg_wsre_w1`; no golden trace was changed. Focused tests
  passed 3/3, and the final full run above includes the correction.
- Repository checks, all exit **0**: `check_encoding.py --expect utf8`
  (0 violations), `check_eol.py`, `check_case.py` (0 findings),
  `check_md_comments.py`, `git diff --check`; milestone-ID selftest/audit
  **49** checks (224 tasks, 177 reports, 165 roadmap rows); release identity
  **5** tests; MacPorts helper **3** tests; recording-release policy **PASS**.
  `clang-format-mp-19 --dry-run --Werror` passes for the new board/chip
  sources/headers and W1 selftest header. This is not a claim that unrelated
  legacy formatting debt was reformatted or eliminated.

### Four actual guest cases

Each case boots a copied MS-DOS image on the VA2/V3/15kHz target with SDL
dummy video/audio, `WSRE=false/true`, and a batch file recording the COM's
exit status. Frame budget: **50,000**. The worker exits **0** in all four
cases; that is not the guest result. Read-back logs are compared using:

`python3 tools/wsre/compare_logs.py tests/wsre/expected/CASE.csv "$GUEST_LOG"
--ignore-r0-counts`

| Case | Guest result | Comparator exit | Local result |
| --- | --- | --- | --- |
| W1PORT, board off | 1, SUM FAIL 1 (E8h) | 1 | **FAIL** |
| W1PROBE, board off | 2, NO-BOARD | 0 | **PASS checks only** |
| W1PORT, board on | no exit file; unclosed zero-length log | NOT_RUN | **FAIL: incomplete** |
| W1PROBE, board on | 0, SUM PASS | 0 | **PASS checks only** |

R0 timing is explicitly ignored here; none of these comparisons is a
hardware timing PASS. The expected logs are independently generated from
the provisional register contract, not copied from captures. E8h remains
FF in the fixture despite the observed failure. Commands, configs, worker
identity, logs, raw disks and screenshots remain in task scratch. These
are not mapped-presenter or approved-reference results.

### Checkpoint artifacts (not a ready G143 handoff)

| Artifact | SHA-256 |
| --- | --- |
| W1PORT.COM | `7750898ea96238c1292b75946316d4b84eae46fe60ff6bd1374c0491ecea94d0` |
| W1PROBE.COM | `fec0b8d3939799aa09321fe63662957cc275482cdfb5d632f56629fb5f2d4880` |
| Separate MS-DOS D88 | `fe3d7149e0a7541b5b187fea57e3b87966e725f56f2e21b2c5c40a7fc7a1d33e` |
| Static Windows executable | `011237eeccda78218b108ec331c1a34e657a233fd72b0a47b2c9fc1dc3e12595` |

`python3 tools/release/build-mingw-macports.py --commit
26b60fcee5b74261b373951bd5b8b3fc60713c18 --work-dir "$MINGW_BUILD"
--sdk-dir "$REVIEWED_SDK" --build-track m143 --jobs 2`: exit **0**, new
clean clone, no reused application objects or container. Recording/native
Windows MP4/librashader ON; bundled encoders OFF; encoder/archive and
Windows system-DLL import audits PASS. Display:
`Rel.20261102 (m143, 26b60fce)`. Windows execution is **NOT_RUN**.

Verified copies (all compared with `cmp`):

- `/Users/maho/work/pc88va/release-handoff/m143-wsre-w1-26b60fce/vaeg.exe`
- `/Users/maho/work/pc88va/release-handoff/m143-wsre-w1-26b60fce/BUILD-REPORT.md`
- COMs and D88 in the same directory's `probes/` subdirectory.

This is a retained implementation checkpoint, not release authorization,
software-gate completion or a request for human acceptance.

## Development findings and remaining local work

- `compare_logs.py` compares row/session identities, metadata apart from
  build id, undriven classes and timing ratios; eight focused tests pass,
  including passing controls and exact-code mutation negatives. Its explicit
  R0-ignore mode is only for emulator fixtures, not a timing PASS.
- Program review found missing restoration of 46E8h and incoming GR09/GR0A.
  W1PROBE now restores them and locks SR06 on exit without touching 40E1h.
  Logging errors no longer silently return success, and a denied log open
  cannot truncate an existing file. CSV column order and CRLF are unchanged.
- Register-only board implementation and optional WSRE v1 save section are
  present. Existing global state version and PCCORE payload are unchanged.
  Reader reset preferences are preserved; an absent section restores an
  absent live board. Focused ROM-less state/port checks pass, including
  four single-mutation wire guards with exact `M143_WSRE_STATE` codes.
- **Blocking contract conflict:** a native V3/MS-DOS W1PORT run with WSRE
  disabled exits **1**, not the required 0. The log reports `00E8h` as
  `DRIVEN 00`, with `SUM ... FAIL 1`; `00E0h`/`00E1h` are `FF` and the
  152h checks pass. W1PROBE without the board exits **2** as required.
  `cgromva_bind()` currently binds the E8h/E9h compatible Kanji handlers
  unconditionally; their reads have no V3-mode guard. This source is
  unchanged from origin/main. The new board's isolation test therefore
  compares E8h with its pre-enable baseline rather than inventing FFh.
  The actual W1PORT failure remains a failure. Do not change the expected
  probe contract, suppress this row, or silently change disabled-board
  behavior to hide the conflict. Resolving it needs a scope decision on a
  V3-compatible-port correction versus the explicit unchanged-disabled
  requirement, followed by baseline/regression checks.
- **Board-on W1PORT remains incomplete:** after 50,000 frames, the screen
  ends partway through the 44E0h result and no guest exit status is written.
  This repeats the shorter development observation. Cause is **OPEN**;
  do not infer that the E8h correction would fix it. W1PROBE completes and
  matches the provisional fixture with the board on.
- **Actual M142-produced SAV compatibility is not proven.** A clean,
  unchanged writer built from main `950efef3` writes a snapshot at a
  synthetic guest checkpoint. Both the current reader and the unchanged
  writer's self-load control fail with `M142_STATE_RESTORE` (reader frame
  10, repeated at frame 3000). Because the control itself fails, this is
  not a qualified passing fixture and does not establish an M143
  regression. The attempted witness and controls are retained privately;
  root cause/fixture qualification and a successful real-state exchange
  remain required. Current-build optional-section round-trip tests are
  passing but do not substitute for this check.
- Full demo/media regression is **NOT_RUN**; the separate W1 disk build
  and full CTest campaign do not cover it. Hardware logs, R0 ratios and
  the human gate remain pending. No merge, main push, cleanup or W2 work
  is authorized by these local results.
- The initial native configure correctly refused tests without integration
  tracing; the subsequent tracing-enabled configure/build succeeded with
  an isolated recording SDK and local bundled MP4 encoders OFF.

## Address scanner follow-up

The maintainer requested importing accepted M144 from main and a separate
rotary-address probe disk. Main was merged into this branch at
`dcf69959` (M143 is not merged to main). The accepted M144 port-70h reset
origin and measurement record are retained; E8h is **not** gated off in V3.
The earlier E8h scope-conflict text above is historical and superseded by
actual VA W1PORT results and M144: E8h=00h also occurs on hardware.

Returned VA observations (raw disks/photos stay private): the no-board
15kHz run gives R0 4770/4770/4770; the later reportedly seated-board 24kHz
run gives 5196/5196/5196. Different display rates prohibit treating the
count difference as a board speed effect. Both have E8h=00, expansion probe
reads FF and passing 152h restore checks. The later W1PROBE returns FF
for both SR06 values and NO-BOARD. This is not proof of absence or failure.
Photos identify CL-GD5428-80QC-A and a rotary address selector. The supplied
setting table defines 0/1/2/3/5/6/7, with low even byte E0+2*n and common
46E8h; setting 4 is forbidden. Do not infer the selected position from the
fixed printed zero: the rotary pointer interpretation is unresolved.

`tests/w1scan.asm` adds W1SCAN (cpu 186) with separate WSRSCAN.TXT output:
read-only default, `/P` repeated SR06 signatures at all seven settings,
and `/W n` explicit provisional wake at just one setting. No relay, VRAM,
GR-bank, CRTC, 152h or interrupt-vector writes. Unknown prior misc/setup
reads are not restored as FF; wake can leave setup/misc enabled. The
[scanner instructions](../../../demos/wsr-e-pseudo-sprite/README.md#w1scan-rotary-address-setting-probe)
spell out this limitation and the four exit statuses. A missing response
remains inconclusive, and the GA-98NB-derived wake is still unverified for
WSR-E. No Hyper Frame+ implementation or W2 work is included.

`tools/wsre/build_scan.py` builds a separate pinned MS-DOS disk, preserving
all system files including AUTOEXEC and LICENSE; it verifies their hashes
and exact added payload readback. It never runs the probe automatically.
An initial development build correctly failed the system-file audit when
its help batch replaced AUTOEXEC; the builder now adds SCANHELP.BAT instead.

Validation before the scanner source commit: isolated Unicorn controls
**9 tests PASS** (all seven settings, constant false replies, multiple/no
responders, wake selection, cleanup, target/argument/log-error controls).
Native build exit 0; focused CTest **5 PASS / 1 SKIP / 0 FAIL** (48.73 s):
WSR-E, BRKEM, state storage and general ROM-less checks. The optional scanner
CTest skips when the selected Python lacks Unicorn; the isolated venv run
above supplies its control evidence. With BUILD_ID=26b60fce the original
W1PORT/W1PROBE COMs remain byte-identical to the prior handoff. Final media
identity and guest checks are recorded separately below when complete.

### Scanner candidate verification

Evaluated scanner source:
[3657c64ee421066a78fd7e41154bc9b62b9724ee](https://github.com/nakatamaho/vaeg/commit/3657c64ee421066a78fd7e41154bc9b62b9724ee).
The main-to-work-branch merge is
[dcf69959672baa5c03e1a2466ebe9f2066238647](https://github.com/nakatamaho/vaeg/commit/dcf69959672baa5c03e1a2466ebe9f2066238647).
No milestone-to-main merge is authorized or performed.

Commands (private bindings and generated evidence kept out of tree):

```sh
python3 tools/wsre/build_scan.py --source "$MSDOS4_SOURCE" --output-dir "$NEW_OUTPUT_DIR"
"$TASK_VENV/bin/python" tools/wsre/test_scan.py
python3 tools/wsre/test_compare_logs.py
cmake --build "$NATIVE_BUILD" --parallel 4
ctest --test-dir "$NATIVE_BUILD" --output-on-failure -R 'vaeg_wsre_|brkem|vaeg_state_storage|vaeg_romless_tests'
```

Both scanner media builds returned 0 and their D88s were byte-identical.
The model controls returned 0 (9 PASS) and comparator tests returned 0
(8 PASS). Native/focused check exits and the optional skip are recorded
above. Repository encoding/EOL/case/Markdown, diff checks and the milestone
49-case validator passed. Hosted CI was not requested for this hardware
probe handoff; commits use the CI skip marker.

Pristine generated artifact SHA-256 values (public source-built probe and
pinned freely distributable MS-DOS system files, no private media content):

- W1SCAN.COM: `7437caa325c3edb9d62cded7bb9884f77f48edea3b46834c42086e6c5ecf8149`
- wsr-e-address-scan-msdos4.d88: `fa38a159ce6ebf05a4e34afebd5ef1ef063d98392ca5f29346f5b8ac9d59ccd9`

Five actual MS-DOS guest cases completed on the **VA2/V3 software target,
SDL dummy**, checking guest exit files, complete 12-column log rows and
final summary rather than using worker exit alone:

| Model | Command | Guest exit | Summary | Contract result |
|---|---|---:|---|---|
| OFF | W1SCAN | 0 | READ_ONLY | PASS |
| OFF | W1SCAN /P | 2 | NONE_INCONCLUSIVE | PASS |
| ON, fixed setting 0 | W1SCAN /P | 0 | MATCH 0 | PASS |
| ON, fixed setting 0 | W1SCAN /W 0 | 0 | MATCH 0 | PASS |
| ON, fixed setting 0 | W1SCAN /W 7 | 2 | NONE_INCONCLUSIVE | PASS |

Worker SHA-256:
`da5623aee2a5f8f86611757230e051b637839f2f2e42445f1c92c3831f9eb2ee`.
It was built after the main merge with the new CTest registration present;
the scanner does not change application/core behavior. Temporary guest
fixtures replace AUTOEXEC for automation; the delivered image retains its
pinned original and runs no probe automatically. Fixture preparation first
hit the install helper's non-vanilla-input rejection, before any guest ran;
rebuilding the fixtures from vanilla and inserting the identical COM resolved
that test-setup error. The candidate D88 remained unchanged, and handoff
copies were checked with `cmp`. Worker, case commands, log digests and raw
evidence are retained privately.

This confirms software execution and negative classification, **not**
physical setting 7, WSR-E wake-up correctness, timing, presenter layout or
Windows runtime. No new application executable is handed off here. Hardware
W1SCAN execution is NOT_RUN; G143 remains NOT_READY, with no W2/release or
full media/demo/presenter campaign performed.

### Hardware scanner return and driver analysis

The returned scanner COM matches the evaluated candidate byte-for-byte.
The VA/24kHz read-only and `/P` sessions return FF at all seven candidates.
A subsequent `/W 7` session, confirmed in both the returned log and screen
photograph, reaches its final NONE_INCONCLUSIVE summary and DOS prompt:
prior setup/misc reads are FF, and all four SR06 reads after provisional
wake are FF. The earlier two sessions remain an exact prefix of the new
log. Hardware execution is now observed, superseding the earlier NOT_RUN
entry, but successful detection, selector position and wake correctness
remain unestablished. No guest exit file was returned; do not infer a
measured ERRORLEVEL from the summary. Originals/extractions stay private.

Analysis of the supplied CL-GD54xx driver (its documentation lists WSR-E)
finds a materially different detection path. These are independently
summarized **static software findings**, not verified hardware requirements:

- The board-family path enumerates offsets 0,2,4,6,10,12,14, skipping 8.
  It first writes 7Ah to 40E1h+offset and requires a 7Ah readback before
  entering that candidate's VGA setup. An earlier, distinct family path
  instead checks an identification port; it must not be conflated with this
  handshake path.
- After the handshake, it writes 18h to common 46E8h, 01h to
  42E0h+offset, 08h to 46E8h, then EFh to 42E0h+offset. Our provisional
  scanner omits the handshake, uses E3h rather than EFh for the last misc
  write, and adds a status-port write not present in this detection path.
- It then selects CR27 at 54E0h+offset and reads 55E0h+offset. A value
  whose FC-mask is FC is rejected; masked 98h selects a distinct chip
  branch. There is no preceding SR06 signature test in this path.
- A bounded Unicorn execution of just the private detection routine,
  with controlled I/O callbacks, reproduces the setting-7 path and its
  return classification. Removing the handshake response or replacing
  the chip response with FF takes the failure path. All three controls
  complete with expected returns. This checks instruction/port-table
  interpretation only, not physical I/O or the validity of the fake chip.

This is a concrete omission relative to the inspected driver, **not yet a
proven cause** of the hardware failure. In particular, 40E1h+offset overlaps
board/relay control excluded from the original probe's write scope; do not
silently add it to `/P` or `/W`, or run the whole multi-board driver on VA.
A revised, explicitly scoped control-port probe requires side-effect and
cleanup review. Raw binaries, documents, addresses within their code,
disassemblies, asset identities and synthetic traces remain outside Git.
No emulator/probe implementation or new D88 is changed by this analysis;
G143 remains NOT_READY.

### WSR-E-specific Windows driver cross-check

The independently written [detailed initialization analysis](../../modernalization/wsr-e.md)
collects the DOS/Windows paths, port tables, observed hardware results,
probe differences and unresolved side-effect/cleanup questions. Its path
uses the spelling explicitly requested by the maintainer.

A second supplied package explicitly binds WSR-E's display device to its
16-bit display driver and E-bank mini-VDD; its separate sound-device INF
must not be mistaken for the graphics setup. Original packages, extracted
modules, converted text, hashes and disassembly remain private. Nothing is
installed or executed against the host OS or physical hardware.

Parsing the NE code segments and LE objects, then resolving the local port
table, finds an additional difference from the current scanner. A wake
routine writes common setup 18h, selected misc 01h, common setup 08h,
**01h twice to 43E0h+offset**, selected misc E3h, then selected status 00h.
At setting 7 the additional port is 43EEh. The 16-bit display driver has
this same sequence in a path entered when its initial CR27 read is FF;
the mini-VDD also contains the sequence. The scanner currently omits the
two 43E0h+offset writes. Their hardware effect and restoration semantics
remain to be verified; the observed software sequence alone does not prove
the cause of the VA's FF results.

This also narrows the previous interpretation: **E3h is used by the
WSR-E-assigned Windows driver**, so the DOS driver's EFh versus our E3h
is not by itself proof of an incorrect misc value. Keep these distinct
software paths rather than assembling an unverified hybrid initialization.
The Windows driver also uses 7Ah/7Bh at the board-control port during display
setup; those broader routines are not a safe minimal detection probe.

Two isolated Unicorn controls, settings 0 and 7 with synthetic port-table
selection and identification reads of FF, execute only the mini-VDD wake
routine. Both return and produce exactly the expected seven byte writes,
including the duplicate enable-port write. These are instruction/port-table
checks, not Windows execution, hardware enable verification, or relay safety
proof. No new D88 or probe/core change is made. Review a minimal enable-only
experiment before considering the broader board-control handshake; the old
no-relay scope is not silently lifted. G143 remains NOT_READY.

### W1INIT probe preparation

The maintainer requested a new D88 probe taking the DOS driver's candidate
search and possible V30-only machine constraints into account. W1INIT is an
independently authored cpu-186 COM, with byte I/O and DOS INT 21h only; it
uses no PC-98 BIOS or CPU-type test. This removes those software dependencies,
not the unresolved physical slot/bus requirements. It is still a VA/V3 tool,
not an assertion of compatibility with PC-98 V30 machines.

W1SCAN is preserved. W1INIT defaults to read-only, writes WSRINIT.TXT, and
separates `/E n` (CR27-pre-FF conditional enable sequence, including duplicate
43E0h+offset writes) from `/D n` or explicit `/D ALL` (7Ah board-control
handshake, then the DOS EFh sequence, without the `/E` writes). `/D` is a
new, conspicuously warned board/relay-write opt-in; it is not silently added
to old modes. All requested ports must be exclusively assigned to WSR-E.
Unknown control/enable values are not restored by guessing. The original
relay-free scope continues for `/E`. No VRAM, GR-bank, CRTC-data, word-I/O,
timing or IRQ-hook test is included.

CR27 is read before/after wake and classifies a masked-98h chip candidate;
SR06 is observed independently, not required for reaching the ID read.
Candidate, multiple and inconclusive outcomes are not a hardware PASS.
Restoration messages describe write attempts, not readback-proven complete
restoration. [Detailed usage](../../../demos/wsr-e-pseudo-sprite/README.md#w1init-separated-enable-and-dos-detection-experiments)
and the [analysis supplement](../../modernalization/wsr-e.md#10-改訂プローブw1init)
state the residual-state and separate-power-on comparison requirements.

Before the source commit, `tools/wsre/test_init.py` passes **13 tests** in
the isolated Unicorn environment, including exact separate output sequences,
all seven settings, false/partial/multiple responses, CR27/SR independence,
cleanup, every log-write failure point, invalid arguments/target, and checks
of the executed instruction/interrupt subset. The old scanner controls
pass 9 tests and original comparator passes 8. `build_scan.py --probe init`
passes the pinned-system and exact-payload audit. With explicit historical
build ID 3657c64e, default scanner generation remains byte-identical to its
prior D88. No existing emulator source behavior is changed; old application
worker evidence can be reused with its original binary identity. Final probe
media and guest verification are recorded separately after the source commit.

### W1INIT candidate media and guest checks

Evaluated source:
[d6fadd3469a6736355ced97050af8dd6ec0d0273](https://github.com/nakatamaho/vaeg/commit/d6fadd3469a6736355ced97050af8dd6ec0d0273).
`build_scan.py --probe init --source "$MSDOS4_SOURCE" --output-dir "$NEW_OUTPUT_DIR"`
returned 0 twice; raw D88s were byte-identical. Pinned boot/system/license
hashes and added COM/help/license payload readback passed. Copies into the
private handoff were verified with `cmp`. Public generated payload identities:

- W1INIT.COM: `a6e15bdc51d017fbab160a6eb36766347ed3462f6fa00d9003a0b6eca56671c1`
- wsr-e-init-probe-msdos4.d88: `23b92eabbb00261e5eb9230da3b97657d4e7913ab1fc59af795de56c1164f332`

The isolated controls passed 13 W1INIT tests (exit 0, 1.114 s), 9 scanner
tests and 8 comparator tests. Encoding/EOL/case/Markdown, diff and milestone
49-case checks passed. There is no application behavior change; diff of the
application/core directories against the retained worker's main-merge source
is empty. The CMake change only registers the optional new control test.
The retained worker digest is unchanged:
`da5623aee2a5f8f86611757230e051b637839f2f2e42445f1c92c3831f9eb2ee`.

Eight actual VA2/V3 MS-DOS guest runs under SDL dummy completed. Initial
integration assertions are **6 PASS / 2 FAIL**, not an all-PASS campaign:

| Model | Command | Actual exit | Actual summary | Initial assertion |
|---|---|---:|---|---|
| OFF | W1INIT | 0 | READ_ONLY | PASS |
| OFF | W1INIT /E 7 | 2 | NONE_INCONCLUSIVE | PASS |
| OFF | W1INIT /D ALL | 2 | NONE_INCONCLUSIVE | PASS |
| ON, setting 0 | W1INIT /E 0 | 0 | CHIP_CANDIDATE 0 | PASS |
| ON, setting 0 | W1INIT /E 7 | 2 | NONE_INCONCLUSIVE | PASS |
| ON, setting 0 | W1INIT /D 0 | 2 | NONE_INCONCLUSIVE | FAIL: assumed positive handshake |
| ON, setting 0 | W1INIT /D ALL | 2 | NONE_INCONCLUSIVE | FAIL: assumed positive handshake |
| ON, setting 0 | W1INIT /D 7 | 2 | NONE_INCONCLUSIVE | PASS |

The two failed assumptions are explained by the unchanged
[`wsre_inp`](../../../cbus/wsre.c): Melco board-port reads intentionally return
FF pending hardware evidence, including 40E1h after a 7Ah write. Both logs
therefore show ACK=FF / REJECT_NO_WAKE. The probe refuses further candidate
initialization as designed. No physical expectation, emulator behavior or
original failed report is changed to fit these results. DOS positive-handshake
integration is **BLOCKED by the current model**, with that success path tested
only in the independent synthetic fixtures. This is not a claim about the
real board's 7Ah readback.

Checks include guest exit files, COM identity, complete CSV rows/build ID,
gate and CR27/SR06 observations and final summaries. Raw per-case commands,
logs, captures and their digests stay private. Test images replace AUTOEXEC
for automation; the pristine handoff retains the pinned original and never
auto-runs a probe. Both candidate image and worker digests remained unchanged.
No expensive rerun of the unchanged worker is needed to preserve these facts.
At this handoff, hardware W1INIT execution was NOT_RUN and G143 remained
NOT_READY. This is an experimental DOS-probe handoff, not an application
build or milestone gate. The subsequent intake below updates execution status.

### W1INIT hardware return (input I1)

The maintainer returned the revised probe disk. Its W1INIT.COM is byte-identical
to the d6fadd34 handoff above. A separate snapshot and extraction preserve the
input unchanged; raw media/logs, receipt and hashes remain private. The log
identifies **VA / FFFFh / 24kHz / d6fadd34**. All four session banners and all
46 data rows satisfy the 12-column schema and metadata checks; each session
has its expected terminal summary. No guest ERRORLEVEL file was returned.

| Recorded path | Observations | Terminal summary |
|---|---|---|
| Read-only survey | All seven legal candidates return FF from the current SR data port | READ_ONLY |
| `/E 7` | CR27 pre/post FF; wake performed, including the two enable writes; SR06 before and all four signature reads FF | NONE_INCONCLUSIVE |
| `/D 7` | Prior control FF, 7Ah write followed by ACK=FF; rejected before VGA wake | NONE_INCONCLUSIVE |
| `/D ALL` | Candidates 0/1/2/3/5/6/7 all have prior control FF and ACK=FF after the 7Ah write | NONE_INCONCLUSIVE |

Every DOS-path candidate records REJECT_NO_WAKE; there are no DOS-path
CR27, SR06 or wake rows. Thus its EFh-ending VGA initialization was **not
reached**, not independently tested and found ineffective. `/E 7` reaches
both CR27 observations without relying on an SR06 signature, but still
produces no chip candidate. Other `/E` candidates are not represented in
this return. Cleanup rows report unknown saved values and skipped writeback,
not readback-proven restoration. Power-cycle boundaries and the physical
selector position are not established by these appended sessions.

Hardware W1INIT execution is now observed, superseding the handoff's NOT_RUN
status, but board detection remains **NOT_ESTABLISHED** and G143 NOT_READY.
A complete summary does not establish the measured DOS exit status. All-FF
control reads narrow the observed failure to the handshake stage under these
conditions; they do not prove bus incompatibility, CPU-based rejection,
incorrect switch position, contact failure or a faulty board. In particular,
do not promote the emulator's existing provisional FF behavior to a validated
functional-board model merely because it matches this unsuccessful intake.
Pause additional write experiments and compare the documented board I/O-enable
conditions and VA slot decode/signals before designing another hardware test.
No probe/emulator/media change, new application build, expensive behavioral
rerun, W2 work or gate acceptance accompanies this evidence-only update.

## Open

- Does the VA slot pass 16-bit I/O cycles (SR06W, GR09W rows)?
- Melco values of the board ports; meaning of 40E1h bit 1.
- CR27 of the WSR-E's 5428 (98h assumed).
