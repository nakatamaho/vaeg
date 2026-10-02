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

# M103a - Machine-owned alternate register storage

Status: **ready (approved on 2026-10-02)**

Series: V1/V2 mode (ADR-0016). First milestone of the series; it lands on
`main` before `topic/v1v2-mode` is created, because the V3 series depends
on the same register model.

Predecessor: G102 passed; M102 merged to `main` at
`659a01e16f5e0c7b79e314b5cbdc0a3e12cddcca`.

Branch: `topic/m103a-alt-registers` (off `main`)

Commit prefix: `M103a:`

## Background

`docs/modernization/upd9002-upd70008-mode.md` §3.1 and §11-4: the µPD9002
carries eight bytes of register storage that native mode cannot address —
`AF'`, `BC'`, `DE'`, `HL'`. A real-machine dump shows the alternate set
holding values distinct from the main set, and the V30 register file has no
room for them. The main set, `IX`/`IY` and the compatible-mode `SP` are
aliases of the native file (`AL`, `CX`, `DX`, `BX`, `SI`, `DI`, `BP`, `IP`).

In vaeg today (`cpu/upd9002_upd70008.cpp`):

- the main set is copied between the native core (`CPU_*`) and the
  suzukiplan core's `reg.pair` at `Enter()`, `Resume()` and
  `SyncToNative()` — a mirror, not an alias, but with a working sync
  discipline;
- the alternate set exists **only** inside the suzukiplan core
  (`reg.back`). It is absent from the native register model, invisible to
  the debugger and GUI register views, persisted only inside the opaque
  `UPD9Z80` compat section, and discarded whenever `Reset()` clears
  `have_compatible_state_` and the next `Enter()` calls `SetReg()`.

V1/V2 mode will run compatible-mode code for long stretches with frequent
CALLN round trips, mode transitions, resets and state saves, so the
alternate set needs a definite owner before that work starts.

A defect found while closing M102 belongs here as well: `StateLoad()`
initialises the compat core when a state is loaded before any `Enter()`,
but does not install `SetNativeVectorRead`, so the undefined ED 00–3F read
page 0 as zeros until the next mode entry.

## Scope

1. **Machine-owned alternate storage.** Add `alt_af`, `alt_bc`, `alt_de`,
   `alt_hl` (eight bytes) to the native uPD9002 state owned by
   `cpu/upd9002/` (the `UPD9CPU` state image, with a version step per the
   M85 state-save conventions and a loader that accepts the previous
   version with zeroed alternates). The adapter copies them in and out
   together with the main set at `Enter()`, `Resume()` and `SyncToNative()`;
   the suzukiplan core keeps using `reg.back` as its working copy.
2. **Lifetime policy**, written into `upd9002-upd70008-mode.md` §11 and
   implemented: the alternate set survives CALLN/RETI round trips,
   RETEM→BRKEM re-entry and adapter `Reset()`; it is cleared only by the
   machine's hardware reset path (`upd9002_state_reset`). The unmeasurable
   reset case is a policy, tagged `[DERIVED]`, not a measurement.
3. **Real-machine probe `ALTPRB`** (CP/M program under CP/MVA, built by
   `tools/cpmva/zex/zexbuild.py`, output `ALTPRB.TXT` in the probe style of
   M102): load known patterns into the alternate set and different patterns
   into the main set, perform a `CALLN 91h` V3 memory read (§7.1), then
   record both sets and F before and after. This measures whether the
   alternate set and the main set and flags survive a genuine native round
   trip (§3.1 `alt-regs` persistence, and the §3.2/§17.6 `flag-callback`
   anomaly). The CP/M host runner cannot execute CALLN; `ALTPRB` is
   real-machine-only like `INPRB`.
4. **`StateLoad` fix**: install the native vector reader on the
   load-before-enter path; add a wrapper or adapter test that loads a state
   first and executes `ED 00`. Ledger entry.
5. **Visibility**: expose the alternate set in the existing debugger
   register dump (and the GUI register view if one exists for the main
   CPU), read-only.
6. Tests: wrapper contract test for the adapter copy-in/copy-out and the
   lifetime policy (Reset preserves, hardware reset clears); state-save
   round trip including the new fields and the old-version load path.

Out of scope: the V1/V2 machine side (M103b onward), changes to the
vendored core, any change to the main-set mirror design.

## Deliverables

- `cpu/upd9002/` state image extension and `cpu/upd9002_upd70008.cpp`
  copy-in/copy-out.
- `tools/cpmva/zex/altprb.asm`, parser and `compare.py` section, installer
  program-set and disk-budget updates, `docs/cpmva-setup.md` row.
- Real-machine evidence under `docs/agents/reports/m103a_altregs_qa/`
  (same layout as `m102_zexund_qa`), with the measured persistence recorded
  in `upd9002-upd70008-mode.md` §3.1/§17 as `[MEAS]` and §15.2 `alt-regs`
  and `flag-callback` updated.
- `docs/modernization/bug-fixes.md` entries for the `StateLoad` reader
  omission and, if the probe shows one, any persistence defect.
- Task-file progress log; ROADMAP status.

## Machine checks

- `cmake --build --preset linux-ci-gcc` and `ctest` (all green).
- `tools/repo/check_case.py`, `check_encoding.py --expect utf8`,
  `check_eol.py --enforce`, `tools/qa/milestone_ids.py --audit`.
- `tools/cpmva/zex/host_reference.py` unchanged outputs for every existing
  program (ALTPRB excluded from the host run).

## Gate G103a (human)

Standard gate (clean build, V3 boot, VA demo, OS boot and simple
operations) plus:

- `ALTPRB` on the real PC-88VA2 and inside vaeg report the same alternate
  and main register values across the CALLN round trip (or the difference
  is explained and fixed in this milestone);
- a state saved while in compatible mode with distinct alternate values
  restores them, and a state file from before this milestone still loads.

## Progress

- 2026-10-02: task created (ADR-0016).
