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

# SGP — グラフィックス描画・転送プロセッサ

[I/O解析資料の索引へ](README.md)

| 項目 | 内容 |
| --- | --- |
| I/O | `0500H`、`0502H`、`0504H`、`0506H`。関連：`0508H`、`0580H` |
| 名称 | SGP（Super Graphic Processor）のコマンドリスト・制御・状態 |
| 対象 | PC-88VAシリーズ。初代VAとVA2系の機種差は個別に記す |
| チップ | 初代VA：μPD92017（D92017-002、回路図IC75/VDP）。VA2：D92046GD-001 |
| ブロック | GVRAM制御エリア（`0500H–05FFH`） |
| アクセス幅 | コマンドリスト先頭はWORD×2、制御・起動・状態はBYTE |

> [!IMPORTANT]
> **LINEの方向ビットは、メンテナの実機確認では `HD=0800H`／`VD=0400H`。**
> 既存のマニュアル由来の整理とM106a修正前のvaegは逆の `HD=0400H`／`VD=0800H`。
> M106aでエミュレータとデモを実機確認のマスクへ修正した。
> [修正と検証記録](../agents/tasks/M106a_sgp_line_demofix.md)を参照。資料初版は文書のみの変更だった。

## 根拠と読み方

- **【資料】**：既存の[SGP再構成仕様](../modernization/upd92017-sgp.md)に整理された
  PC-88VA用の技術資料。今回すべての原本を再照合したわけではない。
- **【実機・メンテナ確認】**：今回の依頼で、LINEのHD/VDマスクについて実機での
  値として指定された情報。今回の執筆作業で新規に測定したものではない。
  試験機種別のraw command・座標・波形をこのPRで公開してはいない。
- **【実装】**：`io/sgp.c`、`io/sgp.h`、`io/gactrlva.c` を読んで確認した動作。
  エミュレータの挙動は、実機の仕様を証明するものではない。
- **【未確定】**：機種差、異常入力、予約bit、タイミングなどに不足が残る事項。

本書は独自に書いたポート中心の解説であり、メーカーの原本や電気的仕様書の代替
ではない。古いM97資料は当時の「資料どおり」の方向定義を記録している。
今回のメンテナ実機確認と同じ結論だった、と読み替えない。

## 機能とTSPとの分担

SGPは、メモリ上の16ビットコマンドリストを実行する描画プロセッサ。
矩形転送、繰返しパターン、16種のBoolean ROP、LINE、連続領域のfill、
左右の色探索を行う。パックド1/4/8/16bppの画素形式を扱う。

[TSP](io_tsp.md)のようにI/Oへopcodeとパラメータを逐次送る装置ではない。
主RAMにリストとワークを用意し、先頭アドレスをI/Oで指定して開始する。
TSPはテキスト／スプライト・走査、SGPは描画、VA側回路はGVRAM表示と合成を担当する。

SGPを `D65200` と呼ばない。初代VAのD65200GD-054は別のGAL-3で、
GVRAMの表示転送・シーケンス・調停を担当する。

## I/O機能一覧

| I/Oアドレス | 幅 | R/W | 内容 | 注意 |
| --- | --- | --- | --- | --- |
| `0500H` | WORD | W | コマンドリスト先頭アドレス下位16ビット | 主RAM上のSGP物理アドレスを指定 |
| `0502H` | WORD | W | 同アドレス上位 | アドレス空間は22ビット。未使用上位bitは0にする |
| `0504H` | BYTE | R/W | 割込み許可・abort要求 | bit 2とbit 1 |
| `0506H` | BYTE | R | コマンド実行状態 | bit 0=BUSY |
| `0506H` | BYTE | W | 実行開始要求 | idle時にbit 0を1へ |
| `0508H` | BYTE | R | 実機での意味は未確定 | vaegはsingle-plane有効時に1を返す |
| `0580H` | BYTE | R/W | GVRAMアクセス制御／RBUSY | コマンドBUSYとは別。GVRAMアクセス回路側のポート |

