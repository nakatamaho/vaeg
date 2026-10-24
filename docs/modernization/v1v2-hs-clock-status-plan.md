<!--
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
-->

# PR #55 — H/Sクロックと機種・動作情報の設計

Status: **合意済み設計、M132実装中。実装完了・実機校正済みではない。**

## 目的と境界

[PR #55](https://github.com/nakatamaho/vaeg/pull/55)を土台に、H/Sの実行速度、
IN6Ehの識別値、起動機種・動作モード・クロックの表示を整合させる。
レビュー対象のPR先端は `1091af8b260139a69d16d7afdb36883678655589`。
PR #56のモノクログラフィックス修正は含めない。

[M132タスク](../agents/tasks/M132_hs_clock_status.md)で実装・検証する。
M131で受け入れたCPUバンクと表示元の分離、全VAモードでの高速テキストRAM、
IN31hのF9h/79hを維持する。IN31h bit 6をH/Sの判定根拠にしない。

## H/Sの仕様

| 項目 | 合意した仕様 |
|---|---|
| H | 基準7.9872MHz相当。通常表示は `CPU 8MHz` |
| S | 基準3.9936MHz相当。識別だけでなく実行速度も低下させる |
| 追加調整 | CPU倍率・速度%は残す。フレーム・タイマー・音楽の速度と分離する |
| 対象範囲 | 88互換モードのH/S。V3ソフトのBRKEM実行へ保存済みS設定を無条件適用しない |
| 待ち時間 | 互換命令サイクルと既にネイティブ側で計上された待ちを区別し、二重換算しない |
| ROMトラップ | ネイティブ処理時間を区別する。全処理が厳密に半速になるとは主張しない |
| FDC | uPD780Cの速度は変更しない |
| 識別値 | VA/VA2の有効な1C9h bit 5状態と、バックアップRAMのH/S選択を整合させる |
| 設定互換 | 旧S・x2・50%などの追加半速化を説明し、意図的なカスタム設定を推測で書き換えない |
| プリセット | 既存プリセットを無条件に削除しない |

実機の命令・メモリ・I/O・ROMトラップを分けた計測は別の証拠が必要。
[速度のissue #58](https://github.com/nakatamaho/vaeg/issues/58)と
[識別のissue #59](https://github.com/nakatamaho/vaeg/issues/59)は、実装や算術試験だけで
実機校正完了として閉じない。

## 情報メニューと下部表示

| メニュー | 意味 | 初期値 |
|---|---|---|
| 情報 → 機種・動作情報 | 下部ステータスバーの表示/非表示 | ON |
| 情報 → 前面パネル (ランプ) | 既存パネル。「画面」から移動 | 既存設定維持 |
| 情報 → CPU/SGP/FPS/FRAME等 | 情報ごとの表示設定 | 既存設定維持 |

新しい表示設定がない場合も機種・動作情報はON。設定を保存する。
ステータスバーと前面パネルは独立して切替可能とする。
タイトルバーからクロック・FPS・FRAMEの重複を除き、基本はアプリ名とする。
既存のFDD情報表示は互換性を保つ。ゲストの画面サイズ・画素を情報表示で変更しない。
狭い画面ではクリップ等で重なりを避け、全情報はツールチップで確認できるようにする。

通常表示は短い英数字のみとする。

```text
PC-88VA2 | V2S | CPU 4MHz | SGP 8MHz | 60.0FPS
PC-88VA2 | V2S | CPU 2MHz | SGP 8MHz | 60.0FPS
PC-8801MH | V2H | CPU 8MHz | 60.0FPS
PC-8801mkIISR | V2 | CPU 4MHz | 60.0FPS
PC-8001mkII | N80 | CPU 4MHz | 60.0FPS
```

例は表示形式であり、全機種の全モードが検証済みという意味ではない。
機種名は起動メニューで使う正式名を共通利用し、内部のVA/VA2対応先を表示しない。
SR以降のPC-8801系にはV1/V2/Nを、H/Sのある機種にはH/Sも表示する。
SGPはPC-88VA系のみ表示する。

CPU/SGPは設定から計算した公称値、FPSはゲスト描画の実測値とする。
ホスト負荷やNo Waitによって公称クロック表示を増減させない。
通常は整数MHz（7.9872→8、3.9936→4、1.9968→2）を表示し、
1MHz未満ではゼロMHzにならないよう小数を許す。詳細は英数字のツールチップへ出す。

```text
CPU: 1.9968 MHz
Base: 3.9936 MHz
Speed: 50%
```

FRAMEは通常非表示、必要時のみ表示する。FPSはモニター周波数や描画APIのpresent数と
混同しない。設定の変更、リセット、状態復元でも表示の意味を維持する。

## モード表示とランプ

共通の判定を使い、メニューの内部名とゲストのBASICの種類を混同しない。

| 動作モード | V1ランプ | V2ランプ | 表示 |
|---|---|---|---|
| V1系 | ON | OFF | V1 / V1H / V1S |
| V2系 | OFF | ON | V2 / V2H / V2S |
| N系 | OFF | OFF | N / N80 / N80SR（確認できた範囲） |

N系で内部に残ったVA側の設定からV2ランプを点灯させない。
ROM内にBASIC名が含まれるだけで、そのBASICが実行中とは判定しない。
判別できないサブモードは曖昧さを明示し、未確認の具体名を断定表示しない。

## PC-8001系の観測と積み残し

メンテナはPC-8001mkIIでN80-BASICが起動し、同時にV2ランプが点灯したと報告した。
「N80は起動しない」という以前の説明は撤回する。一方、起動だけで固有機能の
全実装を証明したとは扱わない。

| 積み残し | 内容 |
|---|---|
| N/N80の識別 | mkIIの実際のモードと選択・切替・表示の対応を確認する |
| mkII固有機能 | グラフィックスモード、拡張ROMバンクなどの対応状況を確認する |
| mkIISR | N/N80/N80SRごとに起動可否と機能対応を確認する |
| 実行中の切替 | ゲスト側の切替と表示・状態復元の対応を確認する |
| 拡張可能性 | 将来のN80/N80SR対応で、共通モード欄を追加・切替可能とする |

未対応機能の全面実装は今回の表示整理に含めない。単にBOOTMODE_Nであることを
理由に「Nのみ実装」と断定する既存文書も訂正する。

## 検証と分割

IN6EhではVA/VA2、V1/V2×H/S、有効/なし/不正バックアップ、自動/手動RAM、
初回/再リセット、H→S→H、保存/復元を確認する。
タイミングでは命令スケール、既計上待ち、V3隔離、倍率・速度%を独立に試験する。
表示では機種プロファイル、N系ランプ、丸め、SGP非表示、設定保存、狭幅と
プレゼンター経路を確認する。M131回帰、全体CTest、媒体・デモ回帰と
クリーン静的Windowsハンドオフを実施し、人間ゲートG132で止める。

設計、識別、タイミング、共通表示モデル、GUI、検証記録は別コミットとする。
