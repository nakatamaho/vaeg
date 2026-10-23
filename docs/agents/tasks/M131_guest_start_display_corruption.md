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

## Pushed correction and final worker

Fixing commit:
[e0be89d2133822514c188f81a2b6f7190b10bf04](https://github.com/nakatamaho/vaeg/commit/e0be89d2133822514c188f81a2b6f7190b10bf04),
pushed to `origin/milestone/m131-display-corruption`.

After configuring/building that pushed commit, the macOS worker SHA-256 is
`ff629acc9ecec05c9492391cd1b5da15e25b9114a6a98f2bbc25e1e10db872a3`.
Its selftest exits 0. The earlier complete CTest run evaluated the identical
corrected production helper; subsequent code edits only made the new selftest's
text-copy assertions NULL-safe and normalized their formatting. Unrelated
expensive tests are not repeated for documentation/build-identity changes.

A local launcher mounts both fixture images with fresh configuration; it and
all captures remain outside Git. Neither firmware nor the guest disk was patched.

## Clean-clone media build regression

Source: a separate clean clone of pushed `e0be89d2133822514c188f81a2b6f7190b10bf04`.
All commands below exit 0 with fresh output paths. `$S` denotes that clone,
`$G` task scratch, `$T` the private 80-cylinder 2HD template, `$P` the supplied
artwork and `$D` the pinned MS-DOS source from the shared cache. No private
input identity or raw capture is published.

- Each of `glass-orbit`, `neon3`, `neon4`, `sgp-pseudo-sprite` and
  `sgp-wireframe`: `$S/demos/<component>/build-d88.sh "$T" "$G/<new>.d88"`.
- `$S/demos/zundamon-orbit/build-from-psd.sh --python /gate/psd-env/bin/python
  "$P" "$T" "$G/<new>.d88"`, inside the task-local Ubuntu container.
- `python3 "$S/tools/pc88va/build-all-demos-distribution-disk.py" --source "$T"
  --distribution-dir "$G/distributions-final" --output "$G/final-all-demos.d88"
  --compressed-output "$G/final-all-demos.d88.xz"`.
- The analogous `build-all-demos-msdos4-disk.py` command uses `$D` as source
  and fresh `final-all-demos-msdos4` output paths.
- `python3 "$S/demos/sgp-boot/build-boot-d88.py" --output "$G/sgp-boot.d88"
  --compressed-output "$G/sgp-boot.d88.xz"`.
- `python3 "$S/tools/pc88va/build-all-demos-bootable-disk.py" --source "$T"
  --distribution-dir "$G/distributions-final" --output "$G/final-all-demos-pce.d88"`.
- `$S/tools/pc88va/build-utility-disk.sh --source "$T" --output "$G/utility.d88"`:
  125 installed files, 4096 bytes free. Host GNU dd and a DOSBox-X command
  bridge are used; the Open Watcom build uses the isolated task Docker daemon.

All six component distributions and both aggregates have **byte-identical stored
file maps** to their committed archives (1/2/2/8/3/2 component files, 19 all-demos
files, 25 MS-DOS files). The self-booting SGP raw disk is exactly identical;
the entire MS-DOS aggregate raw image is also identical. FAT timestamp differences
in other raw images are not treated as payload differences.

Initial private input preflights exposed a 77-cylinder image and then a
write-protected template; these attempts fail, not pass. A separate correct
80-cylinder writable input is used without altering the supplied originals.
Initial NASM 3.01/ImageMagick builds differ from the committed NEON3 and
Zundamon payloads. NASM 2.16.03 still differs in equivalent XCHG register
encodings. The already documented M100s11 reproduction recipe resolves these
without changing any source or distribution: Ubuntu Python 3.12.3, NASM 2.16.01,
psd-tools 1.26.0, Pillow 13.0.0, NumPy 2.5.4, no ImageMagick in PATH.
The five successful unaffected component builds are reused; only NEON3,
Zundamon and the affected aggregates are rebuilt. Failed attempts, resolver
report and successful comparisons are preserved privately, not relabeled.

Raw generated image SHA-256 values (not source-input hashes):

| Image | SHA-256 |
|---|---|
| GLASS | `a414801fbd955943131056ee6cc1ea3394f5b83533a9a8240da6b2b41fa63aa5` |
| NEON3 | `d7a1d9e14a39b85b86beea6470346291f20e074866d0b43e8242b90d5235ee37` |
| NEON4 | `81cfae7a7d672bd31afff4b147f7c74a456f324324ccc1edfc7fd60b609a9833` |
| pseudo-sprite | `57c71e3b85fa5aa3fe8d15287bc11c755f38de17346f0ae5fd0644336563e48f` |
| wireframe | `c20d04394e1dba43444a3d08eda116830398bbd5403420f14ef503941296184e` |
| Zundamon | `75722d5df1c852d0b60a2847d5cdfd7337e553370a3939dc925d6b52ba3ff342` |
| all-demos | `bd2b77405b7f376455809b0d09515cbe20fe2f506f87c948b439e6667da94d65` |
| all-demos-msdos4 | `e88c0e0332094048bcf55b334cd0430fc1c77e158344b00e57946ad9bbc84a78` |
| self-booting SGP | `67ead94dba0cee7aa9d9a0268cf1987bf3a2764fe1275e092b2e5ece06337154` |
| utility FDD | `a262905ea1d47d2e4b769efd2d165a5eb57f162cf802d883fd91b9d6ac6c506c` |
| PC-Engine bootable all-demos | `89ea7b9949cb7196c51a85d7b507fd191f95eaa4b90b029878dae2ecaa4e35e7` |

Runtime checks with the final macOS worker: the newly generated utility FDD,
PC-Engine all-demos image and MS-DOS all-demos image each reach their OS prompt
at frame 1200, exit 0, with dummy SDL and isolated config/backup state. This is
boot coverage only, not a claim that all programs or MANDEL were run.

## Fresh static Windows preview

```sh
tools/release/build-mingw-static-container.sh \
  --commit e0be89d2133822514c188f81a2b6f7190b10bf04 --recording
```

Exit 0. Fresh clone, application objects and pinned shader SDK; recording/OS
MP4 ON, local-only bundled encoders explicitly OFF. Minimal static FFmpeg,
encoder-registration audit and Windows system-DLL import audit pass. The local
VM/configuration/disks and Docker data are under the repository's ignored
`build/container-runtime/`, with an explicitly selected local socket, not the
shared default daemon.

- `../vaeg-mingw-e0be89d2/output/vaeg.exe`, 59,221,494 bytes.
- SHA-256: `76dc753844d54869e84ca8ee1e7a23a57fb5e3c340bc8586dbcb23761458416f`.
- Build report: `../vaeg-mingw-e0be89d2/output/BUILD-REPORT.md`.
- Windows execution is not inferred from compilation/import checks. This is
  available for targeted defect confirmation, **not a G131 gate request**.

## Remaining gate work

- Generate both SASI utility images and boot/DIR each alone on its model.
  The three required generated CP/M inputs are not currently present in task
  scratch; SASI generation is NOT_RUN, not a passing or failing image build.
- Launch every all-demos program under both OSes and verify drawing; run
  `MANDEL.BAS` from N88-BASIC and exercise the self-booting SGP demo.
- Complete the standard V3/demo/OS human checklist and the targeted fixture
  check only after all mandatory machine/media checks are complete.

M131 remains open. No human acceptance, Windows runtime, hardware behavior,
next milestone or cleanup approval is inferred from this investigation.
