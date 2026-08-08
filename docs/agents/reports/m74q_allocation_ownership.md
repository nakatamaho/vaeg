# M74q allocation ownership and continuation-service frame handling

## Identity

- Branch: `topic/m74-va1-basic-command-hang`
- Starting SHA: `f6e03ae772e9f3ad1075c7ab5b4508b99bc83ac6`
- Diagnostic/evidence source SHA: `2199e4db227d97f14bf35f2dbc606cc4d1c3c16f`
- Approved G73 predecessor: `766a132ff6d66e335fe9bb1d0082d777a4a8fe14`
- Starting worker SHA-256: `0e3dcfbe9f8dcd3281bc6a7c12b814ad515e77785808e40583f041987502e678`
- Evidence worker SHA-256: `d6d151640a2263d5b0e084a6ac7818fbc995abfc0aa94dcf3c773c1546c5f2be`
- Runner SHA-256: `1e1aa04b21219def77b10b8557e1e6dcebe785880dcd2be98725e2ae984ab565`
- ROM SHA-256: `656d81bc532ed0bb602d1eb7f03df27c74f841fb19c72ad07d47e79302ac814b`
- Boot-only image SHA-256: 1.00 `bf551fc8d87f91072fefea94983a8477d7f84418bd73b24d5cf1dc6d94c09d4c`; 1.05 `35c17df8b65f747b1d789200bf950f07c092ac791e29169bfd49a089893b7e4d`; 1.10 `258d7d218289ab0437e8772aa50c86763fc904e024e36243823323cd86602275`.
- Final evidence-log SHA-256: 1.00 `0f2e7a0899617756eac51407fd00fc42eddc7d2401a2adc78283d66849c4150c`; 1.05 `8a0a62d4952cdf709502b6ddacfe5f166454eb5d3bf599b14c3b5c7075f7fbb1`; 1.10 `50f0552a208fa428f3368edfb2e3ea4f1b1143a9172bd0fd1ac49d6dd1d1617f`.
- Runtime contract: explicit `--model va`, trace-enabled build, deterministic frame bound 1100, one `PRINT 1` command per clean boot-only run.

The tracked runner is [`tools/m74-diagnostics/run_basic_case.sh`](../../../tools/m74-diagnostics/run_basic_case.sh). It emits source, worker, runner, ROM and boot-image identities; model; command; guest bound; diagnostic switches; working directory; and emulator exit status. Private absolute asset paths and payloads are not tracked.

## M74p established facts

**Proven dynamic observation.** The measured `34BD -> 391D` non-escape paths normalize the stack to `002A,0005,34C0`; `3988 RET` (`PRINT 1`) or `3983 RET` (`A=1`) consumes `002A`; `002A -> 0180 -> 01E4 RETF` then consumes `0005,34C0` and transfers to `34C0:0005`.

**Proven dynamic observation.** All five previously sampled 256-byte continuation segment windows were zero at the terminal checkpoint. The resource selected for `34C05h` was ordinary main RAM and matched the documented resource-selection rule under the captured mapper state.

## Pre-populated-target hypothesis downgrade

**Working hypothesis, downgraded.** “The wrapper simply enters pre-populated continuation code in the five sampled windows” has no positive support: every sampled window was zero. It is not formally refuted because four exact DX offsets are unproven, initialization could occur outside the sampled window or later, and a shared absent upstream state remains possible.

## Meaning and limits of the BASIC free lower bound

**Proven static fact.** The ROM computes the displayed count at `E000:2D72-2D92` from three work-area words:

```text
2D72 A1 1A 00          MOV AX,[001A]
2D75 2B 06 10 00       SUB AX,[0010]
2D79 8B D0             MOV DX,AX
2D7B C1 C2 04          ROL DX,4
2D7E 81 E2 0F 00       AND DX,000F
2D82 C1 E0 04          SHL AX,4
2D85 8B C8             MOV CX,AX
2D87 A1 04 00          MOV AX,[0004]
2D8A F7 D8             NEG AX
2D8C 48                DEC AX
2D8D 03 C1             ADD AX,CX
2D8F 83 D2 00          ADC DX,0000
2D92 E8 CB 2C          CALL 5A60
```

The direct boundary equations entailed by those instructions and the captured words are:

```text
lower = ([0010] << 4) + [0004] + 1
upper = ([001A] << 4) + 10000h
free  = upper - lower
```

