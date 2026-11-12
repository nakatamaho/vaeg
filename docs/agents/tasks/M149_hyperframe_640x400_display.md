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

# M149 — HyPER-FRAME+ 640×400 24bit 色 1 枚表示（VA 実機）

Status: **PLANNING; G149 NOT_READY.**
Branch: `milestone/m145-hyperframe-probe`（M145 の解析成果の上に継続）;
commit prefix `M149:`.

**ID の付け替え（2026-11-12）**: 本タスクは当初 `M146` として作成されたが、
M146 は EMS バンクのタスク（`tasks/M146_ems_bank.md`）として main に merge
済みで ID が衝突した。maintainer の確認により `M149` に付け替えた。
[7c8eb6aa](https://github.com/nakatamaho/vaeg/commit/7c8eb6aa)〜
[6dbe5c15](https://github.com/nakatamaho/vaeg/commit/6dbe5c15) の 4 コミットは
`M146:` の接頭辞のまま残る（push 済みの履歴は書き換えない）。
Base: [45f693ed](https://github.com/nakatamaho/vaeg/commit/45f693edf08e76fe8459529843908c101ae9a4a5)
（M145: document board-decode limits for EC-free VA use）.

## 目標（maintainer により再スコープ、2026-11-09）

Development Directive のレンダラ目標（320×224 16bpp、背景 2 枚、スプライト、
30FPS）は **撤回**。本タスクの目標は単純化され:

**640×400、1600 万色（24bpp）の画像 1 枚を、VA から HF+ のフレームバッファへ
I/O 転送し、HF+ 接続のモニタに表示する。** 動画・FPS・合成・スプライトは
対象外。1 枚表示できればよい。それだけでも大きい。

## 規模の認識

640×400 = 256,000 画素 × 3 成分 = **768,000 バイト** の I/O 転送。
M145 trace の規律（座標は各 RGB 成分の前に再選択、auto-increment 仮定なし）
では 1 画素あたり座標 word 2 + 成分 byte 3 = 5 操作以上。転送時間は
秒単位になる見込みであり、これは許容する（静的な 1 枚表示なので FPS 要件
がない）。

## 背景と経路の根拠

B3 の VRAM 窓は、ソフトから到達可能な全状態で無応答と実機で確定した
（別プロジェクトの M05 系列、private tree）。HF+ は **I/O のみで完結する**
経路を持つ（M145 で解析済みの seven D0-family ports: 00EE 検出、00D0 制御
RMW、00D4/00D6 word 座標、00D8/00DA/00DC 画素データ）。VA では I/O が
無条件に C-bus へ転送されることが実証済みであり、メモリーウィンドウ
（SMBC/EC/バンク窓）の BLOCKED 判定は I/O 経路には適用されない。

HF+ は独自の 15 ピンアナログ出力を持つ。表示は HF+ 接続のモニタへ出力
される（VA 本体の表示系は使わない）。

## 制約（M145 までの証拠から）

1. **EC (00ECh) は書かない**（EC-free 構成）。00D0 と 01D0 を同一視しない。
2. **フレームメモリ窓を使わない**（純 I/O）。frame window の BLOCKED 判定は
   メモリ窓経路にのみ関するものであり I/O 経路には適用されない。
3. **SMBC には触れない**（I/O は SMBC と独立、実証済み）。
4. PC-98 BIOS 呼び出しは VA に移植しない。座標は各 RGB 成分の前に再選択
   （M145 trace の規律。auto-increment が判明した場合のみ代替を検討）。
5. すべての実機 I/O は有界・オペレーター承認制。初期化は M145 trace 由来の
   限定制御列のみ。未知ポート走査・SW1 網羅試行は行わない。
6. Hyper Frame / Hyper Frame+ / Super Frame の亜種を混同しない。
7. 実行環境は PC-Engine / MS-DOS 4.0（b3e02 プロジェクトで確立した D88
   パイプライン）。表示データはまず VA RAM 上で生成する（テストパターン。
   画像ファイル読み込みは後段）。

## Checkpoint 1: バンク窓経路の到達可能性probe（優先）

オペレーターの指摘（2026-11-09）により、転送は **00EC バンク窓経路を第一候補**
とする。P1 [PUBLISHED]: 128KB 窓 80000h-9FFFFh、セレクタ 00EC、
R=C0/C1、G=C2/C3、B=C4/C5。窓経路なら REP MOVSW で 6 回のバンク切替 +
128KB × 6 の転送（推定 0.4-0.7 秒）で済み、I/O 画素転送（1.79M 操作、
推定 3-6 秒）は fallback になる。

ただし三つの未確定項目が到達可能性を gate する:

1. **00EC OUT の VA 側動作**: M144 で V1/V2 ポートは V3 でも full 16-bit
   decode されると実測済み（READ 側）。00EC への OUT が VA フォントポート、
   HF+ セレクタ、両方のどれに届くかは未測定。
2. **80000h-9FFFFh の所有権**: VA 標準 RAM は 512KB（00000-7FFFF）。
   増設 RAM ボードがなければ 80000-9FFFF は未駆動で HF+ 窓が使えるが、
   640KB 構成では DOS RAM と衝突する。
3. **HF+ 窓の decode/output-enable 条件**: reset 時の状態は board_decode.md
   の残課題。

これらを 1 本の有界 probe D88 で測定する（単一実行、撮影後電源OFF、
DOS RAM 破壊は許容）:

1. 80000-9FFFF の駆動状態を記録（パターン書込み/読戻し、FF か RAM か）。
2. IN 00EC → 元値保存。
3. OUT 00EC = C0（HF+ R 前半セレクタ）。
4. 80000-9FFFF のサンプル読み出し/書込みを再度記録し、00EC 書込み前後で
   動作が変化したか（= セレクタが HF+ に届いたか）を判定する。
5. IN 00EC で読み戻し検証し、元値を復元する。

vaeg 側は既存の `io/bmsio.c` で模擬できる（`BMS_Port=00EC` は
`BMSIO_PORT_COMPAT` として定義済み、`BMS_Size>=198` で C0-C5 を収容、
窓 80000-9FFFF、bank 0 = 主 RAM 接続）。

## Checkpoint 2（実現可能性）: 転送コスト

- 窓経路: 6 バンク切替 + REP MOVSW 128KB × 6 の実測時間。
- I/O fallback: M145 trace の転送経路から 1 画素あたりの I/O 操作数を確定
  （byte 順序 7 操作/画素）し、V30 8MHz で総時間を算出する。
- いずれも 1 枚表示として受容できる時間かを maintainer と確認する。

## Checkpoint 3（元 2）: 冷間初期化

- M145 trace 由来の限定制御列（00EE 検出、00D0 制御 RMW）を VA 上の
  有界試験で検証し、表示成立を HF+ モニタの実際の出力で確認する。
- 不成立の場合は board_decode.md の残課題（ASIC 内部・初期状態）へ戻り、
  その時点で報告する。

## Checkpoint 4: 1 枚表示プログラム

- VA RAM 上で 640×400 RGB テストパターンを生成し、窓経路（確立した場合）
  または I/O 経路（fallback）で全画素を転送する。
- 実機でパターンが正しく表示されることを確認する（撮影）。
- 画像ファイル読み込み（768KB raw RGB を 2HD 複数枚または分割）は後段。

## Gate

**G149 NOT_READY。** 転送コスト算出、冷間初期化の実機実証、実機での
パターン表示確認が揃うまで承認しない。M145 の init/banktest 実行禁止と
G145 NOT_READY は変更しない。
