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

# Add an explicit NEC uPD780C-1 flag profile and regression tests

## Summary

This PR adds `Z80::FlagProfile::Upd780C1` to explicitly select the NEC NMOS
uPD780C-1 SCF/CCF flag behavior, together with regression tests based on published
findings and bounded hardware observations.

The existing non-Upd9002 implementation already produces the observed NEC
results. This change gives that behavior a separate, tested API contract without
changing either existing profile or the default selection.

## Motivation

SCF/CCF behavior differs between NEC NMOS and Zilog processors: published NEC
reports describe undocumented X/Y flags copied from A, while Zilog behavior
also depends on flag-writing history, commonly modeled with Q.

The current implementation copies X/Y from A under the `Zilog` selector.
Providing an explicit NEC selector lets callers choose the intended target and
preserves NEC behavior independently of any future Zilog Q-history improvement.
This PR establishes that distinction; it does not implement Q tracking.

```cpp
z80.setFlagProfile(Z80::FlagProfile::Upd780C1);
```

## Changes

- Append `FlagProfile::Upd780C1`, preserving existing enum values and the default.
- Document its SCF/CCF contract:
  - SCF preserves S/Z/PV, clears H/N, sets C, and copies X/Y from A.
  - CCF preserves S/Z/PV, copies old C to H, clears N, toggles C, and copies
    X/Y from A.
- Add C++11 regressions using guest PUSH/POP AF and the actual instruction
  handlers, checking A/flags, PC/SP, IX/IY and exported AF agreement.
- Add CP-flag-writer, NOP, DD and FD boundary cases.
- Exercise strict, no-exception and lean callback configurations through the
  test Makefile.

## Hardware observations

Recorded VA2 uPD780C-1 flag/history controls match the A-only rule. A subsequent
CP B / NOP / DD / FD probe also returned identical 128-row logs on the reported
VA and VA2 targets, with no findings in its bounded RAM/triple/mirror checks.

With A=00 and B=28, CP produces control AF=00BB. All eight repetitions of each
boundary case produced:

| Boundary | SCF AF | CCF AF |
|---|---|---|
| CP B immediately before the instruction | 0081 | 0090 |
| CP B; NOP | 0081 | 0090 |
| CP B; DD prefix | 0081 | 0090 |
| CP B; FD prefix | 0081 | 0090 |

The A=28 control produces AF=286A after CP and AF=2869 after either operation.

These observations support the tested contract, not exhaustive NEC instruction,
variant or timing equivalence. Other original-VA contexts still have unresolved
X/Y differences and are not modeled by this change. The tests use independently
specified answers; no private ROMs, media or raw captures are included.

## Compatibility

- Existing `Zilog` and `Upd9002` behavior is unchanged.
- `Upd780C1` currently shares the normal non-Upd9002 implementation; other
  instructions retain that implementation rather than introducing new NEC rules.
- DD/FD tests use the documented `Z80_NO_EXCEPTION` compatibility decoder.
  Strict-build unknown-opcode exceptions are unchanged.
- Future Zilog Q work can be reviewed separately while retaining the NEC
  A-only contract.

## Testing

```sh
make -C test test-upd780c1-profile
make -C test all
```

Both passed with upstream's C++11 warning-as-error flags:

| Configuration | AF/operation/boundary cases | CP boundary cases |
|---|---:|---:|
| Strict | 131072 | 8 |
| No-exception | 393216 | 16 |
| Lean callback/no-exception | 393216 | 16 |

These are software test counts, with overlapping coverage across configurations,
not physical coverage counts. As a negative check, changing SCF from XY=A to
XY=A\|F in an isolated header copy is detected at AF0008 with `UPD780C1_AF`.

The new test uses BSD-2-Clause; the existing MIT core/license and copyright
headers are unchanged.

## References

- [Published NEC/Zilog/ST flag distinctions](https://github.com/hoglet67/Z80Decoder/wiki/Undocumented-Flags/49e542dd40c36ef7367cb5806292764416dc987a).
- [Recorded CP/prefix observations and verification limits](https://github.com/nakatamaho/vaeg/blob/cb050b2607064525748f01a1a356c0dd8ef49b08/docs/agents/reports/m140_fdc_va_observations.md#returned-fdcpref-controls).

---

## Submission notes — not part of the PR body

**Status: prepared, tested and branch pushed; GitHub PR NOT_SUBMITTED.**
No PR number or upstream acceptance is claimed. Copy the title and sections
above, excluding these submission notes.

- [Create the upstream PR](https://github.com/suzukiplan/z80/compare/master...nakatamaho:m140-upd780c1-flags?expand=1).
- Branch: `nakatamaho/z80:m140-upd780c1-flags`.
- Candidate: [0a927f456ce9717e5bdf8205f50c7df81dcb435c](https://github.com/nakatamaho/z80/commit/0a927f456ce9717e5bdf8205f50c7df81dcb435c).
- Tested base: [c6204813558cd515fc100ea97f6a21dbdb5a4905](https://github.com/suzukiplan/z80/commit/c6204813558cd515fc100ea97f6a21dbdb5a4905).
- [Applicable patch](m140_upstream_upd780c1_profile.patch), checked against an
  independent export of that base. This document edit does not change the patch
  or the already-tested upstream source. Active vaeg vendor files are untouched.

Review notes retained for M140:

- [PR63](https://github.com/suzukiplan/z80/pull/63) is closed without a merge
  record, but the compatibility ignored-prefix decoder is present. The initial
  strict DD37 exception was reclassified as documented policy, not a new bug;
  no duplicate prefix fix is proposed.
- [PR64](https://github.com/suzukiplan/z80/pull/64) is merged.
- The untouched-source compatibility reproducer matches the published Zilog-Q
  prediction in2/8 cases and differs in6/8. This is a software comparison, not
  independent physical Zilog evidence. A complete Q implementation needs
  separate flag-writer, prefix, interrupt, reset and state-import review.
- No API-authenticated submission has been performed. M140/G140 gate status
  is unchanged by this wording-only edit.
