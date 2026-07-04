# AGENTS.md — vaeg (88VA Eternal Grafx)

PC-88VA emulator derived from Neko Project II. Upstream (project-vaeg)
is abandoned; this fork is the living tree.

Two build lineages exist:

- LEGACY (frozen reference): `win9x/np2.dsp` / VS2017 v141. Pulls the
  VA subsystem (`iova/`, `vramva/`, `cpucva/`, `cpuxva/`, `biosva/`,
  `i286x/`) plus the shared NP2 core (root, `io/`, `sound/`, `cbus/`,
  `vram/`, ...). Depends on NINE NASM custom builds and x86 inline
  conventions. It is the behavioral reference until M13 retires it.
  Do not refactor it; do not "improve" it.
- PORTABLE (active target): CMake + gcc/clang/MinGW, SDL2 frontend under
  `sdl2/`, Dear ImGui GUI, macOS / Linux / Windows-MinGW. Built from
  M7 onward. Uses the C cores only: `i286c/` (CPU), `cpucva/z80c.cpp`
  (Z80 side), `sound/opngenc.c` (never define `OPNGENX86`), and the
  C port of the VA memory layer created in M9 (`cpucva/memoryva.c`).

`sdl/` is the historical SDL1 frontend (plain PC-98, no VA). It is
porting source material for M8 and is retired in M13. Do not extend it.

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

## Repository invariants (steady state since phase 1)

- Sources are UTF-8 without BOM. Never introduce CP932 content.
  Exemption: `hlp/` stays CP932 (HTML Help Workshop requirement).
- EOL is LF. Exceptions (`.dsp/.dsw/.sln/.vcproj/.vcxproj` = CRLF) are
  enforced by `.gitattributes`; do not add new CRLF files.
- All tracked paths are lowercase. New files must be lowercase.
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
- The LEGACY v141 build must keep compiling until M13 explicitly
  retires it. If your change breaks it, say so in the PR; do not hide it.
- Core code (root, `io/`, `sound/`, `cbus/`, `vram/`, `*va/`, `i286c/`)
  stays C. C++17 is allowed only under `sdl2/` (frontend + GUI).
- Vendored third-party code lives under `external/` with the exact
  version recorded in `docs/agents/DECISIONS/`. Never hand-edit it.
- Run the machine checks named in your task file (`tools/repo/*.py`,
  cmake builds) and paste their output into the PR description.

## Build reference

- LEGACY: VS2017 toolset v141, Win32/Release,
  `/source-charset:utf-8` + execution charset CP932 (M6 Option A),
  `.rc` in UTF-8 with `#pragma code_page(65001)`. NASM required.
- PORTABLE: CMake >= 3.20. `cmake --preset linux-debug` etc. from M7.
  SDL2 via find_package/pkg-config (never FetchContent). Dear ImGui
  vendored under `external/imgui` from M10.

## Commit messages

UTF-8, LF, English subject, `M<n>:` prefix. Example:
`M9: port cpuxva/memoryva.x86 to C (cpucva/memoryva.c)`.
