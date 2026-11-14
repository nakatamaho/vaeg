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

Status: **PAUSED (temporary main merge on 2026-11-14, maintainer
direction); G149 NOT accepted; no release authorized.**
Branch: `milestone/m145-hyperframe-probe`（M145 の解析成果の上に継続）;
commit prefix `M149:`.
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

## 2026-11-14: VA 実機 1 回目（build 0ef28322、HFV1 旧版）

ログは Git の外（`~/work/pc88va/tmp/m148/HFLOG.va1-1.TXT`）。HFV1 を
byte 座標 → word 座標（`/WORD`）→ `/OFF` → byte 座標の順に 4 回実行。

- `[MEASURED]` 1 回目の PRE（電源投入直後、何も書いていない状態）は
  D0/D3/D8/DA/DC/EE すべて FF。HF+ は I/O の IN には無応答。
- `[MEASURED]` 2 回目の PRE（1 回目の INIT/DRAW の後、`/OFF` の前）は
  **D0 = E1h、D3 = 00h、D8 = DA = DC = FEh と駆動されている。** 1 回目の
  INIT（EE=C0、D0=A5、D3=00、…、D0=E1）の後、ボードはこの状態を保持して
  いる。D0 の読み値 E1h は INIT の最後の D0=E1 と一致し、**制御レジスタ
  への書き込みが届き、読み戻せる**。D8/DA/DC の FEh は INIT が Y=0193、
  X=009F に DA=00 を書いた後の座標位置の VRAM 読み値（FEh）と整合する。
- `[MEASURED]` 全 12 点の RB が書いた値と一致（両モードとも）。**座標 +
  画素の書き込みと読み出しが、byte 座標・word 座標の両方で VRAM に届く。**
- `[MEASURED]` `/OFF`（D0=00、EE=00）の後の PRE は再びすべて FF。停止手順
  はポートを無駆動状態に戻す。
- 保守者によるとモニタには何も出ず（RGB ケーブルなし、HF+ RGB IN も
  未接続。同期の有無は未観測）。
- 判定: **「VA から HF+ に I/O で届くか・書けるか」は PASS。** 未解決は
  表示モードへの移行（同期・モニタ出力）と RGB ケーブルの入手。
- 次の一歩: INIT の後の D0=E1 が駆動を確認したので、表示モードの条件は
  D0/D3/EE と特殊座標のどこかにある。B3E 系で使った「ラッチ/読み出し」
  の観測は不要（読み出しが普通に機能する）。J1 が PC-98 BIOS（INT 18h
  AH=41h）を呼ぶ点と、HF+ が本体 RGB 入力の同期に依存する可能性を、
  次の probe（HFV2）で切り分ける。

## 2026-11-14: VA 実機 2 回目（HFV2 表示有効化スイープ、build e1eaa80e）

ログは Git の外（`~/work/pc88va/tmp/m148/HFLOG.va1-2.TXT`）。電源投入直後
の PRE は再び全部 FF。INIT + 上 64 行のカラーバー描画の後、D0 に 10 値
（A1/A3/A5/A9/B1/E1/E5/21/61/FF）、D0=E1 固定で EE に 5 値
（C0/40/80/00/FF）を順に書いた。保守者の観察: モニタ（HF+ OUT 接続、
CN2 入力なし）は**「信号なし」のまま一切変化なし**。

- `[MEASURED]` D0/EE のどの候補値でも映像出力は開始しない。
- `[MEASURED]` 実装上の欠陥: hold は 5 秒のつもりが約 1 秒だった
  （DOS INT 21h AH=2Ch が CX を壊し、秒カウンタと衝突）。スイープ自体は
  全値を書けており、判定には影響しないが、次版では修正する。
- `[PHOTO]` 基板写真（IMG_7935〜7937）から出力段の構成を確認:
  D-sub 15（ITT CANNON RDA16STW3）= モニタ出力、CN2（6 ピン）= 本体 RGB
  入力、74HC4053AP × 2 = 入力/自前 DAC（MB40978）のアナログ切り替え、
  GRF MEM（Digital Arts）= VRAM、X1 = 結晶。
