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

# M142 — save portability: historical audit and authorized correction

## Current disposition

**The authorized BMS/media correction is implemented and native-tested.**
See [the correction and current evidence](#authorized-restoration-correction-and-verification).
The evaluated corrected application is
[0f3cbe20190ccca6524dda59301303583f535c0a](https://github.com/nakatamaho/vaeg/commit/0f3cbe20190ccca6524dda59301303583f535c0a).
The new container deliberately rejects old SAV files; backward compatibility
was explicitly waived. Genuine cross-OS execution and G142 acceptance remain
NOT_RUN / NOT_READY.

The audit sections below preserve the **pre-correction** source, workers,
failures and then-current conclusions. In those sections, “current”, “unfixed”
and v0/v1 compatibility refer to that historical checkpoint, not the corrected
application. In particular, the previously supplied `72118426` Windows
executable still contains the BMS defect; do not overwrite original SAVs with
that old build. No past failure has been relabeled as a correction PASS.

## Conclusion and evaluated identities

There is **no supplied failing exchange between the current M142 Windows and
Linux/macOS builds**. The known old DISK-layout defect is corrected in M142;
actual native foreign-producer exchange remains **NOT_RUN**, not a demonstrated
failure and not a PASS. macOS is not intentionally excluded from save loading.
Do not describe lack of a Linux build/test target as a save-format failure.
**A separate current defect is now reproduced on macOS: loading a BMS-enabled
snapshot while the reader starts with BMS OFF loses the saved bank-memory
payload.** Thus the audit does not conclude that current state restoration
is defect-free merely because the old DISK layout was corrected.

- Evaluated source: `721184268d42424de4a7abc0f5ac0bf4e5630ea2`.
- Fresh clean Windows build of that pushed source: display
  `Rel.20261031 (M142-dev, 72118426)`, executable SHA-256
  `e2d1f45bdf82bc979ecd94858ba7b7d277c3f3bb9aa247716bba3e160bf332a9`.
  Recording/shaders ON, bundled encoders OFF; static archive, embedded source
  identity and system-DLL import audits PASS. Windows execution NOT_RUN.
- Native macOS arm64 worker: source
  `0d02bc849b5c4fb4f45d21c13716735e77578d37`, SHA-256
  `24f53861e4a81f0914d489c353921113cc81e0aa1f730f00b9a9b07deea70700`.
  Application C/C++/headers are unchanged through the evaluated source;
  the old worker is reused under its real identity, not relabeled.
- The requested Linux target is x86-64 with reported Ubuntu glibc 2.43.
  No Linux counterpart has been built in this follow-up. That glibc version
  alone does not establish a save incompatibility; no glibc-version field is
  used to accept/reject a save.

The maintainer paused Linux provisioning/build work to request this audit.
No application behavior, save format, configuration selection or release
acceptance is changed here. The OS-specific cfg proposal was withdrawn.

## Current BMS restore defect: reproduced and unfixed

A follow-up uses the retained enabled-expansion snapshot, with EMS 13 MiB
and BMS 128 banks/16 MiB at port 01D0h. A known 16-byte synthetic marker is
placed in BMSMEM without changing its length; the same marked save is used
for both cases. All reader settings are identical except `Use_BMS_`:

| Reader at startup | Production load/continuation | Re-saved BMSMEM | Synthetic marker |
|---|---|---:|---|
| BMS ON, matching the snapshot | exit 0, reaches the expected guest PC | 16777216 bytes | Preserved |
| Only BMS changed to OFF | exit 0, reaches the expected guest PC | Section absent, 0 bytes | Lost |

The re-saved BMSIO device payload still declares BMS enabled and is identical
to the input in both cases. The second result is therefore **a reproduced
restoration/data-loss defect, not a PASS**. The original input save remains
byte-identical; it is the restored machine and a subsequent new save that
lose the RAM. No guest BMS-dependent application or invalid-bank access was
run, so a host crash or a particular application's symptom is not claimed.

The demonstrated cause is in the shared C loader:

1. `statsave_load_internal()` calls `cbuscore_reset()` before reading the
   saved device sections. The working BMS allocation follows the reader's
   current configuration at that point.
2. The state table places BMSMEM before BMSIO. `flagload_bms()` reads into
   the current working buffer **only if it is non-NULL**, otherwise returning
   success and skipping the saved RAM.
3. BMSIO is restored later as a raw device structure. No allocation/reload
   reconciles its enabled/size fields with the already skipped BMSMEM.
4. `flagsave_bms()` omits the section when that working buffer is NULL.

This is not an OS-specific DISK ABI mismatch: it occurs within macOS alone.
The same shared loader exists in the supplied Windows source and in Linux
sources, but those native targets have not been executed here. M142's new
BMS-OFF default makes the transition from a BMS-enabled older configuration
particularly relevant. It is **not** proof of the cause of an unsupplied
specific Windows/Linux failure.

Two additional native workers ran. The matching-ON fixture passes before
the single configuration mutation reproduces the defect. Both process
exits are 0, which must not be interpreted as two restoration PASSes.
Original save/media and worker identities are rechecked unchanged. Evidence
is retained privately as `BMS_LOAD_AUDIT_01` (`probe.py`, `results.json`,
per-case `run.json`, config-before text and actual re-saved state files).
`python3 "$BMS_OUT/probe.py"` exits 0 because its expected result is to
**demonstrate the defect**, status
`REPRODUCED_OPEN_DEFECT_NOT_A_FIX_OR_CROSS_OS_PASS`.

No correction is implemented. Matching BMS enabled/size settings is a
passing workaround for this tested case, not a general restoration guarantee.
A correction must establish the saved BMS configuration/allocation before
restoring RAM, check payload consistency, and cover configuration/size
transitions. Keep this in the [open-defect ledger](../../modernization/bug-fixes.md#open-defects)
until a tested correction exists. Total new workers across both audits: eight.

## Demonstrated old defect

Before [the DISK correction](https://github.com/nakatamaho/vaeg/commit/033992d943409d607a9931b9ba90096449708ce5),
`STATDISK` contained `char path[MAX_PATH]` and was written/read using the
host's native `sizeof(STATDISK)`. Ten records describe four FDDs, two SASI
and four SCSI devices.

| Host | Path bytes | Old record bytes | Ten-record DISK bytes |
|---|---:|---:|---:|
| Windows | 260 | 272 | 2720 |
| macOS | 1024 | 1036 | 10360 |
| Linux | 4096 | 4108 | 41080 |

Small C probes compiled against the actual macOS and MinGW headers confirm
those two hosts' widths, date/time sizes and old record offsets. The Linux
width is established by the original [issue #76](https://github.com/nakatamaho/vaeg/issues/76)
and the portable frontend's Linux `PATH_MAX`, not a new Linux execution.
This is a demonstrated representation mismatch, not evidence that the CPU
registers or the entire file are corrupted on disk. A different-width reader
can reject an undersized record or misinterpret a larger record; not every
direction necessarily fails in an identical way.

The old DISK checker did not consistently enforce a section version/size
contract. In particular, do **not** assert that every old reader necessarily
rejects v1 just because its version is new: the old Linux record width is
also 4108 bytes. Arbitrary old-reader/new-file combinations remain unsupported
by this audit and must be inspected rather than diagnosed from a label alone.

## What M142 changes, and what it does not

[`machine/statsave.c`](../../../machine/statsave.c) now writes DISK v1 as ten
explicit **4108-byte** records, always **41080 bytes** in total:
4096 UTF-8 path bytes, little-endian flags/image selection, date/time fields
and one reserved byte. The new reader accepts v1 and the three historical
v0 widths, with bounds/path/flag/version checks. This is a DISK-specific wire
format, **not a conversion of every save section to a universal wire format**.

Many other entries in [`statsave.tbl`](../../../machine/statsave.tbl) still
serialize native project structures. The existing compiled macOS arm64 and
fresh Windows x86-64 objects were inspected without rebuilding vaeg:
all **63** state-table entries agree in name, version, kind and declared size.
Their `long` sizes differ (8 versus 4), but that did not produce a table-size
mismatch. This is a useful ABI/table check; it does not prove all member
offsets, dynamic payloads, pointer rebinding or Linux layouts are equivalent,
and does not replace actual foreign-producer loading.

## New native macOS load probes

Task-local evidence is retained outside Git under the neutral binding
`SAVE_AUDIT_01`: `layout.c`, `layout-audit.py`, compiled probe objects,
`layout-results.json`, `load-probes.py`, `load-results.json`, commands/exits,
per-case `run.json`, worker logs and raw captures/states. Private input paths,
ROM identities and raw save contents are not published.

The probe starts from an identity-checked earlier macOS VA2 state of the
source-built synthetic V128A program. It rewrites **only DISK**, leaving all
other sections byte-identical, and uses Windows-style foreign media paths.
Each case starts a **separate process**, explicitly loads the prior saved
file and continues to the same guest PC. The unchanged reusable
[`runtime_state_roundtrip.py`](../../../tools/qa/runtime_state_roundtrip.py)
checks registers (excluding observation ordinal/clock), native TVRAM and
GVRAM against the retained expected guest point. An actual re-save checks
which local medium was restored. Each case uses a writable media copy;
original snapshot/media and worker digests remain unchanged.

| Case | Actual result | Scope |
|---|---|---|
| Current v1, foreign Windows-style path | PASS | Registers/TVRAM/GVRAM agree; configured local medium restored |
| v0 with 260-byte path field | PASS | Same continuation and media contract |
| v0 with 1024-byte path field | PASS | Same continuation and media contract |
| v0 with 4096-byte path field | PASS | Same continuation and media contract |
| v1, deliberately missing medium | Expected warning/partial restore demonstrated | CPU/video continuation agrees, `FDD1: not found` is logged, re-save proves FDD1 is empty |
| v1 baseline with only version changed to 2 | Expected rejection PASS | Exact `M142_DISK_VERSION`, exit 1 and no restored-state output |

Six new native workers ran: five exit 0 and the controlled version rejection
exits 1. The missing-medium case is **not** complete media restoration, even
though its process exits 0. The negative starts from the passing v1 fixture
and changes exactly one byte in its version field.

Commands (all out of tree except repository sources):

```sh
clang -std=c99 -I sdl2 -I common -I . -c "$OUT/layout.c" -o "$OUT/macos_arm64.o"
x86_64-w64-mingw32-gcc -std=c99 -I sdl2 -I common -I . -c "$OUT/layout.c" -o "$OUT/windows_x86_64.o"
python3 "$OUT/layout-audit.py"  # exit 0; 63 entries, zero differences
python3 "$OUT/load-probes.py"   # exit 0; six cases meet the scoped expectations
```

Status: `PASS_SCOPED_MAC_LOAD_AUDIT_NOT_GENUINE_CROSS_OS`.
These are **synthetic foreign-DISK layouts around a macOS-produced state**,
not genuine Windows/Linux-produced saves. SDL dummy/software is guest
integration coverage, not Cocoa/presenter/GUI acceptance.

The earlier [state campaign](m142_gate_preparation.md#actual-native-full-state-and-process-restart-contract)
proved three same-process continuations and separately checked configuration/
expansion persistence after fresh process starts. Those restart workers did
not load the earlier snapshot. The new four successful cases above add
explicit prior-file loading in a new process; they do not retroactively
upgrade that older campaign.

## Distinguish format errors, missing assets and GUI discovery

1. **Media are external.** A save records disk paths/flags/times, not a full
   copy of the disk images. `np2_state_disk_resolve()` keeps an existing path
   or finds a unique same-basename file in configured media directories.
   It does not recursively search, identify renamed media by hash or choose
   between conflicting same-name candidates. Local `MAX_PATH` limits still
   apply. Same-name files are not content-identity proof.
2. **A warning is not complete restoration.** The induced missing-medium
   case demonstrates a load that restores CPU/video while leaving FDD1
   absent. A copied `.sav` alone is therefore not sufficient evidence that
   the guest's disk operations can continue.
3. **Ordinary GUI feedback has limitations.** In
   [`state_load()`](../../../sdl2/gui/gui.cpp), disk-change warnings are
   allowed past preflight, their text is not included in the eventual
   success status, and the return value of the subsequent `statsave_load()`
   is discarded. The missing-media core behavior above is demonstrated;
   the GUI branches are source findings, **not a newly captured GUI failure
   or an established cause of a reported current cross-OS failure**.
4. **Slot discovery is host-path dependent.** `state_directory_name()`
   hashes the complete boot-media path. The same media on different hosts
   consequently selects different slot subdirectories. Copying a foreign
   directory need not populate the receiving host's current quick slots.
   The file browser can select other subdirectories under the active
   `states/` root; it lists `.sav`, not the harness's `.state` suffix. Its
   extension test is case-sensitive. Thumbnails are separate, optional PNGs.
5. **The root can differ too.** `file_getappdir()` prefers a writable
   directory beside the executable and otherwise uses the OS user-data
   directory. A save can be present on disk but outside the browser's root.
6. **Other preflight guards are intentional.** Main-CPU-role mismatches,
   unsupported CPU/device payloads, missing old compatible-mode metadata
   and HOSTFAT snapshot/mount identity differences can reject a file
   independently of DISK. Matching ROM/machine configuration and suitable
   media are required; this audit does not recommend forcing those guards.

## Current coverage and next diagnostic input

| Question | Current evidence |
|---|---|
| Can current macOS save/load its own files? | PASS for the scoped matching-configuration profiles; explicit new-process loading added above |
| Is restoration correct when BMS settings differ? | FAIL reproduced: BMS-enabled save into BMS-OFF startup loses bank RAM |
| Can current macOS parse the supported old host DISK layouts? | PASS for the four native probe/continuation cases above |
| Does the current Windows binary contain the DISK correction? | Yes: clean source/build and matching compiled table audited |
| Has current Windows actually produced/loaded a save here? | NOT_RUN |
| Has a current Linux counterpart been built/executed here? | NOT_RUN |
| Genuine Windows↔Linux, Windows↔macOS or Linux↔macOS exchange? | NOT_RUN; no blanket PASS or FAIL |

For a specific current failure, first collect the producer and consumer
`--version`, the exact load error and whether the file is absent from the
browser versus selected and rejected versus loaded without media. Request
one private save only if that information is insufficient. Keep its contents
and any captures outside Git/CI; ROMs, media and complete cfg files are not
default diagnostic uploads.

At this historical checkpoint, M142 remained **IN_PROGRESS / G142 NOT_READY**,
with the reproduced BMS defect OPEN/unfixed. That investigation was not a
correction, acceptance, merge or release. The following authorized correction
supersedes its defect status, not its retained observations.

## Authorized restoration correction and verification

### Format, device and media contract

The maintainer subsequently authorized all demonstrated BMS/media restoration
corrections and explicitly waived compatibility with old SAV files.

- Container version is now `0x261031`; old `0x08060a` files reject with
  `M142_STATE_FORMAT_OR_OPEN`. DISK v0/v1 inside the new container reject
  with `M142_DISK_VERSION`. Do not use an old SAV as a passing new-format fixture.
- Mandatory **BMSSTATE v1** replaces separate raw BMSIO/BMSMEM sections.
  A 16-byte explicit little-endian device/allocation header precedes the
  complete saved RAM. Enabled/bank/nomem/port/mask/length/reserved consistency
  is checked. Load allocates and reads saved RAM before replacing the active
  device and rebinds its selected port. The user's `bmsiocfg` remains the
  separate next-reset preference. OFF, bank zero and unavailable selected
  banks retain their distinct memory-window semantics.
- **DISK v2** contains 13 explicit 4160-byte records: four FDDs, two SASI
  drives and all seven SCSI IDs. Each stores a zero-padded 4096-byte UTF-8
  path, flags, 64-bit file length, SHA-256 and reserved zero bytes. FDD flags
  preserve protection and the selected multi-image D88 member.
- A filename locates a candidate; full byte length and SHA-256 establish
  identity. Existing-path/unique configured-basename resolution remains
  bounded: no recursive guessing or accepting a different same-named file.
  Missing, ambiguous, changed or unsupported media and invalid image selection
  fail preflight. This also applies when HOSTFAT override is requested.
- Dirty D88 track data is flushed before identification; flush failure fails
  the save and retains dirty-cache ownership for retry. Every saved reference
  is resolved before mount context changes. All mount results are checked;
  empty saved HDD slots detach stale devices. SAV files **do not embed or roll
  back media contents**: changing an external disk after saving intentionally
  causes identity rejection, rather than an apparently successful wrong-disk
  restoration.
- Required BMS/DISK sections, known-section duplicates, unsupported sections,
  truncation and noncanonical termination are rejected. GUI/debug callers
  consume actual checked-loader results, not only a prior preflight result.
  The debug harness no longer turns media warnings or an implicit HOSTFAT
  retry into success. Explicit HOSTFAT override remains a separate action.
- `file_open_rb()` now really uses `rb`, not `rb+`. Read-only inputs do not
  require an update stream. Saving uses an exclusive sibling temporary file,
  checked serialization/close and atomic replacement; failure preserves the
  prior destination. Quick-load refuses to proceed without a saved undo;
  failed quick undo retains both snapshots instead of deleting the held one.

The correction does **not** claim global machine rollback after every possible
late I/O/allocation failure, fsync/power-loss durability, or a universal wire
ABI for all remaining native-structure sections. GUI controller checks below
are not window inspection or a complete interactive quick-undo test.

Fixing commits:
[BMS](https://github.com/nakatamaho/vaeg/commit/714f6084bb0b23ffbd15d946b1d2650521800426),
[media identity and all slots](https://github.com/nakatamaho/vaeg/commit/540b9800074639acf1d8c396c33be3c32da9b1de),
[actual load-result propagation](https://github.com/nakatamaho/vaeg/commit/b8bbe14c0edf358f2dfe4b727898f1fee4beb84b),
[read-only input](https://github.com/nakatamaho/vaeg/commit/1cf19a8ad21f13aab22caa91872be9f8b531c946),
[atomic save/undo protection](https://github.com/nakatamaho/vaeg/commit/108b30aacba6b07387685a18d4bc07414ca34332),
[BMS port validation and bank-edge coverage](https://github.com/nakatamaho/vaeg/commit/098e9568906338a418180b14fe6b1a3ebba9897d).
[The fresh-process runner](https://github.com/nakatamaho/vaeg/commit/0f3cbe20190ccca6524dda59301303583f535c0a)
is independently callable; its content negatives do not depend on Git history.

### Corrected native evidence

Private evidence binding **STATE_FIX_NATIVE_01** retains exact commands,
source/worker/runner/corpus/contract identities, input/output hashes, logs,
exit codes and immutable producer inputs. No original ROM, state, media,
config, raw capture or private asset identity is committed.

- Evaluated source: **`0f3cbe20190ccca6524dda59301303583f535c0a`**.
- Native arm64 application: **`Rel.20261031 (M142-dev, 0f3cbe20)`**;
  worker SHA-256
  **`380c8199441e63b707a2edf735e43fc3e9b8413a64e890ab82d825b8e19200eb`**.
  Recording ON, bundled encoders OFF; separate corrected application objects
  with audited isolated FFmpeg dependency reuse. The frozen worker copy is
  byte-identical to the tested build executable.
- Final complete CTest: **155 PASS / 16 SKIP / 0 FAIL**, 171 entries,
  exit **0**, 165.28 seconds. This is a fresh complete campaign, not a
  composed recovery. Earlier two 170-entry campaigns at `aefaa756` remain
  retained under their own worker identity; they are not relabeled `0f3cbe20`.
- Production-path BMS tests cover **seven** saved/reader transitions, full
  patterned RAM comparison, next-reset preference preservation, real port
  and bank-window reads, bank-zero/absent-bank semantics, actual re-save
  payload equality, and **11 exact-code** BMS/container rejection guards.
  DISK tests cover multi-image/protection/remapping, ten exact metadata guards
  plus real media-byte identity mutation, SASI and all seven SCSI IDs, and
  nine stale-HDD detachments. SHA-256 passes 16 vector/streaming checks.
- Save-failure tests preserve an existing SAV byte-for-byte, preserve a
  pre-existing exclusive-temp collision, and reject rename-to-directory with
  `M142_STATE_COMMIT`. The read-only probe fails before the mode correction
  with `M142_STATE_READ_ONLY` (CTest exit 8), then passes afterward. GUI result
  controller tests apply five exact non-success results after passing controls;
  these are explicitly headless, not visual checks.
- Updated runtime-state content tests pass **12 exact-code single-mutation
  negatives**; the new prior-file comparator passes **four**. Every negative
  starts with its relevant passing fixture and asserts its intended code.
- Three actual absent/explicit-OFF/enabled profiles pass same-process guest
  save/advance/load continuation and separate config-persistence restarts:
  `PASS_STATE_CONFIG_CONTRACT_NOT_GUI_OR_CROSS_OS`. New state sizes are
  **2124032 / 2124032 / 32532752** bytes. Enabled RAM remains EMS 13 MiB and
  BMS 16 MiB; the size delta is now 29 MiB + 16 bytes. Config restarts still
  do not themselves load the prior SAV.
- The additional prior-file matrix **does** explicitly load that earlier SAV
  in fresh processes. Seven successful loads cover matching ON, reader OFF,
  smaller reader allocation, the opposite reader port, saved OFF into reader
  ON, and two passing media controls. Full marked BMSSTATE bytes survive
  re-save; independently retained guest register/TVRAM/GVRAM continuation and
  the actual resolved drive path/content identity match. Two single-mutation
  fresh-process negatives reject exactly `M142_DISK_MISSING` and
  `M142_DISK_IDENTITY`, with no restored output. Status:
  **`PASS_PRIOR_FILE_BMS_AND_MEDIA_CONTRACT_NOT_CROSS_OS_OR_GUI`**.
  Together these runners execute **18** new workers: 14 ordinary successful
  exits and four expected guard rejections, not 18 successful restorations.
- An initial added OFF-port assertion failed because 00ECh normally belongs
  to the Kanji-ROM interface, not an unbound FFh port. The independent
  BMS-absent bus baseline corrects that test assumption; production code was
  not altered to fit it. Both failed attempts remain in private evidence.
- Repository encoding/EOL/case/Markdown/whitespace checks and milestone
  selftest/discovery/audit pass. The formatting checker still reports **659
  inherited diagnostics** in 29 files; **zero intersect changed lines**
  relative to the pre-correction source. This is not whole-tree format PASS.

Exact final local command contracts (bindings are private and explicit):

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

All five commands exit **0**. The retained build configuration also explicitly
enables recording, tests, static FFmpeg, public-build policy and integration
trace; librashader is OFF for this native test target. No Colima, global SDK
change, private CI upload or local bundled encoder is used.

The 16 skipped CTest entries are explicit, not counted as PASS:

```text
vaeg_recording_h264_hardware_probe
vaeg_sst_v20_ci
vaeg_upd780_fdc_payloads
vaeg_upd780_fdc_seed_payloads
vaeg_upd780_fdc_fixed_payloads
vaeg_upd780_fdc_capture_payloads
vaeg_upd780_fdc_xadr_payloads
vaeg_upd780_fdc_pref_payloads
vaeg_upd70008_probes
test_suffix.sh
test_compress_generated_abc
test_compress_generated_text
vaeg_upd780_probes
test_compress_generated_random
test_files.sh
vaeg_m103b_basic_boot
```

Native SDL-dummy evidence does not establish Cocoa/window/HiDPI inspection,
Windows execution, a Linux build, or genuine producer/consumer exchange
between operating systems. The historical table-size audit is not upgraded
to those results. The withdrawn OS-specific cfg proposal remains withdrawn;
#75's approved correction deferral is unchanged. **M142 IN_PROGRESS /
G142 NOT_READY**; no acceptance, merge or release is implied.
