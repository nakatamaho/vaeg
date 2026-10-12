<!--
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
-->

# uPD9002 ネイティブモードの QA

uPD9002 のネイティブモードは V30 系の命令セット（`cpu/upd9002/`）に NEC の変更を加えたもので
ある。V30 の4命令が削除され、`BRKEM`/`BRKFEM` で uPD70008 互換モードに入り、システム I/O
領域に V50 風の内蔵周辺回路がある（`docs/modernization/upd9002-upd70008-mode.md` §1、
§4.1〜4.2、§5）。uPD9002 の1命令単位のテストコーパスは存在しないので、命令単位の比較相手は
最も近い参照 CPU である NEC V20（uPD70108）になる。3つのレベルと比較相手の優先順位は
[QA の概要](README.md) を参照。

## 1. V20 コーパスで分かることと分からないこと

SingleStepTests V20 `v1_native`（`tests/ssts/v20_dataset_manifest.json` で固定）は、命令ごとに、
V20 で1命令を実行する前後の CPU の状態を記録している。比較相手としてはレベル 3 である。

- uPD9002 と V20 が共通に持つ命令では、一致は強い証拠になる。
- uPD9002 が V20 と異なる命令では、V20 の結果は vaeg にとって誤りであり、合格にも不合格にも
  数えてはならない。
- uPD9002 にあって V20 にないものについては、コーパスは何も言わない。

以下の判断は、ROM から読んだモニタの表（M60b）、意味論の移行計画
（`docs/agents/UPD9002_SEMANTICS_MIGRATION.md`）、G67 の相違登録簿に基づく。G67 の登録簿は
commit `884b7dca97b6576007baa235b81a2597fa5f0c22` の
`tests/ssts/divergence/g67/registry.json` に残っている。

## 2. 外部レベルの検査

`vaeg_sst_v20_ci`（ジョブ `upd9002-ssts`、ツール `tools/qa/sst_v20.py`、基準ファイル
`tests/ssts/v20/baseline_ci.json`）:

| 項目 | 内容 |
|---|---|
| 選び方 | 初期プリフェッチキューが空のレコードを、上流のハッシュの昇順に、形式ごとに先頭500件（`ci` プロファイル） |
| 実行するもの | `vaeg --upd9002-ssts-worker`。本番の命令コアを、平坦なテスト用メモリと I/O の受け口で動かす |
| 比較する項目 | 全レジスタ、コーパスのフラグマスクをかけた FLAGS、レコードが挙げる全 RAM バイト、I/O の読み書き、終了の仕方（通常、HALT、type-0 例外） |
| 判定対象 | 341形式の170500レコード、すべて合格 |
| `target_absent`（判定しない） | 4500レコード: `0F31`、`0F33`、`0F39`、`0F3B`（INS/EXT）、`6C`〜`6F`（INM/OUTM）、`63` |
| `excluded`（判定しない） | 5000レコード: `66`、`67`、`D8`〜`DF`（上流の状態が `fpu`） |
| 失敗の扱い | 判定対象のレコードが1件でも失敗すれば退行（`SST_V20_REGRESSION`）。許容する失敗はない |

全件プロファイル（空キューのレコード1,562,502件すべて）はツールが対応しているが、CI では
実行しておらず、基準ファイルもコミットしていない。

## 3. V20 検査が覆う範囲

判定対象の形式は、すべて uPD9002 が持つと記録されている命令で、比較相手は V20 である。

- V30 にある 8086/80186 の基本命令。M70 が明示的な権限のもとで実装した V30 の
  `REPC`/`REPNC`（`64`/`65`）付き文字列命令（それらのプレフィックス付きの `A4`〜`AF`）を含む。
- uPD9002 のモニタの表にある V30 拡張命令: `TEST1`/`NOT1`/`CLR1`/`SET1`（`0F10`〜`0F1F`）、
  `ADD4S`/`SUB4S`/`CMP4S`（`0F20`/`22`/`26`）、`ROL4`/`ROR4`（`0F28`/`2A`）。
- それらのレコードに含まれるセグメント、`LOCK`、リピートのプレフィックス。
- ソフトウェア割り込み、`INTO`/`BOUND`/除算の例外、`HLT`。いずれも1命令の終了の仕方から
  分かる範囲に限る。

## 4. V20 検査で見えないもの

