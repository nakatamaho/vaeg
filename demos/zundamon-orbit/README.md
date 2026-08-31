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

## Thirty-two scale levels

M98g generates exactly 32 nearest-neighbor frames, numbered 1 through 32.
Their dimensions and row pitch are:

```text
width(i)  = max(1, (source_width  * i + 16) // 32)
height(i) = max(1, (source_height * i + 16) // 32)
pitch(i)  = (width(i) + 3) & ~3
```

Every target pixel samples the source pixel containing its projected center.
Rows have zero padding through the four-byte pitch, and every frame starts at
a 16-byte boundary. All 32 descriptors are retained even when small adjacent
levels have the same dimensions. The anchor is projected with the same
pixel-center convention.

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
64-byte header is followed by 32 fixed 32-byte descriptors and a payload
region beginning at byte 1088. The header declares one pose, 32 scales,
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

## Minimal BMS bank packing

M98i places all 32 frames in increasing scale order into the minimum
deterministic sequence of 128-KiB logical BMS banks. Every frame begins at a
16-byte-aligned bank offset and remains entirely within one bank. When the
next complete frame cannot fit, the remaining bank tail is left unused and
the frame starts at offset zero in the next logical bank. Frames are never
split, reordered, or backfilled.

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
alignment, logical bank-boundary padding, per-bank payload and occupied
bytes, compact payload size, and required bank count. These values are not
printed by the CLI.

Run the exact-fit, overflow, multi-bank, deterministic, corruption, metrics,
and privacy-output tests:

```sh
PYTHONPYCACHEPREFIX=/tmp/vaeg-m98i-pyc \
  python3 demos/zundamon-orbit/tools/test_zundamon_orbit_packing.py
```
