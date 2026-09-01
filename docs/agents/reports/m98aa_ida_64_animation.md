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

# M98aa IDA 64-instance animation

Status: **G98aa: human gate pending**

`REAL_HW_PENDING` remains in force. This milestone is a private local
candidate extension and does not make private pixels, manifests, hashes,
paths, binaries, or disk images distributable.

## Authority and scope

- Starting accepted G98z head: `8d0ce6c5696661e7aba66ca5e36bf75760660d71`.
- Branch: `topic/m98aa-ida-64-animation`.
- Implementation commit: `aa910563b56cadd647c2058758e37a3d1f50cf71`.
- Predecessor: G98z was explicitly passed by the maintainer and its accepted
  head is the starting point.
- Scope: private IDA profile only; public ZUNDAMON behavior is unchanged.
- The later 128-instance extension is not included.

The unrelated pre-existing worktree state was preserved exactly:

```text
 M docs/modernization/bug-fixes.md
 M docs/modernization/pc88va-archive-binary-extraction.md
 M tools/pc88va/build-development-disk.sh
 M tools/pc88va/build-softlib-archive-disk.sh
 M tools/pc88va/stage-development-tools.sh
?? .dosbox-colima-bin/
?? docs/98io/
?? docs/agents/reports/m97f_bms_selected_port.md
?? docs/cpmva/
?? docs/disks/
?? docs/neon/
?? docs/roms/
?? docs/tekumani/
?? tools/pc88va/softlib-fdd-manifest.tsv
?? va2bkupmem.dat
?? vabkupmem.dat
```

## Implementation

The private build selects a capacity of 64 rectangles, draw indices, dirty
interval candidates, and merged intervals. Page-local rectangle offsets and
instance-ID storage use capacity-derived shifts. A four-word (64-bit) seen
mask validates every generated draw permutation (including counts below 64);
the public path retains its original 16-bit code and binary identity. Record
storage remains the accepted 50-byte ABI and uses one shared external atlas.

The private parser accepts exactly `/N1` through `/N64`, rejects malformed and
duplicate tokens, and keeps the existing `/V1` through `/V8` grammar. UP/DOWN
continue from requested state and saturate without wrapping. The private HUD
generator/validator supports 64 fixed two-cell count tiles; the public default
remains 16. Private speed status uses thirteen 0.25X steps from 1.00X through
4.00X, independently of the FPS divisor.

When enabled by the private build, a bounded VBLANK-driven camera demonstrator
steps speed and distance once per nominal 60-edge interval in a deterministic
triangle. Requests still apply only at complete transaction boundaries; no
clear/draw batch is relabelled in flight. A/Z and Q/E remain available for
manual changes. No renderer fork, runtime scaling, extra atlas bank, or
full-page steady-state clear was added.

## Neutral verification

The accepted public guest was rebuilt twice from the unchanged public path:

```text
M98X_GUEST_BUILD_PASS size=52656 runtime_counts=1..16
public guest SHA-256: e0f2111e4da5d0723633f6ac11658fad49f825c2bc6e6346578ac72ff67aa93f
```

The private candidate was built twice from the previously validated local
IDA atlas and 64-count HUD profile. Both builds were 60,848 bytes and
byte-identical. The private output remains untracked. The neutral private
checks passed:

```text
M98H_ATLAS_PASS
M98T_DEPTH_TABLE_VALIDATION_PASS
M98X_HUD_VALIDATION_PASS count_tiles=64
M98AA_PRIVATE_GUEST_BUILD_PASS runtime_counts=1..64
```

The bootable private candidate contains the external atlas and the 60,848-byte
guest; it is available only through the local private build workflow.

Focused host and shell checks passed:

```text
python3 demos/zundamon-orbit/tools/test_m98aa_ida64.py       4 tests OK
python3 demos/zundamon-orbit/tools/test_m98z_orbit_controls.py 15 tests OK
python3 demos/zundamon-orbit/tools/test_m98y_private_profile.py 3 tests OK
sh -n demos/zundamon-orbit/256/build.sh demos/zundamon-orbit/build-local-d88.sh
```

The public binary compares byte-for-byte with the accepted M98z public
identity. The new neutral host test covers all 64 parser values, invalid
forms, 64 unique phase offsets, the 1.00X..4.00X ladder, and source-level
capacity/auto-camera guards. Detailed private framebuffer and timing evidence
stays outside Git; VAEG/VA2 timing is diagnostic only.

## Human gate

The maintainer must inspect the local private IDA64 candidate at counts 1, 4,
16, and 64; verify continuous motion, deterministic speed/distance animation,
no stale or partial page, correct HUD and controls, both page parities, pause,
and ESC restoration. Until an explicit `G98aa passed` statement is recorded,
the milestone remains **G98aa: human gate pending**.
