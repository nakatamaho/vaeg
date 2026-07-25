# uPD9002 semantics migration v5 — all task files

---

<!-- FILE: docs/agents/tasks/M59_upd9002_semantics_evidence_pack.md -->

# M59 — Produce the uPD9002/V20 semantic evidence pack

## Mandatory preparation

Before doing any work:

1. Read `AGENTS.md`.
2. Read `docs/agents/ROADMAP.md` and `docs/agents/CONVENTIONS.md`.
3. Read `docs/agents/UPD9002_SEMANTICS_MIGRATION.md`.
4. Read this task and all reports from prerequisite gates.
5. Run `git status --short`; the tracked worktree must be clean.
6. Record the exact starting branch and SHA.
7. Resolve and verify the exact approved predecessor gate SHA. Do not infer it from the current
   worktree.
8. Work on this milestone only and stop at its gate.

All newly authored source, comments, identifiers, commit messages, test names, and repository
Documentation must be in English.

## Predecessor and identifiers

Prerequisite: G58 explicitly approved.

Branch: `topic/m59-upd9002-evidence-pack`

Commit prefix: `M59:`

Gate: `G59`

## Goal

Produce machine-readable and human-readable evidence for the first semantic corrections. Make no
production core semantic change.

## Universal evidence requirements

For every item:

1. Provide at least one readable representative dump.
2. Analyze the full relevant population, or a deterministic justified stratified subset.
3. Produce a machine-readable table keyed by case hash.
4. Label conclusions `proven`, `hypothesis`, or `underdetermined`.
5. Verify top-level classification and executed count before calling any form green.

Each evidence row contains, where applicable:

- initial architectural state;
- expected SST final state;
- actual emulator final state;
- expected and actual termination;
- expected and actual registers;
- expected and actual RAM bytes;
- mismatch kinds;
- logical and physical addresses used in derived operand or frame mappings.

An expected-only table is not a sufficient defect diagnosis.

## Item 1 — Guest-visible FLAGS materialization

Analyze CC and corroborate with CD and CE. Aggregate at least:

- initial FLAGS, SS, and SP;
- expected and actual final SP;
- expected and actual frame addresses;
- expected and actual saved IP, CS, and FLAGS;
- expected and actual final FLAGS;
- instruction bytes and termination.

Prove frame mapping from observed state without assuming an Intel frame layout. Aggregate frames
crossing a 64 KiB segment boundary or 20-bit physical boundary as separate classes.

For every pushed FLAGS bit classify the observed rule as:

- `copied`
- `forced-zero`
- `forced-one`
- `condition-dependent`
- `undetermined`

Also verify classification, selected count, and executed count for 9C PUSHF, 9F LAHF, 9D POPF,
and 9E SAHF.

- Derive PUSHF stack image independently.
- Derive LAHF AH image independently.
- Derive POPF and SAHF loadable, preserved, and forced bits.
- Test whether POPF/SAHF and interrupt delivery share a defective primitive; do not assume they do.

## Item 2 — Canary cluster

Analyze F7.2 NOT and C6/C7 MOV immediate failures. Determine whether causes include:

- effective-address calculation;
- 16-bit segment wrapping or 20-bit physical wrapping;
- displacement/immediate fetch ordering;
- ModR/M group dispatch;
- unintended FLAGS clobbering;
- another shared primitive.

Partition by register vs memory form, address mode, displacement width, wrap condition, and mismatch
class.

## Item 3 — D4/D5

Verify D4 and D5 classifications and executed counts. Stratify immediate at minimum:

`0, 1, 2, 9, 10, 11, 16, 255`.

Determine expected and actual behavior, including AAM 0 termination. Do not propose a D5 change
unless executed SST evidence demonstrates a D5 mismatch.

## Item 4 — 0F28 and 0F2A

M43 classifies all 5,000 0F28 records as `known_target_gap`; 0F28 is not a passing reference.

Determine:

- whether the intended uPD9002/V52 target supports 0F28;
- its correct `gap_kind`;
- SST-observed 0F28 and 0F2A semantics;
- whether 0F2A failures are local, shared-primitive, or conceptual.

Do not reclassify or implement 0F28 in M59.

## Item 5 — Shift forms

Before comparison, verify that rotate subforms .0-.3 are applicable and executed.

For C0/C1/D2/D3 .4-.7 stratify:

- width 8/16;
- count 0, 1, 2, width-1, width, width+1, 31, 32, 33, 255;
- sign bit 0/1;
- initial CF 0/1.

Extract expected vs actual OF, AF, CF, and count-zero behavior. Do not assume lack or presence of
`& 0x1f` masking.

## Item 6 — FF.7 and BOUND

Partition failures into:

- normal completion;
- interrupt;
- stack-frame mismatch;
- range-result mismatch;
- other register/RAM mismatch.

Determine SST-observed FF /7 behavior without assuming 286 #UD. Quantify how much of BOUND's
failure population is solely explained by the INT 5 frame.

## Deliverables and gate G59

- Evidence tables are deterministic and content-addressed.
- All analyses reproduce from the pinned corpus and approved G58 epoch.
- No production CPU semantic file changes.
- Report includes explicit recommended re-ranking of M60 onward.

Write `docs/agents/reports/m59_upd9002_semantics_evidence_pack.md` and stop.

---

<!-- FILE: docs/agents/tasks/M60a_upd9002_flags_materialization.md -->

# M60a — Correct FLAGS canonicalization and guest-visible materialization

## Mandatory preparation

Before doing any work:

1. Read `AGENTS.md`.
2. Read `docs/agents/ROADMAP.md` and `docs/agents/CONVENTIONS.md`.
3. Read `docs/agents/UPD9002_SEMANTICS_MIGRATION.md`.
4. Read this task and all reports from prerequisite gates.
5. Run `git status --short`; the tracked worktree must be clean.
6. Record the exact starting branch and SHA.
7. Resolve and verify the exact approved predecessor gate SHA. Do not infer it from the current
   worktree.
