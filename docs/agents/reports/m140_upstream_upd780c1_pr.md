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

# M140 upstream PR: explicit uPD780C-1 flag contract

**Status: prepared and locally tested; NOT_SUBMITTED to GitHub.** This is a
profile/API and regression-test contribution, **not a correction to NEC A-only
arithmetic**, not a Zilog-Q implementation, and not an active vaeg vendor update.
The [applicable patch](m140_upstream_upd780c1_profile.patch) is based on upstream
[c6204813558cd515fc100ea97f6a21dbdb5a4905](https://github.com/suzukiplan/z80/commit/c6204813558cd515fc100ea97f6a21dbdb5a4905).
Pushed candidate:
[0a927f456ce9717e5bdf8205f50c7df81dcb435c](https://github.com/nakatamaho/z80/commit/0a927f456ce9717e5bdf8205f50c7df81dcb435c),
branch `nakatamaho/z80:m140-upd780c1-flags`.
[Create the upstream PR](https://github.com/suzukiplan/z80/compare/master...nakatamaho:m140-upd780c1-flags?expand=1)
with the title/body below. No PR number is assigned and no API-authenticated
submission was performed. The patch applies cleanly to an independent export
of that base.

## Upstream review and scope correction

Current upstream already contains the no-exception ignored-prefix decoder.
[PR63](https://github.com/suzukiplan/z80/pull/63) is closed without a merge record,
but that status does not mean its functionality is absent. The README expressly
retains unknown-opcode exceptions in strict builds. An initial strict-build
DD37 exception was retained as a diagnostic, then correctly classified as policy,
**not a new ignored-prefix bug**. Compatibility/no-exception reproduction passes.
[PR64](https://github.com/suzukiplan/z80/pull/64) is merged. No duplicate prefix
fix or change to exception policy is proposed here.

The untouched default Zilog profile copies SCF/CCF X/Y from A. A local source
reproducer gives 0081/0090 after CP B, with or without NOP/DD/FD, while the
published Zilog-Q model predicts 00A9/00B8 after those neutral boundaries.
It matches 2/8 published-Q expectations and differs in 6/8. This is a software
comparison, not an independent physical Zilog run. Q tracking needs a separate
implementation and review of flag writers, POP/EX AF, prefixes, interrupts,
reset and state import; it is deliberately not bundled into this contribution.

---

## Suggested GitHub title

Add an explicit uPD780C-1 flag profile and SCF/CCF regression tests

## Copyable PR body

### Summary

Add `Z80::FlagProfile::Upd780C1` as an explicit selector for the confirmed NEC
NMOS uPD780C-1 SCF/CCF contract. Append the enum value without changing the
existing Zilog/Upd9002 values or the default profile.

The existing non-Upd9002 path already implements the observed A-only behavior.
This PR intentionally does **not** change that arithmetic. Its purpose is to
name and test the NEC compatibility contract independently, so a future Zilog
Q-history improvement does not silently change NEC behavior. No claim is made
that every NEC variant or all original-VA observations are explained.

### Why a separate selector?

Published NEC NMOS reports describe X/Y from A for SCF/CCF; published Zilog
behavior depends on previous flag-writing history. Those are different targets,
although the current implementation uses A-only under the Zilog name.

The intended NEC contract is:

- SCF: preserve S/Z/PV, clear H/N, set C, copy X/Y from A.
- CCF: preserve S/Z/PV, set H from old C, clear N, toggle C, copy X/Y from A.
- Preserve the A register and ordinary POP/PUSH AF storage.

Other instructions retain the existing normal implementation. Full NEC chip,
undocumented-opcode and timing equivalence are outside this PR's evidence.
The uPD9002 compatibility mode is a different CPU role and is unchanged.

### Bounded hardware observations

The reported VA2 uPD780C-1 observations match A-only across the recorded flag
and history controls. A subsequent CP B / NOP / DD / FD test returned identical
128-row logs on both reported VA and VA2 targets, with no bounded RAM/triple/
mirror integrity findings. For A=00 and B=28, CP gives the expected control
AF=00BB; SCF gives 0081 and CCF gives 0090 in all four boundary modes and all
eight passes. For A=28, control AF=286A and either operation gives2869.

This does not prove physical Q contents, all inputs/history or all timing. Other
original-VA POP-AF-target contexts have unresolved X/Y discrepancies; this PR
neither fits those rows with a lookup table nor claims to solve them. Private
ROMs, disks, captures, raw logs and their identities are not included.

References:

- [Published NEC/Zilog/ST flag distinctions, pinned revision](https://github.com/hoglet67/Z80Decoder/wiki/Undocumented-Flags/49e542dd40c36ef7367cb5806292764416dc987a).
- [Bounded returned CP/prefix observations and limitations](https://github.com/nakatamaho/vaeg/blob/cb050b2607064525748f01a1a356c0dd8ef49b08/docs/agents/reports/m140_fdc_va_observations.md#returned-fdcpref-controls).

### Changes

- Append `FlagProfile::Upd780C1`; retain both existing enum values and default.
- Document the current A-only implementation, the separate NEC contract and
  the fact that modern Zilog Q tracking is not yet implemented.
- Add a standalone C++11 regression through guest LD/PUSH/POP AF and the real
  SCF/CCF handlers, checking A, flags, PC/SP, IX/IY and exported AF agreement.
- Add CP-flag-writer / NOP / DD / FD boundary cases with independently specified
  answers, not imported private logs.
- Run strict, no-exception, and lean callback/no-exception configurations.
  DD/FD cases use the documented compatibility decoder; strict exceptions stay
  unchanged. Existing uPD9002 tests and default semantics remain unchanged.

### Local validation

Against upstream c6204813558cd515fc100ea97f6a21dbdb5a4905 plus this patch:

```sh
make -C test test-upd780c1-profile
make -C test all
```

Both exit0 with upstream's C++11 warning-as-error flags. New test output:

```text
strict:       software_AF_cases=131072 CP_boundary_cases=8
no-exception: software_AF_cases=393216 CP_boundary_cases=16
lean:         software_AF_cases=393216 CP_boundary_cases=16
```

These counts include repeated configurations and are **software**, not exhaustive
physical silicon coverage. An isolated single source mutation changing SCF
from XY=A to XY=A|F fails with stable code `UPD780C1_AF` at AF0008, proving the
new contract catches accidental replacement with OR. The mutation starts from
the passing source; no production header is left mutated.

The new test has a BSD-2-Clause header; the existing MIT core/license and all
existing copyright headers remain unchanged.

### Deliberately separate follow-up

Implement accurate Zilog Q semantics under the Zilog selector, with dedicated
Zilog/reference tests and explicit reset/import/prefix/interrupt behavior. Keep
`Upd780C1` on A-only. Do not implement random X/Y, code-address workarounds or
unverified original-VA silicon rules as part of this PR.
