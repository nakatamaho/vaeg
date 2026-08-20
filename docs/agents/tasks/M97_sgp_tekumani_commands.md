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

# M97 - Complete documented SGP command semantics

Status: **candidate published; G97 pending**

Branch: `topic/m97-sgp-tekumani`

Commit prefix: `M97:` or `M97<stage>:`

## Goal

Implement only PC-88VA SGP behavior that can be derived from the local
PC-88VA Technical Manual without real-hardware timing assumptions. Correct
the current LINE direction decoding, implement the documented SCAN commands,
and add deterministic functional regression coverage.

The rejected deterministic-QA/fake-BIOS design is not part of M97. M97 does
not add a ROM-less launcher, synthetic FDD BIOS, audiovisual recorder, golden
corpus, or non-free integration layer.

## Authority

1. Local PC-88VA Technical Manual, display-system SGP chapter
   (`docs/tekumani/4.TXT`). This reference is maintainer-local and is cited
   only in this tracked report/task, never from source comments.
2. Current VAEG SGP source and existing VA software command streams.
3. MAME only as comparison evidence; it is not a hardware oracle.

## In scope

- Preserve the word-oriented `0500h`/`0502h` command-address interface.
- Preserve END, NOP, SET WORK, SET SOURCE, SET DESTINATION, SET COLOR,
  BITBLT, PATBLT, LINE, and CLS where they already match the manual.
- Decode original-VA block width, height, and framebuffer width from the
  documented 12-bit/12-bit/word-aligned fields while retaining the existing
  later-model profile separately.
- Correct LINE `VD=0800h` and `HD=0400h`.
- Implement `SCAN RIGHT` and `SCAN LEFT` normal functional semantics:
  comparison with SET COLOR, maximum count from destination width, zero
  result when the first pixel matches, unchanged descriptor on no match,
  and the documented destination updates on a match.
- Verify all sixteen documented ROP values and destination-zero transfer
  mode through focused selftests.
- Correct stale SGP documentation where direct manual text resolves it.

## Out of scope

- Inventing the unnamed thirteenth command.
- Inventing the internal format or write pattern of the 58-byte work area.
- Defining power-on drawing state or behavior before SET WORK.
- SGP cycle accuracy, bus contention, arbitration, or performance claims.
- Zero width/height behavior, 4MiB wrap, start-while-busy, or partial-word
  abort semantics.
- Guessing reserved `TP=3` behavior.
- Changing TSP, framebuffer, SGP speed controls, demos, ROMs, or disk images.
- Real-hardware validation. Functional results remain manual-derived until a
  later hardware campaign is possible.

## Implementation stages

### M97a - Evidence correction

- Record the command coverage and manual-derived behavior.
- Correct the stale ROP and SCAN conclusions in the SGP reconstruction.

### M97b - Descriptor and LINE decoding

- Apply the original-VA documented descriptor masks without changing the
  existing VA2 profile.
- Use the documented LINE direction bits.

### M97c - SCAN commands

- Add incremental right/left scanning to the existing asynchronous command
  state machine.
- Preserve the destination descriptor when the boundary color is absent.
- Update only the fields documented for each direction.

### M97d - Regression coverage

- Add focused selftests for model-specific descriptors, all ROPs,
  destination-zero transfer, scan first-pixel/middle/not-found/word-boundary
  behavior, and left/right result updates.
- Run all repository validators and supported local builds/tests.

## Validation

```text
GIT_CONFIG_GLOBAL=/dev/null GIT_CONFIG_SYSTEM=/dev/null git diff --check
python3 tools/repo/check_encoding.py --expect utf8
python3 tools/repo/check_eol.py --enforce
python3 tools/repo/check_case.py
python3 tools/repo/clang_format.py
python3 tools/repo/find_unreferenced.py --report
cmake --preset linux-debug
cmake --build --preset linux-debug -j
GIT_CONFIG_GLOBAL=/dev/null GIT_CONFIG_SYSTEM=/dev/null ctest --preset linux-debug
build/linux-debug/sdl2/vaeg --selftest
```

Also run the repository-supported MinGW cross-build discovered from the
current preset/CI configuration.

## G97 human gate

No real PC-88VA is required for this gate.

From a clean checkout of the candidate:

1. boot VAEG in the normal VA configuration;
2. run the existing SGP pseudo-sprite demo and verify its background,
   transparency, sprite overlap, animation, and clean exit;
3. run the existing LINE-based wireframe demo if available and verify line
   direction in all visible octants;
4. verify normal V3/OS boot and display operation are unchanged.

Automated tests establish manual-derived functional behavior. The human gate
checks visual regression only; it does not claim real-hardware equivalence.

**STOP after publishing the G97 candidate until the maintainer states that
G97 passed.**
