# M74c VA1 default-assignment contract adjudication

## Status and identities

M74c is diagnostic and documentation work. It does not approve G74 and
contains no production correction.

| Item | Value |
| --- | --- |
| Branch | `topic/m74-va1-basic-command-hang` |
| Starting report SHA | `c59a9b72555ddfd5bc893a2afff12d429025d502` |
| Approved G73 predecessor | `766a132ff6d66e335fe9bb1d0082d777a4a8fe14` |
| Task-authority SHA | `6cf3942f1637f3ce002affafaa379940ad59e716` |
| Diagnostic commit | `7620decdeb208b62665fda22ca2c6fec82ab0e09` |
| Production correction | None |
| G74 | Not approved |

Private ROM and disk identities are omitted. The same local PC-Engine image and
ROM set were used for the primary VA1/VA2 comparison. No private asset, raw
trace, screenshot, or generated binary is tracked by Git.

## M74b corrections

The old `PROVEN` label for an intended ABI is withdrawn. Six invocations proved
only deterministic stack construction. M74c adjudicates the contract below.
A bounded run without its checkpoint is never success. The old bounded
`A!=1` result is superseded by a prompt-aware run with a cleared and reappeared
second `Ok`. No ownership or loader premise is assigned to `34C0:0005`.

## Claim register

| ID | Verdict | Deciding record |
| --- | --- | --- |
| C1 | **PROVEN** | VA1 T4 slot `1040:0AC3` is `CB 00 00 00 00`: `STUB`, not `LIVE`. |
| C2 | **UNDETERMINED** | VA2 reaches PC-Engine v1.1 `Ready`, but `BASIC` is rejected before `A=1`; there is no equivalent terminal transfer. |
| C3 | **PROVEN** | All five terminal-reachable real call sites preserve post-CALL IP below incoming DX; `3983 RET` consumes incoming SI and `01E4 RETF` consumes `IP=DX`, `CS=post-CALL IP`. |
| C4 | **PROVEN** | `3835 STC` and `3860 CMC` are the final CF writers before `397A`; flags are `0244 -> 0245 -> 0244`, and all 500 selected SST v20 `F5` cases pass. |
| C5a | **UNDETERMINED / not executed** | D2 was prohibited because C2 did not show a populated VA2 target. |
| C5b | **UNDETERMINED / not executed** | D2 was prohibited because C2 did not show a populated VA2 target. |

C1 proves no defect: an optional service can legitimately remain a stub. C3
proves the static transfer contract under the task's all-callsites rule, but
not that an implied target should be populated.

## Diagnostic scope

Commit `7620decdeb208b62665fda22ca2c6fec82ab0e09` adds disabled-by-default
`VAEG_M74_VECTOR_WATCH`, 166-slot snapshots, first-touch/LIVE records, every
five-byte `EA` write-seam notification, vector-call and carry probes, and
prompt-signature/count/timeout controls. It changes only:

```text
cpu/upd9002/upd9002_trace.c
sdl2/headless_input.c
sdl2/headless_input.h
sdl2/np2.c
```

No guest-visible CPU, mapper, memory, FDC, or timing semantics changed.

## M74c-A: `1040` vector table

### Initializer and extent

```text
2C24 PUSH DS          2C25 MOV AX,1040       2C28 MOV DS,AX
2C2A MOV BX,0A00     2C2D MOV CX,009C       2C30 MOV BYTE [BX],CB
2C33 ADD BX,0005     2C36 LOOP 2C30         2C38 MOV BX,0280
2C3B MOV CX,000A     2C3E MOV BYTE [BX],CB  2C41 ADD BX,0005
2C44 LOOP 2C3E       2C46 POP DS            2C47 RETF
```

The code proves a five-byte stride. The main table has 156 slots at `0A00` to
`0D07`; the auxiliary table has 10 at `0280` to `02AD`. Dynamic initialization
covers exactly those spans. Stub first-byte writers are `E000:2C30 C6 07 CB`
and `E000:2C3E C6 07 CB`.

### Complete VA1 census

The following table is the complete 166-slot dump, not a sampled summary.
Every LIVE row's final column includes its target and first writer. Every LIVE
slot was first written by `19E3:C7EB A5` (`MOVSW`). T1 and T4 are identical.

