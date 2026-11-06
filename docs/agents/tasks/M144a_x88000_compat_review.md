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
EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.
-->
# M144a — X88000 compatibility-feature source review

Status: **IMPLEMENTATION_IN_PROGRESS; G144a human gate PENDING;
publication remains BLOCKED_AUTHENTICATION_NOT_PUSHED.**
Branch: `milestone/m144a-x88000-review`. Evaluated vaeg main:
[dc023c9b7ce62c7334d797ba1934ad95b51fd3fd](https://github.com/nakatamaho/vaeg/commit/dc023c9b7ce62c7334d797ba1934ad95b51fd3fd).
This is a read-only compatibility follow-up to accepted M144, separate from
paused WSR-E and M145 work.

## Authorized implementation extension

After receiving the source review, the maintainer explicitly requested implementation
of the 32h GVAM transition and E6h unmask-request changes, PC-8001mkII/SR
emulation, a main-clock-independent nominal 4 MHz FDD CPU, and PC-PR201 output
to PDF (including operation with an 8 MHz main CPU). This extends this same
milestone; it does not declare the previous review accepted or start another gate.
The earlier investigation-only restrictions below describe the completed review
checkpoint, not this newly authorized implementation phase.

Keep separate commits for independently testable concerns. PC-8001 graphics,
ROM banks, mode selection and state persistence must be implemented together
rather than merely adding model names. PR201 requires a guest printer input
path, bounded command/page processing and actual PDF content validation, not
renaming a raw print stream. No ROM/font payload may be bundled. Keep VA
hardware behavior distinct from independent PC-8001/8801 selections.

The FDD clock report is a requested verification/correction, not yet a diagnosed
vaeg defect: current `Subsystem::Reset` scales FDC cycles by the fixed machine
multiple, independently of the configurable main CPU multiple. Verify guest
execution at 4/8 MHz before deciding whether a production correction is needed.
Nominal 4 MHz in the VA clock domain means 3.9936 MHz, not changing the crystal
base to 4,000,000 Hz without evidence.

Required implementation checks: focused synthetic I/O/memory/interrupt tests,
FDC execution-count comparison, model/reset/state regression, printer parser
bounds and PDF page/content/error cases, applicable CMake/CTest builds and
repository checks. Preserve failed tests. A clean static Windows handoff and
human acceptance are still required before the implementation gate passes.
Full media/presenter campaigns require a separate explicit request. No merge,
release, hardware probe, deletion or resumption of WSR-E/M145 is authorized.

## Initial port implementation checkpoint

Implemented compatible-mode GVAM edge reset and E6h newly permitted request
clearing. Native GVAM latch behavior is unchanged. E6h does not revoke a vector
already offered to the CPU: that existing acknowledge transaction remains
owned by the CPU bridge. This is an explicit VA-adapter boundary, not a claim
of byte-for-byte transplantation of X88000's interrupt model.

Added 32 GVAM transition/mode/plane cases and 256 E6h mask/pending cases to
production-path ROM-less selftests. Updated the state fixture to select its
nondefault plane after the GVAM edge. No state format changes.

Local QA: fresh native Debug configure/build exit 0 (recording ON, static
FFmpeg, bundled MP4 encoders OFF; QA-only profile with shaders OFF).
`ctest --test-dir <task-native-build> -R vaeg_romless --output-on-failure`:
1/1 PASS, 41.66 seconds. Log contains `8214 mode ok` and
`GVAM transitions 32 cases ok`. Initial configure rejected missing trace
instrumentation; corrected with `VAEG_Z80_COMPAT_INTEGRATION_TRACE=ON`, with
both logs preserved. No mutation check, full CTest, Windows build, new guest
media execution or hardware verification has yet been performed.

**Remaining:** full PC-8001mkII/SR graphics/ROM/bank/state integration,
production FDC execution-count verification, PC-PR201 command/parser/port
integration and PDF export. None of these is marked implemented by this
port-only checkpoint. Main/WSR-E/M145 remain unchanged.

## Request and scope

The maintainer requests source investigation, **not fixes**, of eight X88000
features and accumulation of findings under the existing `docs/modernization/`
directory (the requested `modanization` spelling is not a new directory):

1. 1.5.1: port 32h GVAM transitions reset the independent access selection.
2. 1.5.1: port E6h enabling a mask clears an interrupt request.
3. 1.5.1: monochrome text reverse colours.
4. 1.5.1: CRTC START DISPLAY DM reverse bit.
5. 1.5.3: variable text attribute count.
6. 1.4.0: HAL Laboratory PCG-8800.
7. 1.4.0: overseas mode (not English host UI).
8. 1.3.2: wait emulation (not host pacing or display frame skipping).

Use pinned public reference sources and identify unavailable historical
releases. Distinguish changelog statements, static source findings, synthetic
checks, prior accepted tests and new runtime evidence. X88000 models PC-8801,
not physical VA compatibility logic; source agreement is not hardware proof.
Do not connect any difference to the deferred demo transition artifact without
reproduction and causal evidence.

## Deliverable and restrictions

An independently written Japanese report, with an eight-item comparison table,
exact functions/source references, boundaries and follow-up tests. Reference
archives/source views, command failures, hashes and working notes stay in
out-of-tree task scratch; no ROMs, fonts, private media or captures are needed.
No emulator/source/configuration change, foreign emulator execution, hardware
I/O, new build, media campaign, merge, release or cleanup is authorized here.
Existing PCG acceptance remains historical, not rerun evidence.

## Checks and review

Run focused document/link/source-scope checks, encoding, EOL, case, Markdown
comments, milestone-ID selftest/audit and Git whitespace checks. Any synthetic
source experiment must identify its exact inputs and must not be presented as
production-path or hardware execution. No emulator rebuild is warranted by a
report-only change. Human review of the findings is pending; no new gate PASS
is inferred from publication. Do not resume either paused hardware task.
G144a for this report-only task is maintainer review of the source findings,
not a new emulator-build/runtime/hardware acceptance gate.

## Recorded findings

[Japanese investigation report](../../modernization/x88000-compatibility-review.md).
The eight items now have source functions, classifications, evidence boundaries
and proposed follow-up checks:

- X01/X02: concrete source-level differences in GVAM transition selection and
  E6h unmask request clearing. Physical VA expectations and guest impact remain
  unverified; these are not newly diagnosed or fixed guest defects.
- X03/X04/X05: corresponding monochrome composition, global reverse and
  variable-attribute rendering already exist. End-to-end ROM translation,
  nonstandard counts and transitional display coverage are not newly passed.
- X06: PCG is already accepted in M115/M115a, with intentional differences from
  X88000. Current main's default-ON policy is retained.
- X07: international mode means optional 8x8 font selection/fallback and PCG
  interaction, not English host UI. There is no equivalent separate feature.
- X08: X88000 has M1/memory/CRTC contention waits. vaeg's measured VA-compatible
  costs and native waits are not that model; independent PC-8801 main uPD780C
  timing must be reviewed separately. No wait constants were transplanted.

## Reference and local evidence record

The official 1.5.3 archive matches ADR-0016's published SHA-256. The 1.5.1
comparison uses public mirror commit
[1606677251c49c5d980953e13b42be5812ea99e7](https://github.com/nakatamaho/x88000/commit/1606677251c49c5d980953e13b42be5812ea99e7).
Constructed older official archive URLs returned HTTP 404; exact historical
1.4.0/1.3.2 implementations were not obtained. Their introduction history and
surviving 1.5.3 behaviour are explicitly distinguished.

Task-local inputs and receipts are retained out of tree: page/archive fetch
records, the public-source checkout/archive, 232 original/UTF-8-view pairs and
hash bindings, the ten-file version comparison, static audit source/output,
repository-check logs and publication notes. No private assets are involved.
The source-view manifest SHA-256 is
`a06cb3b3f01eeebc16ecde5f21944ddfa0c7ce93d0d347696152d8080377dbb7`.
No sources, archives or raw source extracts are committed.

The focused audit checks all 232 original/view hashes, nine unchanged selected
files and the attribute-renderer difference, thirteen function-body source
facts, pinned source/link line bounds and the closed three-document scope.
The monochrome eight-case truth table is **source-derived arithmetic only**;
no production function, emulator or hardware was executed. The final link count
is recorded with the final audit rather than treated as a frozen report schema.

Preserved preliminary failures: the source-link audit rejected an overlong
PCG source-line range; the range was corrected from 242 to the actual 234.
The first repository audit passed 49 milestone-ID selftests but rejected the
new roadmap row because its pending G144a ID was missing; the row was corrected.
Neither is an emulator defect, and the failed logs are not relabeled as PASS.

## Scope and review disposition

Only the new report, this task and the one-row roadmap addition are changed.
No emulator source/configuration, build metadata, dependency, binary, media or
private-input identity changes. No bug-fix ledger entry is added from a static
reference difference without demonstrated incorrect guest-visible behaviour.
Archived reference-tier behaviour/provenance is unaffected. No hosted CI,
media/demo campaign, screen automation, merge, release or cleanup was requested
or performed. WSR-E and M145 remain paused and untouched.

A future main merge still requires explicit maintainer acceptance and the
campaign-choice question required by AGENTS.md. This publication does not
supply either. The existing deferred scene-transition artifact remains
OPEN/DEFERRED and is not attributed to any finding in this report.

## Publication status

The task-scoped HTTPS push preflight exited 128 with
`fatal: unable to get password from user`. No usable authentication is available
within the permitted task boundary. Credential helpers and interactive prompts
were disabled for that attempt; no credentials outside the allowed tree were
read or copied. The source review is retained locally, **not pushed or merged**.
The failed receipt is preserved out of tree. Resume publication only with usable,
permitted authentication; do not claim a GitHub branch/PR was created.

## Local verification

All final checks below exited 0. The task-local static audit reported
`PASS_STATIC_REVIEW_AUDIT_ONLY`: 232 original/view pairs, 13 function bodies,
42 document/source links and 8 monochrome source-derived arithmetic cases.
Pinned source links were checked against local blobs and line ranges; this
is not a new HTTP-availability check for every GitHub link. No emulator build
or runtime test was run for this documentation-only change.

```text
python3.12 -B tools/repo/check_encoding.py --expect utf8
0 violation(s)
python3.12 -B tools/repo/check_eol.py --enforce
0 violation(s)
python3.12 -B tools/repo/check_case.py
0 finding(s)
python3.12 -B tools/repo/check_md_comments.py
0 finding(s)
python3.12 -B tools/qa/milestone_ids.py --root . --selftest --discover --audit
milestone-id-selftest: 49 strict acceptance/rejection checks passed
milestone-id-discovery: tasks=226 reports=178 roadmap_rows=167
milestone-id-audit: ROADMAP is declarative; task/report discovery, gate, branch, commit-prefix, artifact, and CI tooling accept strict canonical lettered identifiers
git diff --check
(no output)
git diff --cached --check
(no output)
```
