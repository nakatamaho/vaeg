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
# uPD9002 Semantics Migration Specification (v4.3)

Repository: `github.com/nakatamaho/vaeg`

Status:

- Repository milestones M52-M56 were consumed by unrelated work and remain historical.
- The maintainer treats G56 as passed because the intended implementation was not feasible.
- This semantics campaign begins at M57. Before any M57 commit, resolve and verify the exact approved
  G56 SHA from the repository's approved G56 report/ROADMAP; do not guess it from current HEAD.
- This document is the fixed campaign specification for M58 onward.
- Codex must execute exactly one milestone or lettered submilestone per session and stop at its gate.

## 1. Current state

- M50 completed deletion of the 286 protected-mode execution machinery. Do not repeat that work.
- M51 completed ownership and naming migration to `cpu/upd9002/`, `upd9002_core_*`,
  `upd9002_dispatch_*`, and `upd9002_regs_*`.
- Active production sources still contain 286-derived handler files and `I286_*` macros. These
  remain until their owning semantic families are green and the final M66b sweep is reached.
- G56's "implementation impossible" acceptance does not waive any semantics-campaign requirement.
- The immutable M43 SST v20 architectural full-profile baseline contains 84,329 failures.
  The deterministic CI profile contains 10,593 failures at approved G51 SHA
  `78c712f0bab53a6960cfc102eae7ee54b3fc29ef`. Before deletion, M57 must reproduce the same
  dataset identity, classifications, pass/failure hashes, signatures, and termination classes at
  the exact approved G56 predecessor. Any drift is a hard stop; do not silently rebaseline.
- M43 summaries and failure sidecars are immutable evidence and must remain byte-identical.
- Real-hardware access is scarce. SST v20 is the primary architectural-state oracle, but an SST
  match is not proof of exact uPD9002 silicon behavior.

## 2. Mission

Replace the remaining 286-derived instruction semantics with V30-class semantics validated against
SST v20 until the blocking architectural full profile has zero failures among records currently
classified `applicable`.

Unconfirmed uPD9002 questions remain visible in `hardware_pending.json`; they are never used to
excuse a failing applicable case or to claim complete silicon accuracy.

## 3. Non-goals

- Cycle counts, prefetch modeling, bus timing, and SST V20 bus traces.
- Z80 compatibility-mode implementation.
- Real-hardware CRC execution.
- Re-deletion of M50 protected-mode paths.
- Rewriting immutable M43 artifacts, historical reports, or provenance records.

## 4. Oracle profiles

### 4.1 Architectural profile — blocking

Compare:

- metadata-masked defined bits of the final FLAGS register;
- final general and segment registers and IP;
- SST-represented final RAM bytes, byte-exact;
- architectural termination class.

Exclude cycles and prefetch.

The denominator is exactly:

```text
blocking_denominator = count(current top-level classification == applicable)
architectural_pass_rate = applicable_pass / blocking_denominator
```

`known_target_gap`, `expected_target_divergence`, `unsupported_fixture`, and
`upstream_nonblocking` are reported separately and never counted as passes.

### 4.2 V20 silicon fingerprint — diagnostic

Compare all 16 bits of the final FLAGS register. This profile provides corroborating evidence for
V20-observed behavior. It is non-blocking unless a later explicit approval changes that policy.

## 5. Guest-visible FLAGS materialization

PUSHF, interrupt/fault frames, and LAHF materialize FLAGS into byte-exact RAM or a general register.
POPF, IRET, and SAHF consume guest-visible images.

- Never mask frame FLAGS bytes, PUSHF stack bytes, or LAHF's AH result.
- Derive an interrupt/fault pushed image from SST expected RAM frame bytes, not from the final FLAGS
  register alone.
- Derive PUSHF and LAHF images independently. Do not reuse the interrupt-frame convention by
  analogy.
- Record adopted images as V20-compatibility conventions. Unconfirmed uPD9002 commonality belongs
  in `hardware_pending.json`.

## 6. Ground-truth discipline

- Do not modify a fixture or harness merely to improve the score.
- A fixture correction requires concrete case hashes, written justification, and a transition
  audit comparable to the G43 OUTS correction.
- Derive semantic conventions from aggregate SST evidence and cite case hashes.
- NEC primary documentation is secondary corroboration. Intel 8086/286 documentation is not an
  authority where SST evidence exists.
- Absence from a failure distribution is not proof of passing. Verify top-level classification and
  executed count before using a form as a green reference.
- Do not infer ordered writes, transient writes, RMW ordering, or rollback behavior from SST final
  RAM data.

## 7. PR, commit, and gate discipline

- One primitive or one instruction family per semantic PR.
- One semantic PR equals one gated milestone or lettered gated submilestone.
- Do not stack a semantic PR on an unapproved semantic PR.
- Evidence-only, documentation-only, and rename-only follow-up commits may be in the same PR.
- After transition evidence is generated, add no further semantic change to that PR.
- Rename historical handlers only after the owning family is green, in a rename-only commit.
- After a shared primitive changes, regenerate the full distribution and re-rank later work.
- Every task ends with a report and a human gate; stop after reporting.
- All newly authored repository code, comments, identifiers, commit messages, and documentation are
  in English.

Before creating the first lettered task or gate, M58 must audit repository tooling for numeric-only
milestone assumptions. Canonical forms are:

- task: `M60a_<name>.md`
- report: `m60a_<name>.md`
- branch: `topic/m60a-<name>`
- commit prefix: `M60a:`
- gate: `G60a`

If tooling cannot support this unambiguously, renumber all affected future tasks contiguously with
integer IDs before semantic work begins. Do not mix naming schemes after work starts.

## 8. Ratchet