Thus `348856` is not inferred from the printed text: for BASIC 1.00 it is directly `8FE00h - 3AB48h = 552B8h = 348856`. “Below lower” means outside the reported free pool; it does not entail initialized or populated storage.

## Continuation-service symbolic stack ledger

Let `S` be SP on entry to `E000:0180`. The observed 1.05 run has `SS:SP=7FE0:01F4`, `SS:[S]=0005`, and `SS:[S+2]=34C0`.

| Point | SP | inherited IP alias | inherited CS alias | Observation |
| --- | --- | --- | --- | --- |
| `0180` entry | `S=01F4` | `[S+0]=0005` | `[S+2]=34C0` | measured |
| `01F7` entry | `S-8=01EC` | `[SP+8]=0005` | `[SP+10]=34C0` | measured |
| `2730` entry | `S-8=01EC` | `[SP+8]=0005` | `[SP+10]=34C0` | measured |
| `020D` entry | `S-4=01F0` | `[SP+4]=0005` | `[SP+6]=34C0` | measured |
| `01E4` | `S=01F4` | `[SP]=0005` | `[SP+2]=34C0` | measured |

**Proven static fact.** `0180` pushes `ES,DS,DI`, copies an eight-byte object from original `DS:DI` to scratch `0374:008F`, invokes `2730` with `DS=0374`, restores the object to original `DS:DI`, restores segments, and executes `RETF`:

```text
0180 FB             STI
0181 06             PUSH ES
0182 1E             PUSH DS
0183 57             PUSH DI
0184 BA 74 03       MOV DX,0374
0187 8E C2          MOV ES,DX
0189 E8 6B 00       CALL 01F7
018C 8E DA          MOV DS,DX
018E E8 9F 25       CALL 2730
0191 EB 47          JMP 01DA
01DA 5F             POP DI
01DB 07             POP ES
01DC E8 2E 00       CALL 020D
01DF 8C C2          MOV DX,ES
01E1 8E DA          MOV DS,DX
01E3 07             POP ES
01E4 CB             RETF
```

## E000:01F7 annotated disassembly

| Address | Bytes | Instruction | Stack/flags | Memory effect |
| --- | --- | --- | --- | --- |
| `01F7` | `8B F7` | `MOV SI,DI` | none | none |
| `01F9` | `BF 8F 00` | `MOV DI,008F` | none | none |
| `01FC` | `B9 04 00` | `MOV CX,0004` | none | none |
| `01FF` | `FC` | `CLD` | clears DF | none |
| `0200` | `F3 A5` | `REP MOVSW` | consumes CX | copies 8 bytes `DS:original-DI -> ES:008F` |
| `0202` | `C3` | `RET` | pops only its near return | none |

**Proven dynamic observation.** At entry: `DS:SI=2E8A:002F` before `MOV SI,DI`, `ES:DI=0374:0236`, and the inherited far words were still `0005,34C0`. The effective destination is `0374:008F`, not SS. Verdict: `01F7` can rewrite the inherited far frame: **NO on the measured path**; it has no transitive call.

## E000:2730 annotated disassembly

| Address | Bytes | Instruction | Architectural effect |
| --- | --- | --- | --- |
| `2730` | `C6 06 8E 00 00` | `MOV BYTE [008E],00` | clears scratch byte in DS=`0374` |
| `2735` | `F6 06 96 00 FF` | `TEST BYTE [0096],FF` | sets flags from scratch control byte |
| `273A` | `74 14` | `JZ 2750` | zero control returns directly |
| `273C` | `9C` | `PUSHF` | conditional path only |
| `273D` | `80 26 96 00 7F` | `AND BYTE [0096],7F` | conditional scratch update |
| `2742` | `E8 0C 00` | `CALL 2751` | conditional transitive path |
| `2745` | `9D` | `POPF` | restores saved flags |
| `2746` | `79 05` | `JNS 274D` | condition from restored flags |
| `2748` | `80 0E 96 00 80` | `OR BYTE [0096],80` | conditional scratch update |
| `274D` | `E8 E8 DB` | `CALL 0338` | conditional transitive path |
| `2750` | `C3` | `RET` | balanced direct return |

**Proven dynamic observation.** The 1.00, 1.05 and 1.10 failing runs all captured `[0374:0096]=00`; `2751=0`, `0338=0`, and `03C2=0`. Therefore `273A` went directly to `2750`, and the inherited words remained `0005,34C0`.

