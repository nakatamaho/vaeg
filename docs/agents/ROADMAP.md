# ROADMAP — vaeg modernization, phase 1 (toolchain + repo hygiene)

Scope of this phase: make the legacy Win32 code build unmodified on
VS2008, then on VS2017/v141, then normalize the repository
(prune → rename → EOL → encoding). Functional refactoring, SDL2 work,
and CMake unification are explicitly OUT of scope here.

## Milestone order and why

The user-facing requirements are: VS2008, VS2017/v141 as-is, UTF-8
without BOM + /utf-8, lowercase file names, CRLF→LF, unreferenced-file
removal. The repository transformations are ordered

    M3 prune  →  M4 lowercase rename  →  M5 EOL  →  M6 encoding

for these reasons:

1. Prune first: every later mechanical pass (rename, renormalize,
   re-encode) then touches the minimum file set.
2. Rename before any content change: rename-only commits keep
   `git log --follow` and `git blame` usable across the subsequent
   mass content commits.
3. EOL before encoding: both are content-wide, but EOL normalization is
   trivially verifiable byte-wise; doing it first means the encoding
   diff (M6) contains only encoding changes.
4. Encoding last, because it is the commit that kills the VS2008
   baseline. UTF-8 **without BOM** cannot be declared to the VC9
   compiler; it will parse the bytes as CP932 and garble every Japanese
   literal and comment (with 0x5C-trailing-byte hazards). VS2008
   support therefore ends, by design, at the end of M5. Tag
   `baseline-vs2008` before merging M6.

## Milestones

| ID | Task file | Deliverable | Gate |
|----|-----------|-------------|------|
| M0 | tasks/M0_inventory.md      | inventory report (no repo mutation) | review only |
| M1 | tasks/M1_vs2008_baseline.md | VS2008 Win32 Release builds as-is | **G1** |
| M2 | tasks/M2_vs2017_v141.md    | v141 build of unmodified code | **G2** |
| M3 | tasks/M3_prune.md          | unreferenced files deleted (approved list) | **G3** |
| M4 | tasks/M4_lowercase.md      | all tracked paths lowercase | **G4** |
| M5 | tasks/M5_eol_lf.md         | LF everywhere except CRLF exceptions; .gitattributes | **G5** |
| M6 | tasks/M6_utf8.md           | UTF-8 (no BOM) sources; charset flags decided | **G6** |

Tags: `baseline-vs2008` after G1. Pushed tags are immutable. After G5
passes, create a NEW tag `vs2008-final` on the G5 commit;
baseline-vs2008 stays where it is. `baseline-v141` after G2.

## Gate protocol (all gates)

Agent side (machine checks, pasted into PR):
- build log of the milestone's required toolchain(s), zero errors
- output of the `tools/repo/` checks named in the task file

User side (manual, per `docs/agents/gates/GATE_CHECKLIST.md`):
1. Build locally from a clean checkout.
2. Boot in V3 mode.
3. Run the bundled VA demo.
4. Boot an OS and perform simple operations (DIR, launch a program).

A gate passes only when the user says so in the conversation/PR.

## Known decision points (do not resolve silently)

- **/utf-8 vs /source-charset:utf-8 (M6).** `/utf-8` sets BOTH source
  and execution charsets. With execution charset = UTF-8, Japanese
  string literals passed to ANSI Win32 APIs (menus, MessageBoxA, file
  dialogs) garble on CP932-ACP Windows unless the process opts into the
  UTF-8 ACP via manifest (Win10 1903+). The conservative choice for the
  legacy Win32 build is `/source-charset:utf-8` alone (execution stays
  CP932); `/utf-8` is the right end state for the SDL2 target. M6
  presents both; the user decides at the gate pre-check.
- **.rc files (M6).** rc.exe ignores /utf-8. Options: convert to UTF-8
  and prepend `#pragma code_page(65001)`, or exempt .rc from the
  migration. Recorded in M6.
- **Wave-dash mapping (M6).** Conversion must use codepage CP932 (not
  SHIFT_JIS) in both directions and must round-trip byte-identically;
  0x8160 ↔ U+FF5E. Enforced by the M6 round-trip check.
