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
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.
-->

# I/O解析資料 — ポート・機能別索引

PC-88VAシリーズのI/Oポートを、機能ブロック別に説明する。
公開資料は `docs/io/io_*.md`。個別ページに「I/O・名称・対象・チップ・機能・
ビット説明・解説・関連」をまとめる。現在の公開詳細資料は**TSPとSGPの2件**。
全ポートの機能や全機種の動作が解明済みという報告ではない。

[BIOS解析資料](../bios/)は割り込み経由のサービスを扱い、
本書は直接I/Oと、それに続くハードウェアコマンドを扱う。
BIOSの管理RAM、CPUのメモリバンク、装置固有の物理アドレスを混同しない。

## 機能別資料

| 機能・資料 | 主なI/Oアドレス | 検証状態 |
| --- | --- | --- |
| [TSP / テキスト・スプライト・走査制御](io_tsp.md) | `0142H`、`0146H`。関連：`0143H`、`0148H` | 資料・現行ソースの静的整理。先行実機記録を参照。全ステータス／転送は未検証 |
| [SGP / グラフィックス描画・転送](io_sgp.md) | `0500H`、`0502H`、`0504H`、`0506H`。関連：`0508H`、`0580H` | 資料・現行ソースの静的整理。LINE方向ビットはメンテナ実機確認と資料／実装との差を明記 |

**SGPのLINEは、メンテナの実機確認では `HD=0800H`／`VD=0400H`。**
既存のマニュアル由来の整理・現行vaegの `HD=0400H`／`VD=0800H` と逆になる。
[方向ビットの比較表](io_sgp.md#lineの方向ビット--実機と資料実装の相違)を先に読む。
この差をBITBLT/PATBLTへ無条件に適用しない。

## I/Oアドレスブロック

アドレスはすべて16進数で、開始・終了アドレスを含む。
ブロック内のすべてのアドレスに装置があるわけではない。
「ユーザーエリア」も自由に書き込んで安全という意味ではなく、拡張ボードなどの
割当て・競合を確認する必要がある。

| I/Oアドレス範囲 | ブロック名 | 備考 |
| --- | --- | --- |
| `0000H–00FFH` | システムエリア0 | PC-88MH/FH互換 |
| `0100H–01FFH` | システムエリア1 | TSPなどのシステム制御 |
| `0200H–02FFH` | フレームバッファー制御エリア | GVRAM表示用の記述子など |
| `0300H–04FFH` | カラーパレット制御エリア | 表示色の制御 |
| `0500H–05FFH` | GVRAM制御エリア | SGPとCPUのGVRAMアクセス制御など |
| `0600H–0FFFH` | システムエリア2 | リザーブ（予約領域） |
| `1000H–FEFFH` | ユーザーエリア | 拡張機器等の割当てに注意 |
| `FF00H–FFFFH` | システムエリア3 | CPU内部使用 |

## ポートを読むときの主要確認点

| 確認事項 | TSPの例 | SGPの例 |
| --- | --- | --- |
| アクセス幅 | コマンド・パラメータはBYTE | コマンドリスト先頭はWORD×2、起動はBYTE |
| ReadとWriteの意味 | `0142H` はRead=状態、Write=コマンド | `0506H` はRead=BUSY、Write=起動要求 |
| コマンドの送り先 | `0142H` にopcode、`0146H` にパラメータ | 主RAMのリストにopcodeとパラメータを置き、SGPがfetch |
| 待ち条件 | 新規コマンドは05H、パラメータは01Hのmaskで待つ | コマンドBUSYと `0580H.RBUSY` は別 |
| アドレスの単位 | テーブルのページ番号、CPUバイト、TSPワードを区別 | 22ビットSGP物理アドレス、FBWはバイト、CLSはワード数 |
| 表示と描画の区別 | SYNCは走査、TSPはテキスト／スプライト | SGPはメモリへ描画。表示開始・走査設定は別 |
| 機種差 | 初代VAの64 KiBとVA2/VA3の256 KiB TVRAM | 初代VAとVA2系のdescriptor幅・pitch解釈を区別 |
| 未実装・未確定 | `0143H` の機能、結果読出し、SCなど | LINE方向の不一致、Kanji source、実測サイクルなど |

`OUT DX,AX` と `OUT DX,AL` は交換可能とは限らない。
BYTEポートの隣へ書くことを、同じ装置へ2バイトを続けて送ることと考えない。
Read値が `FFH` になるスタブや、未接続時のエミュレータ値から、実機の予約bitを
決めてはいけない。

## 根拠と限界

1. **資料上の仕様**：PC-88VAの技術資料と、必要に応じて汎用LSIの仕様を区別する。
2. **現行実装**：`io/` の登録I/O経路とコマンド処理を読む。資料どおりかどうかは別。
3. **実機確認**：メンテナの確認と既存の公開測定記録を明示する。新規に測ったと装わない。
4. **未確定事項**：モデル差、予約bit、異常入力、転送終了、タイミングを推測で埋めない。

このPRはドキュメントのみ。LINEの方向ビットの注意書きを追加しても、
エミュレータの定数や描画処理が修正されるわけではない。
完全な実機一致、全BIOSとの整合、サイクル精度は追加検証が必要。

## 安全性と後続確認

- 待ち処理にtimeoutを設け、未実装の結果や停止しない装置を無期限に待たない。
- 未知のポート・opcode・予約bitを総当たりしない。
- 起動中のコマンドリスト／ワークを変更・再利用しない。
- 描画先の範囲、バンク、パターン、表示管理状態の保存・復元を確認する。
- CRT同期変更は既知のモニタ条件と停止／再開手順を守る。未知のSYNCを試さない。
- 私有ROM・media・manual・raw disassembly・raw captureはGitへ入れない。

## 関連資料

- [BIOS解析資料 — 割り込み別索引](../bios/)
- [PC-88VAの表示モード・フレームバッファ制御](../modernization/pc88va-video-modes.md)
- [TSPの再構成仕様](../modernization/upd72022-tsp.md)
- [SGPの再構成仕様](../modernization/upd92017-sgp.md)
- [恒久的な不具合修正記録](../modernization/bug-fixes.md)