**Specific static boundary.** If `[0096] != 0`, `2730` enters a larger conditional call tree. Its `0338` branch calls `147F`, saves SP in `DS:[0117]`, calls `03C2`, and returns through `123C -> 1257`, where `MOV SP,[0117]` restores the saved stack before `14B9`. That dormant branch is not needed to classify the measured path and was not executed. Verdict: `2730` can rewrite the inherited frame: **NO on the measured path; UNRESOLVED for the dormant nonzero-control transitive branch**. That dormant branch is not used as evidence about this failure.

## E000:020D annotated disassembly

| Address | Bytes | Instruction | Stack/flags | Memory effect |
| --- | --- | --- | --- | --- |
| `020D` | `8B DF` | `MOV BX,DI` | none | none |
| `020F` | `BE 8F 00` | `MOV SI,008F` | none | none |
| `0212` | `B9 04 00` | `MOV CX,0004` | none | none |
| `0215` | `FC` | `CLD` | clears DF | none |
| `0216` | `F3 A5` | `REP MOVSW` | consumes CX | copies 8 bytes `0374:008F -> original DS:DI` |
| `0218` | `8B FB` | `MOV DI,BX` | none | none |
| `021A` | `C3` | `RET` | pops only its near return | none |

**Proven dynamic observation.** At entry in 1.05, `DS:SI=0374:023E` before the local SI assignment, `ES:DI=2E8A:0236`, and the inherited words at `[SP+4],[SP+6]` were `0005,34C0`. The destination is original BASIC `DS:0236`, not SS. Verdict: `020D` can rewrite the inherited far frame: **NO**; it has no transitive call.

## Frame-rewrite hypothesis verdict

**Rejected hypothesis for the measured continuation path.** `FRAME-REWRITE-REJECTED`: the complete executed path is `01F7 -> 2730(JZ) -> 020D`; neither copy aliases SS, `2730` takes no transitive call, and the fixed stack aliases remain `0005,34C0` at every measured boundary. The wrapper words are therefore the actual far-return frame on this path, not a provisional frame left unrevised by a missed active branch.

The result does not claim that the dormant `[0374:0096] != 0` branch can never manipulate stack state under another service input. Its exact unexecuted transitive semantics are the named scope limit.

## 1.05 below-free occupancy map

The samples are side-effect-free 256-byte reads at the command-local pre-`01E4 RETF` event.

| Address | SHA-256 | Nonzero | First | Last | Classification |
| --- | --- | ---: | --- | --- | --- |
| `2E800` | `b10ab9831ebff43b36216b6fefff6ac01a278d63140e3d42fbc79573cb1c1e24` | 85 | `A0` | `FF` | non-zero/data-supported; overlaps the 1.05 DS page at `2E8A0` |
| `30000` | `eba3425093876db9283abc47322a5c4238a11b57e86cf92935a0e3f8ac67b844` | 222 | `00` | `FF` | non-zero/code-supported; coherent entry-like instruction sequence |
| `31000` | `5341e6b2646979a70e57653007a1f310169421ec9bdd9f1a5648f75ade005af1` | 0 | - | - | zero |
| `32000` | `5341e6b2646979a70e57653007a1f310169421ec9bdd9f1a5648f75ade005af1` | 0 | - | - | zero |
| `33000` | `5341e6b2646979a70e57653007a1f310169421ec9bdd9f1a5648f75ade005af1` | 0 | - | - | zero |
| `34000` | `5341e6b2646979a70e57653007a1f310169421ec9bdd9f1a5648f75ade005af1` | 0 | - | - | zero |
| `34C00` | `5341e6b2646979a70e57653007a1f310169421ec9bdd9f1a5648f75ade005af1` | 0 | - | - | zero |
| `35000` | `5341e6b2646979a70e57653007a1f310169421ec9bdd9f1a5648f75ade005af1` | 0 | - | - | zero |
| `36000` | `5341e6b2646979a70e57653007a1f310169421ec9bdd9f1a5648f75ade005af1` | 0 | - | - | zero |
| `38000` | `5341e6b2646979a70e57653007a1f310169421ec9bdd9f1a5648f75ade005af1` | 0 | - | - | zero |
| `3A000` | `5341e6b2646979a70e57653007a1f310169421ec9bdd9f1a5648f75ade005af1` | 0 | - | - | zero |
| `3C000` | `5341e6b2646979a70e57653007a1f310169421ec9bdd9f1a5648f75ade005af1` | 0 | - | - | zero |
| `3E000` | `5341e6b2646979a70e57653007a1f310169421ec9bdd9f1a5648f75ade005af1` | 0 | - | - | zero |
| `40000` | `5341e6b2646979a70e57653007a1f310169421ec9bdd9f1a5648f75ade005af1` | 0 | - | - | zero |
| `41000` | `5341e6b2646979a70e57653007a1f310169421ec9bdd9f1a5648f75ade005af1` | 0 | - | - | zero |
| `415A8` | `5341e6b2646979a70e57653007a1f310169421ec9bdd9f1a5648f75ade005af1` | 0 | - | - | zero/free-boundary control |