| Table | Index | Offset | T0 reset | T1 first prompt | T4 before A=1 | T1 target / first writer |
| --- | ---: | ---: | --- | --- | --- | --- |
| aux | 0 | 0280 | OTHER 0000000000 | LIVE ea7f9ee319 | LIVE ea7f9ee319 | 19e3:9e7f / 19e3:c7eb a5 (MOVSW) |
| aux | 1 | 0285 | OTHER 0000000000 | STUB cb00000000 | STUB cb00000000 | none |
| aux | 2 | 028A | OTHER 0000000000 | STUB cb00000000 | STUB cb00000000 | none |
| aux | 3 | 028F | OTHER 0000000000 | STUB cb00000000 | STUB cb00000000 | none |
| aux | 4 | 0294 | OTHER 0000000000 | STUB cb00000000 | STUB cb00000000 | none |
| aux | 5 | 0299 | OTHER 0000000000 | STUB cb00000000 | STUB cb00000000 | none |
| aux | 6 | 029E | OTHER 0000000000 | STUB cb00000000 | STUB cb00000000 | none |
| aux | 7 | 02A3 | OTHER 0000000000 | STUB cb00000000 | STUB cb00000000 | none |
| aux | 8 | 02A8 | OTHER 0000000000 | STUB cb00000000 | STUB cb00000000 | none |
| aux | 9 | 02AD | OTHER 0000000000 | STUB cb00000000 | STUB cb00000000 | none |
| main | 0 | 0A00 | OTHER 0000000000 | STUB cb00000000 | STUB cb00000000 | none |
| main | 1 | 0A05 | OTHER 0000000000 | STUB cb00000000 | STUB cb00000000 | none |
| main | 2 | 0A0A | OTHER 0000000000 | STUB cb00000000 | STUB cb00000000 | none |
| main | 3 | 0A0F | OTHER 0000000000 | LIVE eafe85e319 | LIVE eafe85e319 | 19e3:85fe / 19e3:c7eb a5 (MOVSW) |
| main | 4 | 0A14 | OTHER 0000000000 | LIVE ea6085e319 | LIVE ea6085e319 | 19e3:8560 / 19e3:c7eb a5 (MOVSW) |
| main | 5 | 0A19 | OTHER 0000000000 | LIVE ea2186e319 | LIVE ea2186e319 | 19e3:8621 / 19e3:c7eb a5 (MOVSW) |
| main | 6 | 0A1E | OTHER 0000000000 | STUB cb00000000 | STUB cb00000000 | none |
| main | 7 | 0A23 | OTHER 0000000000 | STUB cb00000000 | STUB cb00000000 | none |
| main | 8 | 0A28 | OTHER 0000000000 | LIVE ea3ba3e319 | LIVE ea3ba3e319 | 19e3:a33b / 19e3:c7eb a5 (MOVSW) |
| main | 9 | 0A2D | OTHER 0000000000 | STUB cb00000000 | STUB cb00000000 | none |
| main | 10 | 0A32 | OTHER 0000000000 | STUB cb00000000 | STUB cb00000000 | none |
| main | 11 | 0A37 | OTHER 0000000000 | STUB cb00000000 | STUB cb00000000 | none |
| main | 12 | 0A3C | OTHER 0000000000 | STUB cb00000000 | STUB cb00000000 | none |
| main | 13 | 0A41 | OTHER 0000000000 | STUB cb00000000 | STUB cb00000000 | none |
| main | 14 | 0A46 | OTHER 0000000000 | LIVE ea1a87e319 | LIVE ea1a87e319 | 19e3:871a / 19e3:c7eb a5 (MOVSW) |
| main | 15 | 0A4B | OTHER 0000000000 | LIVE eab992e319 | LIVE eab992e319 | 19e3:92b9 / 19e3:c7eb a5 (MOVSW) |
| main | 16 | 0A50 | OTHER 0000000000 | LIVE ea0792e319 | LIVE ea0792e319 | 19e3:9207 / 19e3:c7eb a5 (MOVSW) |
| main | 17 | 0A55 | OTHER 0000000000 | STUB cb00000000 | STUB cb00000000 | none |
| main | 18 | 0A5A | OTHER 0000000000 | STUB cb00000000 | STUB cb00000000 | none |
| main | 19 | 0A5F | OTHER 0000000000 | LIVE eadea2e319 | LIVE eadea2e319 | 19e3:a2de / 19e3:c7eb a5 (MOVSW) |
| main | 20 | 0A64 | OTHER 0000000000 | STUB cb00000000 | STUB cb00000000 | none |
| main | 21 | 0A69 | OTHER 0000000000 | STUB cb00000000 | STUB cb00000000 | none |
| main | 22 | 0A6E | OTHER 0000000000 | STUB cb00000000 | STUB cb00000000 | none |
| main | 23 | 0A73 | OTHER 0000000000 | STUB cb00000000 | STUB cb00000000 | none |
| main | 24 | 0A78 | OTHER 0000000000 | STUB cb00000000 | STUB cb00000000 | none |
| main | 25 | 0A7D | OTHER 0000000000 | STUB cb00000000 | STUB cb00000000 | none |
| main | 26 | 0A82 | OTHER 0000000000 | STUB cb00000000 | STUB cb00000000 | none |
| main | 27 | 0A87 | OTHER 0000000000 | STUB cb00000000 | STUB cb00000000 | none |
| main | 28 | 0A8C | OTHER 0000000000 | STUB cb00000000 | STUB cb00000000 | none |
| main | 29 | 0A91 | OTHER 0000000000 | LIVE eafd9ce319 | LIVE eafd9ce319 | 19e3:9cfd / 19e3:c7eb a5 (MOVSW) |
| main | 30 | 0A96 | OTHER 0000000000 | STUB cb00000000 | STUB cb00000000 | none |
| main | 31 | 0A9B | OTHER 0000000000 | STUB cb00000000 | STUB cb00000000 | none |
| main | 32 | 0AA0 | OTHER 0000000000 | STUB cb00000000 | STUB cb00000000 | none |
| main | 33 | 0AA5 | OTHER 0000000000 | LIVE eaf6a2e319 | LIVE eaf6a2e319 | 19e3:a2f6 / 19e3:c7eb a5 (MOVSW) |
| main | 34 | 0AAA | OTHER 0000000000 | STUB cb00000000 | STUB cb00000000 | none |
| main | 35 | 0AAF | OTHER 0000000000 | STUB cb00000000 | STUB cb00000000 | none |
| main | 36 | 0AB4 | OTHER 0000000000 | STUB cb00000000 | STUB cb00000000 | none |
| main | 37 | 0AB9 | OTHER 0000000000 | STUB cb00000000 | STUB cb00000000 | none |
| main | 38 | 0ABE | OTHER 0000000000 | LIVE eaa9a1e319 | LIVE eaa9a1e319 | 19e3:a1a9 / 19e3:c7eb a5 (MOVSW) |
| main | 39 | 0AC3 | OTHER 0000000000 | STUB cb00000000 | STUB cb00000000 | none |
| main | 40 | 0AC8 | OTHER 0000000000 | STUB cb00000000 | STUB cb00000000 | none |
| main | 41 | 0ACD | OTHER 0000000000 | STUB cb00000000 | STUB cb00000000 | none |
| main | 42 | 0AD2 | OTHER 0000000000 | LIVE eaf191e319 | LIVE eaf191e319 | 19e3:91f1 / 19e3:c7eb a5 (MOVSW) |
| main | 43 | 0AD7 | OTHER 0000000000 | STUB cb00000000 | STUB cb00000000 | none |
| main | 44 | 0ADC | OTHER 0000000000 | STUB cb00000000 | STUB cb00000000 | none |
| main | 45 | 0AE1 | OTHER 0000000000 | LIVE ea0a9be319 | LIVE ea0a9be319 | 19e3:9b0a / 19e3:c7eb a5 (MOVSW) |
| main | 46 | 0AE6 | OTHER 0000000000 | STUB cb00000000 | STUB cb00000000 | none |
| main | 47 | 0AEB | OTHER 0000000000 | STUB cb00000000 | STUB cb00000000 | none |
| main | 48 | 0AF0 | OTHER 0000000000 | STUB cb00000000 | STUB cb00000000 | none |
| main | 49 | 0AF5 | OTHER 0000000000 | STUB cb00000000 | STUB cb00000000 | none |
| main | 50 | 0AFA | OTHER 0000000000 | STUB cb00000000 | STUB cb00000000 | none |
| main | 51 | 0AFF | OTHER 0000000000 | STUB cb00000000 | STUB cb00000000 | none |
| main | 52 | 0B04 | OTHER 0000000000 | STUB cb00000000 | STUB cb00000000 | none |
| main | 53 | 0B09 | OTHER 0000000000 | STUB cb00000000 | STUB cb00000000 | none |
| main | 54 | 0B0E | OTHER 0000000000 | STUB cb00000000 | STUB cb00000000 | none |
| main | 55 | 0B13 | OTHER 0000000000 | STUB cb00000000 | STUB cb00000000 | none |
| main | 56 | 0B18 | OTHER 0000000000 | LIVE ea31a0e319 | LIVE ea31a0e319 | 19e3:a031 / 19e3:c7eb a5 (MOVSW) |
| main | 57 | 0B1D | OTHER 0000000000 | LIVE eacb9ce319 | LIVE eacb9ce319 | 19e3:9ccb / 19e3:c7eb a5 (MOVSW) |
| main | 58 | 0B22 | OTHER 0000000000 | STUB cb00000000 | STUB cb00000000 | none |
| main | 59 | 0B27 | OTHER 0000000000 | LIVE eaf19be319 | LIVE eaf19be319 | 19e3:9bf1 / 19e3:c7eb a5 (MOVSW) |
| main | 60 | 0B2C | OTHER 0000000000 | STUB cb00000000 | STUB cb00000000 | none |
| main | 61 | 0B31 | OTHER 0000000000 | STUB cb00000000 | STUB cb00000000 | none |
| main | 62 | 0B36 | OTHER 0000000000 | LIVE ea749ce319 | LIVE ea749ce319 | 19e3:9c74 / 19e3:c7eb a5 (MOVSW) |
| main | 63 | 0B3B | OTHER 0000000000 | STUB cb00000000 | STUB cb00000000 | none |
| main | 64 | 0B40 | OTHER 0000000000 | STUB cb00000000 | STUB cb00000000 | none |
| main | 65 | 0B45 | OTHER 0000000000 | STUB cb00000000 | STUB cb00000000 | none |
| main | 66 | 0B4A | OTHER 0000000000 | STUB cb00000000 | STUB cb00000000 | none |
| main | 67 | 0B4F | OTHER 0000000000 | STUB cb00000000 | STUB cb00000000 | none |
| main | 68 | 0B54 | OTHER 0000000000 | LIVE ea0094e319 | LIVE ea0094e319 | 19e3:9400 / 19e3:c7eb a5 (MOVSW) |
| main | 69 | 0B59 | OTHER 0000000000 | STUB cb00000000 | STUB cb00000000 | none |
| main | 70 | 0B5E | OTHER 0000000000 | LIVE ea1397e319 | LIVE ea1397e319 | 19e3:9713 / 19e3:c7eb a5 (MOVSW) |
| main | 71 | 0B63 | OTHER 0000000000 | LIVE ea129de319 | LIVE ea129de319 | 19e3:9d12 / 19e3:c7eb a5 (MOVSW) |
| main | 72 | 0B68 | OTHER 0000000000 | STUB cb00000000 | STUB cb00000000 | none |
| main | 73 | 0B6D | OTHER 0000000000 | LIVE ea4986e319 | LIVE ea4986e319 | 19e3:8649 / 19e3:c7eb a5 (MOVSW) |
| main | 74 | 0B72 | OTHER 0000000000 | STUB cb00000000 | STUB cb00000000 | none |
| main | 75 | 0B77 | OTHER 0000000000 | STUB cb00000000 | STUB cb00000000 | none |
| main | 76 | 0B7C | OTHER 0000000000 | STUB cb00000000 | STUB cb00000000 | none |
| main | 77 | 0B81 | OTHER 0000000000 | STUB cb00000000 | STUB cb00000000 | none |
| main | 78 | 0B86 | OTHER 0000000000 | LIVE eaac92e319 | LIVE eaac92e319 | 19e3:92ac / 19e3:c7eb a5 (MOVSW) |
| main | 79 | 0B8B | OTHER 0000000000 | LIVE ea8e91e319 | LIVE ea8e91e319 | 19e3:918e / 19e3:c7eb a5 (MOVSW) |
| main | 80 | 0B90 | OTHER 0000000000 | STUB cb00000000 | STUB cb00000000 | none |
| main | 81 | 0B95 | OTHER 0000000000 | STUB cb00000000 | STUB cb00000000 | none |
| main | 82 | 0B9A | OTHER 0000000000 | STUB cb00000000 | STUB cb00000000 | none |
| main | 83 | 0B9F | OTHER 0000000000 | LIVE ea048ce319 | LIVE ea048ce319 | 19e3:8c04 / 19e3:c7eb a5 (MOVSW) |
| main | 84 | 0BA4 | OTHER 0000000000 | LIVE ea6f97e319 | LIVE ea6f97e319 | 19e3:976f / 19e3:c7eb a5 (MOVSW) |
| main | 85 | 0BA9 | OTHER 0000000000 | LIVE eab297e319 | LIVE eab297e319 | 19e3:97b2 / 19e3:c7eb a5 (MOVSW) |
| main | 86 | 0BAE | OTHER 0000000000 | STUB cb00000000 | STUB cb00000000 | none |
| main | 87 | 0BB3 | OTHER 0000000000 | STUB cb00000000 | STUB cb00000000 | none |
| main | 88 | 0BB8 | OTHER 0000000000 | STUB cb00000000 | STUB cb00000000 | none |
| main | 89 | 0BBD | OTHER 0000000000 | STUB cb00000000 | STUB cb00000000 | none |
| main | 90 | 0BC2 | OTHER 0000000000 | STUB cb00000000 | STUB cb00000000 | none |
| main | 91 | 0BC7 | OTHER 0000000000 | STUB cb00000000 | STUB cb00000000 | none |
| main | 92 | 0BCC | OTHER 0000000000 | LIVE eaf097e319 | LIVE eaf097e319 | 19e3:97f0 / 19e3:c7eb a5 (MOVSW) |
| main | 93 | 0BD1 | OTHER 0000000000 | STUB cb00000000 | STUB cb00000000 | none |
| main | 94 | 0BD6 | OTHER 0000000000 | LIVE eadd8de319 | LIVE eadd8de319 | 19e3:8ddd / 19e3:c7eb a5 (MOVSW) |
| main | 95 | 0BDB | OTHER 0000000000 | STUB cb00000000 | STUB cb00000000 | none |
| main | 96 | 0BE0 | OTHER 0000000000 | STUB cb00000000 | STUB cb00000000 | none |
| main | 97 | 0BE5 | OTHER 0000000000 | LIVE ea508de319 | LIVE ea508de319 | 19e3:8d50 / 19e3:c7eb a5 (MOVSW) |
| main | 98 | 0BEA | OTHER 0000000000 | STUB cb00000000 | STUB cb00000000 | none |
| main | 99 | 0BEF | OTHER 0000000000 | STUB cb00000000 | STUB cb00000000 | none |
| main | 100 | 0BF4 | OTHER 0000000000 | STUB cb00000000 | STUB cb00000000 | none |
| main | 101 | 0BF9 | OTHER 0000000000 | LIVE ea42a3e319 | LIVE ea42a3e319 | 19e3:a342 / 19e3:c7eb a5 (MOVSW) |
| main | 102 | 0BFE | OTHER 0000000000 | STUB cb00000000 | STUB cb00000000 | none |
| main | 103 | 0C03 | OTHER 0000000000 | STUB cb00000000 | STUB cb00000000 | none |
| main | 104 | 0C08 | OTHER 0000000000 | STUB cb00000000 | STUB cb00000000 | none |
| main | 105 | 0C0D | OTHER 0000000000 | STUB cb00000000 | STUB cb00000000 | none |
| main | 106 | 0C12 | OTHER 0000000000 | STUB cb00000000 | STUB cb00000000 | none |
| main | 107 | 0C17 | OTHER 0000000000 | STUB cb00000000 | STUB cb00000000 | none |
| main | 108 | 0C1C | OTHER 0000000000 | STUB cb00000000 | STUB cb00000000 | none |
| main | 109 | 0C21 | OTHER 0000000000 | STUB cb00000000 | STUB cb00000000 | none |
| main | 110 | 0C26 | OTHER 0000000000 | STUB cb00000000 | STUB cb00000000 | none |
| main | 111 | 0C2B | OTHER 0000000000 | STUB cb00000000 | STUB cb00000000 | none |
| main | 112 | 0C30 | OTHER 0000000000 | STUB cb00000000 | STUB cb00000000 | none |
| main | 113 | 0C35 | OTHER 0000000000 | STUB cb00000000 | STUB cb00000000 | none |
| main | 114 | 0C3A | OTHER 0000000000 | LIVE eac7a7e319 | LIVE eac7a7e319 | 19e3:a7c7 / 19e3:c7eb a5 (MOVSW) |
| main | 115 | 0C3F | OTHER 0000000000 | STUB cb00000000 | STUB cb00000000 | none |
| main | 116 | 0C44 | OTHER 0000000000 | STUB cb00000000 | STUB cb00000000 | none |
| main | 117 | 0C49 | OTHER 0000000000 | STUB cb00000000 | STUB cb00000000 | none |
| main | 118 | 0C4E | OTHER 0000000000 | STUB cb00000000 | STUB cb00000000 | none |
| main | 119 | 0C53 | OTHER 0000000000 | STUB cb00000000 | STUB cb00000000 | none |
| main | 120 | 0C58 | OTHER 0000000000 | STUB cb00000000 | STUB cb00000000 | none |
| main | 121 | 0C5D | OTHER 0000000000 | STUB cb00000000 | STUB cb00000000 | none |
| main | 122 | 0C62 | OTHER 0000000000 | LIVE ea8698e319 | LIVE ea8698e319 | 19e3:9886 / 19e3:c7eb a5 (MOVSW) |
| main | 123 | 0C67 | OTHER 0000000000 | LIVE ea3599e319 | LIVE ea3599e319 | 19e3:9935 / 19e3:c7eb a5 (MOVSW) |
| main | 124 | 0C6C | OTHER 0000000000 | STUB cb00000000 | STUB cb00000000 | none |
| main | 125 | 0C71 | OTHER 0000000000 | STUB cb00000000 | STUB cb00000000 | none |
| main | 126 | 0C76 | OTHER 0000000000 | STUB cb00000000 | STUB cb00000000 | none |
| main | 127 | 0C7B | OTHER 0000000000 | STUB cb00000000 | STUB cb00000000 | none |
| main | 128 | 0C80 | OTHER 0000000000 | STUB cb00000000 | STUB cb00000000 | none |
| main | 129 | 0C85 | OTHER 0000000000 | LIVE ea3b84e319 | LIVE ea3b84e319 | 19e3:843b / 19e3:c7eb a5 (MOVSW) |
| main | 130 | 0C8A | OTHER 0000000000 | STUB cb00000000 | STUB cb00000000 | none |
| main | 131 | 0C8F | OTHER 0000000000 | STUB cb00000000 | STUB cb00000000 | none |
| main | 132 | 0C94 | OTHER 0000000000 | LIVE ea4b9ee319 | LIVE ea4b9ee319 | 19e3:9e4b / 19e3:c7eb a5 (MOVSW) |
| main | 133 | 0C99 | OTHER 0000000000 | STUB cb00000000 | STUB cb00000000 | none |
| main | 134 | 0C9E | OTHER 0000000000 | STUB cb00000000 | STUB cb00000000 | none |
| main | 135 | 0CA3 | OTHER 0000000000 | LIVE ea569be319 | LIVE ea569be319 | 19e3:9b56 / 19e3:c7eb a5 (MOVSW) |
| main | 136 | 0CA8 | OTHER 0000000000 | LIVE eab187e319 | LIVE eab187e319 | 19e3:87b1 / 19e3:c7eb a5 (MOVSW) |
| main | 137 | 0CAD | OTHER 0000000000 | STUB cb00000000 | STUB cb00000000 | none |
| main | 138 | 0CB2 | OTHER 0000000000 | LIVE eae987e319 | LIVE eae987e319 | 19e3:87e9 / 19e3:c7eb a5 (MOVSW) |
| main | 139 | 0CB7 | OTHER 0000000000 | STUB cb00000000 | STUB cb00000000 | none |
| main | 140 | 0CBC | OTHER 0000000000 | STUB cb00000000 | STUB cb00000000 | none |
| main | 141 | 0CC1 | OTHER 0000000000 | STUB cb00000000 | STUB cb00000000 | none |
| main | 142 | 0CC6 | OTHER 0000000000 | STUB cb00000000 | STUB cb00000000 | none |
| main | 143 | 0CCB | OTHER 0000000000 | STUB cb00000000 | STUB cb00000000 | none |
| main | 144 | 0CD0 | OTHER 0000000000 | STUB cb00000000 | STUB cb00000000 | none |
| main | 145 | 0CD5 | OTHER 0000000000 | STUB cb00000000 | STUB cb00000000 | none |
| main | 146 | 0CDA | OTHER 0000000000 | STUB cb00000000 | STUB cb00000000 | none |
| main | 147 | 0CDF | OTHER 0000000000 | STUB cb00000000 | STUB cb00000000 | none |
| main | 148 | 0CE4 | OTHER 0000000000 | LIVE ea8ca1e319 | LIVE ea8ca1e319 | 19e3:a18c / 19e3:c7eb a5 (MOVSW) |
| main | 149 | 0CE9 | OTHER 0000000000 | STUB cb00000000 | STUB cb00000000 | none |
| main | 150 | 0CEE | OTHER 0000000000 | STUB cb00000000 | STUB cb00000000 | none |
| main | 151 | 0CF3 | OTHER 0000000000 | LIVE eadba0e319 | LIVE eadba0e319 | 19e3:a0db / 19e3:c7eb a5 (MOVSW) |
| main | 152 | 0CF8 | OTHER 0000000000 | LIVE ea1f8ce319 | LIVE ea1f8ce319 | 19e3:8c1f / 19e3:c7eb a5 (MOVSW) |
| main | 153 | 0CFD | OTHER 0000000000 | STUB cb00000000 | STUB cb00000000 | none |
| main | 154 | 0D02 | OTHER 0000000000 | STUB cb00000000 | STUB cb00000000 | none |
| main | 155 | 0D07 | OTHER 0000000000 | STUB cb00000000 | STUB cb00000000 | none |