【資料】`0500H/0502H` はWORDのI/Oレジスタ。
【実装】vaegは `0500H–0503H` の各BYTEハンドラで値を組み立てるが、これだけで
実機でも任意のBYTE書込みがWORDと同等に保証されるとはいえない。
プログラム先頭アドレスのbit 0は0、リストのwordはlittle-endian。

【実装】SGP実行と有効な状態読出しは `gactrlva.gmsp` が有効なsingle-plane経路。
無効時には代替の読出しパターンになる。`0508H=1` を一般的なready bitとして
無条件にpollする根拠はない。

## 0504H, Read/Write：割込み・abort制御

| bit | マスク | 名称 | 内容 |
| ---: | --- | --- | --- |
| 2 | `04H` | INTF | 完了割込み許可 |
| 1 | `02H` | ABORT | 実行中断要求 |
| その他 | — | — | 予約・未確定。通常は0 |

【実装】書込み値は `06H` でmaskして保持する。ABORTが立つとBUSYをクリアし、
INTFが有効なら割込み要求を立てる。INTFを0にすると、保持していた割込み要求を
解除する。ENDも同じ完了割込みヘルパを使い、IRQ 8を要求する。

【未確定】実機でのabort／割込みの厳密な順序、途中のread-modify-write word、
完了要求のラッチ寿命と解除手順。現行ソースの動作だけを一般化しない。

## 0506H, Read/Write：状態／実行開始

| 操作 | bit 0 | 内容 |
| --- | ---: | --- |
| Read | 0 | コマンド処理停止中 |
| Read | 1 | コマンド処理中（BUSY） |
| Write | 1 | 実行開始要求 |

【実装】idleから1を書くと、プログラムカウンタへ初期アドレスを取り込み、
command fetch状態へ移る。書込み値はbit 0のみ保持する。
実行中に1を書いてもPCの再設定はしない。0を書けば実装上はBUSYがクリアするが、
それを実機で保証されたabortの代わりとして使わない。

ENDや制御ポートのABORTで完了・中断する。実行中のコマンドリストを差し替えたり、
busy中の新規起動を正常な再起動手順と考えたりしない。

## 0580H：GVRAMアクセスとRBUSY

【資料】Read側bit 7がRBUSY。これはレジスタ／バスのreadinessに関係し、
`0506H` のコマンドBUSYとは別の条件である。
CPUのGVRAMデータ書込みモードは `10H`。既存のBIOS経路や描画ツールは
SGP起動直前にこれを再指定している。

【実装】`gactrlva_o580()` は `dat & 18H` を書込みモードとして保持する。
有効時のReadはこの値を返し、動的なRBUSYを生成しない。
コードにはVA2での読出しはbit 7が0という観察コメントがある。
予約bitやすべての書込みモードの実機仕様が解明済みとはしない。

## 実行開始手順

1. single-planeのGVRAMアクセス／表示設定を整える。
2. 主RAMに完成したコマンドリストを置き、58バイトの書込み可能なワークを確保する。
3. source／destination descriptorとSET_COLORをリスト内で明示する。
4. `0506H.BUSY=0` を有限時間で待つ。必要な読出しはRBUSY条件も守る。
5. `0500H` に先頭下位word、`0502H` に上位wordを送る。
6. `0504H` の割込み許可を設定し、CPUのGVRAM書込み経路に必要な `0580H=10H` を再指定する。
7. `0506H` へBYTEで `01H` を書く。
8. BUSY解除または完了割込みを待つ。実行完了までリスト・ワーク・参照元を再利用しない。

これは依存関係の整理。BIOSやPC-Engineの管理状態を保存・復元する完成した
ドライバではない。すべての待ちにtimeoutを設け、描画先範囲を事前に検査する。

## SGPのメモリアドレス

【資料】SGPは22ビット、`000000H–3FFFFFH` の4 MiB空間を使う。
CPUの論理バンク窓ではなく**SGP物理アドレス**をdescriptorへ入れる。

