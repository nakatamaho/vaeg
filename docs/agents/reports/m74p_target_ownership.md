# M74p target ownership

## Identity

Branch: `topic/m74-va1-basic-command-hang`  
Starting SHA: `152b4de802d6ceee4c39bdb5f6e34ab3b9d14306`  
Worker after expanded capture: `72ab508b84d5a313594a16b7ea4e7ec8a2d54481f7570838faa80c91e89c927b`  
ROM `varom00.rom`: `656d81bc532ed0bb602d1eb7f03df27c74f841fb19c72ad07d47e79302ac814b`. Runtime used explicit VA model, trace-enabled Release build, deterministic frame bound, and the 1.05 boot-only image. G74 is not approved.

## Closed wrapper mechanism

The prior M74o result remains accepted. `PRINT 1` reaches `3988 RET` with stack `002A,0005,34C0`; `A=1` uses `3983 RET` with the same normalized continuation frame. Both enter `002A -> 0180 -> 01E4`, and `01E4` consumes `IP=0005, CS=34C0`. H1/H2/H3 are rejected; H4 is proven.

## H6 deferral-loop correction

The expanded `01E4` capture is an extension of the existing command-local one-shot. It uses the same bounded guest-memory read path already used for the prior 16-byte target capture. No new event class, INT hook, lifecycle watch, or per-instruction trace was added.

## Existing 01E4 seam capabilities

The one-shot reads the command-local RETF frame, computes the target, reads bounded target memory, and emits host-only data at run completion. M74p extended it to eight stack words, 256 target bytes, and four 256-byte continuation windows.

## Expanded-capture equivalence recheck

The expanded worker built successfully, but the full C1/C2 equivalence gate was not completed before the available execution window. Therefore expanded snapshot bytes are recorded as diagnostic observations but not promoted as a full seam-equivalence-qualified milestone claim. The prior accepted prompt TVRAM reference remains `c6ff29f6852e02e822ce3ea628817a0fbe15bbb832701d734b3d269ac43044b5`.

## Extended PRINT 1 01E4 capture

Command-local arm occurred for command 3 after the first prompt at frame 720 and injection at frame 840. `PRINT 1` reached `E000:01E4` before external containment. Capture log SHA-256: `dc29c30ade6cd7a788d93f5e3f8c72dea8ebf338b0ee7d7523add3699324cae4`.

```text
CS:IP before RETF = E000:01E4
SS:SP              = 7FE0:01F4  (RETF frame physical read)
stack words        = 0005,34C0,002A,33FD,0000,0000,0000,0000
word 0/1           = RETF IP/CS
word 2..7          = unclassified lower stack words
RETF target        = 34C0:0005
nominal linear     = 34C05h
```

The separate `3988` capture remains `SS:SP=7FE0:01F2`, words `002A,0005,34C0`.

## Continuation target 256-byte dump

The target-relative 256-byte block at `34C0:0005` is all zero. SHA-256:

```text
5341e6b2646979a70e57653007a1f310169421ec9bdd9f1a5648f75ade005af1
```

The first relevant bytes are `00 00 00 00 00 00 00 00`.

## Other continuation segment windows

At the same `01E4` event, the four windows were read with the same bounded side-effect-free read path:

| window | nominal base | SHA-256 | classification | byte offset 0005 |
|---|---:|---|---|---|
| `43B5:0000` | `43B50h` | `5341e6b2646979a70e57653007a1f310169421ec9bdd9f1a5648f75ade005af1` | all zero | `00` |
| `49FC:0000` | `49FC0h` | same | all zero | `00` |
| `75AB:0000` | `75AB0h` | same | all zero | `00` |
| `7F2D:0000` | `7F2D0h` | same | all zero | `00` |

`0005` is an exact continuation offset only for the proven `34BD` caller; no exact target offset is assigned to the other four windows. Outcome: no measured continuation window contained pre-existing non-zero code/data at this checkpoint. This weakens, but does not disprove, a broader pre-populated-target model.

## Terminal stack context

The captured stack supports the established frame derivation. It does not assign semantics to words 2-7 and does not identify an ownership module.

## Mapper/bank state at RETF

Not captured. The current one-shot did not include a complete inventory of production address-resolution inputs. This is the exact remaining mapping-state gap.

## VAEG mapping decision for 34C05h

Prior evidence shows VAEG returns zero-filled ordinary mapped memory at the computed target. The exact selected production mapper path/backing offset was not emitted by the current one-shot. No mapping override or production change was made.

## Continuation-window disposition

All five sampled windows are zero at the terminal checkpoint. This is a substantive negative result: the tested segment-base windows do not look like pre-installed continuation code/data. It does not distinguish absent installation, a later/other target, or an incorrect mapping without mapper state and hardware authority.

## Unique C0 34 ROM occurrence

