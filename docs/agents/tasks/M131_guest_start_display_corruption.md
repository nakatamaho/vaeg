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

# M131 — Guest display corruption after Start

Status: **Correction implemented; local validation passed; G131 not requested.**

Branch: `milestone/m131-display-corruption`. Commit prefix: `M131:`.

## Request and scope

The maintainer reports that private integration fixture `guest-start-01`
boots, but pressing Enter to start causes severe screen corruption.
Reproduce on current main, isolate the first incorrect emulator operation,
and implement an evidence-backed correction with a focused regression test.
Do not modify the guest disk, publish its contents, or infer hardware behavior
from emulator output alone. Private media identities, raw traces, screenshots
and save states remain outside Git in task-specific private scratch.

## Plan

1. Build a recording-enabled macOS diagnostic target and establish reproducible
   captures before and after the Enter operation with clean configuration.
2. Narrow the failure using existing debug scripts, CPU/I/O and graphics
   diagnostics. Separate observed facts from hypotheses.
3. Fix the demonstrated cause, add an isolated regression test, and update the
   permanent bug-fix ledger in the fixing change.
4. Run focused and full applicable tests, repository encoding/EOL/case/Markdown
   checks and formatting/scope checks. Record commands, exit status and results.
5. Complete the mandatory clean-clone demo/media rebuild and runtime regression;
   produce a fresh fully static MinGW handoff from the pushed commit.

## Gate G131 (human)

Standard V3 boot, bundled demo, OS boot and simple operations, plus
`guest-start-01`: press Enter at Start and confirm that subsequent screens
render correctly. Supply the static Windows executable path, SHA-256 and
`BUILD-REPORT.md` path. No next milestone until maintainer acceptance.

## Findings and correction

- The fixture contains two D88 images. Both must be mounted: disk 1 in FDD1,
  disk 2 in FDD2. A single-drive CLI run waits for the absent disk and does not
  reach the reported Start screen; it is not the reproduction.
- Baseline `b69cdbec5b309e9951b3f2137c7c1f0442081bf2`
  with temporary out-of-tree-retained diagnostics reproduces the corrupted
  post-Start display. Independent reconstruction from GVRAM shows intact
  graphics. The extra characters and large bands originate in the text plane.
- The renderer's M103h fallback to main RAM tests 88 mode and port 32h TMODE,
  but not the S-mode preference, although its documented purpose is the explicit
  standard-speed extension. In H mode the guest reuses the underlying RAM for
  non-text game data while fast TVRAM holds blank text rows. The fallback
  incorrectly interprets that data as characters and text attributes.
- `maketextva_3301_row` now additionally requires `np2cfg.v1v2_standard` before
  selecting main RAM. Both rendering and text copy share this correction.
  No CPU, GVRAM, disk, palette, save-state layout or archived source changes.
- A Zilog flag-profile diagnostic A/B did not remove the failure. No diagnostic
  switches, raw traces, guest data or private identities are retained in the
  production patch.

## Local verification

The local build is recording ON, OS-native MP4 enabled, local bundled encoders
OFF, dynamic MacPorts FFmpeg, static fetched SDL2/LibArchive, dynamic optional
librashader loading. This is a macOS diagnostic build, not the static Windows
clean-checkout handoff. Commands below use `$SOURCE` and `$PRIVATE` as neutral
placeholders for the repository and task scratch; all raw output is private.

```sh
cmake -S "$SOURCE" -B "$PRIVATE/build" -G Ninja \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_PREFIX_PATH=/opt/local \
  -DVAEG_ENABLE_RECORDING=ON -DVAEG_STATIC_FFMPEG=OFF \
  -DVAEG_ENABLE_LOCAL_MP4_ENCODERS=OFF -DVAEG_STATIC_LIBRASHADER=OFF \
  -DVAEG_ENABLE_TESTS=ON -DVAEG_Z80_COMPAT_INTEGRATION_TRACE=ON
cmake --build "$PRIVATE/build" -j8
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy "$PRIVATE/build/sdl2/vaeg" --selftest
ctest --test-dir "$PRIVATE/build" --output-on-failure -j8
```

- Configure/build: exit 0. Initial configure without a librashader archive was
  blocked; the diagnostic configuration explicitly uses dynamic loading instead.
- Red test: only the new selftest added to the old helper, exit 1 with
  `H mode TMODE 1 displayed main RAM instead of fast TVRAM`.
- Corrected selftest: exit 0, `TSP 3301 emulation ok`.
- CTest: exit 0, 140 listed, zero failed, nine skipped (SST corpus, two role
  probes, ROM-dependent BASIC boot and five xz shell tests). No hosted CI run.
- Regression coverage: H/TMODE1 keeps fast TVRAM for pixels and text copy;
  S/TMODE1 retains main-RAM pixels and text copy; S/TMODE0 keeps TVRAM; V3 with
  a saved S preference keeps TVRAM; CPU memory-window precedence stays tested.
- Private runtime: fresh configuration and no backup memory, both disks mounted,
  `@wait 1800`, `@enter` (after the input harness's initial 600 frames). Capture
  frames 1790, 2700 and 3600 for VA and VA2, plus frame 6000 for VA2. Both
  models show coherent gameplay instead of text corruption. This is emulator
  output, not hardware verification or maintainer acceptance. Brief palette
  flashes during the transition are not the text-corruption defect.
- `check_case.py`, `check_encoding.py`, `check_eol.py`, `check_md_comments.py`
  and `git diff --check`: exit 0. Whole-file pinned clang-format dry-run finds
  pre-existing formatting drift in `sdl2/selftest.c`; only the newly added
  region is normalized, with no unrelated formatting changes.

## Remaining gate work

Fixing commit identity, final worker digest and pushed-commit verification are
recorded after creation. Clean-clone demo/media source closure, utility model
boots and the fresh static Windows handoff remain pending. No G131 pass claim.
