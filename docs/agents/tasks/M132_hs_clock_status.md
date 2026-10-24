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

# M132 — H/S clock, identification and machine status

Status: **Agreed implementation and static Windows artifact added;
PSD-source closure and interactive GUI verification remain pending.
G132 not requested or passed.**

Branch: `milestone/m132-hs-clock-status`.
Baseline: `49cb0112a1ffaaf26f4bf1830d1df2a89b93746d` (accepted G131).

## Authorization and contract

The maintainer requested implementing PR #55 with the agreed redesign,
updating documentation, moving the existing front panel into 情報 and adding
an independently persistent, default-ON 機種・動作情報 display. PR #56 is excluded.
Canonical specification:
[v1v2-hs-clock-status-plan.md](../../modernization/v1v2-hs-clock-status-plan.md).
Reviewed PR head: `1091af8b260139a69d16d7afdb36883678655589`.
The maintainer subsequently requested accepting the actual PR source/history
rather than independently rewriting it. Merge the original PR commit into this
milestone branch, resolve obsolete IN31 assumptions in favor of M131, then make
the agreed corrections in separate commits. Preserve original authorship and
history. Preset removal in the incoming proposal is to be corrected according
to the agreed design, not silently treated as a new UI decision.

## Implementation sequence (code and documentation added)

1. Record approved design and explicit N80/N80SR backlog.
2. Correct VA/VA2 H/S identification/reset synchronization, preserving checksums
   and unrelated bits; test valid/missing/invalid backup and first boot policy.
3. Apply S timing only in the defined compatibility scope; separate instruction
   cost from already charged native waits; retain FDC and native V3 timing.
4. Introduce a shared frontend machine/mode/status model with nominal clocks,
   short English formatting, testable identity/mode rules and N-family lamps.
5. Integrate default-ON bottom status, tooltips, independent front-panel toggle,
   Information menu placement and persistent settings; remove duplicate title data.
6. Update mode/clock/frontend documentation and correctness ledger with evidence.

## Verification and gate

- Focused red/green tests for identification, clock scaling/waits, V3 isolation,
  profile/mode labels, N lamps, formatter, configuration defaults/persistence.
- Full applicable CMake build and CTest; check_encoding.py, check_eol.py,
  check_case.py, check_md_comments.py, changed-line clang-format 22 and diff checks.
  Full-format baseline debt recorded at G131 is not silently called a pass.
- Preserve evaluated commit, executable digest, corpus/contract and output digests
  for expensive runs; do not rerun unchanged application behavior for docs only.
- Clean-clone full demo build/payload closure, all-demos drawing under both OSes,
  BASIC Mandelbrot, selfboot SGP, utility FDD and both SASI model boots/DIR.
  Record exact final commands/status and generated image SHA-256 values here.
- Fresh static Windows handoff via build-mingw-static-container.sh, explicitly
  local encoders OFF, project-local Docker storage/process-local daemon selection.
- G132 HUMAN: standard V3/demo/OS operations, H/S and IN6Eh behavior, PC-8801 and
  PC-8001 identities/modes, N-family lamps OFF, default/status/front-panel toggles,
  short labels/tooltips, reset/restart/state restore and M131 fixture regression.

No real-hardware timing calibration is inferred from arithmetic selftests,
nominal clock labels, or acceptable demo speed. Issues #58/#59 retain any
unverified hardware/integration work. No next milestone, merge or cleanup until
explicit maintainer gate acceptance.

## Work log

- Design/task created before production changes. Private work uses
  `~/work/pc88va/m132-private/`; private reference assets are read-only.
- On the request to accept the actual PR source, the initial uncommitted
  six-file independent patch was preserved privately and removed from the
  working tree before integration. The original PR commit is merged with its
  authorship/history intact. Only conflict resolution and M131-related wording
  are adjusted at acceptance; original-VA/first-boot fixes, timing scope,
  native-wait accounting and status redesign remain subsequent work.
- The two integration conflicts are the IN31 comment and machine-mode
  documentation. IN31 still reads B1FC6h unchanged; no RAM/display change is
  accepted from the proposal. This intermediate integration is not G132-ready.

### Original PR source acceptance baseline

