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
