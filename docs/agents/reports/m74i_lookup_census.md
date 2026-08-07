# M74i lookup census

## Identity

Branch: `topic/m74-va1-basic-command-hang`  \nStarting SHA: `3d33e4295231060b3bdfb044269221dc246c419b`  \nNo production source was changed. Runtime used explicit `--model va`, the existing trace-enabled worker, deterministic frame bounds, and `pcengine105-bootonly.d88` only.

## D3 correction

At `E000:3837` the instruction is `CALL F798`; the next instruction boundary is `E000:383A`, whose first instruction is `CALL FAR 1040:0AAF`. The call to `F798` uses the ordinary near-call convention and returns before execution falls through to `383A`; the existing disassembly shows no jump around `383A` and no alternate return into `3806`. Therefore an `AX=FFFF` result at `382D` would add the four `3806` calls in `383A`, and could add further `3823` executions.

The M74f plain A=1 count of 4 is therefore not, by itself, enough to exclude `FFFF`; M74h's D3 tier-1 “PROVEN BY CONTROL-FLOW DEDUCTION” was overstated. The corrected tier-1 status is **DOWNGRADED / NOT PROVEN** until the per-hit state census records the return values. Tiers 2 and 3 remain **UNDETERMINED**: the ABI meaning of `FFFEh` and the namespace identity of selectors are not established.

The other preconditions remain valid: `385D`--`385F` are `POP SI`, `POP BX`, `POP AX` and preserve CF, and the static code contains only the fixed `E000:3823` site in this path.

## A: A!=1 alone

Run command: `A!=1` alone after BASIC, with deterministic `VAEG_HEADLESS_MAX_FRAMES=1500` and `VAEG_HEADLESS_PROMPT_TIMEOUT_FRAMES=300`, explicit VA model, and reachability enabled. The run returned normally and reached the requested exit.

| measure | result |
|---|---:|
| first BASIC Ok | frame 720 |
| command injection | frame 840 |
| second prompt | not required; script exited at frame 1080 |
| `391D` | 1 |
| `3983` | 0 |
| `3985` | 1 |
| `002A` | 0 |
| `01E4` | 2 startup events |
| `E000:3823` | **5** |
| exit status | 0 |

This confirms the arithmetic attribution of the earlier nine-count session at the counter level: the standalone A!=1 run contributes five, while the separate A=1 run contributes four. The count is not yet a semantic positive lookup: no AX values were captured.

Evidence: worker SHA `4844fee9833ec1a05bd119588132f53ea93816084e93b025ba05213671d9d5a1`; log SHA `bba75832c2df5ac5884d360431a31889de9c24cf98ba8639d0d99c24cb421d81`; TVRAM SHA `b307e8ca764da5af022b7b6787daa274acaf425e36350c9f52fd0c15ae870550`.

## B: counter arming semantics

Source inspection resolves the previous wording conflict:

- `VAEG_M74_CPU_TRACE_COMMAND` is compared with `script->command_index` in `upd9002_m74_trace_arm()`.
- The trace is armed when the selected input command is about to be injected, not when BASIC starts.
- `reach_*` counters are reset in `upd9002_m74_trace_arm()` and remain active for the rest of the session; they do not reset at each subsequent command.
- Pre-arm hits are not retained.
- The final reachability summary is session-wide from the selected arm point, not a per-command report.
- Counters continue after a terminal event if the emulator continues executing; later input injected after a terminal transfer is not semantically interpretable.

Thus the earlier M74h statement that counters were “armed at command processing” was correct; the statement that the same session's summary could be treated as only the A=1 command was incorrect. In the M74h sequence, `command=3` arms on A!=1 and remains active through A=1, giving `5 + 4 = 9`. In the standalone run, `command=1` arms on the first script command and the observed five are retained through the successful A!=1 command. Startup before the arm contributes zero `3823` events in the measured prompt-only control.

## C: lookup census status

The nonzero standalone A!=1 count satisfies the gate to perform the lookup census, but the existing seam records only the fixed-address count. It does not yet capture caller/path, BX, CX, DS:SI bytes, AX before/after, FLAGS, or the `3831`/`3835` branch. Those fields are a specific instrumentation gap; no AX or positive lookup claim is made. The result establishes only that lookup-site execution occurs during a successful command.

The required five-hit census and the five continuation-segment dumps were not added in this pass. Exact DX values for the four non-`34BD` callers remain unknown. The existing evidence still establishes `34C0:0005` as physical `34C05h`, ordinary VA RAM, zero and unwritten through the A=1 failure.

## E4 and command model

The existing all-image census continues to show `1040:0AC3` as `CB`; a fully decoded installer source record proving a LIVE instance was not produced here, so the three-way E4 answer is “source-record list not yet fully decoded,” not a general refutation.

The single-command A!=1 result supports the working model of approximately one `391D` invocation per command line, but does not prove it universally.

## Verdicts and next boundary

- D3 tier 1 (`AX=FFFEh` on all four A=1 hits): **UNDETERMINED / downgraded** after correcting the FFFF fall-through logic.
- D3 tier 2 (`FFFEh` semantic meaning): **UNDETERMINED**.
- D3 tier 3 (four namespaces): **UNDETERMINED**.
- A!=1 attribution: **PROVEN as a five-count run result**, not yet as five distinct semantic probes.
- Positive lookup: **not established**.
- First incorrect emulator state: **not proven**.
- Production fix: **none**.
- G74: not approved.

The next admissible step is a bounded fixed-address five-record census for the successful A!=1 command. It must capture AX before and after each `3823`, the selector/path, and the branch outcome, without adding a general interrupt hook.