| 対象 | 理由 | 検査されている場所 |
|---|---|---|
| FLAGS の未定義ビット | コーパスのマスクが除外している。全16ビットの比較は診断用のみ（G67 の `fingerprint`） | 実機との照合なし |
| プリフェッチキューの影響 | コーパスの半分（1,562,498件）はキューが空でない状態から始まるが、ワーカーはキューを注入できない | なし |
| サイクル数、バスタイミング、ウェイト | 設計上比較しない | なし |
| 実機 uPD9002 での `0F31/33/39/3B`、`6C`〜`6F`、`63` | 対象外の命令で、uPD9002 での挙動は記録されていない。vaeg には `6C`〜`6F` 用の V20 の `INS`/`OUTS` 処理がまだ残っており、整理は未解決の課題（G67 `reserved_behavior.primary_6c_6f_cleanup`） | 未解決 |
| `66`/`67`（FPO2）と `D8`〜`DF`（ESC） | 上流の状態が `fpu`。uPD9002 のシリコンが FPO2 を実装しているかは確定していない（G67） | 8087 コア: `vaeg_8087_core`（`tools/8087/test_core.c` の単体テスト）。命令単位の実機の証拠ではない |
| `BRKEM`（`0F FF imm8`）、`BRKFEM`/`BRKEM2`（`0F FE imm8`） | コーパスにレコードがない。BRKFEM の意味論は確定していない（G67 のカバレッジゼロの記録） | `vaeg_upd9002_brkem_upd70008`（合成データ） |
| `RETEM`、`CALLN`、MD フラグ | 互換モード側の担当 | [z80-family.md](z80-family.md) を参照 |
| 命令の途中で割り込みや例外が入った後のプレフィックスと再開の挙動 | 1命令のレコードでは表せない。G67 では実機に問うべき課題 | 未解決 |
| ハードウェア割り込み、NMI、割り込みコントローラ、DMA、タイマ、リフレッシュ | V50 風の周辺回路で、命令の意味論ではない | ローカルレベル。`docs/modernization/upd9002-upd70008-mode.md` §5 |
| 平坦なテスト用メモリを超えるメモリマップ（バンク、VA のメモリ制御、マップされた領域をまたぐセグメント付きワードアクセス） | ワーカーは平坦なメモリを使う | `vaeg_upd9002_m68_segmented_memory`（合成データ） |
| I/O トラップ機構 | 機械の機能（モード文書の §9） | ローカルレベル A |
| 形式ごとに501件目以降のレコード | `ci` プロファイルのみ | 全件プロファイルを必要に応じてローカルで実行 |

## 5. vaeg 内部のネイティブ側テスト

これらは `vaeg` の入口を、記録された意味論、ROM の証拠、マイルストーンの記録から書いた
期待値で実行する。合成データ（レベル 2 または 4）であり、実機の測定ではない。

| CTest | 内容 |
|---|---|
| `vaeg_upd9002_flags_materialization` | FLAGS の正規化とゲストから見える値の生成（M60a） |
| `vaeg_upd9002_iret_restoration` | IRET での復元（M60e） |
| `vaeg_upd9002_mov_imm_register` | C6/C7 のレジスタ形式の MOV 即値（M61） |
| `vaeg_upd9002_semantics_bundle` | 意味論修正の統合（M62） |
| `vaeg_upd9002_m64_semantics` | DIV/IDIV と、モニタが裏付ける `0F` 命令（M64） |
| `vaeg_upd9002_m65a_ff7` … `m65e_tail10` | FF /7、BOUND、F7 /2 NOT、FF /6 と残り10ケース（M65a〜M65e） |
| `vaeg_upd9002_m68_segmented_memory` | マップされたメモリでのセグメント付きワードアクセスの振り分け（M68） |
| `vaeg_upd9002_m70_prefix_string` | REPC/REPNC 付きの文字列命令（M70） |
| `vaeg_upd9002_m96d_nop` | NOP の扱い（M96d） |
| `vaeg_upd9002_rep0f_diagnostic_stop` | REP + `0F` の診断停止 |
| `vaeg_upd9002_dispatch_normalization`、`_static`、`vaeg_upd9002_native_invariant`、`vaeg_upd9002_rep0f_transition_*`、`vaeg_upd9002_protected_deletion`、`vaeg_upd9002_rename` | 振り分け表の構造と、退役したコードの検査 |
| `vaeg_upd9002_abi`、`_state_boundary`、`_state_payload_probe`、`_m66a_state_verify`、`_trace_equivalence` | ステート形式、保存と読み込みの境界、決定的なトレース |
| `vaeg_upd9002_ssts_worker_selftest` | SST ワーカーのプロトコルそのもの |

## 6. コードと基準ファイルの更新

| コード | 意味 | 対応 |
|---|---|---|
| `SST_V20_REGRESSION` | 基準で合格していた判定対象のレコードが失敗した | エミュレータを直す |
| `SST_V20_BASELINE_STALE` | 失敗として記録されていたレコードが合格した | `update` を実行して基準ファイルをコミットする |
| `SST_V20_IDENTITY_MISMATCH` | データセット、プロファイル、選び方の規則が異なる | 意図して基準ファイルを作り直す |
| `SST_V20_FORM_SET_MISMATCH` | どちらか一方にしかない形式がある | 同上 |
| `SST_V20_SELECTION_MISMATCH` | 形式ごとの選択件数か状態（`executed`、`excluded`、`target_absent`）が異なる | 同上 |
| `SST_V20_BASELINE_SCHEMA` | 文書の形式が壊れている | ファイルを直す |

```sh
python3 tools/qa/sst_v20.py ci --dataset-root "$V20" --worker build/linux-ci-gcc/sdl2/vaeg \
  --baseline tests/ssts/v20/baseline_ci.json --output /tmp/sst-v20-ci.json
python3 tools/qa/sst_v20.py update --result /tmp/sst-v20-ci.json \
  --baseline tests/ssts/v20/baseline_ci.json
```

結果ファイルには、失敗したレコードごとに結果、不一致の種類、命令バイトが入る。診断用で、
コミットはしない。

## 7. 結果の読み方

- `vaeg_sst_v20_ci` の合格が意味するのは、uPD9002 が V20 と共通に持つと記録されている
  すべての形式の選択レコードで、エミュレートしたネイティブコアが V20 と同じアーキテクチャ上の
  結果を出す、ということである。uPD9002 の実機検証ではない。
- `target_absent` の形式の変化は、設計上この検査には見えない。そこでの uPD9002 の挙動を決めるには
  実機の証拠が要る。
- 基準ファイルを作り直すのは意図した変更のときだけにし（`sst_v20.py update`）、コミットで
  変更の理由を説明する。