The invoked slot is therefore:

```text
T0  1040:0AC3 = 00 00 00 00 00  OTHER
T1  1040:0AC3 = CB 00 00 00 00  STUB
T4  1040:0AC3 = CB 00 00 00 00  STUB
```

The complete VA1 `EA` write-seam stream has 18,384 records and 11,202 unique
textual records. Nested seams can report one architectural write more than
once; 18,384 is not a unique-instruction count. No host-side bulk `EA` write
was observed. The complete filtered stream remains outside Git and is hashed
below.

### Exact call bytes and record correction

```text
E000:391D  9A C3 0A 40 10  CALL FAR 1040:0AC3
E000:39AD  9A C8 0A 40 10  CALL FAR 1040:0AC8
```

The earlier project note assigning `0AC8` to `391D` was wrong; it conflated the
separate `39AD` call. M74b's `0AC3` listing was correct. The VA2 parser at
`E000:077A` also contains `9A C3 0A 40 10`.

### VA2 census

| Stage | LIVE | STUB | OTHER | `0AC3` |
| --- | ---: | ---: | ---: | --- |
| T0 reset | 0 | 0 | 166 | `OTHER 00 00 00 00 00` |
| PC-Engine `Ready`, before command 1 | 0 | 166 | 0 | `STUB CB 00 00 00 00` |
| T4 | unavailable | unavailable | unavailable | `A=1` was not injected |