The predecessor is always the exact previously approved gate SHA, never a mutable file in the
current worktree.

Required invariants:

1. `current_failure_hashes - previous_failure_hashes` is empty.
2. Per-opcode pass counts do not decrease.
3. Dataset identity and comparison-contract identity match.
4. Timeout and crash counts are zero.
5. A current golden cannot authorize itself.
6. The candidate is compared against an explicitly resolved approved predecessor SHA.

Permitted classification transitions:

- `known_target_gap` with `gap_kind=implementation_missing` to `applicable`, only in the
  implementation PR, with every newly applicable hash passing in that PR;
- `applicable` to `expected_target_divergence`, only for records failing in the approved predecessor,
  with explicit approval, qualifying target evidence, and an exact matching entry in
  `approved_target_divergences.json`.

Forbidden:

- previously passing record to `expected_target_divergence`;
- `applicable` to `known_target_gap`, `unsupported_fixture`, or `upstream_nonblocking`;
- outcome-based splitting of a known-gap entry.

A known-gap entry may be split before implementation only by an evidence-based structural selector
such as opcode, subopcode, ModR/M form, prefix class, or documented operand form.

## 9. Transition artifacts

Commit chain:

```text
approved predecessor
  -> semantic commit
  -> transition generation
  -> evidence-only commit containing the artifact
  -> human gate
```

The artifact records `evaluated_sha`, the semantic commit being measured. It never attempts to
contain the SHA of the commit that contains the artifact.

Required identity fields include:

```json
{
  "before_gate": "G60a",
  "before_sha": "40-hex-approved-predecessor",
  "evaluated_sha": "40-hex-semantic-commit",
  "profile": "architectural",
  "scope": "full",
  "dataset_id": "...",
  "comparison_contract_sha256": "...",
  "selected_hash_set_sha256": "...",
  "applicable_hash_set_before_sha256": "...",
  "applicable_hash_set_after_sha256": "...",
  "newly_passing": [],
  "newly_failing": [],
  "changed_failure_count": 0,
  "changed_failure_shards": [],
  "classification_changes": [],
  "scoreboard_before_digest": "...",
  "scoreboard_after_digest": "..."
}
```

Changed failures are fully enumerated in deterministic gzip shards with count and SHA-256. A changed
signature is not automatically an improvement and requires human review.

## 10. Classification and evidence governance

M43 top-level categories remain:

- `applicable`
- `known_target_gap`
- `expected_target_divergence`
- `unsupported_fixture`
- `upstream_nonblocking`

For `known_target_gap` only, M58 adds exactly one `gap_kind`:

- `documented_silicon_absent`
- `implementation_missing`
- `target_support_unverified`

Every `target_support_unverified` entry must be covered exactly by a matching content-addressed entry
in `hardware_pending.json` with identical selector, resolved hash set, count, and sorted-hash digest.

`approved_target_divergences.json` backs every approved transition to
`expected_target_divergence`. `hardware_pending.json` is an orthogonal registry and never changes a
record's classification or removes it from the blocking denominator.

Each entry in either registry includes:

- structural selector;
- exact resolved case hashes;
- resolved count;
- sorted-hash digest;
- reason and evidence;
- first introduced milestone;
- review status.

No open-ended entries or generic mismatch buckets are allowed.

## 11. CI and full-profile enforcement

Hosted PR CI runs the deterministic architectural CI profile and enforces the ratchet where the
verified corpus is available. An explicit hosted external-data skip never satisfies a milestone.

Every milestone gate runs both verified CI and complete full profiles against the exact approved
predecessor. Missing corpus, skipped full execution, or digest mismatch is a hard gate failure.

The 84,329 baseline and campaign completion refer to the architectural full profile.

## 12. Scoreboard epochs

M58 adds new artifacts and does not rewrite M43 evidence:

```text
tests/ssts/epochs/g43/
tests/ssts/scoreboard/g58_architectural_ci.json
tests/ssts/scoreboard/g58_architectural_ci_failures/*.json.gz
tests/ssts/scoreboard/g58_architectural_full.json
tests/ssts/scoreboard/g58_architectural_full_failures/*.json.gz
tests/ssts/scoreboard/g58_fingerprint_full.json
tests/ssts/scoreboard/g58_fingerprint_full_failures/*.json.gz
tests/ssts/transitions/
```

Each summary records `evaluated_sha`, not a self-referential gate SHA, plus:

- `epoch_gate=G58`;
- approved G57 predecessor gate and SHA;
- profile and scope;
- blocking boolean;
- comparison-contract ID and SHA-256;
- dataset ID;
- selected and applicable hash-set digests;
- separate M43 CI/full summary and failure-index digests.

The evidence-commit SHA and approved G58 SHA belong in the gate report.

## 13. Definition of done

1. The architectural full profile has zero failures among all current `applicable` records.
2. Every non-applicable category is separately counted and content-addressed; none is presented as
   a pass.
3. No `implementation_missing` or unclassified record remains.
4. Every remaining `target_support_unverified` hash has exact `hardware_pending.json` coverage.
5. Approved divergences have target documentation or real-hardware evidence.
6. Every previously passing hash remains passing unless an exact approved classification transition
   exists.
7. No active production uPD9002 declaration, definition, dispatch target, source basename, or macro
   uses `I286` or `i286c` identity.
8. Historical reports and immutable evidence remain unchanged for textual hygiene.
9. `win9x/`, `i286x/`, `hlp/`, and `cpuxva/memoryva.x86` are absent while required legal and
   provenance evidence remains in current HEAD.
10. A zero-failure SST result is never represented as complete uPD9002 silicon validation while
    `hardware_pending.json` is non-empty.
