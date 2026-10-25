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
EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
-->

# Fixed-target screenshot comparison

`screen_compare.py` is the content-comparison component of local display QA.
It uses only the Python standard library. It is not yet a real-display capture
adapter, OCR engine, or an independent attestation that supplied metadata is true.
Passing its synthetic tests is not a vaeg visual-regression PASS.

## Capture and reference requirements

- Obtain a **real final-client** capture, including the information/panel area,
  using a trusted application/OS capture adapter. Bind its metadata to the exact
  PNG. A guest framebuffer or dummy-driver screenshot does not qualify.
- Fix the platform, backend, video driver, DPI, logical/client pixel dimensions,
  fullscreen mode, settings, guest readiness/frame, fonts and theme. Pause or
  synchronize the guest for deterministic static comparisons. Record the worker
  commit/digest and private corpus/config identities alongside capture evidence.
- Approve a separate baseline once. Freeze the policy and reference digest
  before evaluating the candidate. Do not regenerate/promote the baseline from
  candidate output merely to make a failing comparison pass. There is no
  automatic baseline-update mode.
- Keep references, capture metadata, policies, hashes of private inputs and
  generated reports/diffs **outside Git and hosted CI**. The policy digest passed
  on the command line must be the previously reviewed/frozen value, not an
  automatically recomputed acceptance decision.
- Use a separately approved policy/reference per target. Cover normal/narrow,
  supported DPI scales, fullscreen, both toggles' four combinations and each
  applicable backend. Use expected fullscreen panel suppression, not an image
  with an incorrectly visible panel as a new baseline.

Human work is initial baseline approval, intentional visual-change approval and
final gate judgment, not repeated overlap/clipping checks. If a display cannot
be captured, the capture case is **BLOCKED**. Do not move it back to the human
checklist or fabricate real-client provenance for dummy screenshots.

## Policy example

The following dimensions and rectangles are **illustrative**, not approved vaeg
geometry or a supplied baseline. All coordinates are physical image pixels;
`logical_size` describes the application/OS logical client size at the fixed DPI.
Replace the reference digest with the approved private image's digest.

```json
{
  "contract": "screen-compare-v1",
  "width": 640,
  "height": 424,
  "reference_sha256": "<approved reference SHA-256, 64 lowercase hex characters>",
  "target": {
    "platform": "macos",
    "backend": "sdl-renderer-software",
    "video_driver": "cocoa",
    "dpi_scale": 1,
    "logical_size": [640, 424],
    "fullscreen": false,
    "status_visible": true,
    "panel_visible": false
  },
  "masks": [
    {"name": "fps-digits", "rect": [610, 405, 20, 10]}
  ],
  "regions": [
    {"name": "full-client", "rect": [0, 0, 640, 424], "max_delta": 0, "max_changed_fraction": 0},
    {"name": "guest", "rect": [0, 0, 640, 400], "max_delta": 0, "max_changed_fraction": 0},
    {"name": "status", "rect": [0, 400, 640, 24], "max_delta": 0, "max_changed_fraction": 0}
  ]
}
```

A `panel` region is mandatory when `panel_visible` is true. `full-client` must
cover the complete image. Guest/status/panel regions must be in bounds and
mutually nonoverlapping. Add further regions for individual critical fields or
lamps as needed. A PNG whose size differs from the frozen policy fails; the
comparator does not scale, align, crop or blur mismatches away.

Exact comparison is the default illustrated above. A reviewed exception may
allow channel deltas up to 16 and changed-pixel fractions up to 1% per region;
these are ceilings, not recommended defaults. Keep critical text/lamp regions
exact. Named FPS/cursor masks may cover no more than 5% of any compared region;
overlapping masks are conservatively counted twice. Do not mask whole labels,
footers, guest borders or rendering defects. The diff image shows all raw pixel
changes, including masked/subthreshold changes, so review cannot lose them.

## Actual capture metadata and execution

The trusted capture adapter must emit an object with:

- `capture_kind`: exactly `final-client`;
- `target`: the actual measured settings, equal to the frozen policy's target;
- `image_sha256`: the exact actual PNG digest.

Only `cocoa`, `windows`, `x11` and `wayland` are accepted policy video drivers.
Software rendering on a real window is not the same as SDL dummy video.
Metadata supplied by the caller is a binding, not cryptographic proof of image
origin; copying policy values into guessed metadata is not capture automation.
The future capture adapter must also retain execution/input identities and prove
that the actual application state/settings match the requested case.

```sh
python3 tools/qa/screen_compare.py --selftest
python3 tools/qa/screen_compare.py \
  --reference "$APPROVED_REFERENCE" --actual "$ACTUAL_CLIENT_PNG" \
  --policy "$FROZEN_PRIVATE_POLICY" --policy-sha256 "$APPROVED_POLICY_DIGEST" \
  --capture-metadata "$ACTUAL_CAPTURE_METADATA" \
  --output-dir "$NEW_PRIVATE_RESULT_DIRECTORY"
```