| SGPアドレス | 初代VAの資料上の用途 | 現在のvaegでの注意 |
| --- | --- | --- |
| `000000H–09FFFFH` | 主RAM | `080000H–09FFFFH` は選択されたBMSを参照する経路あり |
| `0A0000H–0FFFFFH` | 拡張メモリ | 未接続ハンドラの領域あり |
| `100000H–13FFFFH` | Kanji ROM 1 | 読出しハンドラはTODO、現在は0 |
| `140000H–17FFFFH` | Kanji ROM 2／機種別overlay | 読出しはTODO、書込みも未実装 |
| `180000H–18FFFFH` | 初代VAの64 KiB TVRAM | vaegは `180000H–1BFFFFH` を256 KiBのtextmemへ接続 |
| `190000H–1FFFFFH` | 初代VAの予約領域 | 初代の資料と後継機の広い実装範囲を区別 |
| `200000H–23FFFFH` | 256 KiB GVRAM | 主な描画先 |
| `240000H–3FFFFFH` | 予約領域 | 実機動作未確定 |

SGPのTVRAM開始 `180000H` を、CPUのTVRAM窓 `A0000H` と混同しない。
また、画面表示用のFB記述子ポートとSGPのメモリ上のblock descriptorは別。

【実装】メモリアクセスは22ビットへmaskする。
未接続読出しの `FFFFH`、書込み無視、4 MiB端でのwrapは実装ポリシーであり、
実機の全予約領域を確認した結果ではない。ROMは原則としてsourceであって、
書込み可能なdestinationではない。

## Block descriptor — 12バイト

【資料】【実装】6wordを次の順に置く。

| バイトoffset | 幅 | フィールド | 内容 |
| --- | --- | --- | --- |
| `00H` | WORD | MODE / DOT | bit 1:0=画素形式、bit 7:4=開始dot |
| `02H` | WORD | WIDTH | 横の画素数。LINEでは横方向のextent |
| `04H` | WORD | HEIGHT | 縦の画素数。LINEでは縦方向のextent |
| `06H` | WORD | FBW | 1走査行進むバイト数。矩形の横幅そのものではない |
| `08H` | WORD | ADDRESS下位 | 偶数のSGP物理アドレス下位 |
| `0AH` | WORD | ADDRESS上位 | SGPアドレス上位。未使用bitは0 |

資料上、FBWの下位2bitは0（4バイト境界）。幅や高さをword数に丸めない。
開始dotは、最初のword内の画素位置を表す。

| MODE | bpp | 1wordの画素数 | 開始dotの範囲 |
| ---: | ---: | ---: | --- |
| 0 | 1 | 16 | 0–15 |
| 1 | 4 | 4 | 0–3 |
| 2 | 8 | 2 | 0–1 |
| 3 | 16 | 1 | 0 |

【実装】初代VAではWIDTH/HEIGHTを12ビットにmask、FBWを `FFFCH` でmaskする。
VA2モデルではWIDTHが14ビット、HEIGHTが16ビット、FBWは `FFFEH` を保持する。
FBWは符号付き16ビットとして扱い、VA2の負pitchには既存ソフト用の補正もある。
この後継機profileを、全VA／VA3の実機で確認済みとはしない。

## コマンド一覧

リストはlittle-endianの16ビットopcodeとパラメータからなる。
以下の「後続word数」はopcode自身を除く。全長は `2*(1+後続word数)` バイト。

| opcode | 名称 | 後続word数 | 機能 |
| --- | --- | ---: | --- |
| `0001H` | END | 0 | 実行終了、許可時に完了割込み |
| `0002H` | NOP | 0 | 何もしない |
| `0003H` | SET_WORK | 2 | 58バイトワークの開始アドレス |
| `0004H` | SET_SOURCE | 6 | source block descriptor |
| `0005H` | SET_DESTINATION | 6 | destination block descriptor |
| `0006H` | SET_COLOR | 1 | 16ビット色word |
| `0007H` | BITBLT | 1 | mode word付き矩形転送 |
| `0008H` | PATBLT | 1 | mode word付きパターン繰返し転送 |
| `0009H` | LINE | 7 | mode word＋専用destination descriptor（6word） |
| `000AH` | CLS | 4 | 開始アドレス2word＋word count 2word |
| `000BH` | SCAN_RIGHT | 0 | SET_DESTINATIONから右向きに色探索 |
| `000CH` | SCAN_LEFT | 0 | SET_DESTINATIONから左向きに色探索 |

