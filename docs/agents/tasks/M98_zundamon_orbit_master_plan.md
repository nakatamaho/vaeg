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
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.
-->

# M98 - Zundamon billboard-orbit demo master plan

Status: **G98a and G98e human gates and G98b-G98d machine gates passed on 2026-08-31; M98f is unassigned**

Branch family: `topic/m98-zundamon-orbit`

Commit prefix: `M98<stage>:`

## 1. Intended result

Build one isolated PC-88VA/VAEG demonstration named **Zundamon orbit** that:

1. accepts an explicitly supplied local 32-bpp BMP and matching 16-entry
   RGB888 palette through a generic, source-neutral interface;
2. converts the accepted image to VA 8-bpp `GGGRRRBB`, reserving byte `00h`
   for transparency;
3. generates exactly 32 deterministic nearest-neighbor scale levels;
4. stores one shared scale atlas in PC-88VA I/O Bank Memory (BMS);
5. lets the SGP read the selected 128 KiB BMS window directly and
   transparently BITBLT frames into double-buffered Graphic 1;
6. displays a 320x200 scene over a static Graphic 0 background;
7. moves camera-facing images around a deterministic 64-phase ellipse;
8. reuses one atlas for 1 through 16 instances, drawn far to near; and
9. offers one animation update per 1 through 8 VBLANKs, with publication
   synchronized to VBLANK.

This is a billboard effect. It does not claim genuine model rotation or
multiple viewing angles.

## 2. Fixed architecture

| Item | M98 decision |
|---|---|
| Logical display | 320x200 |
| Pixel format | VA 8-bpp direct color, `GGGRRRBB` |
| Transparent value | `00h` only |
| Background | Static nonzero Graphic 0 |
| Billboard layer | Transparent Graphic 1 |
| G1 source surface | 320x400, 320-byte pitch |
| G1 page A | SGP `0220000h`, DSA1 `0020000h` |
| G1 page B | SGP `022fa00h`, DSA1 `002fa00h` |
| Transparent copy | SGP BITBLT mode `0105h` |
| Atlas storage | I/O Bank Memory, 128 KiB window at `80000h-9ffffh` |
| BMS default port | `01d0h`, overridable by guest option |
| Scale levels | Exactly 32, smallest through full size |
| Runtime scaling | None |
| Orbit phases | 64 deterministic lookup-table entries |
| Active instances | 1-16; default 1 |
| Draw order | Far to near; instance ID breaks ties |
| Count controls | `/N1`-`/N16`; DOWN decreases, UP increases |
| Rate controls | `/V1`-`/V8`; LEFT faster, RIGHT slower |
| Other controls | SPACE pauses, ESC restores and exits |
| Page clearing | Full-clear baseline, then per-page dirty-row unions |
| Local payload | `ZUNDORB.BIN`, never tracked or distributed |
| Public guest | `ZUNDORB.COM`, source-built and below 64 KiB |

Every implementation stage must recheck the live BMS bank semantics and SGP
source mapping. The CPU must not change the selected BMS bank while the SGP is
busy, and the guest must restore the ordinary mapping on every exit path.

## 3. Public/private boundary

Tracked and distributable material is limited to source, build scripts,
schemas, validators, documentation, and a deterministic synthetic fixture.
The fixture is an abstract asymmetric face-like marker and is not a depiction
of the named demo subject.

Maintainer-supplied images, palettes, manifests, generated atlases, local disk
images, screenshots, traces, and save states stay outside Git. Tracked text
must not record their filenames, absolute paths, hashes, provenance, or other
identifying metadata. Inputs must be provided through explicit paths outside
tracked directories. M98 neither obtains nor identifies source artwork.

The public pipeline is source-neutral. The maintainer is responsible for
supplying inputs they may lawfully use and for deciding whether any resulting
media may be published.

## 4. Repository layout

Introduce paths only in the stage that owns them:

```text
demos/zundamon-orbit/
  README.md
  build.sh
  build-local-d88.sh
  256/
    zundamon_orbit_256.asm
  tools/
    build_zundamon_orbit_asset.py
    inspect_zundamon_orbit_asset.py
    generate_orbit_table.py
    test_zundamon_orbit_asset.py
```

Generated output belongs below `build/generated/zundamon-orbit/`. Private
inputs must not be copied into the source tree. The guest does not use
`incbin` for the local atlas.

