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
- **C** Add 2, 4, 6, 8, 10 and 12 MHz clock choices and numeric higher choices
  instead of multiplier-only labels. Preserve machine/FDC clock separation.
- **D** Investigate [#75](https://github.com/nakatamaho/vaeg/issues/75): identify
  what limits VA2 VB-time TVRAM writes on hardware before modeling it. Preserve
  other TSPSPR rows. Q/S observations (28.0 VA, 30.1 VA2, 41.7 emulated VA2)
  are not a demonstrated root cause or permission for a fitted slowdown.
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

## Progress

[Implementation/evidence checkpoint](../reports/m142_runtime_progress.md)
contains the complete published issue #75 E–W comparison table, commands,
exits, identities, skip/reuse boundaries and remaining gate checks.

- **MacPorts complete Windows build: PASS (cross-build only).** First complete
  prerequisite at `2e662105164d69948d312c673045b406b8e8f59d`; a fresh clean clone
  of pushed `6f94fcb8467c0cf5beb196ea26de0c2e571ced55` now includes runtime fixes.
  Recording/shaders/static dependencies ON, local encoders OFF, no Colima;
  imports/encoder archive/embedded identity audits pass, command exit 0.
  Final executable SHA-256:
  `795ef24174fdc0b040d07715038c7e15c74d9a6ef312228c0acc6563749893d6`.
- **A:** high-digit measurement/format tests pass; no production digit defect
  is demonstrated. Actual reported overclock/presenter-width case unresolved.
- **B/C:** absent expansion defaults and numeric nominal clock choices are
  implemented; native configuration, domain and existing main/FDC tests pass.
  Remaining GUI/restart/save-size checks are not silently marked passed.
- **D/#75: BLOCKED.** No physical limiter has been demonstrated and no fitted
  emulator slowdown is applied. The source-built chunk/entry diagnostic kit
  is ready; its nine COMs assemble, stock TSPSPR matches the distribution and
  pinned MS-DOS system/payload content checks pass. Hardware results NOT_RUN.
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
  baseline diagnostics in 29 files. Full M142 demo/media/live-guest regression
  remains NOT_RUN. Windows execution and final acceptance remain NOT_RUN.
- **M142 IN_PROGRESS; G142 NOT_READY.** This checkpoint does not authorize a
  merge or reopen the accepted G139/G140 scopes.
