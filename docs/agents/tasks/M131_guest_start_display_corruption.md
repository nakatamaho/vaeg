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

Status: **Provisional H-mode workaround; display-source policy unresolved; no merge or G131 request.**

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

## Findings and provisional workaround

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
  selecting main RAM. Both rendering and text copy share this provisional change.
  Removing the H-mode symptom does not validate the remaining S-mode selector.
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

## Maintainer feedback and unresolved V2S case

The maintainer confirmed the supplied correction, then separately reported
that `guest-start-01` still has a corrupted display in V2S. This is targeted
confirmation of the earlier correction, not a full G131 pass.

The current helper deliberately preserves the S-mode main-RAM text path, so
that path remains a candidate for the V2S symptom. The exact V2S cause and
correct display-source policy are not yet established by a trace or isolated
experiment. Do not conclude that the guest is incompatible with V2S merely
from this result. The historical V1S BASIC observation motivates preserving
coverage, but does not prove that either the original TMODE-only fallback or
the new H/S-qualified fallback is a correct general rule. Resolve the guest's
text-source selection without regressing BASIC before declaring the defect
fully closed. The maintainer and agent agree that the present patch is
provisional and must not yet be merged to main.

## Manual review: CPU banking is not the display source

Independently summarized from the supplied reference material; source copies,
OCR text, private input identities and captures remain outside Git.

- *PC-8801 Series Machine-Language Game Graphics*, section 1-5, printed
  pp. 33-34 (figure 1-5-1 and LIST 1.7), distinguishes standard-mode main-RAM
  text from the separate fast text RAM used in V1H and V2. In the latter modes,
  the main RAM beneath F000h-FFFFh is an additional usable 4 KiB, not another
  image of the displayed text. The example selects that RAM, stores data, then
  explicitly copies it to the separate fast RAM to make it visible.
- Its figure 2-1-1, printed p. 39, assigns port 32h bit 4 to the CPU bank
  selection: 0 selects fast RAM; 1 selects underlying main memory. Consequently
  TMODE=1 does **not** imply that display fetches switch to main RAM in H mode
  or V2. The original renderer fallback conflated these two functions.
- *PC-8801/mkII/SR Analysis Manual Compilation*, section 5-4, describes the
  character/attribute layout and the CRTC/DMAC setup, including channel 2's
  display start and terminal count. These are display-fetch parameters, not
  merely the current CPU-visible bank.
- *PC-8801 Disk N88-BASIC Analysis Manual*, printed p. 231, identifies the
  default text area as F3C8h-FFF7h. *PC-Techknow8801 Vol. 1*, section 3-1-4,
  describes 120-byte rows (80 characters plus 40 attribute bytes), totaling
  3000 bytes. Thus the entire F000h-FFFFh bank is not synonymous with the BASIC
  screen: work areas and the display's programmed start must remain distinct.

The manual establishes a real distinction between V1S and V1H/V2. It does not
specify vaeg's artificial V2S combination or establish that clearing the speed
identification bit in V2 must change display storage. In particular, the
current `np2cfg.v1v2_standard && TMODE` renderer selector remains unproven for
V2. Trace the effective guest mode and firmware's display setup before replacing
it; do not equate an S menu preference with a hardware display-source signal.

## Explicit eight-case game boot matrix

Evaluated production commit and worker are the unchanged `e0be89d2` build
identified above; no production edits or rebuild occurred for this comparison.
Each run has an isolated INI and backup record: `BootMode=1`, explicit
`V1V2_Standard=false/true`, B1FC5h bit 0 and the memory-switch checksum initialized
for V2/V1, both fixture images mounted, and the same scripted Enter at frame
2400. Model, configuration, backup, input and output paths are case-specific.
The originals are not modified. Exit 0 in all eight runs is process completion,
not by itself a successful game boot.

| Model | Requested mode | Result at frame 3600 |
|---|---|---|
| VA | V1H | Gameplay draws coherently after Start |
| VA | V1S | Guest displays `V1-H OR V2 MODE ONLY`; gameplay not reached |
| VA | V2H | Gameplay draws coherently after Start |
| VA | V2S | Gameplay reached; severe text overlay after Start |
| VA2 | V1H | Gameplay draws coherently after Start |
| VA2 | V1S | Guest displays `V1-H OR V2 MODE ONLY`; gameplay not reached |
| VA2 | V2H | Gameplay draws coherently after Start |
| VA2 | V2S | Gameplay reached; severe text overlay after Start |

The saved V1/V2 bit agrees with each requested mode after all eight runs.
The V1S rejection is a guest-visible observation, not an extrapolation from
V2S corruption. The rejection explicitly permits V2; it does not establish
that V2S corruption is correct or that the game is incompatible with V2S.

Additional BASIC boot probes, with separate neutral V1 and V2 system fixtures,
reach N88-BASIC 1.9 (V1) or 2.4 (V2) and an `Ok` prompt in all eight model/mode
combinations. The first PRINT probe was consumed by `How many files(0-15)?`,
so those runs demonstrate boot/display only, not command execution. A separate
query run first answers that prompt, then issues PRINT; both attempts are
retained rather than treating the first as an executed command. All eight
query runs exit 0 and display `M131 2` followed by the requested port value:

| Mode | BASIC version | IN 31h, VA | IN 31h, VA2 |
|---|---|---|---|
| V1H | 1.9 | F9h | F9h |
| V1S | 1.9 | B9h | B9h |
| V2H | 2.4 | 79h | 79h |
| V2S | 2.4 | 39h | 39h |

The query input waits 600 frames beyond the harness's initial 600, sends Enter
for the file-count question, waits 300, then sends
`PRINT "M131";1+1;" PORT31=";HEX$(INP(&H31))`. The final capture is frame 3000.
The second set uses fresh per-case copies of the first runs' configuration,
backup and writable disk copy; no original media is written. Port 31h bits 7
and 6 confirm the intended V1/V2 and H/S identification in these BASIC runs.
These are current-patch observations, not a fallback-disabled A/B or proof of
real VA S-mode behavior. The old claim that BASIC necessarily requires the
existing renderer fallback still needs that controlled A/B.

Documentation-only follow-up: `check_encoding.py`, `check_eol.py`,
`check_case.py`, `check_md_comments.py` and `git diff --check` all exit 0.
The emulator worker and build-relevant sources are unchanged, so no unrelated
build, CTest, media rebuild or hosted CI is repeated.

## Remaining gate work

- Investigate and resolve the maintainer-reported V2S display corruption,
  preserving S-mode BASIC text display. The H-mode correction alone does not
  establish V2S compatibility.
- Generate both SASI utility images and boot/DIR each alone on its model.
  The three required generated CP/M inputs are not currently present in task
  scratch; SASI generation is NOT_RUN, not a passing or failing image build.
- Launch every all-demos program under both OSes and verify drawing; run
  `MANDEL.BAS` from N88-BASIC and exercise the self-booting SGP demo.
- Complete the standard V3/demo/OS human checklist and the targeted fixture
  check only after all mandatory machine/media checks are complete.

M131 remains open. No human acceptance, Windows runtime, hardware behavior,
next milestone or cleanup approval is inferred from this investigation.