No VA2 LIVE slot or first-LIVE writer was observed. The VA2 `EA` stream has
12,010 records and 9,973 unique textual records, with no host-side bulk `EA`
record. This negative result does not prove that VA2 can never install the
table because BASIC was not entered.

## M74c-B: prompt-aware control and matrix

### VA2

The identical image reaches PC-Engine v1.1 `Ready`. Scripted `BASIC` is echoed
and rejected as an incorrect command. BASIC `Ok` does not appear by frame 1320;
the prompt timeout exits 1. Thus `A=1` is not injected, the equivalent routine
is not entered, and DX, SI, target, target bytes, and guest-instruction count
are unavailable. The instruction trace was armed for command 3, which never
occurred. This is a harness/software-entry limitation, not a VA2 semantic
result. C2 is UNDETERMINED.

### VA1 versions 1.00, 1.05, and 1.10

Each cell is from a clean run. `Second Ok` requires the first prompt to be
observed, cleared, and observed again. `Harness failure` means no required next
prompt by frame 1560 and is not a semantic result.

| Command sequence | 1.00 | 1.05 | 1.10 |
| --- | --- | --- | --- |
| `PAINT(0,0),3` | Second Ok | Second Ok | Second Ok |
| `PRINT 1` | Harness failure | Harness failure | Harness failure |
| `? 1` | Second Ok | Second Ok | Second Ok |
| `A=1` | bad continuation / no Ok | bad continuation / no Ok | bad continuation / no Ok |
| `LET A=1` | Harness failure | Harness failure | Harness failure |
| `A%=1` | Second Ok | Second Ok | Second Ok |
| `A!=1` | Second Ok | Second Ok | Second Ok |
| `A#=1` | Second Ok | Second Ok | Second Ok |
| `A$=""` | Second Ok | Second Ok | Second Ok |
| `DEFINT A-Z`; `A=1` | declaration timeout; no `A=1` | same | same |
| `DEFSNG A-Z`; `A=1` | declaration timeout; no `A=1` | same | same |
| `DEFDBL A-Z`; `A=1` | declaration timeout; no `A=1` | same | same |
| `10 A=1`; `20 PRINT "DONE"`; `RUN` | first-line timeout | same | same |