8. Work on this milestone only and stop at its gate.

All newly authored source, comments, identifiers, commit messages, test names, and repository
Documentation must be in English.

## Predecessor and identifiers

Prerequisite: G59 explicitly approved and lettered milestone tooling approved.

Branch: `topic/m60a-upd9002-flags-materialization`

Commit prefix: `M60a:`

Gate: `G60a`

## Goal

Correct the shared FLAGS get/set/canonicalization primitives and the 9C/9D/9E/9F instruction family
using M59 evidence. Do not change interrupt delivery or IRET in this milestone.

## Required work

Implement only evidence-supported rules for:

- canonical internal FLAGS representation;
- defined-bit updates and fixed/preserved bits;
- PUSHF materialized stack word;
- POPF loadable, preserved, and forced bits;
- LAHF materialized AH image;
- SAHF loadable, preserved, and forced bits.

PUSHF, LAHF, interrupt-frame, and final-FLAGS conventions are independent unless M59 proved a shared
rule. Record any adopted undefined-bit image as a V20-compatibility convention and add exact
hardware-pending coverage where uPD9002 commonality is unconfirmed.

## Scope restrictions

- Do not change CC/CD/CE interrupt delivery.
- Do not change CF IRET.
- Do not modify D5 or unrelated arithmetic families.
- Do not rename handlers until semantic evidence is green; any rename is a separate rename-only
  commit after the semantic transition is recorded.

## Required semantic gate

- Run the deterministic architectural CI profile against the exact approved predecessor.
- Run the verified complete architectural full profile against the exact approved predecessor.
- Run the diagnostic fingerprint profile required by this task.
- Verify dataset and comparison-contract identities before comparing results.
- `newly_failing` must be empty.
- Timeout and crash counts must remain zero.
- Enumerate every changed failure signature in deterministic content-addressed shards.
- Run the repository's full standard build, lint, smoke, selftest, ctest, sanitizer, MinGW, and
  hosted-CI matrix required by current repository conventions.
- Regenerate the opcode/form scoreboard and full failure distribution.
- Record the semantic commit as `evaluated_sha` in an evidence-only follow-up commit.
- Write the milestone report, report evidence-commit SHA, and stop for the human gate.

## Gate G60a

In addition to the common gate:

- 9C/9D/9E/9F applicable failure populations are zero or each residual has an evidence-backed
  disposition that remains blocking or is explicitly approved.
- No previously green unrelated opcode regresses.
- Full distribution is re-ranked before M60b starts.

Write `docs/agents/reports/m60a_upd9002_flags_materialization.md` and stop.

---

<!-- FILE: docs/agents/tasks/M60b_upd9002_rom_authority_epoch.md -->

# M60b — Formalize ROM authority and correct the uPD9002 target-policy epoch

## Mandatory preparation

Before doing any work:

1. Read `AGENTS.md`.
2. Read `docs/agents/ROADMAP.md` and `docs/agents/CONVENTIONS.md`.
3. Read `docs/agents/UPD9002_SEMANTICS_MIGRATION.md`.
4. Read this task and every report from prerequisite gates.
5. Run `git status --short`; use a clean dedicated worktree at the exact approved predecessor SHA.
6. Record the starting branch, SHA, remote, tool versions, and verified SST corpus identity.
7. Resolve the approved predecessor from the maintainer-approved report; never infer it from HEAD,
   a branch tip, a mutable tag, or milestone numbering.
8. Execute this milestone only and stop at its candidate gate.

All newly authored source, comments, identifiers, commit messages, test names, schemas, and
repository documentation must be in English.


## Predecessor and identifiers

Prerequisite: G60a explicitly approved. Use the exact approved G60a SHA and fresh post-G60a
scoreboards; do not substitute G59 counts.

Branch: `topic/m60b-upd9002-rom-authority`

Commit prefix: `M60b:`

Gate: `G60b`

## Goal

Create a content-addressed ROM target-authority pack and a new target-policy epoch. Correct
`6C-6F` and exact INS/EXT gap classifications without changing production CPU semantics.

## Authority inputs

Obtain the maintainer-supplied monitor ROM and debugger evidence out of tree. Record:

- ROM SHA-256, byte size, mapping/base, and provenance;
- `0x66A8A` table start, end/terminator, adjacent boundary, and all raw three-byte records;
- deterministic `(mask,value,group)` expansion;
- group-to-mnemonic mapping and string addresses;
- independent debugger evidence for BRKFEM;
- exhaustive string-pool range and search method;
- main-table evidence proving primary `6C-6F` absence.

Do not commit a ROM binary without explicit authorization. Commit lawful extracts, hashes, scripts,
decoded tables, and manifests. If the bytes needed to verify the authority claims are unavailable,
stop before classification changes.

## Required `0F` authority result

Bind and verify:

- TEST1 `0F10/11/18/19`;
- CLR1 `0F12/13/1A/1B`;
- SET1 `0F14/15/1C/1D`;
- NOT1 `0F16/17/1E/1F`;
- ADD4S/SUB4S/CMP4S `0F20/22/26`;
- ROL4/ROR4 `0F28/2A`;
- BRKFEM `0FFE imm8`;
- BRKEM `0FFF imm8`;
- absence of `0F31/33/39/3B` from the complete table.

Record REPC/REPNC and PREPARE/DISPOSE presence independently. Do not infer FPO2, RETEM, or CALLN
absence from this table or from missing generic strings.

## Exact `6C-6F` correction

Before execution, define structural selectors for every selected record whose primary opcode is
`6C`, `6D`, `6E`, or `6F`, including plain, segment-prefixed, and repeat-prefixed forms represented
by the corpus.

- Current `applicable` records transition to `known_target_gap`.
- Every such gap receives `gap_kind=documented_silicon_absent`.
- Existing `known_target_gap` entries under these opcodes retain exact hashes and change only their
  gap kind where needed.
