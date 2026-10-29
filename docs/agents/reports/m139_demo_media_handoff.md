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
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.
-->

# M139 — local demo/media regression and clean MinGW handoff

## Evaluated source and scope

Evaluated application/media source:
[584bfcbf0ce6c635a9198ac868fe606498e87434](https://github.com/nakatamaho/vaeg/commit/584bfcbf0ce6c635a9198ac868fe606498e87434),
already pushed before the clean clones. This completes the maintainer-requested
local build/guest-content/media checks and static executable delivery. The
subsequent [G139 scoped acceptance](m139_cpu_role_acceptance.md) confirms the
resolved MA identification rejection; it does not establish full MA drawing.
The private MA guest's later red-screen stop remains unresolved.
M142's memory defaults, clock menu and MH-demo investigation are not implemented
or included in this executable.

Media were rebuilt in a separate initially clean checkout of that commit.
The Windows executable uses another new clean clone and fresh application
objects. Guest execution uses the retained macOS development worker, whose
instruction/frontend sources remain equivalent to the M139 implementation:
the only later production C/C++ source difference is the documented comment
in the FDC configurator; other C/C++ additions are tests. This worker's earlier
embedded base identity is **not relabeled** as a clean build of that commit. The clean MinGW
artifact is compiled/audited, not executed on Windows here.

The new [content checker](../../../tools/qa/all_demos_draw.py) is independent
of Git history and private paths. It validates guest-visible drawing, BASIC
progress and live native TSP text, not just a zero process exit or saved PNG.
Its selftest passes six profiles and **25 passing-fixture single-mutation
negatives**, requiring exact error codes. This QA/report work changes no
application code or active vendor implementation; the evaluated binary stays
bound to that commit rather than being rebuilt or relabeled for report edits.

## Results

| Requested check | Result | Exact limit |
|---|---|---|
| Six source-built demo distributions | **PASS** | GLASS, NEON3/4, pseudo-sprite, wireframe and PSD-based zundamon; all stored files match the committed distributions byte-for-byte |
| Aggregate and self-booting distributions | **PASS** | Nine `.d88.xz` distributions compared in total; all payload maps equal, entire raw MS-DOS4 and self-booting SGP images also equal |
| Additional standalone demo builds | **PASS** | SGP scan, contention, Z80 timing, VA Voyage/OPNA and zundamon source-built fixture |
| All-demos execution | **34/34 PASS_DRAW_CONTRACT** | All17 programs on both PC-Engine and MS-DOS4; bounded substantial multicoloured guest drawing, not every scene or full animation correctness |
| Unmodified MANDEL.BAS | **PASS_PROGRESS_CONTRACT** | N88 BASIC V3.1 via `BASIC /87`; source-derived first-row colours and increasing plotted rows, not full-fractal completion |
| Utility FDD | **PASS** | Generated successfully; guest executes DIR, displays expected root files/directories and returns Ready |
| VA / VA2 SASI | **PASS for both configured models** | Generated successfully; each boots with only its SASI image attached and reaches its expected filesystem listing/Ready, using initialized firmware backup settings |
| Self-booting SGP | **PASS** | Source-built raw image equals committed image and its guest menu is reached |
| Clean static MinGW | **PASS build/import/encoder audit** | Recording ON; no bundled H.264/AAC; Windows system DLLs only; no Windows runtime/presenter PASS claimed |

The execution target is **macOS SDL dummy, guest-content only**, not Cocoa
presenter/window/fullscreen, Windows execution, physical GPU or silicon coverage.
VA2 base ROM inputs match MAME's published `pc88va2` records. The separately
bound VA-profile inputs are not independently qualified as a matching published
set here; that result is explicitly an emulator-model boot test, not ROM or
physical-machine conformance. Private input names, paths, digests and raw
captures remain out of Git and hosted CI.

### Executed programs

Both OS cases run all of:

- GLASS; NEON200 and NEON400; NEON4 in16 and65536 colours.
- SGPD_7A/B/C/D/S; SGP256S/T; SGP655S.
- SGPWIRE in16,256,65536 colours; ZUNDAORB.

The draw check compares a pre-command frame with post-command frames. It
requires changed guest pixels, saturated colour and multiple nonblack colours,
excluding the frontend strip. The single24x24 sphere uses its source-appropriate
smaller-area profile, not the generic large-scene threshold. Zundamon needs a
longer bounded asset-load/startup window; the early still-loading captures are
retained separately. Every final passing case also rejects replacing its post
capture set with the pre-command image using `G139_DEMO_DRAW_MISSING`.

For MANDEL, the source's MI32 and coordinates predict escape iterations1/3/2
at first-row x0/320/639, hence red/orange/red. Those points match. Completed
left-edge red prefixes increase from6 to12 rows between the6000/9000-frame
captures. This checks actual computation/drawing progress without changing
the BASIC source or reducing its iteration count. Frame numbers are not an
absolute-speed measurement.

Native boot checks decode the live TSP text frame from `VAEGSCN1`, rather than
searching arbitrary RAM for old strings. The utility cases require the expected
root directory entries and final Ready; the SGP case requires its menu markers.
All nonselected FDD/SASI/SCSI slots are explicitly `none`. The SASI cases use a
ROM-initialized backup followed by normal `Boot_FixedDisk=true` and automatic
640KiB RAM settings on reset. The failed cold/uninitialized attempts remain
separate; **fresh uninitialized backup boot is not marked PASS**.

## Rebuild recipe and output identities

The successful distribution rebuild uses Ubuntu24.04, NASM **2.16.01** and the
PSD-tools decoder, with the user-supplied PSD and an explicit system geometry
input. Each command in the following recipe exits0; `$OUT`, private input
bindings and the full per-command/output identity manifest stay out of tree:

```sh
# Each of glass-orbit, neon3, neon4, sgp-pseudo-sprite, sgp-wireframe:
demos/<demo>/build-d88.sh "$SYSTEM_GEOMETRY" "$OUT/<fresh-name>.d88"
demos/zundamon-orbit/build-from-psd.sh --python "$PSD_PYTHON" \
  "$PSD_INPUT" "$SYSTEM_GEOMETRY" "$OUT/<fresh-zundamon>.d88"
python3 demos/sgp-boot/build-boot-d88.py --output "$OUT/sgp-boot.d88" \
  --compressed-output "$DIST/sgp-boot.d88.xz"
python3 tools/pc88va/build-all-demos-distribution-disk.py \
  --source "$SYSTEM_GEOMETRY" --distribution-dir "$DIST" \
  --output "$OUT/all-demos.d88" --compressed-output "$DIST/all-demos.d88.xz"
python3 tools/pc88va/build-all-demos-bootable-disk.py \
  --source "$SYSTEM_SOURCE" --distribution-dir "$DIST" --output "$OUT/bootable.d88"
python3 tools/pc88va/build-all-demos-msdos4-disk.py --distribution-dir "$DIST" \
  --output "$OUT/all-demos-msdos4.d88" --compressed-output "$DIST/all-demos-msdos4.d88.xz"
tools/pc88va/build-utility-disk.sh --source "$SYSTEM_VA2" --output "$UTILITY_FDD"
tools/pc88va/build-sasi-utility-disks.sh --source-va "$SYSTEM_VA" \
  --source-va2 "$SYSTEM_VA2" --payload-d88 "$UTILITY_FDD" \
  --cpm-tools-d88 "$CPM_TOOLS" --cpm-source-d88 "$CPM_SOURCE" \
  --cpm-dev-d88 "$CPM_DEV" --output-dir "$OUT"
```

CP/M inputs were freshly built with the repository installer, pinned assembler
and shared-cache archives; missing pinned license documents were fetched into
task scratch, then explicit CP/M image bindings were passed to the SASI builder.
The automatic offline-CP/M branch initially failed on that cache omission and
is **not claimed passed**. No fabricated license cache or checksum bypass is used.

Raw generated source-distribution SHA-256 values (payload equality, not general
whole-image equality):

| Image | SHA-256 |
|---|---|
| glass-orbit | `2273caa606843ebf1286243587a694d922639a13ff96f065408ba0b37163daac` |
| neon3-distribution | `c56b1455a25b9432960bcbefe98adc621d3d9ad6e2c09c4a109c7f270c589081` |
| neon4-distribution | `5e9cee05b4539d1ab2550fb47290a3a19315181847d0566f824b731668a0993b` |
| sgp-pseudo-sprite | `edab2b61368901f0eae947945e6df00e41e1aa77b562f72a06d18445c791b819` |
| sgp-wireframe | `c01aafb81400fd7289ab8ee83547843d69d6bdd0dc2c358410c04c92d5714277` |
| zundamon-orbit | `11ab5ed0ae32b26edfa9b1d98323b986cb4539a83d88b7847cab66b0c85cfae2` |
| all-demos | `d1e676410eb6bad85f5e3372b87f298cd959e4de3a3da22ba0239265b3a5cbe9` |
| all-demos-msdos4 | `e88c0e0332094048bcf55b334cd0430fc1c77e158344b00e57946ad9bbc84a78` |
| sgp-boot | `67ead94dba0cee7aa9d9a0268cf1987bf3a2764fe1275e092b2e5ece06337154` |

System-derived utility, CP/M and bootable PC-Engine image hashes remain in the
private out-of-tree manifest under the asset-identity policy. No generated disk,
archive, binary, private capture or ROM is committed. Initial macOS NASM3.01 /
automatic ImageMagick builds had NEON3/zundamon differences; the successful
explicit Linux toolchain result is separate, not a universal cross-toolchain
byte-reproducibility claim. Other initial harness/setup and overly broad
content-threshold failures are also retained, not silently overwritten.

## Reported M141 build and MA rejection

Following a `G139 pass` message, the maintainer reported that MA still displays
its early VA-identification rejection, naming build
[9b60f9c1d5fea360ecbb955bf27c71b47454d4c0](https://github.com/nakatamaho/vaeg/commit/9b60f9c1d5fea360ecbb955bf27c71b47454d4c0).
That exact commit was fetched and its source/history inspected:

- It is the M141 state-time change, **not** the584bfcbf handoff above.
- It does not descend from
  [3053ee657eecf0c20aca3b967fa864620b1a2305](https://github.com/nakatamaho/vaeg/commit/3053ee657eecf0c20aca3b967fa864620b1a2305),
  the main-CPU role implementation. Their shared base is5d4dafff.
- Its main adapter/configurator still have the pre-M139 code; the independent
  non-VA main-uPD780 selector/instance is absent. This explains why the original
  identification correction cannot be assumed present in that build.
- The user's rejection is distinct from the **later** terminal red screen in
  the retained M139 candidate. That candidate's worker/input/capture identities
  still validate its earlier CPU-identity PASS / drawing FAIL; this is reuse,
  not a new9b60f9c1 runtime test or a Windows PASS.

At that checkpoint no final CPU-role gate/integration/cleanup was recorded.
The maintainer has since explicitly accepted the resolved identification scope;
the [acceptance supplement](m139_cpu_role_acceptance.md) records that amendment. Latest main also has a separately named M139 whole-screen-reverse
milestone: its acceptance is not transferred to this CPU work, nor overwritten.
Reconciliation must preserve the newer M141 TSP/state implementation and avoid
blindly merging stale milestone records. The all-demos34-case result does not
include or imply a passing MA fixture. M142 A–F remain separate and unstarted.

## Repository validation

The content selftest, UTF-8/no-BOM, LF, lowercase-path, Markdown-comment,
49 strict milestone checks, milestone audit and staged whitespace/scope checks
all exit0. The existing C/C++ whole-manifest formatting failures are not changed
or reclassified; only Python QA and reports change here. No hosted CI is used
as an iterative debugger, and no unchanged expensive emulator suite is replayed.

## Clean MinGW handoff

```sh
tools/release/build-mingw-static-container.sh \
  --commit 584bfcbf0ce6c635a9198ac868fe606498e87434 --recording \
  --shader-sdk "$AUDITED_DEPENDENCY_SDK"
```

Exit0 in vaeg's dedicated Colima daemon. The selected context is unchanged;
there is no shared/default daemon fallback. Only the audited shader dependency
SDK is reused read-only; application objects are new. Compiler GCC13-posix,
Rust1.88.0, preset `mingw-cross`, pinned Ubuntu24.04 image. Identity embedding,
static import audit and minimal-FFmpeg encoder registration audit pass:
**FFV1/PCM only**, local bundled H.264/AAC OFF. Windows Media Foundation export
uses OS APIs rather than bundled encoders.

- Executable: `../vaeg-mingw-584bfcbf/output/vaeg.exe`.
- SHA-256: `94532066a420219a734c993628766e72bf3e5e5ee7232383ec13ddd22e6d9091`.
- Size: 59256473 bytes; output copy equals the clean build artifact.
- Report: `../vaeg-mingw-584bfcbf/output/BUILD-REPORT.md`; import/toolchain,
  encoder-audit logs and third-party notices are retained alongside it.

The maintainer receives absolute paths separately. This is a candidate handoff,
not a new public release. Windows execution and presenter inspection are not
implied by compile/import PASS. G139 is subsequently accepted at the resolved
MA identification scope, with the later stop retained OPEN. This report does
not itself assert that integration, cleanup or a release has occurred.
