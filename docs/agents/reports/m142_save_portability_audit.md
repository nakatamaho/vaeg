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

# M142 — save portability: old defect, current format and actual coverage

## Conclusion and evaluated identities

There is **no supplied failing exchange between the current M142 Windows and
Linux/macOS builds**. The known old DISK-layout defect is corrected in M142;
actual native foreign-producer exchange remains **NOT_RUN**, not a demonstrated
failure and not a PASS. macOS is not intentionally excluded from save loading.
Do not describe lack of a Linux build/test target as a save-format failure.

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
| Can current macOS save/load its own files? | PASS for the scoped native continuation profiles; explicit new-process loading added above |
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

M142 remains **IN_PROGRESS / G142 NOT_READY**. This investigation is not
acceptance, a merge, a release, or a new fix for a demonstrated native
Windows/Linux exchange failure.
