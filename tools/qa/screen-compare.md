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

The remaining G132 work is the trusted real-client capture adapter and private
reference/case matrix. The OS title bar is outside final-client pixels: title
nonduplication needs a window-title API assertion or a separately bound
whole-window capture. Tooltip cases need deterministic pointer placement and
timing; CPU-speed menus need controlled UI actions and observed state. These
are automation tasks, not evidence supplied by the comparator alone.