- `[INFERRED]` 4053 の切替ビットは D0/EE のどの値にも見つからなかった。
  自前 DAC 出力の同期生成が CN2 の入力同期に依存する（genlock/オーバーレイ
  構成）可能性が最有力。J1 は PC-9801 上で CN2 を本体 RGB につないで運用
  しており、入力ありでのみ表示を確認している。
- 次の一歩: CN2 入力ケーブルの自作（VA の D-sub 15 → CN2 6 ピン）。
  ピン配置は CN2 はんだ面の写真の追跡で確定する。6 信号（R/G/B/H/V/GND）
  の割当が最有力。
- 再試験の理由: スイープの hold が 1 秒だったため、モニタの信号検出
  （通常 1 秒以上）より短く、切替ビットがあっても見逃した可能性がある。
  hold を 8 秒に修正した HFV2（wait_secs は INT 21h 2Ch が CX/DX を壊す
  ため、秒カウンタをメモリ変数に移した）で再試験する。
- CN2 のはんだ面写真（IMG_7942/7943）を確認したが、角度と解像度の関係で
  6 ピンそれぞれの配線は追跡できていない。部品面からは CN2 → R34/R36、
  D3/D4、R49、R40 → 4053、同期系は TR13/TR16 と思われる経路が見える。
  ピン配置の確定には CN2 真下のはんだ面のマクロ写真が要る。

## 2026-11-14: VA 実機 3 回目（HFV2 修正版、build 509a8e60）

ログは Git の外（`~/work/pc88va/tmp/m148/HFLOG.va1-3.TXT`）。hold を 8 秒
に修正したスイープ（タイムスタンプが 8 秒間隔で、意図どおり実行された
ことを確認）。D0 10 値 + EE 5 値、合計 15 値。保守者の観察: **モニタは
「信号なし」のまま一切変化なし**。

- `[MEASURED]` 保持時間を十分にしても、D0/EE のどの候補値でも映像出力は
  開始しない。**表示の有効化は D0/EE のビットでは実現されない。**
- `[INFERRED]` 4053 出力段の構成と合わせると、自前 DAC 出力の同期が CN2
  の入力同期に依存する（genlock/オーバーレイ）構成が最有力。J1 の運用
  環境（PC-9801 + CN2 接続済み）では入力が常にあった。
- ソフトだけでの表示確立はここで打ち止め。次は CN2 入力ケーブルの自作
  （VA D-sub 15 → CN2 6 ピン）。ピン配置は CN2 真下のはんだ面マクロ写真
  の追跡で確定する。

## 2026-11-14: VA 実機 4 回目（HFV3 D3 スイープ、build 414a8daa）

ログは Git の外（`~/work/pc88va/tmp/m148/HFLOG.va1-4.TXT`）。D3 = 01 02 03
04 08 10 20 40 80 FF（D0=E1、EE=C0）+ D0 スイープ 10 値（D3=01）。各 8 秒。
保守者の観察: **モニタは「信号なし」のまま一切変化なし**。テスタは見つかっ
たが電池切れ。

- `[MEASURED]` D3 のどの値でも映像出力は開始しない。
- 判定: ソフトからの表示有効化の候補（D0、EE、D3）はすべて尽くした。
  ボードの写真（4053 出力段、自前結晶 21.0526 MHz）と合わせ、自前出力の
  同期が CN2 の入力同期に依存する構成（genlock/オーバーレイ）と結論する。
  表示には CN2 入力ケーブルが必須。
- 次の一歩: テスタの電池交換の後、パススルー/駆動状態での CN2 ↔ D-sub 15
  出力間の抵抗測定で CN2 のピン配置を確定し、VA D-sub 15 → CN2 ケーブルの
  結線表を出す。

## 2026-11-14: vaeg への暫定実装（保守者の指示）

