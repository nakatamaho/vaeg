# AGENTS.md — vaeg (88VA Eternal Grafx)

PC-88VA emulator derived from Neko Project II. Upstream (project-vaeg)
is abandoned; this fork is the living tree.

The active tree is the portable CMake build: C core, SDL2 frontend under
`sdl2/`, Dear ImGui GUI, and macOS / Linux / Windows-MinGW support. It
uses `cpu/upd9002/` for the main CPU instruction core, the built-in CPU
register model in `iova/upd9002_regs.*`, the suzukiplan-backed wrapper in
`cpucva/z80_core.cpp` for the Z80 side, `sound/opngenc.c` for OPN generation
(never define `OPNGENX86`), and `cpucva/memoryva.c` for the VA memory layer.

The former frozen reference tier (`win9x/`, `i286x/`,
`cpuxva/memoryva.x86`, and `hlp/`) was removed from the current tree in
M57. Its exact G56 snapshot remains available at the annotated tag
`archive/frozen-win9x-i286x-g56` for behavior archaeology and provenance.
Normal fixes land in the active CMake/C/SDL2 tree.

## How work is organized

All work is milestone-driven. Read, in this order:

1. `docs/agents/ROADMAP.md`       — milestone sequence and gates
2. `docs/agents/CONVENTIONS.md`   — repo invariants and new-code rules
3. `docs/agents/tasks/M*.md`      — the single task file you were assigned

Do exactly ONE milestone task per session. Every milestone ends at a
HUMAN GATE unless the task file states the gate is machine-verifiable.
The standard human gate is: build from clean checkout, boot in V3 mode,
run the bundled VA demo, boot an OS and perform simple operations.
Never begin milestone N+1 until the user states that gate N passed.
Always push the branch and report the exact commit SHAs when done.

## Permanent bug-fix ledger

`docs/modernization/bug-fixes.md` is the permanent correctness ledger for
this fork. Update it in the same milestone and normally in the same commit
whenever a change fixes incorrect guest-visible behavior, a portability
crash or data-corruption risk, persistence/media handling, or a regression.

Each entry must identify the symptom, affected scope, demonstrated root
cause, correction, verification, milestone/task, and commit. Do not record a
hypothesis as a root cause. Keep unresolved defects in the ledger's open
section until evidence and a tested correction exist. Feature additions and
cosmetic restorations do not belong there unless they also correct a concrete
defect. Evidence documents and fixing commits must be clickable Markdown
links; commit links use the repository's GitHub commit URL with the full SHA.
Release notes may summarize the ledger but do not replace it.

## Repository invariants (steady state since phase 1)

- Sources and documentation are UTF-8 without BOM. Never introduce CP932
  content.
- EOL is LF throughout the current tree. Tool-mandated project formats remain
  declared in `.gitattributes`; do not add new CRLF files without an explicit
  repository-policy exception.
- Tracked paths are lowercase except tool- or project-mandated names listed
  by `tools/repo/check_case.py`, including top-level `CHANGES*.md` release
  notes. New paths must otherwise be lowercase.
- Never modify binary payloads: `romimage/`, ROM/disk images, fonts,
  icons, cursors, wave data.
- Treat private integration asset identities as sensitive. Tracked files must
  use neutral stable test identifiers; do not record private filenames,
  absolute paths, or hashes unless the maintainer explicitly authorizes that
  exact metadata. Keep raw private screenshots, logs, traces, and save files
  outside Git, and never stage, commit, or push them.
- Never re-encode, re-wrap, or reformat lines you are not changing.

## Hard rules

- One concern per commit. Renames (`git mv`) go in rename-only commits;
  reference fixups go in a separate, immediately following commit.
- Deletions happen only from a list the user has explicitly approved.
- New files created in phase 2 carry a 2-clause BSD header
  `Copyright (c) 2026 Nakata Maho` (see CONVENTIONS.md §New code).
  Never alter copyright headers of existing files.
- Do not restore the M57-deleted reference-tier paths (`win9x/`, `i286x/`,
  `cpuxva/memoryva.x86`, or `hlp/`) unless a task explicitly requires it.
  Use tag `archive/frozen-win9x-i286x-g56` for historical comparison.
- Core code (root, `io/`, `sound/`, `cbus/`, `vram/`, `*va/`,
  `cpu/upd9002/`)
  stays C. C++17 is allowed under `sdl2/` (frontend + GUI) and, when an
  approved third-party CPU core requires it, for CPU backends and thin
  compatibility adapters under `cpucva/`. C++ and STL types must not cross
  existing C-facing subsystem or state-save interfaces. Other newly written
  emulator-core code remains C99 unless an ADR separately approves it.
- Vendored third-party code lives under `external/` with the exact
  version recorded in `docs/agents/DECISIONS/`. Never hand-edit it.
- Run the machine checks named in your task file (`tools/repo/*.py`,
  cmake builds) and paste their output into the PR description.

## Build reference

- Active: CMake >= 3.20. `cmake --preset linux-debug` etc. SDL2 is
  discovered via find_package/pkg-config by default, with the
  ADR-0006-pinned FetchContent path only where a preset opts into it.
  Dear ImGui is vendored under `external/imgui`.
- Archived reference: the former VS2017 v141/Win32 tree is available only at
  tag `archive/frozen-win9x-i286x-g56`; it is not a current build target and
  has no CI or compile-guarantee coverage.

### Maintainer release handoff

When the maintainer says to copy release builds to "the usual place", the
destination is:

`/mnt/c/Users/maho/Dropbox/Documents/Emulators/PASOCON/NEC PC-88VA/vaeg_new/`

- Copy `build/linux-release/sdl2/vaeg` there as `vaeg`.
- Copy the MinGW release executable there as `vaeg.exe`; its source is
  `build/mingw-release/sdl2/vaeg.exe` for a native build or
  `build/mingw-cross/sdl2/vaeg.exe` for the Linux cross-build.
- Verify each copied executable against its build artifact with `cmp` or
  SHA-256.

This is a maintainer-local directory outside Git. Unless explicitly requested,
replace only `vaeg` and `vaeg.exe`; do not alter or inventory its ROMs, disk
images, configuration, save data, or other private integration assets.

## Commit messages

UTF-8, LF, English subject, `M<n>:` prefix. Example:
`M57: remove frozen reference tier`.
