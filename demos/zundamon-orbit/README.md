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