Original PR commit
[1091af8b](https://github.com/nakatamaho/vaeg/commit/1091af8b260139a69d16d7afdb36883678655589)
is a direct parent of acceptance merge
[c13f1288](https://github.com/nakatamaho/vaeg/commit/c13f12888718d08b81b249e7c57c26a56bd9f029).
It is not a rewritten or independently recreated patch. Main remains unchanged.

```sh
cmake -S "$SOURCE" -B "$PRIVATE/build" -G Ninja \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_PREFIX_PATH=/opt/local \
  -DVAEG_ENABLE_RECORDING=ON -DVAEG_STATIC_FFMPEG=OFF \
  -DVAEG_ENABLE_LOCAL_MP4_ENCODERS=OFF -DVAEG_STATIC_LIBRASHADER=OFF \
  -DVAEG_FETCH_SDL2=ON -DVAEG_FETCH_LIBARCHIVE=ON \
  -DVAEG_ENABLE_TESTS=ON -DVAEG_Z80_COMPAT_INTEGRATION_TRACE=ON
cmake -S "$SOURCE" -B "$PRIVATE/build"
cmake --build "$PRIVATE/build" -j 8
ctest --test-dir "$PRIVATE/build" --output-on-failure -j 4
ctest --test-dir "$PRIVATE/build" -R '^vaeg_milestone_id_selftest$' \
  --output-on-failure
```

Configure/build exit 0 (1293 build steps). The macOS recording-ON worker embeds
c13f1288 and has SHA-256
`7f5b3ddd35236dee7c546853fa6f33c19804950314240cb5a2e1bb348a26cad5`.
The first full CTest run lists 140 tests, 130 pass, nine skip, one fails
(exit 8, 99.44 seconds). The sole failure is the new roadmap row mentioning
the predecessor gate before its own gate: `ROADMAP:190: gate ID mismatch`.
The row is corrected without changing the validator; the focused recheck
passes (exit 0). Emulator/romless tests already passed, so no unchanged
application tests are rerun for this documentation correction. This records
full-run plus focused-recheck results, not a fabricated second full run.

The nine skips match the G131 baseline (external SST/probes, ROM-dependent
BASIC boot, five xz tests). Encoding/EOL/case/Markdown/diff checks pass.
Exact logs and commands are private. No hosted CI result, guest-media acceptance,
Windows handoff, ROMful IN6Eh integration proof or G132 pass is claimed by
this historical source-acceptance baseline. The incoming
proposal's reset/timing/UI gaps are corrected by the subsequent commits below.

## Implemented corrections and frontend

| Concern | Commit |
|---|---|
| Original VA/VA2 effective H/S input, first-boot mode output, reset/bind matrix | [6b12b849](https://github.com/nakatamaho/vaeg/commit/6b12b8494dadfea994ae95860636d02b66a02d54) |
| 88-only S instruction conversion, native-wait routing, live/restored clock | [576657dc](https://github.com/nakatamaho/vaeg/commit/576657dce046d9eb2d317caaf5c13e5a19098f1a) |
| Canonical profile identity, shared N/V1/V2/V3 model, N-family lamps | [677b57ca](https://github.com/nakatamaho/vaeg/commit/677b57ca4655d5db7a007049b784b0001b859111) |
| Default-ON independent footer, moved lamp setting, tooltip and geometry/configuration tests | [e75f8fb6](https://github.com/nakatamaho/vaeg/commit/e75f8fb626b8513c29690a6a890fe697c8b678bd) |
| Firmware-created original-VA record reconciled after reset on 88 entry | [46f6c75c](https://github.com/nakatamaho/vaeg/commit/46f6c75c20505e95fb74bdd3ed03e6d3b42f19b0) |
| Footer survives screenshot menu suppression, uses final render geometry | [5a431e36](https://github.com/nakatamaho/vaeg/commit/5a431e36e9994e1dda93de61b8828c6e21dc415d) |
| Real uPD70008 instruction/I/O accounting integration tests | [13ebbb1a](https://github.com/nakatamaho/vaeg/commit/13ebbb1a3272a9061f697adcd4a32797e124fddd) |

Correctness entries and demonstrated causes are in the
[permanent ledger](../../modernization/bug-fixes.md). The incoming PR's
historical CPU presets are retained; 50% is now explicitly an additional
adjustment to H/S. Custom settings are never guessed away.

### Focused red/green evidence

Commands use the same recording-ON, local-encoders-OFF macOS build described
above. Exact stdout/stderr remain private; no ROM inputs are needed here.

```sh
cmake --build "$PRIVATE/build" -j 8
ctest --test-dir "$PRIVATE/build" -R '^vaeg_romless_tests$' --output-on-failure
ctest --test-dir "$PRIVATE/build" -R '^vaeg_upd9002_brkem_upd70008$' --output-on-failure
```

- Identification matrix fails on the imported source with
  `VA reset retained stale H/S or damaged unrelated input bits` (exit 8).
  After correction, the suite passes (exit 0). It covers VA/VA2,
  auto/manual RAM, absent/valid/bad signatures, V1/V2, H→S→H, VA2 first-boot
  mode output, checksum/unrelated-bit preservation and restored-latch binding.
- A separate firmware-created-record assertion fails before the 88-entry
  follow-up with `original-VA first boot left the new BIOS record at the wrong H/S`
  (exit 8); the follow-up suite passes (exit 0). V3 BRKEM must not apply or
  consume that synchronization, and invalid RAM is not manufactured.
- V3-isolation assertion fails on the incoming ratio with
  `saved S selection slowed native V3 BRKEM code` (exit 8). Corrected matrices
  pass multiplier/percentage combinations, nominal clocks, native-wait identity
  and 30,000 fractional slices with interleaved waits in each domain (exit 0).
- A passing profile fixture fails after changing only the effective lamp getter
  back to raw-latch behavior (exit 8); the shared classifier passes. Formal
  machine names, no non-VA SGP, short rounding/sub-MHz/overflow, truncation,
  runtime N-ROM versus N boot preference, and explicit ambiguous subtypes are
  checked. ROM banner strings are not runtime evidence.
- Production configuration round-trips all four status/panel visibility
  combinations and per-field flags. A legacy key-less fixture retains the
  ON startup default and existing panel/field choices. Pure footer geometry
  covers 1x/2x DPI, tall fonts/narrow sizes, independent toggles and bounds;
  all four toggles preserve the 640×400 guest area at 2x DPI.
- Actual adapter execution, ten instructions per probe, passes:

  | Probe | H | S | S + 50% | V3 BRKEM with saved S |
  |---|---:|---:|---:|---:|
  | NOP | 40 | 80 | 160 | 40 |
  | IN A,(7Eh) | 130 | 240 | 460 | 130 |
  | OUT (7Eh),A | 130 | 240 | 460 | 130 |

  The I/O fixture explicitly builds/binds the standard machine bus (20 native
  clocks for ten accesses). Initial unbound-fixture failures are retained as
  setup failures, not production defects. Starting from the passing fixture,
  restoring only the old I/O wait route produces H=130, S=260, S50=520 and
  V3S=130 and fails the intended native-wait assertion (exit 8). Restoring the
  native-unit route passes (exit 0). No FDC or native V30 timing is changed.

These checks demonstrate emulator accounting, not real-hardware calibration.
Remaining whole-tree, clean-media, runtime and handoff results are recorded
below once actually completed; no G132 acceptance is implied.

## Code-freeze local verification

Evaluated commit:
[`338174ee17daa885f038b3742dd0f1ed0560bfc5`](https://github.com/nakatamaho/vaeg/commit/338174ee17daa885f038b3742dd0f1ed0560bfc5).
The macOS worker embeds that commit, has recording ON and local bundled
MP4 encoders OFF. SHA-256:
`f905e238e9417a5abe0cc421c3018823823154b44ec7a429ba7b99cc6b8b5f17`.

```sh
cmake -S . -B "$PRIVATE/build"
cmake --build "$PRIVATE/build" -j 8
ctest --test-dir "$PRIVATE/build" --output-on-failure -j 4
python3 tools/repo/check_encoding.py --expect utf8
python3 tools/repo/check_eol.py --enforce
python3 tools/repo/check_case.py
python3 tools/repo/check_md_comments.py
python3 tools/qa/milestone_ids.py --selftest --audit --discover
git diff 49cb0112 --check
```

All exit 0. CTest lists 140 tests: 131 pass, nine skip, zero fail; 91.08s.
The skips are unchanged from the source-acceptance baseline: SST corpus,
external uPD70008/uPD780 probes, private BASIC boot input and the five
LibArchive upstream environment/generated-input cases. This is not a run
of those missing private/corpus tests.
Milestone-ID audit/selftest passes 49 strict cases; discovery reports
211 tasks, 163 reports and 152 roadmap rows. Encoding/EOL each report zero
violations; case/Markdown each report zero findings.

An earlier full preflight of `96ebc20d713fbc3da98dd84246a0a9af0e05a00e`
also passed 140 listed tests with the same skips (87.27s); its worker SHA-256
was `73e9313d01b679742977998a6cb21035ffe42b6ae08ba2bbe92d3b92748523e6`.
The subsequent source change is two initializer-alignment spaces in a test.
The final code-freeze run above evaluates the final formatted source and
restored native-I/O route, not the temporary negative-control binary.

Clang-format 22 checks 978 changed manifest-covered lines against accepted
main with zero intersecting replacements. The separate full-tree formatter
continues to report the same 26-file baseline debt as M131 (exit 1); this
is **not** claimed as a global formatting pass and is not repaired by
reformatting unrelated lines. Original PR ancestry is retained and verified.

### Remaining gate work and CI staging

The required PSD source has not been located in the available input locations;
its source rebuild/payload check is blocked pending the maintainer's input.
A committed atlas or disk is not substituted and described as a fresh PSD build.
The completed partial clean-media, headless runtime and Windows handoff
checks are recorded below. Interactive GUI inspection is also pending because
the current execution environment supplies no Cocoa display.
No G132 request, pass, main merge or cleanup is authorized by these unit results.

The code-freeze checkpoint is pushed with hosted CI explicitly skipped while
private media/runtime/handoff work is incomplete. It makes the exact code
commit available for clean-clone checks without using hosted CI as a debugger.
One final hosted review-candidate run is deferred until applicable local checks
and the evidence layout are settled.

## Media regression portability correction

The clean-media run found a real portability defect, not an archive checksum
mismatch. Under Lhasa on a Linux bind mount backed by a case-insensitive host,
CP/M installation rejected a lowercase member's uppercase lookup as a duplicate.
The directory contained one entry; both spellings existed and had the same
inode. A direct case-only rename also returned success without changing the
stored name. Originals and cached archives were not changed.

[Fix 360eb71c](https://github.com/nakatamaho/vaeg/commit/360eb71c420e1e27b07adaa65a20b0d6516b9161)
retains rejection of distinct entries/inodes and stages real aliases through
an intermediate sibling. The [permanent ledger](../../modernization/bug-fixes.md)
records the demonstrated cause. Focused commands:

```sh
python3 tools/cpmva/tests/test_install_cpmva.py
cmake -S . -B "$PRIVATE/build"
ctest --test-dir "$PRIVATE/build" -R '^vaeg_cpmva_installer$' --output-on-failure
```

Exit 0: 31 Python cases and the newly registered CTest pass. The alias positive
fails on the old guard with `ARCHIVE_DUPLICATE`; removing only staged renaming
fails its stored-name assertion. Collision negatives first prove their passing
baseline, change only the directory-entry list or inode identity, then require
exactly `ARCHIVE_DUPLICATE`. No Git/history or large corpus gates mask these tests.

CTest now registers 141 cases. The prior full 140-case emulator result is
retained, not described as a new full 141-case run. Since the application-code
freeze, only this media tool, its tests, documentation and a pure `add_test`
registration have changed; no application compilation/link flags or sources
changed. The evaluated emulator remains commit 338174ee and its worker remains
`f905e238e9417a5abe0cc421c3018823823154b44ec7a429ba7b99cc6b8b5f17`.
Unchanged expensive emulator evaluations are not rerun for this tool-only fix.

## Clean static Windows artifact

```sh
vaeg-container tools/release/build-mingw-static-container.sh \
  --commit 338174ee17daa885f038b3742dd0f1ed0560bfc5
```

Exit 0, from a new clean clone of the pushed application-code freeze. The pinned
Ubuntu 24.04 container, Rust 1.88.0, `mingw-cross` Release, fresh librashader SDK
and fresh application objects complete. Embedded commit/date and system-DLL-only
PE imports are audited. Recording and local bundled H.264/AAC encoders are OFF;
this is the public clean-build handoff, not a personal recording-enabled build.

- Executable: `../vaeg-mingw-338174ee/output/vaeg.exe`, 57,626,801 bytes.
- SHA-256: `6d18e4f173884a4dbbe029e58e0ee86576c15a7c2e5f081556c0151fbcb6584a`.
- Report: `../vaeg-mingw-338174ee/output/BUILD-REPORT.md`.
- The script compiles/links and audits imports; it does not execute Windows
  guests. No Windows execution or G132 acceptance is inferred.
- Project-local Colima VM/daemon storage is used with process-local connection
  settings. The global Docker context is not changed.

## Partial clean-media regression

All generated images remain outside Git. Components/extras/utility use a clean
clone of pushed commit 338174ee; the corrected SASI stage uses a separate clean
clone of pushed commit 360eb71c. The tool-only correction does not change the
already evaluated component/utility inputs, so those completed stages are reused.
Linux uses Ubuntu 24.04, Python 3.12.3 and NASM 2.16.01. Input originals remain
unchanged; only scratch copies have their D88 write-protection byte cleared.
The shared caches are checksum-checked by the builders.

```sh
for spec in glass-orbit:glass-orbit neon3:neon3-distribution \
  neon4:neon4-distribution sgp-pseudo-sprite:sgp-pseudo-sprite \
  sgp-wireframe:sgp-wireframe; do
  demos/${spec%%:*}/build-d88.sh "$DEMO_TEMPLATE" "$OUT/m132-${spec#*:}.d88"
done
python3 demos/sgp-boot/build-boot-d88.py --output "$OUT/sgp-boot.d88" \
  --compressed-output "$DIST/sgp-boot.d88.xz"
demos/sgp-scan/65536/build.sh "$OUT/SGP65536.COM"
demos/sgp-contention/build.sh "$OUT/sgp-contention"
demos/va-voyage/build.sh "$OUT/va-voyage"
bash demos/va-voyage/build-opna.sh "$OUT/va-voyage-opna"
tools/pc88va/build-utility-disk.sh --source "$SYSTEM_VA2" --output "$OUT/utility.d88"
tools/pc88va/build-sasi-utility-disks.sh --source-va "$SYSTEM_VA" \
  --source-va2 "$SYSTEM_VA2" --payload-d88 "$OUT/utility.d88" --output-dir "$OUT/sasi"
python3 "$PRIVATE/compare-components.py"
```

Final listed stages exit 0. The initial SASI stage exits 1 on the case-alias
bug above; its exact failure is retained rather than relabeled a pass.
Five fresh distributions contain 16 payloads, all byte-identical to the
committed FAT file maps. Six compressed outputs round-trip exactly, and
self-booting SGP matches its committed raw D88 exactly. No replacement binary
is staged. Component labels below omit the scratch `m132-` filename prefix.

| Produced raw image / neutral role | SHA-256 |
|---|---|
| glass-orbit.d88 | `10c9900b67b3948052ed91965e728e29c655e51275d719fcd8d4cde3f34d325b` |
| neon3-distribution.d88 | `67aae4df9bca3f9eb71e6794f9fef3b5c01d463671113322f4bed6ffc8ee0b85` |
| neon4-distribution.d88 | `e629ccfc3ff10c62a103f9ac2ba62f5cc2635bb45be0a2a1f72984478ba2a220` |
| sgp-pseudo-sprite.d88 | `913e7478767adf36924170c943052f15ecad7a5907c7579d3d9a8b0b4ba59449` |
| sgp-wireframe.d88 | `17da016bd716de0e7d51191e91f1261e4686634845910895584af22a99b87ad6` |
| sgp-boot.d88 | `67ead94dba0cee7aa9d9a0268cf1987bf3a2764fe1275e092b2e5ece06337154` |
| utility-fdd-01 | `cf5b38eed92064326f7fc43eecec127dca9e1a7be82f76c126093c2048d5de5a` |
| utility-sasi-va-01 | `fa5a794eb617919683ca02d03561c0ca0c0c2c9db0ad546f12a78781072def22` |
| utility-sasi-va2-01 | `b1aeb0c8d3f1da98307df7a4881894526cbb4ed67a3ae687fb24b57bacd7f4fb` |

| Produced compressed distribution | SHA-256 |
|---|---|
| glass-orbit.d88.xz | `71b286ba5b033dd74d81dfee06db3df6cb261df8451081a76a22cf4821055efe` |
| neon3-distribution.d88.xz | `72b89c13cea76a1664bdac7451d1d65e93429cbd2502ad494c55fc64e7924c3e` |
| neon4-distribution.d88.xz | `50e301cc3d5f8e4be86f1f008bf2f36825126d532d2799c922355b1e539f87a0` |
| sgp-pseudo-sprite.d88.xz | `0fa03de4b17eabb615f75891bbcebe62232cbb2db07808f247c57b5db525e40c` |
| sgp-wireframe.d88.xz | `38dec15498f7a618dabef27485b8ad4e6a72132a395126ba7edbcbd75503977b` |
| sgp-boot.d88.xz | `657f91e25d9417b0b4fd9948d40caeec7f2daa6db6d2218b6f709be62b5238c5` |

**Blocked, not passed:** the required original PSD has not been located,
including a metadata-only search of available ZIP inputs. Zundamon's fresh
source rebuild and therefore fresh all-demos/all-demos-msdos4/PC-Engine
aggregate closure remain pending. Existing committed artwork/disk bytes are
not presented as a fresh source build. This blocks a G132 request.

## Headless guest runtime inspection

All cases execute the unchanged 338174ee worker with the recorded SHA-256.
Exact commands/configurations, input/corpus identities, comparison contracts,
exit status, output digests and inspection records remain private.

```sh
python3 "$PRIVATE/run-runtime-extras.py"
python3 "$PRIVATE/run-all-demos.py"
python3 "$PRIVATE/run-basic-flags.py"
```

Process exit is not used as a drawing assertion: the captured screens and
contact sheets are inspected. Follow-up commands and unsuccessful trials are
retained separately.

- The fresh utility FDD boots on VA2 and accepts DIR. Each fresh SASI disk
  boots **alone** on its intended model and accepts DIR after the firmware's
  backup record has been initialized and reset. Initial empty-record boots
  do not yet apply the signature-gated fixed-disk preference and are retained
  as unsuccessful trials, not successful standalone boots.
- Fresh self-booting SGP reaches the menu and A displays the sprite scene.
- All 17 bundled executables draw under PC-Engine and MS-DOS 4.0: 34 inspected
  program/OS combinations. Normal captures use frame 4200; Zundamon extends
  to frame 16000 and is visibly drawing. Bare `ZUNDAORB` is used under PC-Engine.
  An initial MS-DOS NEON4/16 trial remains at the changed-directory prompt;
  a targeted follow-up increasing the post-CD wait from 60 to 600 frames
  starts it and draws. This is not counted as an emulator correction.
- Runtime uses **committed reference distributions**, not a claim that the
  blocked PSD/aggregate rebuild has completed. A private PC-Engine bootable
  reference is assembled from those distributions; MS-DOS uses its committed
  bootable reference. Their raw SHA-256 values are respectively
  `073dc96bd621fdfce4ae530652f27364911161bcd55ba0d874bde66a15ccf8b2` and
  `e88c0e0332094048bcf55b334cd0430fc1c77e158344b00e57946ad9bbc84a78`.
- Native N88-日本語BASIC V3.1 starts with `BASIC /87`, successfully loads
  `MANDEL.BAS` and RUN begins plotting a coloured band by frame 12000.
  Complete rendering of the fractal is not claimed.

### ROM-backed BASIC flag checks

Supplementary actual-guest BASIC calls cover VA/VA2 × V1/V2 × H/S, using valid
initialized backup records. The checked IPL files match MAME's `pc88va`
`varom1.rom` and `pc88va2` `varom1_va2.rom`; this statement does not assert an
unverified full-set identity or publish any other private ROM identity.

| Mode | IN6Eh | IN31h |
|---|---|---|
| V1H | 7Fh | F9h |
| V1S | FFh | F9h |
| V2H | 7Fh | 79h |
| V2S | FFh | 79h |

Both models produce the table. H uses the original hex query. Short synthetic
S key pulses lose characters in the initial bulk queries; spaced-character
follow-ups obtain decimal 255/249 or 255/121. Original-VA V1S obtains IN6Eh in
one follow-up and IN31h=249 in a separate held-key `?inp(49)` follow-up. Syntax
errors and incomplete queries are retained, not asserted as flag results.
This checks the observed emulator/ROM path, not hardware timing calibration,
full ROMful fresh/invalid/reset/state-file coverage or interactive typing quality.
Issues #58/#59 are not closed on this evidence alone.

## Remaining G132 work

- Supply the original PSD/archive and complete fresh Zundamon and aggregate
  payload/round-trip closure. Generated images stay outside Git.
- Interactive normal/fullscreen/narrow-window and presenter/tooltip checks
  remain pending. The current Cocoa run reports
  `The video driver did not add any displays`. Automation forces SDL dummy;
  its successful guest captures intentionally contain no information/footer UI.
  An attempted Displayed recording under dummy correctly fails as unavailable,
  and is not mislabeled as successful GUI verification.
- Check the master/footer/panel independence, exact tooltip details, N-family
  lamps, formal selected names and post-state-load behavior with the supplied
  Windows artifact. Actual saved-state-file round trips and the original
  game/full 604-byte probe runtime matrix are not claimed by the supplementary
  BASIC queries; relevant ROMless/source-build tests remain green.
- Retain the existing 26-file full-format baseline limitation. No main merge,
  cleanup, next milestone, G132 pass or final hosted CI success is claimed.