The required first 32 bytes of both nonzero samples are:

```text
2E800: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
       00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
30000: B8 00 30 8E D8 FA 8E D0 BC FE 1F FB E8 E3 00 E8
       F7 00 E8 FE 00 52 1D E8 E6 00 FA 81 ED 00 10 8E
```

The `30000` bytes tentatively decode as a coherent setup sequence beginning `MOV AX,3000; MOV DS,AX; CLI; MOV SS,AX`; this is structural support, not a semantic routine name.

**Proven dynamic observation: BAND-MOSTLY-ZERO.** The zero run is broad (`31000` through and beyond `34C00` in the sampled grid). `34C00` is not an isolated sampled hole: both `34000` and `35000` are zero, so adaptive local refinement was not triggered. This directly rejects the inference that below-free necessarily means populated storage in this configuration.

## Refined 34C00 zero-hole map, if triggered

Not triggered. The exact criterion for refinement—nearby nonzero blocks bracketing a zero `34C00` block—was false.

## DS:0000-00FF raw work-area dump

BASIC 1.05: `DS=2E8A`, physical base `2E8A0`, SHA-256 `c1737497d162a4287cbd9f9f737d42d33a439056bc16c85d6b2ddb04c0674fc6`.

```text
00: 8A 2E 04 12 07 12 07 12 07 12 FF FF FF FF FA 3F
10: 3A 40 00 00 3A 40 00 00 E0 7F E0 7F 00 80 00 80
20: 8A 3E FA 3E 00 01 80 00 00 00 50 52 49 4E 54 20
30: 31 00 20 20 20 20 20 20 20 20 20 20 20 20 20 20
40: 20 20 20 20 20 20 20 20 20 20 20 20 20 20 20 20
50: 20 20 20 20 20 20 20 20 20 20 20 20 20 20 20 20
60: 20 20 20 20 20 20 20 20 20 20 20 20 20 20 20 20
70: 20 20 20 20 20 20 20 20 20 20 00 00 00 00 00 00
80: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
90: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
A0: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
B0: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
C0: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
D0: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
E0: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
F0: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
```

## Work-area word table and flagged anchors

The 1.05 page as 128 little-endian words:

```text
00:2E8A  02:1204  04:1207  06:1207  08:1207  0A:FFFF  0C:FFFF  0E:3FFA
10:403A  12:0000  14:403A  16:0000  18:7FE0  1A:7FE0  1C:8000  1E:8000
20:3E8A  22:3EFA  24:0100  26:0080  28:0000  2A:5250  2C:4E49  2E:2054
30:0031  32:2020  34:2020  36:2020  38:2020  3A:2020  3C:2020  3E:2020
40:2020  42:2020  44:2020  46:2020  48:2020  4A:2020  4C:2020  4E:2020
50:2020  52:2020  54:2020  56:2020  58:2020  5A:2020  5C:2020  5E:2020
60:2020  62:2020  64:2020  66:2020  68:2020  6A:2020  6C:2020  6E:2020
70:2020  72:2020  74:2020  76:2020  78:2020  7A:0000  7C:0000  7E:0000
80:0000  82:0000  84:0000  86:0000  88:0000  8A:0000  8C:0000  8E:0000
90:0000  92:0000  94:0000  96:0000  98:0000  9A:0000  9C:0000  9E:0000
A0:0000  A2:0000  A4:0000  A6:0000  A8:0000  AA:0000  AC:0000  AE:0000
B0:0000  B2:0000  B4:0000  B6:0000  B8:0000  BA:0000  BC:0000  BE:0000
C0:0000  C2:0000  C4:0000  C6:0000  C8:0000  CA:0000  CC:0000  CE:0000
D0:0000  D2:0000  D4:0000  D6:0000  D8:0000  DA:0000  DC:0000  DE:0000
E0:0000  E2:0000  E4:0000  E6:0000  E8:0000  EA:0000  EC:0000  EE:0000
F0:0000  F2:0000  F4:0000  F6:0000  F8:0000  FA:0000  FC:0000  FE:0000
```

