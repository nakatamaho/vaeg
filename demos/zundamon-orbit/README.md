<!--
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
-->

# Zundamon orbit demonstration

M98b provides a deterministic public fixture for the future Zundamon
billboard-orbit demonstration. The fixture is an abstract asymmetric marker,
not a depiction of the named subject. It contains transparency, diagonals,
isolated one-pixel features, and an opaque near-black color so later host
tools can be tested without maintainer-supplied inputs.

The fixture consists of a 23x19 indexed raster, a source-neutral 16-entry
RGB888 palette, and a canonical JSON manifest. Palette index 0 is transparent
and index 15 is reserved. Generated files are written below
`build/generated/zundamon-orbit/` by default and are not tracked.

Build a new fixture directory and inspect it:

```sh
demos/zundamon-orbit/build.sh
```

Pass a different, nonexistent output directory to preserve an earlier result:

```sh
demos/zundamon-orbit/build.sh /tmp/zundamon-orbit-fixture
```

Run the deterministic generation and negative-inspection tests:

```sh
PYTHONPYCACHEPREFIX=/tmp/vaeg-pyc \
  python3 demos/zundamon-orbit/tools/test_zundamon_orbit_asset.py
```

M98b builds no guest code and consumes no maintainer-supplied input. M98c and
later remain separately gated.

## Local input manifest

M98c defines a source-neutral local bundle consisting of one version-1
manifest and two sibling files named by lowercase basenames:

- a 32-bpp BMP declared as `bmp32`;
- a 16-entry, 48-byte RGB888 palette declared as `rgb888`.

The manifest also fixes a source-image crop, an exact RGB transparency color,
and an integer anchor relative to the crop's top-left corner. It deliberately
has no free-form notes, origin, source identifier, or hash field. Absolute
paths, directory separators, parent references, missing or unknown members,
duplicate JSON keys, and out-of-range values are rejected.

Validate the neutral public example:

```sh
python3 demos/zundamon-orbit/tools/validate_zundamon_orbit_manifest.py \
  --input demos/zundamon-orbit/examples/input-manifest-v1.json
```

Run the schema, focused negative, privacy-output, and CLI integration tests:

```sh
PYTHONPYCACHEPREFIX=/tmp/vaeg-m98c-pyc \
  python3 demos/zundamon-orbit/tools/test_zundamon_orbit_manifest.py
```

The M98c validator checks only the manifest. It does not open the referenced
BMP or palette and never prints their names or the manifest path. File-content
validation belongs to M98d.

## Image and palette validation

M98d fixes `bmp32` to an uncompressed 32-bpp BMP with a 54-byte header and
BGRA pixel storage. Both bottom-up and top-down row order are accepted. The
RGB888 palette contains exactly 16 entries: indices 0 and 15 equal the exact
background color, while visible indices 1-14 are unique.

The content inspector checks the actual BMP dimensions, crop bounds, palette,
and every crop pixel. Background pixels recover as index 0; every opaque pixel
must exactly match one visible palette entry. It rejects nearest-color input,
an all-transparent crop, and a crop without transparency.

Build and inspect a complete synthetic bundle:

```sh
output_root=$(mktemp -d /tmp/vaeg-m98d.XXXXXX)
python3 demos/zundamon-orbit/tools/build_zundamon_orbit_input_fixture.py \
  --output "$output_root/input"
python3 demos/zundamon-orbit/tools/inspect_zundamon_orbit_input.py \
  --manifest "$output_root/input/input.json"
```

Run the deterministic, negative, row-order, and privacy-output tests:

```sh
PYTHONPYCACHEPREFIX=/tmp/vaeg-m98d-pyc \
  python3 demos/zundamon-orbit/tools/test_zundamon_orbit_input.py
```

For a separately prepared local bundle, pass its manifest through the same
inspector. Success and failure output deliberately omits paths, filenames,
dimensions, palette values, pixel counts, and hashes. M98d does not write the
recovered private indices to disk.

## Crop and anchor preview

M98e builds three deterministic review images from a bundle that has passed
the M98d inspector: a full-source crop/anchor overlay, an unmarked crop, and a
crop-relative anchor overlay. It reads the source bundle without modifying it
and refuses to reuse an existing output directory.

Generate a private review directory with an explicit local manifest:

```sh
python3 demos/zundamon-orbit/tools/build_zundamon_orbit_crop_preview.py \
  --manifest /path/to/private/input.json \
  --output build/generated/zundamon-orbit/private-gate/m98e-preview
```

The optional `--scale` value is an integer from 1 through 8 and affects only
the two crop previews. Generated previews stay below the ignored
`build/generated/zundamon-orbit/` tree and must not be committed.

Run the deterministic pixel, overlay, immutability, overwrite, and
privacy-output tests:

```sh
PYTHONPYCACHEPREFIX=/tmp/vaeg-m98e-pyc \
  python3 demos/zundamon-orbit/tools/test_zundamon_orbit_crop_preview.py
```

Tool success only means that the review images were generated. Crop,
transparency, and anchor approval remains the maintainer-only G98e human gate.

## VA 8-bpp conversion

M98f converts a validated crop to one byte per pixel in VA direct-color
`GGGRRRBB` order. Source index 0 remains byte `00h` for transparency. Visible
RGB888 colors use deterministic nearest-integer 3:3:2 quantization without
dithering.

If an opaque source color would quantize to `00h`, the converter searches all
255 nonzero VA bytes. It minimizes squared RGB error after expanding each
candidate back to 8-bit channel values; equal distances select the lower byte.
This keeps `00h` exclusive to transparency.

Convert an explicitly supplied local bundle into a new ignored directory:

```sh
python3 demos/zundamon-orbit/tools/convert_zundamon_orbit_va8.py \
  --manifest /path/to/private/input.json \
  --output build/generated/zundamon-orbit/private-gate/m98f-va8
```

The directory contains top-to-bottom, left-to-right raw `pixels.va8` and a
private `report.json`. The report records per-palette conversion error,
opaque-zero repairs, and collisions for local inspection. Neither output is
distributable by the public fixture workflow.

Run the bit-layout, rounding, repair, collision, reproducibility, and
privacy-output tests:

```sh
PYTHONPYCACHEPREFIX=/tmp/vaeg-m98f-pyc \
  python3 demos/zundamon-orbit/tools/test_zundamon_orbit_va8.py
```

## Thirty scale levels

M98g generates exactly 30 nearest-neighbor frames, numbered 1 through 30.
Their dimensions and row pitch are:

```text
numerator(i) = i for i=1..29, and 31 for i=30
width(i)     = max(1, (source_width  * numerator(i) + 15) // 31)
height(i)    = max(1, (source_height * numerator(i) + 15) // 31)
pitch(i)  = (width(i) + 3) & ~3
```

Every target pixel samples the source pixel containing its projected center.
Rows have zero padding through the four-byte pitch, and every frame starts at
a 16-byte boundary. All 30 descriptors are retained even when small adjacent
levels have the same dimensions. The anchor is projected with the same
pixel-center convention. Level 30 is the exact full-size source. The omitted
30/31 slot leaves the complete 30-level 98x128 atlas within one bank without
shrinking the maximum frame. With four-byte row pitch and 16-byte frame
alignment, that maximum-bound atlas occupies 127456 of 131072 bytes.

Generate an ignored intermediate scale set directly from a validated bundle:

```sh
python3 demos/zundamon-orbit/tools/generate_zundamon_orbit_scales.py \
  --manifest /path/to/private/input.json \
  --output build/generated/zundamon-orbit/private-gate/m98g-scales
```

The output contains `scales.va8` and a private `report.json`. This is not the
final atlas format: it has no atlas header, CRC, BMS bank number, or bank
packing. Those contracts belong to later milestones.

Run the dimension, sampling, anchor, padding, alignment, reproducibility, and
privacy-output tests:

```sh
PYTHONPYCACHEPREFIX=/tmp/vaeg-m98g-pyc \
  python3 demos/zundamon-orbit/tools/test_zundamon_orbit_scales.py
```

## Version-1 atlas format

M98h freezes the little-endian `ZUNDORB.BIN` version-1 container. Its fixed
64-byte header is followed by 30 fixed 32-byte descriptors and a payload
region beginning at byte 1024. The header declares one pose, 30 scales,
128-KiB BMS banks, required bank count, an explicit first guest bank selector,
payload bounds, complete file size, and payload/file CRC32 values.