Successful cases usually reach frame 960; `PAINT` reaches 960, 966, and 971.
Free-byte values are 348,856, 321,624, and 336,520. The classifications match
across versions, so no version split is established. Declaration and program
rows are explicitly harness failures because later commands were not injected.

```text
A=1
  entry seq=518568 caller=E000:34BD return_ip=34C0 DX=0005 SI=002A
  CF decision seq=521474 FLAGS=0244 CF=0 post=E000:397C
  helper success seq=521480 post=E000:002A
  RETF seq=521515 target=34C0:0005, first 16 bytes all zero
  no second Ok by frame 1560

A!=1
  entry seq=607086, same caller, DX, and SI
  helper failure seq=607157 post=E000:34C0 FLAGS=0245
  second Ok at frame 960; trace_steps=1016400
```

`A!=1` never reaches `3973`, `383A`, or `397A`, so no “last CF writer before
397A” exists for that trace. Its failure return sets CF at `3987 STC`.

## M74c-C: static contract adjudication

### Full `391D` to `3988` disassembly

```text
0000391D  9AC30A4010        call word 0x1040:word 0xac3
00003922  52                push dx
00003923  56                push si
00003924  33C9              xor cx,cx
00003926  33D2              xor dx,dx
00003928  E87DFE            call 0x37a8
0000392B  7304              jnc 0x3931
0000392D  3C2A              cmp al,0x2a
0000392F  7554              jnz 0x3985
00003931  0AE4              or ah,ah
00003933  B80100            mov ax,0x1
00003936  7401              jz 0x3939
00003938  40                inc ax
00003939  03F0              add si,ax
0000393B  83FA28            cmp dx,0x28
0000393E  7303              jnc 0x3943
00003940  03C8              add cx,ax
00003942  42                inc dx
00003943  E871FE            call 0x37b7
00003946  73E9              jnc 0x3931
00003948  3C25              cmp al,0x25
0000394A  7439              jz 0x3985
0000394C  3C21              cmp al,0x21
0000394E  7435              jz 0x3985
00003950  3C23              cmp al,0x23
00003952  7431              jz 0x3985
00003954  3C24              cmp al,0x24
00003956  742D              jz 0x3985
00003958  3C28              cmp al,0x28
0000395A  7429              jz 0x3985
0000395C  5B                pop bx
0000395D  53                push bx
0000395E  51                push cx
0000395F  803F2A            cmp byte [bx],0x2a
00003962  7504              jnz 0x3968
00003964  43                inc bx
00003965  49                dec cx
00003966  741C              jz 0x3984
00003968  83F902            cmp cx,0x2
0000396B  7006              jo 0x3973
0000396D  813F464E          cmp word [bx],0x4e46
00003971  7411              jz 0x3984
00003973  56                push si
00003974  8BF3              mov si,bx
00003976  E8C1FE            call 0x383a
00003979  1E                push ds
0000397A  7208              jc 0x3984
0000397C  59                pop cx
0000397D  5B                pop bx
0000397E  58                pop ax
0000397F  F8                clc
00003980  B88100            mov ax,0x81
00003983  C3                ret
00003984  58                pop ax
00003985  5E                pop si
00003986  5A                pop dx
00003987  F9                stc
00003988  C3                ret
```

Static stack delta, relative to entry after the near CALL has pushed its return
word:

| Instruction | Delta | Meaning |
| --- | ---: | --- |
| `391D CALL FAR` | 0 after return | slot call returns normally |
| `3922 PUSH DX` | -1 word | preserves incoming DX |
| `3923 PUSH SI` | -1 word | preserves incoming SI |
| all loop operations and nested CALLs through `395B` | 0 after return | no retained word |
| `395C POP BX`; `395D PUSH BX` | 0 net | moves then restores saved SI |
| `395E PUSH CX` | -1 word | parser local |
| `3973 PUSH SI` | -1 word | parser local |
| `3976 CALL 383A` | 0 after return | no retained word |
| `3979 PUSH DS` | -1 word | parser local |
| `397C`, `397D`, `397E` | +3 words | consume DS, parser SI, parser CX |
| `3983 RET` | +1 word | consumes original SI as near IP |
| failure `3984`--`3988` | +4 words total | consumes locals, SI, DX, normal near return |

All unlisted instructions have zero static stack delta. The success path's
remaining word is original DX from `3922`, not an unexplained loop artifact:

```text
after CALL:    [34C0]
after PUSH DX: [0005,34C0]
after PUSH SI: [002A,0005,34C0]
after 3983:    [0005,34C0], next IP=002A
after 01E4:    IP=0005, CS=34C0
```

### Full `0000` to `0060` disassembly

```text
00000000  EB37              jmp 0x39
00000002  90                nop
00000003  EB60              jmp 0x65
00000005  90                nop
00000006  EB6A              jmp 0x72
00000008  90                nop
00000009  EB74              jmp 0x7f
0000000B  90                nop
0000000C  EB7E              jmp 0x8c
0000000E  90                nop
0000000F  E98700            jmp 0x99
00000012  E99000            jmp 0xa5
00000015  E9A700            jmp 0xbf
00000018  E9BF00            jmp 0xda
0000001B  E9EC00            jmp 0x10a
0000001E  EB31              jmp 0x51
00000020  90                nop
00000021  E92001            jmp 0x144
00000024  E93101            jmp 0x158
00000027  E94201            jmp 0x16c
0000002A  E95301            jmp 0x180
0000002D  E96401            jmp 0x194
00000030  E97501            jmp 0x1a8
00000033  E98601            jmp 0x1bc
00000036  E99001            jmp 0x1c9
00000039  50                push ax
0000003A  51                push cx
0000003B  57                push di
0000003C  06                push es
0000003D  B87403            mov ax,0x374
00000040  8EC0              mov es,ax
00000042  BF0000            mov di,0x0
00000045  B96E00            mov cx,0x6e
00000048  B0CB              mov al,0xcb
0000004A  FC                cld
0000004B  F3AA              rep stosb
0000004D  07                pop es
0000004E  5F                pop di
0000004F  59                pop cx
00000050  58                pop ax
00000051  53                push bx
00000052  1E                push ds
00000053  BB7403            mov bx,0x374
00000056  8EDB              mov ds,bx
00000058  C6060B010E        mov byte [0x10b],0xe
0000005D  C606140144        mov byte [0x114],0x44
```

