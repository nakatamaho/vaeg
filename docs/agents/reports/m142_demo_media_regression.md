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

# M142 — fresh demo/media closure and live guest regression

**M142 IN_PROGRESS; G142 NOT_READY.** Nine demo media closures, 34 coloured
program-drawing cases, unmodified MANDEL.BAS progress and the self-booting SGP
menu now have new M142 evidence. Utility media remain blocked/not run; #75 is
still OPEN with correction NOT_IMPLEMENTED. This is not a human-gate request.

Task, build commands and public image SHA-256 values:
[M142 demo/media follow-up](../tasks/M142_runtime_portability.md#demo-and-media-regression-follow-up).
Earlier [state/build preparation](m142_gate_preparation.md) is retained as its
own evaluated checkpoint, not relabeled as this run.

## Evaluated inputs and target

- Fresh, initially clean detached clone of pushed media-source commit
  [`33dcb8ba9091cdf93eab8e1399478d5508c3e593`](https://github.com/nakatamaho/vaeg/commit/33dcb8ba9091cdf93eab8e1399478d5508c3e593).
  Tracked files and original distribution archives remain unchanged. The six
  individual distribution scripts leave only new, untracked test archives in
  this out-of-tree task clone; they are not staged or published.
- Actual native worker source:
  [`0d02bc849b5c4fb4f45d21c13716735e77578d37`](https://github.com/nakatamaho/vaeg/commit/0d02bc849b5c4fb4f45d21c13716735e77578d37),
  SHA-256 `24f53861e4a81f0914d489c353921113cc81e0aa1f730f00b9a9b07deea70700`.
  This is the existing recording-enabled, local-encoders-OFF, shaders-OFF QA
  worker, not a new application build of `33dcb8ba`.
- Application C/C++/header/resource inputs, CMake modules, external sources,
  demo sources and media tools match between those source commits. The only
  top-level CMake change is the previously recorded CTest resource lock, not
  application compilation. No unchanged emulator, SDK or expensive CPU suite
  is rebuilt/rerun for this evidence-only work.
- Runtime target: **native macOS, SDL dummy/software guest-frame capture**,
  VA2, nominal H/8 MHz, 24.8 kHz monitor selection, SGP normal speed, expansion
  boards OFF. Each worker has its own copied disk, INI, HOME, input script,
  logs and captures; all other FDD/SASI/SCSI slots are explicitly empty.
- Input bindings, ROM identities, supplied assets, bootable private overlays,
  raw captures and their hashes remain outside Git. Public tables describe
  freely distributable outputs only. Original private inputs are rehashed
  unchanged at finalization. No additional photographs or setup are requested.

### Isolated regeneration tools

The installed system ports and global toolchain are untouched. A task-local
NASM **2.16.01** is built from the source archive with SHA-256
`c77745f4802375efeee2ec5c0ad6b7f037ea9c87c92b149a9637ff099f162558`,
cross-checked against Debian source metadata fetched over HTTPS. No signature
verification is claimed. The isolated Python environment is **3.12.13** with
**psd-tools 1.26.0, Pillow 13.0.0 and NumPy 2.5.4**; the installation report
and tool digests are retained. Restricted PATH selects these tools and omits
ImageMagick `convert`, so the external source asset is rendered through the
existing psd-tools path. There is no Colima/container fallback or port change.

## Fresh media and source closure

All nine builders exit **0**. Every new compressed image decompresses exactly
to its generated raw D88. Recursive path/size/payload-digest maps equal the
committed distributions for GLASS, NEON3, NEON4, pseudo-sprite, wireframe,
Zundamon, all-demos and all-demos-msdos4. The self-booting SGP image is compared
as a whole, including IPL, loader and payloads.

**all-demos-msdos4 and sgp-boot are also whole-image byte-identical** to their
committed references. The other seven results are payload equality, not a
claim that directory timestamps/layout and all raw image bytes match. The
freshly regenerated Zundamon COM and data both match; no reference payload is
substituted for a failed asset conversion. No distribution needs updating.
A new PC-Engine bootable combined disk is also built locally, exit **0**, for
runtime cases; it is not redistributed or included in the public hash table.

Ten additional build entries exit **0**: stock TSPSPR, SGP contention, Z80
timing, VA Voyage, VA Voyage OPNA, both 16-colour SGP scans, the 65536-colour
SGP triangle/scan, and both root wireframe build scripts. Fresh stock TSPSPR
also equals the earlier kit payload byte-for-byte. These are build results,
not ten additional live-execution or timing-conformance claims.

## Live drawing: 17 programs on each OS

Both freshly generated bootable combined disks are actually booted. Guest
scripts change to each program's directory and launch it. A pre-command frame
at 550 and two later completed frames feed the repository's independent
[content validator](../../../tools/qa/all_demos_draw.py). Ordinary PC-Engine
captures are at 2400/4800; MS-DOS captures at 4200/6600 include an explicit
1200-frame pause after CD. Zundamon uses 12000/18000 for its longer startup.

| Program group | Programs | PC-Engine | MS-DOS 4.0 |
|---|---|---:|---:|
| GLASS | GLASS | 1/1 | 1/1 |
| NEON3 | NEON200, NEON400 | 2/2 | 2/2 |
| NEON4 | 16- and 65536-colour NEON4 | 2/2 | 2/2 |
| 16-colour pseudo-sprites | SGPD_7A/B/C/D/S | 5/5 | 5/5 |
| 256-colour pseudo-sprites | SGP256S, SGP256T | 2/2 | 2/2 |
| 65536-colour pseudo-sprite | SGP655S | 1/1 | 1/1 |
| Wireframe | 16-, 256- and 65536-colour SGPWIRE | 3/3 | 3/3 |
| Zundamon | ZUNDAORB with its generated data | 1/1 | 1/1 |
| **Total** | | **17/17** | **17/17** |

All **34 cases PASS_DRAW_CONTRACT** on final content validation. The normal
scene contract requires at least 4096 changed pixels, 1024 coloured changed
pixels and three nonblack colours. SGP256S and SGP655S use the existing
source-derived single-24x24-sphere profile, 256/128 pixels; no threshold is
weakened. Worker exit 0 means capture completion, not program completion or
semantic PASS by itself. This checks guest drawing, not exact scene identity,
all animation phases, presenter/window/fullscreen, Cocoa, GPU or HiDPI output.

### Unmodified BASIC and self-boot

- **MANDEL.BAS: PASS_PROGRESS_CONTRACT.** The fresh PC-Engine overlay contains
  precisely the source program with required CRLF line endings, not a shortened
  iteration bound. `BASIC /87`, `LOAD "MANDEL.BAS"`, then `RUN` produce the
  source-derived first-row red/orange/red points at x=0/320/639. The qualifying
  red-prefix row count increases **12 to 23** at frames 10000/16000. This is
  genuine BASIC drawing progress, not full fractal completion or an absolute
  speed measurement.
- **Self-booting SGP: PASS_BOOT_CONTRACT.** The newly built disk boots alone.
  Its live native TSP text table at frame 900 contains the pseudo-sprite,
  wireframe, SGPTEST, SGPDUMP and launch-prompt menu contract. This is menu boot
  coverage, not execution of every self-boot menu item.

## Retained failures and completed-run reuse

The final result is **completed workers plus corrected content validation**,
not an original all-in-one exit-0 campaign:

1. The first two smoke workers exit 0, but the adapter passes a Path instead
   of bytes to `read_png`; its reduction exits 1. Original source/logs/captures
   remain. Independent decoding confirms PC-Engine GLASS drawing; the initial
   MS-DOS case does not show its launched program and fails the draw contract.
2. A new MS-DOS case with the explicit post-CD pause passes. The good PC-Engine
   smoke evidence is reused. This is an automation input-schedule correction,
   not a demonstrated emulator/keyboard defect or explanation of past physical
   SPACE operation. The initial failed MS-DOS case is not promoted to PASS.
3. The full drawing adapter exits 1 with two `G139_DEMO_DRAW_MISSING` results:
   it incorrectly assigns SGP655S the large-scene profile. Both source programs
   declare a single 24x24 sprite, and the retained frames show the sphere.
   Selecting the existing sphere24 contract rechecks the same captures; no
   guest is rerun and no content threshold changes. Final 34-case reduction
   exits **0**; the failed aggregate and old adapter sources are retained.
4. The additional runner completes MANDEL, self-boot and four source builds,
   then incorrectly executes the documented `bash build-opna.sh` directly and
   gets PermissionError. Its exit 1 remains. Recovery verifies/reuses completed
   outputs, invokes that script through bash and completes the remaining source
   builds. Both successful guest workers are revalidated, not rerun.

The original content selftest passes **six profiles / 25 single-mutation
negatives**. Final independent rechecking exits **0** after checking tool,
worker, input, per-run script/config/log/output, all nine image closures,
34 case results, both special guests and ten build-entry identities. It starts
zero guest workers. Exact commands, source/contract identities and failed
attempts remain in the task-specific evidence, not in Git.

## Utility media and remaining gate blockers

The actual utility-FDD command exits **1** at prerequisite validation:
`required host command is missing: dosbox`. Host PATH probing also finds no
DOSBox; this is not merely the deliberately restricted asset-rendering PATH.
No utility FDD is produced. Both SASI generations and all three utility-media
boot checks are **NOT_RUN**, not PASS or guest boot failures.

The existing utility recipe also invokes the Open Watcom container-only
SQEMM builder. That backend is not invoked under M142's no-container-fallback
policy; installing DOSBox alone is not declared sufficient. No default Docker
daemon is queried, no environment is changed and no legacy image is substituted
for fresh generation. The complete mandatory media regression therefore
remains **BLOCKED**, despite the nine demo closures passing.

The current macOS API preflight still returns zero active displays, screen
capture disallowed and accessibility untrusted. Actual FPS/layout and GUI-driven
settings checks remain blocked/pending, not covered by these dummy captures.
Windows runtime and real Windows/Linux state exchange remain NOT_RUN; the
recording/shader-enabled Windows artifact remains the previously audited
`ea62eda1` build. #75 cause/correction are unchanged. No merge, release, new
Windows build, hosted CI, additional hardware test or gate acceptance is claimed.