資料の「13コマンド」に対し、識別されたopcodeは12種類。
不明な13番目を推測で追加しない。
【実装】`0001H–000CH` が既知範囲で、不明opcodeはtraceに記録する。
現行ソースは不明opcodeだけで必ず実行を停止するわけではないので、ENDやtimeoutを
省略して安全になる、と考えない。

### SET_WORK / SET_COLOR

SET_WORKは偶数のワーク開始アドレスを下位word、上位wordの順に置く。
58バイトを安定した書込み可能RAMとして予約する。
【実装】アドレスを保持するが、ワーク内部の全fieldが解析済みという意味ではない。

SET_COLORは16ビット値。1/4/8bppでは対応する画素単位の値をword全体へ揃える。
例えば4bppの色Aなら `AAAAH`、8bppの色12なら `1212H`。
16bppは全wordを1画素として使う。現在のソースがwordをそのまま保持することと、
単一nibbleを自動複製することは同じではない。

## BITBLT / PATBLTのmode word

ここは資料と現行ソースのBITBLT/PATBLT用定義。
**LINEの実機方向マスクとは別表**である。

| bit | マスク | 名称 | 内容 |
| --- | --- | --- | --- |
| 12 | `1000H` | SF | 0=sourceをdestinationのdotへ合わせてshift、1=そのshiftなし |
| 11 | `0800H` | VD | 縦方向。通常0=正方向、1=逆方向 |
| 10 | `0400H` | HD | 横方向。通常0=正方向、1=逆方向 |
| 9:8 | `0300H` | TP | 透明処理 |
| 3:0 | `000FH` | OP | Boolean ROP番号 |
| その他 | — | — | 予約bitは0 |

資料で定義されるTPは0–2：0=通常、1=sourceの0画素を透過、
2=destinationが0の画素にだけ転送。3は未定義であり、「第4の仕様」としない。
1bpp sourceから多bit destinationへの展開はSET_COLORを使う。
資料上、異形式転送はこの展開に限られ、展開時はHD=0、HD=1の転送はTP=0。
現在の妥当性チェックは異常組合せをtraceするが、全件を拒否して停止するわけではない。

BITBLTはsource矩形をdestinationへ転送する。重なる領域は未読sourceを壊さない方向を
選ぶ。現在の実装ではSET_DESTINATIONの幅・高さをsourceの幅・高さへ置き換える。
PATBLTはsourceを2次元タイルとしてdestinationの幅・高さまで繰り返す。
単純な1回の横コピー、または垂直だけの繰返しではない。

### Boolean ROP

S=source、D=元destination。結果は対象bppにmaskし、矩形端の非対象画素を保存する。

| OP | 式 | 主な意味 |
| --- | --- | --- |
| `0H` | `0` | 0へ |
| `1H` | `S & D` | AND |
| `2H` | `~S & D` | |
| `3H` | `D` | destination保持 |
| `4H` | `S & ~D` | |
| `5H` | `S` | source copy |
| `6H` | `S ^ D` | XOR |
| `7H` | `S OR D` | OR |
| `8H` | `~(S OR D)` | NOR |
| `9H` | `~(S ^ D)` | XNOR |
| `AH` | `~S` | source反転 |
| `BH` | `~S OR D` | |
| `CH` | `~D` | destination反転 |
| `DH` | `S OR ~D` | |
| `EH` | `~(S & D)` | NAND |
| `FH` | 全bitが1 | 対象画素を全1へ |

OPの番号を他機種のROP表から流用しない。画素順、word境界、逆方向、透明処理との
組合せはそれぞれ照合が必要。

## LINE（0009H）

LINEはopcodeに続けてmode wordと12バイトのdescriptorを持つ。
直前のSET_DESTINATIONだけを使ってパラメータを省略してはいけない。
色はSET_COLORから取る。