Each descriptor records dimensions, pitch, projected anchor, logical bank
slot, offset within the bank, absolute file offset, payload length, flags, and
frame CRC32. Guest bank selector values are derived as
`first_bank_value + logical_bank_slot`; selector zero remains the ordinary
memory mapping.

Build and inspect the public format fixture:

```sh
fixture_root=$(mktemp -d /tmp/vaeg-m98h.XXXXXX)
python3 demos/zundamon-orbit/tools/build_zundamon_orbit_atlas_fixture.py \
  --output "$fixture_root/atlas.bin"
python3 demos/zundamon-orbit/tools/inspect_zundamon_orbit_atlas.py \
  --input "$fixture_root/atlas.bin"
```

The public fixture assigns one frame to each logical bank solely to exercise
the format without implementing M98i packing. It is not a private or
distribution atlas. The inspector independently rejects malformed geometry,
flags, offsets, bank crossing or overlap, nonzero padding, noncanonical scale
or anchor geometry, and bad frame, payload, or file CRCs.

Run the deterministic writer/inspector and focused negative tests:

```sh
PYTHONPYCACHEPREFIX=/tmp/vaeg-m98h-pyc \
  python3 demos/zundamon-orbit/tools/test_zundamon_orbit_atlas.py
```

## Single-bank BMS packing

M98i places all 30 frames in increasing scale order into exactly one 128-KiB
logical BMS bank. Every frame begins at a 16-byte-aligned bank offset. The
production packer rejects an atlas that would require a second bank; frames
are never split, reordered, or backfilled.

The compact atlas file does not serialize unused bank tails. It retains the
M98h canonical file layout and records each logical bank slot and bank offset
in the descriptor table. The M98h one-frame-per-bank fixture remains a valid
format test; the stricter M98i validator deliberately rejects it as
nonminimal production packing.

Build and inspect the minimally packed public fixture:

```sh
packing_root=$(mktemp -d /tmp/vaeg-m98i.XXXXXX)
python3 demos/zundamon-orbit/tools/pack_zundamon_orbit_atlas.py \
  --fixture-output "$packing_root/packed"
python3 demos/zundamon-orbit/tools/pack_zundamon_orbit_atlas.py \
  --inspect "$packing_root/packed/zundorb.bin"
```

The generated `packing-report.json` reconciles useful pixels, row and frame
alignment, per-bank payload and occupied bytes, compact payload size, and the
required bank count of one. These values are not
printed by the CLI.

Run the exact-fit, overflow, multi-bank rejection, deterministic, corruption,
metrics, and privacy-output tests:

```sh
PYTHONPYCACHEPREFIX=/tmp/vaeg-m98i-pyc \
  python3 demos/zundamon-orbit/tools/test_zundamon_orbit_packing.py
```

## Complete host-asset pipeline

M98j connects manifest validation, exact indexed-pixel recovery, VA8
conversion, downscale-only source normalization, all 30 scale levels,
single-bank BMS packing, and both final atlas inspectors without writing
intermediate private pixels or scale streams. The normalized source fits
within the 98x128 bound while preserving its aspect ratio; a 98x128 source is
retained exactly as the maximum frame. Selection is deterministic
center-sampled nearest-neighbor, the anchor uses the same coordinate rule,
and the pipeline never upscales. An input that already fits remains
byte-for-byte unchanged. The fixed 30-level schedule keeps every permitted
normalized atlas within one 128-KiB bank. A bounded contact sheet displays
every level in order over a checkerboard,
marks the projected anchor, and labels the level, dimensions, and anchor.

Run the complete public fixture through the production pipeline:

```sh
pipeline_root=$(mktemp -d /tmp/vaeg-m98j.XXXXXX)
python3 demos/zundamon-orbit/tools/build_zundamon_orbit_pipeline.py \
  --fixture-output "$pipeline_root/output"
```

Run an explicitly supplied local bundle into the ignored generated tree:

```sh
python3 demos/zundamon-orbit/tools/build_zundamon_orbit_pipeline.py \
  --manifest /path/to/local/input.json \
  --output build/generated/zundamon-orbit/private-gate/m98j-pipeline
```

A successful output directory contains only `zundorb.bin`,
`contact-sheet.bmp`, and `pipeline-report.json`. Local success establishes
`LOCAL_HOST_PIPELINE_READY`, not human approval. Inspect contact-sheet levels
1, 8, 15, 23, 29, and 30 before passing G98j.

