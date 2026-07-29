# M72 - Miscellaneous compile-flag cleanup

M72 starts from the formally approved and main-integrated G71 candidate:

```text
24950894eca79e308afae8d574d43c8f393bb483
```

Branch: `topic/m72-misc-compile-flag-cleanup`

Commit prefix: `M72:`

Candidate gate: `G72`

Report: `docs/agents/reports/m72_misc_compile_flag_cleanup.md`

Do not start M73. Do not merge M72 to `main` before G72 approval. Do not
declare G72 passed.

## Scope

M72 is a behavior-preserving cleanup for obsolete active-tree compile-time
feature controls.

M72 owns:

1. Fold `VAEG_FIX` as always enabled in the active CMake tree.
   - Remove the public compile definition from CMake targets.
   - Remove source `#if defined(VAEG_FIX)` conditionals by keeping the
     currently built active behavior.
   - Preserve the current runtime behavior and validation baselines.
2. Remove inactive `SUPPORT_PC9821` guarded code from the active tree.
   - Do not introduce PC-9821 support.
   - Do not preserve dead PC-9821 drawing, BIOS, PCI, GDC, FDC, palette or
     state-save branches as active code.
   - Preserve the supported PC-88VA active behavior.
3. Audit `VAEG_EXT` and remove obsolete active-tree references where safe.
   - Do not blindly enable the former extension/debug/SCSI paths.
   - Preserve the current non-`VAEG_EXT` behavior unless a specific branch is
     proven to be the active intended behavior.
   - Do not change state-save format or SCSI/SASI behavior without explicit
     evidence and tests.
4. Audit frontend asset embedding and font stubs only to classify future work.
   - Do not remove embedded GUI assets in M72.
   - Do not modify ROM/font payloads.
   - Remove only a source stub if it is proven unused by the active build and
     does not affect guest font ROM loading, GUI font loading, or packaging.

## Non-goals

M72 must not:

- modify uPD9002 instruction semantics, SST policies, or generated evidence;
- modify M68, M69, M70, or M71 approved artifacts in place;
- change guest-visible FDD, SASI, SCSI, GDC, BIOS, TVRAM, audio, keyboard,
  mouse, save-state, or display behavior;
- remove ROM or font payloads;
- remove embedded GUI font, splash, or icon assets without a separate
  maintainer-approved task;
- enable `VAEG_EXT` globally;
- implement PC-9821 support;
- start any unrelated cleanup.

## Required startup audit

Before production changes, record:

```bash
git status --short --branch
git rev-parse HEAD
git rev-parse origin/main
git diff --check
rg -n "VAEG_FIX|VAEG_EXT|SUPPORT_PC9821|PCMODEL_PC9821|PC-9821|PC9821" .
```

Confirm:

- the branch starts from `24950894eca79e308afae8d574d43c8f393bb483`;
- the worktree is clean;
- no active M72 task already exists;
- `VAEG_FIX` is currently defined by the active CMake build;
- `VAEG_EXT` is not currently defined by the active CMake build;
- `SUPPORT_PC9821` is not currently defined by the active CMake build.

Stop if an apparently dead conditional owns current guest-visible behavior.

## Implementation rules

Keep one concern per commit:

1. task authority and roadmap update;
2. `VAEG_FIX` constant-fold;
3. `SUPPORT_PC9821` removal;
4. `VAEG_EXT` cleanup, if safe;
5. optional unused-source-stub cleanup, if proven safe;
6. report and evidence.

For every removed conditional, document which side is retained and why.

For every removed file or block, prove that the active build no longer
references it.

Do not delete a binary payload.

Do not hide a behavior change by changing tests or baselines.

## Validation

Run, at minimum:

```bash
git diff --check
python3 tools/repo/check_encoding.py
python3 tools/repo/check_eol.py
python3 tools/repo/check_case.py
cmake --preset linux-debug
cmake --build --preset linux-debug
ctest --preset linux-debug --output-on-failure
```

Also run GCC, Clang, ASan/UBSan, MinGW, and hosted CI where available or
record the exact local blocker.

Run M68, M69, M70, and M71 protected checks if their repository commands remain
available after the cleanup.

## Report

Write `docs/agents/reports/m72_misc_compile_flag_cleanup.md` with:

- starting SHA;
- branch;
- commit list;
- removed compile definitions;
- retained conditional sides;
- files changed;
- PC-9821 removal inventory;
- `VAEG_EXT` disposition;
- font/embed audit result;
- validation commands and exit statuses;
- hosted CI URL and result;
- deviations and remaining risks;
- G72 human-review checklist.