保守者の指示で、測定済みの資料に基づく HyPER-FRAME+ のボードモデルを
vaeg に実装した。コミットは本節の追加と同時。

**実装したもの**（実機で検証できた挙動のみ）:

- `io/hfplus.c` / `io/hfplus.h`: ボードモデル。ポート 00D0/00D3/00D4/
  00D5/00D6/00D7/00D8/00DA/00DC/00EE。測定された意味論:
  - 冷間（電源投入直後）は無駆動で IN は FF。停止手順 D0=00/EE=00 でも
    無駄駆動に戻る。control/ee のどちらかが非ゼロで駆動（INIT 後の
    D0=E1/D3=00 読み戻しと一致）。
  - IN 00D0/00D3 は最後に書いた値を返す。IN 00D8/00DA/00DC は現在の座標
    の R/G/B を返す（12 点読み戻しと一致）。
  - IN 00EE は駆動していても FF（実機 2 回目の PRE で EE=FF を確認）。
    IN 00D4-00D7 は未測定のため attach しない（FF を返す）。
  - 座標は byte 順序（D7 D6 D5 D4）と word OUT（D6、D4）の両方で届く
    （実機で両方確認）。フレームバッファは 6 バンク分 786432 バイトで、
    Y=0191/0193 の特殊座標も範囲内。行 0-399 が可視ラスタ。
- `vram/scrndrawva.c`: 暫定映像出力。駆動状態のとき、HF+ のフレーム
  バッファ（640×400 24bit）を最隣接スケーリングで画面全体に重ねて
  表示し、非駆動なら本体画面に戻す（スルー）。**実機では有効化条件が
  未確定（「信号なし」）なので、これは暫定の合成表示であり、実機の
  挙動の再現ではない。**
- 設定: `np2cfg.hfplus_enable`（vaeg.ini の `HyperFramePlus`、既定
  OFF）。GUI の「デバイス」メニューに「HyPER-FRAME+ (要リセット)」を
  追加（ポートの bind はリセット時なので要リセット）。
- 状態保存: `HFPLUS` セクション（レジスタ 6 バイトのみ。フレーム
  バッファは状態ファイルに保存しない。電源投入時のクリアと同じ）。
- テスト: `tests/hfplus/model.c`（CTest `vaeg_hfplus`）。冷間 FF、INIT
  後の D0/D3 読み戻し、12 点 × byte/word の読み戻し、特殊座標の範囲、
  停止手順、リセット（電源投入相当）のクリア、PPM ダンプ。111 check。
- デバッグ用に `hfplus_write_ppm()` を追加（可視ラスタの P6 出力）。

**実装しなかったもの**: 4053/出力段の実機の有効化条件、CN2 入力（genlock
仮説は未検証）、特殊座標領域のレジスタ意味論（実機の読み値 FE とモデルの
不一致は未解決）。

`vaeg_hfplus` と `vaeg_sdl2` のビルドを通し（macOS, clang）、
`vaeg_hfplus` は 111 checks / 0 failures。リポジトリチェック 4 種も通過。

ビルド識別の宣言マイルストーンが WSR-E 時代の `M143-dev` のままだった
ため、`cmake/release_identity.cmake` を `M149-dev` に更新した（ポリシー
どおり、ゲート未受入れの間は -dev を保持）。

次の一手（実機側）: テスタの電池交換 → CN2 ↔ D-sub 出力の抵抗マップ →
CN2 ケーブル → 実機での表示確立。表示の有効化条件が実機で判明した場合、
`vram/scrndrawva.c` の駆動条件を実測の意味論へ置き換える。

## 2026-11-14: main への一時 merge（保守者の指示）

保守者の指示により、この時点での成果を main に `--no-ff` で merge して
作業を一時停止する。G149 は未受入れ、リリースは承認されていない。