The uniqueness lead was reverified in `varom00.rom`: `C0 34` occurs exactly once at file offset `0x277D8` (bank 2 offset `0x77D8`). The surrounding bytes are:

```text
2f3101bd303102bdde323101bf333101c0343101c2353102c5b7de373101cade383101cfd4393101d73a3101d83b3101
```

A bounded 16-bit decode at the byte boundary beginning `0x77C8` yields table-like/ambiguous data and does not establish a code entry point or a literal `34C0` pointer. No structurally supported consumer was identified. The lead remains unresolved and is not treated as causal evidence.

No aligned `05 00 C0 34` or `C0 34 05 00` occurrence was found in the ROM. Raw-byte coincidence is therefore not a proven far-pointer structure.

## DX=0005 provenance

The first producer remains unresolved. `DX=0005` is proven live at the wrapper boundary and is preserved into `3922 PUSH DX`; no admissible backward slice identified a first producer or a specific ABI boundary in this pass.

## A%=1 escape-side dynamic confirmation

Not completed with the expanded worker before external containment. It is not promoted as a current-worker result. Existing prior evidence predicts the balanced `3988 -> E000:34C0` path, but that historical row is kept separate from the M74p worker.

## A=1 current-worker reference

The expanded-worker A=1 rerun was externally preempted before its command-local summary. Existing A0 evidence remains separate and is not merged into this expanded-worker table.

## PC-88VA hardware mapping at 0x34C00

No authoritative rule plus terminal mapper-state application was completed. The required missing inputs are: the PC-88VA-specific mapping rule for `34C05h` in the captured VA1 state, and the exact VAEG production mapper state at `01E4`. No hardware/VAEG mismatch is asserted.

## Hardware versus VAEG mapping comparison

UNRESOLVED. Plain zero RAM observed in VAEG is not by itself evidence of a mapper defect.

## E4 dynamic source/destination differential

Not performed. E4 remains BLOCKED by the absence of a bounded installer source/destination capture at the installer event; it is no longer described merely as generic record-format uncertainty.

## Tracked reproducer identity

The tracked runner is `tools/m74-diagnostics/run_basic_case.sh`, introduced in M74o. It accepts worker, ROM root, boot disk, command, frame bound, script path, and output path and emits worker/disk identity fields. The M74p runtime used the equivalent five-line headless script; the expanded runtime row should be rerun through the tracked runner before final corpus use.

## Validation restoration

- Trace-enabled Release build: PASS (`cmake --build build/linux-release --target vaeg -j2`).
- `git diff --check`: PASS before commit.
- `check_encoding.py`: not rerun after the expanded source edit.
- `check_eol.py`, `check_case.py`: not run in this pass.
- Selftest: NOT RUN.
- ROM-less: NOT RUN.
- M68/M69/M70: NOT RUN.
- MinGW/cross-build: NOT RUN.
- Hosted CI: NOT RUN.

No persistent Git configuration was changed. External containment preempted the incomplete A=1/A%=1 reruns; those are not guest verdicts.

## Hypothesis table

- H1 `3983` is the failure boundary: **REJECTED**.
- H2 `3985` implies ordinary caller completion: **REJECTED AS GENERAL STATEMENT**.
- H3 `34C0=0` is contradictory: **REJECTED**.
- H4 non-escape positive/negative paths normalize to the same continuation frame: **PROVEN**.
- H5 escape/non-escape determines ordinary return versus continuation dispatch in this wrapper: **STATICALLY PROVEN; current-worker escape confirmation pending**.
- H6 ownership of `34C0:0005`: **PRIMARY UNRESOLVED CAUSAL BOUNDARY**.
- H7 VAEG maps `0x34C00` incorrectly: **UNRESOLVED**.
- H8 continuation state/module was never installed: **UNRESOLVED**.
- H9 unique `C0 34` ROM lead is causal: **UNRESOLVED STATIC LEAD**.

## First incorrect emulator-produced state

None proven. The expanded capture demonstrates zero-filled target/windows but does not establish the expected hardware resource or terminal mapping state.

## Production fix

None.

## Remaining gaps

Complete the expanded-capture equivalence gate, current-worker `A%=1` and `A=1` rows through the tracked runner, mapper state and hardware authority, E4 bounded source/destination differential, and full validation. These are specific blockers; no generic deferral claim is used.

## Changed files

- `cpu/upd9002/upd9002_trace.c`
- `docs/agents/reports/m74p_target_ownership.md`

## Worktree status

Pre-existing untracked files remain `cpu/upd9002/upd9002_trace.c.orig`, `cpu/upd9002/upd9002_trace.c.rej.orig`, and `tools/__pycache__/`. No ROM, disk, generated worker, raw log, or private asset was staged.

## Hosted CI status

NOT RUN.

## G74 status

NOT APPROVED.
