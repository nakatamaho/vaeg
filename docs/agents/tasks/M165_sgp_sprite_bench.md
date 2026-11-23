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

# M165 — スプライト・ベンチマーク（SGP、CPU、両方、ダブルバッファ）

Status: **IN PROGRESS.** 実機での測定は保守者が行う。
Branch: `milestone/m165-sgp-sprite-bench`（origin/main
[7aa05049](https://github.com/nakatamaho/vaeg/commit/7aa05049) から）;
commit prefix `M165:`.

## 目標

保守者の依頼: SGP のスプライト描画の速さを、320x200 と 320x224、16・256・
65536 色、8x8・16x16・32x32・64x64 で測る D88 を作る。キャラクターは著作権の
ないよう生成する。同じプログラムを実機でも動かして測る。

続く依頼: SGP でなく CPU で描くスプライトの、同様のベンチマークも作る。
CPU 描画の方式は、保守者の VA 向けアーケードゲーム移植の CPU スプライト
（コンパイル済みスプライト）を参考にする。

さらに続く依頼: SGP と CPU を混ぜて（同時に）描くテストも作る。その後、
ダブルバッファ（消去と描画を隠れたページで毎 VRTC 行う）のテストも作る。

## 内容

`demos/sprite-bench/`（[README](../../../demos/sprite-bench/README.md)、
はじめ `demos/sgp-sprite-bench` として作り、CPU 版を足すときに改名）:

- `gen_data.py`: 4 種のオリジナルキャラクター（丸いロボット、イカ、箱
  ロボット、星）を円・矩形・多角形から各サイズで描く。16 色、色 0 透明。
  8・16 bpp への展開表と 5x7 フォントも出す。
- `sgpbench.asm`: MS-DOS の `SGPBENCH.COM`。G0 だけの 1 画面（単一
  プレーン、320 ドット）で、画面 2 種 × 色 3 種 × 転送元 2 種（主記憶、
  GVRAM）× サイズ 4 種の 48 テスト。各テストは同じ SGP リスト（`BITBLT
  0105h`、128/64/32/16 個）を 180 VRTC のあいだ繰り返し、終わったリストの
  スプライト数 ÷ 180 を出す。320x224 は 15.98 kHz の 224 ライン SYNC
  （`demos/tsp-sprites` と同じ）で、24.8 kHz では飛ばす。最後に結果表を
  320x200 の画面に出し、キーで DOS に戻って同じ表を DOS に出力する。機種は
  F000:FFFE から表示する。
- `cpubench.asm`: `CPUBENCH.COM`。同じ 48 テストを CPU で描く。転送元の
  代わりに描き方 2 種: `COMPLD`（画像ごと、16 色ではドットの偶奇ごとの
  far ルーチン。不透明なワードは即値の MOV、透明を含むワードは読んで
  AND・OR して書く、透明なバイトは何もしない）と `LOOP`（画像のデータを
  なめる素朴なループ）。どちらも `gen_data.py --cpu` が `CPU4.DAT`・
  `CPU8.DAT`・`CPU16.DAT` に作り、プログラムが色数ごとに読み込む。VRTC は
  割り込み（IR2 だけ許可）で数え、180 VRTC で描き終えたスプライトを数える。
- `mixbench.asm`: `MIXBENCH.COM`。各リストを SGP と CPU で分け、同時に
  描く。転送元の代わりに SGP の受け持ち 3 種（1/4、1/2、3/4。4 個ずつの
  ブロック単位）で 72 テスト。1 ラウンドは、SGP に受け持ち分のリストを
  始めさせ（主記憶の転送元、`BITBLT 0105h`）、CPU が残りをコンパイル済み
  ルーチンで描き、SGP の終わりを待つ。180 VRTC 内に終わったラウンドの
  スプライトを数える。
- `dblbench.asm`: `DBLBENCH.COM`。隠れたページに毎フレーム消去と描画を
  行い、次の VRTC で FB0 の DSA を書き換えて表示する。1 VRTC に 1 フレーム
  を保てる最大枚数を、0〜500 の二分探索（1 回 30 フレーム、遅れた
  フレームで打ち切り）で求める。方式 3 種: `CPU`（2 ページ、CPU が矩形を
  消してコンパイル済みルーチンで描く）、`SGP`（2 ページ、1 つのリストで
  `BITBLT 0005h` によりゼロから消し、`BITBLT 0105h` で描く）、`MIX`
  （3 ページ、前のフレームで表示したページを SGP が消す間に、CPU が前の
  フレームで SGP の消したページに描く）。ページが 256 KB に入らない
  320x224 の 65536 色と、65536 色の `MIX` は `-`。
- `common.inc`: 4 つのプログラムが共有する部分（テストの順、画面の設定、
  見出し、位置、結果表）。2 番目の次元の数 `N_WAYS`（既定 2、MIXBENCH は
  3）を取り、表が 200 ラインに収まらないときは行間を 8 ラインにする。
  `WHOLE_RESULT`（結果を整数で出す）と `FOOT_TEXT`（表の最後の行）も
  プログラムが決められ、`measure` は実行できないテストに `SKIPPED` を
  返せる。`sgp.inc` はページの先頭（`page_lo`/`page_hi`）と消去
  （`sgp_erase`）、任意のアドレスのリストの起動（`submit_at`）も扱う。
  `sgp.inc` は SGP の部分（転送元、リストのコマンド、起動）、`cpu.inc` は
  CPU の部分（`.DAT` の読み込み、表の項目、コンパイル済みルーチンの呼び
  出し、VRTC の数え方）。
- `build.sh`: 4 つのプログラムと `.DAT` を作る。`--msdos4` で msdos4-va.2
  の MS-DOS 4.0 に 7 つのファイルを入れた `sprite-bench-msdos4.d88` も
  作る。nasm はカレントディレクトリの `data.inc` を先に拾うため、作業
  ディレクトリで動かす（古い `data.inc` のある場所から実行するとカンマの
  ない古いフォントが入っていたのを直した）。

## 確認

vaeg（linux-release、--nowait）で msdos4-va.2 の MS-DOS 4.0 から
`SGPBENCH` を実行した。

- PC-88VA2、24.8 kHz: 200 ラインの 24 テストを実行し、224 ラインは `-`。
  キャラクターが各色数で正しい色・透明で描かれ、1 行目にテスト名と結果が
  出る。結果表のあと DOS に戻る。
- PC-88VA2、15 kHz（`Monitor15kHz=true`）: 48 テストすべてを実行。320x224
  の画面で 223 ラインまで描かれる。
- PC-88VA、15 kHz: 48 テストを実行。値は PC-88VA2 のおよそ 0.6 倍
  （`demos/sgp-contention` の VA と VA2 の比と同じ傾向）。

CPUBENCH（PC-88VA2、15 kHz）: 48 テストを実行。`COMPLD` と `LOOP` が
同じ絵を描く。生成したコンパイル済みルーチン 64 本は、命令を解釈する小さな
シミュレータでループ版のデータと同じ書き込みになることも確かめた（ツールは
リポジトリ外）。分割後の SGPBENCH は分割前と同じ値を出す。

MIXBENCH（PC-88VA2 と PC-88VA、15 kHz）: 72 テストを実行し、表は 8 ライン
間隔で 23 行が 200 ラインに収まる。PC-88VA2 の 320x200・16 色では、CPU
だけ（CPUBENCH `COMPLD`）の 193.1 / 82.3 / 29.2 / 9.5 に対し、`SGP1/4` が
200.5 / 88.5 / 31.8 / 10.3、`SGP1/2` が 145.0 / 86.0 / 33.6 / 10.8。分割
後の SGPBENCH・CPUBENCH は分割前と同じ値（CPUBENCH は差 0.1 以内）を
出す。

DBLBENCH（PC-88VA2 と PC-88VA、15 kHz）: 表のとおり 56 テストを実行し、
ページの入らない 16 テストは `-`。途中の画面で、どのページにもタイトルと
スプライトが正しく出ることを確かめた。PC-88VA2 の 320x200・16 色で `CPU`
101 / 43 / 15 / 4、`SGP` 37 / 22 / 9 / 2、`MIX` 76 / 46 / 17 / 5。
`sgp.inc` を変えたあとも、SGPBENCH は同じ値、MIXBENCH は 1 か所が 0.2
違うだけだった。

vaeg では主記憶と GVRAM の転送元が同じ値になる（転送元の違いを時間に
反映していない）。

## 実機の結果

保守者が PC-88VA と PC-88VA2/VA3 の 15 kHz で 4 つのプログラムを動かした
（2026-11-23）。結果表の写真から読んだ値を
[results.md](../../../demos/sprite-bench/results.md) に記録した。vaeg との
違い:

- CPU の描画（CPUBENCH）: 実機は 2 機種で同じ速さ（差 1% 以内）。vaeg は
  6〜16% 速すぎ、65536 色で最も大きい。
- SGP の転送元: 実機の PC-88VA2 では GVRAM の転送元が主記憶より 4〜15% 速く、
  PC-88VA では 1〜6% 遅い。vaeg は区別しない。
- MIXBENCH の `SGP1/4`（CPU の分が長い）で vaeg が速すぎる。SGP の分が
  半分以上では数 % 以内で合う。
- DBLBENCH: 実機ではどちらの機種も `CPU` が最も多く描く。vaeg は
  PC-88VA2 の `MIX` が 16x16 以上で `CPU` を上回っていた。

保守者の指示（「1-4.で」）により、M165 の中で次も行う。M165 のゲートが
まだ通っていないため、別のマイルストーンにせずこのタスクで扱う。

1. 実機の結果の記録（上記）。
2. vaeg の CPU の GVRAM 書き込みの速さと、SGP と CPU の競合を実機に合わせる。
3. SGP の転送元（主記憶 / GVRAM）による速さの違いを vaeg に入れる。
4. 24.8 kHz の比較用に vaeg の値を出す（実機の測定は保守者）。

### 結果ファイル

リダイレクト（`SGPBENCH > RESULT.TXT`）は vaeg の msdos4-va.2 で空の
ファイルになった（キー待ちのあとの出力で再現、原因は未解明。保守者の判断
で MS-DOS 側の問題として扱い、ここでは追わない）。各プログラムが表を
`PROGRAM.TXT` に直接書くようにした（aea974c3）。キー待ちは AH=07h。

### vaeg の修正（2026-11-23）

実機の表との比較で、次の 3 つを直した（台帳に記録）:

- e369c6cb: CPU が待つ命令語は主記憶から 4.5 クロック、GVRAM へのワード
  書き込みはバスを 6.5 クロック占有（CPU は待たない）。CPUBENCH の
  `COMPLD` は実機の -1〜+6%（修正前 +6〜+16%）、両機種同じ。
- 27dfccda: SGP の動作中に CPU が失う時間は BIU の命令取り込みも止める。
  MIXBENCH は全シェアで 3% 以内（修正前 SGP1/4 で最大 +17%）。DBLBENCH の
  PC-88VA2 `MIX` は `CPU` を上回らなくなった（実機と同じ）。
- 00004337: SGP の転送元が GVRAM のとき、PC-88VA2 で 1 ワード 6 SGP
  クロック速く、PC-88VA で 1.5 遅い。GVRAM/主記憶の比が両機種で 0.04 以内。

`demos/sgp-contention`（vaeg / 実機）: A-K は 0.93-1.04（修正前と同じ）、
N 1.00 / 0.95（修正前 1.13 / 1.11）、P 0.89 / 0.93（修正前 0.95 / 0.99）、
Q 0.99 / 0.98、R 1.01 / 1.05。ctest（linux-ci-gcc）160 件中 159 件が通り、
残る 1 件（ROADMAP の M165 行にゲート ID がない）を直した。

SGP 自体の速さは両機種で約 5% 遅いまま（M128 の校正、変更なし）。
24.8 kHz の vaeg の値は results.md に記録した（実機の測定は保守者）。

### SGPMEM と 2 回目の修正（2026-11-23）

保守者の依頼: SGP の動作中の CPU の遅れが、CPU の使うメモリで違うかを
測るベンチを作る（BNN テクニカルマニュアル 3.5 節の待ちの表には、SGP との
競合の記述はない）。`demos/sgp-contention/sgpmem.asm`（SGPMEM.COM、
1e5ee5e3）: 同じ形の 64 回ループを、レジスタだけ、主記憶 30000h・60000h、
GVRAM（G1）、TVRAM（バンク 1）で、読み・バイト書き・ワード書き。それぞれ
SGP なし、SGP が主記憶の転送元で描く間、GVRAM の転送元で描く間の 3 回。
表は画面と SGPMEM.TXT に出る。

保守者が PC-88VA と PC-88VA2/VA3 で測った（表は
[va-bus-timing.md](../../modernization/va-bus-timing.md)）。分かったこと:

- CPU の遅れは、CPU の使うメモリより SGP の転送元の場所で大きく変わる
  （レジスタだけのループ: VA 0.70 / 0.87、VA2 0.46 / 0.75。主記憶 / GVRAM）。
- 読むループは書くループより遅れが少ない（0.08〜0.19、未実装）。
- 単独では、TVRAM のバイト読みは主記憶より 13 クロック、バイト書きは 4
  クロック遅い。GVRAM のワード書きは主記憶と同じ速さ。
- SGP の 1 枚のコストは、書き先の開始ドットによらない。

これにより直したもの（775e2f08、2411c127、b00b3d05、3e2023f6。台帳に記録）:

- e369c6cb の 4.5 クロックの命令語と GVRAM ワード書きの 6.5 クロックの
  バス占有をやめ、転送後 3 語目以降の、CPU が待つ主記憶の命令語を 5 クロック
  にした。COMPLD は 16・256 色で 0.97〜1.04、65536 色で 1.06〜1.10。
- TVRAM のバイト読みの待ち 17、バイト書きの待ち 4。
- SGP の BITBLT の課金を転送元のワード単位にし、倍率を 4.6% 上げた。
- SGP の転送元が GVRAM の間、CPU の割合を VA 30/256、VA2 55/256 にした。

確認: linux-ci-gcc の ctest 160/160。SGPCONT の A〜K は修正前と同じ。
スプライトの 4 本との比は va-bus-timing.md の表。

残る問題（va-bus-timing.md の Open questions）: 主記憶のバイト読みの
ループが 13% 速い（奇数番地への分岐の扱いが疑わしい。偶奇を分けたベンチで
切り分けたい）、CPUBENCH の `LOOP` が 7〜10% 速い、読みと書きの遅れ方の差、
SGPCONT の P が 0.89 / 0.92、GVRAM 読みの待ち。


## Gate G165

標準の人間ゲート。加えて、保守者が実機（PC-88VA / VA2 / VA3）で
`SGPBENCH`、`CPUBENCH`、`MIXBENCH`、`DBLBENCH` を動かし、結果表を記録する。