No aligned little-endian word equals `34C0`, `0005`, or `002A`. The direct ROM references establish `[0004]`, `[0010]`, and `[001A]` as inputs to the free calculation above; no other word is assigned pointer semantics from numeric coincidence.

## Cross-version work-area comparison

| Version | DS | Physical base | Page SHA-256 | `34C0`/`0005`/`002A` aligned occurrences |
| --- | --- | --- | --- | --- |
| 1.00 | `27F4` | `27F40` | `63f4298f22c9d07b554258806726c169b70a68e65643defbdce2ec4128d72234` | none / none / none |
| 1.05 | `2E8A` | `2E8A0` | `c1737497d162a4287cbd9f9f737d42d33a439056bc16c85d6b2ddb04c0674fc6` | none / none / none |
| 1.10 | `2AE7` | `2AE70` | `dab135eb58ffd4779dd6856b892def5a982a8d595b307212a6633da46579aa0d` | none / none / none |

The direct boundary input words are:

```text
1.00  0004:1207  0010:3994  001A:7FE0
1.05  0004:1207  0010:403A  001A:7FE0
1.10  0004:1207  0010:3C97  001A:7FE0
```

Complete raw pages and word tables for 1.00 and 1.10 follow.

```text
BASIC 1.00 raw
00: F4 27 04 12 07 12 07 12 07 12 FF FF FF FF 54 39
10: 94 39 00 00 94 39 00 00 E0 7F E0 7F 00 80 00 80
20: F4 37 54 38 00 01 80 00 00 00 50 52 49 4E 54 20
30: 31 00 20 20 20 20 20 20 20 20 20 20 20 20 20 20
40: 20 20 20 20 20 20 20 20 20 20 20 20 20 20 20 20
50: 20 20 20 20 20 20 20 20 20 20 20 20 20 20 20 20
60: 20 20 20 20 20 20 20 20 20 20 20 20 20 20 20 20
70: 20 20 20 20 20 20 20 20 20 20 00 00 00 00 00 00
80: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
90: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
A0: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
B0: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
C0: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
D0: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
E0: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
F0: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00

BASIC 1.00 words
00:27F4  02:1204  04:1207  06:1207  08:1207  0A:FFFF  0C:FFFF  0E:3954
10:3994  12:0000  14:3994  16:0000  18:7FE0  1A:7FE0  1C:8000  1E:8000
20:37F4  22:3854  24:0100  26:0080  28:0000  2A:5250  2C:4E49  2E:2054
30:0031  32:2020  34:2020  36:2020  38:2020  3A:2020  3C:2020  3E:2020
40:2020  42:2020  44:2020  46:2020  48:2020  4A:2020  4C:2020  4E:2020
50:2020  52:2020  54:2020  56:2020  58:2020  5A:2020  5C:2020  5E:2020
60:2020  62:2020  64:2020  66:2020  68:2020  6A:2020  6C:2020  6E:2020
70:2020  72:2020  74:2020  76:2020  78:2020  7A:0000  7C:0000  7E:0000
80:0000  82:0000  84:0000  86:0000  88:0000  8A:0000  8C:0000  8E:0000
90:0000  92:0000  94:0000  96:0000  98:0000  9A:0000  9C:0000  9E:0000
A0:0000  A2:0000  A4:0000  A6:0000  A8:0000  AA:0000  AC:0000  AE:0000
B0:0000  B2:0000  B4:0000  B6:0000  B8:0000  BA:0000  BC:0000  BE:0000
C0:0000  C2:0000  C4:0000  C6:0000  C8:0000  CA:0000  CC:0000  CE:0000
D0:0000  D2:0000  D4:0000  D6:0000  D8:0000  DA:0000  DC:0000  DE:0000
E0:0000  E2:0000  E4:0000  E6:0000  E8:0000  EA:0000  EC:0000  EE:0000
F0:0000  F2:0000  F4:0000  F6:0000  F8:0000  FA:0000  FC:0000  FE:0000

BASIC 1.10 raw
00: E7 2A 04 12 07 12 07 12 07 12 FF FF FF FF 57 3C
10: 97 3C 00 00 97 3C 00 00 E0 7F E0 7F 00 80 00 80
20: E7 3A 57 3B 00 01 80 00 00 00 50 52 49 4E 54 20
30: 31 00 20 20 20 20 20 20 20 20 20 20 20 20 20 20
40: 20 20 20 20 20 20 20 20 20 20 20 20 20 20 20 20
50: 20 20 20 20 20 20 20 20 20 20 20 20 20 20 20 20
60: 20 20 20 20 20 20 20 20 20 20 20 20 20 20 20 20
70: 20 20 20 20 20 20 20 20 20 20 00 00 00 00 00 00
80: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
90: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
A0: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
B0: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
C0: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
D0: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
E0: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
F0: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00

BASIC 1.10 words
00:2AE7  02:1204  04:1207  06:1207  08:1207  0A:FFFF  0C:FFFF  0E:3C57
10:3C97  12:0000  14:3C97  16:0000  18:7FE0  1A:7FE0  1C:8000  1E:8000
20:3AE7  22:3B57  24:0100  26:0080  28:0000  2A:5250  2C:4E49  2E:2054
30:0031  32:2020  34:2020  36:2020  38:2020  3A:2020  3C:2020  3E:2020
40:2020  42:2020  44:2020  46:2020  48:2020  4A:2020  4C:2020  4E:2020
50:2020  52:2020  54:2020  56:2020  58:2020  5A:2020  5C:2020  5E:2020
60:2020  62:2020  64:2020  66:2020  68:2020  6A:2020  6C:2020  6E:2020
70:2020  72:2020  74:2020  76:2020  78:2020  7A:0000  7C:0000  7E:0000
80:0000  82:0000  84:0000  86:0000  88:0000  8A:0000  8C:0000  8E:0000
90:0000  92:0000  94:0000  96:0000  98:0000  9A:0000  9C:0000  9E:0000
A0:0000  A2:0000  A4:0000  A6:0000  A8:0000  AA:0000  AC:0000  AE:0000
B0:0000  B2:0000  B4:0000  B6:0000  B8:0000  BA:0000  BC:0000  BE:0000
C0:0000  C2:0000  C4:0000  C6:0000  C8:0000  CA:0000  CC:0000  CE:0000
D0:0000  D2:0000  D4:0000  D6:0000  D8:0000  DA:0000  DC:0000  DE:0000
E0:0000  E2:0000  E4:0000  E6:0000  E8:0000  EA:0000  EC:0000  EE:0000
F0:0000  F2:0000  F4:0000  F6:0000  F8:0000  FA:0000  FC:0000  FE:0000
```

