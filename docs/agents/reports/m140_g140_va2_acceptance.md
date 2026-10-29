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

# M140 / G140 — scoped VA2 uPD780C-1 acceptance

## Decision and authorization

**G140: SCOPED_ACCEPTED — confirmed VA2 uPD780C-1 instruction/flag contract.**
The maintainer explicitly directed:

> VA2 μPD780C-1の確認済み範囲でG140を締め、VA側の未解決差と残りの検証を後続課題へ分離する

This closes the confirmed VA2 checkpoint, not the original broad conformance
and application gate. It is an explicit scope amendment: remaining checks are
carried forward below, **not converted into PASS**. Historical BLOCKED statuses,
failed attempts and their original evidence remain historical records.

The target is the FDC uPD780C-1, not the uPD9002 main CPU, its uPD70008-compatible
mode, a CP/MVA host test or a generic physical Zilog Z80. Reported machine/mode
labels are not independently encoded chip/firmware identities. This decision
does not assert that every NEC specimen has identical undocumented behavior.

## Accepted evidence

| Confirmed item | Evidence and result | Limit |
|---|---|---|
| Relocated full ZEX | 67/67 observed group CRCs equal the identical-image mapped host | The 29 stock-address differences remain relocation effects shared with the host, not a stock-address 67/67 PASS claim |
| Legacy AF / controlled histories | 256 AF rows and 2048 history rows in the explicitly counted VA2 sets match the host flag contract | Recorded inputs/settings only, not all physical instructions or unique inputs |
| Additional controls | Seed80, fixed160, capture96, crossed128, CP/prefix128 rows match | Eight counted VA2 receipt sets total **2896 AF/history rows**; CP/prefix includes positive A-preservation controls |
| SCF/CCF baseline | Keep X/Y from A, preserve A and documented flags as specified | No speculative Q, OR, random-bit or PC-address workaround |
| Software regression | **393216 AF/operation/neutral-boundary cases**, production configurator and guest POP/PUSH export, PASS | Exhaustive software inputs for that contract, not exhaustive physical history/timing |
| Diagnostic safety/integrity | Prior mapped/ROM H/S, persistence/refusal/fault safety and bounded triple/RAM/mirror checks retain their original passing scope | Recovery markers are not independent physical full-RAM dumps; dummy SDL is not presenter acceptance |

[VA2 baseline](../tasks/M140_fdc_upd780_conformance.md#va2-upd780c-1-instructionflag-baseline-frozen)
and [returned observations](m140_fdc_va_observations.md#returned-fdcpref-controls)
provide the detailed contracts and qualifications. This checkpoint freezes the
already-matching A-only implementation; it does **not** claim a new correction
to NEC arithmetic. The original VA discrepancy remains open.

## Closure validation and reuse

The eight counted VA2 received-image snapshots still match their recorded
receipt digests. Content was revalidated from those snapshots without guest
reexecution: extracted logs match the retained log bytes, all 2896 AF/history
rows satisfy the applicable contracts, and all67 relocated CRCs match the
preserved mapped-host report. No private assets or identities are published.
Prior supplementary-file package discrepancies are not erased by flag acceptance.

The VA2 test binary/source and native FDC/application worker digests are
unchanged, as are the probe/content sources. The passing 393216-case regression,
14 focused tests and costly full-ZEX/ROM observations are reused under their
original identities. Documentation-only closure does not justify replaying them.
Repository encoding/LF/case/Markdown, 49 strict milestone checks, audit and
commit-scope/diff checks validate this record. Exact commands/status, corpus,
worker/program/contract and output digests remain in out-of-tree evidence.
The previously recorded whole-manifest format failures remain pre-existing;
no unrelated formatting is changed or newly marked passed.

## Deferred work — not included in accepted G140 scope

These are carry-forward requirements for a separately authorized future task,
not a new milestone started in this session.

| Carry-forward item | Status at closure | Required before claiming completion |
|---|---|---|
| Original-VA SCF/CCF X/Y differences | **OPEN, unresolved** | Explain and independently validate a rule or retain the mismatch; preserve FDCFIX OR159/160, FDCCAP OR56/96, crossed OR68/128 and the CP-based A-only match |
| Independent physical Zilog reference | **NOT_ESTABLISHED** | Approved independent target/corpus; MAME/SST software is not that measurement |
| Absolute speed/MHz, physical H/S observer coupling | **NOT_ESTABLISHED** | Approved reference/uncertainty and appropriate timing/isolation checks; edge counts and nominal clocks are insufficient |
| Remaining opcode/boundary/interrupt and taken/untaken timing coverage | **INCOMPLETE / NOT_RUN as previously recorded** | Explicit contracts and independent results; no complete-silicon claim from the current subset |
| Standard demo/media regression and clean static MinGW application handoff | **DEFERRED; NOT accepted by this checkpoint** | The original clean-build, static-import, all-demo/media/private-system and application gate obligations before the relevant integration/release acceptance |
| Upstream PR submission | **NOT_SUBMITTED** | API-authenticated submission/review; the tested contributor branch and PR body already exist, but no PR number or upstream acceptance is claimed |
| Zilog Q implementation | **NOT_IMPLEMENTED** | Separate selector/reference/state/prefix/interrupt review; preserve NEC A-only behavior |

The [upstream candidate and ready-to-paste PR](m140_upstream_upd780c1_pr.md)
provide an explicit NEC selector and contract tests. Their source contribution
is pushed; it is not a vaeg vendor refresh or a Zilog-Q fix. Posting the PR remains
a nonblocking administrative follow-up, not a fabricated upstream PASS.

## Integration and retention boundaries

Subsequent integration note: the maintainer has since explicitly accepted the
CPU identification scope in the [separate G139 supplement](m139_cpu_role_acceptance.md).
[Merge a9b1d6c0](https://github.com/nakatamaho/vaeg/commit/a9b1d6c02fdbcec5781565a626a0bffa939bf01c)
integrates both accepted histories while preserving M141; it does not enlarge
the G140 contract or settle its deferred work. The following paragraphs retain
the restrictions **at this original G140 closure**, not a current merge status.

At that closure M139/G139 remained separate and unaccepted. The current topic branch contains
pending M139 main-CPU changes as well as M140 work; scoped G140 acceptance is
**not authorization to merge that ancestry into main** or relabel its application
as a clean handoff. No main merge, release, vendor update or new executable is
performed by this closure. Future application integration must satisfy its own
outstanding gate and clean-build/media obligations.

No working directory, branch, build, handoff, private receipt or cache is removed.
The branch is not contained in origin/main, so normal post-merge cleanup
prerequisites are not met. Existing evidence and supplied files stay available
for the deferred investigations. Do not begin another milestone without a
separate assignment.