`0021` through `0036` are seven contiguous three-byte near-JMP entries.
`002A` is local entry 3 and overall entry index 14 when earlier short entries
are counted. It unambiguously dispatches to `0180`.

### Full `0180` to `01E4` disassembly

```text
00000180  FB                sti
00000181  06                push es
00000182  1E                push ds
00000183  57                push di
00000184  BA7403            mov dx,0x374
00000187  8EC2              mov es,dx
00000189  E86B00            call 0x1f7
0000018C  8EDA              mov ds,dx
0000018E  E89F25            call 0x2730
00000191  EB47              jmp 0x1da
00000193  90                nop
00000194  FB                sti
00000195  06                push es
00000196  1E                push ds
00000197  57                push di
00000198  BA7403            mov dx,0x374
0000019B  8EC2              mov es,dx
0000019D  E85700            call 0x1f7
000001A0  8EDA              mov ds,dx
000001A2  E87B20            call 0x2220
000001A5  EB33              jmp 0x1da
000001A7  90                nop
000001A8  FB                sti
000001A9  06                push es
000001AA  1E                push ds
000001AB  57                push di
000001AC  BA7403            mov dx,0x374
000001AF  8EC2              mov es,dx
000001B1  E84300            call 0x1f7
000001B4  8EDA              mov ds,dx
000001B6  E87719            call 0x1b30
000001B9  EB1F              jmp 0x1da
000001BB  90                nop
000001BC  FB                sti
000001BD  06                push es
000001BE  1E                push ds
000001BF  57                push di
000001C0  E82200            call 0x1e5
000001C3  E89321            call 0x2359
000001C6  EB12              jmp 0x1da
000001C8  90                nop
000001C9  FB                sti
000001CA  06                push es
000001CB  1E                push ds
000001CC  57                push di
000001CD  BA7403            mov dx,0x374
000001D0  8EC2              mov es,dx
000001D2  E82200            call 0x1f7
000001D5  8EDA              mov ds,dx
000001D7  E88618            call 0x1a60
000001DA  5F                pop di
000001DB  07                pop es
000001DC  E82E00            call 0x20d
000001DF  8CC2              mov dx,es
000001E1  8EDA              mov ds,dx
000001E3  07                pop es
000001E4  CB                retf
```

No genuine instruction-boundary `CALL FAR E000:0180` or `JMP FAR E000:0180`
reference was found in VA1. The observed path arrives through the entry table
after a near `RET`. Its common tail is far-returning, but there is no separate
genuine far-call frame as an independent proof source.

### All five real `CALL 391D` sites

| Caller | Bytes | Post IP | DX provenance | Terminal reachable | Implied target |
| --- | --- | --- | --- | --- | --- |
| `34BD` | `E8 5D 04` | `34C0` | incoming DX survives `3705`; observed `0005` | yes | `34C0:DX` |
| `43B2` | `E8 68 F5` | `43B5` | preserved across `3ED2` save/restore | yes | `43B5:DX` |
| `49F9` | `E8 21 EF` | `49FC` | preserved across `3ED2` save/restore | yes | `49FC:DX` |
| `75A8` | `E8 72 C3` | `75AB` | preserved across `3ED2` save/restore | yes | `75AB:DX` |
| `7F2A` | `E8 F0 B9` | `7F2D` | incoming DX survives `3753` | yes | `7F2D:DX` |

None loads a fixed module-entry constant immediately before `391D`; each
passes incoming DX. This weakens a simple fixed-module interpretation but does
not make parser-derived DX incompatible with an entry offset. Across every
terminal-reachable site, post-CALL IP supplies CS, incoming DX supplies IP,
and incoming SI supplies the intermediate near entry. Five distinct implied
segments are unusual; that is a prior, not a refutation. Under the task's
all-callsites rule C3 is PROVEN, without assigning target ownership.

### `383A` and deciding CF

Full helper neighborhood through the final CF producer:

```text
000037F3  BB0500            mov bx,0x5
000037F6  EB0E              jmp 0x3806
000037F8  E80AFF            call 0x3705
000037FB  BB0100            mov bx,0x1
000037FE  EB06              jmp 0x3806
00003800  E802FF            call 0x3705
00003803  BB0000            mov bx,0x0
00003806  9AAA0A4010        call word 0x1040:word 0xaaa
0000380B  51                push cx
0000380C  52                push dx
0000380D  53                push bx
0000380E  E87300            call 0x3884
00003811  8A04              mov al,[si]
00003813  8BF3              mov si,bx
00003815  5B                pop bx
00003816  721B              jc 0x3833
00003818  3C24              cmp al,0x24
0000381A  7501              jnz 0x381d
0000381C  41                inc cx
0000381D  0AED              or ch,ch
0000381F  7512              jnz 0x3833
00003821  B400              mov ah,0x0
00003823  CD97              int byte 0x97
00003825  3DFEFF            cmp ax,0xfffe
00003828  7409              jz 0x3833
0000382A  3DFFFF            cmp ax,0xffff
0000382D  7408              jz 0x3837
0000382F  5A                pop dx
00003830  59                pop cx
00003831  F8                clc
00003832  C3                ret
00003833  5A                pop dx
00003834  59                pop cx
00003835  F9                stc
00003836  C3                ret
00003837  E85EBF            call 0xf798
0000383A  9AAF0A4010        call word 0x1040:word 0xaaf
0000383F  50                push ax
00003840  53                push bx
00003841  56                push si
00003842  BB0000            mov bx,0x0
00003845  E8BEFF            call 0x3806
00003848  7313              jnc 0x385d
0000384A  BB0700            mov bx,0x7
0000384D  E8B6FF            call 0x3806
00003850  730B              jnc 0x385d
00003852  E89EFF            call 0x37f3
00003855  7306              jnc 0x385d
00003857  BB0400            mov bx,0x4
0000385A  E8A9FF            call 0x3806
0000385D  5E                pop si
0000385E  5B                pop bx
0000385F  58                pop ax
00003860  F5                cmc
00003861  C3                ret
```

The nested parser helper used by `3806` is:

```text
00003884  8BDE              mov bx,si
00003886  9AB40A4010        call word 0x1040:word 0xab4
0000388B  33C9              xor cx,cx
0000388D  33D2              xor dx,dx
0000388F  E816FF            call 0x37a8
00003892  7218              jc 0x38ac
00003894  0AE4              or ah,ah
00003896  B80100            mov ax,0x1
00003899  7401              jz 0x389c
0000389B  40                inc ax
0000389C  03F0              add si,ax
0000389E  83FA28            cmp dx,0x28
000038A1  7303              jnc 0x38a6
000038A3  03C8              add cx,ax
000038A5  42                inc dx
000038A6  E80EFF            call 0x37b7
000038A9  73E9              jnc 0x3894
000038AB  F8                clc
000038AC  C3                ret
```

