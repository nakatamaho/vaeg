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

# 解析記録 — 入力未入手部分を含む

## 結論と到達点

**公開論文の画素 I/O・バンク算術は確認した。
JPGVHF 原本の逆解析は入力不足で BLOCKED。**
本書を、提示されたランドマークを再発見した報告として扱わない。
独立確認済みのボード関連 `[BINARY]` 所見はまだない。
コード生成・mock 成功は、原本解析や実機試験の代わりにはならない。

今回は、安全な既知部分まで停止させず、公開資料に基づく C89 の画素処理、
バックアップ/照合/復元、明示的停止候補、bank 算術、mock を実装した。
未確定の初期化・窓アクセスは実行不可とした。
以前の M145 の multi-board viewer **D1** と、今回求められた JPGVHF **J1** は
別資料であり、以前の D1 の解析結果を J1 の確認に流用しない。

## 入力台帳

| 入力 | 今回の状態 | 証拠としての扱い |
|---|---|---|
| P1 論文 PDF | 取得・本文確認、List 1/2 はページ画像でも確認 | `[PUBLISHED]` |
| `jpgvhf1e.lzh` | アクセス可能な作業領域で未検出 | 提示 hash は期待値のみ |
| `JPGVHF.EXE` | 未検出 | MZ/命令/制御フローは未照合 |
| `JPGVHF.DOC` | 未検出 | CP932、版、日付等は依頼記載の未照合情報 |
| `JPGVHF_UTF8.txt` | 未検出 | 原文との一致未確認 |
| `HF_IO_NOTES.txt` | 未検出 | 実験メモは、入手しても独立検証とはしない |
| `HFDEMO.C`, `HFTEST.C` | 未検出 | 旧実験コードを正解や完成実装として採用しない |
| 1990 年メーカー manual、68 pages | 未入手 | P1 の参考文献欄に掲載された事実だけ確認 |

検索した場所で見つからなかったという結果であり、一般に入手不能という意味ではない。
公開配布 URL の候補も調べたが、404 または HTML 応答であり、期待 hash に一致する
LHa 原本は取得できなかった。HTML を archive として採用・実行していない。

依頼で明示された期待 SHA-256（**検証済み値ではない**）:

```text
527d0f162c80f5f8a963ee6f3c6da89168a62eec9436345683ac6ba49f3b2476  jpgvhf1e.lzh
dc89dda016e6b687eef95593dc58cc225269a20fda3b96a1bd4227ae8f993324  JPGVHF.DOC
f2f845543be983a9534eedea0b22a4647dcf685dfd5f0372e13a16acd11647ab  JPGVHF.EXE
```

期待構造は LHa `-lh5-`、DOC 14049 bytes、EXE 50976 bytes、
DOC は 1994-05-08 / JPG Viewer 1.14、MZ `e_cparhdr=20h`。
これらの期待値と、実際に読んだ header を混同しない。

