<!--
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
-->

# M127 — restore the letter-key keypad overlay

Status: **G127 human gate passed on 2026-10-19; merged to main.**

## Request and history

After M126, the maintainer requested the old YUI-based keypad mapping as an
item immediately below Key > かな入力. Follow the commit history first.
M125 remains on hold; this task starts from main without its quick-save code.

- Original feature: M14, `136c4b5ef2632ec57886b7d5238962a8f909664d`.
- M106, `a55c447ab833b83fc99018507ebaa0bdb5ab7769`, replaced the letters
  with cursor keys at the maintainer's request.
- M110 integrated that cursor overlay into カーソル割り当て; the old
  letter-keypad setting must not repurpose the current cursor setting.

## Scope

- Restore Y/U/I = 7/8/9, H/J/K = 4/5/6, N/M/comma = 1/2/3, period = 0.
- Independent persisted boolean `keyboard_letter_keypad`, off by default.
- A checked menu item directly below かな入力, with a mapping tooltip.
- Preserve cursor assignment, JIS/US/custom layouts, and normal guest
  make/break injection. Overlay precedes Roman-Kana and US punctuation.
- Release held overlay keys when disabling, resetting or losing focus.
- No emulator-core, save-state format, private media or vendored changes.
- If cursor and letter overlays hold the same digit, send its break only
  after the last overlay source releases it. Enabling while a normal key is
  held ignores its autorepeat until a fresh keydown.

## Maintainer-requested pre-gate follow-up: FDD XZ visibility

The maintainer additionally requested `.d88.xz` and `.88d.xz` in FDD >
開く before G127. Keep this correction in a separate M127 commit. The
existing XZ extraction path already supports these images; the GUI filter
omits them and inspects only the last extension. Match complete,
case-insensitive suffixes, retain ZIP/7z/LZH, and reject unrelated `.xz`
and backup suffixes. Test the production filter and XZ extraction with
synthetic public fixtures; never record private media names or alter inputs.
Include this correction in the updated static gate build and bug ledger.

