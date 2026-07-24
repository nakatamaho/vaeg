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
# M59 — Produce the uPD9002/V20 semantic evidence pack

## Mandatory preparation

Before doing any work:

1. Read `AGENTS.md`, `docs/agents/ROADMAP.md`, and
   `docs/agents/CONVENTIONS.md`.
2. Read `docs/agents/UPD9002_SEMANTICS_MIGRATION.md`, this task, and
   `docs/agents/reports/m58_upd9002_ssts_ratchet.md`.
3. Inspect the approved G58 scoreboards, transitions, G43 references,
   classifications, taxonomy, and registries.
4. Record the repository state and tool versions.
5. Use a clean dedicated worktree based exactly on the approved predecessor.
6. Reproduce architectural CI/full and fingerprint full before editing.

All newly authored source, comments, identifiers, filenames, commit messages,
test names, schemas, and documentation must be in English.

## Predecessor and identifiers

- Approved predecessor: G58 at
  `bc8a55c6da1082b85b794068e0d933e31fe46b13`
- G58 artifact `evaluated_sha`:
  `d384dbc4d0b7dcece50499a9847b2bd9eb8b3d53`
- Branch: `topic/m59-upd9002-evidence-pack`
- Commit prefix: `M59:`
- Candidate gate: G59

The approved gate SHA and the implementation SHA recorded by the G58
artifacts are intentionally different. Do not rewrite approved evidence.

## Goal and scope

M59 is an evidence and analysis milestone. It must not change production CPU
semantics, SST fixtures, comparison semantics, classifications, approved G58
artifacts, or immutable G43/M43 evidence.

Allowed work is limited to read-only extractors, deterministic generators,
versioned schemas, content-addressed evidence, validators, negative tests,
analysis documentation, and the M59 report.

In particular, do not modify:

- `cpu/upd9002/`;
- active handlers, decode, dispatch, effective-address, register, FLAGS,
  interrupt, stack-frame, termination, or RAM-write behavior;
- selected or applicable hash sets;
- top-level classification or gap taxonomy;
- `approved_target_divergences.json` or `hardware_pending.json`;
- approved G58 scoreboards, sidecars, transitions, or G43/M43 evidence.

If a semantic or harness change appears necessary, stop instead of including
it in M59.

## Approved-state reproduction

Run without skip:

- architectural CI: 10,593 failures;
- architectural full: 84,329 failures;
- fingerprint full: 1,257,109 pass and 186,767 fail;
- zero timeouts and zero crashes.

Counts alone are insufficient. Prove exact dataset and comparison-contract
identity, selected/applicable/pass/failure hash sets, failure signatures,
termination classes, classifications, taxonomy sets, scoreboards, sidecars,
transitions, and immutable G43 references. Any drift is a hard stop.

## Evidence-pack contract

Create a deterministic, versioned, content-addressed pack under
`tests/ssts/evidence/g59/`. Its manifest records:

- the M59/G59 and approved G58 identities;
- the G58 artifact `evaluated_sha`;
- the M59 analysis implementation SHA;
- corpus and comparison-contract identities;
- selected and applicable hash-set digests;
- generator and environment versions;
- every artifact path, byte count, row count, and SHA-256 digest.

The manifest records the analysis implementation commit, never its containing
evidence commit. JSON and compressed artifacts must be canonical and
deterministic in the recorded environment. Every item has both a complete
machine table keyed by SST case hash and a readable representative dump.

Every row records initial, expected, and actual architectural state side by
side, including execution/termination, registers, final RAM, mismatch kinds,
classification, structural partitions, and logical/physical addresses when
derivable. Every material conclusion is labelled exactly `proven`,
`hypothesis`, or `underdetermined`.

## Required analysis

Analyze the complete selected populations for:

1. CC/CD/CE and 9C/9D/9E/9F FLAGS materialization;
2. F7 /2, C6, and C7 canary forms;
3. D4 and D5;
4. 0F28 and 0F2A;
5. C0/C1/D2/D3 rotate references and shift subforms;
6. FF /7 and BOUND.

### FLAGS materialization

Derive interrupt-frame placement and pushed FLAGS images from expected and
actual state, separating normal, segment-boundary, physical-boundary, and
underdetermined mappings. Derive PUSHF and LAHF independently. Classify each
materialized bit as copied, forced-zero, forced-one, condition-dependent, or
undetermined. Derive POPF/SAHF loadable, preserved, forced, and underdetermined
bits, without claiming a shared primitive unless the evidence proves one.

### Canary cluster

Analyze F7 /2 independently from the other F7 subforms. Partition F7 /2, C6,
and C7 by register/memory form, addressing mode, displacement/immediate width,
segment, offset and physical wrap, termination, destination bytes, and FLAGS.
Do not infer a shared primitive from correlated failures alone.

### D4 and D5

Analyze D4 and D5 separately and include immediate strata 0, 1, 2, 9, 10, 11,
16, and 255. Determine D4 immediate-zero behavior from SST state rather than
Intel documentation. Prove D5 selection, execution, and population results
instead of inferring success from absence in a failure list.

### 0F28 and 0F2A

Keep all 5,000 0F28 records as the approved known gap. Diagnostic replay
remains an official skip and is not a passing reference. Record the exact G58
selector and resolved hashes, inspect NEC primary evidence where available,
and label target support or absence underdetermined unless primary uPD9002
evidence proves it. Analyze 0F2A separately. Do not reclassify or implement
either form.

### Shift forms

Prove rotate subforms 0 through 3 are applicable, selected, and executed
before using them as references. Analyze shift subforms 4 through 7 for C0,
C1, D2, and D3 across width, destination kind, count source, required boundary
counts, sign, carry, destination, FLAGS, termination, and RAM effects. Test
raw and masked count models as competing observations; do not assume Intel
count or undefined-FLAGS rules.

### FF /7 and BOUND

Derive FF /7 behavior without assuming 286 `#UD`. Partition BOUND into normal
completion, interrupt, frame-only, range/register/RAM/termination, and other
mismatches. Content-address the FF /7 termination classes and BOUND frame-only
and non-frame-only failure sets.

## Dependency analysis and future ordering

After the six analyses, record exact supporting hash sets, counterexamples,
confidence, and the smallest future milestone for each possible shared
primitive. Distinguish proven, probable, possible, independent, and
underdetermined dependencies.

Recommend an evidence-based ordering for M60 onward using the canonical
lettered scheme established by M58. Label each recommendation as required by
proven dependency, recommended by evidence, optional, or blocked by
underdetermined evidence. Do not edit future tasks as though the order were
already approved.

## Validation

Add fail-closed tests for schema/version/hash/classification errors,
selected/executed inconsistency, expected-only rows, row and manifest count
mismatches, artifact digest mismatch, nondeterministic order/JSON/gzip,
missing representatives, representative/table mismatch, overlapping or
incomplete partitions, classification and hash-set drift, and mutations to
approved G58 or immutable G43/M43 evidence.

Regenerate the complete pack twice in the same recorded environment and prove
byte identity. State the bounded gzip reproducibility claim; do not reopen the
accepted G58 environment limitation.

## Commit chain and gate

Commit 1 contains only analysis implementation, schemas, validators, tests,
and necessary documentation. Its SHA is `analysis_evaluated_sha`. Run all
required profiles and generate evidence against exactly that unamended
commit.

Commit 2 is evidence-only and contains the generated pack plus
`docs/agents/reports/m59_upd9002_semantics_evidence_pack.md`. Prove no
production semantic change and no protected-evidence mutation, push the
candidate branch, and stop for human G59 review. Do not approve G59, merge the
branch, or begin M60.
