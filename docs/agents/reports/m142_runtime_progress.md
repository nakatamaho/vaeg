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
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
-->

# M142 implementation and verification checkpoint

**M142 is IN_PROGRESS; G142 is NOT_READY.** The complete MacPorts Windows
build succeeds, and the settings/state implementations have software tests.
The hardware mechanism for #75 was **BLOCKED** at this checkpoint, not fixed.
The later [ten-screen hardware follow-up](m142_tvram_vb_hardware.md) lifts the
missing-results blocker; cause and correction remain OPEN/NOT_IMPLEMENTED.
The reported FPS example and final GUI/presenter width are not reproduced/verified. The full
milestone media/guest regression and acceptance are still outstanding.

Task: [M142](../tasks/M142_runtime_portability.md). This is a progress record,
not a gate-pass or release authorization. The maintainer reported the MH demo
fixed by the CPU-role change; it is no longer an M142 investigation and no new
MH fix is claimed here. The separate later MA stop is also not closed.

## Evaluated identities and reuse

- Clean Windows source:
  [`6f94fcb8467c0cf5beb196ea26de0c2e571ced55`](https://github.com/nakatamaho/vaeg/commit/6f94fcb8467c0cf5beb196ea26de0c2e571ced55).
- Native diagnostic worker source/embedded identity:
  [`f5f7dbc43f4113214464c6051a9480aca5d65352`](https://github.com/nakatamaho/vaeg/commit/f5f7dbc43f4113214464c6051a9480aca5d65352).
  SHA-256 `b1e350c24d16d6d1582ce46b1aa7cff624ffd4faf76faa966470674661bc415d`.
- The native worker is a Release QA build with recording/static minimal
  FFmpeg ON and local encoders OFF, **librashader OFF**, SDL dummy for guest
  tests. It is not the complete desktop/Windows handoff or presenter evidence.
- Between these commits only the static CPU preset validator and whitespace
  in two C files changed. A rebuild after the whitespace edits produced the
  **identical native executable SHA-256**. The completed CPU/guest tests are
  retained; only the corrected static validator is rerun. The native binary
  is not relabeled as a clean build of the later commit.
- Commands, caches, raw logs and captures remain in task-specific, out-of-tree
  evidence. No private input names, ROM identities or captures are published.

## A — FPS measurement and digits

The existing measurement retains 32-bit tenths after 64-bit arithmetic, and
the status formatter uses unrestricted `%u`. New tests cover **32.0, 99.9,
100.0, 132.0, 999.9 and 1000.0 FPS**, requiring the exact expected string and
measurement. They pass in the native romless suite. No adding 100, fixed
three-digit buffer, rate cap or guessed production correction was introduced.

`draw_machine_status()` submits the full line with `TextUnformatted`; its
left origin does not move to the FPS substring. A line wider than a narrow
window can be clipped at the right and has a full-line tooltip. That source
inspection is not a final-client layout/reference comparison. The original
visible `32.0` has not been established as a clipped `132.0`. FPS measures
actual rendered frames, not the selected nominal CPU MHz; extra work per
frame need not increase measured FPS. **Actual reported overclock case and
presenter width: NOT_RUN/unresolved.**

## B — fresh expansion-memory settings

Fresh initializers and absent INI keys now select `ExMemory=0` and disabled
BMS. Explicit persisted selections are preserved. The board APIs and GUI
still support EMS 1–13 MB and optional BMS at native port `01D0H` with its
existing capacity. Native lifecycle/configuration/state selftests pass,
including absent-key defaults. Current utility/GUI documentation is updated;
historical acceptance records are not rewritten as new results. An empirical
fresh-versus-enabled state-file-size comparison is not recorded in this run.

## C — numeric clock choices

The evaluated checkpoint offered **2, 4, 6, 8, 10, 12, 16, 32 and 64 MHz**,
explicitly nominal. The maintainer's later menu-only follow-up narrows this to
**4, 8, 10, 12 and 20 MHz**; see the task record for the new handoff. The common resolver chooses CPU multiplier and 50%/100% CPU speed
against the active H/native or compatible S domain. All nine choices in both
domains and invalid inputs pass the software contract tests. Existing main/
FDC-role and lifecycle tests also pass; no FDC/peripheral clock assignment was
changed by this menu feature. These are not new physical clock measurements
or a completed GUI-operation/restart matrix.

## D — #75: published screen-5 measurements, not a new hardware run

The following are **all E–W rows published in
[issue #75](https://github.com/nakatamaho/vaeg/issues/75)**. The ratios are
vaeg/hardware as reported there. No unreported A–D/X hardware values are
invented, and these historical measurements are not bound to the M142 worker.

| Row | VA hardware | vaeg VA | Ratio | VA2 hardware | vaeg VA2 | Ratio |
|---|---:|---:|---:|---:|---:|---:|
| E TV B WRITE OFF | 1292 | 1246 | 0.96 | 1201 | 1246 | 1.04 |
| F TV B WRITE 31 | 1307 | 1246 | 0.95 | 1142 | 1246 | 1.09 |
| G TV B READ OFF | 1306 | 1202 | 0.92 | 1211 | 1202 | 0.99 |
| H TV B READ 31 | 1307 | 1204 | 0.92 | 1226 | 1206 | 0.98 |
| I TV W MOVSW OFF | 3252 | 3235 | 0.99 | 3368 | 3393 | 1.01 |
| J TV W MOVSW 31 | 3251 | 3235 | 1.00 | 3359 | 3393 | 1.01 |
| K TV W ATTR 31 | 3252 | 3285 | 1.01 | 3360 | 3443 | 1.02 |
| L TV W READ 31 | 3242 | 3277 | 1.01 | 3349 | 3436 | 1.03 |
| M TV W STOSW 31 | 4160 | 4091 | 0.98 | 4281 | 4336 | 1.01 |
| N TV W MOVSW 8 | 3251 | 3235 | 1.00 | 3365 | 3393 | 1.01 |
| O TV W MOVSW 31M32 | 3252 | 3235 | 0.99 | 3347 | 3393 | 1.01 |
| P TV W MOVSW 31C64 | 3252 | 3235 | 0.99 | 3364 | 3393 | 1.01 |
| **Q TV VB 128B 31** | **2407** | **2408** | **1.00** | **602** | **875** | **1.45** |
| R G1 VB 128B | 4815 | 5112 | 1.06 | 1505 | 1505 | 1.00 |
| S VB 1/1000 FRM 31 | 86 | 85 | 0.99 | 20 | 21 | 1.05 |
| T SPRON CMDS 31 | 11782 | 11747 | 1.00 | 10836 | 10857 | 1.00 |
| U TV W MOVSW S200 | 3252 | 3150 | 0.97 | 3282 | 3304 | 1.01 |
| V TV B WRITE S200 | 1305 | 1326 | 1.02 | 1297 | 1326 | 1.02 |
| W TV VB 128B S200 | 2407 | 2407 | 1.00 | 2407 | 2407 | 1.00 |

Thus Q/S is approximately **28.0 (VA), 30.1 (VA2), 41.7 (emulated VA2)**.
Every published ratio other than VA2 Q is 0.92–1.09. The short-blank Q
exception does not establish a generic TVRAM word wait: I–P/M and long-blank
W are controls that a correction must preserve.

Source findings: Q checks port `0142H` bit `40H`, executes `REP MOVSW` for
64 words and counts completed chunks. The last counted chunk can finish
outside VB. Entry accepts an already active VB rather than requiring a new
edge. TVRAM mapping/interrupt masking and entry/poll overhead can therefore
matter disproportionately in a short blank; S is itself a polling fraction.
These facts define hypotheses, **not a demonstrated hardware cause**. #74's
separate VRTC/VB phase mismatch is not adopted as #75's explanation.

A free [hardware measurement kit](../../../tools/hardware/tvram_vb/README.md)
now builds the unchanged TSPSPR plus eight fixed-layout variants: 8/32/128/256
bytes per chunk, current-VB/new-edge entry, full word/byte/G1/long-blank controls.
It uses the pinned MIT MS-DOS 4 disk and identifies machine/variant on screen.
The nine COMs assemble; all variants are 7350 bytes and the stock 7338-byte
COM is byte-identical to the committed TSPSPR distribution. Independent disk
content checks confirm the exact pinned vanilla system files and ten source-
built COM/help payloads. Raw generated image SHA-256:
`c49ea0e1ab41b80a1804e48c1d863f09ef183f84d5a319e4b87710ec4af1c3aa`.

One VA2 variant was observed drawing its DONE/results screen under the native
SDL-dummy worker at 15 kHz; that is only a smoke observation, not an automated
readout contract or physical timing result. The first run correctly returned
the demo's 24.8-kHz rejection until the explicit 15-kHz setting was supplied.
The initial generator attempt failed its fixed-label-width guard and is
retained; the corrected final output uses fixed-width labels/autostart and
retains the variant ID at DONE. At this checkpoint no physical results had
been returned. **Checkpoint hardware limiter: BLOCKED; correction:
NOT_IMPLEMENTED.** The later [hardware follow-up](m142_tvram_vb_hardware.md)
retains all 240 returned rows and ten matched software observations separately
from this historical issue table; it does not claim a cause or correction.
The later [instruction-cost/SYNC analysis](m142_tvram_vb_events.md) records six
Q traces and three control executions using the unchanged worker. Extra
128/256-byte copies are authorized near the short-window threshold; REP
redispatch changes charged costs. A preselected 200-line SYNC also yields
S/X=87/87 on VA2 without a VA hold, but does not prove the older VA's initial
selection or key history. Cause OPEN; correction NOT_IMPLEMENTED; no new
hardware request or binary build. The same report's later
[G1/long-blank controls](m142_tvram_vb_events.md#g1-and-long-blank-controls)
add three R/W trace runs, reuse matching Q traces and capture one full set of
A–X register returns. A frozen-timeline common cutoff cannot preserve the
matched G1 control while reducing 256-byte Q; rescheduled execution and
physical timing are not covered by that test. A reusable content analyzer
passes six Python tests and the focused builder CTest (1/1), not a new full
emulator suite. No waits, VB/REP implementation or VA hold change.

## Portable states — #76

DISK v1 is ten explicit **4108-byte** records with 4096-byte UTF-8 paths,
little-endian flags/date and explicit time/reserved bytes, not a native C
structure. Legacy v0 readers accept 260/1024/4096-byte path layouts. A shared
resolver used by GUI/debug preflight and load keeps existing paths or finds a
unique basename in configured media folders, recognizing either separator.
Missing/ambiguous candidates remain disk-change warnings; unsupported layout,
version, unterminated path, invalid flags or reserved bytes fail closed.

Native tests load independent whole-state layout fixtures, restore both D88
image selections and read-only state, test foreign paths, existing-file
precedence, missing/overlong paths and ambiguous folders. Five malformed-state
negatives require exact codes and nondestructive rejection. **Native complete
Windows↔Linux execution/exchange: NOT_RUN.** See the
[correctness ledger](../../modernization/bug-fixes.md#fixed-defects).

## Release identity

Explicit tracked metadata declares `M142-dev`. Title, About, CLI and build
reports share `Rel.20261029 (M142-dev, 6f94fcb8)` for the clean Windows source.
The package-name helper includes milestone/SHA for development archives and
emits the approved `vaeg-20261029-m141-windows-x64` example for an official
M141 fixture. Release workflow preflight rejects development milestones;
canonical-ID and single-mutation content/CLI tests pass. No tag/release/asset
was created or renamed. The 16-byte legacy operation-record version stays
date-only; display text is not inserted into that wire header.

## Commands, exits and remaining gate checks

`$SCRATCH` below is a task directory outside Git; raw commands/paths are in
local evidence, not a private-input binding in this report.

| Check | Command / outcome |
|---|---|
| Native configure/build | `cmake -S . -B "$SCRATCH/native"`; `cmake --build "$SCRATCH/native" -j4`: exits 0 |
| Full native suite | `ctest --test-dir "$SCRATCH/native" --output-on-failure -j4`: exit 8, 148 PASS / 16 SKIP / 1 FAIL |
| Corrected static case | `ctest --test-dir "$SCRATCH/native" -R '^vaeg_upd9002_native_invariant$' --output-on-failure`: exit 0; aggregate **149 PASS / 16 SKIP / 0 FAIL** with unchanged-worker reuse |
| Release identity | `python3 tests/frontend/test_release_identity.py`: exit 0, 4 tests |
| MacPorts SDK contracts | `python3 tests/frontend/test_mingw_macports.py`: exit 0, 3 tests |
| Hardware-kit generator | `python3 tools/hardware/tvram_vb/test_build.py`: exit 0, 2 tests; final build/disk content audit exit 0 |
| Recording contracts | `python3 tests/frontend/recording/test_release_recording.py`: exit 0, 28 tests; MP4 policy 21 tests exit 0 |
| Repository checks | UTF-8, LF, case, Markdown, whitespace, milestone audit all exit 0; milestone selftest 49 checks |
| Reusable guest checker | `python3 tools/qa/all_demos_draw.py --selftest`: exit 0, 6 profiles / 25 negatives; **not** a live demo-suite rerun |
| Formatting | Global checker exit 1, **659 pre-existing diagnostics / 29 files**; all map to the M142 base, **zero new** after scoped corrections |

The sole full-suite failure was the native-CPU validator's closed preset list
not yet containing the newly added `mingw-macports`. Its correction adds that
one name without weakening other invariants. First full-suite log SHA-256:
`49469c46f31749af99f58cc71c2921b45d08e3def77c222e081e21f8ccd4f558`;
focused passing log:
`b732b688d239000dd73e928606ba71270fdb77095be726ba11b6cc12d8e520b0`.
Skipped cases: H.264 hardware probe, SST corpus, six assembled FDC payloads,
two assembled main/FDC probes, private BASIC boot and five xz shell/generated
fixture tests. Skips do not become passes by aggregation. Hosted CI: NOT_RUN.

Clean static Windows command, exit **0**:

```sh
python3 tools/release/build-mingw-macports.py \
  --commit 6f94fcb8467c0cf5beb196ea26de0c2e571ced55 \
  --work-dir "$SCRATCH/mingw-runtime" --sdk-dir "$SCRATCH/mingw-sdk" --jobs 4
```

Fresh source clone/application objects; only audited SDKs reused. MacPorts
GCC/G++ 15.2.0, binutils 2.45, pinned Rust 1.88.0; no Colima. Recording,
shaders, static SDL/archive/FFmpeg are ON; bundled local encoders OFF.
Encoder-archive/system-DLL import/embedded identity audits pass. The executable
is 73296250 bytes, SHA-256
`795ef24174fdc0b040d07715038c7e15c74d9a6ef312228c0acc6563749893d6`.
It is `output/vaeg.exe` beside `BUILD-REPORT.md`, `build.json`, the generated
release identity, notices and import evidence. **Windows execution/presenter
and real Windows/Linux state exchange are not cross-compilation results.**

Before G142, still required: #75 causal hardware evidence/correction, actual
FPS/presenter-width case, remaining GUI/settings/interoperability checks,
all demo/media rebuilds and payload comparisons, live all-demos on both OSes,
MANDEL and the utility FDD/both SASI boot contracts. The one stock TSPSPR
payload comparison does not mark that suite passed. Prior G139/G140 gate
acceptances remain closed and are not being requested again.
