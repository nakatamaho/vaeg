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
This implementation follow-up extends the original source review of accepted
M144, separately from paused WSR-E and M145 work. Earlier checkpoints below
record their own scope at the time; they are not current completion declarations.

## Current completion scope

The maintainer requested completion of M144a, not only N80 demo execution,
then explicitly separated PR201/PDF into [M144b](M144b_pr201_pdf.md).
This is an authorized scope transfer, not completion or silent omission of the
printer work. PR201/PDF is no longer a G144a requirement; its implementation,
printer-specific tests and acceptance belong to G144b. FDC clock verification
remains in M144a, along with PCG phase persistence, explicit BASIC startup
selection and applied-state persistence, remaining display/timing review,
final regression and clean Windows handoff, then G144a.
Accepted M144/G144 remains complete; M144a/G144a is not accepted by this request.
PCG phase persistence and explicit startup selection now have focused software
checkpoints below. The fixed-clock FDC execution check also passes without a
production clock change. The native synthetic regression sweep and assembler
follow-up below extend those checkpoints; they do not complete the media,
presenter, display/timing qualification or clean Windows handoff. Creating M144b
does not start its implementation or mark either human gate passed.
Physical-machine comparison is excluded, not passed. M88M v1.2.4 is a reference
candidate, not an executed comparison claim. Push is now authorized, but the
HTTPS attempt failed because credentials were unavailable. No publication or
merge is inferred; earlier checkpoint statements retain their historical scope.

Public records contain public-source analysis and synthetic tests actually
performed. Private-input tests and emulator-comparison results are not published
or relabeled as public-specification-only tests. Conversation reporting and
out-of-tree test work remain separate from this publication boundary.

## Approved architecture: independent uPD3301, not VA emulation

