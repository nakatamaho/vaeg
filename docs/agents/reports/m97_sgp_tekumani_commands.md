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

# M97 - SGP Technical Manual command completion report

Evaluated baseline: `79ce89af64958cd85cdffa030890fb24a2af8148`

Status: **candidate published; G97 pending**

## 1. Rejected QA milestone removal

The unmerged `topic/m97-deterministic-qa` branch contained only the rejected
M97/M98 QA foundation after the evaluated `main` baseline. The replacement
branch was recreated from the baseline. No QA source, generated D88, fake
BIOS, capture frontend, guest injection, task, or report from that branch is
part of this candidate.

Maintainer-local untracked references and private media were not removed or
modified.

## 2. Manual-derived implementation matrix

| Area | Manual-derived behavior | M97 action | Hardware status |
|---|---|---|---|
| Command address | Word writes at `0500h` and `0502h`, even address | Preserve | Documented |
| Start/status | Start and BUSY at `0506h` | Preserve | Documented |
| Abort/IRQ | Control at `0504h`, IRQ at END | Preserve; timing deferred | Functional documented; ordering unresolved |
| SET WORK | Even address, stable writable 58-byte area | Preserve address only | Internal layout unresolved |
| Descriptors | Start dot, mode, 12-bit dimensions, aligned pitch/address | Correct original-VA profile | Documented for original VA |
| ROP | Sixteen Boolean functions | Verify current table | Documented |
| `TP=2` | Transfer only where destination pixel is zero | Verify current final-mask path | Documented |
| PATBLT | Repeat source in two dimensions | Preserve and regress | Documented normal case |
| LINE | `VD=0800h`, `HD=0400h` | Correct masks | Documented; raster tie rules unresolved |
| CLS | Fill a contiguous word count | Preserve | Documented normal case |
| SCAN RIGHT | Search boundary color and update width | Implement | Documented normal case |
| SCAN LEFT | Search boundary color and update left edge/width | Implement | Documented normal case |
| Thirteenth command | Manual says thirteen but names twelve | No implementation | Unresolved |
| Timing/contention | No recovered command-cycle table | No change | Hardware pending |

## 3. Evidence corrections

Direct reading of the Technical Manual resolves two stale conclusions in the
existing reconstruction:

- the documented ROP order matches the current VAEG implementation;
- SCAN always searches for SET COLOR and documents first-pixel, found, and
  not-found results; it does not expose an undocumented equality selector.

LINE direction bits also use the same `VD` and `HD` positions as BITBLT and
PATBLT. Exact discrete-line tie breaking remains unresolved.

## 4. Implementation

### 4.1 Descriptor and LINE decoding

`fetch_block()` now selects the descriptor profile from `pccore.model_va`.
The original VA profile masks width and height to 12 bits and framebuffer
pitch to a four-byte boundary, as documented by the Technical Manual block
diagrams. The existing VA2 profile retains its 14-bit width, 16-bit height,
two-byte pitch alignment, and observed R-TYPE compatibility adjustment.

LINE no longer has a separate swapped direction mapping. Its aliases now use
the documented common values `VD=0800h` and `HD=0400h`.

### 4.2 SCAN state machine

`SCAN RIGHT` and `SCAN LEFT` execute one pixel at a time through the existing
asynchronous SGP state machine. They reuse the saved destination runtime
fields, so `_SGP` and the binary `SGP` save-state section did not change.

Each step extracts the selected packed pixel and its corresponding packed SET
COLOR field. A match updates the documented output fields. A miss leaves the
input address, dot, and width unchanged. `SCAN LEFT` advances one pixel right
from the boundary color before recording the scanned region's left edge, as
shown by the manual diagram.

The implementation subtracts one internal scheduler quantum per scan pixel
only to make state-machine progress finite. This is not a recovered hardware
cycle count, and M97 makes no SGP timing or contention claim.

### 4.3 Focused regression coverage

The compiled selftest now verifies:

- original-VA and VA2 descriptor decoding separately;
- the common LINE direction masks;
- all sixteen documented Boolean ROP values;
- `TP=2` destination-zero masking;
- SCAN RIGHT first-pixel, later-pixel, miss, and packed-word-boundary cases;
- SCAN LEFT first-pixel, later-pixel, and miss cases, including returned left
  address/dot and width.

## 5. Validation

| Check | Result |
|---|---|
| `git diff --check` | PASS |
| UTF-8 / EOL / path-case validators | PASS, zero findings |
| Targeted clang-format 22 check for changed C files | PASS |
| Repository-wide clang-format check | PRE-EXISTING FAIL in unchanged `sdl2/np2.c` and `sdl2/scrnmng.c` |
| Unreferenced-source report | Completed; 40 pre-existing candidates, none added by M97 |
| Linux debug configure/build | PASS |
| `build/linux-debug/sdl2/vaeg --selftest` | PASS, including `SGP manual commands ok` |
| CTest | 83 PASS, 1 external-fixture SKIP, 0 FAIL |
| MinGW cross release build | PASS, PE32+ x86-64 GUI executable |

The task's initial `ctest --preset linux-debug` spelling was corrected because
the repository has configure/build presets but no CTest preset. The executed
command was `ctest --test-dir build/linux-debug --output-on-failure`.

MinGW artifact:

```text
build/mingw-cross/sdl2/vaeg.exe
SHA-256 ec8a7fdd05008540aafab3fca5a119ed39eff88e159bba46c4847c9792438b3f
```

No ROM, disk, font, icon, cursor, wave, or maintainer-local reference file was
modified. No real-hardware test was performed or claimed.

## 6. Commits

| Stage | Commit |
|---|---|
| M97 definition | `eafcb77c2b47459a7e044d6074d454b01b07f82a` |
| M97a evidence correction | `da9981cc7bde84057c76a5d87081e4955dfbb8b8` |
| M97b descriptor/LINE/ROP/TP2 | `ffb85210c62f984108ad9d022f7d046107744f60` |
| M97c SCAN implementation | `7b788edc6e657f2d9e8e48f759c5cab6eb7c4899` |

## 7. Remaining unknowns

- the unnamed thirteenth command;
- the internal 58-byte SET WORK format;
- zero extents and other explicitly undefined descriptor cases;
- LINE tie-breaking details not stated by the recovered diagram;
- SCAN command timing, SGP/CPU bus arbitration, and status timing;
- reserved `TP=3` behavior;
- real-PC-88VA equivalence.

## 8. Human gate

G97 is pending. It is a VAEG visual-regression gate and does not require or
claim a real-hardware run.