- Never partition by pass/fail outcome.
- Report exact per-selector and union counts and sorted-hash digests.
- After correction, no selected `6C-6F` record may remain in the blocking applicable denominator.

This is the one authorized `target_authority_correction` in the master specification.

## Other exact gap corrections

Preserve top-level `known_target_gap` and exact resolved hashes for `0F31`, `0F33`, `0F39`, and
`0F3B`, but set `gap_kind=documented_silicon_absent` if necessary.

Preserve `0F28` as `known_target_gap/implementation_missing`; record that M62b2 is mandatory. Do not
implement or make it applicable here.

No other classification or gap kind may change.

## Historical G43 reconciliation

Preserve G43/G58/G59/G60a artifacts byte-for-byte. Explicitly record:

- the OUTS fixture correction made 1,204 V20 records pass;
- the historical post-fix residuals included 6E=417 and 6F=224;
- these are V20 differential outcomes, not target progress;
- reclassified passes and failures are `retired_applicable`, not `newly_passing`.

Do not revert the fixture correction. Keep any V20 diagnostic execution separate from the target
blocking profile.

## New target-policy epoch

Create a versioned `target_policy_id` and canonical digest. Dataset and comparison-contract IDs do
not change. Generate separate architectural CI/full and fingerprint artifacts under G60b.

The transition must enumerate:

- retired applicable pass hashes/count/digest;
- retired applicable failure hashes/count/digest;
- exact classification changes;
- unaffected applicable before/after digest;
- authority-manifest digest;
- denominator, pass, and failure totals derived from exact hashes.

Compare unaffected applicable hashes against G60a and require no new failure or signature
regression. Do not guess the new total from G59 or historical 641 failures.

## FPO caveat

Record that missing `FPO1`, `FPO2`, or `ESC` strings are non-evidence because the monitor uses
individual FPU mnemonics. Do not classify 66/67 in M60b; M60c owns that audit.

## Scope restrictions

- No production semantic change and no change under `cpu/upd9002/`.
- Do not remove or modify active `6C-6F` handlers here.
- Do not alter fixtures, comparison contracts, or historical artifacts.
- Do not describe denominator retirement as a fix.

## Gate G60b

- Authority pack is deterministic, content-addressed, and independently reviewable.
- Exactly the authorized structural hashes leave `applicable`.
- Exactly the authorized gap-kind changes occur.
- `0F28` remains implementation-missing and mandatory.
- Unaffected applicable results satisfy the ratchet.
- Historical artifacts and the production CPU tree are unchanged.

Write `docs/agents/reports/m60b_upd9002_rom_authority.md`, report the candidate SHA, and stop.

---

<!-- FILE: docs/agents/tasks/M60c_upd9002_fpo2_main_dispatch_audit.md -->

# M60c — Audit the main opcode dispatch and FPO2/66-67 target status

## Mandatory preparation

Before doing any work:

1. Read `AGENTS.md`.
2. Read `docs/agents/ROADMAP.md` and `docs/agents/CONVENTIONS.md`.
3. Read `docs/agents/UPD9002_SEMANTICS_MIGRATION.md`.
4. Read this task and every report from prerequisite gates.
5. Run `git status --short`; use a clean dedicated worktree at the exact approved predecessor SHA.
6. Record the starting branch, SHA, remote, tool versions, and verified SST corpus identity.
7. Resolve the approved predecessor from the maintainer-approved report; never infer it from HEAD,
   a branch tip, a mutable tag, or milestone numbering.
8. Execute this milestone only and stop at its candidate gate.

All newly authored source, comments, identifiers, commit messages, test names, schemas, and
repository documentation must be in English.


## Predecessor and identifiers

Prerequisite: G60b explicitly approved.

Branch: `topic/m60c-upd9002-fpo2-audit`

Commit prefix: `M60c:`

Gate: `G60c`

## Goal

Determine, without implementing FPU semantics, how primary opcodes 66/67 are classified in SST and
how the monitor ROM main dispatch maps them. Remove unsupported absence claims; do not guess FPO2.

## Current-classification audit

For every selected record with primary opcode 66 or 67, report:

- upstream metadata mnemonic/status/architecture and whether it labels FPO2;
- top-level classification;
- selected, executed, pass, and failure counts;
- support-map/dispatch classification;
- exact resolved hashes and digests;
- `gap_kind` if and only if top-level classification is `known_target_gap`.

Absence from the failure distribution is not evidence of passing. Distinguish explicitly among
`applicable`, `known_target_gap`, `unsupported_fixture`, and `upstream_nonblocking`.

## ROM main-dispatch audit

Using the same content-addressed ROM authority:

- locate and bound the main opcode group table near `0x66900`;
- record raw records including the observed forms such as `fe f6 00`, `ff ff 01`, `ff 8f 02`,
  `fc 80 05`, `fc d0 07`, and `e7 26 08` only after verifying their exact offsets;
- determine the record format instead of assuming it from the `0F` table;
- verify that `e7 26` represents segment overrides before using it as a decoder check;
- trace 66 and 67 through group tables/handlers;
- record the D8-DF FPU record region near `0x66B3B` and the individual 8087 mnemonics it references.

The hypothesis that 66/67 are FPO2 is not a conclusion until the dispatch path proves it.

## Explicit non-evidence rule

The absence of generic strings `ESC`, `FPO1`, or `FPO2` is expected when the monitor uses individual
names such as FADD/FMUL. It cannot support `documented_silicon_absent`.

## Permitted evidence-taxonomy result

M60c makes no production semantic or top-level classification change.

For records already classified `known_target_gap`, it may set the exact gap kind according to
positive evidence:

- `implementation_missing` if target support is proven;
- `documented_silicon_absent` only if target absence is positively proven;
- `target_support_unverified` if support remains unresolved.

A change to `target_support_unverified` requires exact matching `hardware_pending.json` coverage.
If records are `upstream_nonblocking` because they are FPU metadata, preserve that top-level class
and record the target question in the orthogonal authority-pending registry.