Pixel PASS returns 0; FAIL returns 1 with a stable `SCREEN_QA_*` code. Valid
pixel comparisons produce `result.json` and `diff.png`, with input/tool/policy/
output digests and per-region tested/changed counts, peak delta and fraction.
Red pixels in the diff mean an actual RGBA difference. Invalid bindings,
policies or inputs fail before generating a pixel report; preserve the JSON
stdout/error code with the invoking local-QA command log.

Supported input is noninterlaced 8-bit RGB/RGBA PNG with row filters 0–4. CRC,
inflation size and dimensions are bounded/checked; unsupported transparency or
encoding fails closed. Colour-profile chunks must match the approved reference.
No colour conversion, font normalization or automatic tolerance expansion occurs.

## Validation layers and remaining work

- `compare_pixels` validates content/geometry independently on small fixtures.
- `compare_images` adds PNG decoding and colour metadata comparison.
- `validate_capture` checks source-kind/target/image binding independently.
- `evaluate` composes reference binding, capture binding and pixel checks.
- The CLI freezes the policy digest and writes only to a new out-of-tree path.

No protected Git artifacts or evidence-commit topology are defined by this tool;
private baselines are not Git evidence. Do not couple content-negative tests to
repository history. Selftests start from passing synthetic fixtures, mutate one
invariant with an exact error code, and retain a final file/CLI integration.
CTest runs only those synthetic tests, never private reference images.

## Executable X11 capture and reference workflow

`x11_capture.py` now supplies the X11 capture adapter. It starts an explicitly
identified worker with copied private inputs, discovers its visible window by
PID, checks ownership and executable path, and reads geometry/title/fullscreen
state from the X server. It injects actual X11 keyboard events, verifies its
source BASIC readiness marker in the final TVRAM dump, and checks the settings
saved by the application. It suspends the worker while `xwd` reads the mapped
client; RGB/DirectColor LUT decoding produces the comparison PNG. It does not
read guest-only framebuffer pixels or substitute a requested state for observed
fullscreen/renderer state. Title nonduplication is checked through the X11
window property, outside the PNG. There is no emulator-side validator bypass.

Do not add `--headless-input-script` or `--debug-script` to this adapter: the
production frontend forces the dummy video driver for those options. External
input is required for a real X11 window. ROM inputs must resolve inside the
read-only mount; if original symlinks use host-absolute targets, make separately
hashed regular-file copies in task scratch without altering the originals.

`x11_matrix.py` defines a closed 32-case set: SDL software/OpenGL × normal,
320-pixel narrow, fullscreen, 200% GUI font scale × the four status/panel toggle
combinations. Large GUI font scale is **not** a claim of true HiDPI rendering;
these X11 targets use measured 1:1 client/framebuffer dimensions. Footer/menu
rectangles are fixed by the pinned font/layout contract, not inferred from
candidate pixels to hide a changed layout. FPS/FRAME fields are disabled and
the guest cursor is disabled, so these cases require exact, unmasked matches.

Use an existing X11 server/window manager. `x11-qa.dockerfile` defines a minimal
local integration-QA environment; build using the project-local Docker daemon
and stdin as build context (never send private assets as Docker build context).
Bind the repository read-only, scratch read/write and private inputs read-only.
Record image ID and package inventory. Configure QA-only binaries with explicit
`VAEG_ENABLE_LOCAL_MP4_ENCODERS=OFF`; this is not a maintainer personal build.
Recording/native-CRT OFF builds only cover their actual tested profile.

For example, inside the dedicated environment, start independent display
servers for baseline and candidate to avoid focus/event interference:

```sh
Xvfb :132 -screen 0 1280x960x24 -dpi 96 -nolisten tcp > "$SCRATCH/xvfb.log" 2>&1 &
DISPLAY=:132 openbox > "$SCRATCH/openbox.log" 2>&1 &
python3 tools/qa/x11_capture.py --selftest
python3 tools/qa/x11_matrix.py selftest
python3 tools/qa/x11_matrix.py capture \
  --worker "$BASELINE_WORKER" --evaluated-commit "$BASELINE_COMMIT" \
  --roms "$READ_ONLY_ROM_COPIES" --backup-seed "$V1_SEED" \
  --display :132 --display-kind virtual-x11 --output-dir "$NEW_BASELINE_CAPTURE"
python3 tools/qa/x11_matrix.py prepare \
  --capture-dir "$NEW_BASELINE_CAPTURE" --output-dir "$NEW_DRAFT_REFERENCE"
```

Capture the candidate with the same command and its own worker/commit/display/
new output path. `prepare` always creates **DRAFT**, including `index.html`,
reference PNGs and hash-bound policies. Freeze the manifest before comparison:

