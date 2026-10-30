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
# M142 utility-media regression

## Result and identities

**Required utility build, payload and live boot contracts: PASS. G142 NOT_READY.**
This closes the previously missing utility-media subset, not Windows execution,
GUI/FPS/presenter coverage, actual Windows/Linux state exchange or acceptance.
The [earlier demo-media closure](m142_demo_media_regression.md) remains separate.
The extra redirected-LHA result below remains FAIL/unclassified; it is not
silently promoted to success by the passing boot or nonredirected query.

- DOS backend implementation: [3551f034e61c7780845c8117a624c4cf6ae51751](https://github.com/nakatamaho/vaeg/commit/3551f034e61c7780845c8117a624c4cf6ae51751).
- Evaluated media fix/source, checked out into a new clean clone **after push**:
  [826eb848b37720aa6de1c89b1b40e56f296c1179](https://github.com/nakatamaho/vaeg/commit/826eb848b37720aa6de1c89b1b40e56f296c1179).
- Native worker remains source `0d02bc849b5c4fb4f45d21c13716735e77578d37`,
  SHA-256 `24f53861e4a81f0914d489c353921113cc81e0aa1f730f00b9a9b07deea70700`.
  The source comparison has **no changed C/C++/header files**; CMake changes
  concern test registration/scheduling. No unchanged application objects or expensive CPU suites
  were rebuilt/rerun for media tooling and documentation.
- Local DOSBox binary SHA-256:
  `7c6d49fe06292d14507133ad438b9a9591ab31563f4098698730abb37ac619a3`.
  The pinned complete Open Watcom archive is checked before extracting four
  DOS tools. No container, Linux host compiler or system-port change is used.
- Raw evidence stays outside Git in task-local `media-final-r1/` and
  `media-final-r2/`: commands, private bindings, configs, worker/ROM/media
  digests, retained failures, payload maps, captures and final validation.
  Public bindings: SYSTEM_01 = PC-Engine 1.1, SYSTEM_02 = PC-Engine 1.05,
  UTILITY_FDD_01, UTILITY_SASI_VA_01 and UTILITY_SASI_VA2_01 = generated media.
  Original system inputs remain byte-identical. No private originals or
  their names/hashes, raw captures or generated images are committed.

## Commands and generated images

Run in the clean checkout, with child-local PATH including the isolated
DOSBox and NASM; `TMPDIR` is inside the approved work tree:

```sh
export VAEG_OPENWATCOM_BACKEND=dosbox
export VAEG_OPENWATCOM_DOS_ARCHIVE="$OPENWATCOM_ARCHIVE"
export VAEG_CACHE_DIR="$CACHE"
tools/pc88va/build-utility-disk.sh \
  --source "$SYSTEM_01" --output "$UTILITY_FDD_01"
tools/pc88va/build-sasi-utility-disks.sh \
  --source-va "$SYSTEM_02" --source-va2 "$SYSTEM_01" \
  --payload-d88 "$UTILITY_FDD_01" --output-dir "$SASI_OUTPUT" \
  --cpm-tools-d88 "$CPM_TOOLS_01" --cpm-source-d88 "$CPM_SOURCE_01" \
  --cpm-dev-d88 "$CPM_DEV_01"
```

Both final commands **exit 0**. The FDD installs **125 files / 1,088,374 bytes**
in addition to the retained system files; DIET processes **50 executables**
and saves **407,478 bytes**. Both 40 MB SASI builders exit 0. Recursive
readback counts **129 / 632 / 623 stored regular files** for FDD / VA SASI /
VA2 SASI; these include boot/generated files beyond the builders' installed
payload counts. FAT-copy/layout readback and the existing driver identities
pass. Compared with the first source-built media, **CONFIG.SYS is the only
changed file payload on each image**. RDBMS driver/manual bytes are unchanged;
all compressed program payloads remain byte-identical. Whole-image equality
is not claimed: allocation metadata and FAT timestamps are not that contract.

| Generated binding | Bytes | SHA-256 |
|---|---:|---|
| UTILITY_FDD_01 | 1338960 | `aab53f40a0563ea58eca4991e7e22cdb1291cb7b6807086aad78044e9e53a634` |
| UTILITY_SASI_VA_01 | 41568256 | `508e44b577fdb8498af16d65d9427d32207f515aabea453b6b3e2494b289c13a` |
| UTILITY_SASI_VA2_01 | 41568256 | `0a4f37180b9828b2dc8a646ee406fa7ce6551a0b3ffee1f5ea6136c1e69d63ad` |

The first SASI attempt exits 1 with **OFFLINE_MISS: cpm_license**. Preserve
that failure. The existing CP/M installer then runs with a task-local copy
of the cached archives, its pinned notice download, `--image-backend native`,
`--keep-work`, SYSTEM_01 and all four documented `--accept-*-license` flags;
it exits 0. Tools, work and extracted license notices stay in that task-local
cache, not the shared private cache. Three of its six normal D88 outputs are
bound explicitly above. They are reused for the corrected-media build because
`tools/cpmva/` inputs are unchanged between the two evaluated source commits.
The other three outputs are **build-only**, not new CP/M/ZEX execution claims.

| Generated prerequisite binding | Bytes | SHA-256 |
|---|---:|---|
| CPM_TOOLS_01 | 348848 | `1428440a48af8e63067f05626eaacb6a88ced146eb7e2f4b7b30e5462dfe69aa` |
| CPM_SOURCE_01 | 348848 | `8014e2b8ddb0f4097bd79056e22b32a470a3a60f0625788fd861392810e9f326` |
| CPM_DEV_01 | 348848 | `a34b484bbc43107d7910bfb1b5ceb114505dc1ea67e72e4bd9b0c5f8297bed1c` |
| CPM_BOOT_01 | 1338960 | `1bbeb814ef340771e40eb1574643b1ee11a085a94d54bdb7f792e53ea126d709` |
| CPM_TEST_01 | 348848 | `a8080e6141865696f96991ef1ae5cdf6f10e9414aceae74d4be35f0039728b8b` |
| CPM_TEST_2D_01 | 348848 | `80b684392a8c1d7e18b25294b89766b4b0d87fabc71f1475cb8d0da447dcd17d` |

## Live boot and RDBMS correction

All cases use isolated writable copies, private per-case HOME/config/backup
files, `clk_mult=2`, `CPU_Speed=100`, 24.8 kHz, **EMS/BMS absent**, and all
unrelated floppy/SASI/SCSI slots empty. Missing per-case backup files are
initialized by `Main_RAM_Auto=true`; `Boot_FixedDisk=true` is explicit.
The target is **native macOS SDL dummy/software guest integration only**,
not Cocoa/final-client, GPU, fullscreen or HiDPI coverage.

| Case | Attached media | Result |
|---|---|---|
| FDD / VA | UTILITY_FDD_01 alone | PASS_BOOT_CONTRACT |
| FDD / VA2 | UTILITY_FDD_01 alone | PASS_BOOT_CONTRACT |
| SASI / VA | UTILITY_SASI_VA_01 alone; no FDD | PASS_BOOT_CONTRACT |
| SASI / VA2 | UTILITY_SASI_VA2_01 alone; no FDD | PASS_BOOT_CONTRACT |

Each final worker exits 0 **and** the unchanged
[`classify_boot`](../../../tools/qa/all_demos_draw.py) contract verifies the
live native TVRAM directory, expected model-media filesystem content and
final Ready. No process exit or screenshot alone counts as PASS.

Before correction, VA2 returns Ready but types only `R` from `DIR`. Increasing
startup delay, changing character case, spacing text, holding keys, disabling
backup persistence, installing EMS, removing MSE's BMS switches or removing
MSE itself do not fix it. Enabling BMS alone does. Removing **only RDBMS
autoload**, with all other file bytes identical, passes with BMS absent.
This establishes the unsafe unconditional guest load policy, not a physical
keyboard/CPU timing cause or a specific internal driver corruption address.
The production fix retains RDBMS files but requires explicit bank-memory
installation before opt-in. See the [permanent ledger](../../modernization/bug-fixes.md#m142--utility-media-autoloaded-a-bank-ram-disk-without-a-bms-board).

The nine initial/control batches include 17 workers, with semantic failures
preserved. One split-text attempt incorrectly used a blank line rather than
`@enter`; one diagnostic adapter first tried the vanilla-only installer on
a full disk and was corrected to the existing file-replacement API. Neither
caller error is hidden or counted as a separate emulator cause.

## Packed-program smoke and limits

One final FDD/VA2 worker successfully executes the packed LHA from the current
archive directory **without redirection**, lists `TFD.SYS` with expected
original size **2072**, and returns Ready. A separate retained DIET query
produces its **1.44** usage output and returns to the directory/prompt.
These are two executable smoke checks, **not execution of all 50 programs**,
not verification of every utility, and not BMS-dependent feature coverage
when the board is absent.

Two earlier redirected LHA queries (absolute and relative archive operands)
return `no file` rather than the required members, although their boot/return
contract passes. Those **program-result contracts remain FAIL/unclassified**;
nonredirected success does not repair or relabel them. No cause or emulator
fix is established for that OS/tool redirection interaction.

Across initial/control, final boot and packed-smoke batches there are **24
actual guest workers**, all process exits 0, with distinct semantic failures
and passing results retained. Final evidence revalidation exits 0 with **no
additional guest workers**. Local backend tests: 7 PASS; utility-config tests:
2 PASS covering eight SASI combinations and the FDD's retained payload policy;
Softlib tests: 3 PASS / 1 SKIP (private integration inputs not bound there).
Repository checks pass. No hosted CI, Windows/application rebuild, human-gate
request, acceptance, merge or release is implied.

## Remaining M142 gate obligations

The existing Windows artifact remains source
`ea62eda1eb2d1d3abed11f72af2e5e7899868652`, SHA-256
`d9c3bbdc36d88e5cc23a3587f2cbe7870594af2c55634dfe97e4e6f32b2b132a`;
its application C/C++/headers also match the evaluated media source. It is
not rebuilt or relabeled for host-media changes. A cross-build is still not
Windows execution.

Actual GUI settings/persistence, FPS/width/overclock and presenter checks,
the guest-visible main/FDC/peripheral clock matrix, genuine legacy states
and actual Windows/Linux state exchange remain uncompleted or blocked by
the unavailable execution/display targets. SDL dummy and native same-host
state tests do not satisfy those contracts. The extra redirected-tool
result above also remains unclassified. Issue #75's **investigation is
accepted; cause/correction are deferred**, not an M142 correction prerequisite.
No new hardware photographs or #75 reruns are requested. **G142 NOT_READY.**
