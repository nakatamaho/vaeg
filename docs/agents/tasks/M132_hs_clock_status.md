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

Status: **Implementation started; G132 not requested or passed.**

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

## Implementation sequence

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
