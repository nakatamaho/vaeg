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

# M146 — HyPER-FRAME+ I/O レンダラ（VA 実機）

Status: **PLANNING; G146 NOT_READY.**
Branch: `milestone/m145-hyperframe-probe`（M145 の解析成果の上に継続）;
commit prefix `M146:`.
Base: [45f693ed](https://github.com/nakatamaho/vaeg/commit/45f693edf08e76fe8459529843908c101ae9a4a5)
（M145: document board-decode limits for EC-free VA use）.

## 背景と再適用

B3 の VRAM 窓は、ソフトから到達可能な全状態（イネーブル有無 × SMBC 状態 ×
SR7 ルーティング確認済み）で無応答であることが実機で確定した（別プロジェクト
の M05 系列試験、private tree）。オペレーターの判断により、Development
Directive のレンダラ目標（320×224、16bpp、独立スクロール背景 2 枚、
半透明 32×32 スプライト 50 枚、30 displayed FPS 維持）を
**HyPER-FRAME+ のフレームバッファ**へ再適用する。

M145 で確立した証拠により、HF+ は **I/O のみで完結する**経路を持つ
（seven D0-family ports: 00EE 検出、00D0 制御 RMW、00D4/00D6 word 座標、
00D8/00DA/00DC 画素データ）。VA では I/O が無条件に C-bus へ転送される
ことが実証済みであり、メモリーウィンドウ（SMBC/EC/バンク窓）の BLOCKED
判定は I/O 経路には適用されない。

## 出力先

HF+ は独自の 15 ピンアナログ出力を持つ。レンダリング結果は HF+ 接続の
モニタへ出力される（VA 本体の表示系は使わない）。

## 制約（M145 までの証拠から）

1. **EC (00ECh) は書かない**（EC-free 構成）。00D0 と 01D0 を同一視しない。
2. **フレームメモリ窓を使わない**（純 I/O）。frame window の RAM 競合・
   EC・初期応答の BLOCKED 判定は、メモリ窓経路にのみ関するものであり
   I/O 経路には適用されない。
3. **SMBC には触れない**（I/O は SMBC と独立、実証済み）。
4. PC-98 BIOS 呼び出しは VA に移植しない。座標は各 RGB 成分の前に再選択
   （M145 trace: address hold / auto-increment は未確認であり仮定しない）。
5. すべての実機 I/O は有界・オペレーター承認制。初期化は M145 trace 由来の
   限定制御列のみ。未知ポート走査・SW1 網羅試行は行わない。
6. Hyper Frame / Hyper Frame+ / Super Frame の亜種を混同しない（M145 の
   分離を維持）。

## Checkpoint 1（実現可能性）: I/O 帯域

- M145 trace の転送経路から 1 画素あたりの I/O 操作数を確定する
  （現状の想定: 座標 word 2 + RGB byte 3 = 5 操作/画素）。
- V30 8MHz の I/O 操作コストを推定し、全画面転送・ダーティ矩形転送の
  到達 FPS を算出する。30 FPS 維持の可否とアーキテクチャ（ダーティ画素
  追跡、HF+ のベースアドレス更新 / auto-increment 相当機能の有無調査）を
  ここで決定する。
- 帯域が不足する場合の代替（部分更新の徹底、目標 FPS の再合意、
  表示内容の簡素化）は maintainer の承認を得る。

## Checkpoint 2: 冷間初期化

- M145 trace 由来の限定制御列（00EE 検出、00D0 制御 RMW）を VA 上の
  有界試験で検証し、表示成立を HF+ モニタの実際の出力で確認する。
- 不成立の場合は board_decode.md の残課題（ASIC 内部・初期状態）へ戻り、
  その時点で報告する。

## Checkpoint 3: レンダラ実装

- VA RAM 上のコンポジタ（背景 2 枚 + スプライト 50 枚、16bpp 合成）と
  ダーティ画素転送を実装する。
- 実行環境は PC-Engine / MS-DOS 4.0（b3e02 プロジェクトで確立した
  D88 パイプライン）。
- 資産・フレームバッファは VA RAM に置く（HF+ VRAM へは I/O 転送のみ）。

## Gate

**G146 NOT_READY。** 帯域算出、冷間初期化の実機実証、実機レンダリング結果
が揃うまで承認しない。M145 の init/banktest 実行禁止と G145 NOT_READY は
変更しない。