After normalization, the complete atlas occupies exactly one BMS bank. The
pipeline never splits or reorders a frame.

Run the public end-to-end, contact-sheet, report, overwrite, and privacy tests:

```sh
PYTHONPYCACHEPREFIX=/tmp/vaeg-m98j-pyc \
  python3 demos/zundamon-orbit/tools/test_zundamon_orbit_pipeline.py
```

## Bootable 320x200 8-bpp baseline

M98k is the first guest-side milestone. `ZUNDORB.COM` selects the established
320x200 G0/G1 direct-color configuration, fills G0 with a nonzero checkerboard,
and submits one bounded SGP command list. That single list clears G1 and uses
transparent BITBLT mode `0105h` to place one embedded 16x16 synthetic marker
at `(152, 92)`. The marker uses a 16-byte row stride with no padding and three
nonzero `GGGRRRBB` values. The settled guest does not redraw the marker.

M98k does not load an atlas or any external asset and contains no BMS, EMS,
XMS, scaling, animation, or multi-instance path. Those concerns remain gated
to later milestones. Its exact historical commands and artifact identities are
recorded in `docs/agents/reports/m98k_zundamon_guest_bringup.md`; the current
guest and runner advance that baseline to M98l.

## One-bank BMS stream and direct G1 transfer

M98l installs the public fixture atlas as `ZUNDORB.BIN`, validates its fixed
1024-byte metadata region, and streams its 4888-byte payload into BMS selector
1 through one 4096-byte staging buffer. Selector 0 remains the ordinary RAM
mapping. The guest checks selectors 1, 2, and 128 independently and verifies
that invalid selector 129 is an open-bus mapping rather than an alias.

After the file and BMS-resident CRC32 values agree, the guest overwrites the
staging buffer with `a5h`. It then submits one bounded SGP command list. The
list clears G1 and performs exactly one transparent `0105h` BITBLT directly
from the selected BMS window. For the public fixture, level 30 is 23x19 with a
24-byte source pitch at bank offset `1150h`; its SGP source is therefore
`081150h`, centered at `(148, 90)` on G1. The completed frame remains static.

Build the current guest and its NASM listing into an existing output
directory:

```sh
NASM=/opt/local/bin/nasm \
  demos/zundamon-orbit/256/build.sh \
  /tmp/ZUNDORB.COM /tmp/ZUNDORB.LST
```

Build the public atlas, then make a local bootable disk from an explicitly
supplied PC-Engine 2HD template. The template and atlas are read-only inputs,
the output must not exist, and generated media remains untracked:

```sh
atlas_root=$(mktemp -d /tmp/vaeg-m98l-atlas.XXXXXX)
python3 demos/zundamon-orbit/tools/build_zundamon_orbit_pipeline.py \
  --fixture-output "$atlas_root"
demos/zundamon-orbit/build-local-d88.sh \
  /path/to/local-bootable-2hd.d88 \
  "$atlas_root/zundorb.bin" \
  /tmp/zundamon-orbit-m98l.d88
```

Run the complete bounded proof in VA2 mode. The output directory must not
exist. The runner regenerates the public atlas, builds the COM and listing,
builds and lists the D88, boots a disposable copy, enables the generic SGP
descriptor trace, captures exact GVRAM, and runs the oracle:

```sh
VAEG_ZUNDAMON_MODEL=va2 demos/zundamon-orbit/run-vaeg.sh \
  /path/to/local-bootable-2hd.d88 \
  /path/to/vaeg \
  /path/to/rom-directory \
  build/generated/zundamon-orbit/m98l-run
```

The runner uses dummy SDL only as transport. PASS is based on BMS phase
signatures, the direct SGP source/destination trace, exact indexed G0/G1
contents, consecutive stable captures, and a nonblack composed frame. Run its
focused fail-closed tests separately with:

```sh
PYTHONPYCACHEPREFIX=/tmp/vaeg-m98l-pyc \
  python3 demos/zundamon-orbit/tools/test_zundamon_orbit_guest.py
```

M98l does not traverse scale levels at runtime, animate, exchange pages, draw
multiple objects, measure performance, load maintainer-supplied artwork, or
claim physical-machine validation. Those concerns remain separately gated.