On `A=1`, `3835 STC` changes FLAGS `0244 -> 0245`; no later instruction
changes CF until `3860 CMC`, which produces `0244`. `3979 PUSH DS` is
flag-neutral, so `397A` correctly sees CF=0. Slot `0AAF` is a `CB` stub and
its far call/RETF is flag-neutral. SST v20 records F5 selected=500,
executed=500, pass=500, fail=0. C4 is PROVEN and no CPU-defect stop triggers.

## Branch decision

```text
C1 PROVEN
C2 UNDETERMINED
C3 PROVEN
C4 PROVEN
```

D1 is inapplicable because C3 is not refuted. D2 is prohibited because C2 did
not demonstrate a populated VA2 target. C5a/C5b were not measured. No logical
write, DMA ownership, loader attribution, unproven regression, or production
correction was added.

The earliest unresolved boundary is why original VA1 guest code treats helper
success (CF=0) as permission to execute the terminal continuation while
service `1040:0AC3` is a `CB` stub. Evidence does not choose between optional
service installation, caller-side stub handling, and an unidentified runtime
contract. It rules out wrong RETF pop order and wrong vaeg CF calculation on
this path.

## Artifact identities

| Artifact | SHA-256 |
| --- | --- |
| Final diagnostic/test worker | `9a9bb6848b9822e9a15681773591c27ed6aa4698f0dafb361446f7cf429c8d50` |
| Production worker | `30256b1ffb3c69a7cbacb43e96415b2e7d9b03e5d53f18e9483b21aad23987d0` |
| Primary VA1 `A=1` trace | `fef674149deb7e67f7ee27132393a92ad656491eec2f9907c8077ea66320c3ad` |
| Primary VA1 `A=1` TVRAM | `126908bee355934c5e357d1b5f7d210ca9d1ecb3d2ff25ca0cef02c3c4b5c5bc` |
| Primary VA1 rendered screen | `2aad3a5611532eee264ea4344711a40baad39e34731f9d4add9d66e12cee85a6` |
| Primary VA1 `A!=1` trace | `b599041958223d46331df32a0e953efe56a22dd09f38726a8eb152c368db22e6` |
| Primary VA1 `A!=1` TVRAM | `b307e8ca764da5af022b7b6787daa274acaf425e36350c9f52fd0c15ae870550` |
| VA2 control trace | `610b148246432cf2216ecc4c1167fb1985b972e432df49336c4dc62f1800ec09` |
| VA2 control TVRAM | `790b56f87941bc7688b5f12ecc127bf51ba7d9b1f3bf6399b08fe19781aba8cb` |
| VA2 rendered screen | `307fa86e2f8136b2b62f6966e13f53aa9aef1acd1223bbfaea0d5eefe650163b` |
| Complete VA1 vector census | `ee0cf5e46a3a89e7fb73e6b8d9f82a4c26fc6ce3e360d89a27f3e43ae5eb1bf4` |
| Complete VA1 EA stream | `2f5063749da6c6fbc04091d79dcfbdde88766435dd0544dcc067afd4ee030ec3` |
| Complete VA2 EA stream | `9ca7b1dd392c7461d4baf0c0c724a85c911e501273237814105e41f27a629a66` |
| VA1 disassembly | `e122a48d15ec3477dc22e9673185e9844966b0d2372a5892288d599d1ce4ddd6` |
| VA2 disassembly | `8348ac2b14f89d46dbc76fe2a06a2ecfe0c6f4bfcb7ce4f95b4b1d1efc2e35bd` |

Every matrix trace and TVRAM dump is identified below as
`version case trace-sha256 tvram-sha256`:

```text
100 paint    a40c42cae363ff81b99c116fc7dc83cca167f4a716b75af5a36a8caf365a9421 f5e76741a0147a571ac3bc09f2918ce0538c971a5fa2464f188cc1cbfd049d9a
100 print1   1b56c2cfa7e40d31b3ab886fbce55a8ea1e689d5f7bb8a4d14f808d0901346fe a6a56c2078700a0a33d19d824022bf816cd2ec9c760f95ca7c1e0262435d8f19
100 q1       3a28d90a74c74b116151c83a0bb059f27f2580431aae688a8da1cc74328ee491 a9547d56fdf32449d02e149c7da0df5383514da246b16d1ad90521fe399f10ec
100 a1       d964800c6f127d9ec2d3ea804a3515ff86e06021124a4d81fe5ba1d82c9ee01c 55ec29212748fd80ad638d2980e271c790ae988116f72713a9c8917167339536
100 let-a1   59577d1c977ab4abb19de8064e306af5d515b637a059f37cc10d910e34ad8b95 c5af197abca263f738c978bffbe86f7d1a8a90fe8c8e6114735892ac491612bb
100 apercent 32566e4b0a4a8e042c59ab32cde8dc29b203daddb812e4c5c2222d88980b4535 486e765123bf330cb0fcdfd8366ac341262eda6b5f1449befbdbf7ba721531b7
100 abang    947c4fb20448a4629975bfd70bfd2c6cc0258936157550e554ff334fab994ab7 fdae424cacde540cd611a3738c381921080dc10e5dd635f770bc1e3bea5847d6
100 ahash    a4b91302e56b32d471c2dc81c5c389596fcc49bc92934fd58cbad3d3bda02109 676296203a12921e5c4f65a228bdb8b56893eb9c4abe8c58a4f2ae15df0eeea7
100 astring  c2ebb758e7d389560c51fdfbbc4cb8a09bef2e7cd0379e3b632b955275c088b0 0adadee181d43de0dc84203075931ca75c64583d1423564acde0249a7789cda5
100 defint   ddcfe1af121f159851d9bd8247e6e69dec20b7f5bab144e4c46b9b71e546febc 1902fb661cb20305e612ba6e6a9988fc68762c8066c58d09b1337df2191f493a
100 defsng   31303ec7730be0f8486f22a82ff7972386ecc958f839e10a7823b24de5ce055a 4996e9ff8bfa14f4d9fe50ad5ae23abff7eb0d1944753d958d769a933aa1d4ed
100 defdbl   a211a70f6e7d4127596985edc361e49355055ad44251eb81f3db6f5553d41624 102d403c276b16c104834474ea006239218e6a9aa22007d4912d3a28d2d0a44c
100 program  03b2293d8547a8355421028c945847923a58ae99534077332f68efcb326e5fd2 57421e5371e614fb86d5c5ee79fdd2fdacbc2250aea6c4dc0db36d783033fef9
105 paint    f3e15f88cbcd5f48825cabca266e998595bca6f88dc896daf4e028819d4ff045 00114b6b82e7ea9c57cbc5d66395501834e7bc4ee021db15a8b17695163e0a9d
105 print1   3eb75e652e798945c6d152d5d6d777222017f9496c2ad76375eee79af2e44b04 97e1294b1f19f4e683d0a6e1daed44de7c43b744e8f865a515a6d11fd1ac9438
105 q1       36023a06ec0ec5394a68a79824b56b9efabe39a6f39bbd63dfcb53dc69a4280f ddd8d81cd36480eddaf99885596d431785a51f1f64713434d90f70a438b349c2
105 a1       18d3961737287fe1d4e8778c711da1f32ddca8c1761f123c9667be403ca23774 126908bee355934c5e357d1b5f7d210ca9d1ecb3d2ff25ca0cef02c3c4b5c5bc
105 let-a1   0ee296bfbf41b57c73959afbc307a8d29caaa86ade6dd0ab2ab03a2b4478532d e3639f2bd3236f4df38e87f013da98ae57686b92302affd35e4508e456d1bb15
105 apercent fb74d4934c523e60ae68138e34e4f5d2b44493016d88000e21b9975ff36dd303 d4c0cca75402f59ef65007034b6e5106e840dc55d9c2f014fbab4f4d236de7b7
105 abang    0c3dbeb8050c840633565f6f69a20bdc199e13f60bdfc36742fb2aeba3bfac4f b307e8ca764da5af022b7b6787daa274acaf425e36350c9f52fd0c15ae870550
105 ahash    f639a9f5cabc2cb182e4e407af55a0f618d9c05bd2fd5959cfe263ce4bbb5c14 c46a6f2f5e8f44f747a8934affab345a859ff65274663c1afc7d02e53c66bb4f
105 astring  1d01c37192d8d276e27ba5490afd90fcd56b9fd8bf0987ae0c5c37c3cac3928a f95f440cbb985efefc8b1be9bf05eb070a0fd6dbfbde329ff2432ed920b88d52
105 defint   eac2266789bbfc4584921a863d4342980d189fa5d61634fb45efd93b52426920 4c56f6e6c382d721785bfc19e73f5b7a950e81c7756ccb8d64d027a611ed50f7
105 defsng   d1fd9b96d745d44431fe4b5486aa7d7b686ea50e4e24f8ecc150cae4e8e767ea 81e112dcf82b327df65f99f26d77376ae5caf57ec5295b2050878b4085be6c9b
105 defdbl   fccb7b7b9c0f2c8907a0c8e17732651509252c49f805ed3c4a1fe260b79943a1 a81603e5b0fe35f83b302a1d8f10eb4f9c6b511f486dd3ddffeb57e18541799f
105 program  a9952cfdcf8f4439ed71ebbe901bc32d84b6df7764cc02b9826e7ee67235ddc2 6b2e070e5886252789c8667d646e6c0950b89c129108722dae7a293b82a10833
110 paint    4f9bcc1d4079dc5745053a764e6498854a0713bf17990e205d43e42c994e3186 940aabf2e997274f7b019f7e42e62d290601b216c47d80dd333211998e7eef81
110 print1   7263a5a371f23f14591060b1c259136c1bfaa3f46adc9a4bddf2ff2b5c2be121 7e964c4a04d93e1fee51b4a9090f0e98192995949c63c179dafdb34629eca059
110 q1       3330628c2729e027bc6e7fd7db302f494ac22d31df76041b4e64bc522eac50a3 b567827bad5e415b27acde4a0071ce1f2c81510d33cb6815f1d3a9e5d863997d
110 a1       b18d75ab3b5d43bf033039632e118300a802f026c47edcd4cdeb632a407ba05b f4cb0a0753a0e12079b9efbc6844fa9109da0d8de26a142844ee7686f2f75c60
110 let-a1   c7e1add101c9e4c713aee83e668fbae2d51a118d5a525556a9aee53d9d70cb75 b14940339b8ac468ffc03c454e4574280c23b2a69d91e2982ca72f52ac00e329
110 apercent 7d064283c0b16173e57c77dbcc7452b2bb2aeeb8e736d3cc747dc000ce1c466b deee3a92ca95d3a29741a335051d3df849c17f2f90645f52dd926a64baf2298c
110 abang    482a3c21e7e65e1fb7cbcc90d351972b006e56e0ad248e81c936f3ef008fb6b0 d183a88391a38a96e6280f2ab2a3daea305008bdab278db6a55a41efd2c3124d
110 ahash    7fc5062798f405cf4adeb146b21ed915264dc71317f5395fffdc5218875ed1d4 3040f2626bae217bb2f21dd08e9eef0f3b75336d2d88016d71da31e7ad5eb33c
110 astring  a7a26cc6bc37f45e2ed2484dec12187e4a2946de8e63a0d911316ca347a0cf88 417749a096c274365c0ebc7c6bd69a77c119e0e7ff2dd84cbeabb1e09ef9d116
110 defint   ba0939d98ca081ccce45063aa56160de912eacdd2ed822d71eb9d5259ca44033 832c47900a1dac1d80287c8a9675fe08b6a6bd175b6030ff4c93f31dd74486cd
110 defsng   9a73558040f0ebc533f775f74cb3f0e67e3305b45f2c93dd7aff0b307e28b3f3 ca829bcb0411af3ad003a81eacbe1f4f6a8e2a14857e229fa74b1b67dc72a68b
110 defdbl   2fa854f7084212511928d21786ab2c71ab46bbbb89e7bda03ba645c925325bc8 6987c89cb9e224f4aeb55b38893ebcd9fe972d8fcdadfbdb6c0b69bd4ae97b06
110 program  012073649ca9c1211bc6c1c997a885fa438b3e47fa2dd9c8e233642f944fce44 fb94595a834747f3ad639fe1d52ef155ea5ab5be6116e18f6dae7377cef3106e
```

## Validation

```text
cmake -S . -B /private/tmp/m74c-prod-build -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH=/opt/local
cmake --build /private/tmp/m74c-prod-build --target vaeg_sdl2 -j4
PASS, exit 0

cmake -S . -B /private/tmp/m74c-test-build -DCMAKE_BUILD_TYPE=Release \
  -DVAEG_ENABLE_TESTS=ON -DVAEG_Z80_COMPAT_INTEGRATION_TRACE=ON \
  -DCMAKE_PREFIX_PATH=/opt/local
cmake --build /private/tmp/m74c-test-build -j4
PASS, exit 0

SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
  /private/tmp/m74c-test-build/sdl2/vaeg --selftest
PASS, exit 0; all selftests passed

vaeg --upd9002-m68-segmented-memory
PASS: mapped dispatch checks passed
vaeg --idp-m69-status-composition
PASS: status composition checks passed
vaeg --upd9002-m70-prefix-string
PASS: directed checks passed
git diff --check
PASS, exit 0, no output
```

The complete ROM-less set was run in bounded shards after the one-process run
encountered the managed macOS sandbox's inaccessible system Perl at test 43.
The final shard environment preferred `/opt/local/bin` and disabled global and
system Git configuration. The union covers all 71 tests: 68 passed, 2 failed,
and 1 external-corpus test was skipped.

The only failures are `vaeg_upd9002_protected_deletion` and
`vaeg_upd9002_protected_deletion_selftest`. Both report
`cpu/upd9002/upd9002_ops.mcr` expected
`dbfcc5b3ce7d3f0b4df493cd494b7fe297aa932e231904ddeb4b59411cd73183`,
actual `73c75f7a82706487b51a66e30718d6daef21caa9f73458cd3d538a059fe4d089`.
That file predates M74c and neither M74c commit modifies it. These are the
task-authority-listed pre-existing protected-deletion failures, not new
diagnostic regressions.

## Scope audit and remaining boundary

- No production CPU, RETF, far CALL, LOCK, FPU, BCD, mapper, FDC, memory, or
  timing behavior changed.
- No regression was added for an unproven defect.
- No ROM, disk, private identity, raw trace, screen, or binary is committed.
- C4 was not refuted; no named instruction-level vaeg defect was found.
- C5a/C5b remain unmeasured because D2 was not authorized.
- A working equivalent VA2 BASIC entry or another independent contract is
  required before continuation ownership can be investigated.

G74 remains not approved and requires human review.