Do not promote any hash to `applicable` in M60c. If target support is proven, propose a separately
gated implementation/profile-policy task for M65 review.

## Validation

- Reproduce G60b target-policy and all blocking profiles exactly.
- Add deterministic table decoders and fail-closed tests for wrong boundaries, overlapping masks,
  unresolved group links, unsupported generic-string inference, and taxonomy/registry mismatch.
- Generate the audit twice with byte-identical results in the recorded environment.
- Prove no change under `cpu/upd9002/` and no selected/applicable-set change.

## Gate G60c

- Every 66/67 selected hash has an explicit classification/execution account.
- Main-dispatch evidence is content-addressed and the support conclusion is labelled proven or
  underdetermined.
- No false absence claim rests on string-pool nomenclature.
- Any gap-kind correction is exact and governance-complete.
- No production semantics or top-level classification changes.

Write `docs/agents/reports/m60c_upd9002_fpo2_main_dispatch_audit.md` and stop.

---

<!-- FILE: docs/agents/tasks/M60d_upd9002_interrupt_frame.md -->

# M60d — Verify or correct residual synchronous interrupt-frame semantics

## Mandatory preparation

Before doing any work:

1. Read `AGENTS.md`.
2. Read `docs/agents/ROADMAP.md` and `docs/agents/CONVENTIONS.md`.
3. Read `docs/agents/UPD9002_SEMANTICS_MIGRATION.md`.
4. Read this task and every report from prerequisite gates.
5. Run `git status --short`; use a clean dedicated worktree at the exact approved predecessor SHA.
6. Record the starting branch, SHA, remote, tool versions, and verified SST corpus identity.
7. Resolve the approved predecessor from the maintainer-approved report; never infer it from HEAD,
   a branch tip, a mutable tag, or milestone numbering.
8. Execute this milestone only and stop at its candidate gate.

All newly authored source, comments, identifiers, commit messages, test names, schemas, and
repository documentation must be in English.


## Predecessor and identifiers

Prerequisite: G60c explicitly approved and a fresh target-correct scoreboard reviewed.

Branch: `topic/m60d-upd9002-interrupt-frame`

Commit prefix: `M60d:`

Gate: `G60d`

## Conditional status

M60a already owns guest-visible FLAGS materialization, including any saved-FLAGS change made by its
live approved scope. M59 proved frame placement matched for its observed interrupt population.
Therefore first determine whether an independent synchronous-frame residual remains after G60c.

If CC/CD/taken-CE and the dependent BOUND frame-only population are green with no unexplained frame
signature, make no semantic edit. Produce an evidence-only closure report and stop at G60d.

## Goal if residuals remain

Correct only evidence-proven residuals in INT3, INT imm8, and taken INTO frame delivery:

- stack addresses/wrapping;
- saved CS/IP values;
- final SP;
- vector fetch/final CS:IP;
- TF/IF post-entry handling;
- event classification.

Do not revisit a green saved-FLAGS image by analogy.

## Scope restrictions

- No IRET change.
- No DIV/IDIV arithmetic.
- No BOUND range-decision change.
- No `6C-6F`, FPO2, decoder, timing, or prefetch work.
- Do not broaden scope merely to create a semantic commit.

## Gate

Run architectural CI/full and fingerprint profiles against exact G60c, enumerate all changed
hashes/signatures, and require no new failure. Verify CC/CD/CE and the M59 BOUND frame-only set
separately from BOUND range residuals.

Write `docs/agents/reports/m60d_upd9002_interrupt_frame.md` and stop.

---

<!-- FILE: docs/agents/tasks/M60e_upd9002_iret.md -->

# M60e — Correct IRET restoration semantics

## Mandatory preparation

Before doing any work:

1. Read `AGENTS.md`.
2. Read `docs/agents/ROADMAP.md` and `docs/agents/CONVENTIONS.md`.
3. Read `docs/agents/UPD9002_SEMANTICS_MIGRATION.md`.
4. Read this task and every report from prerequisite gates.
5. Run `git status --short`; use a clean dedicated worktree at the exact approved predecessor SHA.
6. Record the starting branch, SHA, remote, tool versions, and verified SST corpus identity.
7. Resolve the approved predecessor from the maintainer-approved report; never infer it from HEAD,
   a branch tip, a mutable tag, or milestone numbering.
8. Execute this milestone only and stop at its candidate gate.

All newly authored source, comments, identifiers, commit messages, test names, schemas, and
repository documentation must be in English.


## Predecessor and identifiers

Prerequisite: G60d explicitly approved.

Branch: `topic/m60e-upd9002-iret`

Commit prefix: `M60e:`

Gate: `G60e`

## Goal

Correct CF IRET restoration using executed SST evidence without introducing unrelated interrupt,
arithmetic, classification, or timing changes.

## Required work

Derive and implement:

- stack read addresses and 16-bit/20-bit wrapping;
- restored IP and CS;
- independently evidenced loadable/preserved/forced FLAGS bits;
- final SP and termination.

Do not copy POPF rules into IRET unless aggregate evidence proves identity. Preserve the approved
G60b target-policy set and G60c FPO evidence taxonomy.

## Gate

- All applicable CF failures are cleared or exactly governed.
- CC/CD/CE and 9C/9D/9E/9F retain approved results.
- Dataset, contracts, target policy, selected/applicable sets, timeout/crash, and unrelated forms
  satisfy the ratchet.
- Publish a fresh full ranking before M61.

Write `docs/agents/reports/m60e_upd9002_iret.md` and stop.

---

<!-- FILE: docs/agents/tasks/M61_upd9002_mov_immediate_register.md -->

# M61 — Correct C6/C7 register-form MOV-immediate execution

## Mandatory preparation

Before doing any work:

