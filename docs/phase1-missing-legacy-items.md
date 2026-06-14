# Phase 1.5 / 1.6 Legacy Item Check

## Repository State

- HEAD before check: `637f62f Document legacy Visual Studio project inventory`
- Branch: `main`
- Untracked instruction files intentionally excluded: `AGENTS.md`, `refactor-instructions.md`

## Phase 1.5: `writetag.cpp`

Result: no tracked `writetag.cpp` file exists.

Commands:

```sh
git ls-files '*writetag*'
git grep -in "writetag" -- .
```

Findings:

- `git ls-files '*writetag*'` returned no tracked files.
- `git grep -in "writetag" -- .` only matched existing documentation in `docs/phase1-inventory.md`.
- No source, project, script, or build input references `writetag`.

Decision:

- No file is moved or deleted in this phase.
- If a future merge introduces `writetag.cpp`, inspect references before proposing relocation or deletion.

## Phase 1.6: Bundled zlib

Result: no bundled zlib source or zlib API usage is present.

Command:

```sh
git grep -in -E "zlib|inflate|deflate" -- .
```

Findings:

- The search only matched existing documentation in `docs/phase1-inventory.md`.
- No source, project, script, or build input references `zlib`, `inflate`, or `deflate`.
- `accessories/lzxpack` is not treated as zlib; its compression behavior is left untouched.

Decision:

- No dependency is added, removed, or replaced in this phase.
- If zlib is introduced later, prefer a system dependency such as `find_package(ZLIB)` in a future CMake build, with vendoring only as a fallback after review.

## Verification

- This phase is documentation and repository inspection only.
- Windows build gate was not rerun.
- The previous verified gate remains: Debug and Release Win32 passed after `76b4e15`, with demo and PC-Engine runtime checks performed by the owner.
