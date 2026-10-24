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

Status: **G131 requested: M130-integrated candidate 3326349a passes CTest,
24 isolated compatibility cases, media regeneration/payload closure and the
34-case all-demos drawing check. Clean static Windows handoff available;
human acceptance and merge pending. Full-tree formatting retains baseline
drift; the 89 M131 edited C/C++ lines pass. H/S timing and IN6Eh remain
separate issues #58/#59.**

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

## X88000 and quasi88 source comparison

This is static implementation comparison, not a claim of guest execution in
those emulators or a substitute for VA firmware/hardware evidence. Reference
checkouts remain outside Git; no third-party implementation is copied into
vaeg. This comparison uses these exact public revisions, rather than silently
assuming they are the X88000 1.5.3 reference cited by earlier milestones:

- [nakatamaho/x88000, 1606677251c49c5d980953e13b42be5812ea99e7](https://github.com/nakatamaho/x88000/tree/1606677251c49c5d980953e13b42be5812ea99e7),
  source directory `x88_1_5_1_src`.
- [waitingmoon/quasi88, 19898729564def241def40c663c4d78759dad203](https://github.com/waitingmoon/quasi88/tree/19898729564def241def40c663c4d78759dad203),
  README identifies version 0.6.5.

### X88000

- [`PC88Z80Main.cpp`, Reset, lines 712-733](https://github.com/nakatamaho/x88000/blob/1606677251c49c5d980953e13b42be5812ea99e7/x88_1_5_1_src/PC88Z80Main.cpp#L712-L733):
  N88 V2 unconditionally sets `m_bFastTVRamUse=true`. The default branch uses
  `IsHighSpeedMode()` instead. Thus V2 fast-text selection does not depend on
  the H/S setting.
- [`PC88Z80Main.h`, GetTVRam, lines 824-833](https://github.com/nakatamaho/x88000/blob/1606677251c49c5d980953e13b42be5812ea99e7/x88_1_5_1_src/PC88Z80Main.h#L824-L833):
  display fetch uses fast TVRAM when fast-text use is enabled and the effective
  DMA address is at least F000h. It does not consult `m_bFastTVRamDisable`.
  Below F000h it reads main RAM; the programmed display address still matters.
- [`PC88Z80Main.cpp`, MemBankFastTVRamMode, lines 1681-1696](https://github.com/nakatamaho/x88000/blob/1606677251c49c5d980953e13b42be5812ea99e7/x88_1_5_1_src/PC88Z80Main.cpp#L1681-L1696):
  the disable flag does select CPU read/write handlers for the F000h bank,
  subject to the graphics mapping. `WriteIO_32` passes port 32h bit 4 to this
  function. It does not change the display's fast-text-use flag.
- [`ReadIO_31`, lines 2378-2383](https://github.com/nakatamaho/x88000/blob/1606677251c49c5d980953e13b42be5812ea99e7/x88_1_5_1_src/PC88Z80Main.cpp#L2378-L2383)
  separately reports the high-speed setting in bit 6 and V1/V2 in bit 7.
  Even if the high-speed setting is false with V2, Reset's V2 branch still
  selects fast text. This is a code-path observation, not a claim that every
  frontend exposes a V2S option.

### quasi88

- [`pc88main.c`, boot mode initialization, lines 2790-2815](https://github.com/waitingmoon/quasi88/blob/19898729564def241def40c663c4d78759dad203/src/pc88main.c#L2790-L2815):
  V1S clears `high_mode`; V1H and V2 set it. V2 also sets the H identification
  bit. There is no independent BASIC_V2S mode in the mode enumeration.
- [`pc88main.c`, memory-layout note, lines 243-252](https://github.com/waitingmoon/quasi88/blob/19898729564def241def40c663c4d78759dad203/src/pc88main.c#L243-L252):
  in high mode, `main_ram[F000h..FFFFh]` represents **fast display RAM**, while
  `main_high_ram[0..0FFFh]` represents the **underlying main RAM**. Variable
  names alone would misleadingly suggest the reverse.
- [`main_memory_mapping_c000_ffff`, lines 436-461](https://github.com/waitingmoon/quasi88/blob/19898729564def241def40c663c4d78759dad203/src/pc88main.c#L436-L461):
  with high mode and port 32h bit 4 set, CPU access selects `main_high_ram`.
  Otherwise, absent the dictionary mapping, it selects `main_ram`. This is
  CPU banking, not a change to the text renderer's source.
- [`crtcdmac.c`, character/attribute fetches](https://github.com/waitingmoon/quasi88/blob/19898729564def241def40c663c4d78759dad203/src/crtcdmac.c#L481-L507)
  read `main_ram` using the programmed DMA addresses. The transparent-attribute
  path does likewise. Consequently V2 text continues to use the fast backing
  at F000h even while the CPU selects underlying RAM.

### Consequence for M131

Both implementations agree with the manual distinction: V2 uses fast text;
TMODE CPU banking does not redirect that display source. X88000 additionally
makes the V2 selection independent of the H/S setting. quasi88 has no V2S
selection, so it must not be reported as having passed a V2S runtime test.

This strongly supports expecting coherent rendering in vaeg V2S and rejects
S preference alone as justification for displaying unrelated underlying RAM.
The candidate policy must distinguish V1S from V2 (and respect the programmed
display address), rather than treating every S preference as main-RAM display.
CPU banking in V1S also deserves separate coverage: the references disable fast
text there irrespective of the CPU's current TMODE bank selection. Translating
this to the VA's emulated 3301/TSP window still needs implementation and focused
BASIC/game regression tests. No production change, merge or gate pass is
claimed by this source comparison.

## M88-derived source and X88000-aligned follow-up

The maintainer requests alignment with X88000 after the comparison above.
The earlier instruction not to merge remains in force; this authorizes the
follow-up implementation, not milestone acceptance.

The additional reference is the M88-derived macOS port
[bubio/M88M at c6e3300d9331e4fa5d9af667232201a7571a63d1](https://github.com/bubio/M88M/tree/c6e3300d9331e4fa5d9af667232201a7571a63d1),
not a claim that an unmodified original M88 release was executed:

- [`pc88.cpp`, lines 270-277](https://github.com/bubio/M88M/blob/c6e3300d9331e4fa5d9af667232201a7571a63d1/src/pc88/pc88.cpp#L270-L277)
  connects display DMA to TVRAM when IN 31h bit 6 is set, otherwise to main
  RAM. This connection is separate from subsequent CPU banking.
- [`memory.cpp`, UpdateF0](https://github.com/bubio/M88M/blob/c6e3300d9331e4fa5d9af667232201a7571a63d1/src/pc88/memory.cpp#L836-L837)
  selects CPU TVRAM access only when bit 6 allows fast text and port 32h bit 4
  is clear. TMODE does not reconnect display DMA.
- [`config.h`, lines 21-22](https://github.com/bubio/M88M/blob/c6e3300d9331e4fa5d9af667232201a7571a63d1/src/pc88/config.h#L21-L22)
  encodes V1S/V1H/V2 as 20h/21h/31h;
  [`Base::Reset`](https://github.com/bubio/M88M/blob/c6e3300d9331e4fa5d9af667232201a7571a63d1/src/pc88/base.cpp#L63-L67)
  derives the H bit from bit 0 of this mode. Supported V2 therefore enables
  fast text. This is consistent with X88000 for supported N88 modes, but
  does not directly test vaeg's artificial V2S combination.

The follow-up uses one shared C policy, `memoryva_88_fast_text()`: the effective
V1 identification is MS27 (backup B1FC6h bit 7, also returned by IN 31h), and
only effective V1 with the explicit S preference disables fast text. V2 keeps
fast text irrespective of the S preference. No new saved-state field is added.

- Display rasterization and copied text select main RAM only for V1S, regardless
  of current TMODE. The existing TSP window/address translation is retained.
- CPU F000h access still observes graphics/dictionary/extended-memory precedence.
  Fast text is available in V1H/V2 only and selected with TMODE clear; V1S
  accesses main RAM even with TMODE clear, matching X88000.
- The main-RAM display sleep guard follows this same policy, rather than TMODE,
  so V1S RAM changes cannot be hidden by the fast-TV RAM dirty flag.
- S-mode IN 31h identification, CPU timing, ROMs, media and V3 behavior are not
  changed by this follow-up.

The focused selftest now crosses V1/V2, H/S and both TMODE values, checking
raster pixels, copied text and CPU read/write bank isolation. It retains the
V3-with-saved-S and GVRAM-priority checks. Against the old provisional helper,
the new test exits 1 with `V2 display source depends on S preference or TMODE`.
With the shared policy it exits 0, `TSP 3301 emulation ok`. The recording-ON
macOS build succeeds; full CTest exits 0, 140 listed, zero failed, the same nine
skipped tests as before. Formatting-only changes after that run do not alter
the executed logic. Runtime and final pushed-worker identities will be recorded
separately, without relabeling the earlier eight-case matrix as fixed output.

## Pushed follow-up runtime verification

Evaluated source commit:
[bf215b998a616d07c9c909383c4160f5712f4597](https://github.com/nakatamaho/vaeg/commit/bf215b998a616d07c9c909383c4160f5712f4597).
The recording-ON macOS worker rebuilt from that pushed commit has SHA-256
`e032203ecf1dd7fee94c57b8f0e1fa08f284c6640a7f65d3f10f35cb22d4cb3b`.
The final worker's selftest and `ctest --test-dir "$PRIVATE/build"
--output-on-failure -j8` both exit 0 (140 listed, zero failed, nine skipped;
96.98 seconds for CTest).

The same eight-case contract documented above is rerun in new, isolated output
directories: fresh per-case configuration/backup/disk copy, explicit V1/V2 and
H/S selection, both fixture images mounted, Enter at frame 2400, screenshots
at frames 1790 and 3600. All eight game runs exit 0 and their saved mode bits
agree with the request. On **both VA and VA2**, V1H, V2H **and V2S** now show
coherent gameplay after Start; V1S still displays the guest's explicit
`V1-H OR V2 MODE ONLY` rejection. This is not an assertion that V1S gameplay
works. The previous V2S overlay disappears without changing the guest media.

All eight BASIC boot and all eight corrected PRINT/query runs also exit 0.
BASIC versions and IN 31h results remain those in the earlier query table;
every query prints `M131 2`. In particular V1S still displays BASIC and its
output. These comparisons use the same neutral V1/V2 BASIC fixtures as before;
the newly supplied common-system fixture is a separate probe-media input.
Raw logs, commands, captures and output digests remain in private scratch.

Repository encoding/EOL/case/Markdown-comment checks and `git diff --check`
exit 0. New C regions are normalized with clang-format-mp-22. The whole-manifest
format checker is blocked by the pre-existing `manifest paths are not sorted`
error; its manifest, configuration and checker are identical to pre-follow-up
commit `9be7754d`. No unrelated manifest/source formatting is changed.

## Hardware probe preparation and correction of earlier assertions

The maintainer states that VA/VA2 can select V1S/V1H/V2S/V2H. The agent's earlier
claim that V2S could not be checked on these machines was premature and is
withdrawn. Record selected mode and actual IN 31h separately; manual memory-
switch descriptions and emulator source do not replace the hardware check.

The supplied non-Japanese N88-BASIC system fixture is described by the
maintainer as V1/V2 compatible. Its D88 is 2HD (154 tracks), not 2D. A copy is
extracted outside Git with a neutral identifier; the supplied archive and
read-only reference tree are unmodified. No system image is distributed in Git.
The source-only diagnostic and instructions live under
[`tools/pc88va/m131-tvram/`](../../../tools/pc88va/m131-tvram/README.md).
It measures CPU-bank aliasing and displayed A/B patterns in two TMODE phases,
then restores both banks and the original port 32h before reporting to BASIC.
Hardware output is not yet available; probe emulator/media verification remains
in progress. No original-M88 or physical-machine run is implied.

Initial probe build 01 loaded at 9000h and requested `CLEAR ,&H8FFF`. The common
disk BASIC rejected that reservation with `Illegal function call in 1000` in
all eight requested mode/model runs. Process exit 0 is not a probe pass: no
machine-code phase ran. The disk BASIC memory map places disk code and its
buffers above 8400h, so the diagnostic must reserve space above those areas,
not assume ROM-BASIC layout. Failed output and the first saved disk remain
private. Build 02 relocates code/stack to B000h-BFFFh, backup banks to
C000h-DFFFh, and requests `CLEAR ,&HAFFF`; that reservation succeeds. Its single
VA2/V2H preflight then exposes `Syntax error in 1050`: this N88 CALL syntax
requires an address variable, not a literal. The preflight's monochrome setup
also makes its text unreadable in the rendered capture; the TVRAM dump reveals
the error. Build 03 uses `A=&HB000:CALL A` and explicit colour-mode
`SCREEN 1:COLOR 7,0,0`. No machine-code restoration pass is inferred from the
failed preflight. Build 03 reaches a readable phase 0 but does not advance:
the initial SPACE mask used bit 7, whereas the compatibility matrix uses port
09h bit 6 (also consistent with the manual's two rotates). Build 04 corrects
both polling masks to 40h, retaining the same layout. The blocked preflight
is not a restoration pass and its captures are preserved. Build 04 completes
both phases and reports `RESTORE=1` and PRINT result 2 in a VA2/V2H preflight,
but its final BASIC text is still obscured: SCREEN selects graphics mode,
whereas CONSOLE's fourth argument selects colour text attributes. Build 05
explicitly issues `CONSOLE 0,25,0,1` before COLOR. The machine-code binary is
unchanged from build 04. Verify the final loader and readable result across
all cases before handoff.
The independent builder test verifies the
DATA loader exactly reproduces the binary, its checksum, license and layout,
and byte-identical output from two fresh builds.

## Final probe and private hardware handoff

Build 05's source-only diagnostic is ready for targeted hardware testing, not
a hardware-verified result. Probe source commit:
[acda0456ad9c7536d4b1da4843eedc1e720bde67](https://github.com/nakatamaho/vaeg/commit/acda0456ad9c7536d4b1da4843eedc1e720bde67).
A fresh out-of-tree probe rebuild of that pushed source exits 0;
`cmp` confirms its binary, BASIC loader and build report are byte-identical to
the tested handoff. The final commit adds only this identity record; no probe
payload or application input changes, so emulator/media runs are not repeated.
It uses the same final emulator worker/production
commit identified above; application sources and worker digest are unchanged.
The private common-system fixture is `basic-common-01`, separate from the
older V1/V2 boot fixtures and `guest-start-01`.

```sh
python3 tools/pc88va/m131-tvram/build.py \
  --assembler "$VAEG_Z80ASM" --output-dir "$PRIVATE/probe-build"
VAEG_Z80ASM="$VAEG_Z80ASM" python3 tools/pc88va/m131-tvram/test_build.py
```

Both commands exit 0 with z80asm 1.8. The integration test performs two fresh
builds and checks byte-identical output, the DATA loader/binary equality,
checksum, reserved layout, entry/result offsets, SPACE masks and required
BASIC setup/CALL syntax. No assembler is downloaded and no system image is
built or distributed by this public source-only builder.

Freely distributable probe-output identities (not system-image/input hashes):

- Machine-code bytes: 604; loader checksum: 62810; origin B000h.
- Binary SHA-256:
  `1f991a024393ea10aabd1aea6f9fae1689a19c1b8ac9ff85dc0e58558745d47d`.
- ASCII BASIC loader SHA-256:
  `bd99fd35ee8ad9715466beb08f76326796c0a47b4752b28acd6e7194a87bb656`.
- Assembly source SHA-256:
  `ccf5ca8e4b815b0732b527d84442be6fb4bd18a5532cff3868b8a90f8d307f8a`.

The saved BASIC program is installed by the guest's own SAVE on private disk
copies, then reloaded and run. All eight final emulator subprocesses exit 0.
Captures at frames 5500/6500 show the two stable phases; frame 9000 shows the
readable result, `RESTORE=1` and PRINT result 2. Both models give:

| Mode | IN31 | Original32 | Phase32 P0/P1 | CPU P0/P1 | Display P0/P1 | Restore / PRINT |
|---|---|---|---|---|---|---|
| V1H | F9h | 8Ah | 8Ah/9Ah | 41h/42h | A/A | 1 / 2 |
| V1S | B9h | 9Ah | 8Ah/9Ah | 42h/42h | B/B | 1 / 2 |
| V2H | 79h | AAh | AAh/BAh | 41h/42h | A/A | 1 / 2 |
| V2S | 39h | AAh | AAh/BAh | 41h/42h | A/A | 1 / 2 |

This is an **emulator** result table. It is not filled in as a hardware result.
The initial private aggregation script exits 1 because it looks for results
only in the raw TVRAM dump: V1S instead displays main RAM. Its false negatives
are retained, not relabeled as successful raw-dump checks. A separate rendered-
content check exits 0 for all eight cases: phase banners/labels match the
inspected A/B references and the result/PRINT rows are pixel-identical to the
successful fast-text reference. The eight result screens are also inspected
visually, including the V1S main-RAM results. No unchanged emulator run is
repeated merely to correct this private analysis helper.

The private handoff contains `m131-tvram-2hd.d88`, the source-only BASIC loader,
machine-code binary, build report and operator README outside Git. The D88 is
a copy of the tested VA2/V2H candidate, verified byte-for-byte at handoff. It
contains the maintainer-supplied system files and must **not** be uploaded or
committed. Use an appropriate D88 writer; write-protect the physical test disk
after preparation. The runtime routine itself performs no disk I/O. The input
archive/read-only reference tree and original media were not modified.

Repository encoding/EOL/case/Markdown-comment and diff-whitespace checks pass.
No hosted CI, new application rebuild, static Windows handoff or full milestone
gate is claimed for these source-only diagnostic/evidence changes. All failed
probe attempts and raw evidence remain private. The earlier Windows preview
predates the source-aligned correction and is not a corrected V2S executable.

## Maintainer hardware observations: current policy contradicted

The maintainer supplied 24 private hardware photographs and confirmed the
sequence: VA V1S, V1H, V2S, V2H, then VA2 V1S, V1H, V2S, V2H; three photographs
per case, in PHASE 0, PHASE 1, return order. Model/requested-mode assignments
come from that confirmation, not from interpreting IN31 bit 6. The probe is
from source commit
[acda0456ad9c7536d4b1da4843eedc1e720bde67](https://github.com/nakatamaho/vaeg/commit/acda0456ad9c7536d4b1da4843eedc1e720bde67),
with the freely distributable build identities recorded above.

All 24 photographs were visually inspected, with enlarged return panels for
reading numeric fields. Originals remain untouched outside Git. Only this
independently written transcription is published; source filenames, paths,
hashes, raw images and converted contact sheets remain private.

| Stable case | Requested mode | IN31 | Original32 | Phase32 P0/P1 | CPU P0/P1 | Display P0/P1 | Restore / BASIC |
|---|---|---|---|---|---|---|---|
| hw-va-v1s-01 | VA V1S | F9h | 8Ah | 8Ah/9Ah | 41h/42h | A/A | 1 / Ok |
| hw-va-v1h-01 | VA V1H | F9h | 8Ah | 8Ah/9Ah | 41h/42h | A/A | 1 / Ok |
| hw-va-v2s-01 | VA V2S | 79h | AAh | AAh/BAh | 41h/42h | A/A | 1 / Ok |
| hw-va-v2h-01 | VA V2H | 79h | AAh | AAh/BAh | 41h/42h | A/A | 1 / Ok |
| hw-va2-v1s-01 | VA2 V1S | F9h | 8Ah | 8Ah/9Ah | 41h/42h | A/A | 1 / Ok |
| hw-va2-v1h-01 | VA2 V1H | F9h | 8Ah | 8Ah/9Ah | 41h/42h | A/A | 1 / Ok |
| hw-va2-v2s-01 | VA2 V2S | 79h | AAh | AAh/BAh | 41h/42h | A/A | 1 / Ok |
| hw-va2-v2h-01 | VA2 V2H | 79h | AAh | AAh/BAh | 41h/42h | A/A | 1 / Ok |

Observed conclusions, limited to this probe and common BASIC boot:

- CPU selections remain distinct at F3C8h in all eight cases: 41h then 42h.
  The stable display continues to show the CPU-bank-0 A pattern with either
  TMODE value. This supports separating CPU banking from display sourcing.
- In the requested S cases, IN31 bit 6 is still set. V1 returns F9h and V2
  returns 79h, just as the corresponding H cases. Four selectable modes do
  **not** imply that this port reports their H/S difference on VA hardware.
- The current emulator's V1S result (CPU 42h/42h, display B/B, IN31 B9h) is
  contradicted. Its V2S display/CPU result agrees, but IN31 39h does not.
  H-mode display/CPU/port results agree at the measured points. Passing the
  current policy's selftest does not resolve either S-mode discrepancy.
- Every result photograph shows `RESTORE=1` and BASIC `Ok`. None shows the
  requested `PRINT 1+1`, so hardware PRINT is **not observed**, not a pass.
- No elapsed-time/clock measurement or IN6Eh result is present. These photos
  neither prove nor disprove the separate H/S clock proposal. Do not infer
  the selected speed from IN31, relabel S cases as H, or conclude that the
  selectable S modes do not exist.

The earlier X88000-derived V1S prediction and emulator table are retained as
historical predictions/results, not rewritten to match the photographs.
The evidence requires reviewing the VA-specific port-identification and RAM
availability policy separately from PC-8801 compatibility extensions. No
emulator correction is made in this evidence-only update; in particular, the
old BASIC compatibility path must not be removed without controlled regression
checks. No production worker, media payload or comparison input for the prior
emulator runs changes, so those expensive runs are not repeated.

## Hardware-driven correction requested by the maintainer

The maintainer explicitly requested correcting both mismatches above:

- Remove the V1S fast-RAM availability exception. CPU F000h-FFFFh still
  selects TVRAM with TMODE clear and underlying RAM with TMODE set, subject
  to the unchanged GVRAM/dictionary/extended-memory precedence.
- Remove the main-RAM display fallback. The 3301 renderer and copied text
  always use TVRAM; H/S, V1/V2 and CPU TMODE do not change that source.
  Retire the unused mode helper/RAM-row buffer and its main-RAM sleep guard.
- Return B1FC6h unchanged for IN31 in V3 and 88 mode. Do not mask bit 6 or
  mutate the backup record based on H/S.
- Do not change compatible execution timing, clock flags, save-state layout,
  menu selections or speed presets. Separate follow-ups:
  [H/S timing #58](https://github.com/nakatamaho/vaeg/issues/58) and
  [IN6Eh identification #59](https://github.com/nakatamaho/vaeg/issues/59),
  both related to proposal PR #55, not merged as part of M131.

Controlled red/green sequence with the macOS recording-ON, local-encoders-OFF
build:

1. Update the expected V1/V2 x H/S x TMODE raster, text-copy and CPU-bank
   behavior and V1/V2 x H/S x V3/88-mode IN31 readback. The old production
   code fails with `TSP 3301 emulation failed: VA display source depends on
   V1/V2, H/S or TMODE` (exit 1).
2. Correct only the RAM/display policy. The TSP test passes; the suite next
   fails with `V1/V2 memory switch failed: H/S changed VA IN 31h or its
   memory switch` (exit 1).
3. Remove IN31 masking. The entire selftest passes (exit 0), including N-ROM
   extension, V3 saved-S behavior and existing GVRAM/state checks.

Build command: `cmake --build "$BUILD" -j 8`. Test command:
`SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy "$WORKER" --selftest`.
An earlier invocation without the dummy SDL drivers exits 1 in the viewport
setup before reaching these tests; it is retained as a harness preflight
failure, not counted as the controlled red test.

The old `NEW ON 1` instructions depended on the now-removed compatibility
behavior and are marked historical, not silently advertised as still working.
Direct N-ROM boot remains an independent extension. The canonical mode/text
policy documents are updated without claiming a measured H/S clock.
The pre-commit candidate also passes CTest (exit 0; 140 listed, zero failed,
the same nine skips; 87.06 seconds) and encoding/EOL/case/Markdown-comment and
diff-whitespace checks. Edited C lines match clang-format 22. The full canonical
formatter entry point is still blocked by `manifest paths are not sorted`;
its manifest, checker and configuration are unchanged from bf215b99.
Final evaluated-worker identity and runtime verification follow separately.

## Evaluated hardware-corrected worker and runtime verification

Evaluated source/fixing commit:
[de47a4c0af20f457c7bc114951e44ce6222e3f68](https://github.com/nakatamaho/vaeg/commit/de47a4c0af20f457c7bc114951e44ce6222e3f68).
Worker SHA-256:
`4c3e56a93e5321a965afea5f1af88072314020ea89322233bf1bd15a93c479ee`.
The exact binary is preserved privately along with the earlier worker, build
logs, commands, case results, raw dumps, output digests and contact sheets.

Identity caveat: this incremental test build retains the earlier configure-
time version header stamped e0be89d2, although its compiled sources are the
corrected de47a4c0 tree. The evaluated-source identity is established by the
clean source commit/build record and the worker digest, not by that stale
version label. This is **not** a clean-build/release handoff or an executable
to redistribute. A future handoff must configure a fresh tree and verify the
embedded commit as required by the clean-build helper. No attempt is made to
relabel the old worker or hide its configure-time identity.

Commands/status:

```sh
cmake --build "$BUILD" -j 8
ctest --test-dir "$BUILD" --output-on-failure -j 4
/opt/local/bin/python3.12 "$PRIVATE/run-hardware-fix-matrix.py"
```

All exit 0. The evaluated worker's CTest run lists 140 tests, zero failures,
the same nine skips and 93.08 seconds. The runtime driver performs 24 fresh
isolated subprocesses: eight games, eight BASIC queries, and eight common-
BASIC hardware probes. Every emulator subprocess exits 0. It checks BASIC
PRINT/IN31 and probe IN31/CPU-bank/restore/PRINT content; all 16 content checks
pass. A separate content-only comparison verifies both phase banners/labels
against the inspected A-pattern reference in all eight probe cases (exit 0).
All game, BASIC-query and phase/return contact sheets are also inspected
visually. The worker digest remains unchanged throughout.

Corpus/contract: `guest-start-01` in both drives (second concatenated image
in drive 2), `basic-v1-01` (BASIC 1.9), `basic-v2-01` (BASIC 2.4), and
`basic-common-01` with the unchanged source-only build-05 probe. VA and VA2 each
run V1H/V1S/V2H/V2S with explicit configuration and isolated backup/media
copies. Game screenshots are at frames 1790/3600, BASIC at 1100/3000 and
probe phase/return checks at 5500/6500/9000. The driver waits for the BASIC
file-count question and presses Enter before PRINT, rather than consuming
PRINT as an answer to the question. Private metadata and full output digests
remain outside Git.

Both models now produce the following probe results, agreeing with every
photographed hardware field at the tested points:

| Mode | IN31 | Original32 | Phase32 P0/P1 | CPU P0/P1 | Display P0/P1 | Restore / emulator PRINT |
|---|---|---|---|---|---|---|
| V1S | F9h | 8Ah | 8Ah/9Ah | 41h/42h | A/A | 1 / 2 |
| V1H | F9h | 8Ah | 8Ah/9Ah | 41h/42h | A/A | 1 / 2 |
| V2S | 79h | AAh | AAh/BAh | 41h/42h | A/A | 1 / 2 |
| V2H | 79h | AAh | AAh/BAh | 41h/42h | A/A | 1 / 2 |

Every BASIC case prints `M131 2` and F9h (V1) or 79h (V2), with readable
output. All eight game cases reach coherent gameplay after Enter without
the spurious overlay. In particular, the corrected V1S cases no longer
show the earlier `V1-H OR V2 MODE ONLY` rejection. That earlier observation
used the former bit-6-masked IN31 and V1S RAM policy; it must not be generalized
to the photographed VA mode.
This does not claim the game has been tested on hardware in every mode.
Hardware PRINT remains unphotographed; emulator PRINT does not substitute
for it. No clock calibration or IN6Eh validation is claimed.

Only evidence/documentation changes follow the evaluated source commit.
Application inputs and worker digest are unchanged, so the completed runs
are preserved rather than repeated for the final evidence commit. Encoding,
EOL, case, Markdown-comment and diff checks pass. Canonical edited-line
formatting passes; the pre-existing full-manifest ordering failure is unchanged.
No hosted CI was requested, no private capture/media was committed, and no
new Windows executable or full G131 media/demo gate is claimed.

## Remaining gate work at 820094be (historical)

- Produce a current clean static Windows handoff before requesting the full
  gate; the earlier e0be89d2 preview does not contain this correction.
- Generate both SASI utility images and boot/DIR each alone on its model.
  The three required generated CP/M inputs are not currently present in task
  scratch; SASI generation is NOT_RUN, not a passing or failing image build.
- Launch every all-demos program under both OSes and verify drawing; run
  `MANDEL.BAS` from N88-BASIC and exercise the self-booting SGP demo.
- Complete the standard V3/demo/OS human checklist and the targeted fixture
  check only after all mandatory machine/media checks are complete.

M131 remains open. The targeted hardware observations above are not full human
acceptance, Windows runtime verification, approval for the next milestone or
cleanup approval.

## G131 preparation after accepted M130 integration

The maintainer requested G131 and a subsequent merge. This is a request to
perform the human gate, not a statement that it passed. The previously pending
machine/media work above is now completed as follows; human acceptance remains
pending.

### Evaluated source and workers

Accepted M130 main baseline `4c2354cf7fde394e02f633f38656d95975d7549b`
is integrated without conflicts at
[3326349a](https://github.com/nakatamaho/vaeg/commit/3326349ae4b9f6ac352f5aabc07891b0009a407a).
All new application and media evaluations below use that pushed commit.
The macOS recording-ON target was reconfigured and rebuilt; its generated
version identifies 3326349a, unlike the historical incremental worker above.
Its SHA-256 is
`db9ca01b5e71b137c566bd40d8effddddc6a14ecf88705903db452d8cf4f9e7f`.

```sh
cmake --build "$BUILD" -j 8
ctest --test-dir "$BUILD" --output-on-failure -j 4
python3 "$PRIVATE/g131/run-matrix.py"
```

All exit 0. CTest lists 140 tests, zero failures and the same nine skips,
87.59 seconds. The unchanged compatibility corpus/contract described above
runs in 24 isolated processes: all eight game cases show coherent gameplay,
all eight BASIC queries pass, and all eight probe CPU-bank/IN31/restore/PRINT
checks pass. All 16 BASIC/probe content checks pass. A separate content-only
comparison verifies the entire rendered phase-0 and phase-1 images against
the inspected A/A references in all eight cases (16 images, exit 0).
The game/BASIC contact sheets are inspected, not inferred from process exit.
Private commands, configurations, captures, reference/output digests and
inspection records are retained outside Git; the worker digest is unchanged.

### Clean static Windows handoff

```sh
vaeg-container tools/release/build-mingw-static-container.sh \
  --commit 3326349ae4b9f6ac352f5aabc07891b0009a407a
```

Exit 0, fresh clone and application objects, pinned Ubuntu 24.04 container,
`mingw-cross` Release. Embedded commit/date and system-DLL-only PE imports
are verified by the helper. Recording is OFF in this handoff; local bundled
H.264/AAC encoders are explicitly OFF. This is not a recording acceptance test.

- Executable: `../vaeg-mingw-3326349a/output/vaeg.exe`, 57,611,737 bytes.
- SHA-256: `1c371b2e242ecc7e9dc8689c6565cabaefa29367e56774eca599115894389f1a`.
- Report: `../vaeg-mingw-3326349a/output/BUILD-REPORT.md`.
- The helper compiles/links and audits imports; it does **not** execute Windows
  guests. The following runtime results are from the identified macOS worker.
- Docker uses the project-local daemon through process-local `DOCKER_HOST`;
  the global context remains `default`. No context is activated or overwritten.

### Clean-clone media regeneration and payload closure

The source clone is detached at 3326349a with tracked sources unchanged.
Inputs use neutral bindings `$DEMO_TEMPLATE` (PC-Engine 1.1 demo template),
`$SYSTEM_VA` (PC-Engine 1.05 boot template), `$SYSTEM_VA2` (PC-Engine 1.1
boot template), `$ARTWORK` and the pinned public MS-DOS release cache.
Private originals/reference trees are unchanged; only scratch copies are
made writable when required. Generated images are not staged or published.

Final commands/stages all exit 0:

```sh
demos/glass-orbit/build-d88.sh "$DEMO_TEMPLATE" "$OUT/g131-glass-orbit.d88"
demos/neon3/build-d88.sh "$DEMO_TEMPLATE" "$OUT/g131-neon3-distribution.d88"
demos/neon4/build-d88.sh "$DEMO_TEMPLATE" "$OUT/g131-neon4-distribution.d88"
demos/sgp-pseudo-sprite/build-d88.sh "$DEMO_TEMPLATE" \
  "$OUT/g131-sgp-pseudo-sprite.d88"
demos/sgp-wireframe/build-d88.sh "$DEMO_TEMPLATE" "$OUT/g131-sgp-wireframe.d88"
demos/zundamon-orbit/build-from-psd.sh --python "$PSD_PYTHON" \
  "$ARTWORK" "$DEMO_TEMPLATE" "$OUT/g131-zundamon-orbit.d88"
python3 tools/pc88va/build-all-demos-distribution-disk.py \
  --source "$DEMO_TEMPLATE" --distribution-dir "$DIST" \
  --output "$OUT/all-demos.d88" --compressed-output "$DIST/all-demos.d88.xz"
python3 tools/pc88va/build-all-demos-msdos4-disk.py \
  --source "$MSDOS4" --distribution-dir "$DIST" \
  --output "$OUT/all-demos-msdos4.d88" \
  --compressed-output "$DIST/all-demos-msdos4.d88.xz"
python3 tools/pc88va/build-all-demos-bootable-disk.py \
  --source "$SYSTEM_VA2" --distribution-dir "$DIST" \
  --output "$OUT/all-demos-pce.d88"
python3 demos/sgp-boot/build-boot-d88.py --output "$OUT/sgp-boot.d88" \
  --compressed-output "$DIST/sgp-boot.d88.xz"
demos/sgp-scan/65536/build.sh "$OUT/SGP65536.COM"
demos/sgp-contention/build.sh "$OUT/sgp-contention"
demos/va-voyage/build.sh "$OUT/va-voyage"
bash demos/va-voyage/build-opna.sh "$OUT/va-voyage-opna"
tools/pc88va/build-utility-disk.sh --source "$SYSTEM_VA2" \
  --output "$OUT/utility.d88"
tools/pc88va/build-sasi-utility-disks.sh --source-va "$SYSTEM_VA" \
  --source-va2 "$SYSTEM_VA2" --payload-d88 "$OUT/utility.d88" \
  --output-dir "$OUT/sasi"
python3 "$PRIVATE/g131/compare-media.py"
```

The private driver collects the six compressed component outputs under the
canonical distribution names in `$DIST` before building aggregates. Demo
assembly/PSD decoding uses Ubuntu Python 3.12.3, NASM 2.16.01,
psd-tools 1.26.0, Pillow 13.0.0 and NumPy 2.5.4, without ImageMagick.
Utility/SASI commands complete through the host GNU-dd/DOSBox-X bridge and
the project-scoped Docker connection, reusing the checksum-checked shared
caches. The SASI builder regenerates its CP/M inputs from cached sources;
the earlier missing-generated-input blocker is resolved without private
reference-tree writes.

All nine fresh compressed images round-trip exactly to their generated raw
D88s. The eight FAT file maps compare byte-for-byte with the committed
canonical distributions (62 file comparisons); self-booting SGP compares
as an exact raw image. All assertions pass. Timestamp/header-only differences
in other raw containers are not payload changes; no distribution replacement
is required. Initial input-validation/harness setup failures are preserved
separately, not counted as successful builds: write-protection/layout
preflights, the OPNA script's explicit bash invocation, and a container lacking
the Docker CLI. The final commands above succeed.

Produced raw-image SHA-256 values:

| Generated image / neutral role | SHA-256 |
|---|---|
| all-demos-msdos4.d88 | `e88c0e0332094048bcf55b334cd0430fc1c77e158344b00e57946ad9bbc84a78` |
| all-demos-pce.d88 | `743e9a441cf6770bf10158e5d8717ae88d46e5de67e82d610a49a329879ecc7c` |
| all-demos.d88 | `897b9219458f3d3255cadda3f3ea1c525f9ced732749bc4d5a0e026d78d0a716` |
| glass-orbit.d88 | `43dbc5bd18e2068321b14a59d94c416e3e4caa633de39b672b92c26c461276f9` |
| neon3-distribution.d88 | `3f1043b72d84796efbe8ddf446f85069684ce20b30e95a5e2f496303edb5768e` |
| neon4-distribution.d88 | `ac6a9bfce29a937ebb05ac863dc948f7ff606384797f8bec3e93a827b2c4664f` |
| sgp-pseudo-sprite.d88 | `f007b689bdc03ae6e0cd999ae17570341f06060e8fd175778423c6bb6531c978` |
| sgp-wireframe.d88 | `103bcb066f3c4f53c4f42f0a326d44731ea24020f8032817b0f0139cb8224e19` |
| zundamon-orbit.d88 | `299104233327faf333df9030825f42d2c240450d5812867b5158147e1eaff300` |
| sgp-boot.d88 | `67ead94dba0cee7aa9d9a0268cf1987bf3a2764fe1275e092b2e5ece06337154` |
| utility-fdd-01 | `8e8b2d9f48d0e927cc7392ab62e069ca0ebe2fa1046034884e9f5e9ec8615bd7` |
| utility-sasi-va-01 | `e53875d430d3a8e99c1204cc966fdaccffce04f3e397386549c3543dde2c76c7` |
| utility-sasi-va2-01 | `56fe61cb73384b74ffdb6da1c6a5f0097e5e5797545bebbd1eb5ab7f625ac638` |

Produced compressed-image SHA-256 values:

| Generated distribution | SHA-256 |
|---|---|
| all-demos-msdos4.d88.xz | `67ddb64012bc89464cdaa2809c62a2a9c2460c52ecec7f815dc45a6193eef241` |
| all-demos.d88.xz | `45715a444e7110a93f9f2b82872044de4854dbeb2d5c6eb6384281aaa053a420` |
| glass-orbit.d88.xz | `24e93e4fc848ad288a4632ee89631221c24c2e3e1735978fb517b6f2b1069e59` |
| neon3-distribution.d88.xz | `c4da5afb9c1bef645750e726767403b048c29f06f1115bccde38543d1d9eea06` |
| neon4-distribution.d88.xz | `2432b7bdeab3042cdfc97dbb46e461aa6b1927ea7550a5ce1d7c6414042f0767` |
| sgp-boot.d88.xz | `657f91e25d9417b0b4fd9948d40caeec7f2daa6db6d2218b6f709be62b5238c5` |
| sgp-pseudo-sprite.d88.xz | `44ea5701f85e4fbfe840da868482d19cadb27be91f0f966bbe71a3a439afd180` |
| sgp-wireframe.d88.xz | `a1696e6dc41a1df2151d18a50c2f24df53691773e855821b1c6579730975d6d1` |
| zundamon-orbit.d88.xz | `fdeaf8401ac8272d27c1c117b49e3401570780ed65293c06d1b9a9ad18c3a218` |

### Runtime drawing and boot inspection

```sh
python3 "$PRIVATE/g131/run-demos.py"
python3 "$PRIVATE/g131/run-extras.py"
python3 "$PRIVATE/g131/run-followups.py"
python3 "$PRIVATE/g131/run-pce-invocations.py"
```

All drivers exit 0 and retain each subprocess's exact command/configuration,
status and output digests. Cases use isolated media/backup/configuration,
VA2, 640 KiB, CPU multiplier 2, no-wait and mute, except the original-VA SASI
case. Process exit alone is not a drawing assertion: captures are inspected.

- All 17 all-demos executables draw under both PC-Engine and MS-DOS 4.0,
  34 cases total: GLASS, NEON200/400, both NEON4 variants, eight pseudo-sprite
  variants, three wireframe variants and ZUNDAORB. The first 32 are inspected
  at frame 4200; ZUNDAORB follow-ups extend to frame 16000.
- Under PC-Engine, after `CD \ZUNDAMON`, use **`ZUNDAORB` without `.COM`**.
  The suffix and suffix-qualified full-path trials return to Ready without
  drawing; they are retained as unsuccessful invocations, not passing cases.
  The bare-name trial draws at frames 8000/16000. No shell/parser root cause
  or emulator correction is inferred from this invocation difference.
  MS-DOS 4.0 uses `ZUNDAORB.COM`; the original frame-4200 INIT-only capture
  is superseded by the inspected extended-run drawing, not relabeled a pass.
- Utility FDD boots on VA2; each SASI image boots **alone** on its intended
  model (no FDD/second HDD). All three accept DIR and display directory
  listings followed by Ready at frame 4200. This verifies the required
  boot/simple operation, not every installed utility.
- Self-booting SGP reaches its menu and selecting A displays the sprite demo
  at frame 4200, without an OS disk.
- N88-日本語BASIC V3.1 starts with `BASIC /87`, loads `MANDEL.BAS` and runs it.
  The plotted colour band visibly progresses from frame 4000 to 12000;
  completion of the entire 640x400 fractal is not claimed. The initial README
  invocation `LOAD "MANDEL.BAS",A` reports Syntax error; the controlled
  follow-up `LOAD "MANDEL.BAS"` draws. README examples and the 8.3 filename
  are corrected in [eb7ea0a3](https://github.com/nakatamaho/vaeg/commit/eb7ea0a38bf9149e90990ea9f7b2c63102731164).
  BASIC/emulator sources and the demo payload are unchanged.

### Repository checks, reuse and human handoff

Encoding, EOL, case, Markdown-comment and diff checks exit 0. The two adjacent
formatter-manifest paths are reordered at
[a85469b2](https://github.com/nakatamaho/vaeg/commit/a85469b2698419bb5495f2611f2aab82a6781125),
with all 387 members unchanged; manifest and pinned formatter validation
pass. Full `tools/repo/clang_format.py` now reaches the source check and exits
1 on baseline drift in 26 files. This is **not** reported as a full format
pass. The 89 added/changed C/C++ lines relative to accepted main pass the
pinned formatter; unrelated lines are not reformatted. Full diagnostics are
preserved privately for maintainer review.

Only the enumerated formatter manifest and Markdown documentation change
after evaluated commit 3326349a. Application/build-relevant inputs, demo
payload sources and worker digest remain unchanged. Completed expensive
builds and guest/media runs are therefore reused for the evidence-only
handoff instead of rerunning identical behavior. No hosted CI success is
claimed; no private original/capture/media or application binary is committed.
The archived reference tier is unaffected.

Human G131 still requires the supplied static Windows executable: standard
V3 boot, bundled VA demo, OS boot/DIR/program launch, and Enter at Start in
`guest-start-01` with its two images mounted in FDD1/FDD2. Confirm the formerly
corrupt subsequent screens; exercising all four requested VA/VA2 modes is
recommended. Existing hardware photos do not replace this Windows/manual
acceptance. Clock timing and IN6Eh remain #58/#59, not newly verified.

**No G131 pass, main merge, next-milestone authorization or cleanup is recorded.
Wait for explicit maintainer acceptance before the gate record and `--no-ff`
merge.**