一方、取得できた P1 PDF は 1277220 bytes、SHA-256 は
`12f8769a0a3943923d0f9da08a1a606c29b9f6f465b64d664c146c83b9128eae`。
[取得元・ページ番号・仕様上の読み方](hyper_frame_plus_spec.md#公開資料-p1)を参照。
OCR は誤りを含むため、コードの O/0、数値、段組みをページ画像で確認した。
原資料全文や List 全体の転載ではなく、独立した説明と実装だけを Git に置く。

## J1 の未照合ランドマーク

全行 `[UNKNOWN]`。**以下は依頼中の報告を整理した作業待ち表**であって、
実ファイルを読んで確認した命令表ではない。
image offset は、期待 header が正しい場合に限り file offset − 0200h。
runtime の CS:IP ではなく、DOS の load segment 加算や relocation とは別である。

| image offset | 条件付き file offset | 依頼記載の候補 | 未解決の文脈 |
|---:|---:|---|---|
| 563F | 583F | `BA EE 00 B0 C0 EE`、EE=C0 | 初期化入口・呼出し条件 |
| 5645 | 5845 | D0=A5 | 前後の状態、分岐 |
| 564B | 584B | D3=00 | レジスタの意味、後続ループ |
| 56AA | 58AA | D0=A1 | どの条件で到達するか |
| 56CF | 58CF | D0=E1 | モード・データ依存 |
| 未指定 | 未指定 | D0=E5 | 正確な箇所、呼出し元、用途 |
| 581D | 5A1D | D0=00 → EE=00 | 関数境界、全 return 経路、exit との関係 |
| 4056 | 4256 | `E7 D6`、word OUT | AX の値を作る処理と caller |
| 40A5 付近 | 42A5 付近 | D4 への word OUT | 厳密な命令境界・幅・順序 |
| 54A7 付近 | 56A7 付近 | Y=0193、X=0086/0087、DA | 特殊レジスタ説と通常画素の区別 |
| 未指定 | 未指定 | Y=0191/0193 | 書込み値の由来、ループ、表示条件 |

D0 の各 bit に enable/clock/scroll 等の名前を付ける根拠はまだない。
一続きの I/O 列が存在しても、途中の分岐、遅延、前提設定、割込み、
サイズ依存値を除いて汎用 `init` にしてはいけない。

## 再現可能な入力検証

[`verify_reference.py`](../verify_reference.py) は原本を変更しない。
出力は新しい checkout 外ディレクトリに限定する。

```sh
python3 -B verify_reference.py \
  --references "$REFERENCE_DIR" --output "$NEW_EVIDENCE_DIR"
```

1. 実際に存在する archive/EXE/DOC の hash を期待値と比較。
2. archive がある場合は `bsdtar` で対象二ファイルだけを stdout に展開。
   両方の形で渡された EXE/DOC はバイト一致を要求。
3. MZ page 数、最終 page サイズ、header 範囲、entry、relocation table と
   全 relocation target の範囲を検査。固定 0200h を検査前に仮定しない。
4. 検証後の load image に対して、依頼中の**明示されたバイト列二点だけ**を照合。
   残りは周辺窓を私的記録へ出す。列の意味や関数境界は自動認定しない。
5. DOC があれば strict CP932 で別ファイルに変換。原本は保存。
   `ndisasm` があれば線形逆アセンブルも私的出力する。

**線形 disassembly にはデータが混ざる。命令 grep や byte match だけでは
実行経路・関数境界・到達性を立証できない。**
ツールの成功名も `BYTE_CHECK_PASS`、文脈は `NOT_REVIEWED` と明示する。
欠落入力の実行結果は `HFRE_INPUT_MISSING` / BLOCKED / exit 2 だった。

検証は hash、MZ 構造、landmark 内容の独立関数と、それらを合成する入口に分離。
負例は小さな合格 fixture から一項目だけを変え、固有エラーコードを比較する。
実際の商用プログラムや現在の Git 履歴がないと通らない負例にはしない。
これは入力内容の検査であり、保護済み成果物の履歴・gate の Git topology を
検査したものではない。

## 原本入手後の制御フロー追跡

- 開始点は MZ entry と確認済み caller。code/data と relocation 対象を分離する。
- 563F 近辺の routine の入口と全 return を取り、caller の引数・global を追う。
  image mode、サイズ、`-m6` / `-3` 等の option 分岐は DOC と命令の両方で照合する。
- D0=A5/A1/E1/E5 の各列は別々に前提条件を記録する。
  到達した一例を全モード共通初期化に一般化しない。
- 0191/0193 の座標値を組み立てるコードから、DA のデータ源、loop 上限、
  X=0086/0087 の更新と戻り先まで追う。PC-98 work area と board 操作を区別する。
- 581D の停止経路は ESC と他キー、正常終了と異常終了の caller を確認する。
- 必要なら切り出した routine を限定命令数で実行し、IN を合成、OUT を記録する。
  入力変更で各分岐を対照化する。元 viewer 全体や installer は実行しない。
- 最終的な `[BINARY]` 項目には、確認した offset、短い命令バイト、caller、
  条件、データ依存、戻り先、未解明点を添える。原本・raw dump は Git 外に保つ。

## 自作コードの検証との区別

C の device/backend/CLI を分離し、host は mock だけをリンクする。
DOS backend は Open Watcom の 16-bit DOS 以外でコンパイルすると `#error`。
生成 MZ、backend の `IN AL,DX` / `OUT DX,AL` / `OUT DX,AX` の opcode と、
実際の MZ にバイト一致する backend leaf routine の限定実行を検査できる。
これは **自作コードの型・命令幅・拒否条件の検証**であり、J1 の解析でも
HyPER-FRAME+ の電気的仕様・実機動作確認でもない。

[試験計画と実行記録](test_plan.md)に、PASS、BLOCKED、NOT_RUN を分けて記録する。