Implemented in [32cdedf6](https://github.com/nakatamaho/vaeg/commit/32cdedf64048b04ec46e0f56ee846da376bf54fd).
Production-filter tests cover both compound suffixes, mixed case, ZIP/7z/LZH,
bare `.xz`, backup suffixes and unchanged tape/SASI behavior. Four controlled
mutations (last-extension-only, missing `.88d.xz`, overbroad `.xz`, and no
case folding) fail with `M127_FDD_FILTER_ACCEPT` or
`M127_FDD_FILTER_REJECT` as intended. GCC and ASan CTest each pass 94/94
(four external skips). `linux-release` with fetched LibArchive 3.8.7 passes
selftest, including lower/upper-case D88/88D XZ extraction. Real GUI fixture
checks list all four case/suffix variants while omitting invalid suffixes;
selecting `.d88.xz` and `.88d.xz` through FDD Open mounts each and preserves
the source bytes. Fixtures use only a public source-built demo. Temporary
GUI code is removed. No private user media or names enter Git.

## Maintainer-requested pre-gate follow-up: partially missing recent boots

A recorded boot must remain selectable if some recorded media still exist,
not be disabled because a secondary floppy, HDD or tape is gone. Restore
existing media and explicitly eject missing media; show which were skipped.
Keep all-missing media records disabled and recheck at selection time, before
changing the running session. Media-less ROM boots remain valid. Verify
per-record isolation and partial/all-missing cases with synthetic fixtures,
and a real GUI selection using public demo media. Keep this fix in its own
commit and update the correctness ledger. Do not change the history format.

Implemented in [f85cec0b](https://github.com/nakatamaho/vaeg/commit/f85cec0b5fda6198845e149d0f58ff9ccefeef63).
The old menu required every nonempty media path in the record to exist.
The new `HistoryBootPlan` preserves existing paths and clears missing ones,
including the disk index of an absent floppy. Both the menu and the boot
action use this plan; the action checks again before session/mount changes.

Synthetic filesystem tests pass for complete, partial, all-missing,
media-less ROM and HDD records, per-record isolation and disappearance after
an earlier check. Mutations restoring the old all-paths gating, allowing
all-missing records, rejecting media-less records, and retaining missing
paths fail with their intended `M127_RECENT_PARTIAL`,
`M127_RECENT_ALL_MISSING` or `M127_RECENT_ROM_BOOT` codes. Every mutation is
restored before the integration checks.

A real GUI fixture shows a disabled all-missing entry, an enabled partial
entry and an enabled complete entry together. Selecting the partial entry
mounts its existing public demo, ejects a different disk previously mounted
in FDD2 and shows the missing FDD2/HDD/tape paths as a warning. Temporary GUI
code is removed; fixtures, screenshots and logs remain outside Git.

Final integrated GCC and ASan CTest each pass 94/94 (the same four external
skips). The fetched LibArchive 3.8.7 release selftest passes the filter,
recent-boot media, archive extraction and letter-keypad tests. The local
release worker for this final source tree has SHA-256
`eb2dfc0f97fd14ac1f7c48ed79285c3f7a4c2863817ca5e418f57b2811ab05d0`;
it was built before the fixing commit was recorded (its configure-time
version header still names `29067b97`). The clean static gate build must
embed the final pushed candidate instead. Media/demo source-build results
above are reused: `git diff da5988f575e2eb864685eb752a6e212b0b926afe HEAD
-- demos tools` is empty, so every generated-media build input is unchanged.
No unchanged media builders or all-demo build profiles are rerun solely for
these GUI-only fixes.

## Verification and G127

Run GCC and ASan/UBSan builds, CTest, smoke/selftest, focused mapping and
routing tests with mutations, repository invariant/format checks, and media
build regression from a clean checkout. Keep generated/private outputs out
of Git. Reuse unchanged expensive demo results only with explicit input
identity proof. Push the topic branch after local validation is final and
produce the clean static MinGW handoff.

Human gate: standard V3/demo/OS check, plus toggle the new item, check the
letter keypad in a guest, toggle off and check normal typing, restart and
check persistence. Cursor assignment must remain independent. Confirm that
FDD Open lists/mounts `.d88.xz` and `.88d.xz`, and that an existing recent
boot can be selected while missing secondary media are explicitly ejected. Stop at G127;
do not merge or begin the next milestone before maintainer approval.

## Local verification

Implementation: [da5988f5](https://github.com/nakatamaho/vaeg/commit/da5988f575e2eb864685eb752a6e212b0b926afe).

- `cmake --preset linux-ci-gcc`, `cmake --build --preset linux-ci-gcc -j16`:
  exit 0; no new warnings. The existing `saved_auto`/`saved_tenkey_overlay`
  maybe-uninitialized warnings in `kbdmap_selftest` remain unchanged.
- `ctest --test-dir build/linux-ci-gcc -j12 --output-on-failure`: 94/94,
  four external-fixture skips. `--selftest`: all tests pass, including
  `letter keypad`; `--no-cfg --no-bkupmem --smoke --mute`: exit 0.
- `linux-ci-asan` configure/build and
  `ASAN_OPTIONS=detect_leaks=0 ctest --test-dir build/linux-ci-asan -j12
  --output-on-failure`: 94/94, the same four skips, no sanitizer reports.
- The routing test independently supplies the ten expected VA keypad codes
  through keystat, then compares their keyboard-matrix effects with host
  keydown/keyup in JIS, US and custom layouts, with Roman-Kana active.
  It also checks ordinary typing when off, disable/reset release, autorepeat
  after enabling, cursor-setting independence and a shared held digit.
- Six one-mutation checks fail with their intended stable code:
  wrong Y mapping (`M127_MAPPING`), omitted keyup (`M127_KEYUP`), omitted
  disable release (`M127_DISABLE`), omitted reset release (`M127_RESET`),
  remapped autorepeat (`M127_REPEAT`), and premature shared-digit release
  (`M127_SHARED_HOLD`). Every mutation was restored before validation.
- GUI harness: the item appears immediately below かな入力; clicking it
  writes `keyboard_letter_keypad=true`, restart retains its checkmark,
  while `keyboard_tenkey_overlay=false` and `cursor_keys=0` are unchanged.
  Screenshots and configuration remain private scratch outputs, outside Git.
  The temporary GUI harness was removed and the production binary rebuilt.
- Encoding, EOL, path case, Markdown comments, `git diff --check`, and
  `git clang-format --binary ~/work/pc88va/cfbin/clang-format-mp-22`: pass.
  No binary payloads or archived-reference files changed. This restoration
  is a feature, not a claimed fix of an existing guest defect.

### Media regression

From a fresh local clone of `da5988f575e2eb864685eb752a6e212b0b926afe`:

```sh
tools/pc88va/build-utility-disk.sh --source "$PCE110" --output "$NEW_FDD"
tools/pc88va/build-sasi-utility-disks.sh --source-va "$PCE105" \
  --source-va2 "$PCE110" --payload-d88 "$NEW_FDD" --output-dir "$NEW_HDDS"
```

Both commands exit 0; the utility FDD has 4096 bytes free. Private input
values are deliberately omitted. Generated outputs stay outside Git:

| Output | SHA-256 |
|---|---|
| Utility FDD | `2bb5f17f1066a4cb3c911a09d1bd75d38b8005d83d3c744748bf8f32bdb7c45a` |
| SASI VA | `03ae60013a2f52a8a775935f323cc1d0c128ba7bcc11b62299f10f0f9e84ec73` |
| SASI VA2 | `dfc262de0d7aa052739f5abcf85ab9174a67b27b7115687df1aaabb92a2ca7ad` |

ROM-enabled smoke runs with the generated FDD and each SASI mounted alone
on its model all exit 0; their output hashes remain unchanged. These are
smoke checks, not a new claim about every bundled program's visible output.

The M124 demo-build results are reused, not rerun: `demos` has the same
Git tree `a9e33fa1c343704b3b3c1fbd493d419a5ef781bd` at M124's evaluated
`9af8cb4ea808f617251cf8ec72812ded3af1197d` and this implementation. Under
`tools/` the only differences since that baseline are M126's HDD wrapper
and HDD-only Softlib staging helper, neither used by demo builders. Demo
assembly, other build tools and committed demo media are unchanged. This
reuses source-build/payload results only, not unchanged-worker runtime
results: the frontend worker has changed. Human V3/demo/OS and letter-keypad
verification is still required at G127.

The initial letter-keypad GCC worker after restoring its mutations and GUI harness has
SHA-256 `4d7cf8e771563460be8966960a8c6aae96defabed7457db294b6df38f300056b`.
Raw logs, generated images and captures are retained in task-local scratch.
All 129 regenerated FDD payloads also match the previous utility disk.

### Initial static Windows handoff (superseded by the follow-ups)

`tools/release/build-mingw-static-container.sh --commit
 da5988f575e2eb864685eb752a6e212b0b926afe` exited 0, using a fresh clone in
a pinned Ubuntu 24.04 container. Source commit/date embedding and the PE
import audit pass (Windows system DLLs only). The executable SHA-256 is
`0843af77c2ea7ea7052875d9878ef106023e43f65bd0c61a6d31c03162af2fdc`.
The absolute `vaeg.exe` and `BUILD-REPORT.md` paths are supplied to the
maintainer. Windows execution and the G127 human checks remain pending.
The report-only checkpoint did not require rebuilding. The later FDD filter
and recent-boot corrections require an updated gate executable.

## G127 passed and main integration

The maintainer stated **G127 gate passed** on 2026-10-19, accepting the
standard gate and the letter-keypad, FDD XZ and partial recent-boot follow-ups.
This closes the pending checks at the earlier verification checkpoints.

Final gate candidate:
[cb35e722](https://github.com/nakatamaho/vaeg/commit/cb35e722d7460f5376af4a93abac0b7e802737ca).
The clean static MinGW build of that pushed commit exited 0 and verified
commit/date embedding and Windows-system-DLL-only imports. Executable
SHA-256: `7583d8f0152c09a18524da62fb78b28eb00660fa710eeb071fd2dcfbad5444f8`.
All hosted checks passed for that candidate, including Linux GCC/Clang/ASan,
macOS, Windows compatibility/release, standalone conformance, native SST,
repository invariants and the guest-driver distribution:
[CI](https://github.com/nakatamaho/vaeg/actions/runs/39139369006).

[PR #44](https://github.com/nakatamaho/vaeg/pull/44) had already been merged
with a two-parent merge commit at
[c7a14dd0](https://github.com/nakatamaho/vaeg/commit/c7a14dd03b071c7cc3e67fb2ed576e69f6f6e5c1).
The final topic candidate is contained in main; no duplicate merge is needed.
The gate and roadmap record is a documentation-only follow-up based on
origin/main, not another emulator change or an authorization for M125.

After pushing the gate record, remove only M127's clean merged worktree/local
branch, its temporary gate-record worktree, its initial/final static MinGW
build directories and `/tmp/m127/`. Keep remote branches, user-supplied
assets, shared caches, the private reference tree, utility HDD outputs and
all other milestones' worktrees untouched.
