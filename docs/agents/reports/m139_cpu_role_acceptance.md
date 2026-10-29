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

# G139 — CPU-role acceptance supplement

## Explicit maintainer acceptance and historical ID collision

The maintainer separately confirmed both scopes that had acquired the same
historical M139/G139 identifier:

- CARIGRAPH is black lines on white on VA, VA2 and the PC-8801mkIISR model: OK.
- The MA demo's VA-identification rejection is resolved: OK, **G139 passed**.

The second statement explicitly accepts the CPU-role work at the resolved
identification boundary. Its current disposition is **G139 SCOPED_ACCEPTED**.
It does not claim that every original end-to-end condition was demonstrated.
The separately accepted main-branch whole-screen-reverse milestone and its
task/roadmap record must remain intact. The CPU task's historical filename,
commit prefixes and stable state/error identifiers are retained for provenance;
this supplement is not a new gate or a silent renumbering of either task.

The acceptance message follows the supplied clean MinGW handoff at584bfcbf.
It does not independently name an executable digest, so do not manufacture a
new private-run identity or broader Windows execution receipt from that text.
The handoff remains bound to its actual source and SHA-256 in the
[QA record](m139_demo_media_handoff.md).

## Accepted work and retained evidence

- [3053ee657eecf0c20aca3b967fa864620b1a2305](https://github.com/nakatamaho/vaeg/commit/3053ee657eecf0c20aca3b967fa864620b1a2305)
  selects an independent main uPD780 for applied non-VA machines while retaining
  uPD9002 execution for VA/VA2 and an independent FDC uPD780.
- Main/FDC state, flags, timing/native-opcode isolation and actual state-file
  round trips retain their documented software checks:138 PASS/10 SKIP, no FAIL
  at the original evaluated implementation, not a newly executed merged tree.
- The retained unmodified MA fixture demonstrates H=1 and result16 at its own
  CPU check. The maintainer now confirms the early rejection is resolved.
- The nine distribution payload comparisons,34 guest-drawing cases, unmodified
  BASIC computation/progress, four native boot contracts and clean static
  MinGW build/import/encoder audit retain their original explicit target limits.
- The separately reported9b60f9c1 M141 build lacks the CPU-role implementation;
  its early rejection remains a historical report, not a candidate regression.
  It must not be confused with the584bfcbf handoff or relabeled as a local run.

The two reported failure stages and their initial evidence remain preserved.
No unchanged emulator behavior is rerun merely to write this acceptance record.

## Carry-forward: open, not fixed or passed

The candidate's later MA terminal red-screen stop remains **OPEN**. Its cause
is not established; the former full-drawing criterion is explicitly outside
this accepted identification scope. Do not mark the entire MA demo, all scenes,
physical timing, presenter/fullscreen or full Windows execution as passed.
Retain the defect, traces and independent receipt identities for follow-up.
This is not automatically the same defect as M142's non-VA MH-demo reset.

The previously scoped G140 VA2 instruction/flag acceptance is unchanged.
Original-VA discrepancies, absolute timing/H/S, independent silicon controls,
further boundary work and the unsubmitted upstream PR retain their recorded
OPEN/DEFERRED/NOT_SUBMITTED statuses. This acceptance does not enlarge that scope
or authorize a vendor refresh.

## Integration and retention requirements

The now-accepted CPU ancestry may be integrated with the already scoped-accepted
G140 work by a non-fast-forward merge into current main, preserving M141's TSP,
TVRAM and state UI changes and the separate reverse-screen G139 record. Validate
the actual merged tree locally before pushing it; do not replace newer main
with the old branch or infer integration from a gate declaration alone.

Keep the584bfcbf executable's original identity. Any newly built merged candidate
must receive its own source/worker identity and clean static MinGW report.
No public release or tag update is authorized.

After the gate record, merge and roadmap record are pushed, clean only eligible
M139 worktrees/local branches whose contents are in origin/main and clean.
Retain user-supplied inputs, remote branches, existing handoffs, shared caches,
M140-dependent workers/master programs and evidence for the open MA defect.
Do not delete another milestone's assets to satisfy this cleanup. M142 A–F are
assigned follow-up work, not implementation or acceptance in this session.

## Completed integration and local revalidation

Accepted-record commit:
[0eb5b0d0](https://github.com/nakatamaho/vaeg/commit/0eb5b0d00ff0b89661c9611501809136f2259c93).
The actual non-fast-forward main merge is
[a9b1d6c0](https://github.com/nakatamaho/vaeg/commit/a9b1d6c02fdbcec5781565a626a0bffa939bf01c),
with parents c598d661 (current M141 main) and0eb5b0d0 (accepted CPU/FDC history).
The separately accepted reverse-screen task and original roadmap row remain
intact; a historical-ID note links this additional CPU task instead of replacing
that milestone. The conflict resolution keeps both correctness-ledger histories.
M141's TSP/TVRAM, sprite sources/image, state UI/load-state and UTF-8 build fix
are byte-identical at the merge in an explicit23-path preservation manifest.
The shared pccore/selftest edits are additive; no accepted M141 code is reverted.

A new native worker built from the clean merged commit gives:

- **CTest154 PASS /8 SKIP /0 FAIL**,162 total,101.20 seconds. Skips remain
  explicitly unrun: hardware encoder probe, optional SST corpus, private BASIC
  boot binding and five upstream xz shell tests. They are not passed by this gate.
- **34/34 new demo draw contracts** on the two OS images, plus unmodified
  MANDEL source-point matches and6-to12 red-prefix-row progress.
- **Four new native boot contracts**: utility FDD, each initialized standalone
  SASI image on its configured model, and self-booting SGP menu.
- **MA identification PASS** again: the guest exports H=1 and stores16, with
  its input disk unchanged. Its later screenshot is red; complete MA drawing
  remains outside the acceptance and the known defect remains OPEN.
- Six content profiles /25 exact-code single-mutation negatives; encoding,
  LF/case/Markdown,49 strict milestone checks, audit and Git whitespace checks
  pass. The full C/C++ formatter still FAILS with659 diagnostics in29 files:
  every diagnostic maps to the existing c598d661 baseline, **zero new findings**.
  Unrelated main formatting is not silently fixed or claimed passing.

The old media-generation inputs are byte-identical under an explicitly closed
set of demo/builder prefixes, so their image/command/hash evidence is retained,
not expensively regenerated for unchanged sources. Newly inherited TSPSPR is
rebuilt and its7338-byte payload exactly equals the committed distribution.
New guest executions use the merged worker rather than relabeling the old
worker's captures. Runtime/worker/commands/corpus/contract/output digests and
raw private evidence stay in task scratch outside Git and hosted CI.

The execution target remains **macOS SDL dummy guest content**, not physical
GPU/Cocoa presenter/Windows execution. Previous firmware qualification and
initialized-backup limits remain unchanged. No hosted CI is used for iteration.

## New clean static MinGW handoff

```sh
tools/release/build-mingw-static-container.sh \
  --commit a9b1d6c02fdbcec5781565a626a0bffa939bf01c --recording \
  --shader-sdk "$AUDITED_WINDOWS_SHADER_SDK"
```

Exit0 on the dedicated Colima daemon, pinned Ubuntu24.04 image. The clean clone
and fresh application objects include **both** the accepted CPU-role work and
current M141 TSP/state changes, unlike the older584bfcbf handoff. Only the
reviewed shader dependency SDK is reused. Recording ON, local bundled H.264/AAC
OFF; static FFmpeg registration audit accepts FFV1/PCM only. Embedded source
identity, Windows-system-DLL-only import audit and output/build-artifact equality
all pass. This is compile/link/audit, not a Windows runtime or presenter receipt.

- Executable: `../vaeg-mingw-a9b1d6c0/output/vaeg.exe`.
- SHA-256: `cd5013ebf83c68335a25b65ad910bcbf04044308d7404c9018973fb94dbda115`.
- Size:59263609 bytes.
- Report: `../vaeg-mingw-a9b1d6c0/output/BUILD-REPORT.md`, with imports,
  toolchain/encoder audit logs, source/relinking material and notices retained.

Absolute paths are supplied to the maintainer separately. Documentation-only
closure commits do not relabel this executable as their own build. The older
handoff remains available for identity comparison. No release tag or published
package is changed.