1. Read `AGENTS.md`.
2. Read `docs/agents/ROADMAP.md` and `docs/agents/CONVENTIONS.md`.
3. Read `docs/agents/UPD9002_SEMANTICS_MIGRATION.md`.
4. Read this task and every report from prerequisite gates.
5. Run `git status --short`; use a clean dedicated worktree at the exact approved predecessor SHA.
6. Record the starting branch, SHA, remote, tool versions, and verified SST corpus identity.
7. Resolve the approved predecessor from the maintainer-approved report; never infer it from HEAD,
   a branch tip, a mutable tag, or milestone numbering.
8. Execute this milestone only and stop at its candidate gate.

All newly authored source, comments, identifiers, commit messages, test names, schemas, and
repository documentation must be in English.


## Predecessor and identifiers

Prerequisite: G60e explicitly approved and fresh ranking confirms the M59 population.

Branch: `topic/m61-upd9002-mov-immediate-register`

Commit prefix: `M61:`

Gate: `G61`

## Goal

Correct only the C6/C7 register-destination defect proven by M59. Do not treat F7 `/2` as the same
primitive.

## Evidence-bound scope

M59 established:

- every C6/C7 memory form passed;
- every C6/C7 failure was a register form whose actual destination remained initial;
- apparent register passes included value coincidences;
- no unexpected FLAGS changes;
- F7 `/2` failures were a distinct low-memory word-path defect.

Reverify those exact properties in the current target-policy epoch before editing.

## Required work

- Fix register-form dispatch/execution for C6 and C7 only.
- Add focused tests that cannot pass by initial/immediate value coincidence.
- Preserve all memory forms, FLAGS, instruction length, IP, prefixes, and termination.
- Do not modify generic EA logic, F7 `/2`, DIV/IDIV, or target classification.

## Gate G61

Every applicable non-coincidental C6/C7 register form is green; all memory forms and unrelated
opcodes retain exact approved results. Full profiles, ratchet artifacts, and deterministic evidence
are required.

Write `docs/agents/reports/m61_upd9002_mov_immediate_register.md` and stop.

---

<!-- FILE: docs/agents/tasks/M62a_upd9002_aam.md -->

# M62a — Correct AAM (D4) semantics

## Mandatory preparation

Before doing any work:

1. Read `AGENTS.md`.
2. Read `docs/agents/ROADMAP.md` and `docs/agents/CONVENTIONS.md`.
3. Read `docs/agents/UPD9002_SEMANTICS_MIGRATION.md`.
4. Read this task and every report from prerequisite gates.
5. Run `git status --short`; use a clean dedicated worktree at the exact approved predecessor SHA.
6. Record the starting branch, SHA, remote, tool versions, and verified SST corpus identity.
7. Resolve the approved predecessor from the maintainer-approved report; never infer it from HEAD,
   a branch tip, a mutable tag, or milestone numbering.
8. Execute this milestone only and stop at its candidate gate.

All newly authored source, comments, identifiers, commit messages, test names, schemas, and
repository documentation must be in English.


## Predecessor and identifiers

Prerequisite: G61 explicitly approved and fresh ranking confirms D4.

Branch: `topic/m62a-upd9002-aam`

Commit prefix: `M62a:`

Gate: `G62a`

## Goal

Correct D4 using the complete M59 immediate-stratified evidence. D5 remains protected.

## Required work

- Revalidate D4 expected/actual behavior for all immediate values represented by the corpus.
- Cover immediate 0, 1, 2, 9, 10, 11, 16, and 255 explicitly.
- Preserve M59's proven normal termination for the D4 immediate-zero population unless newer
  target evidence contradicts it.
- Implement quotient/remainder, defined FLAGS, and side effects exactly as observed.

Do not assume a base-10 rule from tradition; state the evidence-derived formula. Do not modify D5:
M59 proved all 5,000 D5 architectural records executed and passed.

## Gate G62a

All applicable D4 failures are cleared with no D5 or unrelated BCD regression. Run target-correct
architectural CI/full, fingerprint, ratchet, and full repository gates.

Write `docs/agents/reports/m62a_upd9002_aam.md` and stop.

---

<!-- FILE: docs/agents/tasks/M62b1_upd9002_ror4.md -->

# M62b1 — Correct 0F2A ROR4 semantics

## Mandatory preparation

Before doing any work:

1. Read `AGENTS.md`.
2. Read `docs/agents/ROADMAP.md` and `docs/agents/CONVENTIONS.md`.
3. Read `docs/agents/UPD9002_SEMANTICS_MIGRATION.md`.
4. Read this task and every report from prerequisite gates.
5. Run `git status --short`; use a clean dedicated worktree at the exact approved predecessor SHA.
6. Record the starting branch, SHA, remote, tool versions, and verified SST corpus identity.
7. Resolve the approved predecessor from the maintainer-approved report; never infer it from HEAD,
   a branch tip, a mutable tag, or milestone numbering.
8. Execute this milestone only and stop at its candidate gate.

All newly authored source, comments, identifiers, commit messages, test names, schemas, and
repository documentation must be in English.


## Predecessor and identifiers

Prerequisite: G62a explicitly approved and fresh ranking confirms 0F2A.

Branch: `topic/m62b1-upd9002-ror4`

Commit prefix: `M62b1:`

Gate: `G62b1`

## Goal

Correct the applicable 0F2A ROR4 family using M59 SST evidence and the M60b ROM inventory.

## Required work

M59 observed 308 pass and 4,692 fail and found a full-source-byte expected result versus an actual
low-nibble merge in most records. Revalidate register/memory, ModR/M, address, AL, destination, and
FLAGS behavior, then correct only 0F2A or its proven local helper.

Do not use unimplemented 0F28 as a semantic oracle. Do not implement 0F28, BRKFEM, BRKEM, or other
`0F` forms here.

## Gate G62b1

All applicable 0F2A hashes pass, no new failure occurs, and the exact 0F28 known-gap set remains
unchanged. Run all target-correct profiles and transition gates.

Write `docs/agents/reports/m62b1_upd9002_ror4.md` and stop.

---

<!-- FILE: docs/agents/tasks/M62b2_upd9002_rol4.md -->

