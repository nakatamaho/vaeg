# M49 — Inventory the remaining NP2 286 protected-mode cluster

Derived from `docs/agents/plans/upd9002-core-consolidation-M42-M49-v6.md`
(v7 M47–M51 sequence amendment).

## Authoritative context

Before editing, read and obey:

1. `AGENTS.md`
2. `docs/agents/ROADMAP.md`
3. `docs/agents/CONVENTIONS.md`
4. `docs/agents/plans/upd9002-core-consolidation-M42-M49-v6.md`
5. This task file

The master plan is authoritative for shared preconditions, immutable baselines,
state compatibility, dispatch identity, regression traces, SingleStepTests V20
classification, execution protocol, and G42–G51 human-gate requirements. If this
extracted task conflicts with the master plan, stop and report the conflict; do
not improvise.

## Session boundary

Work only on **M49** in this Codex session. Do not begin the next
milestone, do not claim the human gate has passed, and stop after producing the
required report and final SHA. A failed baseline is evidence to investigate, not
permission to re-record a golden, expand a known-gap set, weaken an allowlist, or
change CPU semantics.

## Extracted task specification

Task: docs/agents/tasks/M49_upd9002_isolate_np2_286_protected_mode.md

Goal:
Inventory the remaining NP2 partial 80286 protected-mode implementation after
the exact G47-approved correctness/state decision has been implemented and
accepted at G48. Produce exact deletion or deferral evidence for M50. This
milestone must not delete protected-mode handlers, files, descriptor state, or
supporting code, and must not assume the cluster is unreachable.

Prerequisites:
- G48 explicitly accepted.
- pre-upd9002-refactor points at accepted G43.
- M42 final graph, M43 V20 baseline, M44 state boundary, M45 native invariant,
  and M46 single-step construction path are green.

Protected-mode cluster definition:
The cluster includes every candidate whose purpose is 80286 protected-mode or
system-mode semantics, including but not assumed limited to: 0x0F system groups,
MSW/GDTR/IDTR/LDTR/TR operations, descriptor-cache loading, selector validation,
privilege/type/presence checks, protected interrupt/return paths, and helper/state
code used only by those paths. Names alone are not evidence; classify by behavior
and references.

Steps:
1. Create docs/agents/reports/m49_np2_286_protected_mode_inventory.md with one
   row per file, handler, helper, table edge, macro, and runtime field:
   a. Exact definition and all direct/macro-expanded/address-taking references.
   b. Which NP2 286 protected-mode behavior it implements.
   c. Root/secondary/base-table reachability and whether native final dispatch can
      reach it.
   d. Runtime state reads/writes, range writes, reset/shut effects, and serialized
      byte ranges.
   e. Compiler cross-reference, function-section, gc-sections, and linker-map
      evidence under every supported preset.
   f. Proposed M50 disposition: delete, retain-shared, split-before-delete, or
      ambiguous/defer.

2. Establish an explicit internal boundary without deleting semantics:
   a. Identify intentional native behavior, every still-reachable legacy edge,
      shared helpers, and compatibility-only state after M48.
   b. Add a repository/ctest guard proving every active dispatch, selector,
      callback, direct-call, lifecycle, state, and serialization edge is
      enumerated and excluded from deletion.
   c. Reachability is a retain/defer result, never a failed assumption.
   d. If a translation unit irreducibly mixes shared native helpers and protected
      handlers, record the exact split required by M50. Do not perform it in M49.

3. Keep every protected-mode source and handler behavior unchanged:
   a. Do not redirect base slots to _reserved in M49.
   b. Do not delete i286c_0f.c or equivalent system-instruction code.
   c. Do not delete descriptor/selector/privilege helpers.
   d. Do not remove GDTR, IDTR, MSW, LDTR, LDTRC, TR, TRC, or related runtime
      fields merely because native execution does not read them.
   e. Do not alter the serialized CPU286 compatibility image.

4. Ordinary native cleanup is permitted only outside the protected cluster:
   a. A dead selector macro, declaration, or wrapper may be proposed only if it is
      not required to compile or audit the retained protected cluster.
   b. Use the normal evidence and explicit approval rule for any such deletion.
   c. Ambiguous code remains. Do not use M49 to perform the M50 deletion early.

5. Produce the exact M50 candidate list:
   a. Every candidate has an immutable path/symbol identity and evidence row.
   b. Group candidates by dependency-closed deletion unit.
   c. State which base-table redirections, source splits, runtime-field removals,
      CMake changes, and compatibility-image overlays M50 would require.
   d. State what must remain shared, especially real-mode/V30 interrupt mechanics,
      memory helpers, exception delivery, and CPU_SHUT behavior.
   e. Include expected construction-provenance changes and prove the M42 final
      graph is expected to remain identical.

6. Run all gates with the cluster still present and the G48 transition fixed:
   a. G41/M23 checkpoints and state payload fixtures.
   b. M42 trace, direct harness, final graph, root-pointer snapshot, and shutdown
      fixture.
   c. M43 V20 CI and full comparison: exact dataset/category/hash/failure-signature
      equality and unchanged known-gap manifest.
   d. ASan plus all standard presets and the human gate.

7. Stop after reporting the final M49 SHA and exact candidate list. G49 acceptance
   must explicitly state which dependency-closed groups, if any, are approved for
   M50. Silence or a general approval of the series is not deletion approval.

Non-goals:
- No NP2 286 protected-mode handler/file/state-field deletion.
- No base-slot redirection that makes such deletion possible.
- No change to the G47/G48 REP+0F semantic rule or protected-state policy.
- No uPD9002 semantic, timing, instruction-set, state-layout, or rename change.

Gate:
- Complete active-edge enumeration/exclusion guard green.
- Complete evidence table and dependency-closed M50 candidate list reviewed.
- Protected cluster still present and behaviorally untouched.
- M42 and M43 baselines exactly unchanged.
- ASan and standard human gate green.
