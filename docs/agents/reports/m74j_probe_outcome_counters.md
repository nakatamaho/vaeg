# M74j probe-outcome counters

## Identity and run contract

Branch: `topic/m74-va1-basic-command-hang`  \nStarting SHA: `247eb1732b38f3a9cd202807be079374b5944523`  \nRuntime used explicit `--model va`, the trace-enabled build, deterministic guest-frame bounds, and `pcengine105-bootonly.d88`. No general interrupt hook, register capture, or per-instruction trace was added.

The diagnostic extension adds only six fixed E000 address counters: `3816`, `3818`, `3821`, `3831`, `3835`, and `3837`. The existing `3823` counter remains unchanged.

## Six-counter measurements

Each command was armed at its own injection. `A=1` used `BASIC / @prompt / A=1 / @prompt / @exit`, with injection frame 840 and deterministic second-prompt boundary 1260. `A!=1` used `BASIC / @prompt / A!=1 / @prompt / @exit`, with injection frame 840 and scripted exit frame 1080.

| command | 3816 | 3818 | 3821 | 3823 | 3831 | 3835 | 3837 | result |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| `A=1` | 4 | 4 | 4 | 4 | 0 | 4 | 0 | terminal `3983`, no second `Ok` |
| `A!=1` | 5 | 5 | 5 | 5 | 0 | 5 | 0 | `3985`, normal completion |

### A=1 disposition

`3823=4` and `3837=0` exclude the `FFFFh` route. `3831=0` excludes a positive fall-through. `3835=4` identifies the negative branch for every lookup. Therefore **D3 tier 1 is PROVEN: all four A=1 lookups return `AX=FFFEh`**, by the mutually exclusive fixed-address control-flow outcomes. No AX capture was used.

The corrected M74i downgrade was caused by the real loophole: if the `FFFFh` route entered `383A` and its inner calls exited at `3816` or `381F`, it could add no `3823` hits. The new `3837` counter closes that loophole directly.

The semantic meaning of `FFFEh` remains **UNDETERMINED**, as does the mapping of selectors to namespaces.

### A!=1 disposition

The successful command reaches `3823=5`, but `3831=0` and `3835=5`. Thus all five lookup-site executions in this successful command take the negative `FFFEh` branch. No positive lookup exists in this command. This is a first-class result: the earlier model that treated `383A` carry aggregation as a simple identifier-found/not-found test must be discarded or substantially revised. A nonzero lookup count during a successful command does not establish a positive result.

## Counter arming semantics

Source inspection of `sdl2/headless_input.c` and `upd9002_m74_trace_arm()` establishes:

- `VAEG_M74_CPU_TRACE_COMMAND` is matched against the headless script command index.
- Counters are reset when the selected command is armed immediately before injection.
- Pre-arm events are not retained.
- Counters remain active after the selected command and accumulate session-wide until shutdown; they do not reset at later command lines.
- A terminal event does not automatically stop the counters, so later input after terminal control flow is invalid as a guest interpretation.

This corrects M74h's mixed wording. The M74h `3823=9` session armed at command 3 (`A!=1`) and remained active through the later `A=1`: 5 + 4. The standalone `A!=1` measurement arms at that command and gives 5. The startup prompt-only control had no `3823` events before first `Ok`; its `01E4` startup events do not alter this attribution.

## 01E4 correction

The earlier label “startup events” for the two `01E4` events in the longer A!=1 run was not admissible once arming semantics were resolved. Those events occurred after the selected command was armed. The shorter A!=1 run used here exited before the two service returns were observed (`01E4=0` in that shortened run). Existing M74c evidence shows the normal successful path can enter the `0180` service through ordinary far-call entry paths and return through `01E4`; however, the exact two handler entry addresses in the longer A!=1 run were not captured by the fixed-address counter seam. This remains a specific gap.

What is proven is that `01E4 RETF` itself is a normally executable service return in successful guest activity; the A=1 anomaly is the frame consumed by the terminal path, not a demonstrated RETF implementation error.

## Seam equivalence

The new counter table is compiled in but guarded by the existing reachability enable switch. Disabled prompt-gate recheck on 1.05 reached first `Ok` at frame 720 and exited at frame 840 with TVRAM SHA `c6ff29f6852e02e822ce3ea628817a0fbe15bbb832701d734b3d269ac43044b5`. This matches the established M74f N1/N0 prompt observables. Enabled A=1 and A!=1 runs produced the deterministic results above.

## Outstanding items

The five continuation-segment byte dumps at reset, first prompt, and pre-command were not captured. The E4 installer source-record list is not fully decoded; no claim that `1040:0AC3` is LIVE or should be LIVE is made. AX values, selector names, DS:SI bytes, and a positive lookup control remain unavailable.

## Evidence hashes

- Enabled A=1 log: `4973e104ddd6b2c9df5d9c8eef25a54864b78174932f1cb801de9b9764f6574a`
- Enabled A=1 TVRAM: `126908bee355934c5e357d1b5f7d210ca9d1ecb3d2ff25ca0cef02c3c4b5c5bc`
- Enabled A!=1 log: `91c3e1111f7b3e62655bbf9616995f9b7f6dbe84fb38f4be62bcd603091be80c`
- Enabled A!=1 TVRAM: `b307e8ca764da5af022b7b6787daa274acaf425e36350c9f52fd0c15ae870550`
- Disabled prompt log: `8675070b80e36253f267e90070a21fb963e1c43aba1e6b672c099b11f6b22167`
- Disabled prompt TVRAM: `c6ff29f6852e02e822ce3ea628817a0fbe15bbb832701d734b3d269ac43044b5`
- Diagnostic worker: `efe027376f95645c274bfd3395ee9abdc507d2ea3c21bc52087ca5f1cd10d379`

## Verdicts

- D3 tier 1 (`AX=FFFEh` on all four A=1 lookups): **PROVEN**.
- Positive lookup during successful `A!=1`: **REFUTED for this command** (`3831=0`); no universal positive lookup claim is made.
- `FFFEh` ABI meaning: **UNDETERMINED**.
- Four selector namespace mapping: **UNDETERMINED**.
- First incorrect emulator-produced state: **not proven**.
- Production fix: none.
- G74: not approved.
