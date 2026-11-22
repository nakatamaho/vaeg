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
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
-->

# M153 — Native interrupt shadow after loading SS

[Task](../tasks/M153_mov_ss_shadow.md) · [Implementation](https://github.com/nakatamaho/vaeg/commit/e9b165affcf9bc2efc29a27d36b13eb56d5ad8ef)

## Cause and semantics

`NEXT_OPCODE` extended the clock budget but did not inhibit arbitration at a
slice boundary. If the PIC acknowledged INT after an SS load, the hardware
entry path pushed FLAGS, CS, and IP using the new SS and the old SP. The
native trap latch existed, but the instruction loop did not deliver vector 1.

The hardware reference is NEC, *μPD70108/70116 V20/V30 User's Manual*, October
1986. The repository's [uPD9002 mode analysis](../../modernization/upd9002-upd70008-mode.md#35-interrupts-reset-and-halt)
explains the separate compatible dispatch and native handler transitions. It
does not supply a divergent uPD9002 rule for the native stack-switch sequence.

| Case | Semantics and source |
|---|---|
| `MOV Sreg,r/m16`, `POP Sreg` | SS, ES and DS inhibit INT and vector 1 across the boundary before the next instruction. The V30 native instruction entries for `MOV sreg,reg16`, `MOV sreg,mem16`, and `POP sreg` specify this for the documented destinations, not only SS. The existing undocumented MOV-to-CS treatment is unchanged; it does not arm a shadow. |
| `LES`, `LDS` | Unchanged. The `MOV DS1,reg16,mem32` and `MOV DS0,reg16,mem32` entries (pp. 12-9–12-10) do not specify this inter-instruction inhibit, and the chapter 6 inhibit list does not identify those forms. |
| NMI | The same MOV/POP entries and chapter 6, *Interrupt Disable Timing* (p. 6-4), specify an NMI inhibit with the request retained. There is no native NMI producer or pending-NMI input in this tree. Vector number 2 does not identify the interrupt source; the internal vector-entry routine is not an NMI input. No NMI implementation or test is claimed. |
| `STI` (`EI` in the manual) | When IF changes from clear to set, maskable INT waits across the following instruction. The `EI` entry and chapter 6 specify an INT-only delay; TF is not inhibited. No new delay is armed if IF was already set. |
| TF | Chapter 6, *BRK Flag (Single-Step Interrupt)* (p. 6-3): vector 1 is an after-instruction event, independent of IF. TF is sampled before the instruction, so enabling it with a flags restore steps the following instruction. A synchronous interrupt or fault still enters normally and does not also cause a vector 1 entry for the same instruction. |

Chapter 6 separately describes external-inhibit cases for MOV *from* a
segment register and for prefix boundaries. This change does not implement
those independent cases; it uses the existing combined segment-override/
REP dispatch. The standalone BUSLOCK handling is unchanged. IRQ/TF
collision ordering at shadow release is not covered by the focused tests;
this change does not replace the general slice-driven arbitration model.

## Implementation and dispatch coverage

- All ordinary, segment-override, and REPE/REPNE paths that write SS reach
  `_pop_ss` or `_mov_seg_ea`. REPC/REPNC do not dispatch these non-string
  instructions. There is no native `LSS` or alternative fast SS setter. The
  uPD70008-compatible ISA and independent uPD780C instances do not have
  these native opcodes; native handlers reached from compatible mode use
  the same native dispatch.
- The PIC checks the shadow before acknowledgement or request mutation. The
  scheduler polls again after the protected instruction when still in the
  current slice. A new qualifying load renews the shadow. A REP slice
  restart preserves it; a stopped diagnostic dispatch does not consume it.
- Reset and shutdown clear it. SAV stores a nonzero shadow in the optional
  one-byte, version-0 `UPD9SHDW` section without changing `UPD9CPU` or
  `UPD9MODE`. Older saves default to no shadow; older readers need not be
  able to restore a new save containing the section. The debugger still
  stops before guest instructions without executing them, and TF entry is
  traced as a CPU exception, not as a debugger breakpoint or PIC interrupt.

## Verification

- Focused CPU test: `vaeg_upd9002_interrupt_shadow` checks the retained IRR,
  unchanged ISR and old-stack location, and the frame at the new SS:SP for
  both SS load opcodes and prefixed forms. It also checks TF with IF clear,
  ordinary IRQ, consecutive segment loads, STI, REP restart, INT3, HLT and
  reset.
- The existing SAV boundary test adds a round trip with an active shadow, a
  single corrupt-byte rejection (`M153_INTERRUPT_SHADOW_STATE`), and an
  absent-section load that clears the prior shadow.
- Linux Debug full configured CTest run: 151 passed, 0 failed, 10 skipped,
  1 disabled (162 registered). This includes the SAV boundary cases in the
  ROM-less selftest and the trace-equivalence test. The build used the
  implementation changes before the final formatting and release-identity
  update; recording was disabled for this test configuration. After a
  rebuild at `2d44fab1`, the focused CPU, protected-state, and ROM-less
  subset passed 11/11.
- Case, EOL, encoding, Markdown-comment, and diff-whitespace checks passed.
  The repository-wide clang-format check still fails on inherited findings
  also present on `main`; the new test and changed source lines were checked
  with clang-format 22.1.8.
- Static Linux-host MinGW clean-clone build at `2d44fab1`: PASS, including
  embedded build-identity, FFmpeg encoder archive and PE import audits.
  Windows execution is **NOT RUN**.
- The standard human gate, physical hardware, native NMI, and the separate
  full media/demo and presenter campaigns are **NOT RUN**. No archived
  reference-tier behavior or provenance was changed.
