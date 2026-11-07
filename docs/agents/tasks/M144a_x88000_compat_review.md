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

Status: **IMPLEMENTATION_IN_PROGRESS; G144a human gate PENDING.**
Branch: `milestone/m144a-x88000-review`. Evaluated vaeg main:
[dc023c9b7ce62c7334d797ba1934ad95b51fd3fd](https://github.com/nakatamaho/vaeg/commit/dc023c9b7ce62c7334d797ba1934ad95b51fd3fd).
This is a read-only compatibility follow-up to accepted M144, separate from
paused WSR-E and M145 work.

## N80SR FM interrupt-mask checkpoint

[FM mask ownership and transition checks](../../modernization/x88000-compatibility-review.md#n80sr-fm-interrupt-mask-ownership)
connect SR port 33h bit 1 to the effective timer-interrupt gate. The old code
saved/read the bit but used only the VA mask to decide timer requests. The
baseline generated-input failure is `M144A_N80_FM_MASK_SELECTION`.

The effective SR gate is derived from the existing N80 latch, not duplicated
in a new state field. Its transitions use the existing FM/PIC mechanism;
latched/offered PIC semantics and clocks are not changed. Native and other
machine selections retain the VA gate. Checks cover 6,144 selections,
A/B timers under both controller modes, native setter controls, guest OUT/IN
with both trap directions enabled, and SR-mask versus VA-mask SAV restoration.
The two reverted mutations report exactly `M144A_N80_FM_MASK_SELECTION` and
`M144A_N80_FM_UNMASK_PENDING`. Native build and nine focused CTests pass,
including the sound-generation and sound-capture checks.

This checkpoint records only public-source and synthetic-device evidence,
not media execution, physical IRQ timing, M88 comparison or G144a acceptance.

## Scripted audio opt-in checkpoint

Added the explicit `--headless-audio` CLI option for software mixer/capture QA
with either input scripts or the debug harness. Scripted execution remains
muted by default, and explicit `--mute` wins in either argument order. Dummy
SDL video/audio remain in use; the option is not a physical-output test and
must not override a disabled sound configuration. Without a script it fails
with `HEADLESS_AUDIO_REQUIRES_SCRIPT`.

ROM-less CLI checks cover 24 combinations of script type, opt-in, explicit mute
and argument order, with a passing seed before rejection cases. A single
mutation overriding explicit mute with the opt-in fails with exactly
`HEADLESS_AUDIO_POLICY`; it is reverted. Native build and the seven focused
CTests pass on the restored sources. This is an automation feature, not an FM
emulation correction or a change to normal interactive audio. This checkpoint
contains no media-execution claims. G144a remains pending.

## Automatic N80 startup-switch input checkpoint

[Port-30 automatic-startup readback](../../modernization/x88000-compatibility-review.md#automatic-n80-startup-switch-readback)
now overrides only bits 0/1 for mkII/SR in compatible mode, using the same applied
model/input predicate as the existing automatic handoff. Previously the VA
backup-memory byte was returned unchanged. Upper bits and the backing byte are
preserved, and native, non-PC-8001 and base-PC-8001 behavior is retained. IN 30h
joins the independent main CPU's direct N80 input path; OUT 30h does not.

Verification covers 1,024 input combinations with three guard controls each,
1,476 instruction/trap/model cases, and guest IN 30h remaining stable while
OUT/IN 33h and ROM reads traverse SR/N80/SR. The earlier readback fixture enabled
only output traps; this checkpoint enables both directions and does not
retroactively promote its earlier result to input-trap coverage. Baseline bit
failure and the controller-only/input-trap failure were reproduced. From the
passing fixture, bit inversion and route removal fail with exactly
`M144A_N80_BOOT_SWITCH_BITS` and `M144A_N80_MODE_READ_ROUTE`; both are reverted.
Native build and seven focused CTests pass. A changed worker digest prevented
reuse after the role-test expectation edit, so the focused tests were rerun.

No configurable N/N80/N80SR selector, key timing, 33h polarity correction,
clock change, state-format change or disk-boot PASS is implied. Explicit startup
selection and its applied-state representation remain work to do. G144a is pending.

## mkII primary-manual mode and WAIT checkpoint

[Five-page image verification](../../modernization/x88000-compatibility-review.md#pc-8001mkii-user-manual-verification)
of the public mkII user's manual now supports the SW8/CTRL four-case startup
matrix. Appendix 3 documents SW1 as unused and SW3–7 as unavailable to users,
not as electrically unused. Its SW2 condition concerns slow **ROM**, correcting
the earlier hearsay's RAM wording. Section 12.2 separately documents the normal
M1 wait and the selectable memory-read wait; their overlap and actual timing
remain unqualified. The disk-startup instructions require a matching BASIC
selection even when an incorrect combination appears to start.

This is documentation-only primary-source verification, not a mode-selector
implementation or a timing correction. The mkIISR user's guide was not located
in the bounded search; no global absence claim is made. No private-media result,
new runtime PASS or G144a acceptance is asserted. Existing source-model evidence
remains reusable because emulator and test sources are unchanged.

## Port 33 readback and primary-manual checkpoint

The guest RAM mode-switch fixture now performs OUT/IN/ROM reads with an active
trap range covering port 33h. Three transitions check current source-model
readback, the bit-0/5 read mask and distinct generated ROM contents. Inverting
only input bit 7 fails with `M144A_N80_MODE_READBACK`; the mutation is reverted.
Native build and seven focused CTests pass. This extends coverage, not hardware
polarity qualification; no emulator behavior or state-format change is made.

[Source comparison](../../modernization/x88000-compatibility-review.md#port-33-source-model-readback-checkpoint)
separates X88000's set-bit SR read/write convention, MAME's disabled code, the
contradictory Web table and the still-unverified 30h startup path.
[Primary-manual intake](../../modernization/x88000-compatibility-review.md#primary-manual-intake)
records image checks of the public N80SR-BASIC reference manual's cover and
preface. These support the three-BASIC distinction, not DIP/key timing or port
polarity. Basic operation is referred to a separate user's guide. No disk-boot
PASS or G144a acceptance follows from this checkpoint.

## BASIC mode selection documentation checkpoint

The maintainer-supplied public-source excerpts are summarized in
[BASIC mode selection review](../../modernization/x88000-compatibility-review.md#basic-mode-selection-review).
This records the N/N80/N80SR machine matrix, reset-time DIP/key selection,
separate BASIC/clock/wait settings, the unresolved 33h bit-7 polarity question,
and the order of UI, mapping, banner and disk-boot verification. Related PC-8801
reset operations are reference material, not an added implementation mandate.
The additional mkII DIP account is explicitly unverified hearsay: SW1 unused,
SW2 memory-read wait, SW3–7 unpublished, SW8 N/N80 selection. It is not sufficient
evidence to change timing or infer the unpublished switches' behavior.

This checkpoint changes documentation only. No linked-page full-text audit,
hardware qualification, private-media result or new boot PASS is asserted.
It does not establish the cause of a disk I/O error. G144a remains pending.

## Guest timer continuation through SAV

The generated RAM program now initializes all three PCG timer channels through
CPU-executed OUT instructions before the existing glyph checkpoint. It leaves
binary reload 10,000 on channel 0, a pending low-byte BCD load on channel 1,
and binary zero on channel 2. The main checkpoint remains at C03Ah.

After saving, guest execution advances to C048h, completing the pending timer
write and enabling the gates. The controller is then reset before SAV load.
The restored CPU mode/IP/A, trap configuration and complete PCG state must match
the checkpoint. Guest instructions—not host reconstruction—then select the completed
glyph, finish BCD 1234 and enable all three gates. Raw reload/latch checks
and the three expected frequency queries verify continuation on both models.

Two single-bit negatives are applied after the existing full-state equality
check, to exercise the continuation assertions independently. Clearing the
pending-high-byte latch fails with `M144A_N80_PCG_TIMER_CONTINUATION`; clearing
the BCD flag fails with `M144A_N80_PCG_TIMER_TONES`. Both start from a passing
fixture and are reverted. The native build and seven focused CTests pass.

No emulator behavior or state format correction was needed. The saved gates
are off and are enabled by the resumed guest, so this does not qualify active
oscillator phase preservation, audio queues, elapsed timer timing or complete
visual replay. G144a remains pending.

## PCG BCD zero-reload interpretation

The 8253 reload contract uses zero for the selected counting modulus: 65,536
in binary, 10,000 in four-digit BCD. The shared PCG decoder previously mapped
both to 65,536. At the unchanged model clock of 3,993,600 Hz, the BCD-zero
integer frequency query therefore returned 60 instead of 399 Hz. The correction
selects the modulus by the BCD flag; binary zero and nonzero decoding retain
their existing behavior. This does not retune the input clock or change state
formats, timer load sequencing or the short-count mute policy.

The fixture checks 60,003 mode-3 frequency cases: all 10,000 valid BCD words
and equivalent binary reloads on three channels, plus binary zero separately.
For each channel, 1,024 stereo PCM frames at 44,100 Hz compare BCD zero with
binary 10,000 after equal gate resets. An independent integer-phase calculation
also checks the reference waveform's sign and stereo agreement, so two equally
wrong generated buffers cannot by themselves pass the period contract.

Three single-mutation negatives reject a shared wrong modulus
(`M144A_PCG_COUNT_ENCODING`), a wrong modulus only in BCD PCM generation
(`M144A_PCG_BCD_ZERO_PCM`), and the same wrong PCM period for both encodings
(`M144A_PCG_PCM_PERIOD`). All start from passing fixtures and are reverted.
Native build and seven focused CTests pass. These are deterministic model/PCM
checks, not physical clock, analog waveform, output-device or save/replay phase
qualification. The FDC clock work remains paused. See the
[bug-fix ledger](../../modernization/bug-fixes.md); G144a remains pending.

## PCG state-domain preflight

The PCG state section previously received version/size checks but no field-domain
validation. A generated SAV with a bank selector changed to 2 passed preflight,
although glyph lookup and programming index a two-bank array with these selectors.
The test stops before applying an accepted malformed fixture; it does not execute
an out-of-bounds access or claim a reproduced crash.

`pcg88_state_valid()` checks the 20 selector/flag/mode bytes independently of
file I/O and live device state. State preflight calls it before machine reset or
state application and rejects violations with `M144A_PCG_STATE`. Every raw RAM,
data, address and counter bit pattern remains allowed, including nondecimal BCD
counter nibbles. This is bounded field-domain validation, not a proof that every
accepted combination is historically reachable. The section layout/version and
optional-section handling are unchanged.

For both model profiles, all 256 values of each constrained byte are classified
(10,240 checks). Eighty generated SAV negatives each change one byte of an
initially passing fixture to either the first invalid value or FFh. They require
the exact error and failed load, with CPU IP, controller state, N80 banking and
a RAM sample unchanged. Removing only the preflight validator call fails with
`M144A_PCG_STATE_GUARD`, independently of the passing pure domain checks. The
mutation is reverted; native build and seven focused CTests pass. See the
[bug-fix ledger](../../modernization/bug-fixes.md). G144a remains pending.

## PCG configuration-toggle refresh

The menu's direct configuration assignment did not notify text refresh. For
an otherwise blank text-only frame, disabling and then re-enabling PCG could
therefore leave the renderer asleep, independently of the port 03h correction.
The menu now uses `pcg88_set_enabled()`, which changes the option and requests
text refresh without resetting the board. Configuration persistence and the
existing redraw call remain in the menu handler.

The live fixture extends the previous 112 frame starts to 144 by adding option
off/on for each profile. It checks the same first-character-row pixels and
byte-for-byte preservation of controller state. A completed counter and a
pending low-byte counter load survive both transitions; the next high-byte
write completes the latter after re-enabling. This does not qualify audio phase
continuity or physical timer timing.

The shared setter and renderer contracts pass the native build and seven
focused CTest checks. Removing the setter's refresh notification fails with
`M144A_N80_PCG_CONFIG_REFRESH`; resetting the board during the toggle fails with
`M144A_N80_PCG_CONFIG_PRESERVATION`. Both mutations start from passing fixtures
and are reverted. Actual ImGui click/presenter automation is NOT_RUN; this is
backend coverage of the menu's shared setter, not a claim of GUI interaction
coverage. G144a remains pending. See the [bug-fix ledger](../../modernization/bug-fixes.md).

## Programmable blank glyphs and frame refresh

The text decoder's blank-code shortcut ran before PCG selection, suppressing
programmable glyphs at 00h and 20h on an ordinary transparent background.
PCG selection now precedes that shortcut; conventional blank handling remains
when no programmable glyph is selected. The matrix covers 80h, 00h and 20h,
for 196,608 combinations and 50,331,648 final-pixel comparisons.

A second refresh defect affected text-only operation: after PCG display was
disabled on an otherwise blank frame, re-enabling it did not wake text decoding.
Port 03h now marks text refresh pending when display or selected-bank state
changes. Data/address and write-bank-only changes do not acquire that new
notification. This preserves the existing character-row cache policy.

The live fixture checks 112 frame starts across the two models, both column
widths, both blank codes and graphics-enabled/disabled settings. Seven phases
exercise a zero glyph, a rewrite, display-bank switching, an inactive-bank
rewrite, disabling and re-enabling display. It checks the first character-row
pixels without caller-forced text refresh after the initial frame. A separate
SR case checks front-page, palette, visibility and background changes at
subsequent raster calls. These are renderer API boundaries, not independently
qualified hardware sampling cycles or final-client presentation.

Starting from passing fixtures, removing the PCG qualification from the blank
shortcut fails with `M144A_N80_PCG_LOW_CODES`; suppressing the refresh notification
fails with `M144A_N80_PCG_LIVE_FRAME`. Both mutations are reverted. The native
build and seven focused CTest checks pass. Fine within-character-row PCG timing,
audio-phase continuity and G144a remain unqualified/pending. The corrections are
recorded in the [bug-fix ledger](../../modernization/bug-fixes.md).

## Guest PCG I/O and state continuation

Enabled PCG writes on the independent mkII/SR main CPU now bypass an overlapping
VA compatible-I/O trap. The earlier N80 direct-I/O allow-list covered memory and
digital video, but omitted PCG. A RAM-resident instruction fixture demonstrated
that the first PCG OUT instead entered native mode at the configured trap vector.
The correction admits only writes to 00h–03h and 0Ch–0Fh when PCG is enabled;
reads, disabled PCG, neighbouring ports, other model roles and inactive 88 mode
retain their existing routing. See the [bug-fix ledger](../../modernization/bug-fixes.md).

The instruction-routing matrix now has 1,440 cases, including immediate, ED and
block I/O, enabled/disabled PCG and six role/mode settings. Its PCG callbacks
check the port, data and exact write count. Removing only the enabled guard is
rejected with `M144A_N80_PCG_GUEST_ROUTE` for a disabled-PCG control case.

The pending-write fixture now uses guest LD/OUT instructions, not host controller
writes, for programming and continuation. It saves at IP C03Ah, advances to
C040h, resets the controller and trap state, then loads the state file. It checks
restored CPU IP/A, trap configuration and PCG state before resuming the pending
OUT. Both model variants must complete the intended store and display-bank
switch without host reconstruction of the pending data or address.

The native build and seven focused CTest checks pass. The 65,536-case compositor
matrix remains passing. These are instruction-dispatch, controller and state-file
contracts; frame-boundary timing, audio-phase continuity and whole-machine
acceptance remain separate. G144a is pending.

## Pending PCG write state contract

The state-file fixture now covers pending PCG programming for both N80 model
variants. It displays bank 0 while programming bank 1, leaves the write bit
set, then changes data and the low address without issuing another write-port
operation. Distinct bytes at the target and adjacent glyph rows detect premature
writes, a stale address, the wrong bank and the wrong source data.

The contract saves that state, resets the PCG, loads the state file, and compares
the complete serialized PCG structure. Issuing only port 02h must then complete
the pending store into bank 1 without altering bank 0 or the previous row.
Switching the displayed bank must select the newly written glyph byte. A
single-bit mutation of the restored data latch is rejected specifically with
`M144A_N80_PCG_PENDING_STATE`; the mutation is not retained.

This checks controller-I/O and state-file semantics, not guest-instruction timing,
audio-phase continuity or whole-machine visual replay. No production correction
was necessary. The native build and seven focused CTest checks pass. Earlier
checkpoint limitations below retain their historical scope; G144a remains pending.

## SR two-page composition contract

The matrix now adds 32,768 SR 320-dot combinations, for 65,536 total cases
and 16,777,216 final-pixel comparisons. The new profile crosses front-page
selection, independent page visibility, text-behind priority, programmable
banks, 40/80-column text, all decoration combinations, blink, global reverse
and graphics pattern presence. The two pages have different transparent
positions and different overlapping colours, with independent expected-index
tables. Disabling both pages must select the backdrop; an enabled zero-valued
page must retain the separate palette-zero behaviour.

A single mutation treats a visible front page as opaque even at colour index
zero while preserving fallback from a disabled front page. The new profile
rejects it with `M144A_N80_PCG_PAGE_COMPOSITION`. The mutation is not retained.
This expands component coverage; it is not whole-machine or timing acceptance.
Programming-time state transitions remain separate work. G144a remains pending.

## Colour and visibility composition contract

The PCG/attribute/compositor matrix now specifies 32,768 combinations and
8,388,608 final-pixel comparisons. It retains the monochrome cases and adds
mkII packed 320-dot colour, SR planar 640-dot colour, SR text-behind priority
and graphics-plane visibility. All four packed and all eight planar colour
indices occur in the independent expected patterns.

The fixture deliberately maps index zero to blue and index one to black.
This distinguishes index-based transparency from RGB-based transparency, and
checks that disabling graphics selects the backdrop instead of palette zero.
The matrix combines these cases with the existing character attributes,
blink phases, global reverse, programmable banks and 40/80-column layouts.
A single-mutation negative replaces index-based priority with RGB-based
priority and must fail specifically with `M144A_N80_PCG_COLOR_COMPOSITION`.
The mutation is reverted; no production behaviour correction was necessary.
The native build and all seven focused CTest checks pass.

This is component coverage, not whole-machine acceptance. SR 320-dot page
composition with the full attribute matrix, programming-time state transitions
and timing boundaries remain separate work. The earlier monochrome-only
checkpoint below retains its original scope. G144a remains pending.

## PCG composition contract refinement

Added a self-contained component matrix for PCG programming, character
attribute decoding and final monochrome composition. The fixture writes
synthetic patterns through ports 00h–03h and supplies character/attribute
memory; it does not inject prepared text or colour rasters. A contrasting
alternate glyph checks programmable-character selection priority.

The matrix specifies 4,096 combinations: two model variants, 40/80-column
layout, two programmable banks, all 32 decoration combinations, both blink
phases, global reverse, graphics pattern presence and text-plane visibility.
It checks 1,048,576 final pixel values. The restored implementation passes
all seven focused checks. A single controlled bypass of programmable glyph
selection fails with the exact code `M144A_N80_PCG_COMPOSITION`; that mutation
is not retained in production code. No production behaviour change was needed.

This extends component coverage, not whole-machine acceptance. Colour-mode
layer priority, state transitions during programming and broader timing
contracts remain separate work. G144a is still pending.

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

## Attribute and capture contracts

[n80_attributes.py](../../../tools/qa/n80_attributes.py) generates
BASIC attribute patterns and defines a fixed-region pixel oracle. Its
parameters cover 40/80-column layout, character 65, ordinary ANK and
semigraphics, all 32 decoration combinations, both blink phases and
global reverse. The rectangle contract specifies an inclusive 51-by-21
dot box and a blank border. These describe test capabilities, not
execution results.

[n80_media_survey.py](../../../tools/qa/n80_media_survey.py) accepts an
explicit plan, isolates per-case configuration and records execution
status. Zero worker exit yields `UNASSESSED`, never a semantic PASS.
It rejects in-tree output and worker-identity changes; intermediate
capture scheduling is optional. Its selftest uses synthetic plumbing
fixtures. The two Python content suites are registered with CTest.

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
