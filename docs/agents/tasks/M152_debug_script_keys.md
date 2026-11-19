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
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
-->

# M152 — debug-script: held keys and video-register capture

Branch `topic/m152-debug-script-keys` from `origin/main` (b1ad18d0), worktree `../vaeg-m152`.
Commit subjects `M152: <english imperative>`. Read AGENTS.md, docs/agents/CONVENTIONS.md and
docs/agents/tasks/M74_debug_harness.md (the debug harness's design) first.

## Motivation
Automated tests of PC-88VA software (e.g. a game port driven by keyboard) need to hold keys
for a number of frames, release them, and send keys that are not printable ASCII (ESC, cursor
keys, keypad, function keys, SHIFT/CTRL). The `--debug-script` interface currently offers only
`input-line` (printable ASCII + Enter) and `enter`. Its `capture ... registers` export has CPU
registers only; a capture cannot show the video state (display start addresses, pitch, palette,
palette mode, screen enables) that decides how GVRAM appears.

## Scope
1. New debug-script commands (scheduled by frame like the existing ones):
   - `key-down <key>` and `key-up <key>`; `<key>` is a name from one documented table covering
     every key of the PC-88VA keyboard matrix (letters, digits, ESC, RETURN, SPACE, TAB, BS,
     cursor keys, keypad 0-9 and its operators, F1-F10, SHIFT, CTRL, GRPH, KANA, CAPS, INS, DEL,
     HOME/CLR, HELP, ...), mapped to the same keyboard-matrix bits the SDL frontend uses.
   - optionally `key-tap <key> [frames]` = down, then up after N frames (default 1).
   - Keys still held when the script ends or at `exit` are released. Invalid names are a parse
     error with the line number, as other commands.
2. `capture <id> ... video`: a new capture item writing the video state as a stable, versioned,
   documented text or JSON file: graphics display start addresses and pitches per screen,
   palette entries and palette mode, screen enable/priority registers, display mode (15/24 kHz),
   and whatever else `scrndrawva.c` uses to compose the picture. Read from the emulator's state,
   not reconstructed.
3. Optionally (only if small): allow the legacy headless input interface and `--debug-script` in
   one run, or document why not.
4. Tests: CTest unit/contract tests for the parser (valid/invalid key names, ordering, release at
   end) and for the video capture format; a headless end-to-end check that a held key is seen by
   the guest for exactly the scheduled frames (a free synthetic MS-DOS 4.0 or self-booting test
   program that reads the keyboard matrix and shows/records the state; no copyrighted data).
5. Docs: the debug-script reference updated (command list, key-name table, capture format);
   ROADMAP row M152; this task file's status section.

## Gate (machine-verifiable)
Clean configure + build (Linux, `-DVAEG_ENABLE_TESTS=ON`), all CTest tests pass, repository
validators pass (clang-format manifest checker etc. per CONVENTIONS), the end-to-end held-key
check passes on the VA and VA2 models. Existing debug-script behaviour unchanged (existing
harness self-tests pass). Push the branch; open no PR (the PR is opened after review).

## Status

**G152 PASS under the review-round scope.** Fresh Linux configure/build,
all enabled CTest cases (including the VA and VA2 guest checks), and the
formatter on all M152-touched C sources and headers passed. The 33 inherited
repository-wide format findings are out of scope, not a gate blocker. This
is branch review evidence, not merge or release acceptance.

Evaluated source: [6c40ac5d922164dd7ce5cb4385f6954be2fb90ea](https://github.com/nakatamaho/vaeg/commit/6c40ac5d922164dd7ce5cb4385f6954be2fb90ea),
following the original commit chain and status commit `a8a9ad7d169eba49c3663b52a433f642e98ae71d`.
The fix implements sequential `key-tap` (default 1 frame), paste backpressure,
line-numbered held-key overlap errors, same-frame warnings, and release on a
completed frame after a PC stop. Scripted transitions participate in the
keyboard's reset-resend and CAPS/KANA LED bookkeeping. The expanded video
schema stays at `vaeg-video-v1` because the initial draft never shipped;
it adds the applied VA board model, 15 kHz status and five TSP fields.

Commands from the worktree, with a new build directory:

```sh
export TMPDIR=/home/maho/work/pc88va/tmp
b=/home/maho/work/vaeg-m152-build/gate-fix1
cmake -S . -B "$b" -G Ninja -DCMAKE_BUILD_TYPE=Debug \
  -DVAEG_ENABLE_TESTS=ON -DVAEG_Z80_COMPAT_INTEGRATION_TRACE=ON \
  -DVAEG_ENABLE_LOCAL_MP4_ENCODERS=OFF -DVAEG_BUILD_TRACK=m152
cmake --build "$b" -j 6
ctest --test-dir "$b" -j 6 --output-on-failure
ctest --test-dir "$b" --output-on-failure -R '^vaeg_romless_tests$'
ctest --test-dir "$b" --output-on-failure -R '^vaeg_m75_transfer_info_compiled$'
```

Configure and build exited 0. The first CTest invocation reached 159 of 161
configured results before the command runner's 300-second build-and-test
budget expired. The two outstanding tests were run separately on the same
binary and both commands exited 0. Combined: **150 passed, 10 skipped by
existing optional-test contracts, one disabled, zero failed**. The M74
harness and trace equivalence passed. The three focused M152 cases also
exited 0 when invoked directly from the build directory:

```text
PASS M152 parser: matrix names, same-frame warnings, tap, paste and cleanup contracts
PASS M152 va: five VRTC samples; RIGHT/SHIFTR 1 frame, A 3 frames; live video v1
PASS M152 va: keyboard reset, CAPS/KANA LED bookkeeping and PC-stop cleanup
PASS M152 va2: five VRTC samples; RIGHT/SHIFTR 1 frame, A 3 frames; live video v1
PASS M152 va2: keyboard reset, CAPS/KANA LED bookkeeping and PC-stop cleanup
```

The executable (`gate-fix1/sdl2/vaeg`) reports `Rel.20261119 (m152, 6c40ac5d)`.
Local SHA-256 identifiers (all paths relative to `gate-fix1`):

| Artifact | SHA-256 |
| --- | --- |
| `sdl2/vaeg` | `09c3be1501770a914672c20f983a9d11e0023768fabb86d6d39c67b832ef26b7` |
| `ctest.log` | `a0f8160e4e0960af8567a618fc86aae5e72e7327d83646ce286ac434072a8d97` |
| `ctest-romless.log` | `27a5352d842ca97329a703d5c3987e1aa30c7ab77a481bdb9ccd39ff25a52725` |
| `ctest-transfer.log` | `8410c2844b905afbaa8b78c3dcfc5fed1f2de58e2e90c4b825afb5f6f57f444e` |
| `m152.log` | `a8f9bff927a39c66c31e2204f8fa38d6add2da7847aeebb22672ce5931376434` |

Encoding, the task's EOL check, path case, Markdown comments and `git diff
--check` exited 0. clang-format 22.1.8 passed on `io/serial.c`, `io/serial.h`,
`machine/keystat.c`, `machine/keystat.h`, `machine/statsave.h` and
`sdl2/debug_harness.c`. The full `tools/repo/clang_format.py` exited 1 with the
same 33 paths and diagnostic locations as `b1ad18d0` with a sorted manifest;
those files and `.clang-format` are unchanged from that base. The full log
matches the earlier format log digest below. An extra `check_eol.py --enforce`
run reports one pre-existing mismatch: the checker expects CRLF by extension
for `tools/hardware/hyperframe-plus/build.bat`, which `.gitattributes` explicitly
sets to LF. Both files are unchanged from the base. No unrelated formatting
or policy changes were made.

### Original pre-review gate record

**Implementation complete; G152 was BLOCKED at this checkpoint.** The clean Linux build and executable
checks passed. The repository-wide format gate did not. No branch push, PR,
merge or release was performed. See the
[debug-script reference](../../../sdl2/README.md#held-keys).

Evaluated source: `7af1077c0339adf8319828ea5207757504262264`, based on
`b1ad18d0f67adf6f550d4a089341432198b1abf4`. Commit chain:

- `3aa3badddaf6447d3f6cf04cd4df6786b7c5a448` — scheduled keys and video capture.
- `858d3be65301d75b6f5dac1feb4bb17f47c84825` — CLI contracts and native guest probe.
- `bdb08442aaca4d38af478f24daf44e939b24253e` — restore sort order in the format manifest.
- `2a0a295d8e6e2ff715e9afc9519111f1af42d7a9` — development build metadata.
- `7af1077c0339adf8319828ea5207757504262264` — reference, task and roadmap.

Commands from the M152 worktree (`b` was a fresh out-of-tree directory):

```sh
export TMPDIR=/home/maho/work/pc88va/tmp
b=/home/maho/work/vaeg-m152-build/gate
cmake -S . -B "$b" -G Ninja -DCMAKE_BUILD_TYPE=Debug \
  -DVAEG_ENABLE_TESTS=ON -DVAEG_Z80_COMPAT_INTEGRATION_TRACE=ON \
  -DVAEG_ENABLE_LOCAL_MP4_ENCODERS=OFF -DVAEG_BUILD_TRACK=m152
cmake --build "$b" -j 8
ctest --test-dir "$b" -j 8 --output-on-failure
```

All three commands exited 0. Ninja finished at `[760/760] Linking CXX
executable sdl2/vaeg`. CTest printed `100% tests passed, 0 tests failed out of
160`: 150 passed, 10 skipped by the existing optional-test contracts, and
one additional configured test was disabled. The M74 harness, ROM-less
selftests and trace equivalence passed. The synthetic guest and parser results
were:

```text
PASS M152 va: five consecutive VRTC samples; RIGHT/SHIFTR 1 frame, A 3 frames; live video v1
PASS M152 va2: five consecutive VRTC samples; RIGHT/SHIFTR 1 frame, A 3 frames; live video v1
PASS M152 parser: matrix names, line diagnostics, alias/idempotence, exit/end/limit cleanup
```

The executable is `gate/sdl2/vaeg` under `/home/maho/work/vaeg-m152-build`,
reports `Rel.20261119 (m152, 7af1077c)`, and has SHA-256
`71bc195397b50461c474f525b792002c9fa66d226c0703d60189b38223138d84`.
The out-of-tree `gate-ctest.log` has SHA-256
`693ca3897923fd58d4fb8b0fc1b391f4b48325e3d55ad21aa50bf95912c3c5cd`.
The synthetic fixture and comparison contract are defined in
`tests/m152/debug_script.py` at the evaluated SHA. SDL dummy execution is not
presenter or physical-hardware coverage.

Repository checks:

| Command | Result |
| --- | --- |
| `python3 tools/repo/check_encoding.py` | exit 0 |
| `python3 tools/repo/check_eol.py` | exit 0 |
| `python3 tools/repo/check_case.py` | exit 0, `0 finding(s)` |
| `python3 tools/repo/check_md_comments.py` | exit 0, `0 finding(s)` |
| `git diff --check b1ad18d0 HEAD` | exit 0 |
| `clang-format-mp-22 --dry-run --Werror --style=file --fallback-style=none io/serial.c io/serial.h sdl2/debug_harness.c` | exit 0 |
| `python3 tools/repo/clang_format.py` (clang-format 22.1.8) | exit 1: 33 unchanged source files have format violations. First: `cpu/upd9002/upd9002_core.c:170:14`. |

At the base SHA, the manifest validator stopped first on `manifest paths are
not sorted`. Correcting the order exposed the source violations; `git diff
--exit-code b1ad18d0 HEAD -- <the 33 paths from clang_format.log>` returned 0.
The full diagnostic log is `/home/maho/work/vaeg-m152-build/clang_format.log`
(SHA-256 `820861f85591fc6b4177ff4c6837262aa391a80f724953bde7e67ff3de011f69`).
Unrelated source lines were not reformatted and the checker was not weakened.
The build/test evidence applies to the evaluated SHA; this Status update is
documentation-only.

## G152 record

2026-11-19: the maintainer stated that G152 passed. Gate build `8c6a2e44`
(the M152 branch with main `8eac2218` merged in; Linux host MinGW clean build,
`Rel.20261119 (m152, 8c6a2e44)`, SHA-256
`016153f78d5e5b69d43a1fa400253e052b8fbf31821ed40c05960d3d6ea81cbc`; encoder
and Windows system-DLL import audits PASS). No release is authorized.
