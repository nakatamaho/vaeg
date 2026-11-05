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

# HyPER-FRAME+ 暫定仕様 — M145

**対象は Digital Arts HyPER-FRAME+ と PC-9801 系 DOS。
メーカー仕様書の代替ではなく、実機適合も未確認。**
HyPER-FRAME、HyPER-FRAME III、Super Frame、WSR-E は別対象である。
PC-88VA/V30 は後段の実現可能性調査であり、このツールを VA に挿して
実行することは指示しない。G145 は NOT_READY。

## 根拠の区分

| 表記 | 意味 |
|---|---|
| `[PUBLISHED]` | 今回確認した刊行物にある記述。対象実機での測定ではない |
| `[BINARY]` | 原本の同一性、実際の命令バイト、分岐・呼出し文脈を確認した静的所見 |
| `[INFERRED]` | 上記から導いた計算・実装上の推論。ハードウェア仕様への昇格は禁止 |
| `[UNKNOWN]` | 未確認。依頼文のランドマークも原本照合前はここに置く |
| `[HARDWARE-TESTED]` | 機体、カード、条件、実行物、観測結果を結び付けた実機確認 |

今回、ボード仕様について独立確認済みの `[BINARY]` および
`[HARDWARE-TESTED]` 項目は **ない**。自作 EXE の生成命令検査や
mock/命令エミュレーションは、その代用ではない。

## 公開資料 P1

