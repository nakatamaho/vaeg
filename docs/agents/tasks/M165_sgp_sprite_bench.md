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

# M165 — SGP スプライト・ベンチマーク

Status: **IN PROGRESS.** 実機での測定は保守者が行う。
Branch: `milestone/m165-sgp-sprite-bench`（origin/main
[7aa05049](https://github.com/nakatamaho/vaeg/commit/7aa05049) から）;
commit prefix `M165:`.

## 目標

保守者の依頼: SGP のスプライト描画の速さを、320x200 と 320x224、16・256・
65536 色、8x8・16x16・32x32・64x64 で測る D88 を作る。キャラクターは著作権の
ないよう生成する。同じプログラムを実機でも動かして測る。

## 内容

`demos/sgp-sprite-bench/`（[README](../../../demos/sgp-sprite-bench/README.md)）:

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
- `build.sh`: `--msdos4` で msdos4-va.2 の MS-DOS 4.0 にプログラムを
  入れた `sgp-sprite-bench-msdos4.d88` も作る。

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

vaeg では主記憶と GVRAM の転送元が同じ値になる（転送元の違いを時間に
反映していない）。実機の値と比べて、違いがあれば別のマイルストーンで扱う。

## Gate G165

標準の人間ゲート。加えて、保守者が実機（PC-88VA / VA2 / VA3）で
`SGPBENCH` を動かし、結果表を記録する。
