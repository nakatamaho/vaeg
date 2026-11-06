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
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.
-->
# M144a — X88000 compatibility-feature source review

Status: **IMPLEMENTATION_IN_PROGRESS; G144a human gate PENDING;
publication remains BLOCKED_AUTHENTICATION_NOT_PUSHED.**
Branch: `milestone/m144a-x88000-review`. Evaluated vaeg main:
[dc023c9b7ce62c7334d797ba1934ad95b51fd3fd](https://github.com/nakatamaho/vaeg/commit/dc023c9b7ce62c7334d797ba1934ad95b51fd3fd).
This is a read-only compatibility follow-up to accepted M144, separate from
paused WSR-E and M145 work.

## Authorized implementation extension

After receiving the source review, the maintainer explicitly requested implementation
of the 32h GVAM transition and E6h unmask-request changes, PC-8001mkII/SR
emulation, a main-clock-independent nominal 4 MHz FDD CPU, and PC-PR201 output
to PDF (including operation with an 8 MHz main CPU). This extends this same
milestone; it does not declare the previous review accepted or start another gate.
The earlier investigation-only restrictions below describe the completed review
checkpoint, not this newly authorized implementation phase.

Keep separate commits for independently testable concerns. PC-8001 graphics,
ROM banks, mode selection and state persistence must be implemented together
rather than merely adding model names. PR201 requires a guest printer input
path, bounded command/page processing and actual PDF content validation, not
renaming a raw print stream. No ROM/font payload may be bundled. Keep VA
hardware behavior distinct from independent PC-8001/8801 selections.

The FDD clock report is a requested verification/correction, not yet a diagnosed
vaeg defect: current `Subsystem::Reset` scales FDC cycles by the fixed machine
multiple, independently of the configurable main CPU multiple. Verify guest
execution at 4/8 MHz before deciding whether a production correction is needed.
Nominal 4 MHz in the VA clock domain means 3.9936 MHz, not changing the crystal
base to 4,000,000 Hz without evidence.

Required implementation checks: focused synthetic I/O/memory/interrupt tests,
FDC execution-count comparison, model/reset/state regression, printer parser
bounds and PDF page/content/error cases, applicable CMake/CTest builds and
repository checks. Preserve failed tests. A clean static Windows handoff and
human acceptance are still required before the implementation gate passes.
Full media/presenter campaigns require a separate explicit request. No merge,
release, hardware probe, deletion or resumption of WSR-E/M145 is authorized.

## PC-8001mkII/SR implementation checkpoint

The maintainer subsequently prioritized PC-8001mkII/SR graphics, ROM banks,
mode switching and state persistence. PR201/PDF and FDC clock verification
are paused while this work proceeds; neither is silently dropped or completed.

Added `io/pc8001_video.c/.h` with independently testable N80 address and raw
pixel primitives, and `tests/pc8001/video.c` / CTest `vaeg_pc8001_video`.
The source is currently linked **only into that test target**, not the emulator.
This intentional integration boundary prevents partially implemented N80SR
selection from replacing the accepted N-mode startup.

Implemented primitive contracts:

- ROM/RAM selection below 8000h; N80 fourth-ROM socket versus BASIC upper
  bank from 31h bit 0; N80SR upper BASIC versus IE ROM from 71h bit 0.
  Missing ROM handling and qualified model/mode selection remain caller-owned.
- GVRAM window at 8000h-BFFFh, independent-plane versus extended-access enable.
- mkII 640x200 one-bit and 320x200 packed two-bit pixels (doubled to 640 dots).
- SR 640x200 B/R/G pixels and 320x200 two-page B/R/G composition, transparent
  front colour zero, selectable front page and each page's 53h disable bit.
- Bounds rejection without reading input for invalid coordinates/null buffers.

Verification: 2,879,495 assertions PASS in the normal target and independently
compiled C99 `-Wall -Wextra -Werror -fsanitize=address,undefined` target. The
nonuniform coordinate fixtures check stride, bit order, page offset and doubling;
solid-colour cases cover all colour/priority/page-disable combinations. Exhaustive
ROM-address and mode-bit cases include every 16-bit address. Seven task-local
single-mutation checks start from the passing baseline and reject ROM edge,
IE polarity, GVRAM edge, packed colour, priority, page-offset and stride mutations
with `PC8001_VIDEO_CONTRACT`. Raw logs/copies remain outside Git.

**Not yet integrated:** production ROM loading (including 4TH/IE assets), raw
31h/33h/35h/53h/71h state, I/O-trap interaction, main CPU memory read/write
and write protection, ALU backing, palette/monochrome/text priority, display timing,
font selection, runtime mode detection/GUI and save-state serialization. In
particular these primitive tests are not a machine boot, screenshot, or SAV
round trip. The requested four-part feature remains IN_PROGRESS, not complete.

Next integration must qualify machine type from applied ROMs, preserve the old
N-mode fallback, avoid using VA aliases to represent SR pages, and validate
state decode before modifying live state. ROM payloads must remain private
inputs, never new tracked assets. A state codec's unit round trip alone cannot
substitute for save/load through `machine/statsave.c`.

## Production N80 ROM/banking checkpoint

The maintainer explicitly approved accepting `e8.rom` after clarification that
this name and the fourth-ROM mapping were verified in X88000 1.5.3 source,
not independently qualified as a MAME filename or physical-machine result.
It is an **optional X88000-compatible 8 KiB input**, not a required machine ROM.
Its role is appended to preserve existing role IDs. No ROM data is distributed.

Production changes (following the earlier primitive-only checkpoint):

- `extrarom_apply` now records applied mkII versus SR identity, loads the
  optional fourth ROM and SR's complete 40 KiB BASIC/IE image into separate
  read-only host buffers, and initializes absent optional ROMs to FFh.
- Native I/O-dispatch handlers retain raw N80 31h/33h and the 71h bank bit;
  the memory read path selects N80, fourth ROM, SR BASIC or SR IE, without
  copying/overwriting ROM buffers during a bank transition. Non-N80 models
  retain their existing selection path.
- mkII/SR independent GVRAM access is now decoded at 8000h-BFFFh rather
  than C000h-FFFFh. SR extended access takes GVAM from 33h bit 6 and reuses
  the tested ALU/latch backend and existing persisted plane storage. Its
  lower aperture does not suppress the upper VA-bridge text window. VA
  32h GVAM edges no longer reset the independent N80 plane selection.
- `N80BANK` version 1 adds a four-byte, endian-independent state section
  (model, 31h, 33h, 71h). Preflight rejects invalid models, a different applied
  model, invalid 71h state and SR-only state on a non-SR model with
  `M144A_N80_BANK_STATE`, before resetting the live machine. Older files without
  the section restore the original unbanked N80 upper BASIC selection.
  Existing GVRAM/ALU/plane sections remain the backing storage.

Synthetic production-path tests create their own ROM fixtures for both models,
exercise real `iocore_out8` and byte/word memory access, save actual SAV files,
change live banks, load, and check restored bank state and mapped bytes. They
also test the legacy missing-section path and three single-mutation negatives
per model, requiring the exact error code and unchanged live bank/CPU state.
GVRAM tests cover all three planes, C000h RAM isolation, BFFFh/C000h word
crossings and SR ALU write/latch copy. The ordinary ROM-less suite and the
primitive target pass (2/2 CTest tests; full ROM-less suite about 43 seconds).
A further eight focused state tests pass: runtime-state contract, restore-matrix
contract, cross-OS contract, uPD9002 boundary, payload probe, M66a verification,
compatibility state regression and actual state-storage tests. The contract
selftests are not cross-OS executable or guest-display coverage.

Preserved intermediate failures: the first selftest build used an incorrect
memory-read symbol and failed compilation; corrected to the real VA bus API.
An anchored CTest selector matched no test and was **NOT_RUN**, not PASS;
subsequent runs use the discovered `vaeg_romless_tests` name/pattern.

**Still unfinished:** guest-instruction I/O-trap routing (the checks above call
production port dispatch directly), palette and raster/compositor connection,
SR text/page priority and fonts, reset entry into SR BASIC, and precise GUI mode
labels. ROM writes/write protection and extra RAM interactions still require
qualification. No private-media boot, final-client image comparison, Windows
handoff or full G144a acceptance is claimed. In particular, a successful banking
SAV round trip does not qualify the still-unimplemented SR display state.

## Guest instruction I/O checkpoint

The next M144a checkpoint implements a closed direct-I/O route in the existing
main CPU adapter. It requires the independent main uPD780C, an applied mkII/SR
model and 88 compatibility mode. Writes to 31h, 71h and 5Ch-5Fh are direct;
SR additionally owns writes to 33h-35h. Reads are direct only for 5Ch, 71h,
and SR's 33h. All other ports retain the VA firmware trap policy, including
CRTC/text/palette control. Direct bus dispatch uses the low eight address bits,
not the A/B high address byte of a Z80 I/O instruction. No trap registers are
rewritten and no global trap-disable option is introduced.

`tests/upd9002/main_cpu_roles.c` adds 360 actual instruction cases with an
intentionally overlapping enabled VA trap: immediate IN/OUT, ED IN A,(C)/
OUT (C),A and INI/OUTI; ten ports including an unimplemented 50h control;
VA, PC-8801, original PC-8001, mkII, SR and compatibility-disabled controls.
The direct cases check register/memory input results or real bank/ALU/plane
side effects, compatible PC advancement and unchanged native stack. Trapped
controls check the native handler entry/stack and unchanged controller state.
This matrix does not cover every ED register form, ignored prefixes, repeated
block instructions or interrupt-boundary interactions.

The matrix is invoked by **`vaeg_upd9002_brkem_upd70008`**, not by the ordinary
ROM-less test. Both have been run; the CPU log explicitly reports all 360 cases.
A single mutation restoring the former unconditional trap branch fails at
`M144A_N80_GUEST_IO` (mkII, immediate OUT 31h). Restoring the implementation
passes again. A first build missing the I/O-trap declaration header failed;
that log is retained separately from successful builds.

This supersedes the earlier NOT_RUN status only for the instruction forms and
ports above. It is not a BASIC boot, full graphics result or private-media test.
The next dependencies are raster/palette/text-priority integration and
state-transition contracts.

## N80 colour raster and composition checkpoint

Connected the mkII/SR **colour** path to production `makegrphva_raster` and
`scrndrawva_compose_raster`, rather than merely exposing the earlier pixel
helpers. The SR pixel helper now accepts a plane stride so CPU access and
raster reads use the same persisted VA-bridge GVRAM storage, without a stale
frame copy. The non-N80 compositor branch remains unchanged.

Implemented in this checkpoint:

- mkII packed 320-dot four-colour and 640-dot one-bit colour-index output;
  SR planar 640-dot eight-colour and 320-dot two-page colour output;
- direct guest output to 52h (background), 53h (visibility), 54h-5Bh (digital
  palette); the closed guest-I/O matrix now contains 720 cases;
- mkII automatic 31h palette selection, without replacing a custom palette
  on identical mode writes;
- SR 33h bit 2 front-page selection with zero-index transparency and 53h
  page disable; bit 3 places text behind nonzero graphics in SR colour mode;
- existing text raster/colour indices participate in composition; 53h bit 0
  disables text; no fictitious N80 sprite layer is inserted;
- existing layer toggles and video-output enable are respected;
- `N80VIDEO` version 1 stores the two raw video-control bytes; palettes remain
  in the existing `VIDEOVA` state section. Missing `N80VIDEO` starts at zero.

Synthetic tests inspect actual final `vabitmap` pixels after production raster
and compositor calls: automatic palette, text foreground, SR page/text priority,
front-page disable and backdrop. Actual SAV load restores video controls,
modified palette and resulting compositor pixel, in addition to the earlier
bank tests. A passing-fixture single mutation disabling text-behind priority
fails specifically with `M144A_N80_VIDEO_PRIORITY`; the mutation is reverted.
A first build used `sizeof` on an incomplete raster declaration and failed;
that log is preserved, followed by corrected builds using `SURFACE_WIDTH`.

**Coverage boundary:** these are final emulator-buffer checks with synthetic
text/VRAM, not an approved final-client screenshot or a running BASIC/game.
Monochrome N80-specific attribute composition, border timing, alternate SR fonts,
full CRTC/text commands, reset into SR BASIC and a live mid-frame mode switch
remain unqualified. In particular the colour path does not claim completion
of those other modes. Further mode and state-transition contracts remain
separate work. No human gate or main merge is authorized here.

## N80 monochrome, SR glyph bank and BASIC startup checkpoint

Extended the production N80 raster path to monochrome: mkII's one plane,
SR's three 640-dot planes, and SR's six independently masked 320-dot
plane/page bits. Monochrome composition applies graphics before text reverse,
using the existing decorated text raster and post-reverse foreground. A second,
derived foreground raster retains the pre-reverse colour so disabling text with
53h removes reverse/glyphs but retains the graphics attribute colour. Both
80-column raster extraction and 40-column duplication carry that derived colour.
No new text buffer is persisted: the renderer regenerates it from saved text state.

`font80sr.rom` is now loaded into a separate 4 KiB host-input buffer. 33h bit 4
selects the eight-row glyphs at offset 0800h, following X88000's explicit
HIRAGANA glyph-bank access. Normal ANK bytes are not overwritten, and PCG still
has precedence at the renderer call site. Missing SR font input falls back to
ordinary ANK; native/other-machine selections retain the original path. Do not
confuse X88000's optional 512-byte `hirafont.rom` fallback with MAME's separate
2 KiB `fonthira.rom`: the latter's relationship remains unqualified and it is
**not** silently substituted. MAME tag `mame0280`, `src/mame/nec/pc8001.cpp`,
corroborates the 4 KiB font80sr input size but does not establish the selection
logic; the X88000 loader asks for 8 KiB while the glyph bank used here lies
within the first 4 KiB. No source or font payload was imported into Git.

The one-shot VA-bootstrap handoff now selects SR ROM only when the applied
machine is SR and its complete BASIC/IE ROM is loaded. Subsequent trap returns
do not reselect it. Live 33h bit 7 drives the `N80SR` status label; the older
ambiguous N/N80 label remains for the fallback family mode rather than guessing
an internal BASIC variant.

Synthetic coverage adds all 256 normal/hiragana glyph address selections on
both models and a 32-case final-pixel truth table across GVRAM bit, glyph bit,
reverse and hidden text, using the sixth SR plane/page as an aperture witness.
It checks the production raster/compositor with injected attribute rasters;
it is **not** an end-to-end secret/blink/underline/attribute-parser matrix.
Earlier real-file SAV, colour-priority, CPU-role and I/O regressions still run.

## N80 runtime continuation and input qualification checkpoint

### Hiragana input relationship

The loader accepts `fonthira` as a fallback for the second 2 KiB glyph
bank when the complete font80sr input is absent. The complete font has
precedence, and ordinary ANK is never replaced by hiragana. Required-role
checks accept `font80sr|fonthira`. These are supported-interface rules,
not a statement about a particular input or an execution result.
Synthetic fixtures deliberately give the two inputs different bytes to check
precedence, then remove only the generated complete-font fixture to exercise
the fallback and return to normal ANK.

### Guest mode continuation

A synthetic program now executes from guest C000h RAM, performs actual OUT 33h
instructions selecting SR -> N80 -> SR, reads address 0000h after each switch,
and stores the three distinct ROM witnesses back to guest RAM. It must remain
in compatible execution and reach the expected instruction boundary. This is
a live guest CPU/ROM mapping test, not just port dispatch. It does not imply
that switching ROM underneath a currently executing BASIC interpreter is safe;
software must use an appropriate RAM transition routine.

### State comparison contract

`tools/qa/n80_state_contract.py` provides content validation for required
state sections, dynamic progress and restored state/frame equality. PNG
structure, bounds and CRCs are checked. EVENT and BEEP are excluded by
this contract, so it must not be presented as all-state or audio equivalence.
Equal outputs alone do not establish equal execution intervals. Its
selftest uses passing fixtures and single-mutation exact-code negatives.

## Initial port implementation checkpoint

Implemented compatible-mode GVAM edge reset and E6h newly permitted request
clearing. Native GVAM latch behavior is unchanged. E6h does not revoke a vector
already offered to the CPU: that existing acknowledge transaction remains
owned by the CPU bridge. This is an explicit VA-adapter boundary, not a claim
of byte-for-byte transplantation of X88000's interrupt model.

Added 32 GVAM transition/mode/plane cases and 256 E6h mask/pending cases to
production-path ROM-less selftests. Updated the state fixture to select its
nondefault plane after the GVAM edge. No state format changes.

Local QA: fresh native Debug configure/build exit 0 (recording ON, static
FFmpeg, bundled MP4 encoders OFF; QA-only profile with shaders OFF).
`ctest --test-dir <task-native-build> -R vaeg_romless --output-on-failure`:
1/1 PASS, 41.66 seconds. Log contains `8214 mode ok` and
`GVAM transitions 32 cases ok`. Initial configure rejected missing trace
instrumentation; corrected with `VAEG_Z80_COMPAT_INTEGRATION_TRACE=ON`, with
both logs preserved. No mutation check, full CTest, Windows build, new guest
media execution or hardware verification has yet been performed.

**Remaining:** full PC-8001mkII/SR graphics/ROM/bank/state integration,
production FDC execution-count verification, PC-PR201 command/parser/port
integration and PDF export. None of these is marked implemented by this
port-only checkpoint. Main/WSR-E/M145 remain unchanged.

## Request and scope

The maintainer requests source investigation, **not fixes**, of eight X88000
features and accumulation of findings under the existing `docs/modernization/`
directory (the requested `modanization` spelling is not a new directory):

1. 1.5.1: port 32h GVAM transitions reset the independent access selection.
2. 1.5.1: port E6h enabling a mask clears an interrupt request.
3. 1.5.1: monochrome text reverse colours.
4. 1.5.1: CRTC START DISPLAY DM reverse bit.
5. 1.5.3: variable text attribute count.
6. 1.4.0: HAL Laboratory PCG-8800.
7. 1.4.0: overseas mode (not English host UI).
8. 1.3.2: wait emulation (not host pacing or display frame skipping).

Use pinned public reference sources and identify unavailable historical
releases. Distinguish changelog statements, static source findings, synthetic
checks, prior accepted tests and new runtime evidence. X88000 models PC-8801,
not physical VA compatibility logic; source agreement is not hardware proof.
Do not connect any difference to the deferred demo transition artifact without
reproduction and causal evidence.

## Deliverable and restrictions

An independently written Japanese report, with an eight-item comparison table,
exact functions/source references, boundaries and follow-up tests. Reference
archives/source views, command failures, hashes and working notes stay in
out-of-tree task scratch; no ROMs, fonts, private media or captures are needed.
No emulator/source/configuration change, foreign emulator execution, hardware
I/O, new build, media campaign, merge, release or cleanup is authorized here.
Existing PCG acceptance remains historical, not rerun evidence.

## Checks and review

Run focused document/link/source-scope checks, encoding, EOL, case, Markdown
comments, milestone-ID selftest/audit and Git whitespace checks. Any synthetic
source experiment must identify its exact inputs and must not be presented as
production-path or hardware execution. No emulator rebuild is warranted by a
report-only change. Human review of the findings is pending; no new gate PASS
is inferred from publication. Do not resume either paused hardware task.
G144a for this report-only task is maintainer review of the source findings,
not a new emulator-build/runtime/hardware acceptance gate.

## Recorded findings

[Japanese investigation report](../../modernization/x88000-compatibility-review.md).
The eight items now have source functions, classifications, evidence boundaries
and proposed follow-up checks:

- X01/X02: concrete source-level differences in GVAM transition selection and
  E6h unmask request clearing. Physical VA expectations and guest impact remain
  unverified; these are not newly diagnosed or fixed guest defects.
- X03/X04/X05: corresponding monochrome composition, global reverse and
  variable-attribute rendering already exist. End-to-end ROM translation,
  nonstandard counts and transitional display coverage are not newly passed.
- X06: PCG is already accepted in M115/M115a, with intentional differences from
  X88000. Current main's default-ON policy is retained.
- X07: international mode means optional 8x8 font selection/fallback and PCG
  interaction, not English host UI. There is no equivalent separate feature.
- X08: X88000 has M1/memory/CRTC contention waits. vaeg's measured VA-compatible
  costs and native waits are not that model; independent PC-8801 main uPD780C
  timing must be reviewed separately. No wait constants were transplanted.

## Reference and local evidence record

The official 1.5.3 archive matches ADR-0016's published SHA-256. The 1.5.1
comparison uses public mirror commit
[1606677251c49c5d980953e13b42be5812ea99e7](https://github.com/nakatamaho/x88000/commit/1606677251c49c5d980953e13b42be5812ea99e7).
Constructed older official archive URLs returned HTTP 404; exact historical
1.4.0/1.3.2 implementations were not obtained. Their introduction history and
surviving 1.5.3 behaviour are explicitly distinguished.

Task-local inputs and receipts are retained out of tree: page/archive fetch
records, the public-source checkout/archive, 232 original/UTF-8-view pairs and
hash bindings, the ten-file version comparison, static audit source/output,
repository-check logs and publication notes. No private assets are involved.
The source-view manifest SHA-256 is
`a06cb3b3f01eeebc16ecde5f21944ddfa0c7ce93d0d347696152d8080377dbb7`.
No sources, archives or raw source extracts are committed.

The focused audit checks all 232 original/view hashes, nine unchanged selected
files and the attribute-renderer difference, thirteen function-body source
facts, pinned source/link line bounds and the closed three-document scope.
The monochrome eight-case truth table is **source-derived arithmetic only**;
no production function, emulator or hardware was executed. The final link count
is recorded with the final audit rather than treated as a frozen report schema.

Preserved preliminary failures: the source-link audit rejected an overlong
PCG source-line range; the range was corrected from 242 to the actual 234.
The first repository audit passed 49 milestone-ID selftests but rejected the
new roadmap row because its pending G144a ID was missing; the row was corrected.
Neither is an emulator defect, and the failed logs are not relabeled as PASS.

## Scope and review disposition

Only the new report, this task and the one-row roadmap addition are changed.
No emulator source/configuration, build metadata, dependency, binary, media or
private-input identity changes. No bug-fix ledger entry is added from a static
reference difference without demonstrated incorrect guest-visible behaviour.
Archived reference-tier behaviour/provenance is unaffected. No hosted CI,
media/demo campaign, screen automation, merge, release or cleanup was requested
or performed. WSR-E and M145 remain paused and untouched.

A future main merge still requires explicit maintainer acceptance and the
campaign-choice question required by AGENTS.md. This publication does not
supply either. The existing deferred scene-transition artifact remains
OPEN/DEFERRED and is not attributed to any finding in this report.

## Publication status

The task-scoped HTTPS push preflight exited 128 with
`fatal: unable to get password from user`. No usable authentication is available
within the permitted task boundary. Credential helpers and interactive prompts
were disabled for that attempt; no credentials outside the allowed tree were
read or copied. The source review is retained locally, **not pushed or merged**.
The failed receipt is preserved out of tree. Resume publication only with usable,
permitted authentication; do not claim a GitHub branch/PR was created.

## Local verification

All final checks below exited 0. The task-local static audit reported
`PASS_STATIC_REVIEW_AUDIT_ONLY`: 232 original/view pairs, 13 function bodies,
42 document/source links and 8 monochrome source-derived arithmetic cases.
Pinned source links were checked against local blobs and line ranges; this
is not a new HTTP-availability check for every GitHub link. No emulator build
or runtime test was run for this documentation-only change.

```text
python3.12 -B tools/repo/check_encoding.py --expect utf8
0 violation(s)
python3.12 -B tools/repo/check_eol.py --enforce
0 violation(s)
python3.12 -B tools/repo/check_case.py
0 finding(s)
python3.12 -B tools/repo/check_md_comments.py
0 finding(s)
python3.12 -B tools/qa/milestone_ids.py --root . --selftest --discover --audit
milestone-id-selftest: 49 strict acceptance/rejection checks passed
milestone-id-discovery: tasks=226 reports=178 roadmap_rows=167
milestone-id-audit: ROADMAP is declarative; task/report discovery, gate, branch, commit-prefix, artifact, and CI tooling accept strict canonical lettered identifiers
git diff --check
(no output)
git diff --cached --check
(no output)
```