【実装】WIDTH/HEIGHTの大小で主軸を選び、extent−1を傾きの分子・分母へ入れる。
通常の非0extentでは、指定した開始画素から端点を含む長さを描く。
この処理は整数accumulator式だが、hostの汎用Bresenhamへ置き換えて
画素位置が同じと保証しない。X主軸には7×6の実機合わせというコメントがあり、
初期誤差の丸めを特別扱いしている。

幅・高さが0のLINEは使わない。既存ソースに実機での大きなメモリ破壊を疑う
コメントがあり、0を安全なNOPと定義できない。

### LINEの方向ビット — 実機と資料・実装の相違

**【実機・メンテナ確認】LINEの横方向HDはbit 11（0800H）、
縦方向VDはbit 10（0400H）。マニュアルなどの記述と逆。**

| 根拠・対象 | LINE HD（横） | LINE VD（縦） | 扱い |
| --- | --- | --- | --- |
| 今回のメンテナ実機確認 | `0800H` | `0400H` | 実機用LINEの方向選択はこれを基準にする |
| 既存のマニュアル由来の整理 | `0400H` | `0800H` | 記述との差を残す。実機結果の代わりにしない |
| M106a修正前の `io/sgp.h` のLINE aliases | `0400H` | `0800H` | BLTMODE_HD/VDと共通の定義。実機確認と不一致 |
| M106a以後の `io/sgp.h` のLINE aliases | `0800H` | `0400H` | 実機確認と一致。BLTの方向maskは変更しない |
| BITBLT/PATBLTの資料・現行実装 | `0400H` | `0800H` | LINEの確認結果だけでこちらも逆と断定しない |

【実装】M106a以後は `SGP_BLTMODE_LINE_HD = SGP_BLTMODE_VD`、
`SGP_BLTMODE_LINE_VD = SGP_BLTMODE_HD`。修正前はそれぞれ同名の共通maskだった。
`exec_line_x()` / `exec_line_y()` はそのmaskで横／縦の符号を選ぶ。
M106aのraw mode回帰試験は4組の方向を検証する。端点・tie rule・機種別の
全挙動が実機一致したという主張ではない。

通常の方向指定で0を増加、1を減少として、OP=5（色のcopy）、TP=0の
mode wordを組み立てると次のようになる。

| 希望する向き | HD | VD | 実機用LINE mode | 現行vaegがそのraw modeで選ぶ向き |
| --- | ---: | ---: | --- | --- |
| 右・下 | 0 | 0 | `0005H` | 右・下 |
| 左・下 | 1 | 0 | `0805H` | 左・下（M106a修正前は右・上） |
| 右・上 | 0 | 1 | `0405H` | 右・上（M106a修正前は左・下） |
| 左・上 | 1 | 1 | `0C05H` | 左・上 |

つまり、両bitが0または両方1の例だけではswapを見分けられない。
横・縦が非対称な線で、HDだけ／VDだけを立てる例を別々に確認する。
例えば7×3と3×7を同じ安全な描画領域の中央から引くと、どちらの軸が反転したかを
区別しやすい。これは確認方法の提案であり、今回実行した試験結果ではない。

以前の[NEON4資料](../port/neon4_p3.md)には別のhardware／emulator方向profileの
説明が残るが、その値・当時のenum・対象時点を現在の実機確認と混ぜない。
現行header、資料の表、今回の実機確認をそれぞれ明示して記録する。

### LINEリストの構造例

以下はリストのword順を示す説明用の例。実機へ新しく投入して検証したバイナリ
ではない。address、work、dot、bpp、pitch、表示設定は呼出し側で整える。

```text
0003H                 ; SET_WORK
work_low, work_high
0006H                 ; SET_COLOR
AAAAH                 ; 4bppの色Aを各laneに設定
0009H                 ; LINE
0805H                 ; 実機用：HD=1、VD=0、TP=0、OP=5（左・下）
0001H                 ; MODE=1（4bpp）、DOT=0
0007H                 ; WIDTH=7
0003H                 ; HEIGHT=3
0140H                 ; FBW=320 bytes（640 dots × 4bpp / 8）
start_low, start_high ; 偶数のSGP物理アドレス、画面端を避けた開始位置
0001H                 ; END
```