**Inference rejected.** Because the DS bases vary while the continuation target is fixed, `34C0` is not supported as a simple fixed DS-relative word in these pages. No absolute anchor for `34C00` appears in the captured low work-area page.

## BASIC 1.00 free boundary

**Proven dynamic observation.** `[0004]=1207`, `[0010]=3994`, `[001A]=7FE0`; lower `3AB48`, upper `8FE00`, free `552B8h = 348856` bytes.

## BASIC 1.05 free boundary

**Proven dynamic observation.** `[0004]=1207`, `[0010]=403A`, `[001A]=7FE0`; lower `415A8`, upper `8FE00`, free `4E858h = 321624` bytes.

## BASIC 1.10 free boundary

**Proven dynamic observation.** `[0004]=1207`, `[0010]=3C97`, `[001A]=7FE0`; lower `3DB78`, upper `8FE00`, free `52288h = 336520` bytes.

## 34C05 versus free-region disposition

| Version | Lower | Upper | Free bytes | `lower - 34C05` | Position |
| --- | --- | --- | ---: | --- | --- |
| 1.00 | `3AB48` | `8FE00` | 348856 | `5F43` (24387) | below free pool |
| 1.05 | `415A8` | `8FE00` | 321624 | `C9A3` (51619) | below free pool |
| 1.10 | `3DB78` | `8FE00` | 336520 | `8F73` (36723) | below free pool |

**Proven dynamic observation: ALL-BELOW.** The continuation target is outside the BASIC-reported free pool for all three measured versions. **Rejected inference:** this does not imply populated storage; the 1.05 occupancy map directly shows broad zero regions below lower.

## E4 installer-source capture

**Proven static fact.** The executed installer at `19E3:C7D4-C7F1` sets `DS=CS`, `ES=1040`, `SI=C7F2`, and `CX=002A`, then loops exactly 42 fixed four-byte source records:

