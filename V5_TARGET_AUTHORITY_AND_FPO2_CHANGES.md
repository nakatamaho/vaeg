# v5 target-authority, string-I/O, and FPO2 policy changes

## Authority status

The facts below are maintainer-provided ROM-analysis results. They are approved as the basis for the
prospective plan, but M60b/M60c must record ROM SHA-256, mapping, table boundaries, raw records,
decoding scripts, and evidence digests before applying repository classification changes.

## V30-side `0F` inventory

The monitor table begins at ROM address `0x66A8A`. Each record is `(mask, value, group)`. The decoded
inventory is:

| Encoding | Family |
|---|---|
| `0F10/11/18/19` | TEST1 |
| `0F12/13/1A/1B` | CLR1 |
| `0F14/15/1C/1D` | SET1 |
| `0F16/17/1E/1F` | NOT1 |
| `0F20/22/26` | ADD4S / SUB4S / CMP4S |
| `0F28/2A` | ROL4 / ROR4 |
| `0FFE imm8` | BRKFEM |
| `0FFF imm8` | BRKEM |

The complete table lacks `0F31/33/39/3B`. ROL4 is therefore a target implementation requirement,
not a target-support question. BRKFEM existence and encoding are established; vector handling,
destination mode, and return mechanism remain unresolved.

## Primary opcodes `6C-6F`

The uPD9002 target does not implement the V20/80186 string-I/O forms represented by primary opcodes
`6C`, `6D`, `6E`, and `6F`. Every selected structural form under these opcodes must ultimately be:

```text
top-level classification = known_target_gap
gap_kind = documented_silicon_absent
```

This is a structural target-authority correction. It must not be partitioned by observed pass/fail
outcome.

### Historical G43 reconciliation

G43 corrected an OUTS fixture and made 1,204 V20 records pass. After the correction, 6E and 6F still
had 417 and 224 failures. Preserve those artifacts and statements exactly, but interpret them as:

- improved fidelity to V20 silicon;
- not evidence that uPD9002 supports the opcodes;
- not uPD9002 progress;
- records to be reclassified out of the blocking target denominator, never reported as newly passing.

Do not revert the fixture correction. A V20 diagnostic profile may retain those cases.

## FPO1/FPO2 evidence rule

The monitor does not need generic strings `ESC`, `FPO1`, or `FPO2`: it stores individual 8087
mnemonics such as FADD and FMUL. D8-DF records begin near `0x66B3B`. Therefore generic-string absence
is explicitly **non-evidence** for FPO1/FPO2 absence.

The possible FPO2 encoding at primary opcodes 66/67 is unconfirmed. M60c must:

- inspect current SST classification, selected/executed counts, and `gap_kind` where applicable;
- decode the main opcode group table near `0x66900` and follow 66/67 to their handlers/groups;
- distinguish target support, target absence, and unresolved support;
- correct an unsupported `documented_silicon_absent` annotation only with exact governance;
- never infer passing from absence in the failure list.
