# M74h continuation-target census

## Identity and admissibility

Branch: `topic/m74-va1-basic-command-hang`  \nStarting SHA: `496a8c0f3c8bbe0f3d6d35288083b1c6e575c0b6`  \nCurrent source tree was inspected with `--model va`; runtime work used the existing trace-enabled VA1 worker and deterministic frame/prompt bounds. No production source was changed and no SCSI/HOSTFAT runtime was used.

## A: static D3 closure

The existing M74f B0 count is four executions of `E000:3823` on each boot-only image. The complete static CFG establishes that `3806` has four logical calls from `383A`: selectors `BX=0`, `7`, `5`, and `4`. The following preconditions are confirmed by the disassembly:

- `382D` calls `F798` and then returns to the sequential `383A` instruction; the route does not return to `3806` and cannot create extra `3823` executions in the observed four-count window.
- `385D`--`385F` are `POP SI`, `POP BX`, and `POP AX`; none writes FLAGS.
- The fixed-address count is armed at command processing and no alternate `3823` site exists in the measured `E000` code.

Consequently, the control-flow deduction is:

- all four logical `3806` calls reach `3823`;
- probes 1--3 return CF=1, otherwise `JNC 385D` would short-circuit;
- none takes the `FFFF`/`F798` route;
- probe 4 also returns CF=1 because `3860 CMC` produces the CF observed by `397A`;
- the only producer of those negative probe returns in `3806` is `3825 CMP AX,FFFE` followed by `3835 STC`.

D3 is therefore split: `AX=FFFEh` on all four probes is **PROVEN BY CONTROL-FLOW DEDUCTION**; `FFFEh` meaning “not found” is **UNDETERMINED**; and the four selectors being four distinct namespaces is **UNDETERMINED**. Four executions are not reported as four semantically identified probes.

## B: pre-created variable control

The deterministic run used `pcengine105-bootonly.d88`, `--model va`, `VAEG_M74_REACHABILITY=1`, `VAEG_M74_CPU_TRACE_LIMIT=1`, `VAEG_M74_CPU_TRACE_COMMAND=3`, `VAEG_HEADLESS_MAX_FRAMES=2500`, and `VAEG_HEADLESS_PROMPT_TIMEOUT_FRAMES=300`. The input sequence was `A!=1`, `A=1`, `? A`, each separated by `@prompt`.

| event | result | fixed counters at run end |
|---|---|---|
| `A!=1` | first command injected; escape path returns normally | contributes `391D=1`, `3985=1` |
| `A=1` | injected at frame 840; no subsequent prompt by frame 1500 | `391D=2`, `3983=1`, `3985=1`, `002A=1`, `01E4=1`, `3823=9` |
| `? A` | injected at frame 1080, but not processed after the prompt timeout | no result claim |

The 9 count includes startup/other activity and the four A=1 logical calls; this run did not include the requested per-hit AX/CX/DS:SI snapshots. Therefore it does not provide a positive lookup control. It does establish that creating `A` through the suffixed path did not make the following unsuffixed `A=1` reach the balanced return within the run. Whether that is selector coverage or lookup-table semantics remains open.

A fresh-session `A=1` baseline remains the established `3983` chain. Typed controls were not reinterpreted as non-escape controls.

## C: continuation target census

The corrected target formula is `CS=post-CALL IP`, `IP=saved DX`, physical address `CS*16+DX`. For `E000:34BD`, DX is known as `0005`, giving `34C0:0005` / physical `34C05h`. Existing lifecycle evidence shows `34C00h--34CFFh` is ordinary VA RAM, remains zero from reset through the failure, and receives no write. The exact target bytes are therefore 16 zero bytes at the measured boundary.

The other callers are: `43B5:DX`, `49FC:DX`, `75AB:DX`, and `7F2D:DX`. Their DX values were not observed in this run, so their exact target offsets are undetermined. The requested 256-byte dumps at reset/first prompt/pre-command were not captured; this is a named instrumentation gap, not an inference that those regions are zero.

## D: caller/callsite matrix status

The static five-site census is:

| caller | post-CALL IP | DX | 3983 target |
|---|---:|---|---|
| `34BD` | `34C0` | known `0005` | `34C0:0005` |
| `43B2` | `43B5` | unknown | `43B5:DX` |
| `49F9` | `49FC` | unknown | `49FC:DX` |
| `75A8` | `75AB` | unknown | `75AB:DX` |
| `7F2A` | `7F2D` | unknown | `7F2D:DX` |

The existing admissible traces establish `A=1` at `34BD` and `A!=1` at the same caller. `A%=1` has two scanner invocations in prior evidence, but this run did not add caller-order events; whether both callers are identical is therefore **UNDETERMINED in this report**.

## E: 1040:0AC3 status

The existing eight-image census has `1040:0AC3` as `CB` in the available configurations, including configurations where escape-form BASIC commands complete. No runtime image with this slot LIVE, and no installer source record proving it should be LIVE, is currently known. This strongly rejects “0AC3 is missing and therefore explains the escape-path behavior”; it does not by itself establish the non-escape service contract.

## H1/H2 and remaining boundary

H1 is **supported structurally**: all five real callers share the post-CALL-IP/DX continuation shape, and the measured stack is consumed exactly by `3983 -> 002A -> 0180 -> 01E4 RETF`. H2 is **not supported**: no successful same-semantic non-escape comparator or incorrect emulator-produced selector input has been proven.

The first incorrect emulator state remains **not proven**. The remaining high-value gap is the bounded four-hit state capture for the pre-created-variable second command, followed by a proven positive `AH=00` lookup or a populated continuation target. No production fix is authorized.

## Evidence hashes

- Worker: `4844fee9833ec1a05bd119588132f53ea93816084e93b025ba05213671d9d5a1`
- Control log: `sha256` recorded outside Git; current run hash is reported in the handoff.
- TVRAM: recorded outside Git; current run hash is reported in the handoff.
- No ROM/disk/private asset bytes were added to Git.

## Validation and status

`git diff --check` and repository encoding/EOL/case checks were run with the repository Git configuration disabled. No production build change was made. Hosted CI was not run. G74 remains not approved.