## 5. Milestone sequence

Execute exactly one assigned stage and stop at its gate.

| Stage | Single concern | Gate |
|---|---|---|
| M98a | Reserve the generic namespace and privacy boundary | Human |
| M98b | Add the deterministic public fixture and scaffold | Machine |
| M98c | Freeze the generic local-input manifest and schema | Machine |
| M98d | Validate indexed pixels and the 16-entry palette | Machine/local |
| M98e | Approve crop, transparency, and anchor | Human/local |
| M98f | Convert opaque colors to VA `GGGRRRBB` | Machine |
| M98g | Generate exactly 32 deterministic scale levels | Machine |
| M98h | Freeze the atlas format and fail-closed inspector | Machine |
| M98i | Pack complete frames into 128 KiB BMS banks | Machine |
| M98j | Run the complete local host-asset pipeline | Human/local |
| M98k | Bring up the isolated 320x200 8-bpp guest | Human/VAEG |
| M98l | Probe BMS mapping and required capacity | Human/VAEG |
| M98m | Stream and validate the atlas into BMS | Human/VAEG |
| M98n | Prove one direct BMS-to-G1 SGP transfer | Human/VAEG |
| M98o | Add transparent G1 double buffering | Human/VAEG |
| M98p | Visit all 32 scales with a full-clear baseline | Human/VAEG |
| M98q | Add page-local dirty-row clearing | Human/VAEG |
| M98r | Add VBLANK rate selection and telemetry | Human/VAEG |
| M98s | Add a constant-size 64-phase ellipse | Human/VAEG |
| M98t | Couple orbit depth to the 32-level atlas | Human/VAEG |
| M98u | Integrate and tune the approved local image | Human/local |
| M98v | Generate deterministic multi-instance depth order | Machine |
| M98w | Add the multi-instance full-clear baseline | Human/VAEG |
| M98x | Add multi-instance dirty-row unions | Machine + human |
| M98y | Add 1-16 runtime controls and load telemetry | Human/VAEG |
| M98z | Validate the approved local image at multiple counts | Human/local |
| M98aa | Complete negative, deterministic, and performance QA | Machine + human |
| M98ab | Finish documentation and the final gate | Human |

M98f and later are deliberately unassigned. No superseded draft or retired
stage is authority for this plan.

## 6. Deterministic host contracts

Palette conversion uses a declared 16-entry RGB888 input. Source value zero
is transparent. Opaque colors must match a visible entry exactly unless
an explicit diagnostic-only nearest-color option is requested. Conversion to
VA direct color is:

```text
red3   = (red8   * 7 + 127) // 255
green3 = (green8 * 7 + 127) // 255
blue2  = (blue8  * 3 + 127) // 255
va8    = (green3 << 5) | (red3 << 2) | blue2
```

An opaque result that quantizes to zero must be repaired to the nearest
nonzero value with deterministic tie breaking. Scaling uses:

```text
width(i)  = max(1, (source_width  * i + 16) // 32)
height(i) = max(1, (source_height * i + 16) // 32)
pitch(i)  = (width(i) + 3) & ~3
```

for `i=1..32`, deterministic center-sampled nearest-neighbor selection,
zeroed row padding, 16-byte frame alignment, and no frame crossing a 128 KiB
bank boundary.

## 7. Guest correctness contracts

- Load only a small header and descriptor table into conventional memory;
  stream payload through a bounded staging buffer.
- Validate lengths, geometry, bank ranges, padding, and CRCs before graphics
  mode.
- Keep the selected BMS bank stable until every dependent SGP list completes.
- Render a complete hidden page before publishing it on a low-to-high VBLANK
  edge.
- Maintain independent dirty state for both Graphic 1 pages.
- At multiple-instance stages, clear the complete union of the hidden page's
  old row intervals before drawing all new instances far to near.
- A missed animation slot retains the old published page and increments
  telemetry; it never exposes a partial frame.
- Restore video, keyboard, files, staging memory, and ordinary BMS mapping on
  every success and failure exit.

## 8. Final evidence boundary

Public-fixture, local-private, VAEG, and physical-machine claims are reported
separately. Automated fixture success cannot pass a human visual gate. VAEG
timing is not physical-machine throughput evidence. Unless physical testing
is actually performed, the final status remains `REAL_HW_PENDING`.