main に入るもの:
- M145〜M149 の解析文書（暫定仕様、逆 ASM 記録、基板 decode 調査、試験記録）
- HFTEST（PC-9801 C89/DOS 試験ツール、init/banktest は実行不可のまま）
- VA 用プローブ HFV0〜HFV3 と試験ディスク build.sh（D88 は Git の外）
- vaeg のボードモデル `io/hfplus.c`（実機測定済みの意味論のみ）、暫定
  映像出力（`vram/scrndrawva.c`）、設定 `HyperFramePlus`（既定 OFF）、
  GUI メニュー、状態保存セクション、CTest `vaeg_hfplus`（111 checks）
- ビルド識別の宣言マイルストーン `M149-dev`

開いている項目（再開時の最初の一手）:
1. テスタの電池交換 → CN2 ↔ D-sub 15 出力の抵抗マップ（パススルー状態
   で測定）→ CN2 結線表 → CN2 入力ケーブル → 実機での表示確立
2. 実機で表示の有効化条件が判明したら、vaeg の駆動条件を実測の意味論へ
   置き換える
3. 映像出力の検証（HF+ モニタでのカラーバー確認）

## 2026-11-14: 動作確認での修正 3 点（保守者の指摘）

vaeg で probe ディスクを動かした保守者の指摘:

1. GUI メニュー「HyPER-FRAME+ (要リセット)」のリセット要件を撤廃。
   ポートの attach を常時に行い、各ハンドラが `np2cfg.hfplus_enable`
   を見る形に変更（無効なら書き込み無視・読み出し FF = スロットが空の
   場合と同じ）。トグルは即時反映。
2. HFV1 が終了後も元の画面に戻らない → HFV1 は読み戻しの後に停止手順
   （D0=00、EE=00）を書いて終わるように変更（OFF 行をログに出す）。
3. HFV2/HFV3 が終了時に駆動状態のままだったため、vaeg では描画した
   上 1/4 のパターンが残って見えた（実機では撮影用の意図だったが、
   実機表示は確立していないので意味がない）→ 両方とも終了時に停止手順
   を書くように変更。

`vram/scrndrawva.c` の駆動条件（駆動状態 = HF+ 表示、非駆動 = 本体画面
スルー）は変更していない。smoke.py の模擬で HFV1/HFV2/HFV3 がすべて
終了コード 0 で停止手順まで到達することを確認。CTest `vaeg_hfplus`
111 checks / 0 failures。probe ディスクを作り直し。

### 暫定表示の表示モード（保守者の指摘、2026-11-14 続き）

駆動中は画面全体が HF+ フレームバッファに置き換わり、probe の CSV ログ
（テキスト画面）が読めないという指摘を受けた。表示モードを 3 段階に
した（`hfplus_view`、既定 PiP）:

- **PiP（既定）**: 本体画面はそのまま表示し、HF+ 出力を右上に 1/2 サイズ
  で重ねる。ログを読ながらボード出力を確認できる
- **全画面**: 従来どおりボード出力で画面全体を置き換える
- **本体のみ**: ボード出力を表示しない（実機の「信号なし」に相当）

GUI の「デバイス」メニューの「HyPER-FRAME+...」サブメニューで切り替える
（「有効」のチェックボックス + 表示モードの選択。無効のときは表示モード
がグレーアウトする）。当初は 2 つの別メニュー項目だったが、保守者の指摘
でサブメニューに統合し、さらに表示モードの名称を保守者の指定に変更した:

- 「右上に出力」= PiP（本体画面はそのまま、ボード出力を右上 1/2 サイズ）
- 「HyperFrame+ ON」= 全画面（ボード出力で画面全体を置き換え）
- 「HyperFrame+ OFF」= 本体のみ（ボード出力を隠す。実機の「信号なし」相当）

内部のモード ID（HFPLUS_VIEW_PIP/FULL/OFF）は変わっていない。DOS への復帰は、HFV1/HFV2/HFV3 が停止手順を
書くのでプログラム終了時に自動。PiP は vaeg 固有の便宜的表示であり、
実機の挙動（モニタ 1 台が本体/ボードのどちらかを表示）の再現ではない。
