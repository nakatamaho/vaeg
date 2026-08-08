# M74l classifier control

## Identity

Branch: `topic/m74-va1-basic-command-hang`  \nStarting SHA: `511092eb9b7945807a6c35f5071e875ee04476ba`. Runtime used explicit `--model va`, trace-enabled build, deterministic guest-frame bounds, and `pcengine105-bootonly.d88`.

## PRINT 1: P2

`PRINT 1` was run alone from a fresh BASIC start. The deterministic run injected at frame 840 and reached the fixed boundary at frame 1260. The fixed-address result was:

| counter | value |
|---|---:|
| `391D` | 1 |
| `3976` | 1 |
| `397A` | 1 |
| `3816` | 1 |
| `3818` | 1 |
| `3821` | 1 |
| `3823` | 1 |
| `3831` | 1 |
| `3835` | 0 |
| `3837` | 0 |
| `3985` | 1 |
| `3983` | 0 |
| `002A` | 1 |
| `01E4` | 1 |

This is Case P2: `PRINT 1` reaches the same `383A` classifier, has a positive `3831` outcome, then reaches the `3985` side. The command did not reach a verified second prompt within the bound, but that later outcome is independent of the classifier result.

The result establishes the measured contract:

```text
positive probe -> 3831 / CF=0 from 3806
              -> 383A short-circuit / 3860 aggregate
              -> 397A branch taken
              -> 3985 side
```

Together with A=1 (`3823=4, 3831=0, 3835=4, 3837=0, 3983=1`), this proves the found/not-found-style aggregate for the measured non-escape paths. It does not prove that the lookup string was literally `PRINT`; the name/token and selector still require bounded state capture if needed.

## Additional controls

`LET A=1` and `DEFINT A-Z` were each run as single commands. Both produced the same classifier shape: `391D=1, 3976=1, 397A=1, 3823=1, 3831=1, 3835=0, 3837=0, 3985=1, 3983=0`. Their prompt completion was not used as the classifier verdict.

The standalone `DEFINT A-Z` result does not prove that a later `A=1` uses the changed default type; the two-command stateful test was not completed.

## D4 and remaining semantics

D4 is **PROVEN for the measured 383A classifier contract**: a positive probe reaches the `3985` side, while the all-negative A=1 path reaches `3983`. The evidence is the same-context `PRINT 1` run, not the `A!=1` escape path.

The semantic meaning of `FFFEh`, the actual token/bytes presented to INT97, selector namespace names, vector/handler/table ABI, and the first incorrect emulator-produced state remain separate open questions. No production fix was made.

## Evidence

- Diagnostic worker after the two added fixed counters: `efe027376f95645c274bfd3395ee9abdc507d2ea9de1e588c18872e23a24120cf3` (worker identity was recorded from the built artifact; exact run artifact remains outside Git).
- `PRINT 1` log SHA: `fda578e2fefbcba881defb671dcba8a651f4e4a6ee4492dc223122f537e39c57`
- `PRINT 1` TVRAM SHA: `97e1294b1f19f4e683d0a6e1daed44de7c43b744e8f865a515a6d11fd1ac9438`
- `DEFINT A-Z` log SHA: `4a7a52757650d9b4f6435a8da72b83515cb8a88c809172e9e8c57744e566ff69`
- `DEFINT A-Z` TVRAM SHA: `4c56f6e6c382d721785bfc19e73f5b7a950e81c7756ccb8d64d027a611ed50f7`

## Validation

Trace-enabled build passed after adding only fixed counters. `git diff --check` passed. Hosted CI was not run. Existing untracked diagnostic backups and `tools/__pycache__` remain untracked. G74 remains not approved.
