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

Status: **Agreed implementation, static Windows artifact and full demo/media
source closure complete; interactive GUI and remaining regression verification
are pending. G132 not requested or passed.**

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

The initial PSD-source blocker was resolved after the maintainer supplied the
original archive. Fresh source/payload closure and the exact limits on reuse of
earlier runtime evidence are recorded below. Interactive GUI inspection is
still pending because the current execution environment supplies no Cocoa display.
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

## Clean-media regression

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

### PSD-source and aggregate closure follow-up

The maintainer supplied the original archive after the initial metadata-only
search had failed to locate it. That initial BLOCKED result is superseded by
this completed build, not retroactively relabeled PASS. The reference directory
and archive remain read-only; only the PSD member was extracted into task-private
scratch. Archive/PSD identities are recorded privately and the original archive
is checked unchanged. Neither private source metadata nor generated assets are
staged in Git.

Evaluated source: clean clone of pushed
[360eb71c](https://github.com/nakatamaho/vaeg/commit/360eb71c420e1e27b07adaa65a20b0d6516b9161).
The six component builder/source inputs are unchanged from the application-code
freeze; five completed distributions and selfboot results above are reused.
The isolated Ubuntu 24.04 environment uses Python 3.12.3, NASM 2.16.01,
psd-tools 1.26.0, Pillow 13.0.0 and NumPy 2.5.4, with no `convert` in PATH.

```sh
python3 -m venv "$PRIVATE/psd-env"
"$PRIVATE/psd-env/bin/pip" install psd-tools==1.26.0 Pillow==13.0.0 numpy==2.5.4
export TMPDIR="$PRIVATE/media-tmp"
demos/zundamon-orbit/build-from-psd.sh --python "$PRIVATE/psd-env/bin/python" \
  "$PSD_INPUT" "$SYSTEM_VA2" "$OUT/m132-zundamon-orbit.d88"
cp demos/disks/m132-zundamon-orbit.d88.xz "$DIST/zundamon-orbit.d88.xz"
python3 tools/pc88va/build-all-demos-distribution-disk.py --source "$SYSTEM_VA2" \
  --distribution-dir "$DIST" --output "$OUT/all-demos.d88" \
  --compressed-output "$DIST/all-demos.d88.xz"
python3 tools/pc88va/build-all-demos-msdos4-disk.py --source "$MSDOS4_SOURCE" \
  --distribution-dir "$DIST" --output "$OUT/all-demos-msdos4.d88" \
  --compressed-output "$DIST/all-demos-msdos4.d88.xz"
python3 tools/pc88va/build-all-demos-bootable-disk.py --source "$SYSTEM_VA2" \
  --distribution-dir "$DIST" --output "$OUT/all-demos-pce.d88"
python3 "$PRIVATE/compare-psd-aggregates.py"
```

Final commands exit 0. Tracked IDA profile regeneration matches; the two fresh
Zundamon payloads, 19 all-demos files and 25 MS-DOS aggregate files are exactly
equal to their committed FAT file maps. All three compressed outputs round-trip
exactly. The MS-DOS aggregate additionally equals the committed raw D88.
An initial unprefixed Zundamon output name correctly refuses to overwrite the
clone's committed compressed disk (exit 1); the prefixed output above resolves
that naming collision without deleting or changing the reference distribution.

| Produced raw image / neutral role | SHA-256 |
|---|---|
| zundamon-orbit.d88 | `7801fdfd91535686a4579e169d1b8abde0696f5c15e5bf3e3df13e93a62c9d61` |
| all-demos.d88 | `f4296aa825113ba5495ef87a007d7c5b2a1cc12d5a6a0ec72d39c3ec31c0ae16` |
| all-demos-msdos4.d88 | `e88c0e0332094048bcf55b334cd0430fc1c77e158344b00e57946ad9bbc84a78` |
| all-demos-pce-bootable-01 | `2053ccaa4228a3300f3a2a9ece9d118c291579b76d0428ecff03f8623c2bdfbe` |

| Produced compressed distribution | SHA-256 |
|---|---|
| zundamon-orbit.d88.xz | `549273004aa2a80359b1c08291933f44a6a4b057aa315e51eccd548bd43b8766` |
| all-demos.d88.xz | `9ef55a760375d43521a10f79a497d03b49e8d48bc51c707222487d95dd2ce256` |
| all-demos-msdos4.d88.xz | `67ddb64012bc89464cdaa2809c62a2a9c2460c52ecec7f815dc45a6193eef241` |

#### Binding the completed runtime evidence to fresh media

The fresh MS-DOS image equals the earlier runtime corpus byte-for-byte.
The fresh PC-Engine bootable image has identical file paths/payloads (23 files),
boot/system bytes, FAT allocation and all other bytes except directory creation/
modification timestamps: 244 differing bytes. A comparison that clears only the
FAT directory-entry time/date words at offsets 14–17 and 22–25 recursively
produces equal complete images; it never masks file contents or arbitrary offsets.
Normalized raw SHA-256:
`66cb4a415e5267414d0e808b311872276ae283230135c7af4792280daa8437d0`.
The initial exact-raw assertion failed on that timestamp difference and is
retained separately; raw equality is not claimed for PC-Engine.

This closes fresh media source/payload requirements and permits reuse of the
unchanged worker's 34 inspected demo/OS executions and BASIC Mandelbrot check
below. Those executions used the earlier reference containers, not the newly
produced containers; this is evidence reuse under a proven timestamp-only
container difference, not a claim of 34 new executions. No application input,
worker, payload, comparison policy or execution-target setting has changed.
Interactive GUI, state-file and M131 runtime regression work remains pending.

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
  then-blocked PSD/aggregate rebuild had completed. The later source closure
  above establishes exact MS-DOS identity and a timestamp-only PC-Engine
  difference, permitting reuse without claiming fresh executions. A private PC-Engine bootable
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

- PSD/archive source and fresh Zundamon/aggregate payload/round-trip closure
  are complete as recorded above. Generated images remain outside Git.
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

## S-mode paste regression follow-up

After the VA2/V1H score of 29801, the maintainer reports that V1S clipboard
paste drops the C in CLEAR and many other characters. This is a paste defect,
not a request to change compatible CPU timing or weaken the speed benchmark.

The production paste queue had one two-guest-frame interval for native/H/S.
The same two frames no longer allow the slower S BASIC scan to consume every
press/release. A normal No Wait automation run can hide this: the separate
20 ms host-time floor admits many more guest frames before each event.
Both line-at-a-time and full-queue No Wait trials initially appear correct.

A private, temporary test driver feeds the whole benchmark, including all
newlines, into one production paste queue. It advances the host-time argument
by at least 20 ms per guest frame, so the existing guest-frame floor, not a
slow host or No Wait, controls event admission. It does not change CPU, keyboard
matrix, BASIC or paste state-machine behavior. With that identical driver,
the original two-frame policy reproduces `10 LEAR,...`, merged/dropped lines,
`Overflow` and `IST` rather than `LIST` in VA2/V1S; VA2/V1H is intact. Changing
only paste pacing to four frames for live S produces the complete correct
11-line program and LIST in both H and S. The source/bytes, worker/driver
patch identities, commands, output hashes and inspected captures stay private.
The driver's initial NULL-action-text crash is retained as a test-driver setup
failure, then corrected before either controlled red/green run.

Production correction: use four frames between SHIFT/DOWN/UP events only when
88 mode is active and the live/restored input selects S. Keep two frames for
H and native V3, even with a saved/pending S choice. Keep the independent 20 ms
host floor, keyboard matrix semantics, cancellation/focus cleanup and all
CPU clocks unchanged. Extremely slow custom CPU settings are not claimed as
newly covered by this H/S-specific correction.

The temporary driver is removed byte-for-byte before production validation;
no private paths, virtual-clock hook or fixture marker enters the commit.
The original 338174ee executable is retained privately before the build tree
is updated. Earlier guest/media evidence still refers to that preserved
worker, not the new executable now at the old build output path.

```sh
cmake --build "$PRIVATE/build" -j 8
ctest --test-dir "$PRIVATE/build" -R '^vaeg_romless_tests$' --output-on-failure
ctest --test-dir "$PRIVATE/build" --output-on-failure -j 4
```

Focused and full production checks exit 0. The full run now lists 141 tests:
132 pass, nine unchanged skips, zero failures, 84.83s. New focused cases check
native H/S-latch and 88 H/S, deliberately opposite pending preferences, and
the actual DOWN/UP queue boundary on every guest frame. Starting from the
passing fixture, changing only the interval getter back to constant two frames
fails exactly `M132_PASTE_SPEED_POLICY` (CTest exit 8). Restoring the policy
passes. Full tests are run only after removing the private driver and
restoring the positive policy. This full run evaluates the precommit production
source; the committed version-only rebuild and focused suite also pass.
No desktop clipboard-menu/shortcut acceptance or G132 pass is inferred from
the controlled paste-queue test.

### Maintainer confirmation of the paste fix

After the refreshed 7fe3bebf Windows handoff, the maintainer reports
VA2/V1S IN6Eh=FFh and SCORE=15303, then explicitly confirms the paste character
loss is resolved (`解消しました。OK`). The preceding VA2/V1H report was
IN6Eh=7Fh/SCORE=29801; the observed S/H score ratio is approximately 0.5135.
This accepts the reported V1S paste correction and confirms the guest speed
comparison. It does not independently identify the exact desktop menu/shortcut
used, calibrate physical hardware, resolve TIME$, or pass the remaining G132
GUI/state-file/M131 regression requirements. G132 remains pending; no merge,
cleanup or next milestone is authorized by this scoped confirmation.

### Partial G132 maintainer checklist acceptance

On the displayed G132 checklist, the maintainer accepts section 1 (V3 boot,
bundled VA demo and OS/simple operations). In section 3, the maintainer accepts
both Information-menu entries, independent information/panel ON/OFF operation,
visibility persistence across save/restart and individual field-setting
persistence. These items are removed from the next remaining-work checklist.
The request to display `G123` again is interpreted in context as G132, not
approval of another milestone. Default-ON behavior for a new configuration,
window/fullscreen/presenter geometry, tooltip/content checks, additional machine
profiles and actual state-file/M131 regression checks remain unaccepted.
The maintainer subsequently also accepts three section-5 checks: the selected
machine's formal name is displayed, all V1/V2/V3 lamps are off during PC-8001
family N execution, and non-VA machines show no SGP clock. These are scoped
runtime/UI confirmations, not acceptance of every additional-ROM profile or
precise N80/N80SR subtype classification. The maintainer further accepts
PC-8801 live N/V1/V2 display consistency and identification/speed/display
consistency after a mode change and reset. Ambiguous-subtype wording and actual
saved-state-file round trips remain pending.
This is partial checklist acceptance, not passage of G132 or authorization to
merge, clean up or start the next milestone.

### Refreshed production worker and clean Windows paste-fix handoff

Fix/evaluated commit:
[7fe3bebf](https://github.com/nakatamaho/vaeg/commit/7fe3bebf83cdd418b4c7154f99d929a61f648a80).
Production macOS worker SHA-256:
`aa1a046e801e36f901183f30122ab7e6300acd6a6a441a8d24421426e22223fd`.
Recording remains ON and local bundled encoders OFF. The committed worker,
without the private driver, enters and runs the same BASIC speed program in
VA2/V1H and V1S. Captures at frame 20000 show H IN6Eh=7Fh/SCORE=29802 and
S IN6Eh=FFh/SCORE=15303. This confirms that the paste correction does not
undo the CPU slowdown. The ordinary No Wait production run is not substituted
for the controlled frame-floor red/green proof above.

```sh
python3 "$PRIVATE/run-paste-production.py"
vaeg-container tools/release/build-mingw-static-container.sh \
  --commit 7fe3bebf83cdd418b4c7154f99d929a61f648a80 \
  --shader-sdk "$PREVIOUS_HANDOFF/source/build/mingw-static"
```

Both exit 0. The Windows build uses a new clean clone and fresh application
objects, reusing only the prior audited pinned librashader dependency SDK
read-only. The archive/notices audits run again. The pinned Ubuntu 24.04,
Rust 1.88.0, static `mingw-cross` Release profile embeds the new commit/date
and passes the system-DLL-only PE import audit. Recording and local bundled
H.264/AAC encoders remain OFF in this public clean-build handoff.

- Current executable: `../vaeg-mingw-7fe3bebf/output/vaeg.exe`, 57,627,313 bytes.
- SHA-256: `efd47fb7f6b7fd3e1fb76eb13ef7e33441ecd98ed43fcffedea8c17514b7c6cd`.
- Report: `../vaeg-mingw-7fe3bebf/output/BUILD-REPORT.md`.
- This supersedes the 338174ee Windows artifact for further paste/GUI checks.
  Both old and new artifacts/evidence are retained; no Windows runtime or
  maintainer acceptance is inferred from compile/import success.
- Encoding/EOL/case/Markdown/milestone-ID/diff and changed-line clang-format 22
  checks pass. Global formatting debt is unchanged. The final hosted review
  run remains deferred while the other G132 prerequisites are pending.

## User-reported BASIC stopwatch failure and clock-independent check

The assistant's proposed `TIME$="00:00:00"` / nested FOR-loop stopwatch
was not validated before being offered. The maintainer reports a completed
H run printing `ELAPSED: 00:00:00`. The unchanged 338174ee worker reproduces
that exact H result under NEC N88-BASIC Version 2.4. The S capture at frame
18000 is still executing the original long loop; it is not recorded as a
completed S stopwatch result. The underlying TIME$ cause is unresolved and
tracked in the [open ledger](../../modernization/bug-fixes.md#m132--basic-time-stopwatch-remains-at-zero)
and, at the maintainer's request, independent [issue #60](https://github.com/nakatamaho/vaeg/issues/60).
Issue creation is not a clock fix or G132 acceptance.

A pure-BASIC VRTC polling alternative yields H=1837/S=1650 counts, but its
interpreter-level polling can miss short VRTC pulses. It is not a reliable
speed-ratio test and is not supplied as evidence of half speed. Initial
per-character automation mishandles isolated spaces (`@text ` becomes literal
input after trimming); those syntax-error/setup trials are preserved, not
attributed to the guest CPU. Whole-line automation correctly enters the
programs. One H setup trial times out; one S screenshot is taken before RUN's
Return has been injected. The targeted S follow-up extends the capture horizon.

A 47-byte compatible-mode routine instead counts fast polling iterations over
eight complete VRTC periods, preserving AF/BC/DE/HL and briefly masking
interrupts during measurement. It reserves B000h upwards via CLEAR, writes the
16-bit result at B100h, then returns with interrupts enabled. It uses neither
TIME$ nor a host stopwatch. Exact BASIC used in the successful H and S cases:

```basic
10 CLEAR ,&HAFFF
20 CLS
30 FOR I=0 TO 46:READ D:POKE &HB000+I,D:NEXT I
40 A=&HB000:CALL A
50 PRINT "IN6E=";HEX$(INP(&H6E))
60 PRINT "SCORE=";PEEK(&HB100)+256*PEEK(&HB101)
70 END
100 DATA 245,197,213,229,243,33,0,0,6,8,219,64
110 DATA 230,32,40,250,219,64,230,32,32,250,35,219
120 DATA 64,230,32,40,249,35,219,64,230,32,32,249
130 DATA 16,240,34,0,177,251,225,209,193,241,201
```

```sh
python3 "$PRIVATE/run-basic-benchmark-bulk.py"
python3 "$PRIVATE/run-basic-benchmark-vrtc.py"
python3 "$PRIVATE/run-basic-benchmark-vrtc-followup.py"
python3 "$PRIVATE/run-basic-benchmark-vrtc-machine.py"
```

All four final commands exit 0; result screens are inspected, not inferred from
process exit. Configurations select VA2/V2, standard multiplier x2 and CPU 100%,
using the same initialized backup seed and worker hash as the prior BASIC flag
checks. Automation uses No Wait; the measurement is tied to guest VRTC, not host
elapsed time. The final machine-code captures at frame 20000 show:

| Selection | IN6Eh | Score (higher is faster) |
|---|---|---:|
| H | 7Fh | 29802 |
| S | FFh | 15303 |

S/H=0.51349 for this loop. Native bus waits, including IN waits, remain in the
native domain, so an exact 0.5 ratio is not required. This is observed emulator
execution against its video timeline, not physical hardware calibration or a
TIME$ fix. Private commands, runner/probe identities, captures and output hashes
are retained. No application source is changed and unchanged full tests/builds
are not rerun for this evidence-only follow-up.
