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
Implementation complete; G152 is blocked by inherited repository-wide format violations.
See the [debug-script reference](../../../sdl2/README.md#held-keys).
