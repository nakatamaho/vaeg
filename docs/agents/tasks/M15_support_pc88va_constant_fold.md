# M15 — Treat PC-88VA support as an active-tree invariant

Status: proposed  
Branch: `topic/m15-support-pc88va-fold`  
Gate: G15 (machine checks; human VA boot/demo check if the maintainer wants an A/B confirmation)  
Depends on: G14 passed.

## Goal

The active CMake/SDL2 tree always builds the PC-88VA machine. Therefore
`SUPPORT_PC88VA` is no longer a useful compile-time feature flag in the
active tree.

M15 folds `SUPPORT_PC88VA` to true in active code and removes the CMake
compile definition. This is intended to be a behavior-preserving cleanup:
only code that was already compiled in the active build remains.

The frozen reference tier remains untouched and keeps whatever
`SUPPORT_PC88VA` archaeology it needs.

## Non-goals

Do not mix any of these into M15:

- uPD9002/uPD70002 naming work;
- V30 instruction integration;
- Z80 core replacement;
- `VAEG_FIX` folding;
- `VAEG_EXT` folding;
- removal of runtime machine-model checks;
- PC-98/EPSON/9821 runtime behavior cleanup;
- formatting or indentation sweeps.

## Constraints

- Do not modify the frozen reference tier:
  - `win9x/`
  - `i286x/`
  - `cpuxva/memoryva.x86`
  - `hlp/`
- Do not modify `docs/agents/reports/raw/`.
- Historical reports under `docs/agents/reports/` may mention
  `SUPPORT_PC88VA`; leave them alone unless the maintainer explicitly
  asks for a historical-note update.
- `VAEG_FIX` and `VAEG_EXT` are separate compile-time controls and must
  remain as-is.
- Runtime model checks such as `pccore.model_va != PCMODEL_NOTVA`,
  `PCMODEL_VA1`, and `PCMODEL_VA2` must remain. `SUPPORT_PC88VA` is a
  compile-time invariant; `pccore.model_va` is guest runtime state.
- Do not remove PC-98, EPSON, or PC-9821 compatibility branches just
  because this task removes a VA compile-time guard.
- Keep edits mechanical and local to changed preprocessor regions.
- Preserve UTF-8 without BOM, LF line endings, and lowercase path rules.

## Target scope

Active build files only:

- root C files and headers;
- `CMakeLists.txt`;
- `sdl2/`;
- `io/`;
- `sound/`;
- `fdd/`;
- `cbus/`;
- `bios/`;
- `biosva/`;
- `cpucva/`;
- `iova/`;
- `vram/`;
- `vramva/`;
- `generic/`;
- current active CPU core directory (`i286c/` unless renamed by a later
  milestone);
- necessary non-raw docs that describe active-tree invariants.

Do not edit vendored third-party code.

## Required audit

Before changing code, list:

1. CMake definition sites for `SUPPORT_PC88VA`.
2. Active-tree source/header/table uses.
3. Excluded uses in frozen reference paths.
4. Historical-report mentions that will intentionally remain.

Use a command equivalent to:

```sh
rg -n "SUPPORT_PC88VA" . \
  --glob '!win9x/**' \
  --glob '!i286x/**' \
  --glob '!cpuxva/**' \
  --glob '!hlp/**' \
  --glob '!docs/agents/reports/raw/**'
```

## Mechanical folding rules

Apply these transformations only where `SUPPORT_PC88VA` is the condition
being folded:

- `#if defined(SUPPORT_PC88VA)` -> keep the true branch.
- `#ifdef SUPPORT_PC88VA` -> keep the true branch.
- `#ifndef SUPPORT_PC88VA` -> remove the guarded branch or keep the
  `#else` branch if one exists.
- `#if !defined(SUPPORT_PC88VA)` -> remove the guarded branch or keep
  the `#else` branch if one exists.
- `#if defined(VAEG_EXT) || defined(SUPPORT_PC88VA)` -> keep the true
  branch.
- `#if defined(SUPPORT_PC88VA) || ...` -> keep the true branch.
- `#if defined(SUPPORT_PC88VA) && defined(X)` -> simplify to
  `#if defined(X)`.
- `#if defined(SUPPORT_PC88VA) && !defined(X)` -> simplify to
  `#if !defined(X)`.

Do not simplify standalone `VAEG_FIX`, standalone `VAEG_EXT`, or runtime
machine checks.

When a file is wrapped entirely in `#if defined(SUPPORT_PC88VA)`, remove
only the wrapper. Do not reindent the file.

## CMake update

Remove `SUPPORT_PC88VA` from active target compile definitions. The
active target must still define the other existing controls, including
`USE_I286C`, `SUPPORT_BMS`, `SUPPORT_V30ORIGINAL`, and `VAEG_FIX`.

## Documentation update

Update active architecture documentation if it states that VA helpers are
conditional on `SUPPORT_PC88VA`. The correct post-M15 wording is that the
active CMake/SDL2 tree is always built with PC-88VA support.

Do not rewrite historical reports for this milestone.

## Required verification

Run these checks, or explain exactly why any command is unavailable:

```sh
rg -n "SUPPORT_PC88VA" . \
  --glob '!win9x/**' \
  --glob '!i286x/**' \
  --glob '!cpuxva/**' \
  --glob '!hlp/**' \
  --glob '!docs/agents/reports/**'

cmake --preset macos-release
cmake --build --preset macos-release
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy ./build/macos-release/sdl2/vaeg --smoke
SDL_AUDIODRIVER=dummy ./build/macos-release/sdl2/vaeg --selftest
git diff --check
python3 tools/repo/check_encoding.py
python3 tools/repo/check_eol.py
python3 tools/repo/check_case.py
```

The `rg` command above intentionally excludes all historical reports.
If the maintainer specifically wants to see historical-report mentions,
run the raw-only-excluded variant separately and label those results as
historical residue.

## Deliverables

- Local code/docs changes or one mechanical commit if the maintainer
  asks for commit/push.
- A short report listing:
  - files changed;
  - where `SUPPORT_PC88VA` was removed;
  - where `SUPPORT_PC88VA` remains by design;
  - confirmation that runtime `pccore.model_va` checks remain;
  - verification command results;
  - residual risk.

## Gate G15

G15 is primarily a machine gate because the intended change is
compile-time constant folding of code already used by the active build.

The maintainer may still request the standard manual VA check before
acceptance:

- clean build;
- V3-mode boot;
- bundled VA demo;
- OS boot and simple commands.
