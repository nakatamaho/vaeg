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
