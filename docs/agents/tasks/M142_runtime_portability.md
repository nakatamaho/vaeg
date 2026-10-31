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
- **D** Investigate [#75](https://github.com/nakatamaho/vaeg/issues/75).
  **Maintainer-approved scope revision: investigation complete for M142;
  cause identification and correction deferred.** Keep issue #75 OPEN and
  preserve other TSPSPR rows. Q/S observations (28.0 VA, 30.1 VA2, 41.7
  emulated VA2) are not a demonstrated root cause or permission for a fitted
  slowdown. The deferral below supersedes earlier mandatory-fix statements.
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

## Maintainer-approved #75 investigation closure

The maintainer explicitly directs: “#75は調査部分を完了扱い、原因特定・修正を延期で”.
This accepts the investigation deliverable only. Existing hardware comparisons,
Q/R/W controls, REP accounting and event-budget exclusions are retained;
no timing change, verified physical cause or reduced impact is claimed.
Issue #75 stays OPEN in the permanent bug-fix ledger, with cause identification
and correction deferred to separately assigned follow-up work. Do not start a
new milestone or request additional hardware measurements implicitly.

Under this revised scope, #75 correction is no longer an M142 gate prerequisite.
Earlier reports and the Claude handoff describing it as mandatory are historical
checkpoints, superseded by this decision rather than retroactively rewritten.
Other M142 obligations remain: utility-media generation/boots, actual GUI/FPS
and clock/settings checks, cross-OS state exchange, Windows execution and the
required human gate. **M142 IN_PROGRESS; G142 NOT_READY.** This is not gate
acceptance, issue closure, release authorization or approval to merge.

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

## DOS utility-tool execution follow-up

Using the isolated DOSBox, source `8140c2645dbc2400ef8c2dd3e4f5549d05004d4d`
now has the following **new, scoped local results**, retained under task
scratch `dos-tools-r1/`. These are prerequisite executions, not a utility
FDD/SASI generation or boot PASS. The #75 deferral above remains in force.

- The exact six BUPDATE/WSP commands from `build-utility-disk.sh` execute on
  copies of checksum-verified cached packages. All **ten** existing
  `verify_generated` output SHA-256 values match, including MSE, BMS,
  K-Launcher, LHA, PCPLUS and both TSCLVA files. No expected hash is changed.
- DIET runs `diet -b *.exe` and `diet -b -xc *.com` on a three-program subset
  (DIET.EXE, KLVA.EXE, KLL.COM). Every file has its expected processing-log
  entry and does not grow; compression actually reduces at least one file.
  Packed-program runtime and the complete FDD payload set are **NOT_RUN**.
- The unmodified `stage-softlib-additions.py --packages "$PACKAGES" --output
  "$OUT/softlib"` exits 0 and stages **22 files**. Its pinned DOS JWasm builds
  VA3DDEMO.EXE and PCODE.COM and verifies both existing output hashes. This is
  compilation/content coverage, not a VA3DDEMO rendering claim.
- The exact Open Watcom release archive pinned by `tools/openwatcom/containerfile`
  is downloaded over HTTPS and matches
  `e1bc4e88fa47191118f29e53731e7f4542803c7f4c503e15d72f1f571ac0832f`.
  Its **DOS `binw/wasm.exe` and `wlink.exe`** run under DOSBox; no container or
  system installation is used. The pinned SQEMM source is prepared with the
  existing `prepare-sqemm98.py`, then assembled/linked using the same arguments
  as the existing recipe. `check-sqemm98.py` passes, size **13253**, SHA-256
  **`eb3d443d7c12b6eb204e03a7ebac4b68a69d4690d0899ca93a37ad7a546d4930`**,
  exactly the existing guest-driver bundle expectation.
- The same DOS Open Watcom tools assemble/link VIEW480 from its hash-verified
  source archive: command exit 0, linker reports a DOS COM, size **368**,
  SHA-256 `7717775e1ad5d45c4cfebccc65be702b8c849878a566deb5a0bb08ead908862c`.
  This is a fresh build identity, not yet a comparison with an independently
  retained prior VIEW480 binary or a runtime PASS.

The local patch adapter's first attempt fails before DOS execution because
Python zipfile cannot decode a legacy ZIP compression method. It is corrected
to use `unzip`, as the production recipe already does. Original source/log/exit
are retained; the corrected attempt exits 0. No production archive decoder or
expected output is weakened to hide the failure.

A non-container Open Watcom route is therefore demonstrated for both required
sources, rather than merely proposed. **Integration into the production media
builders is still pending**: they still select the container backend. Next,
add an explicit, checked DOSBox backend with focused tests, then generate the
utility FDD and both SASI images from a clean pushed checkout and perform their
model-specific boot contracts. Do not substitute these successful standalone
builds for full media regression. G142 remains NOT_READY.

## Explicit DOS Open Watcom backend

The production `build-sqemm98.sh` and `build-view480.sh` now accept
`VAEG_OPENWATCOM_BACKEND=dosbox` and the explicit pinned release binding
`VAEG_OPENWATCOM_DOS_ARCHIVE`. Default container behavior is unchanged;
invalid backend/archive/compiler states never fall back to a container.
The shared Python runner verifies the complete archive, extracts only four
named DOS tool files, stages an isolated source copy and retains command,
source/tool/output identities and logs under TMPDIR. Both compiler stages
must pass DOS error-level/output checks, independently of process exit.

- Synthetic backend tests: **7 PASS**, including exact-code single mutations
  for digest, names, mount path, archive members, guest status and output
  presence. The composed passing fixture tests source preservation, output
  non-overwrite and the batch control-flow contract. Added CTest entry:
  `vaeg_openwatcom_dos_backend` (no DOSBox/archive required for unit tests).
- Actual production shell entry points produce SQEMM98 and VIEW480 matching
  the preceding independently retained output hashes. A single invalid-opcode
  insertion into the passing VIEW480 source returns **OWDOS_GUEST_STATUS**,
  exit 1, and creates no caller output. Original input remains unchanged.
- First integration attempt assembles/links but times out because directly
  invoking a BAT transfers AUTOEXEC control and skips the later exit command.
  The runner now uses **CALL VABUILD.BAT**; a fresh run of both entry points
  succeeds. Original timeout/log/evidence remain, not relabeled as success.

This changes host-side build tooling/test registration only, not emulator
behavior. No unchanged CPU corpus, vaeg application or Windows executable is
rebuilt for this tool change. Clean pushed-source utility-media generation
and guest boot contracts are the next checks; **G142 NOT_READY**.

## Utility-media RDBMS load-policy correction

Fresh source `3551f034e61c7780845c8117a624c4cf6ae51751` generates the
utility FDD and both SASI images successfully with the DOS backend. FDD DIET
validation covers **50 executables / 407,478 bytes saved**. The first SASI
attempt stops on **OFFLINE_MISS cpm_license**. A separate task-local CP/M
cache is seeded from shared input archives, the pinned notice is downloaded
and verified by the existing installer, and six CP/M-related disks are built
there (three are consumed by SASI). Its tools/work/licenses never write into the shared private cache.
The SASI retry supplies those three explicit images and succeeds. The original
failed attempt remains separate.

Live native checks reveal a new load-policy defect: on VA2 with BMS absent,
`DIR` becomes only `R`; a returned Ready alone is not a filesystem PASS.
Longer startup waits, split text, held keys, lowercase, disabled backup
persistence, EMS installation, removal of MSE switches and removal of MSE
itself do not correct that profile. BMS installation alone does. A controlled
CONFIG-only mutation removing **RDBMS autoload alone** passes the unchanged
boot directory/prompt contract with BMS absent; all other file payloads are
identical. A split-text attempt also omits the explicit `@enter` directive;
that caller error is retained, not treated as an independent guest defect.

The utility FDD and SASI default configs now omit RDBMS autoload, while
retaining the unchanged driver/manual payloads. The documented bank-memory
opt-in remains available after explicit BMS installation. This does not
change the default-off boards, the bank aperture, CPU clocks, keyboard/paste
implementation or issue #75. Two focused tests cover the FDD generated
configuration/retained files and eight SASI policy combinations. Clean
pushed-source regeneration and final default-off boot checks follow.

## Utility-media regression closure

The [utility-media record](../reports/m142_utility_media_regression.md) now
supersedes the earlier pending utility-build/boot checkpoints, without
relabeling their failed or NOT_RUN attempts. A fresh clone of pushed fix
`826eb848b37720aa6de1c89b1b40e56f296c1179` generates all three utility
images with the local DOS backend, exit 0. Recursive readback has 129 / 632 /
623 regular files; only CONFIG.SYS differs from the first generated FDD / VA
SASI / VA2 SASI payload maps. Driver/manual/packed program bytes remain
identical. Six CP/M prerequisite outputs and all three final utility-image
sizes/SHA-256 values, commands and failure/recovery details are in the record.

- **Four final live boot contracts PASS:** FDD on VA and VA2, VA SASI alone
  and VA2 SASI alone. Both boards remain absent. Explicit fresh per-case
  backup RAM, automatic main RAM and fixed-disk boot settings are bound.
  The unchanged validator checks actual directory content and Ready, not
  merely worker exit or a screenshot.
- Packed DIET 1.44 usage and nonredirected LHA member/size listing return
  successfully. Two redirected LHA queries still produce `no file`; these
  auxiliary semantic checks remain **FAIL/unclassified**, not fixed or
  upgraded by the nonredirected success. Not all 50 packed utilities execute.
- **24 actual guest attempts** across initial controls, final boots and
  packed checks; all process exits 0, semantic failures retained. Final
  identity/content recheck exits 0 with no new workers.
- Native worker remains `0d02bc849b5c4fb4f45d21c13716735e77578d37`; its
  application C/C++/headers are unchanged. Existing Windows artifact remains
  bound to `ea62eda1eb2d1d3abed11f72af2e5e7899868652`, not relabeled.
  No unnecessary application/Windows/CPU-corpus rebuild or hosted CI.

**M142 IN_PROGRESS / G142 NOT_READY.** Required utility build/boot coverage
is complete, but actual GUI/settings/FPS/presenter inspection, main/FDC clock
matrix and genuine Windows/Linux state exchange remain uncompleted or
blocked on the missing execution targets. The extra redirected-tool result
also remains unclassified. Issue #75 investigation is accepted; cause/fix
remain deferred, not an M142 correction prerequisite. No human-gate request,
acceptance, merge or release is authorized by this record.

## Save-portability audit and requested Windows rebuild

The maintainer requests a fresh MinGW executable for save checks. Clean
pushed source `721184268d42424de4a7abc0f5ac0bf4e5630ea2` is rebuilt with
`build-mingw-macports.py --commit 721184268d42424de4a7abc0f5ac0bf4e5630ea2
--work-dir "$WINDOWS_WORK" --sdk-dir "$SDK" --jobs 4`: exit 0. Only audited
SDK dependencies are reused; checkout/application objects are fresh. Display
is `Rel.20261031 (M142-dev, 72118426)`; executable SHA-256 is
`e2d1f45bdf82bc979ecd94858ba7b7d277c3f3bb9aa247716bba3e160bf332a9`.
Recording/shaders ON, bundled encoders OFF, archive/import/embedded-source
identity checks PASS. The artifact is not a Windows execution result.
MacPorts/release-identity focused tests pass (3 and 4); no source changes,
Colima or hosted CI are involved in the build request.

The Linux request identifies x86-64/Ubuntu glibc 2.43, but the maintainer
then pauses build provisioning to ask what is actually broken, including
macOS. The [save audit](../reports/m142_save_portability_audit.md) separates
the demonstrated old DISK widths from the corrected M142 format and genuine
cross-OS NOT_RUN scope. No current failing foreign-producer save has been
supplied; lack of a Linux worker is not itself a save-format failure.
The OS-specific cfg proposal is withdrawn; no implementation/task M143 is
started and existing config selection remains unchanged.

New out-of-tree C/object probes confirm Windows/macOS old widths and find
zero differences across 63 compiled state-table entries (name/version/kind/
size), not universal payload-layout equivalence. Six fresh native macOS
workers explicitly load an earlier file in a new process: four v1/legacy
layout cases restore the expected guest registers/TVRAM/GVRAM and local
medium; an induced missing-medium case restores CPU/video but logs a warning
and leaves FDD1 empty; one version-only mutation rejects with exact
`M142_DISK_VERSION` (expected exit 1). All six meet their scoped expectations,
not six complete-restoration PASSes. Original inputs/worker remain unchanged.
The earlier fresh-process checks covered config/expansion persistence, not
loading the previous snapshot; only the new cases add that explicit load.

Evidence remains task-local as `SAVE_AUDIT_01`; scripts/logs/exits and private
snapshots are untracked. Native worker remains source
`0d02bc849b5c4fb4f45d21c13716735e77578d37`, identity-bound reuse with no
application-source changes. This is not a foreign Windows/Linux producer,
Cocoa/GUI acceptance or a new emulator fix. GUI warning/status and slot/root
selection findings are source analysis, not an invented current cross-OS
failure. **M142 IN_PROGRESS / G142 NOT_READY** remains unchanged.

### Current BMS restore defect reproduced after the layout audit

**Historical investigation checkpoint:** the subsequent authorized correction
below supersedes the OPEN/NOT_IMPLEMENTED status, not these observations.

Two further native workers use the retained enabled-expansion snapshot with
a known synthetic marker in its 16 MiB BMS payload. The passing fixture starts
with matching BMS ON. Changing only `Use_BMS_` to OFF still returns success
and reaches the expected guest PC, but a new save has no BMSMEM/marker while
BMSIO still declares the enabled saved device. Original input/worker/media
remain unchanged. The [current defect and demonstrated cause](../reports/m142_save_portability_audit.md#current-bms-restore-defect-reproduced-and-unfixed)
are recorded in the [permanent ledger](../../modernization/bug-fixes.md#m142--bms-snapshot-ram-is-skipped-when-the-reader-starts-with-bms-disabled).

The loader allocates/resets the working bank RAM before reading the saved
BMSIO configuration; BMSMEM is earlier in the state table and silently skipped
when that buffer is NULL. This is a current configuration-transition/data-loss
defect reproduced within macOS, not merely an unexecuted cross-OS case or
proof of the cause of a specific unsupplied Windows/Linux failure. Both worker
exits are 0, but the OFF result is a reproduced defect, not a restoration PASS.
The matching-ON control is a workaround for this case, not a correction.
No bank-dependent guest access/crash or all-bank-count claim is made.

Evidence is private `BMS_LOAD_AUDIT_01`; total new workers across both audits
are eight. No emulator change is made because this follow-up is investigation.
At that checkpoint the BMS defect was OPEN/NOT_IMPLEMENTED. No default-board
reversal, gate acceptance or merge was inferred.

## Authorized BMS and saved-media restoration correction

The maintainer authorizes the demonstrated BMS/media corrections and explicitly
waives old-SAV compatibility. The new `0x261031` container, mandatory BMSSTATE
v1 and 13-slot/content-bound DISK v2 are implemented. Saved BMS device/RAM is
restored independently of next-reset preferences; required media mismatches
are errors, not successful partial loads. Checked GUI/debug results, genuine
read-only input, atomic SAV replacement and guarded quick undo are included.
Old SAV files are intentionally rejected; original files must be preserved.

Evaluated corrected source:
**`0f3cbe20190ccca6524dda59301303583f535c0a`**. Native display:
`Rel.20261031 (M142-dev, 0f3cbe20)`; worker SHA-256:
`380c8199441e63b707a2edf735e43fc3e9b8413a64e890ab82d825b8e19200eb`.
Fresh complete CTest: **155 PASS / 16 SKIP / 0 FAIL** (171 entries), exit 0.
The [corrected evidence and fixing commits](../reports/m142_save_portability_audit.md#authorized-restoration-correction-and-verification)
record seven BMS transitions, exact-code negatives, complete disk-slot tests,
read-only before/after evidence, atomic-save protection and the explicit skips.
The permanent ledger moves BMS out of Open Defects and records the other fixes.

Exact final contracts, with explicit private/out-of-tree bindings:

```sh
cmake -S "$SOURCE" -B "$NATIVE_BUILD" -DVAEG_ENABLE_LOCAL_MP4_ENCODERS=OFF
cmake --build "$NATIVE_BUILD" -j4
ctest --test-dir "$NATIVE_BUILD" -j4 --output-on-failure
python3 tools/qa/runtime_state_roundtrip.py --worker "$WORKER" --roms "$ROMS" \
  --kit "$KIT" --evaluated-commit 0f3cbe20190ccca6524dda59301303583f535c0a \
  --output-dir "$PRODUCER_OUTPUT"
python3 tools/qa/state_restore_matrix.py --worker "$WORKER" --roms "$ROMS" \
  --producer-output "$PRODUCER_OUTPUT" --output-dir "$PRIOR_LOAD_OUTPUT"
```

All exit **0**. Build also explicitly keeps recording/tests/static FFmpeg/public
policy/trace ON and librashader OFF. The first runner's three config restarts
still do not load the prior file. The new second runner does: seven actual
fresh-process loads preserve complete BMS payload/re-save bytes, independently
retained guest continuation and resolved disk identity, including reader OFF,
smaller/opposite-port settings and saved OFF into reader ON. Two controlled
mutations reject exactly `M142_DISK_MISSING` and `M142_DISK_IDENTITY`. Total:
18 new workers across both runners, 14 ordinary successful exits and four
expected guard rejections, not 18 restoration PASSes. Private inputs and the
frozen corrected worker are retained unchanged under `STATE_FIX_NATIVE_01`.

The old `72118426` Windows artifact remains an unfixed historical build; the
corrected-source fresh handoff is recorded below. No Windows/Linux execution,
Cocoa/window/HiDPI check or genuine cross-OS exchange is inferred from the
native SDL-dummy checks. Existing media-generation results retain their original
source/worker identities. The withdrawn cfg proposal and approved #75 deferral
remain unchanged. **M142 IN_PROGRESS / G142 NOT_READY**.

### Corrected-source fresh Windows handoff

The tested correction chain is pushed before the clean build. Exact command:

```sh
python3 tools/release/build-mingw-macports.py \
  --commit 0f3cbe20190ccca6524dda59301303583f535c0a \
  --work-dir "$WINDOWS_WORK" --sdk-dir "$SDK" --jobs 4
```

Exit **0**; new clean source/application objects, audited dependency SDK reuse
only. Recording/shaders ON, bundled encoders OFF. Archive/import/embedded full
source and display, copied executable equality and clean-source checks PASS.
Display `Rel.20261031 (M142-dev, 0f3cbe20)`; output `vaeg.exe`, **73318779**
bytes; SHA-256
**`9d413f8bdd3902fe8942c5dd89bd7201f3a708c7529bd55573a4f3f79d031a92`**.
`BUILD-REPORT.md`, `build.json`, import/policy logs and `third-party-notices.txt`
remain beside the executable. Transfer the notices too, not an SDK/DLL bundle.

[The handoff evidence](../reports/m142_save_portability_audit.md#corrected-source-windows-clean-build-handoff)
also confirms all 62 current macOS/Windows compiled state-table entries match;
this remains table ABI only, not complete payload or runtime portability.
Windows execution and the requested Linux build/exchange are NOT_RUN. This
replaces the old artifact for correction testing; it does not accept G142.

## PCG and VA N-mode startup defaults

The maintainer requests PCG and “N モード（88VA/VA2）” enabled by default.
Change only the startup initializers for `PCG` and `V1V2_NModeEnable` to ON.
Missing configuration files/keys retain these defaults; explicit persisted
OFF remains OFF. Existing installations with either key explicitly false
must opt in through the GUI or change that key to true; no private cfg is
silently rewritten. Boot selection stays 自動 (`BootMode=0`), and a separate
user-supplied N-BASIC ROM is still required. EMS/BMS defaults, PCG volume,
ROM payloads, the SAV format and actual PCG/N-mode device implementations
are unchanged. This is a requested policy change, not a newly claimed bug fix;
the accepted historical M137 default-OFF record remains historical.

The real startup initializer assertion fails on the old-default build with
`M142_COMPAT_FEATURE_DEFAULTS` (focused CTest exit 8), then passes after the
two defaults change. Production `initload`/`initsave` coverage checks a missing
file, absent keys and all four explicit PCG/N-mode ON/OFF combinations, restoring
both preferences afterward. Existing synthetic PCG, N-BASIC ROM presence/OFF
and absent-ROM tests also pass inside the complete ROM-less selftest.

```sh
cmake -S "$SOURCE" -B "$NATIVE_BUILD" -DVAEG_ENABLE_LOCAL_MP4_ENCODERS=OFF
cmake --build "$NATIVE_BUILD" -j4
ctest --test-dir "$NATIVE_BUILD" \
  -R '^vaeg_(romless_tests|state_storage|runtime_state_contract|state_restore_matrix_contract)$' \
  --output-on-failure
```

Configure/build exit 0; focused **4/4 PASS**, exit 0. Recording/static FFmpeg
remain ON and bundled encoders OFF. Private `COMPAT_DEFAULTS_01` evidence
retains source/patch and final worker identities, plus before/after commands,
logs and exits (the initial red build's binary digest was not captured).
No unchanged expensive CPU corpus or media-generation campaign is repeated;
prior results retain their actual evaluated identities, not this follow-up's.
The earlier `0f3cbe20` executable does not contain these requested defaults.
Actual window/presenter, Windows runtime and OS-to-OS exchange are not inferred.
**G142 NOT_READY**.

Final evaluated/pushed source:
**`666c7746b6f34f617f22ce4ce1f35623b3173875`**. Reconfigured native display
`Rel.20261031 (M142-dev, 666c7746)`; frozen worker SHA-256:
`120c83e726b1d57a967821c5a695935ed3df1bd2b5dfc3e5ab36d2f5f7478466`.
The same exact focused CTest command above again passes **4/4**, exit 0,
37.21 seconds, on this identity-correct binary. Whole-tree format retains
659 inherited diagnostics; none intersects this follow-up's changed lines.
Encoding/EOL/case/Markdown/whitespace, 49 milestone selftests and discovery/
audit pass; MacPorts builder/release-identity unit tests pass 3 and 4.

A fresh clean clone/application-object Windows build of that pushed source:

```sh
python3 tools/release/build-mingw-macports.py \
  --commit 666c7746b6f34f617f22ce4ce1f35623b3173875 \
  --work-dir "$WINDOWS_WORK" --sdk-dir "$SDK" --jobs 4
```

Exit **0**. Recording/shaders ON, bundled encoders OFF; audited isolated SDK
reuse only, no container/Colima/global toolchain changes. Static archive,
system-DLL import, embedded source/display, output-copy equality and clean
source checks PASS. Display `Rel.20261031 (M142-dev, 666c7746)`; executable
SHA-256:
**`79b881a5af84a622d0b7a947e3115ebe78f4db7dce3b6d999a2bed236b3618b0`**.
`vaeg.exe`, `third-party-notices.txt`, `BUILD-REPORT.md` and build/policy/import
identities are retained together outside Git. Historical handoffs and private
inputs are preserved. This is a new handoff, not Windows execution or G142
acceptance.