The maintainer superseded the narrow geometry workaround with complete device
separation: retain VA/VA2's firmware/TSP 3301 emulation; use an independent
X88000-derived uPD3301 for PC-8801/PC-8001 models. Removal of non-VA adaptations
from the VA emulation path is approved; unrelated code/files are not deletion
targets. See [ADR-0016 amendment](../DECISIONS/ADR-0016-v1v2-mode-series.md#m144a-amendment-independent-lsi-versus-va-emulation).
No further non-VA workaround is to be added to the VA/TSP controller path.

The first implementation subset is `io/upd3301.c/.h`, a per-instance C99
command-state adaptation of the pinned X88000 1.5.3 sources. It has no VA/TSP,
ROM, frontend, global-instance or CPU-clock dependency. RESET parameters take
effect individually as in the source; interrupted RESET/cursor/light-pen commands
and command cancellation retain their source behavior. Display-start status and
DMA-enabled display activity are distinct. Blink phase preserves the source's
strict boundary and 16-bit frame-counter wrap. This is **not yet a connected
machine device**; the application does not link this subset yet.

Synthetic standalone tests cover 1,280 parameter-position/value cases, all 256
command bytes, 256 parameter decodes, 262,144 phase samples, copy continuation
and independent instances. Copy continuation is not a machine-SAV qualification.
C99 `-Wall -Wextra -Werror`, ASan/UBSan and the dedicated CTest pass. Independent
single mutations force the attribute count to 20 and change the cursor phase
boundary; they fail with `UPD3301_DECODE` and `UPD3301_PHASE`, respectively.
The existing emulator executable is unchanged; no full-runtime sweep or new
Windows handoff is claimed for this unconnected subset.

### Independent board text-DMA control and memory view

`io/pc88_text_dma.c/.h` now separates X88000's board-side 64h/68h control
and text-source selection from the LSI command core. The board supplies the
reset address; the device retains the partially written address and byte phase.
68h resets that phase without clearing the address. Source selection wraps to
16 bits before choosing ordinary RAM or an explicitly enabled fast-text window.
The read-only view has no CPU memory-decoder callback and no VA/TSP dependency;
missing/short selected buffers fail without modifying the output byte or silently
falling back to another bank. This is not a timed transfer or a complete 8257.

Synthetic tests pass for all 65,536 address-byte pairs, all 256 mode bytes,
65,534 unsupported full-width port values, and 1,048,576 source lookups spanning
wrap and window boundaries. Controller/DMA enable composition, copied partial
state and invalid-view rejection are covered, but not actual machine routing
or SAV. C99 warnings-as-errors, ASan/UBSan and both dedicated CTests pass.
Three independent mutations break byte-phase reset, pre-window address wrap
and view bounds; the required failures are `PC88_TEXT_DMA_MODE`,
`PC88_TEXT_DMA_ADDRESS` and `PC88_TEXT_DMA_SHORT_SOURCE`. Each starts from the
passing test fixture and changes only the intended implementation expression.
The application still does not link either independent subset.

### Independent text rows and full-cell cursor masks

`vram/pc88_text.c/.h` now decodes bounded rows from the independent board
memory view, using the pinned X88000 `RatchText` attribute-walk behavior.
It supports transparent mono/color and no-attribute modes with 80 logical
slots, packing those into 80 eight-pixel or 40 sixteen-pixel cells. The supplied
carry is independent of TSP and output is committed only after all row reads
succeed. Colour/decorations, secret/blink substitution, row carry and per-row
line-decoration reset are explicit. Duplicate positions consume one pair per
displayed cell. Forty-column mode skips alternate logical slots without
consuming extra attribute pairs for those skipped slots.

The cursor reverses its entire independent character cell; odd raw cursor
positions do not match a 40-column cell, as in the source. Synthetic zero-glyph
masks verify eight pixels at 80 columns and sixteen pixels at 40 columns, not
a stretched VA sprite. This is **not yet a fix in the running application**.
Font/PCG selection, vertical scaling, palette/graphics priority, display gating,
clocked scanout and other cursor shapes still require integration/qualification.
Nontransparent/reserved attribute modes and other logical widths fail explicitly;
the source drawer's plain-text fallback is not presented as hardware support.

Tests pass for 3,200 pitched rows (all 1-32 pair counts, both widths and
transparent modes), 8,192 cursor cases, 65,536 glyph-row mask cases, start/end
and duplicate attribute positions, carry/decorations, secret/blink boundaries,
no-attribute mode and atomic failure. The row matrix includes 16-bit address
wrap. C99 warnings-as-errors, ASan/UBSan and all three independent CTests pass.
Single mutations detect fixed-20-pair pitch (`PC88_TEXT_PITCH`), half-width
expanded cursor masks (`PC88_TEXT_CURSOR_WIDTH`), leaking line decoration
between rows (`PC88_TEXT_LINE_DECORATION_RESET`) and partial output on a failed
read (`PC88_TEXT_ATOMIC_SOURCE`). No application runtime or machine-SAV PASS
is inferred; the emulator binary and existing VA implementation are unchanged.

### Independent vertical clock and sync interval

The independent `UPD3301` now owns a device-domain countdown and resolution
profile. X88000's four vertical cycle/sync tables are explicitly source-model
values, not measured hardware timing. Integer scaled-tick arithmetic preserves
fractional phase at non-integer-MHz rates and makes elapsed time independent
of dispatch batch size. Multiple frame boundaries are counted without a
per-frame loop; the frame counter wraps to 16 bits while the returned boundary
count remains 64-bit. A boundary latches the vertical interrupt request even
when display/DMA is stopped, matching the source's no-WAIT clock path.

R2 timing changes preserve the remaining countdown, including when it exceeds
the new cycle length. START and RESET COUNTERS restart the countdown and frame
counter; interrupt acknowledge leaves phase intact. Invalid clock configuration
or advance leaves state and output unchanged. This numerical component guard
does not substitute for machine/SAV binding or whole-controller validation.

Synthetic tests cover 320 profile/rate combinations, 2,880 advances, 320
bulk-versus-partition comparisons, sync/frame edges, pending-RESET copied-state
continuation, command effects and invalid-state guards. Rates of one Hz and
UINT32_MAX Hz are arithmetic stress fixtures, not supported machine claims.
A composed test drives the 40-column cursor from elapsed device ticks through
the display/DMA gates, row decoder and glyph mask: frame eight is hidden and
frame nine has a full sixteen-dot mask, with the adjacent cell unchanged.
C99 warnings-as-errors, ASan/UBSan and the four independent CTests pass.
Single mutations are detected as `UPD3301_CLOCK_VSYNC_EDGE`,
`UPD3301_CLOCK_R2_CONTINUITY`, `UPD3301_CLOCK_COMMAND_RESET` and
`UPD3301_CLOCK_ADVANCE` (truncated multi-frame count).

Horizontal scanout, WAIT/DMA arbitration, board clock-domain mapping, IRQ
routing and actual machine SAV remain unimplemented. The emulator application
still does not link the independent components; existing VA/M135/FDC timing
and the retained application worker are unchanged. No new runtime gate or
physical-timing qualification is claimed.

### Independent display state codec

`io/pc88_display_state.c/.h` serializes the independent controller, its
byte-phase DMA control and the clock countdown as an explicit 32-byte
little-endian versioned payload. Validation runs before any commit in both
directions: command/parameter consistency per command mode, binary-only
flags, interrupt mask/request bounds, attribute bit 3 (the source model's
DMA-colour bit), DMA-enable agreement between the controller and the mode
byte, numeric clock bounds and the board-supplied clock binding. A payload
whose clock profile differs from the board is rejected (`PC88_STATE_BINDING`)
instead of silently rescaled. Invalid input leaves the destination state and
any output byte unchanged.

This is a component codec, not a machine SAV: no section is registered in
`statsave.tbl`, no machine identity is checked here, old `N80CRTC` geometry
packets are not interpreted as complete device state, and no RAM/font payload
is carried. Machine binding, legacy-state policy and atomic SAV integration
are the next integration step, to be qualified against real saves.

Synthetic tests pass for all 256 command-byte valid states, 38 flag/clock/
DMA guard cases, the full 32-byte single-flip sweep (each flip either
rejected or re-encoded bit-exactly) and interrupted-RESET continuation with
clock phase. C99 warnings-as-errors, ASan/UBSan and five independent CTests
pass. Single mutations are detected as `PC88_STATE_ENCODE_BINDING` (encode
skips the clock binding), `PC88_STATE_INTERRUPT` (decode skips candidate
validation) and `PC88_STATE_EXHAUSTIVE` (encode truncates the frame counter).
No application runtime or machine SAV PASS is claimed.

### Guest I/O routing to the independent device

`io/n80_crtc.c/.h` now binds the independent device into the application:
the board adapter owns the global `UPD3301` and text-DMA control, resets them
via the iocore reset table, and attaches 50h/51h (parameter read/write,
status/command) and 64h/68h (text DMA address/control) handlers. Handlers
gate on the live model, so binding order versus model selection does not
matter. Port 68h keeps the controller's DMA gate in
agreement with the mode byte, as X88000's `WriteIO_68` does, preserving the
state-codec invariant.

The maintainer confirmed the machine roles: in VA mode the uPD3301 is the
firmware/TSP emulation; on the actual PC-8801/PC-8001 machines it is a real
chip. `N80DirectIO` therefore enables the independent device for the whole
8801/8001 family (PC-8801 selections and PC-8001mkII/SR models): immediate,
ordinary ED and block I/O to 50h/51h/64h/68h are no longer intercepted by the
VA firmware trap; the instruction itself delivers each byte through the compat
bus to the device. This is the approved removal of the firmware-trap route for
independent machines. The other implemented N80 controls stay PC-8001-only,
and the VA/VA2 firmware trap remains armed and asserted for VA roles.

`tests/upd9002/main_cpu_roles.c` is updated deliberately: the guest-I/O
selftest adds ports 64h/68h and asserts device-visible effects for direct
50h/51h/64h/68h writes and device reads for 50h/51h inputs on PC-8801 and
mkII/SR roles, while retaining the native-trap expectations for the same ports
on VA roles. The dispatch selftest now expects actual per-byte delivery with the
firmware trap armed (`M144A_N80_CRTC_TRAP_BOUNDARY` no longer applies to
independent models; `M144A_N80_CRTC_OUTPUT_SEQUENCE` and
`M144A_N80_CRTC_OUTPUT_DATA` cover the delivery contract, 168 cases). The
full native suite passes 183/183 after the change.

Not yet wired: the independent vertical clock into the scheduler, IRQ
delivery, rendering consumption of the device state, and the machine-SAV
section. The retained application worker identity changes with this commit;
new runtime and Windows qualification are still outstanding before any gate.

### Migration and approved removal scope

| Current coupling | Replacement/removal condition |
| --- | --- |
| `maketextva.c` independent N80 pair/pitch overrides | Remove from VA text emulation when the independent DMA/text renderer is connected; keep VA native frame-table behavior. |
| `makesprva.c/.h` exported `cursorraster` and its N80 consumption in `scrndrawva.c` | Replace with the independent 3301 cursor. Preserve VA CURDEF, sprite collision/budget and its established 40-column behavior. |
| Non-VA glyph/attribute selection inside VA text emulation | Move the model-specific choices to independent text rendering; do not indiscriminately remove accepted VA glyph/PCG support. |
| `memoryn80_crtc` geometry-only state and its rendering helpers | Replace with independent controller ownership; define explicit old-state handling rather than interpreting an old packet as a complete device. |
| Firmware trapping of independent 50h/51h and width/DMA controls | **Done for 50h/51h/64h/68h** (`io/n80_crtc.c`): direct delivery with block I/O; VA trap retained for VA models. Width/other controls and rendering consumption remain. |

Remaining work includes machine wiring of DMA/source addressing, the independent scan/clock path,
interrupt wiring, text/attribute rendering, cursor width/position and state codec,
then guest-port integration and removal of the approved couplings. `WIDTH 40,25`
and 80-column cases must verify cursor width against the character cell on
independent machines, while retaining the VA reference behavior. Do not reuse
X88000's WAIT estimates as measured hardware constants or alter M135 VA timing
as a side effect. All independent 8801/8001 models need coverage; mkII/SR are the
initial integration targets, not the final architectural boundary.

## Independent N80 cursor composition correction

[Fix `e3adf0af83ddeee6ccdd3f2bee87477f06e43091`](https://github.com/nakatamaho/vaeg/commit/e3adf0af83ddeee6ccdd3f2bee87477f06e43091)
corrects an omitted cursor in the independent mkII/SR compositor. The existing
CURDEF sprite machinery generated the cursor and applied its blink phase, but
the independent N80 branch composed only text and graphics and returned before
the generic text/sprite composition. Consequently the cursor never reached the
final framebuffer even when its raster was visible.

The sprite generator now exposes a separate derived cursor raster, populated only
by the enabled CURDEF cursor and cleared alongside the ordinary sprite raster.
The independent N80 compositor consumes it subject to text visibility, sprite
layer visibility and the existing text/graphics priority. Other native sprites
are not admitted. The original VA sprite raster, collision processing, clipping,
line budget and blink clock are unchanged; no SAV section or timing change is
introduced.

Synthetic source-built fixtures cover 256 combinations of mkII/SR, colour/mono,
blink phase, blink enable, cursor enable, DSPON, SPRON and text hiding. They check
both cursor pixels and exclusion of an unrelated native sprite, layer hiding and
raster clearing. Two independent reverted mutations fail as intended:
`M144A_N80_CURSOR_COMPOSITION` when the compositor omits the cursor and
`M144A_N80_CURSOR_RASTER` when the blink suppression is removed. The restored,
identity-refreshed native worker reports `Rel.20261108 (m144a, e3adf0af)`.
The complete configured CTest suite passes: **178 registered, 162 PASS, 16 SKIP,
zero FAIL**, 375.31 seconds. Skips remain skips; this checkpoint does not claim
Windows execution, presenter qualification, hardware agreement or G144a acceptance.
Private-input observations remain outside Git under the existing publication rule.
The separate variable-attribute guest-input correction remains incomplete.

## Authorized X05 geometry correction

The maintainer approved the following qualification statement:
**「X88000ソースに基づく互換モデル。M88Mソースでも整合。実機一致は未確認」**
The requested correction is scoped to independent PC-8001mkII/SR: propagate the
CRTC attribute-pair count to drawing, use `80 + 2 * pairs` bytes per 80-character
row, retain the 20-pair baseline and check 2/5/10 pairs, including interrupted
parameter packets, reset and actual SAV restoration. This does not authorize
changing VA/VA2 native TSP behavior, WAIT constants or SR port polarity.

### Guest-output dispatch qualification (geometry intake still unconnected)

[Test commit `90c85fee150ab5fc0843eeb9d721e841cd5b042b`](https://github.com/nakatamaho/vaeg/commit/90c85fee150ab5fc0843eeb9d721e841cd5b042b)
adds 168 synthetic instruction cases to `tests/upd9002/main_cpu_roles.c`:
both independent models, ports 50h/51h, trap enabled/disabled, immediate output,
all eight ordinary ED output encodings, and OUTI/OUTD/OTIR/OTDR. Block fixtures
use B=1/3/0 and source offsets that cross the 16-bit boundary in both directions;
repeat B=0 must produce exactly 256 writes. Synthetic byte sequences, not ROM
or guest-program payloads, supply the expected data.

With the trap enabled, these instructions enter the native handler before a
monitored device write, with original HL/BC/A and the instruction address retained
in the native return frame. With the trap disabled, the monitor receives the
expected port, byte sequence and exact write count; final PC/HL/BC are checked.
This qualifies dispatch, not firmware execution or CRTC behavior: the monitor
replaces the output device, and the test stops at trapped handler entry.

Two single production mutations were applied and restored independently:
bypassing the 51h trap fails `M144A_N80_CRTC_TRAP_BOUNDARY`; aliasing its output to
50h fails `M144A_N80_CRTC_OUTPUT_DATA`. Both begin from the passing fixture and
reach the intended failure without an earlier test masking it. The restored,
identity-refreshed worker reports `Rel.20261108 (m144a, 90c85fee)`; nine focused
CTests pass. The earlier complete regression belongs to its recorded worker,
not to this new test-only candidate. No full-sweep result is relabeled.

**Implementation consequence:** attaching geometry only to the adapter's normal
`Out()` callback misses enabled traps. Conversely, pre-reading a trapped block's
source buffer is not a proven substitute: `upd9002_memoryread_nowait()` still
enters the memory decoder, and a repeat's later reads must not be assumed to have
happened at trap entry. A safe connection must preserve the existing firmware
command path, actual byte-delivery order and partial-state semantics without
inventing a second read stream. No speculative observer, trap bypass, WAIT change
or ROM-address workaround was introduced. Guest geometry intake and the original
display defect remain OPEN; this checkpoint is test-only, not a correction.

### Renderer and machine-SAV connection checkpoint

[Implementation `1a827742a3442211e7cdefa5074e971fcadd921a`](https://github.com/nakatamaho/vaeg/commit/1a827742a3442211e7cdefa5074e971fcadd921a).
The core now owns `memoryn80_crtc`, with writes and geometry use gated to the
applied mkII/SR machine in compatible mode. Raster rendering and text-copy row
addressing share the independent byte pitch. The native TSP frame table is not
rewritten; VA/VA2, base PC-8001, other machines, native mode and unavailable
geometry retain their existing paths.

Optional `N80CRTC` version 1 stores the eight packet bytes. Its section-level
preflight reports `M144A_N80_CRTC_STATE` for invalid version/size, noncanonical
payload or incompatible applied-machine content, before mutation. Whole-container
CPU/machine checks can reject an incompatible machine earlier. Machine reset clears both
applied and partial geometry. Missing legacy sections select the saved TSP
layout; lost partial-command information is not reconstructed. Older readers
that reject unknown sections may reject newly written states, including states
whose new section contains inactive geometry.

Synthetic model fixtures exercise real SAV files for 2/5/10/20 pairs, at all six
parameter boundaries: 24 continuations per model, 48 total. Each continuation
checks a three-row, differently coloured raster and the corresponding text-copy
addresses while deliberately keeping the native table at a 120-byte stride.
Fourteen malformed-file cases cover flags, active-state consistency, packet
index, pending bytes, version and size. Legacy omission, production ROM-reset
clearing, inactive-model/native-mode guards and the content-level machine guard
are checked separately.

Four single reverted production mutations fail with the intended codes:
`M144A_N80_CRTC_RASTER_PITCH`, `M144A_N80_CRTC_TEXT_PITCH`,
`M144A_N80_CRTC_SAV_GUARD` and `M144A_N80_CRTC_SAV_CONTINUATION`.
The restored native build and nine focused CTests pass. The complete configured
suite also passes with **162 PASS, 16 SKIP, zero FAIL** (178 registered; 367.33
seconds). That sweep used the final implementation with pre-commit build
identity. After regenerating identity for `1a827742`, all 25 CTests whose commands
bind the application were rerun and passed (365.89 seconds); hashes of all other
build-local command artifacts remained unchanged. The qualified application
reports `Rel.20261107 (m144a, 1a827742)`. Skips retain their declared scope;
the assembler-dependent direct runs
recorded below are separate evidence, not reclassified results of this sweep.

Preserved fixture-development failures: an incorrectly ordered `FillMemory`
argument pair initially produced the wrong synthetic characters; corrected the
fixture. An attempted whole-SAV machine-guard negative was intercepted earlier
by `M139_MAIN_CPU_STATE_MACHINE_MISMATCH`; isolated the production content/profile
validator instead, so an unrelated validator cannot mask the intended check.
Neither failure is represented as a diagnosed emulator defect.

**Guest I/O remains unconnected.** These tests call the controller API, not guest
50h/51h instructions. The full guest-visible defect remains OPEN until that route,
including block output and existing command behavior, is connected and verified.
No private-input execution result, real-window coverage or human acceptance is
claimed by this checkpoint.

### Geometry packet foundation (not yet the guest-path correction)

[961f0e97d0274909867115cc077567e6ddebbfdf](https://github.com/nakatamaho/vaeg/commit/961f0e97d0274909867115cc077567e6ddebbfdf)
adds a pure C99 geometry packet layer to `io/pc8001_video.*`. It consumes RESET
command/parameter packets, applies R4 only after the fifth parameter, and retains
the preceding applied geometry until that point. A different command cancels an
incomplete packet. Machine reset removes the applied geometry. Transparent mono
and colour modes report the specified pair count and 80-byte-character-region
pitch; other modes return an explicit unavailable result, not an invented layout.

The eight-byte, pointer-free representation retains the partial packet and the
previous applied R4 separately. Validation rejects noncanonical flags, indices,
unreceived bytes and invalid sizes before loading any state. This is a packet
codec, **not yet an `N80CRTC` SAV section or an actual machine save/load test**.
There is no new global device state or guest I/O interception in this checkpoint.

Synthetic tests cover 32 counts, eight mode fields and all six parameter-packet
boundaries (1,536 combinations), including the requested 2/5/10/20 counts.
They test continuation through a codec round trip, pending-versus-applied state,
command cancellation, ignored unrelated ports/data, machine reset and atomic
invalid-load rejection. The existing pixel/address suite plus these checks
passes 2,900,774 assertions in normal and ASan/UBSan standalone C99 builds;
the production CMake `vaeg_pc8001_video` target also passes.

Four isolated source mutations, each starting from the passing fixture, fail
with their intended stable codes: fixed count → `M144A_N80_CRTC_PAIRS`, fixed pitch
→ `M144A_N80_CRTC_PITCH`, lost packet position → `M144A_N80_CRTC_CONTINUATION`,
and omitted index guard → `M144A_N80_CRTC_STATE_GUARD`. Mutations remain outside
the repository. These files are outside the closed clang-format manifest; no
unrelated formatting or copyright-header changes are made.

**Still required:** connect real guest I/O (including block-I/O semantics without
duplicate bus accesses), the renderer/text-copy row addressing, model/reset guards
and actual SAV preflight/restore. The existing VA firmware bridge must not be
silently bypassed for commands this layer does not implement. The previously
identified guest-path issue is not fixed by these primitive tests; the old native
worker and previous integration evidence are not promoted to a corrected build.
G144a remains pending and push remains held.

## Native regression and assembler follow-up

Evaluated repository state:
[47af24d17138613af9d5e810ba1ae5cf3c9fa36e](https://github.com/nakatamaho/vaeg/commit/47af24d17138613af9d5e810ba1ae5cf3c9fa36e).
The native worker remains the built
[fe5d87f0368f76ef453dd3d57a72a41e33975533](https://github.com/nakatamaho/vaeg/commit/fe5d87f0368f76ef453dd3d57a72a41e33975533)
worker, SHA-256
`86dc1ec99749c9efac3bc05db78a3d927d3317240e11a8b97e9343523710e4aa`.
The intervening changes are exactly four Markdown paths under `docs/`; emulator,
build and test-source inputs are unchanged. This is explicit identity-bound reuse,
not a claim that the worker displays the later documentation commit.

The complete configured native suite was run, not only the nine focused tests:

```sh
ctest --test-dir "$BUILD" --output-on-failure -j 4 --timeout 900
```

Exit **0**, 367.54 seconds: **178 registered, 162 PASS, 16 SKIP, zero FAIL**.
CTest's aggregate success line does not turn skipped tests into passes.
Additionally, `vaeg_cpmva_zexbuild` initially passed with three inner unittest
cases skipped. The suite covers synthetic/local contracts, not the mandatory
media rebuild/guest campaign or actual window, speaker or Windows execution.

Eight skips were caused by the missing z80asm 1.8 executable. Built that exact
upstream version locally, without installing a system package, from Debian's
[`z80asm_1.8.orig.tar.gz`](https://deb.debian.org/debian/pool/main/z/z80asm/z80asm_1.8.orig.tar.gz).
The archive SHA-256
`67fba9940582cddfa70113235818fb52d81e5be3db483dfb0816acb330515f64`
matches the publisher's source descriptor retrieved over HTTPS; this does not
claim an independently verified OpenPGP signature. Build and upstream tests exit 0.
Using that assembler and Python 3.12, the following direct supplemental commands
all exit **0**:

```sh
for name in fdc_tools seed fixed capture xadr pref; do
  python3.12 -B "$REPO/tools/hardware/upd780/test_${name}.py" \
    --runner "$BUILD/vaeg_upd780_fdc_probe_runner" --assembler "$Z80ASM"
done
for role in upd70008 upd780; do
  python3.12 -B "$REPO/tools/qa/z80_role_probes.py" run \
    --manifest "$REPO/tests/z80_compat/${role}_probes.json" \
    --runner "$BUILD/vaeg_cpm_runner" --assembler "$Z80ASM" \
    --jobs 2 --work "$OUT/${role}-probes"
done
PATH="$(dirname "$Z80ASM"):$PATH" \
  python3.12 -B "$REPO/tools/cpmva/tests/test_zexbuild.py"
```

The six FDC payload suites and both CPU-role probe manifests PASS; the CP/M build
suite now executes all **14 tests, no inner skips**. These are separate executions
of the same test entry points with an explicitly bound assembler and Python 3.12,
not a retroactively edited CTest result. The synthetic
FDC checks do not establish physical fetch/SP/Q behavior or silicon timing.
Assembler sources, binaries, generated payloads and exact receipts stay out of tree.

Eight original CTest entries remain **NOT_RUN/SKIP** in this sweep:

- `vaeg_recording_h264_hardware_probe`: no hardware-encoder coverage inferred;
- `vaeg_sst_v20_ci`: external corpus binding not configured;
- `vaeg_m103b_basic_boot`: required integration bindings not configured;
- upstream xz's `test_suffix.sh`, `test_files.sh` and three
  `test_compress_generated_*` cases: the xz command-line program was not built.

### Remaining gate work, not promoted by this sweep

- M1/display-DMA/GVRAM WAIT equivalence and physical SR port-33 polarity remain
  unqualified. Preserve the existing source-model polarity, VA timing profile and
  fixed FDC domain; do not infer numerical corrections from these software tests.
- X05's guest CRTC-to-renderer connection remains open. In the current public
  [main-CPU bridge source](https://github.com/nakatamaho/vaeg/blob/47af24d17138613af9d5e810ba1ae5cf3c9fa36e/cpu/upd9002_upd70008.cpp),
  ports 50h/51h are outside `N80DirectIO` and retain the VA firmware trap path.
  Variable-pair support in the TSP renderer alone is not an independently modeled
  N80 CRTC command/parameter interface. Qualify variable attribute counts, byte-row
  pitch, partial command state and reset/SAV behavior before closing this item;
  do not patch a private firmware address or hardcode one observed row layout.
- X07 remains the original optional international-font review finding, not an
  implemented `optfont` switch. SR glyph-bank/fallback support must not be relabeled
  as that separate feature; the authorized implementation extension below lists
  the N80 work rather than an international-font implementation.
- Complete the applicable display/font and startup/history presenter checks on an
  available capture target. A mapped software-X11 target is valid only under its
  explicit target label; dummy video is not a substitute. Routine visual checks
  remain machine-owned, not reassigned wholesale to the maintainer.
- Complete the required clean-commit demo/utility media and guest regression,
  recording/shader-enabled static Windows handoff and its stated runtime scope.
  This native sweep does not satisfy those campaigns.
- Push remains held; G144a remains PENDING. No M144b implementation, acceptance,
  merge, release or cleanup follows from these test results.

## FDC main-clock independence verification

Added a production-reset execution check, not a nominal-frequency-only check.
The ordinary FDC integration fixture deliberately uses multiplier one; this
new case calls `subsystem_reset()` after setting the real clock configuration,
so `Subsystem::Reset` installs the fixed `pccore.multiple`. It varies main CPU
multipliers 1/2 and S/H, covering nominal 2/4/8 MHz main clocks, including the
requested 4-versus-8 comparison. The shared machine timeline remains fixed.

An independently counted synthetic guest loop increments HL and stores it to
FDC RAM: 10 initial T-states plus 100 iterations of 32 T-states. Production
main-clock conversion supplies the corresponding shared-clock budgets. Batch
and instruction-loop-sized deliveries must both produce RAM counter 100,
HL=100, the expected PC, zero remaining credit and identical complete CPU save
images across all eight cases. The FDC executes instructions; this is not a
comparison of two printed frequency fields.

The positive case passes without a production clock correction. A single
reverted mutation replacing the fixed multiplier with `pccore_cpu_multiple()`
fails exactly `M144A_FDC_MAIN_CLOCK_COUPLING`. Retain the nominal 4 MHz FDC domain
of 3.9936 MHz; no crystal, WAIT, PIO sleep or instruction timing is changed.
This bounded contract does not qualify disk mechanics, physical elapsed time,
firmware workloads or hardware WAIT behavior. Existing wrapper WAIT/debt tests
remain their legacy software contract, not newly established physical timing.

Native configure/build and the same nine focused CTests pass at
[fe5d87f0368f76ef453dd3d57a72a41e33975533](https://github.com/nakatamaho/vaeg/commit/fe5d87f0368f76ef453dd3d57a72a41e33975533).
This verification adds tests, not a new timing correction or bug-fix ledger entry.

## Explicit N80 BASIC startup selection

Implemented requested configuration versus applied startup state versus live ROM
banking as separate values. The model menu offers N/N80 and, on SR, N80SR when
its role is available. `N80BasicMode=0..3` retains automatic as the default.
Port 30h reads the applied choice; one-shot bootstrap selects SR only for an
applied SR start. Pending preferences and subsequent port-33 writes do not change
those startup bits. Optional `N80BOOT` version 0 saves the applied byte without
altering `N80BANK`; preflight rejects invalid values before mutation. Missing
sections preserve automatic legacy behavior. Unsupported configuration requests
are explicitly warned and fall back to automatic after ROM-role loading.

Public implementation references distinguish the input contract from hardware
claims: MAME's published [pc8001 driver](https://github.com/mamedev/mame/blob/mame0280/src/mame/nec/pc8001.cpp)
has the N/N80/N80SR input choices and
combined N/N80 ROM roles; the mkII manual's SW8 behavior is already recorded
below. The existing source-model SR bank polarity is unchanged. No physical
reset-key timing or new WAIT model is inferred.

Synthetic tests exercise all four requests on each model, including the mkII
unsupported-SR fallback; applied switches and bootstrap mapping; pending setting
and live-bank independence; eight SAV round trips, eight invalid-state mutations
and eight legacy-section omissions. Configuration uses the real INI reader/writer.
Recent-history tests cover all four new field values, old row lengths, malformed
field normalization, retained display metadata and mode-sensitive record equality.
These history tests exercise production serialization and the new field parser,
not actual presenter interaction or a full history-file restart campaign.

Reverted single mutations use pending configuration for port 30h and omit the SAV
value guard; they fail exactly `M144A_N80_STARTUP_PENDING` and
`M144A_N80_STARTUP_SAV_GUARD`. Native configure/build and all nine focused CTests
pass on [71a959a8005ba4c764ebaf2d9fdec2df65ad8a73](https://github.com/nakatamaho/vaeg/commit/71a959a8005ba4c764ebaf2d9fdec2df65ad8a73).
This includes ROM-less, PC8001 video, main-CPU bridge, state storage, the three
N80 QA contracts and sound-generation/capture tests. Full milestone regression,
presenter checks and G144a remain pending.
See [configuration and compatibility behavior](../../modernization/extra-roms.md#pc-8001-basic-startup-selection).

## PCG active-phase SAV continuation

The generated-input regression isolates a missing mixer state: `PCG88` saved
controller registers and glyph RAM, but not the separate three `pcgsnd.phase`
values. Load reset those phases, restarting active square waves even when the
controller bytes matched. The correction adds optional `PCGPHASE` version 1:
24 bytes, three little-endian IEEE binary64 normalized phases. The existing
`PCG88` payload is unchanged. Old SAVs without the extension retain deterministic
phase-zero reset; phase continuity cannot be recovered from their registers alone.

Preflight rejects wrong version/size, negative values (including negative zero),
values at least one, infinities and NaNs with `M144A_PCG_PHASE_STATE`, before
changing live state. The codec rejects unsupported floating-point representations
and validates all channels before applying any. Mixer volume, output-device
queues and sample-rate configuration are not part of this extension.

Synthetic checks cover 24,576 sign/exponent/fraction/channel classifications,
known little-endian encodings, independently known half-cycle arithmetic,
null/length guards and failed-load atomicity. Full SAV tests on both N80 models
program three unequal binary/BCD counters through guest-facing I/O, warm the
oscillators, save, render the reference continuation, perturb, restore and compare
all 512 stereo sample values at the same rate and volume. Fresh/reset PCM must
differ from continuation. Sixteen independently mutated SAVs test payload,
version and size rejection with unchanged sampled CPU/controller/phase state.
Removing only `PCGPHASE` tests the legacy reset behavior.

Reverted production mutations fail exactly `M144A_N80_PCG_PHASE_RESTORE`
(omit phase application) and `M144A_PCG_PHASE_SAV_GUARD` (omit preflight payload
validation), starting from the passing fixture. This is synthetic mixer/SAV
coverage, not speaker, cross-platform PCM equivalence, hardware timing or G144a
acceptance. The native build and nine focused CTests pass: ROM-less, PC8001
video, main-CPU bridge, state storage, N80 state/attributes/media-survey
contracts, sound generation and sound capture. Build/test receipts are retained
out of tree; this is not the final milestone-wide regression campaign.

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

Historical authorization: the printer requirements below are now carried by
[M144b](M144b_pr201_pdf.md), following the maintainer's explicit scope split.
The N80 and FDC requirements remain in M144a.

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