```sh
python3 tools/qa/x11_matrix.py compare \
  --capture-dir "$CANDIDATE_CAPTURE" --reference-dir "$FROZEN_REFERENCE" \
  --manifest-sha256 "$FROZEN_MANIFEST_DIGEST" --output-dir "$NEW_COMPARISON"
```

A complete match to a draft returns **2 / APPROVAL_REQUIRED**, not PASS. Actual
mismatches return 1 / FAIL. Only a genuinely approved reference manifest with
its approval record and newly frozen digest can yield suite PASS; no command
approves or regenerates a reference automatically. Suite PASS is still not
whole G132 approval. Preserve `matrix.json`, per-case reports/diffs and input/
worker/adapter/output identities. New output paths prevent reference overwrite;
completed unchanged captures can be prepared/compared again without rerunning
the emulator.

An Xvfb/Openbox client is a real SDL X11 window, not dummy output, but its
coverage is **virtual X11 with software/Mesa**, not a physical GPU, macOS Cocoa,
Windows, true HiDPI or the separate native CRT presenter. Those targets remain
separate automation work/environment blockers. The operation runner below adds
normal-size software-X11 tooltip, dynamic-field and CPU-menu checks. These are
not inferred from the static toggle matrix and do not cover other native targets.

## Real operations, serialization and dynamic fields

`x11_gate.py` drives the actual mapped client at normal emulation speed. It uses
production clipboard paste/copy, held XTest keys, menus and real quick-save files;
it does not inject guest state or add emulator hooks. Separate scenarios cover
H/S save/load, VA/VA2 mode/reset transitions, preset/custom clocks, paste cancel
and focus loss, and the source-built M131 probe. `display` checks precise tooltip
clocks/domain and client bounds, measured FPS under a live BASIC redraw stimulus,
advancing FRAME, FRAME opt-in, four live information/panel toggles and restart
persistence. FRAME toggles reset metrics: allow a complete sampling window before
judging the measured rate. A freshly reset zero is not a steady-state FPS failure.

```sh
python3 tools/qa/x11_gate.py --selftest
python3 tools/qa/x11_gate.py \
  --worker "$WORKER" --evaluated-commit "$WORKER_COMMIT" \
  --roms "$ROM_ROOT" --backup-seed "$BASIC_SEED" --display :133 \
  --reference-dir "$APPROVED_REFERENCES" --manifest-sha256 "$MANIFEST_DIGEST" \
  --scenario display --output-dir "$NEW_PRIVATE_RESULT"
```

The Linux ELF64 observer only reads the owned child's production paste queue and
CPU multiplier/speed globals. It checks executable ownership, ELF ABI and closed
symbol names/sizes, and never writes process memory. `PCCORE.multiple` is the
fixed peripheral-time multiplier, **not** the user's CPU multiplier; use the live
CPU globals and rendered effective clock for preset/custom checks. Unsupported
observer ABIs fail closed, not as PASS. Tool/package/worker/input/output identities,
clipboard text, owned-client captures and actual state files remain private.

Content selftests use synthetic state, glyph, lamp and field fixtures. Each
negative starts from a passing fixture and asserts the exact intended error code;
it does not depend on Git topology or private media. A failed later phase does
not make an earlier completed phase fail, but reuse must cite its checkpoint and
preserve the original overall FAIL rather than relabel it PASS.

## Guest-start bank/display regression

`guest_start_qa.py` runs `guest-start-01` in all eight VA/VA2 × V1/V2 × H/S
conditions, using new media/backup copies for each run. Its explicit target is
**dummy guest-frame regression**, not a real-window/presenter check. It requires
successful process exit, different pre/post-start captures, four aligned solid
HUD components and exact matches in two unchanged HUD/banner regions against a
frozen, independently produced pre-M132 positive capture. Animated world/portrait
areas are not compared; this checks entry-scene coherence, not game completion.

```sh
python3 tools/qa/guest_start_qa.py --selftest
python3 tools/qa/guest_start_qa.py \
  --worker "$WORKER" --evaluated-commit "$WORKER_COMMIT" --roms "$ROM_ROOT" \
  --media "$PRIVATE_START_MEDIA" --backup-seeds "$SEED_DIRECTORY" \
  --policy "$FROZEN_PRIVATE_POLICY" --output-dir "$NEW_PRIVATE_RESULT"
```

The closed `private-guest-hud-v1` policy and `reference.png` stay outside Git.
Retain the prior acceptance/reference-origin record independently; a JSON basis
string alone does not attest origin or approval. Never generate the reference
from the current candidate. Synthetic component/policy/one-pixel mutations test
exact failure codes; a composed PNG check tests integration. A preserved historical
bad capture can additionally demonstrate rejection of the actual regression.
Only synthetic selftests run in CTest/hosted CI; private inputs, identities,
policies, raw logs and images must never be staged or uploaded.
