# M75 - uPD9002 uPD780 emulation-mode authority audit

M75 audits the uPD9002 main-CPU uPD780 emulation-mode evidence and decides the
safe implementation authority before the later I/O and BIOS restructuring
milestones.

Predecessor: approved G74.

Branch: `topic/m75-upd9002-upd780-emulation-mode-authority`

Commit prefix: `M75:`

Candidate gate: `G75`

Report: `docs/agents/reports/m75_upd9002_upd780_emulation_mode_authority.md`

Do not start M76. Do not merge M75 to `main` before G75 approval. Do not
declare G75 passed.

## Scope

M75 owns the authority boundary for the uPD9002 main CPU's uPD780
emulation-mode mechanism. This is separate from the FDC subsystem
uPD780-compatible CPU currently wrapped through `cpucva/z80_core.cpp`.

M75 must:

- audit `docs/modernization/upd9002-upd780-mode.md` and current uPD9002 mode
  state, decode, trace, interrupt, and state-save boundaries;
- use M73 VA1 BASIC command-hang evidence and retired VA1 diagnostic investigation SCSI evidence as inputs
  when available;
- distinguish project-target software evidence from physical-silicon evidence;
- record the limits of V20/V30 8080-emulation-mode analogy sources;
- identify any VA ROM, BASIC, Debug 8800, or other guest software path that
  attempts to enter uPD9002 emulation mode;
- decide whether production implementation is authorized now, should be split
  into a later milestone, or remains blocked pending hardware evidence;
- define the exact state-save, trace, decoder, and test obligations for any
  later implementation.

## Non-goals

M75 must not:

- implement broad generic V20, V30, i286, i386, 8080, or Z80 compatibility;
- conflate the main uPD9002 emulation-mode mechanism with the FDC subsystem
  uPD780-compatible CPU;
- change FDC subsystem CPU behavior;
- move `iova/*`;
- implement SCSI;
- remove 98-only I/O or BIOS code;
- claim complete physical uPD9002 silicon proof while the repair/test hardware
  remains unavailable.

If M75 proves that emulation-mode implementation is required and sufficiently
bounded, it must still close with a clear authority report unless the approved
task is explicitly amended to include implementation.

## Validation

Run repository invariant checks, normal builds, native tests, focused uPD9002
mode-state audits, and any trace or ROM-less probes needed to prove whether
guest software reaches the emulation-mode entry path.

The human gate must review the authority conclusion and decide whether a later
implementation milestone is authorized.