寿 国梁・斎尾 乾二郎「パソコン画像処理システムの開発とその応用
（第1報）システムの構成と基本的制御」、農業情報研究 1(1), 61–76, 1992。
[DOI: 10.3173/air.1.61](https://doi.org/10.3173/air.1.61)、
[論文ページ](https://www.jstage.jst.go.jp/article/air/1/1/1_61/_article/-char/en)、
[PDF](https://www.jstage.jst.go.jp/article/air/1/1/1_61/_pdf/-char/ja)。

PDF と抽出テキストを取得し、印刷 p.72 / PDF 12 ページの List 1、
p.73 / PDF 13 ページの List 2 はページ画像でも確認した。
p.65 に I/O 方式とバンク方式の説明がある。
論文が引用する Digital Arts の 1990 年・68 ページのユーザーズマニュアルは
**未入手**。論文の引用を、マニュアルを直接読んだ証拠にしない。
資料・ページ画像・抽出テキスト自体は Git に収録しない。

## 通常画素の I/O

`[PUBLISHED]` P1 List 1 は、**一つの色成分ごと**に次のアクセスを行う。
全ポート番号は 16-bit I/O アドレスの十六進数である。

| 順序 | ポート | 幅 | 内容 |
|---|---:|---|---|
| 1 | 00D7 | byte OUT | Y の上位バイト |
| 2 | 00D6 | byte OUT | Y の下位バイト |
| 3 | 00D5 | byte OUT | X の上位バイト |
| 4 | 00D4 | byte OUT | X の下位バイト |
| 5 | 00D8 / 00DA / 00DC | byte IN または OUT | R / G / B の一成分 |

- `[PUBLISHED]` 描画座標は `0 <= x < 640`, `0 <= y < 400`、各成分は 8 bit。
- `[INFERRED]` 実装は RGB の各成分の前で座標を指定し直す。
  List 1 は「座標を一回指定して RGB を連続転送してよい」という証明ではない。
  データアクセス後に座標が保持されるか、自動更新されるかを仮定しない。
- `[UNKNOWN]` 電源投入直後のモード、読出し時の詳細な副作用、待ち時間、
  SW1 の意味、カード改版差、割込みとの関係は未確定。
  `PIXEL-READY` 確認は利用者による前提の申告であり、プログラムによる検出ではない。
- `[UNKNOWN]` `IN` 一回や FF/00 の読出しだけでは装着・故障を判定できない。
  pixel read にも座標 OUT が必要なので「無書込みの検出」ではない。
- `[UNKNOWN]` word OUT と byte OUT の実機上の同値性は未確認。
  通常経路は上記の byte 順序。`readback word` は追加確認付きの別実験である。
  生成コードの `EF` (`OUT DX,AX`) は確認対象だが、バス上の分割順序の証明ではない。

## バンクとセグメント計算

`[PUBLISHED]` P1 p.65 / List 2 は 128 KiB の窓と六バンクを使用する。
セレクタは 00EC、値は次のとおり。

| 成分 | 前半 | 後半 |
|---|---:|---:|
| R | C0 | C1 |
| G | C2 | C3 |
| B | C4 | C5 |

`[PUBLISHED]` List 2 の転送は segment:offset と `movedata` を用いる。
最初のバンクの行基準 segment は `8000h + 28h * y`。
後半の同一バンク内経路は `8000h + 28h * (y - 205) + 8h` として、
x を offset に加える。**28h は segment の増分であり、1 行 640 bytes に相当する。**

`[INFERRED]` これを物理アドレスに直すと、色面内の線形位置 L と窓内位置 O は
次のように整理できる。コードの逐語転載ではなく、独立した算術表現である。

```text
L = 640 * y + x
half = L / 131072
O = L % 131072
physical = 0x80000 + O
normalized_segment = 0x8000 + O / 16
normalized_offset = O % 16
```

`204 * 640 + 512 = 131072` なので、境界は **y=204 の x=512**。
「y < 205 は全部前半」という実装は誤る。

| 座標・範囲 | 必要な計算 |
|---|---|
| (511,204) | 前半、窓内 131071 / 1FFFFh |
| (512,204) | 後半、窓内 0、8000:0000 |
| (0,204) から 640 画素 | 前半 512 bytes と後半 128 bytes に分割 |
| (0,205) | 後半、窓内 128、8008:0000 |
| (639,399) | 後半、窓内 124927 / 1E7FFh、9E7F:000F |

List 2 の y=204 後半を元の式で表すと segment は 7FE0h になるが、
x>=512 を加えた物理アドレスは 80000h 以上になる。
segment 単独を窓の物理開始位置と取り違えない。
新しい `hf_plan_line` は unsigned long の線形位置で計算し、上記の正規化形を返す。
転送バッファの offset は「要求区間の先頭=0」。元の List 2 の 640-byte 行全体に
対する x 添字とは異なる。最大二つの span が必要で、容量不足はエラーにする。

`[PUBLISHED]` List 2 の読み書き方向は、書込み時が DS のバッファから窓、
読出し時が窓から DS のバッファ。境界を跨ぐ場合は、前半を `512-x0` bytes、
後半を `x1-511` bytes 転送する（x1 は両端含み）。
`[INFERRED]` 元コードの near pointer/DS 前提、unsigned 座標に対する負値検査、
端点・行・バッファ長の扱いを、そのまま別メモリモデルへ移植してはいけない。
本ツールは式だけを検証し、`movedata`、`MK_FP`、窓への書込みを実装しない。

- 有効画像: `640 * 400 * 3 = 768000` decimal bytes。
- 六バンクの容量: `6 * 131072 = 786432` bytes = 768 KiB。
- 差の 18432 bytes を、自由な作業領域だと解釈してはいけない。
  特殊座標の意味は未解明である。

**`[UNKNOWN]` 実機の 80000h–9FFFFh の所有権は未確認。**
通常 RAM、DOS、常駐ソフト、EMS/UMB 等を置換・破壊するおそれがある。
DOS が空き RAM と報告することも、ボード窓の安全な予約を意味しない。
そのため `banktest` は無条件 BLOCKED。DOS backend 自体も EC 出力を拒否する。
六値の出力試験は host mock に限る。

## 初期化・停止・特殊座標

`[UNKNOWN]` 依頼に示された JPGVHF の候補列は EE=C0、D0=A5、D3=00、
D0=A1/E1/E5 等を含む。しかし原本未入手なので、条件・呼出し元・モード依存を
再確認できていない。ビット名や汎用初期化列を創作しない。`init` は実行不可。

`[UNKNOWN]` `off` の候補は **D0=00 → EE=00**。
これは依頼に記載された binary 所見を明示的に実験可能にしたものにとどまる。
ローカルの原本照合が未完了であることを CLI にも表示する。
以前の表示経路・モードを保存する操作ではなく、停止成功も readback できていない。

`[UNKNOWN]` Y=0191h/0193h、X=0086h/0087h、DA の使用は特殊レジスタ領域の
候補であって、通常画素やスクロール機能との対応は未確定。
通常 API は y=400 以上を拒否し、この領域にはアクセスしない。
詳細な未検証ランドマークと追跡手順は [解析記録](reverse_engineering.md) に分離する。

## 保存・復元の限界

`readback` は (0,0), (1,0) の **両方**を保存してから、異なる RGB を書き、
両方を読み返し、両方の復元を試み、復元後の値も照合する。
比較不一致・途中 I/O エラーでも、保存が完了していれば cleanup に進む。
第一画素の復元が失敗しても第二画素を試みる。

保存途中のエラーでは画素データを書かず終了する。ただし座標 OUT は既に行っている。
復元中に座標指定が失敗した成分にはデータを書かず、次の成分を再選択して続ける。
FF は画素なら正しい白成分でもあり、未知の制御レジスタ値 FF の保存・復元とは別問題。
既知の pixel mode という前提なしにこの処理を実行してはいけない。

照合成功でも、座標レジスタ、表示経路、モード、全画像、割込み状態まで
元に戻したことにはならない。pattern は領域を上書きしたまま残す。
自動初期化・自動 off・自動バンク切替は一切行わない。
