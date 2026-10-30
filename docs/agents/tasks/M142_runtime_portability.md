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

# M142 — Runtime settings, TVRAM investigation and portable states

## Assignment and scope

Base: `75ad3b9129038c3c54a834283ac709b8c02a6ce9`.

The current assignment covers A–D, portable states and the Windows handoff
on macOS using MacPorts MinGW, **without Colima**. Establish that build
first; if it cannot be built, stop and report the precise blocker. Otherwise
report results together at the end. Do not install/change system ports or
reuse a default container daemon. Keep generated SDKs and evidence out of Git
and within the work-tree boundary. Earlier scratch/builds have been deleted;
old reports do not substitute for available, identity-bound evidence.

- **A** Investigate Information → FPS above 100 FPS, including formatting,
  measurement and actual bar width. Do not assume `32.0` means `132.0`.
- **B** Fresh settings omit EMS and bank memory; explicit saved choices survive.
- **C** Offer 4, 8, 10, 12 and 20 MHz clock choices. The maintainer's follow-up
  removes 2, 6, 16, 32 and 64 MHz from the menu and adds 20 MHz. Existing saved
  multiplier/speed settings and machine/FDC clock separation remain unchanged.
- **D** Investigate [#75](https://github.com/nakatamaho/vaeg/issues/75): identify
  what limits VA2 VB-time TVRAM writes on hardware before modeling it. Preserve
  other TSPSPR rows. Q/S observations (28.0 VA, 30.1 VA2, 41.7 emulated VA2)
  are not a demonstrated root cause or permission for a fitted slowdown.
- **Portable states** Correct [#76](https://github.com/nakatamaho/vaeg/issues/76):
  a host-independent DISK record, same-host legacy loading, GUI-equivalent path
  remapping and positive/negative selftests.

The original assignment called #75 E and portable states F. Its MH reset
item is superseded: the maintainer reported the CPU-role change fixed MH.
No duplicate MH investigation/new MH correction is required; the separate
later MA red-screen stop is not being closed.

## Verification and gate

Focused builds/tests and stable-code single-mutation negatives precede the
integration suite. Record evaluated commits, commands, exits, worker and output
identities outside Git; public summaries use neutral fixture identifiers.
Run encoding, EOL, case, Markdown, milestone audit/selftest and whitespace
checks. Respect existing formatting debt without unrelated reformatting.
No hosted iterative debugging or private-asset upload.

The final gate requires the normal clean pushed-commit static Windows handoff,
recording ON, bundled local encoders OFF, system-DLL import audit, embedded
identity, plus demo/media rebuild/payload and live guest-content regression
specified in AGENTS.md. MacPorts is the maintainer-approved alternative to the
container for this task; do not silently omit shaders or recording to obtain
a successful build. Windows execution is distinct from cross-compilation.
The standard human acceptance is still required; prior G139/G140 acceptances
are not reopened. A hardware-dependent item remains BLOCKED, not PASS, if no
causal measurement can be obtained.

## Progress

[Implementation/evidence checkpoint](../reports/m142_runtime_progress.md)
contains the complete published issue #75 E–W comparison table, commands,
exits, identities, skip/reuse boundaries and remaining gate checks.

- **MacPorts complete Windows build: PASS (cross-build only).** First complete
  prerequisite at `2e662105164d69948d312c673045b406b8e8f59d`; a fresh clean clone
  of pushed `6f94fcb8467c0cf5beb196ea26de0c2e571ced55` now includes runtime fixes.
  Recording/shaders/static dependencies ON, local encoders OFF, no Colima;
  imports/encoder archive/embedded identity audits pass, command exit 0.
  Checkpoint executable SHA-256:
  `795ef24174fdc0b040d07715038c7e15c74d9a6ef312228c0acc6563749893d6`.
- **A:** high-digit measurement/format tests pass; no production digit defect
  is demonstrated. Actual reported overclock/presenter-width case unresolved.
- **B/C:** absent expansion defaults and numeric nominal clock choices are
  implemented; native configuration, domain and existing main/FDC tests pass.
  The later gate-preparation checks below establish synthetic save size and
  process restart, not GUI-driven changes or presenter acceptance.
- **D/#75: OPEN; hardware results received.** Ten DONE photographs now cover
  VA and VA2/3 labels, with all 240 A–X rows transcribed; ten matched native
  software executions are also recorded in the [hardware follow-up](../reports/m142_tvram_vb_hardware.md).
  VA2/3 32-byte Q nearly matches vaeg, but 128/256-byte counts do not; the new
  VA S=20 also conflicts with the earlier VA S=86 condition. The missing-input
  blocker is lifted, not the unresolved-cause/correction requirement. No fitted
  slowdown, physical-limiter claim or automatic gate PASS is applied.
  The [instruction-cost/SYNC follow-up](../reports/m142_tvram_vb_events.md)
  adds six Q traces and three controls with the unchanged worker: late polls
  authorize whole extra copies, REP redispatch affects modeled cost, and a
  prior 200-line selection is an alternative explanation for a large S even
  without a VA hold. It does not establish historical key/SYNC conditions or
  a physical limiter. Nine capture runs complete; one earlier script attempt
  is retained as incomplete. A subsequent three R/W trace runs and one all-row
  register-return run complete with the same worker. The frozen-cost/phase
  common-cutoff test cannot both reduce 256-byte Q and preserve G1's five-chunk
  control; this is not a hardware-cause proof or a modified-scheduler result.
  Ordinary A–X controls and reusable trace analysis are recorded in the same
  report. Python tests: 6 PASS (14 new exact-code content/CLI negatives);
  focused `vaeg_tvram_vb_builder` CTest: 1/1 PASS. Three matching Q traces are
  reused, not rerun; no emulator/COM change or binary rebuild. No additional
  photographs are requested. Cause OPEN; correction NOT_IMPLEMENTED.
- **#76:** explicit DISK v1/legacy layouts and shared remapping implemented;
  real synthetic state loads, image/read-only preservation, ambiguity and
  exact-code malformed-input tests pass. Native Windows/Linux exchange NOT_RUN.
- Approved release identity is implemented/tested as explicit `M142-dev`
  metadata shared by UI/CLI/reports and future package names; legacy operation
  records retain their date-only compatibility field. No release published.
- Native suite ran all 165 entries: 148 PASS / 16 SKIP / 1 FAIL; the sole
  failure was the new preset missing from the static invariant's closed list.
  That one-name correction passes on focused rerun, giving **149 PASS / 16
  SKIP / 0 FAIL with unchanged-worker reuse**, not a new all-suite execution.
- Repository checks pass; formatting has zero new findings and the same 659
  baseline diagnostics in 29 files. Demo/media/live regression was NOT_RUN at
  this original checkpoint; the later follow-up below records the new subset.
  Windows execution and final acceptance remain NOT_RUN.
- **M142 IN_PROGRESS; G142 NOT_READY.** This checkpoint does not authorize a
  merge or reopen the accepted G139/G140 scopes.

## Gate-preparation automation

The [gate-preparation record](../reports/m142_gate_preparation.md) adds actual
native full-state continuation and process-restart checks for absent, explicit
OFF and EMS 13 MiB/BMS 16 MiB configurations. All three continuation/restart
pairs pass, with registers and TVRAM/GVRAM restored. States are 2111040 bytes
when disabled versus 32519776 enabled: the delta is 29 MiB plus two section
headers. A passing save fixture and two exact-code live rejection cases verify
non-overwrite/path safety. The content selftest has 10 single-mutation negatives;
failed validator assumptions and completed-run reuse are explicitly retained.
Evaluated native worker: pushed `0d02bc849b5c4fb4f45d21c13716735e77578d37`.

The complete local CTest run has 166 entries, initially 148 PASS/16 SKIP/2 FAIL.
Roadmap gate-label ambiguity and parallel full-selftest shared scratch are
corrected; the four affected CTest entries pass. Aggregate **150 PASS/16 SKIP/0
FAIL with unchanged-worker reuse**, not a second fresh full run. A new clean
MacPorts Windows build of pushed `ea62eda1eb2d1d3abed11f72af2e5e7899868652`
succeeds with recording/shaders ON and bundled encoders OFF; its SHA-256 is
`d9c3bbdc36d88e5cc23a3587f2cbe7870594af2c55634dfe97e4e6f32b2b132a`.
It remains a cross-build, not Windows execution or final gate acceptance.

The [REP alignment accounting](../reports/m142_tvram_vb_events.md#rep-alignment-accounting)
reuses nine old traces and explains R's extra continuation cost by odd-address
prefetch after scheduler yields. It is not a hardware-cause proof or correction.
Local macOS API preflight reports zero active displays, no screen-capture
permission and no accessibility trust; actual Cocoa FPS/layout automation is
BLOCKED, not replaced by SDL dummy. No Windows execution target was found.
GUI-driven settings and actual Windows/Linux exchange remain outstanding.
The following media follow-up runs the demo subset; utility generation/boot
still block the complete mandatory regression. #75 OPEN; G142 NOT_READY.

## Demo and media regression follow-up

The [fresh media/live report](../reports/m142_demo_media_regression.md) records
new execution, not historical G139 reuse. Media builders run from a fresh,
initially clean clone of pushed
[`33dcb8ba9091cdf93eab8e1399478d5508c3e593`](https://github.com/nakatamaho/vaeg/commit/33dcb8ba9091cdf93eab8e1399478d5508c3e593).
The actual native worker remains source `0d02bc849b5c4fb4f45d21c13716735e77578d37`,
SHA-256 `24f53861e4a81f0914d489c353921113cc81e0aa1f730f00b9a9b07deea70700`.
Application inputs are unchanged; no emulator/Windows rebuild or relabeling.

Isolated NASM 2.16.01, Python 3.12.13, psd-tools 1.26.0, Pillow 13.0.0 and
NumPy 2.5.4 reproduce all nine distributions. Installed ports are untouched;
no Colima/container or ImageMagick rendering fallback is used. The following
checkout-relative commands substitute neutral read-only local input bindings
and task output paths for the exact private argv retained outside Git.
`SYSTEM_01` is the boot-template binding, `SYSTEM_02` the pinned MS-DOS input,
and `SOURCE_ASSET_01` the approved external source asset. **All ten image-build
commands below exit 0.**

```sh
demos/glass-orbit/build-d88.sh "$SYSTEM_01" "$OUT/images/m142-glass-orbit.d88"
demos/neon3/build-d88.sh "$SYSTEM_01" "$OUT/images/m142-neon3-distribution.d88"
demos/neon4/build-d88.sh "$SYSTEM_01" "$OUT/images/m142-neon4-distribution.d88"
demos/sgp-pseudo-sprite/build-d88.sh "$SYSTEM_01" "$OUT/images/m142-sgp-pseudo-sprite.d88"
demos/sgp-wireframe/build-d88.sh "$SYSTEM_01" "$OUT/images/m142-sgp-wireframe.d88"
demos/zundamon-orbit/build-from-psd.sh --python "$PSD_PYTHON" \
  "$SOURCE_ASSET_01" "$SYSTEM_01" "$OUT/images/m142-zundamon-orbit.d88"
# The binding adapter copies these new archives, never the old references.
for name in glass-orbit neon3-distribution neon4-distribution \
  sgp-pseudo-sprite sgp-wireframe zundamon-orbit; do
  cp "demos/disks/m142-$name.d88.xz" "$OUT/distributions/$name.d88.xz"
done
python3 tools/pc88va/build-all-demos-distribution-disk.py --source "$SYSTEM_01" \
  --distribution-dir "$OUT/distributions" --output "$OUT/images/all-demos.d88" \
  --compressed-output "$OUT/distributions/all-demos.d88.xz"
python3 tools/pc88va/build-all-demos-msdos4-disk.py --source "$SYSTEM_02" \
  --distribution-dir "$OUT/distributions" --output "$OUT/images/all-demos-msdos4.d88" \
  --compressed-output "$OUT/distributions/all-demos-msdos4.d88.xz"
python3 demos/sgp-boot/build-boot-d88.py --output "$OUT/images/sgp-boot.d88" \
  --compressed-output "$OUT/distributions/sgp-boot.d88.xz"
python3 tools/pc88va/build-all-demos-bootable-disk.py --source "$SYSTEM_01" \
  --distribution-dir "$OUT/distributions" --output "$LIVE_BOOT_01"
```

All compressed outputs round-trip to their new raw D88. Eight recursive
payload maps and the whole self-boot image match their committed references;
**all-demos-msdos4 and sgp-boot also match the entire raw image**. Other rows
claim payload equality, not whole-image equality. Public generated identities:

| Distribution | Raw D88 SHA-256 | Compressed D88.xz SHA-256 |
|---|---|---|
| glass-orbit | `f51be19912043be77370049bbcb4b1af224ac0f990826fa69797cf889f2b1f1c` | `5ed800f01327bf3fd1e33bc3039ec24948c104c2d876d751de6c6e3974ce446e` |
| neon3-distribution | `36cebf5bc7bb01458a2223524a3a5325bf5e13253f08568566f5edaefb2a9299` | `31b5b61e8b3dbda59166c8c0422cde399ae8f8c741738d95d2e46607adbfee04` |
| neon4-distribution | `b0d934955da56128e2010933d8eabc0346e378905b78e50609166fc6dc93c806` | `9a1bf1b3a30be0e4312013be7326ddfe5b3071b9fa64c8c266a18edcb6fe9c52` |
| sgp-pseudo-sprite | `ad9f9dcf349718e19077b2beb718ac8d6481f40e0f29b991973a7ffbb16faf9a` | `25ad559a7dccde9afad838c6cafaad01ac9a7cd5bba39e5978dc109739f672ca` |
| sgp-wireframe | `60b5690bd04367e1b16bc42f00c5f9983fcb52933e153470dfbc4d32f36cb071` | `dab2f6ff971f4c2cb9e27a767b23c5801babfb7a7c7294ae0ff21bc3c20e66c6` |
| zundamon-orbit | `c3ddf0395f72a670423b0500d8bbb2731110728fb40bb5618f994ce9c41578f3` | `abbc8d6a3d71b0d61801dd4fb1a4172524df84aa9c9d49964b1335892c3a5ac7` |
| all-demos | `dacaf718a1d8c6daff1958164d842b49bc6a69f43e587f706b85b0ed97626ae4` | `bc55d62f3fda857bf34ec3666c0f2ddec6e4980bb1f3fc9492ba11360a478b43` |
| all-demos-msdos4 | `e88c0e0332094048bcf55b334cd0430fc1c77e158344b00e57946ad9bbc84a78` | `67ddb64012bc89464cdaa2809c62a2a9c2460c52ecec7f815dc45a6193eef241` |
| sgp-boot | `67ead94dba0cee7aa9d9a0268cf1987bf3a2764fe1275e092b2e5ece06337154` | `657f91e25d9417b0b4fd9948d40caeec7f2daa6db6d2218b6f709be62b5238c5` |

The nonredistributable live boot overlay and private input identities are
bound locally, not published. Original inputs/archives remain unchanged.
Ten supplementary source-build entries also exit 0, including stock TSPSPR,
SGP contention, Z80 timing, both VA Voyage builds and all SGP scans.

- **34/34 live drawing cases PASS:** all 17 COM programs under each of PC-Engine
  and MS-DOS 4.0, using fresh generated disks, isolated INIs and pre/post frames.
  Existing source-derived scene/sphere24 contracts apply; no tolerance change.
- **MANDEL.BAS PASS_PROGRESS_CONTRACT:** unmodified source, `BASIC /87`, LOAD/RUN;
  expected first-row colours and 12→23 qualifying rows at frames 10000/16000.
- **Self-boot SGP PASS_BOOT_CONTRACT:** live native TSP menu at frame 900.
- Capture target is native macOS SDL dummy/software guest drawing, **not** Cocoa,
  Windows, final-client/layout, fullscreen, physical GPU or HiDPI acceptance.
- Original adapter failures remain: PNG argument type, initial missing MS-DOS
  program launch, wrong SGP655S profile and direct execution of a bash script.
  The corrected MS-DOS input schedule is rerun once; unchanged successful
  workers/captures/builds are reused and revalidated. The original full drawing
  and supplementary invocations exit 1; final reductions/recovery exit 0.
  Content selftest: six profiles/25 exact-code single-mutation negatives PASS.

The actual utility command is:

```sh
tools/pc88va/build-utility-disk.sh --source "$SYSTEM_01" --output "$UTILITY_FDD"
```

It exits **1**, `required host command is missing: dosbox`; no image is produced.
Host probing confirms DOSBox is absent. The recipe additionally requires the
Open Watcom container backend, not invoked under current M142 policy. Both
SASI generations and all utility boots are **NOT_RUN**, not PASS. Thus the
complete media gate remains **BLOCKED**, despite the demo subset passing.
Actual GUI/FPS, Windows execution and Windows/Linux state exchange remain
pending/blocked. #75 OPEN/correction NOT_IMPLEMENTED; **G142 NOT_READY**, no
human-gate request, merge, release or hosted CI.

## Clock-menu follow-up

The maintainer removes 2, 6, 16, 32 and 64 MHz menu entries and adds 20 MHz.
The numeric menu now contains exactly **4, 8, 10, 12 and 20 MHz**. Machine
default/custom controls, saved settings, resolver capability and peripheral/
FDC clocks are unchanged. The prior checkpoint's nine-choice results remain
historical, not relabeled as tests of this five-choice revision.

- Evaluated source: `30bceba24c102eb3ee60bdfcf90e0e4b93836b3c`.
- Native configure/build exit 0. `ctest --test-dir "$SCRATCH/native" -R
  '^vaeg_romless_tests$' --output-on-failure` exits 0 (1/1); the clock test
  checks all five choices including 20 MHz in both active clock domains.
- Native worker SHA-256:
  `8ee71b64e92175b4e965902b4f29bb7112d3318a949ec6f997524a57ca7358be`.
- `python3 tools/release/build-mingw-macports.py --commit
  30bceba24c102eb3ee60bdfcf90e0e4b93836b3c --work-dir
  "$SCRATCH/mingw-clock-menu" --sdk-dir "$SCRATCH/mingw-sdk" --jobs 4`
  exits 0, fresh clone/application objects with audited SDK reuse, no Colima.
  Recording/shaders ON, local encoders OFF; encoder archive, embedded source
  identity and system-DLL import checks PASS.
- Windows display: `Rel.20261030 (M142-dev, 30bceba2)`; output `vaeg.exe`
  SHA-256 `e82b049e95610e0da3ed4a046e9370b33ef693c1bf874ec067e544700fe46f57`.
  `BUILD-REPORT.md` is beside it in `mingw-clock-menu/output/` outside Git.
- The unaffected expensive CPU profiles are not rerun for this menu change;
  this is a focused test/build result, not a new full-suite or Windows-runtime
  claim. Other M142 blockers and G142 NOT_READY remain unchanged.

## Claude handoff

The maintainer requests [a two-part handoff](../reports/m142_claude_handoff.md):
continue the existing M142, especially unresolved #75, and replace host-side
BUPDATE patch execution with a small Python tool. No replacement is implemented
by this documentation checkpoint. WSP/DIET/JWasm DOSBox uses and Open Watcom
container backends remain separate dependencies; a BUPDATE-only replacement
must not be reported as complete utility-media regeneration or G142 readiness.

The maintainer subsequently defers BUPDATE work and prioritizes #75 cause
identification/correction. The [event-budget follow-up](../reports/m142_tvram_vb_events.md#event-budget-and-bus-wait-exclusion-follow-up)
reuses nine digest-checked Q/R/W traces: all three 128-byte extra-copy polls
finish with positive remaining scheduler budget (21/20/35 clocks), excluding
simple deadline-overrun stale VB as their explanation. Device waits already
reach the prefetch bus accounting through the memory wrapper; no missing-wait
fix is justified by that code path. No emulator change, guest rerun or timing
correction is claimed. #75 OPEN; G142 NOT_READY.

## Isolated DOSBox build follow-up

At the maintainer's request, DOSBox 0.74-3 is now built outside Git in the
specified local tools directory, with isolated SDL 2.32.10 and sdl12-compat
1.2.68. No system ports or containers are changed/used. The macOS arm64
console-entry build executes a DOS echo and the original BUPDATE PCPLUS
patch; exit 0 and the existing expected output SHA-256 both pass. The first
Cocoa startup timed out without a display; a separate console entry resolves
that startup dependency without modifying upstream sources. An unnecessary
forced autotools regeneration failed before an ordinary relink succeeded;
both failures are retained in the local build report/logs.

Executable SHA-256:
`7c6d49fe06292d14507133ad438b9a9591ab31563f4098698730abb37ac619a3`.
The local launcher confines HOME/configuration to the tools directory. It
requires its adjacent SDK libraries and is not a static executable or a
GUI-tested application. WSP/DIET/JWasm and complete utility regeneration are
NOT_RUN at this checkpoint. Open Watcom SQEMM98/VIEW480 container recipes
remain separate blockers. BUPDATE Python replacement stays deferred; #75
remains unresolved and G142 NOT_READY.
