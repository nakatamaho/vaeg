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

# HyPER-FRAME+ 逆ASM・操作経路の保存記録 — M145

## 結論と到達点

**専用 JPEG viewer J1、ディザ変換 H1、multi-board viewer D1 の三原本を確認し、
HF 系の初期化・画素 I/O・表示切替・特殊座標・終了経路を再構成した。**
初回の「原本未入手」は下記の歴史的記録となった。
現在は、下記の限定範囲に独立確認済み `[BINARY]` 所見がある。
実カード、映像、PC-9801 runtime は未実施で、`[HARDWARE-TESTED]` は **ない**。
既配布 HFTEST の `init` / `banktest` は解除しない。G145 は NOT_READY。

本書を操作解析の集約先とする。数値仕様は [暫定仕様](hyper_frame_plus_spec.md)、
自作ツールの使用法は [README](../README.md)、検証範囲は [試験記録](test_plan.md)。
原本 DOC の説明、実バイト列から確定した処理、ハードウェア上の推論を区別する。
以下の疑似記述は独立した解析表現であり、そのまま実行する初期化コードではない。

- [入力とアドレスの読み方](#hf-inputs)
- [J1 の条件付き初期化](#hf-j1-init)
- [画素と E5](#hf-pixels)
- [特殊座標・スクロール・終了キー](#hf-scroll)
- [H1 の変換と残存状態](#hf-h1)
- [D1 の検出と関数 table](#hf-d1)
- [復元した使用手順](#hf-usage)
- [限定実行と再現方法](#hf-reproduction)

初回実装では、安全な既知部分まで停止させず、公開資料に基づく C89 の画素処理、
バックアップ/照合/復元、明示的停止候補、bank 算術、mock を実装した。
未確定の初期化・窓アクセスは実行不可とした。
以前の M145 の multi-board viewer **D1** と、今回求められた JPGVHF **J1** は
別資料であり、以前の D1 の解析結果を J1 の確認に流用しない。

<a id="hf-inputs"></a>

## 入力台帳

| 入力 | 今回の状態 | 証拠としての扱い |
|---|---|---|
| P1 論文 PDF | 取得・本文確認、List 1/2 はページ画像でも確認 | `[PUBLISHED]` |
| J1 `jpgvhf1e.lzh` | 提供コピーを保全、`-lh5-` 展開 | 既提示 hash と一致 |
| J1 `JPGVHF.EXE` | MZ/relocation/命令・限定制御フローを確認 | 下記 `[BINARY]` 所見の対象 |
| J1 `JPGVHF.DOC` | strict CP932 変換、原文確認 | 1.14、1994-05-08 の付属文書 |
| H1: HFDITHER 1.51 | COM と付属 DOC を確認 | HF 画像を PC-98 画面へ変換する別プログラム |
| D1: MJL 1.00 | MZ と付属 DOC、内包 archive の目録を確認 | 先行 M145 の D1 と本体バイト一致 |
| `JPGVHF_UTF8.txt` | 未検出 | 原文との一致未確認 |
| `HF_IO_NOTES.txt` | 未検出 | 実験メモは、入手しても独立検証とはしない |
| `HFDEMO.C`, `HFTEST.C` | 未検出 | 旧実験コードを正解や完成実装として採用しない |
| 1990 年メーカー manual、68 pages | 未入手 | P1 の参考文献欄に掲載された事実だけ確認 |

初回の検索では未検出、公開 URL 候補は 404 または HTML だった。
その失敗記録は保持し、後から入手済みに書き換えていない。
今回、アクセス可能な作業領域にあった提供コピー三本を別の新規領域へ保全した。
遠隔側との hash 照合は SSH 認証で BLOCKED であり、遠隔から転送できたとは報告しない。
原 archive、展開物、変換 DOC、raw 逆ASM、入力 hash、出所、trace は Git 外に保持する。
H1/D1 の私的 identity は neutral ID に結び付け、ここには追加公開しない。
「原本」は提供された参照コピーを指す。版名だけで別配布物との同一性や、
作者由来の真正性を証明したものではなく、解析対象は私的台帳の bytes に固定する。

依頼で明示されていた J1 の SHA-256 は、今回 **三点とも原本コピーと一致**した:

```text
527d0f162c80f5f8a963ee6f3c6da89168a62eec9436345683ac6ba49f3b2476  jpgvhf1e.lzh
dc89dda016e6b687eef95593dc58cc225269a20fda3b96a1bd4227ae8f993324  JPGVHF.DOC
f2f845543be983a9534eedea0b22a4647dcf685dfd5f0372e13a16acd11647ab  JPGVHF.EXE
```

確認した構造は LHa `-lh5-`、DOC 14049 bytes、EXE 50976 bytes、
MZ `e_cparhdr=20h` / header 512 bytes / load image 50464 bytes / relocation 3、
entry 0000:0000、overlay 0、trailing bytes 0。DOC の版・日付も一致した。
`verify_reference.py` は exit 0 / `BYTE_CHECK_PASS`。この結果自体は、後述する
caller・分岐の手作業による追跡や限定命令実行まで自動認定するものではない。

一方、取得できた P1 PDF は 1277220 bytes、SHA-256 は
`12f8769a0a3943923d0f9da08a1a606c29b9f6f465b64d664c146c83b9128eae`。
[取得元・ページ番号・仕様上の読み方](hyper_frame_plus_spec.md#公開資料-p1)を参照。
OCR は誤りを含むため、コードの O/0、数値、段組みをページ画像で確認した。
原資料全文や List 全体の転載ではなく、独立した説明と実装だけを Git に置く。

## アドレス表記と確認済みランドマーク

アドレス・port・I/O 値は十六進表記、画素寸法・バイト数・件数・計算式の数値は
十進表記とする（明示した `h` / `0x` は十六進数）。以下の位置は原本を再確認する
ための座標であり、原命令列全体の転載ではない。関数名・変数名は解析上の仮名である。

- **J1 `I:xxxx`**: MZ load image 内 offset。file offset = `0200h + I`。
  MZ entry は I:0000。実行時の load segment 加算と別である。
- **D1 `G:xxxx`**: graphics code segment 内 offset。
  image offset = `12800h + G`、file offset = `13C00h + G`。
  MZ header は 1400h bytes、relocation 1201、entry 0000:0000。
  本文中の G を、EXE file offset や J1 の I と混同しない。
- **H1 `C:xxxx`**: COM の IP 表記。file offset = `C - 0100h`。
  MZ header/relocation はない。入口は C:0100。

| J1 image | file | `[BINARY]` 確認した処理 | 文脈 |
|---:|---:|---|---|
| 54CF | 56CF | 初期化関数の入口 | caller I:0DC7 / 16F4、寸法・option globals 使用 |
| 563F | 583F | `BA EE 00 B0 C0 EE` | 寸法計算後、EE=C0 |
| 5645 | 5845 | D0=A5 | 続いて D3=00、条件付き消去へ |
| 564B | 584B | D3=00 | D0=A5 の後 |
| 56AA | 58AA | D0=A1 | Y=0191 の八要素の初期化後 |
| 56CF | 58CF | D0=E1 | Y=0193 の 32 要素の初期化後 |
| 42BD | 44BD | D0=E5 の送出部分 | モノクロ行関数 I:4238、I:437D で E1 に戻す |
| 581D | 5A1D | D0=00 → EE=00 の関数 | I:04F8 から呼出し、I:582D で return |
| 4056 | 4256 | `E7 D6` | I:3FE9 の行処理、AX=行番号 mod 400 |
| 40AA | 42AA | `E7 D4` | I:40A5 は準備命令であり OUT 自体ではない |
| 54A7 | 56A7 | Y=0193、X=0086/0087、DA low/high | I:5474 の条件付き縦位置更新 |
| 568B / 56B0 | 588B / 58B0 | Y=0191 / 0193 | 下記の異なる二ループ |

**逆ASMの表記上の落とし穴**:
NDISASM 3.01 の `out byte 0xd6,ax` はポート即値が 8 bit という表記で、
`E7` のデータ幅は **AX の 16 bit**。`E6` は AL の 8 bit。
同 worker の D0/D1 shift 表示には count=0 と見える箇所もあるが、
opcode の定義と限定実行では 1-bit shift である。表示文字列を仕様にしない。
また J1 I:57D4 の mode table や H1 C:0A66 以降の定数をコードとして読めば、
存在しない I/O が現れる。J1 I:8202 の関数も table 直後から再同期が必要で、
線形 listing に見える I:8204 の `IN` は命令境界を外した誤読である。

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
初回の欠落入力は `HFRE_INPUT_MISSING` / BLOCKED / exit 2。
今回の原本では exit 0 / `BYTE_CHECK_PASS` となり、両方の記録を保持している。

検証は hash、MZ 構造、landmark 内容の独立関数と、それらを合成する入口に分離。
負例は小さな合格 fixture から一項目だけを変え、固有エラーコードを比較する。
実際の商用プログラムや現在の Git 履歴がないと通らない負例にはしない。
これは入力内容の検査であり、保護済み成果物の履歴・gate の Git topology を
検査したものではない。

## 制御フロー追跡の方法

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

<a id="hf-j1-init"></a>

## J1 初期化の条件付き制御フロー

### Option と globals の対応

原本 DOC は PC-9801/互換機、186 以上の CPU、HyPER-FRAME+ を対象とする。
`-3` は HF III の HF 互換モード用で、スクロール非互換を明記する。
これは「+ と III が同一」「VA でも動く」という仕様ではない。

`[BINARY]` I:022D の main と option parser を追い、次の対応を確認した。
DS は起動時に image-relative 0B09h を基に設定される。
以下の offset は **DS 内**であり、I:offset ではない。

| DS offset | 解析上の意味 | 初期値・設定箇所 |
|---:|---|---|
| 1490 / 1492 | 入力画像の幅 / 高さ | JPEG 側の処理から参照 |
| 1494 / 1496 | 出力幅 / 出力高さ | I:576E–5780 で確定、幅は最大 640 |
| 14BC | `-o` 横 crop offset | I:030C の option branch |
| 14BE | `-m` 描画モード | 既定 3、I:02C3 の branch |
| 14C0 | `-i` 情報表示・キー待ち | 既定 1、I:0348 の branch |
| 14C2 | `-c` 消去条件 | 既定 1、I:0323 の branch |
| 14C4 | `-3` 指定 flag | I:036A で 1 |

I:54CF の入口は即 I/O ではなく、I:57D4 の table で寸法計算へ分岐する。
有効な `-m` は 0,1,2,3,5,6,8。4/7 の table slot を、公開された動作モードとして
扱わない。通常の正の寸法についての再構成は次のとおり。

| mode | 出力予定寸法 w,h の計算 | 備考 |
|---:|---|---|
| 0 / 1 / 3 | 元の W,H | mode 1 の画素出力は後述の単成分経路 |
| 2 | W/2, H/2 の整数商 | 独立した縮小描画経路 |
| 5 | W>640 なら Q7 scale `floor(81920/W)` | 寸法は `ceil(W*s/128)`, `ceil(H*s/128)`、不要なら mode 0 |
| 6 | W>640 または H>400 なら `min(floor(81920/W), floor(51200/H))` | 同じ Q7 寸法計算、不要なら mode 0 |
| 8 | `(W+7)/8`, `(H+7)/8` の整数商 | quick scan |

これは全 JPEG 入力の安全性監査ではない。極端な寸法・overflow・破損画像は未検証。
`-o` は画像側の crop offset であり、任意の画面表示位置を指定する option ではない。
横幅が 640 を超える場合に範囲を丸め、mode 3 では sampling block 境界へ丸める。

### 初期化の順序

`[BINARY]` I:0DC7 / 16F4 は decoder 側の buffer 準備後に I:54CF を呼ぶ。
電源投入直後に無条件で呼んでよい関数であるとは認定しない。
I:563C 以降の順序を、通常の寸法計算が済んだ場合について再構成すると:

1. PC-98 BIOS wrapper I:5866 を呼ぶ（INT 18h、AH=41h）。
2. **byte** `EE=C0 → D0=A5 → D3=00`。
3. `-c` の条件を満たすと、通常画像領域へ消去目的のゼロ出力をする。
   Y は 0..399 を word OUT D6、各行の X は **639..0 の降順**を word OUT D4。
   各画素のデータは **D8=00 一回だけ**で、DA/DC はこのループにはない。
4. **word** Y=0191、X=0180..0187 の各点へ **byte D8=00**（八点）。
5. **byte D0=A1**。
6. **word** Y=0193、X=0080..009F の各点へ **byte DA=00**（32 点）。
7. **byte D0=E1**。
8. `-3` なし、かつ予定幅 **w<639** の場合だけ、Y=0193 を保持したまま:
   - `q = 640 - floor((640-w)/2)` を X=0084/0085、DA low/high に分割して送る。
   - 399 (=018Fh) を X=0086/0087、DA low/high に分割して送る。
9. crop/output globals を確定し、signal handler を設定する。
   `-i>0 && h>400 && -3なし` のときだけ EMS 準備関数 I:7C01 を呼ぶ。
10. I:57D0–57D3 で stack/register を戻して return。

**EE=C0, D0=A5, D3=00, D0=A1, D0=E1 の五つだけ抜き出すと、
間にある 40 個の特殊座標へのデータ出力と寸法依存処理を失う。**
一方、これら 40 個を「画像の余剰領域を黒塗りしただけ」とも解釈しない。

`-c1` の実バイト列は `w<640 OR h<400` で消去する。
DOC の「640×400 以下」という略記を `W<=640 AND H<=400` に実装し直してはいけない。
次の境界は限定実行でも確認した（w,h は縮小後の予定寸法）。

| 条件 | 画素領域の消去 |
|---|---|
| c0 / 640×400 | なし |
| c1 / 640×400 | **なし** |
| c1 / 639×401 | **あり** |
| c1 / 641×399 | **あり** |
| c1 / 641×401 | なし |
| c2 / 640×400 | あり |

`-3` を付けても上記 4–7 の特殊座標初期化は通る。
主に後段の位置設定、縦更新、EMS 利用が抑制されるのであって、
「特殊座標を一切触らない安全モード」ではない。

<a id="hf-pixels"></a>

## J1 画素出力と D0=E5 の役割

`[BINARY]` 通常カラー行関数 I:3FE9 は、行番号 mod 400 を **word D6** へ出す。
一画素ごとに X を **word D4** へ出し、三つの buffer から
**byte D8 → DA → DC** に値を出す。X を増やして次の画素へ進む。
I:40AA が最初の unrolled loop の X 出力であり、I:40A5 はその準備である。

モノクロ行関数 I:4238 は、I:42BD で **D0=E5** とし、画素ごとに
**word X → byte D8** だけを出し、I:437D で **D0=E1** に戻す。
E5 の使用はこの一箇所だけではない。I:4977、4EEA、4F9C、539C にも
対応する単成分出力の前置きがあり、各経路には E1 への復帰がある。
これらすべてを一つの無条件初期化定数と扱ってはいけない。

- `[BINARY]` E1/E5 の差は bit 2、単成分出力の間だけ切り替える。
  D1 の消去経路も D0 の bit 2 を立てて D8 だけを出す。
- `[INFERRED]` bit 2 は同じ値を複数色面へ送る動作に関係する有力候補。
  「RGB 同時書込み enable」というメーカーの正式定義は未確認。
  mock が三色に複製したからといって、実機の定義の証明にはならない。
- `[BINARY]` J1/H1/D1 は、一度指定した座標で RGB を続けて転送する設計である。
  ソフトが address hold を期待していることは分かったが、実カードの全 mode での
  保持、自動更新、word/byte の電気的同値性までは確認していない。
  したがって HFTEST の「各成分前に byte 座標を再選択」は変更しない。

<a id="hf-scroll"></a>

## 特殊座標と縦スクロール経路

### 更新対象の整理

| 選択座標とデータ port | `[BINARY]` 用途・値の由来 | 限界 |
|---|---|---|
| Y=0191, X=0180..0187, D8 | J1/D1 が bit 2 を立てた経路で八点をゼロ化 | 個別 register 名、zoom/LUT 等との対応は未確定 |
| Y=0193, X=0080..009F, DA | J1/D1 が 32 点をゼロ化 | 全要素の意味は未確定 |
| Y=0193, X=0084/0085, DA | D1 が横位置引数を low/high、J1 が幅依存 q を送る | 表示位置への寄与は強く支持、pixel origin の厳密な物理対応は未実測 |
| Y=0193, X=0086/0087, DA | D1 が縦位置引数、J1 が行番号由来の値を low/high | 縦表示位置更新としてソフトが使用、実モニタ上の移動は未実測 |

これは通常の y=0..399 画素 API と別経路として扱う。
特殊座標がある事実から、bank の余りを自由 RAM と認定してはいけない。

### J1 の更新式と off-by-one

`[BINARY]` I:5474 は行描画から呼ばれ、`-3` があれば I/O なしで戻る。
通常の正の行番号 s に対して、出力幅が **639 以上**なら `(s+1) mod 400`、
それ未満なら `s mod 400` を、Y=0193 / X=0086,0087 / DA に送る。
その後 `s-400` を引数として I:7E0A を呼ぶ。この後続は EMS 側の画像保持処理で、
今回の限定 I/O 検査では戻り値を与える stub とした。EMS round trip は未検証。

I:7B64 の block 更新は別の式で、`-3` と画像下端を検査する。
開始行と今回の block 行数の和から、元幅が 640 未満なら 1 を引いて mod 400 とし、
同じ特殊座標へ出す。これを I:5474 の式と無条件に統合しない。
この block 関数は静的追跡までで、53 ケースの限定実行には含めていない。

対話スクロールから使う **I:82B6 は byte 座標出力**でもある。
`-3` なら無 I/O。通常は `v=(引数+400+delta) mod 400`、
`delta=(出力幅>=639 ? 1 : 0)` として:

```text
OUT8 D7=01, D6=93, D5=00, D4=86
OUT8 DA=low(v)
OUT8 D4=87
OUT8 DA=high(v)
```

これは P1 の D7→D6→D5→D4 と整合する別の実バイナリ例でもある。
幅 638/639 と行 399/400/655 の境界を限定実行で区別した。
実機なしに「639 は誤りだから 640 へ修正」と判断しない。

### キー待ちと終了

`[BINARY]` main I:047A は `-i>=1` のときだけ I:04D4 を呼ぶ。
その中で I:8107 のキー取得結果を調べ:

- ESC=001Bh: **I:581D を呼ばず**終了側へ進む。
- その他の終了キー: **I:581D を呼ぶ**。
- I:581D は byte **D0=00 → EE=00**、RET。制御値の保存も読返しもない。

通常の待ち経路と、EMS で保持した背の高い画像の対話経路は異なる。
I:8107→8202 は PC-98 INT 18h と keyboard state を使い、上下移動、ESC、
space/return を扱う。対話時の任意キーすべてを終了キーとみなさない。
`-i0` はこの通常終了時の待ち・停止呼出しを省くので、単なる表示文の抑制ではない。

原本 DOC の「ESC で画面を残す／他の終了キーで出力を止め、画像自体は残る」は、
この分岐と整合する。ただし今回確認したのは **送出有無**であり、映像や VRAM 保持の
実測ではない。異常終了はすべて I:581D を通るわけでもない。
例えば I:57F6 は PC-98 側の復帰 wrapper I:582E を呼ぶだけであり、
signal handler I:57FE の経路も正常な非 ESC 終了と同じ列ではない。
「どんなエラーでも最後に off される」と想定して運用しない。

<a id="hf-h1"></a>

## H1 の目的・読出し・後始末

**HFDITHER は、HF メモリに既に存在する画像を読み、PC-98 本体の
8 色または 16 色画面へディザ変換するプログラムである。**
原本 DOC は HF/HF+、HF III の HF 互換モードを挙げるが、機種互換性には留保がある。
新しい JPEG をロードする機能や、HF の正常性を判定する機能ではない。

`[BINARY]` C:0100→0144 の option 解析後、`-d` または `-p` が選ばれた経路が
C:0207 へ進む。どちらもなければ usage 側へ進む。
実処理の board 部分は以下で、機種/カードの存在確認を挟まない。

| 段階 | COM IP | 処理 |
|---|---:|---|
| 準備 | 0207 | byte EE=C0 |
| 保存 | 020B / 020F | byte IN D0 と IN D3、C:0CD0 に D3=low / D0=high で保存 |
| 切替 | 0214–021A | D3=`saved_D3 OR 80h`、D0=81h |
| PC-98 側設定 | 021C 以降 | INT 18h、6Ah、A8h 系 palette、PC-98 VRAM への出力 |
| 画素読出しの一例 | 0337–0371 | word Y、word X、byte IN D8→DA→DC |
| 終了処理 | 027E–028B | INT 18h AH=0Ch の後、保存 D3、保存 D0 の順で byte OUT |

Matrix 系は C:0325、diffusion 系は C:046E 等の分岐を持つ。
zoom は **元画像の読出し座標のソフトウェア計算**であり、ここで HF の zoom register
を設定しているわけではない。倍率 1,2,4,8 を shift count 0,1,2,3 にし、
`source_x=(output_x >> shift)+x_offset`、同様に y を計算して D4/D6 に出す。
実行した一画素の対照では zoom=8、出力行 399、y_offset=10 が source_y=59 になった。
mono 系の一例 C:03E9 は、読んだ成分を table に通した後、77R+151G+28B を用いる。
ディザ品質や全 VRAM plane の見え方は今回の試験対象ではない。

追加の未解決点として、digital 側 C:023A は BX=0A62h を INT 18h/AH=43h へ渡す一方、
提供個体の四 byte 定数 `67 45 23 01` は C:0A66 から始まる。
C:0A62 はまだ命令末尾である。ここでは BIOS の table 引数解釈まで検証しておらず、
誤りの原因や正しい置換値を断定しない。本体 palette の全色が正常との保証もない。
この点も原 viewer 全体を実行・修正せず、今後の確認対象として保存する。

**後始末の限界**:

- EE を保存して元に戻す出力は、この cleanup にない。EE=C0 は残り得る。
- D0/D3 の保存値が FF でも、そのまま送出する経路である。
  合成 FF ケースでも読値の拒否・ボード不在判定はしない。
- PC-98 側の palette、表示モード、描いた画像まで保存・復元する処理ではない。
- 通常画像の読み出しが主目的でも、制御 port と本体 VRAM を変更する。
  「read-only な HF probe」と呼んではいけない。

<a id="hf-d1"></a>

## D1 の検出・dispatch・HF 系アクセス

### 全検出を移植してはいけない理由

MJL は付属 DOC で PC-98 と PC/AT、DOS 3.3 以降、V30/80186 以上を挙げる。
同梱の外部 driver 群は別 board 向けであり、HF 経路は本体内にある。
補助 archive の画像 loader を「HF 専用 SDK/source library」とは解釈しない。
今回はそれらの installer/driver/loader を実行せず、WSR-E の解析も再開していない。

`[BINARY]` graphics 初期化 G:0056 は INT 10h の応答で platform table を選ぶ。
PC-98 側 G:0332 は本体 work area 0000:045C/0458/054D や FD80:0002 なども参照する。
G:03B3 / 03BD / 03C7 は、順に G:08C7、0B06、0C52 を試す。
失敗した次の候補も **別方式の書込み**をするので、全体を「同じ board の再試行」と
解釈してはいけない。VA BIOS/work area へそのまま移植することは特に危険である。

| 候補 | 検出入口 | protocol の差 | 今回の分類 |
|---|---:|---|---|
| F1 | G:08C7 | EE/D0、Y=0191/0193、D8/DA/DC | J1/H1/P1 と対応する HF-family 経路 |
| F2 | G:0B06 | D0/D2 に 15 組の table、表示切替は DE | F1 と別 protocol。Super Frame 等の厳密な variant 名は未資格 |
| F3 | G:0C52 | D8/DA/D2 等の word 設定、画素データは D0 を反復 | F1 と別 protocol。固有製品名への割当ては未資格 |

F1 の HF-family 対応は同じ特殊座標・port 操作の照合によるもの。
実カードの「+」改版を識別する ID register が見つかった、という意味ではない。

### F1 の検出列と失敗時の残存状態

`[BINARY]` G:08C7 の列は:

1. byte EE=C0。
2. D0 を読み、`a=read_D0 OR 81h` として D0 に書く。
3. D0 を再読し a と比較。不一致なら G:0910 で AX=FFFFh、RET。
   **EE/D0 の書戻しはない**。
4. a を退避し、D0=`a OR 04h`。
   Y=0191 / X=0180..0187 / D8=00 の八点を処理。
5. D0=a に戻し、Y=0193 / X=0080..009F / DA=00 の 32 点を処理。
6. G:0B16 へ合流。word X=0、word Y=0。
   D8 の旧値を保存し、D8=11h、D8 を読んで 11h と比較する。
7. 一致時だけ旧 D8 値を送出し、AX=0、CX=640、DX=400 で戻る。
   不一致時は G:0B38 で AX=FFFFh とし、**旧 D8 値の出力をしない**。

step 2 の read が FF、step 3 の read も FF なら最初の gate は通る。
その後の画素 test まで進むので、FF の制御 read を使う安全な検出手段ではない。
画素 test の成功時も、全制御状態を元に戻す処理ではない。
この古い実装の cleanup を、HFTEST の復元規則として採用しない。

### 選択される関数 table

検出成功で G:089F の 18-word table が候補に保存される。
640×400 / 24-bit mode の選択は G:03EF から、既存 mode の leave 呼出し、
table copy、選択 mode の entry 呼出しへ進む。
主要 slot と実処理は次のとおり（名前は解析上の仮名）。

| table slot | routine | 処理 |
|---:|---:|---|
| 1 | G:0914 | D0=`(read_D0 AND CFh) OR 10h`、D3=00 |
| 2 | G:0923 | D0=`read_D0 AND CFh`、D3=80 |
| 3 | G:0930 | D0 の bit 2 を立て、640×400 の D8=00、保存した D0 を送出 |
| 4 | G:095C | word X/Y、D8=CH、DA=DL、DC=CL |
| 5 | G:0971 | word X/Y、D8→CH、DA→DL、DC→CL |
| 6 | G:0984 | 行書込み。buffer は R,G,B 順、X を明示更新 |
| 7 | G:09D7 | 行読出し。下記の buffer 順に注意 |
| 12 | G:09EE | D0 bit 6 を立て、Y=0193、X=0084..0087 に横/縦 low/high |
| 13 | G:0A36 | 重なり方向を考慮した、port 経由の画素コピー |

slot 1/2 は **J1 の D0=00→EE=00 と別の表示切替手順**である。
一方の手順を別プログラムの復帰処理として無条件に混ぜない。
bit 4/5/7、D3 bit 7、EE の正式な意味は manufacturer manual での確認が必要。

G:09D7 は D8 を AH、DA を AL に置いて STOSW、その後 DC を STOSB する。
したがって buffer は **G,R,B** 順となる。限定実行でも読値 12/34/56 に対し
格納 bytes が 34/12/56 であることを確認した。
G:0984 の書込み buffer と自動的に対称だと仮定してはいけない。
これは実命令の格納順の所見であり、未追跡のすべての caller の表示不具合まで
実証したものではない。原 viewer を改変したり、ledger に架空の修正を加えない。

G:0984 の行送りも単純な `x<640` の安全 API ではない。
通常の内部経路は x>=1024 で 640 を引く処理を持ち、最終行 399 の跨ぎには別処理がある。
長さゼロや不正座標を外部から渡す安全性を保証する関数ではないので、
古い leaf routine の引数規約をそのまま公開 API にしない。

<a id="hf-usage"></a>

## 復元した PC-9801 上の使用手順と制限

以下は **DOC と逆ASMから復元した運用説明**であり、今回実行済みの手順ではない。
対応本体、カード改版、SW1、port 競合、モニタ/pinout/同期、DOS 環境を先に資格化する。
原 viewer を VA や開発ホストで起動する指示ではない。

1. **HF に画像を入れる**: J1 の文書上の例は
   `JPGVHF -m6 TEST.JPG`。画像は自作の合成データにする。
   mode 6 は 640×400 内への縮小であり、bank/safety mode ではない。
   c1 は上記の条件付き消去。既存画像を残したいからといって、c0 が制御 register を
   変更しないとは考えない。
2. **画像を残して DOS へ戻る**: 通常の終了待ちでは ESC。
   byte 停止列は通らないが、DOS の text 側の処理等は別途ある。
   `-i0` も待ちを省くが、初回確認で入力・終了記録を省略する目的に使わない。
3. **本体画面へディザ変換する**: H1 の文書上の例は
   `HFDITHER -d0 -a`。HF 内の既存画像を読む。`-p0` 等は matrix 法、
   `-m` は mono、`-z1/2/4/8` と `-x/-y` は読出し範囲の指定。
   `-a` は本体 analog 16 色用なので、対応していない本体/表示器には使わない。
   H1 は制御 port、palette、本体 VRAM を変えることを記録する。
4. **HF 出力を停止する**: J1 の通常の非 ESC 終了と、D1 の mode-leave は異なる。
   後者を前者の完全復元と呼ばない。HFTEST `off` の送出列は J1 と一致したが、
   物理的な停止・画像保持は未実測である。
5. **多機種検出を診断代わりにしない**: D1 の mode 選択 option は
   幅/高さ/色数を選ぶもので、検出時の他 protocol への書込みを排除する保証ではない。
   したがって「640,400,24 を選べば HF だけを安全に probe する」とは説明しない。

J1 DOC は、メルコ製 RAM board と EPSON PC-386M/P の内蔵 scanner interface の
I/O 競合にも言及する。これは当該文書の注意であり、現在の機体での実測ではない。
RAM board の switch 記述を WSR-E や HF の SW1 に流用しない。
写真に写った DIP の意味は、この三本だけでは解決していない。

<a id="hf-reproduction"></a>

## 限定実行による裏付けと再現

[`trace_reference.py`](../trace_reference.py) は、原本から上記の閉じた範囲だけを
Unicorn の 16-bit CPU に置き、IN は script から返し、OUT は全件記録する。
**DOS の EXE/COM entry、viewer 全体、installer、実ハードウェアは実行しない。**
指示していない code 範囲への遷移、割込み、IN の port/幅不一致、余った IN script、
命令 budget 内に return しないケースは失敗する。最大 2000000 instructions/case。
PC-98 BIOS、CRT、signal/EMS 関数等の省略位置と呼出し回数を明示して照合する。
H1 の準備・一画素サンプルは途中境界で終了させた prefix 検査であり、
元 routine 全体の自然な return を再現したものではない。

private binding の schema は次の形。実際のパス/hash は Git に置かない。

```json
{
  "j1": {"path": "inputs/j1.bin", "sha256": "<actual-sha256>"},
  "h1": {"path": "inputs/h1.bin", "sha256": "<actual-sha256>"},
  "d1": {"path": "inputs/d1.bin", "sha256": "<actual-sha256>"}
}
```

J1/D1 は EXE 全体、H1 は COM 全体を binding する。hash を確認した後、
J1 は load image、D1 は graphics segment、H1 は origin 0100h で配置する。
全 DOS loader を再現する代わりに、対象の DS/stack/global/引数を明示する。
元の数学 helper は必要範囲で実行するが、BIOS/EMS を stub した結果を
OS 実行や EMS 保存/復元の成功に昇格しない。

```sh
python3 -B verify_reference.py --references "$REFERENCE_DIR" --output "$NEW_INPUT_REPORT"
python3 -B tests/test_trace.py
python3 -B trace_reference.py --bindings "$PRIVATE_BINDINGS" --output "$NEW_TRACE_OUTPUT"
```

Unicorn 2.1.4 を使い、**53 cases、全件 PASS**。
`instructions` 欄の合計 5393136 は code-hook 訪問数（stub 境界を含む）であり、
物理 CPU の実命令数や時間ではない。
比較は期待する全 IN/OUT の方向・port・幅・値・順序、必要な返り値/global/buffer、
stub 呼出し順まで行う。原本を含まない 12 fixture tests も PASS。
負例は合格 fixture から方向、port、幅、値、順序、欠落、入力 script、許可範囲、
budget、割込みを一項目だけ変え、固有コードを照合する。

| ケース群 | 件数 | 実際に照合したもの |
|---|---:|---|
| J1 初期化 | 17 | c 境界、幅 638/639、mode 0/1/2/3/5/6/8、縮小不要 fallback、-3、EMS 呼出し有無 |
| J1 word 縦更新 | 12 | 幅 638/639 × -3 有無 × 行 399/400/655 |
| J1 byte 縦更新 | 2 | 同じ境界での byte 順序・値 |
| J1 停止・終了キー | 4 | leaf off と、ESC/space/return を与えた終了判定 |
| J1 行描画 | 2 | 合成二画素の RGB と mono、E5/E1 切替 |
| D1 F1 検出 | 4 | 成功、D0 gate 拒否、pixel 拒否、D0=FF 継続 |
| D1 残りの leaf | 7 | entry/leave、pixel put/get、行 buffer 順、位置更新、全画素消去 |
| H1 prefix/書戻し | 5 | 既知値/FF の保存と出力、zoom 8 の一画素読出し |

raw trace は little-endian `<BHBH>` の連続（方向 0=IN/1=OUT、port、幅、値）。
`report.json` / `cases.json` は入力 identity、runner/validator hash、Unicorn 版と
library digest、entry/ranges、命令数、trace digest を保存する。
この trace に実カードからの応答は一つも含まれない。
初回 51-case の記録も保持し、RGB/H1 sample を加えた 53-case と区別した。
最初の fixture tests は意図した IN 不一致のコードを検出したが、ctypes の補助警告も
出した。IN callback が整数を返してから停止エラーを通知する形へ修正し、
最終 fixture tests では警告なしで同じコードを検出することを確認した。
初期ログと修正後の結果を両方保持し、原本の正常 trace に差がないことも照合する。

## 未解決事項と次の保存対象

- **実ハードウェア**: board revision/SW1、read side effect、電源投入状態、
  wait、割込み、monitor timing、bus 幅と word transaction の分割。
- **register の正式定義**: D0/D3/EE の全 bit、0191 の八点、0193 の 32 点。
  bit 2/6 等の候補を正式な enable 名として API に固定しない。
- **variant**: HF/HF+ の改版識別、HF III 互換動作、D1 F2/F3 の固有機種対応。
- **bank**: EC=C0..C5 と 80000h 窓は P1 由来。今回の限定経路はこれを実証しない。
  128 KiB aperture の予約/競合が未資格なので `banktest` は実装・実行しない。
- **OS/全ソフト**: 全 JPEG 入力、signal/異常終了、EMS round trip、D1 全 dispatcher、
  H1 全ディザ画像・本体 VRAM/palette 出力は未実行。53 件に含めない。
- **VA**: PC-98 BIOS/work area の置換、低位 I/O の所有権、bus/映像の互換性は別調査。
  186 命令があることや native emulator に handler がないことだけでは許可しない。

将来原本や実機結果が増えたら、本書の source ID と offset 表を起点に追記する。
原資料全文、raw 逆ASM、私的 artifact identity を public Git に追加せず、
独立説明・検証器・未解決表を残す。これが記録を分散させないための保存境界である。

## 自作コードの検証との区別

C の device/backend/CLI を分離し、host は mock だけをリンクする。
DOS backend は Open Watcom の 16-bit DOS 以外でコンパイルすると `#error`。
生成 MZ、backend の `IN AL,DX` / `OUT DX,AL` / `OUT DX,AX` の opcode と、
実際の MZ にバイト一致する backend leaf routine の限定実行を検査できる。
これは **自作コードの型・命令幅・拒否条件の検証**であり、J1 の解析でも
HyPER-FRAME+ の電気的仕様・実機動作確認でもない。

[試験計画と実行記録](test_plan.md)に、PASS、BLOCKED、NOT_RUN を分けて記録する。