```text
C7E5 LODSW       ; destination offset
C7E6 MOV DI,AX
C7E8 MOV AL,EA
C7EA STOSB
C7EB MOVSW       ; target offset
C7EC MOV AX,CS
C7EE STOSW       ; segment
C7EF LOOP C7E5
```

The bounded one-shot captured all 42 `C7EB` events (`total_hits=42`, `table_hits=42`). Reconstructed source extent: `19E3:C7F2`, 168 bytes, SHA-256 `7c47706167e275850b06d0acc1b612953efcce2474f2590a7d2de17ea8e2c4f6`; first 32 bytes `270bf19b540b00945e0b1397360b749c850c3b84630b129d140a6085190a2186`; last 16 bytes `a30c569b5f0adea2cc0bf09780027f9e`.

## E4 installed 1040 state

The 166-slot reset table SHA-256 is `80e0f31f19a8ed96f01f9fc611a72829badeeee21b6876019e24c22e68b207ba`; at first prompt and pre-injection it is `d49094786dd6c35e0191bed12ef86d5c97cff42a4bd2c43cc05070730187753c`.

Queried prompt slots `1040:0AAA`, `0AAF`, `0AB4`, `0AC3`, and `0AC8` are each `CB 00 00 00 00` (STUB). The only captured source record in the immediate `0AAA-0AC8` neighborhood is `0ABE -> 19E3:A1A9`.

## E4 differential reconstruction

The complete executed source mapping is:

```text
0B27->9BF1  0B54->9400  0B5E->9713  0B36->9C74  0C85->843B  0B63->9D12
0A14->8560  0A19->8621  0A0F->85FE  0B6D->8649  0CB2->87E9  0A46->871A
0B9F->8C04  0CF8->8C1F  0AE1->9B0A  0BE5->8D50  0BD6->8DDD  0BA4->976F
0BA9->97B2  0AD2->91F1  0B8B->918E  0CA8->87B1  0AA5->A2F6  0A28->A33B
0A50->9207  0B86->92AC  0A4B->92B9  0C62->9886  0C67->9935  0B1D->9CCB
0A91->9CFD  0C94->9E4B  0B18->A031  0CF3->A0DB  0BF9->A342  0C3A->A7C7
0CE4->A18C  0ABE->A1A9  0CA3->9B56  0A5F->A2DE  0BCC->97F0  0280->9E7F
```

**Proven static and dynamic observation for the boot-only 19E3 installer:** no destination record for `0AC3` exists in the complete 42-record source extent, so that installer cannot populate the relevant slot; the prompt slot remains STUB.

**E4-PARTIAL across all named installer families.** The boot-only `19E3:C7EB` source/destination relation is complete and establishes absence there. The exact remaining ambiguity is the alternative `1CC5:C6BB` installer list: it was not executed or captured by the authorized boot-only corpus, and its complete source extent has not been reconstructed. No generic “record format unknown” blocker remains for the executed 19E3 list.

## Validation

| Command | Environment | Result |
| --- | --- | --- |
| `python3 tools/repo/check_encoding.py` | `GIT_CONFIG_GLOBAL=/dev/null GIT_CONFIG_SYSTEM=/dev/null` | PASS, exit 0 after report |
| `python3 tools/repo/check_eol.py` | same | PASS, exit 0 after report |
| `python3 tools/repo/check_case.py` | same | PASS, exit 0, 0 findings after report |
| `git diff --check` | same | PASS, exit 0 after report |
| `python3 tools/qa/milestone_ids.py --root . --selftest --discover --audit` | same | PASS, exit 0, 48 selftests |
| `cmake --build build/linux-release -j2` | native Release, trace-enabled diagnostic source | PASS, exit 0 |
| `build/linux-release/sdl2/vaeg --selftest` | evidence worker | PASS, exit 0 |
| targeted ROM-less/M68/M69/M70 CTest regex | same build | PASS, 6/6 |
| full non-external CTest partitioned as `1-30`, `31-45`, `46-60`, `61-80`, and remaining named tests | Git isolation, `-j4` | 92 PASS, 2 FAIL, 5 SKIP |
| `cmake --preset mingw-cross` | cross compiler available | PASS, exit 0 |
| `CCACHE_DISABLE=1 cmake --build --preset mingw-cross -j2` | process-local ccache disable | PASS, 349/349 across contained resumptions |