`0805H` を現行vaegへそのまま送ると、上の比較表のとおり方向が実機用と異なる。
画像がエミュレータで一致したことだけを、実機ビット定義の検証としない。

## CLS（000AH）

後続は開始アドレス下位／上位、word count下位／上位の4word。
SET_COLORのwordを連続範囲へ書く。矩形のpitchを自動で飛ばすfillではない。

【実装】countは32ビットのword数。640×400、4bppの全バッファなら
128000バイト、64000word=`0000FA00H`。この場合countの2wordは `FA00H,0000H`。
実機の全境界・0count・abort・4 MiB越えを今回確認したわけではない。
矩形だけを消したい場合は行ごとのCLSや、1×1 patternのPATBLTなどを組み立てる。

## SCAN_RIGHT / SCAN_LEFT（000BH / 000CH）

SET_DESTINATIONの開始画素、形式、最大幅とSET_COLORを使い、指定色まで探索する。
追加パラメータはない。単独のPAINT／flood-fill opcodeではない。

【資料】【実装】開始画素が対象色なら結果の幅は0。
途中で一致したら、開始から境界までの画素数をdestination.widthへ残す。
最大幅内に一致がなければ幅は変わらない。
SCAN_LEFTは一致時にdestinationのaddress/dotも左側の開始位置へ更新する。
これらはSGP内部descriptorの更新であり、CPUへ別のI/O結果FIFOが出るという
説明ではない。CPUからの結果取出しABIや58バイトワークの全fieldは未確定。

現在のvaegには左右・最初の画素・後続一致・不一致・packed word境界のselftestが
あるが、それは実機での全SCAN条件の一致を証明するものではない。

## タイミング・競合・未確定事項

【実装】標準SGPクロックは初代VAで約3.9936 MHz、VA2系で約7.9872 MHz。
GUIの速度設定と、実機の命令ごとのサイクル数を混同しない。
コマンドsetupや画素／wordの費用は歴史的な近似値であり、
SGPの各メモリアクセスにCPU側4clockを引く処理も実測サイクル表ではない。

未確定事項は以下。

- 実機LINEの機種別raw記録・tie rule・端点・0extent。M106aの方向修正・回帰試験と、追加の実機検証を区別する。
- BITBLT/PATBLTの全方向・重なり・透明処理・異形式転送・境界mask。
- VA2/VA3のdescriptor拡張、負pitch、TVRAM／Kanji overlayのdecode。
- 58バイトワークの全構造、CPUからのSCAN結果取得、PC読戻し。
- busy中の再起動、active list変更、abortの部分書込み、完了割込みの厳密な順序。
- 不明な13番目のコマンド、未知opcode、予約領域、4 MiB境界、0幅／高さ。
- 描画と表示／CPUのバス競合、実機のサイクル費用。

このPRは独立した解析資料の追加・整合化のみ。新しい実機測定、ROM実行、
ゲスト描画試験、ソース修正はしていない。以前の機械テストやhuman gateを、
今回の実機方向マスクと同一の契約を確認したものとして再利用しない。

## 関連・参考文献

- [I/O解析資料の索引](README.md)、[TSPポート](io_tsp.md)
- [SGPの再構成仕様](../modernization/upd92017-sgp.md)
- [表示モード・フレームバッファ制御](../modernization/pc88va-video-modes.md)
- [M97の当時の実装報告](../agents/reports/m97_sgp_tekumani_commands.md)
- [当時のNEON4 LINE profile説明](../port/neon4_p3.md)
- 実装：[SGP命令・I/O](../../io/sgp.c)、[状態・mode定数](../../io/sgp.h)、
  [GVRAMアクセス制御](../../io/gactrlva.c)
- BNN, *PC-88VA Technical Manual*, 1987。
  [公開スキャン](https://archive.org/details/PC88VA)