# M62b2 — Implement mandatory 0F28 ROL4 semantics

## Mandatory preparation

Before doing any work:

1. Read `AGENTS.md`.
2. Read `docs/agents/ROADMAP.md` and `docs/agents/CONVENTIONS.md`.
3. Read `docs/agents/UPD9002_SEMANTICS_MIGRATION.md`.
4. Read this task and every report from prerequisite gates.
5. Run `git status --short`; use a clean dedicated worktree at the exact approved predecessor SHA.
6. Record the starting branch, SHA, remote, tool versions, and verified SST corpus identity.
7. Resolve the approved predecessor from the maintainer-approved report; never infer it from HEAD,
   a branch tip, a mutable tag, or milestone numbering.
8. Execute this milestone only and stop at its candidate gate.

All newly authored source, comments, identifiers, commit messages, test names, schemas, and
repository documentation must be in English.


## Predecessor and identifiers

Prerequisite: G62b1 explicitly approved and the G60b ROM authority manifest remains valid.

Branch: `topic/m62b2-upd9002-rol4`

Commit prefix: `M62b2:`

Gate: `G62b2`

## Status

This milestone is no longer conditional. M60b target authority proves that the target dispatch
contains 0F28 ROL4. The exact existing gap must be `known_target_gap/implementation_missing`.

## Goal

Implement 0F28 and transition its complete structural hash set to `applicable` in the same PR.

## Required work

- Verify the exact pre-approved selector, expected 5,000 resolved hashes, count, and digest; stop if
  the live epoch differs.
- Add dispatch/handler behavior and focused direct-harness tests.
- Implement SST-observed register/memory, AL, destination, address, prefix, and defined-FLAGS rules.
- Apply `known_target_gap/implementation_missing -> applicable` without outcome partitioning.
- Every newly applicable hash must pass in this PR.

Do not combine other `0F` work or BRK/mode-transition semantics.

## Gate G62b2

The transition manifest equals the complete approved 0F28 set, all newly applicable hashes pass,
and no other classification/gap entry changes.

Write `docs/agents/reports/m62b2_upd9002_rol4.md` and stop.

---

<!-- FILE: docs/agents/tasks/M62c_upd9002_bcd_adjust.md -->

# M62c — Correct the BCD/ASCII adjust family

## Mandatory preparation

Before doing any work:

1. Read `AGENTS.md`.
2. Read `docs/agents/ROADMAP.md` and `docs/agents/CONVENTIONS.md`.
3. Read `docs/agents/UPD9002_SEMANTICS_MIGRATION.md`.
4. Read this task and every report from prerequisite gates.
5. Run `git status --short`; use a clean dedicated worktree at the exact approved predecessor SHA.
6. Record the starting branch, SHA, remote, tool versions, and verified SST corpus identity.
7. Resolve the approved predecessor from the maintainer-approved report; never infer it from HEAD,
   a branch tip, a mutable tag, or milestone numbering.
8. Execute this milestone only and stop at its candidate gate.

All newly authored source, comments, identifiers, commit messages, test names, schemas, and
repository documentation must be in English.


## Predecessor and identifiers

Prerequisite: G62b2 explicitly approved and fresh ranking confirms the family.

Branch: `topic/m62c-upd9002-bcd-adjust`

Commit prefix: `M62c:`

Gate: `G62c`

## Goal

Correct AAS and only evidence-confirmed residual AAA/DAA/DAS behavior.

Before treating any form as green, verify classification, selected count, executed count, and exact
pass set. Implement result adjustment, AF/CF, defined/materialized flags, and branch conditions from
aggregate SST evidence. If root causes are independent, stop and split before semantic editing.

Do not modify D5 by analogy and do not combine packed-BCD rotate or mode-transition work.

## Gate G62c

Every included adjust family is green; excluded green/non-applicable forms retain exact results.
Run all target-correct profiles and ratchet gates.

Write `docs/agents/reports/m62c_upd9002_bcd_adjust.md` and stop.

---

<!-- FILE: docs/agents/tasks/M63_upd9002_shift_semantics.md -->

# M63 — Correct SHL/SAL/SHR/SAR semantics

## Mandatory preparation

Before doing any work:

1. Read `AGENTS.md`.
2. Read `docs/agents/ROADMAP.md` and `docs/agents/CONVENTIONS.md`.
3. Read `docs/agents/UPD9002_SEMANTICS_MIGRATION.md`.
4. Read this task and every report from prerequisite gates.
5. Run `git status --short`; use a clean dedicated worktree at the exact approved predecessor SHA.
6. Record the starting branch, SHA, remote, tool versions, and verified SST corpus identity.
7. Resolve the approved predecessor from the maintainer-approved report; never infer it from HEAD,
   a branch tip, a mutable tag, or milestone numbering.
8. Execute this milestone only and stop at its candidate gate.

All newly authored source, comments, identifiers, commit messages, test names, schemas, and
repository documentation must be in English.


## Predecessor and identifiers

Prerequisite: G62c explicitly approved and current ranking confirms shifts.

Branch: `topic/m63-upd9002-shifts`

Commit prefix: `M63:`

Gate: `G63`

## Goal

Correct C0/C1/D2/D3 subforms `.4-.7` from the M59 stratified evidence.

## Evidence discipline

M59 showed that raw-count, `count & 0x1f`, and neither-model observations coexist. Do not begin by
installing one unconditional count rule. Re-stratify by width, register/memory, subform, count source,
count 0/1/>1/at-width/beyond-width, sign, and initial CF.

If count/destination semantics and FLAGS semantics have independent causes that cannot be safely
reviewed as one family, stop before production edits and obtain separately gated M63 submilestones.

Implement only evidenced destination, CF, OF, AF, ZF, SF, PF, count-zero, alias, and memory rules.
Preserve the exact 40,000 applicable/green rotate hashes confirmed by M59/current epoch.

## Gate G63

All included applicable shift failures clear, no rotate regression occurs, and all target-policy
identities/classifications remain unchanged.