The full-suite partitions were exactly `ctest --test-dir build/linux-release -LE external -I 1,30 -j4 --output-on-failure`, ranges `31,45`, `46,60`, and `61,80` with the same options, followed by `-R '^(vaeg_m75_transfer_info_compiled|test_)' -j4`. A monolithic attempt was externally preempted near 30 seconds and is not counted; partitioning preserved the same test definitions and semantic criteria.

The two full-suite failures are pre-existing protected-deletion checks: expected `cpu/upd9002/upd9002_ops.mcr` SHA-256 `dbfcc5b3ce7d3f0b4df493cd494b7fe297aa932e231904ddeb4b59411cd73183`, actual `73c75f7a82706487b51a66e30718d6daef21caa9f73458cd3d538a059fe4d089`. The five dependency skips are `test_suffix.sh`, three generated-compress cases, and `test_files.sh`; they are SKIP, not PASS.

Without process-local isolation, Git reports `fatal: unable to access '/Users/maho/.gitconfig': Operation not permitted`. No persistent Git configuration was modified.

## Runtime identity and containment

All final guest rows use source `2199e4d`, worker `d6d151...`, runner `1e1aa...`, the version-specific boot-only image above, command `PRINT 1`, and frame bound 1100. First `Ok` was observed at frame 720 and command injection occurred at frame 840. The required command-local `01E4` event was reached in all three runs. No external containment fired. Exit 1 denotes the requested deterministic frame bound, not a guest failure verdict; only pre-bound fixed-event evidence is interpreted.

Incremental equivalence passed. Disabled diagnostics reached the first prompt at frame 720 and produced TVRAM SHA-256 `c6ff29f6852e02e822ce3ea628817a0fbe15bbb832701d734b3d269ac43044b5`, exactly equal to the accepted reference. Enabled 1.05 reproduced `34BD=1 391D=1 3983=0 3985=1 3988=1 E000:34C0=0 002A=1 0180=1 01E4=1`, with inherited far frame `0005,34C0` throughout the service.

## Hypothesis table

| Hypothesis | Status | Entailing evidence |
| --- | --- | --- |
| Pre-populated continuation targets | DOWNGRADED | five sampled windows zero; four exact offsets remain unproven |
| Below-free implies populated | REJECTED AS INFERENCE | broad 1.05 below-free zero band |
| `0180` rewrites the measured inherited frame | REJECTED FOR MEASURED PATH | complete executed call path and fixed stack aliases |
| `34C00` is an isolated zero hole | REJECTED IN COARSE SAMPLE | `34000`, `34C00`, `35000` and broad neighbors zero |
| VAEG resource-selection defect | DOWNGRADED/REJECTED at `34C05` | M74p hardware/resource match; no contradiction here |
| Missing installation | PARTIALLY SUPPORTED only for executed 19E3 slot absence | complete 42-record list lacks `0AC3`; alternative list unresolved |

## First incorrect emulator-produced state

None proven. No independent authority establishes nonzero expected contents at `34C05h`, and VAEG selects the hardware-authorized resource class.

## Production fix

None. Production fix SHA: None.

## Remaining gaps

1. The expected byte-level ownership contract for ordinary RAM at `34C00-34CFF` is not present in available hardware mapping authority; a specific loader/module producer for that range has not been identified.
2. The dormant `2730` branch for `[0374:0096] != 0` is outside the measured failing path; its full transitive semantics are not needed for the frame verdict but remain unclassified for other service inputs.
3. The alternative installer at `1CC5:C6BB` lacks a captured/fully reconstructed source extent under an authorized boot-only execution path; this is the exact E4 cross-configuration ambiguity.
4. The pre-existing protected-deletion manifest expects the old `upd9002_ops.mcr` hash and causes exactly two current validation failures.

## Changed files

- `cpu/upd9002/upd9002_trace.c`
- `tools/m74-diagnostics/run_basic_case.sh`
- `docs/agents/reports/m74q_allocation_ownership.md`

## Worktree status

Pre-existing untracked maintainer files remain: `cpu/upd9002/upd9002_trace.c.orig`, `cpu/upd9002/upd9002_trace.c.rej.orig`, and `tools/__pycache__/`. No ROM, disk, generated worker, or runtime log is staged.

## Hosted CI status

NOT RUN. Local full validation retains two specifically identified pre-existing protected-history failures.

## G74 status

NOT APPROVED.
