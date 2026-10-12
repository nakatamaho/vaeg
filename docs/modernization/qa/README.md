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

# vaeg の QA

vaeg の QA は、実行に必要な非公開の入力によって3つのレベルに分けている。検査の正しさは
比較相手（オラクル）の正しさを超えないので、以下の各検査には比較相手と、その比較相手で
見えないものを明記する。

| レベル | ROM | ディスクメディア | 実行場所 | 状態 |
|---|---|---|---|---|
| 外部 | なし | なし（固定された公開入力と、コミット済みの実機記録のみ） | hosted GitHub Actions と任意のチェックアウト | 実装済み |
| ローカル B | メンテナの ROM | なし | メンテナの環境 | 定義のみ、未実装 |
| ローカル A | メンテナの ROM | PC-Engine のシステムメディアと、その他の非公開 NEC メディア | メンテナの環境 | 一部実装（個別のハーネス、下記） |

CPU ごとの検査範囲は別の文書にまとめている。

- [Z80 互換 CPU](z80-family.md): BRKEM で入る uPD9002 の uPD70008 互換モードと、FDC
  サブシステムの uPD780C。
- [uPD9002 ネイティブモード](upd9002-native.md): SingleStepTests V20 コーパスと比較する
  V30 系命令セット。

## 比較相手の優先順位

情報源どうしが食い違うときは、上位のものを採る。

1. 実機 PC-88VA（VA、VA2）での測定。プログラムとメディアの識別情報つきで、バイト単位で
   記録したもの（`docs/agents/reports/`）。
2. NEC の資料と、VA の ROM の静的解析（モニタの表、ファームウェアのコード）。
   `docs/modernization/` と `docs/bios/` に記録している。
3. 関連する参照 CPU。ネイティブ命令セットには SingleStepTests V20 コーパス、Z80 には
   ZEX が持つ Zilog Z80 の標準 CRC。
4. エミュレータ自身の過去の出力（決定性と退行の検出のみ）。

3 または 4 に対する合格は、その参照についての証拠であって、PC-88VA についての証拠では
ない。CPU ごとの文書では、各項目がどのレベルまで届いているかを書いている。

## 外部レベル

ROM もメディアもない素のチェックアウトで動く。入力は、追跡中のソースからビルドするか、
固定したリビジョンを取得してダイジェストを検証したものだけである。

| 入力 | 固定と検証 |
|---|---|
| SingleStepTests V20 `v1_native` | 上流のコミットとファイルごとの SHA-256（`tests/ssts/v20_dataset_manifest.json`） |
| ZEXDOC/ZEXALL のバイナリ | suzukiplan/z80 のコミットと SHA-256（`tests/z80_compat/fetch_zex.py`） |
| CP/M のプローブプログラム | `external/zex/` と `tools/cpmva/zex/` から z80asm 1.8 でビルド |
| 実機の記録 | `docs/agents/reports/m101_zexall_qa/` と `m102_zexund_qa/` にコミット済み |

hosted CI（`.github/workflows/build.yml`）:

| ジョブ | 検査内容 |
|---|---|
| `lint` | 文字コード、改行コード、パス名の大文字小文字 |
| `guest-drivers` | NASM によるゲストドライバ一式とそのマニフェスト |
| `upd9002-ssts` | uPD9002 ネイティブモードと V20 基準ファイルの比較 |
| `compat-conformance` | Z80 バックエンドの約束事、役割ごとの ZEX、役割ごとのプローブ |
| `linux` gcc/clang、`linux-asan`、`windows`、`macos` | ビルド、ROM なしの `--smoke`、CTest 全件 |
| `windows-release` | リリース用ビルド、smoke、PE インポートの監査 |

外部の検査では原理的に見えないもの: NEC のファームウェアやゲストメディアがないと動かない
ものすべて。BIOS の初期化、IPL、ファームウェア下でのモード切り替え、ゲストのソフトウェアが
出す画面表示、本物のサブシステムファームウェアを通した FDC の通信、音声出力、OS。ROM なしの
`--smoke` では一色画面の検出が無効になっている。

## ローカルレベル B（ROM あり、ディスクなし）

目的: 実際のファームウェアがエミュレータ上で機械を正しく初期化することを確かめる。
予定している検査: `va` と `va2` での ROM ありの `--smoke`、V3/V2/V1 各モードでディスクなしの
起動到達点を `--screen-tvram-dump` で照合、ROM BASIC がある場合の往復、繰り返し実行の決定性、
ステートの保存と読み込みの往復。いずれもまだ CTest に登録しておらず、ディスクなしで到達する
画面の期待値も記録していない。

## ローカルレベル A（ROM と PC-Engine メディアあり）

目的: 人間ゲート（V3 モードでの起動、同梱デモの実行、OS を起動して簡単な操作）のうち、
機械で判定できる部分を置き換える。

既存のハーネス:

| ハーネス | 状態 |
|---|---|
| `tools/qa/m103b_basic_boot.py`（V2 BASIC） | `vaeg_m103b_basic_boot` として登録済み。`VAEG_V1V2_ROM_DIR` と `VAEG_V1V2_BASIC_MEDIA` を設定しない限りスキップ |
| `tools/qa/m75_storage_regression.py` の実行モード（SASI/SCSI/HOSTFAT） | 手動のみ |
| `tools/m74-diagnostics/run_debug_case.sh` | 手動のみ |
| PC-Engine 上の CP/MVA（`tools/cpmva/`） | 手動。M101/M102 の実機記録の出どころ |

予定: PC-Engine での V3 起動とプロンプト到達、`DIR`/`VER` の応答、同梱デモ1本の実行、
上記の既存ハーネスを1つのスキップ規約のもとにまとめること。

非公開の入力は Git の外に置き、追跡するファイルには中立的な識別子だけを使う
（`AGENTS.md` を参照）。
