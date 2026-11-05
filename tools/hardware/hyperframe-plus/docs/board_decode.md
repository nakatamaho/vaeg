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

# HyPER-FRAME+ — 基板写真と EC 非使用経路の成立条件

**M145 / G145 NOT_READY。EC を使用しない画素転送経路はソフトウェア上に存在する。
しかし、VA に挿してよい構成、冷間初期化、診断 D88 の実行条件はまだ確定していない。**
故障・非互換の確定でも、実機試験の指示でもない。
[三原本の操作解析](reverse_engineering.md)、[暫定仕様](hyper_frame_plus_spec.md)と
分けて、写真で追える範囲と、残る電気的確認箇所を記録する。

## 証拠と作業範囲

- **B1 / B2**: 提供された表面 / 裏面写真。ローカル原本と保全コピーをバイト照合した。
  指定された遠隔ファイルとの同一性確認・転送は行っていない。
- 本書の **`[PHOTO]`** は部品の刻印、配置、露出したパターンの視認だけを指す。
  `[HARDWARE-TESTED]`、導通確認、回路図の完成を意味しない。
  他の根拠区分は[暫定仕様](hyper_frame_plus_spec.md#根拠の区分)に従う。
- 原画から切出しと裏面の左右反転を行った。反転は表面との対応確認用であり、
  実物の裏面にプローブを当てる際の左右・ピン順とは異なる。
  写真は透視・倍率が異なり、単純な重ね合わせを電気的接続の証拠にしない。
  鮮鋭化、生成補完、推定パターンの描込みは行っていない。
- スイッチ / ジャンパ変更、通電、導通測定、実機 IN/OUT は **NOT_RUN**。
  元写真、切出し、座標、hash、資料 PDF、解析用出力は Git 外に保持する。

## 写真から絞れた調査箇所

| 箇所 | `[PHOTO]` 所見 | 確定していないこと |
|---|---|---|
| SW1 / RA1 / IC1 | 四連 SW1、その直下の RA1、`Digital Arts LISA` 刻印の IC1 が識別できる。裏面の RA1 に対応する五つのパッドのうち四つから、四本の独立した線が IC1 フットプリント内のビア列まで続く | SW1 各接点から RA1 への正確な対応、ビア以降の IC1 ピン、プルアップ/ダウン、スイッチ機能 |
| J0〜J6 / RA2 / RA4 | SW1 とは別の七つのジャンパ位置がある。写真の並びは左から J4/J5/J6/J3/J2/J1/J0。J3 だけ他の六位置にあるキャップが見えず、二本のピンが見える | 出荷設定、正常/異常、電気的開閉状態、各ビットの意味。J3 を埋める指示ではない |
| IC2 | `SN74HC273N` の刻印。20-pin、ノッチは B1 の左側 | どの I/O 書込みを保持するか、入力がバスデータかアドレスか、リセットの配線 |
| IC3 | `SN74HC85N` の刻印。16-pin、IC2/J 群の下側、RA4 の直下 | 何と何を比較するか。J 群との全ネット、I/O とメモリのどちらの選択か |
| IC27 / IC28 | 二つの `HD74HC257P` が IC3 の下に並ぶ | 入力の信号名、選択・出力許可条件、メモリ窓またはバス駆動への関与 |
| IC29 / IC30 | 刻印はそれぞれ `HD74LS138P` / `HD74HC138P` | A/B/C と enable の接続先、出力と EC/EE/D0 系との対応。LS と HC を同一の電気特性としない |

### SW1 について言える範囲

`[PHOTO]` RA1 側の四本の線は裏面で右へ回り、IC1 の領域内にあるビア列で表側へ移る。
**その先は部品下や不鮮明な部分に隠れ、IC1 の特定ピンまで確定できない。**
したがって「SW1 が IC3 のアドレス比較値を直接変更する」ことは今回確認できなかった。
逆に、IC1 が内部でデコードしている可能性も排除できないため、
「SW1 は映像専用でアドレスには無関係」とも断定しない。

`[INFERRED]` 調査対象を SW1 だけに限定するのは不十分である。
別の J0〜J6 と、保持素子 IC2、比較器 IC3、後段の選択回路を併せて追う必要がある。
これは部品の機能と配置から得た**調査優先順位**であって、
「J 群がメモリアドレス設定」「IC2 が EC レジスタ」という回路の確定ではない。
近接したパッド、交差、反射で途切れた線を接続済みネットとして補完しない。

## EC 非使用で何を省けるか

`[PUBLISHED]` P1 の画素 I/O は D4〜D7 に座標、D8/DA/DC に RGB を指定する。
別の bank 方式は EC と `80000h–9FFFFh` の 128 KiB 窓を使う。
[画素と bank の仕様](hyper_frame_plus_spec.md)を参照。

`[BINARY]` J1/H1/D1 の限定解析には、EC を使用せず座標と色成分を転送する経路がある。
保存済みの 53-case trace を再実行せず hash 照合して集計したところ、
当該限定 trace 内の EC アクセスはゼロで、ポート集合は次のとおりだった。

| 種類 | 保存済み trace に現れたポート |
|---|---|
| IN | 00D0, 00D3, 00D8, 00DA, 00DC |
| OUT | 00D0, 00D3, 00D4, 00D5, 00D6, 00D7, 00D8, 00DA, 00DC, 00EE |

原本、runner、validator、Unicorn libraries、全 trace の digest と既存集計を照合した。
これは既に限定した経路の再集計であり、whole viewer、BIOS/EMS stub の内側、
全エラー経路の I/O やメモリ動作を追加検証した結果ではない。

候補は二段階に分ける。

1. **既知の画素モードにあるボードへの転送だけ**なら、D4〜D7 と D8/DA/DC の
   七つの byte ポートで構成する余地がある。EC と aperture を使う必然性はない。
   ただし、そのモードが電源投入時に成立するとは分かっていない。
2. **電源投入から表示まで**には、少なくとも解析済みの制御候補 D0/D3/EE と
   特殊座標処理も調査対象となる。J1 の五つの制御値だけを抜き出したものでは足りない。
   条件付き clear、40 点の特殊座標処理、寸法依存更新、PC-98 BIOS 呼出しを分離する
   必要がある。[J1 初期化解析](reverse_engineering.md#hf-j1-init)を参照。

**「CPU が窓を使わない」と「カードが窓に応答しない」は別条件である。**
初期状態でメモリ decode が有効なら、診断開始前の既存 RAM へのアクセスと競合し得る。
仮に reset 直後は無効でも、起動 firmware / OS / 他の処理による EC 書込みで
有効化される回路なら、診断プログラムから EC を除くだけでは回避できない。
`[UNKNOWN]` 今回の HF+ が実際にそう応答するか、どの値で有効になるかは未確認。

## VA 側で既知の重なりと、未確定の重なり

| 対象 | 根拠と現在の判断 |
|---|---|
| 00ECh | VA の第2水準漢字 ROM アクセスと重なる。manual では EC OUT がアドレス下位、ED OUT が上位、EC/ED IN が右/左パターン。HF+ の bank select と同じ数値のポートであり、利用回避を優先する |
| V3 なら EC が空くか | そうは扱えない。manual の V1/V2-only 注記に反し、M144 のボードなし V3 読出しは VA の EC=FF、VA2 の EC=86 を記録している。これは **読出し**の観測であり V3 の OUT 動作や FF の駆動状態は証明しない |
| 00D0 / 00D3 / 00EE / 座標・RGB ポート | 調査した VA 資料で個別割当や競合の全条件を確定できていない。「未掲載」「エミュレータの handler がない」を「空き」「IN なら安全」としない |
| 00D0 と 01D0 | 後者の VA RAM 制御とは別の 16-bit アドレス。HF+ 側の部分 decode や alias を確認するまで同一視しない |
| 80000h–9FFFFh | HF+ の掲載された窓位置と VA の拡張メイン RAM / bank RAM 領域が重なる。カードの応答条件、実装済み RAM の構成と選択・抑止条件が不明であり、常時競合とも安全とも未確定 |

VA manual の I/O area 表は `0000h–00FFh` を system area 0、
`1000h–FEFFh` を user area としている。前者への所属だけでは個別ポートの衝突を
証明しないが、自由使用の根拠にもならない。
根拠は manual の I/O area 表・漢字 ROM 節（印刷 p.28）、
メモリ配置・RAM 節（印刷 pp.47–48）と
[M144 の測定記録](../../../../docs/agents/tasks/M144_v1v2_port_decode.md)。
原資料は Git に転載しない。

## 次の確認を少数の箇所に絞る

以下は **未実施の確認計画**。スイッチ総当りや VA 上の IN/OUT より先に行う。
最初の追加資料として、SW1/RA1、IC2/J 群/RA4/IC3、IC27〜IC30 の
**表裏をそれぞれ真上から撮った接写**が有用である。
部品名・ノッチ・ピン・ビアが同時に写る範囲とし、現在の設定を動かさない。
ただし、部品下や ASIC 内部は高解像度写真でも見えない。

必要に応じた導通調査は、ボードを取り外し、本体・映像ケーブルなどから切り離した
無給電状態でのみ検討する。下記の pin は部品上面基準。
裏面を反転した解析画像の左右をそのまま実物に当てはめない。

| 優先 | 調査点 | 判別したいこと |
|---|---|---|
| 1 | IC2 の pin 1 (`/CLR`) と pin 11 (`CLK`) の行先。まずパターンで隣接 IC/ビアまで追う | 何が保持値を消去し、どの選択信号が保持を更新するか。`74HC273` があるだけで power-on 値を 00 と仮定しない |
| 2 | J0〜J6 の各二端子、RA2/RA4、IC3 の比較入力群と pin 6（A=B 出力）の対応 | ジャンパと比較器が接続しているか、比較結果がどの選択回路に入るか。入力を C-bus 信号へ追う前に「A19〜A16」などの名前を付けない |
| 3 | IC29/IC30 の入力 1/2/3 と enable 4/5/6 の接続先、出力側の行先 | 低位アドレスだけでなく上位アドレス、I/O / memory サイクル、read / write の区別がどこで成立するか |
| 4 | SW1 接点と RA1 の四本、IC1 内側のビアから先 | SW1 の入力先を確定する。LISA のピンに到達しても、仕様なしで内部の意味は決められない |

IC3 の 74HC85 系の比較入力は、A0/A1/A2/A3 が 10/12/13/15、
B0/B1/B2/B3 が 9/11/14/1。ここでの A/B は **IC 内の入力群名**であり、
C-bus の A/B 面や CPU のアドレス名ではない。
カスケード入力 2/3/4 も比較出力を左右するため、pin 6 だけで decode は決定しない。

ブザーの有無だけをネットの確定にしない。基板内の抵抗・半導体・閉じたスイッチを
介した並列経路がある。測定する場合は端点、向き、抵抗値、計器条件を記録し、
直接の銅箔接続と区別する。低電圧の測定器を使い、絶縁抵抗計、高電圧、
外部電源の注入、ピンの短絡、部品の取り外しはこの計画に含めない。

### 照合した論理 IC 資料

- TI [SN74HC273](https://www.ti.com/lit/ds/symlink/sn74hc273.pdf),
  SCLS136F、2022-04、p.3 の pin table と機能記述。
- TI [CD74HC85](https://www.ti.com/lit/ds/symlink/cd74hc85.pdf),
  SCHS136F、2022-02、p.3 の 16-pin 図。**74HC85 系の pin/論理の参照**であり、
  写真の `SN74HC85N` と同じ品番の原データシートを得たという意味ではない。
- TI [SN74LS138](https://www.ti.com/lit/ds/symlink/sn74ls138.pdf),
  SDLS014、1988-03 改訂、p.1 の pin/enable 記述。
  **138 系の機能と pin の参照**であり、写真の Hitachi 製 LS/HC 品の
  電気特性・遅延を代用していない。

これらは汎用 IC の仕様であり、HF+ の接続図や SW1 説明書ではない。

## EC 非使用構成へ進むための判定

1. **I/O 側**: 上記七ポート、必要な制御候補、上位アドレスの alias、byte 幅、
   read/write の応答条件、VA 本体側の競合を確認する。
2. **メモリ側**: reset から起動過程までの窓の選択条件と、データ出力などの
   バス駆動条件を確認する。窓を独立に無効化できるなら、その設定が画素 I/O を
   残すことも確認する。無効化設定の存在自体はまだ分かっていない。
3. **表示側**: 既知モードの成立または冷間初期化、ボード改版、映像端子と
   ケーブル/monitor/sync の適合を別に確認する。

現時点は **「EC 非使用の転送方式は候補として残るが、VA 実行可否は BLOCKED」**。
写真だけで ASIC 内部や初期状態の論理が解けなければ、無通電の配線追跡で
分かる限界を明示して止める。必要な PC-9801 上の信号観測は、構成と観測点を
決めた別実験として承認を得る。VA での未知ポート走査や 16 通りの SW1 試行に
置き換えない。先に提案した D0/D3 の read-only probe は安全性未成立として撤回し、
診断プログラム/D88 はまだ作成していない。
HFTEST の `init` / `banktest` と VA 実行の禁止は解除しない。