Write `docs/agents/reports/m63_upd9002_shift_semantics.md` and stop.

---

<!-- FILE: docs/agents/tasks/M64_upd9002_div_idiv.md -->

# M64 — Correct DIV/IDIV and divide-error semantics

## Mandatory preparation

Before doing any work:

1. Read `AGENTS.md`.
2. Read `docs/agents/ROADMAP.md` and `docs/agents/CONVENTIONS.md`.
3. Read `docs/agents/UPD9002_SEMANTICS_MIGRATION.md`.
4. Read this task and every report from prerequisite gates.
5. Run `git status --short`; use a clean dedicated worktree at the exact approved predecessor SHA.
6. Record the starting branch, SHA, remote, tool versions, and verified SST corpus identity.
7. Resolve the approved predecessor from the maintainer-approved report; never infer it from HEAD,
   a branch tip, a mutable tag, or milestone numbering.
8. Execute this milestone only and stop at its candidate gate.

All newly authored source, comments, identifiers, commit messages, test names, schemas, and
repository documentation must be in English.


## Predecessor and identifiers

Prerequisite: G63 explicitly approved and current ranking confirms F6.6/F6.7/F7.6/F7.7.

Branch: `topic/m64-upd9002-div-idiv`

Commit prefix: `M64:`

Gate: `G64`

## Goal

Correct unsigned/signed division and divide-error behavior after FLAGS/frame/IRET foundations are
approved.

## Required work

Derive and implement quotient/remainder, zero divisor, overflow boundaries, most-negative quotient,
defined successful FLAGS, failure-path register preservation, pushed IP/CS/FLAGS, termination, and
applicable REP-prefix behavior. Use transactional execution so a fault does not leak partial state
unless exact SST evidence requires it.

Do not alter F7 `/2`, other group subforms, target classifications, cycles, or prefetch. Choose saved
IP from observed SST frame bytes, not Intel terminology.

## Gate G64

All applicable DIV/IDIV failures clear, no unrelated F6/F7 regression occurs, and a complete fresh
failure inventory is published for M65.

Write `docs/agents/reports/m64_upd9002_div_idiv.md` and stop.

---

<!-- FILE: docs/agents/tasks/M65_upd9002_residue_replan.md -->

# M65 — Re-plan the target-correct structural and long-tail residue

## Mandatory preparation

Before doing any work:

1. Read `AGENTS.md`.
2. Read `docs/agents/ROADMAP.md` and `docs/agents/CONVENTIONS.md`.
3. Read `docs/agents/UPD9002_SEMANTICS_MIGRATION.md`.
4. Read this task and every report from prerequisite gates.
5. Run `git status --short`; use a clean dedicated worktree at the exact approved predecessor SHA.
6. Record the starting branch, SHA, remote, tool versions, and verified SST corpus identity.
7. Resolve the approved predecessor from the maintainer-approved report; never infer it from HEAD,
   a branch tip, a mutable tag, or milestone numbering.
8. Execute this milestone only and stop at its candidate gate.

All newly authored source, comments, identifiers, commit messages, test names, schemas, and
repository documentation must be in English.


## Status and identifiers

This is planning/evidence only. Prerequisite: G64 explicitly approved.

Branch: `topic/m65-upd9002-residue-plan`

Commit prefix: `M65:`

Gate: `G65`

## Goal

Replace the residue bucket with actual independently gated tasks from the exact G64 target-correct
scoreboard and all remaining `implementation_missing` entries.

## Mandatory populations

At minimum classify and plan:

- F7 `/2` low-memory word RMW, kept separate from C6/C7;
- BOUND range-decision residuals, separate from frame-only history;
- FF `/7` normal-completion state behavior;
- active `6C-6F` V20 handlers: acquire reserved-opcode behavior evidence, then remove or route them so
  they are not reachable/advertised as uPD9002 string-I/O instructions; never return them to the
  blocking denominator;
- BRKFEM `0FFE` vector handling, destination mode, and return mechanism;
- BRKEM/BRKFEM relationship and Z80-side RETEM/CALLN questions;
- the M60c 66/67/FPO2 conclusion, including a separate implementation/profile-policy task if target
  support is proven, or exact pending evidence if unresolved;
- remaining NEC `0F` implementation-missing forms;
- undefined/reserved opcode policy;
- REPC/REPNC and multi-prefix restart;
- remaining long-tail failures.

Do not interpret missing FPO generic strings as absence. Do not plan INS/EXT or `6C-6F` V20
semantics as implementation work after M60b target authority.

## Required output

For every proposed task provide exact selectors/hashes/digests, mismatch classes, evidence status,
blast radius, prerequisite order, task/report/branch/gate names, and classification transitions.
Create the actual future task files only after the decomposition is supported by evidence.

## Gate G65

No production semantic change. Every remaining applicable failure and implementation-missing entry
has one non-overlapping owner or an explicit evidence task. Maintainer approval is required before
any generated semantic task begins.

Write `docs/agents/reports/m65_upd9002_residue_replan.md` and stop.

---

<!-- FILE: docs/agents/tasks/M66a_upd9002_drop_cpu286_state_compat.md -->

# M66a — Remove obsolete CPU286 save-state compatibility

## Mandatory preparation

Before doing any work:

1. Read `AGENTS.md`.
2. Read `docs/agents/ROADMAP.md` and `docs/agents/CONVENTIONS.md`.
3. Read `docs/agents/UPD9002_SEMANTICS_MIGRATION.md`.
4. Read this task and all reports from prerequisite gates.
5. Run `git status --short`; the tracked worktree must be clean.
6. Record the exact starting branch and SHA.
7. Resolve and verify the exact approved predecessor gate SHA. Do not infer it from the current
   worktree.
8. Work on this milestone only and stop at its gate.

All newly authored source, comments, identifiers, commit messages, test names, and repository
Documentation must be in English.

## Scheduling

