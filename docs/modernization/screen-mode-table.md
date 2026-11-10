<!--
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
-->

# 画面モード対比表（スキャンライン・ANKフォント・ディスプレイ構成）

M144a で確定した、機種モード × CRT 周波数ごとの表示モデル。実機の CRT
に表示したときに「ラインとラインの間に隙間が見えるか」を基準に、
エミュレータが走査線の見た目を再現するか（する／しない）を記録する。

- 作成: 2026-11-10 (M144a)
- 実装の詳細は [M144a タスク](../agents/tasks/M144a_x88000_compat_review.md) 参照。

## PC-8001 / mkII / mkIISR（N モード、CPU 4MHz、独立経路）

| CRT | ディスプレイ構成 | テキスト ANK | テキスト走査線 | グラフィックス 640×200 走査線 |
|---|---|---|---|---|
| 15kHz | CRTC 200ラスタ ×2 = 400コンポーズ（点灯/空白インターリーブ） | 8×8 ROM を 1:1（8/10ラスタ、空き2） | する | する（1ライン = 1ラスタ） |
| 24kHz（hack） | CRTC 200ラスタ、各ラスタ2回提示 = 400コンポーズ | 8×8 ROM を縦2倍（16/20ラスタ） | しない | する（1点灯 + 1空白） |

- ANK フォント: **8×8 ROM**（font.rom / font80sr）
- **24kHz は hack**: 実機の PC-8001 / mkII / mkIISR は 15kHz 出力のみ。
  24kHz CRT はエミュレーション専用の提示で、モニタ設定（DIP SW1 と
  `Monitor15kHz`）に従って選ばれる（機種フックは 15kHz を強制しない）。
- 初代 PC-8001（model 0）は VA ファームウェア経路のまま。24kHz では
  ファームウェアが 8×16 ANK モードを選ぶため、描画側で 8×8 ROM を 1:1 に
  置き換え、偶数コンポーズラスタだけを描画して各ラスタを2回提示する。
- 初代 PC-8001 は 25 行 × 8 ラスタ（フォントは行を満たす）。640×200
  グラフィックスは該当なし。

## PC-8801 ファミリ（V1V2 モード: 8801/mkIISR/FH/MH/FA/MA2 など、独立経路）

| CRT | ディスプレイ構成 | テキスト ANK | テキスト走査線 | グラフィックス 640×200 走査線 | 640×400 走査線 |
|---|---|---|---|---|---|
| 15kHz | CRTC 200ラスタ ×2 = 400コンポーズ（インターリーブ） | 8×8 ROM を 1:1（8/10ラスタ） | する | する | 使用不可 |
| 24kHz | CRTC 400ラスタ、1:1 | 8×8 ROM を縦2倍（16/20ラスタ） | しない | する（1点灯 + 1空白） | しない（1:1） |

- ANK フォント: **8×8 ROM**（font.rom）。24kHz CRT では実機同様に
  8×16 ROM フォントは使わず縦2倍で表示する。

## VA の V1V2（VA/VA2 本体の V1/V2 モード、VA 経路）

| CRT | 8801表示オプション | テキスト ANK | テキスト走査線 | グラフィックス 640×200 走査線 |
|---|---|---|---|---|
| 15kHz | — | 8×8 ROM を 1:1（8/10ラスタ） | する（実機モデル） | する（実機モデル） |
| 24kHz | ON（emulation 専用） | 8×8 ROM を縦2倍 | しない | する（1点灯 + 1空白） |
| 24kHz | OFF（実機 VA の挙動） | 8×16 ROM を 1:1（暫定） | しない | しない（ライン二重化） |

- ANK フォント: 8801表示ON = **8×8 の2倍**、OFF = **8×16 ROM**
  （VA フォント）。

## VA モード（V3、VA 経路）

| CRT | テキスト ANK | テキスト走査線 | グラフィックス 640×200 走査線 | グラフィックス 640×400 走査線 |
|---|---|---|---|---|
| 24kHz | 8×16 ROM を 1:1 | しない（正しい挙動・変更なし） | しない（垂直拡大） | しない（1:1） |

- ANK フォント: **8×16 ROM**（VA フォント、クリーン表示）。

## 実装状況（M144a）

全セルを実装済み。ヘッドレス撮影（ゲスト領域 400 ラスタ）で次を確認した。

- PC-8001 / mkII / mkIISR: 15kHz はテキストの奇数ラスタが空白、24kHz は
  テキストのラスタが対で点灯（縦2倍、隙間なし）。mkIISR の 640×200
  グラフィックス（`LINE(0,100)-(639,100)` 等）は 15kHz・24kHz とも
  ラスタ 200/300 の1本だけが点灯する。
- PC-8801MH / FA: 15kHz は走査線あり。24kHz はテキスト縦2倍、640×200
  グラフィックスは 1点灯 + 1空白（ファームウェアは非インターレース
  モード 1 を設定する。このモードでも独立経路は奇数ラスタを空白にする）。
- VA の V1V2（15/24kHz、8801表示 ON/OFF）と VA モード（V3）は、以前の
  ビルドとバイト単位で同一の画像。
- VA の V1V2 15kHz の 640×200 走査線は、15kHz 非インターレース経路で
  既に表示されている（撮影で奇数ラスタが空白であることを確認）。
- 640×400（24kHz の 8801）は 200 ライン フラグが立たないので 1:1 の
  まま。この表の作業では撮影していない。

## 補記

- ディスプレイ構成: 全モード 400 コンポーズラスターで統一。15kHz CRT =
  点灯/空白インターリーブ、24kHz CRT = 1:1 または各ラスタ2回提示。
- CPU クロック: PC-8001 ファミリ（N/N80/N80SR）= 4MHz 規定値
  （機種フックで CPU 倍率 1 に固定）。PC-8801 ファミリ = V1/V2 クロック
  （V2-H = 8MHz）。
- キーリピート: PC-8001 ファミリはホストのオートリピート event を
  0.5 秒遅延・10 Hz に整形して注入する（ファームウェアのキーボード
  ブリッジは自前リピートを持たない）。
- 下部ステータスバーの並び: 機種 | モード | CPU | SGP | CRT | FPS | FRAME。
  CRT は `videova_hsyncmode()` から 15kHz/24kHz を表示する。
