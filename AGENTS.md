# AGENTS.md — vaeg (88VA Eternal Grafx)

PC-88VA emulator derived from Neko Project II. Upstream (project-vaeg)
is abandoned; this fork is the living tree.

The active tree is the portable CMake build: C core, SDL2 frontend under
`sdl2/`, Dear ImGui GUI, and macOS / Linux / Windows-MinGW support. It
uses `i286c/` for the CPU, `cpucva/z80c.cpp` for the Z80 side,
`sound/opngenc.c` for OPN generation (never define `OPNGENX86`), and
`cpucva/memoryva.c` for the VA memory layer.

A frozen reference tier remains for behavior archaeology only:

- `win9x/`: VS2017 v141 Win32 reference frontend and project files.
- `i286x/`: x86 assembly CPU reference used by the v141 build.
- `cpuxva/memoryva.x86`: original VA memory assembly reference.
- `hlp/`: CP932 HTML Help payload paired with the frozen Win32 tree.

Do not refactor or improve the frozen reference tier. Normal fixes land
in the active CMake/C/SDL2 tree. The frozen tier is protected by tags and
history, not by a current compile guarantee or CI job.

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

- Sources are UTF-8 without BOM. Never introduce CP932 content.
  Exemption: `hlp/` stays CP932 (HTML Help Workshop requirement).
- EOL is LF. Exceptions (`.dsp/.dsw/.sln/.vcproj/.vcxproj` = CRLF) are
  enforced by `.gitattributes`; do not add new CRLF files.
- Tracked paths are lowercase except tool- or project-mandated names listed
  by `tools/repo/check_case.py`, including top-level `CHANGES*.md` release
  notes. New paths must otherwise be lowercase.
- Never modify binary payloads: `romimage/`, ROM/disk images, fonts,
  icons, cursors, wave data.
- Never re-encode, re-wrap, or reformat lines you are not changing.

## Hard rules

- One concern per commit. Renames (`git mv`) go in rename-only commits;
  reference fixups go in a separate, immediately following commit.
- Deletions happen only from a list the user has explicitly approved.
- New files created in phase 2 carry a 2-clause BSD header
  `Copyright (c) 2026 Nakata Maho` (see CONVENTIONS.md §New code).
  Never alter copyright headers of existing files.
- Frozen reference files (`win9x/`, `i286x/`, `cpuxva/memoryva.x86`,
  `hlp/`) are reference-only. Do not edit them unless a task explicitly
  says to update the reference tier.
- Core code (root, `io/`, `sound/`, `cbus/`, `vram/`, `*va/`, `i286c/`)
  stays C. C++17 is allowed only under `sdl2/` (frontend + GUI).
- Vendored third-party code lives under `external/` with the exact
  version recorded in `docs/agents/DECISIONS/`. Never hand-edit it.
- Run the machine checks named in your task file (`tools/repo/*.py`,
  cmake builds) and paste their output into the PR description.

## Build reference

- Active: CMake >= 3.20. `cmake --preset linux-debug` etc. SDL2 is
  discovered via find_package/pkg-config by default, with the
  ADR-0006-pinned FetchContent path only where a preset opts into it.
  Dear ImGui is vendored under `external/imgui`.
- Frozen reference: `win9x/np2_v141.sln` / VS2017 v141 / Win32 remains
  available for behavior comparison, but it is no longer an active build
  target with CI or compile-guarantee coverage.

## Commit messages

UTF-8, LF, English subject, `M<n>:` prefix. Example:
`M9: port cpuxva/memoryva.x86 to C (cpucva/memoryva.c)`.