This task starts only after all semantic, target-authority, FPO2, and residue tasks generated by G65 are approved and complete.
If the approved roadmap renumbers this task, follow the approved integer or lettered identifier.

Branch: `topic/m66a-upd9002-drop-cpu286-state`

Commit prefix: `M66a:`

Gate: `G66a`

## Goal

Make the explicitly permitted state-format break that removes obsolete CPU286 compatibility
serialization and loading. This is not a handler rename milestone.

## Required work

- Inventory every active save/load path, tag, payload, adapter, compatibility transform, and test.
- Define the new uPD9002 state version and exact rejection behavior for obsolete payloads.
- Remove obsolete compatibility code only after focused import/export and rejection tests exist.
- Update current documentation and migration notes.
- Preserve historical reports and immutable fixtures as evidence; they may remain named CPU286.

## Scope restrictions

- Do not rename remaining active handler files/macros here unless required solely by the state API;
  broad active-core identity cleanup belongs to M66b.
- Do not silently accept an old payload under a new layout.

## Gate G66a

- New state round trips exactly at supported boundaries.
- Obsolete CPU286 payloads fail deterministically and atomically.
- Rejection leaves live machine state unchanged.
- Standard system boot/save/load regressions are green.
- Architectural SST result is unchanged from the approved predecessor.

Write `docs/agents/reports/m66a_upd9002_drop_cpu286_state_compat.md` and stop.

---

<!-- FILE: docs/agents/tasks/M66b_upd9002_remove_i286_identity.md -->

# M66b — Remove the remaining active I286/i286c implementation identity

## Mandatory preparation

Before doing any work:

1. Read `AGENTS.md`.
2. Read `docs/agents/ROADMAP.md` and `docs/agents/CONVENTIONS.md`.
3. Read `docs/agents/UPD9002_SEMANTICS_MIGRATION.md`.
4. Read this task and all reports from prerequisite gates.
5. Run `git status --short`; the tracked worktree must be clean.
6. Record the exact starting branch and SHA.
7. Resolve and verify the exact approved predecessor gate SHA. Do not infer it from the current
   worktree.
8. Work on this milestone only and stop at its gate.

All newly authored source, comments, identifiers, commit messages, test names, and repository
Documentation must be in English.

## Scheduling

Prerequisite: G66a explicitly approved and every production semantic family green.
If the roadmap renumbers this task, use the approved identifier consistently.

Branch: `topic/m66b-upd9002-remove-i286-identity`

Commit prefix: `M66b:`

Gate: `G66b`

## Goal

Rename and delete the final active production 286-derived identities without changing behavior.

## Required work

For production uPD9002 build sources only:

- rename remaining source basenames;
- rename declarations, definitions, macros, and dispatch targets;
- delete now-unreachable helpers and dead compatibility tables;
- update build lists and current documentation;
- use rename-only commits followed by reference-fix commits where repository conventions require it.

## Active-scope zero gate

No active production declaration, definition, dispatch target, source basename, or macro used by the
uPD9002 build may use `I286` or `i286c` identity.

The check must exclude historical/evidence paths, at minimum:

- `docs/agents/reports/**`
- `docs/agents/tasks/archive/**`
- `tools/qa/golden/**`
- historical legal/provenance documents

Do not edit excluded evidence merely to satisfy grep.

## Gate G66b

- Active-scope identity count is zero.
- Production dispatch graph and all architectural SST hashes are unchanged.
- Full build and system regression gates are green.
- Deleted helpers have proven zero reachability.

Write `docs/agents/reports/m66b_upd9002_remove_i286_identity.md` and stop.

---

<!-- FILE: docs/agents/tasks/M67_upd9002_divergence_consolidation.md -->

# M67 — Consolidate divergences and hardware/authority questions

## Mandatory preparation

Before doing any work:

1. Read `AGENTS.md`.
2. Read `docs/agents/ROADMAP.md` and `docs/agents/CONVENTIONS.md`.
3. Read `docs/agents/UPD9002_SEMANTICS_MIGRATION.md`.
4. Read this task and every report from prerequisite gates.
5. Run `git status --short`; use a clean dedicated worktree at the exact approved predecessor SHA.
6. Record the starting branch, SHA, remote, tool versions, and verified SST corpus identity.
7. Resolve the approved predecessor from the maintainer-approved report; never infer it from HEAD,
   a branch tip, a mutable tag, or milestone numbering.
8. Execute this milestone only and stop at its candidate gate.

All newly authored source, comments, identifiers, commit messages, test names, schemas, and
repository documentation must be in English.


## Predecessor and identifiers

Prerequisite: G66b explicitly approved and the target-correct architectural full profile has zero
applicable failures.

Branch: `topic/m67-upd9002-divergence-consolidation`

Commit prefix: `M67:`

Gate: `G67`

## Goal

Publish the final minimal, content-addressed target inventory, approved divergences, and unresolved
hardware questions without production semantic change.

## Required review

- Verify every `expected_target_divergence` against exact primary target/hardware evidence.
- Verify every remaining `target_support_unverified` hash has exact hardware-pending coverage.
- Verify all `6C-6F` and `0F31/33/39/3B` records are exact documented-silicon-absent gaps and are
  never reported as passes.
- Verify `0F28` is applicable and passing.
- Reconcile active/reserved behavior for `6C-6F` with the final handler reachability decision.
- Consolidate BRKFEM/BRKEM, RETEM/CALLN, MD/Z80 mode, and FPO2 questions.
- State explicitly that generic FPO string absence is non-evidence.
- Preserve the historical G43 1,204 OUTS gain as V20 evidence only.

## Gate G67

- Zero target-correct applicable failures.
- No implementation-missing or unclassified record.
- Every target-absent and divergence entry is exact/evidence-backed.
- Unresolved FPO2/BRK/mode questions are minimal and explicit.
- No active I286/i286c identity remains and historical artifacts are unchanged.
- Report does not claim complete silicon validation.

Write `docs/agents/reports/m67_upd9002_divergence_consolidation.md` and stop.
