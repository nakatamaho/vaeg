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

# M160 — ビデオアートボードのサンプルディスク

Status: **COMPLETE; G160 human gate PASSED on 2026-11-22.**
Branch: `milestone/m160-vab-demo`（origin/main
[3c2c9cd0](https://github.com/nakatamaho/vaeg/commit/3c2c9cd0) から）;
commit prefix `M160:`.

## 目標

保守者の依頼: V2 モード向けに、PC-8801-17 ビデオアートボード（M150）で
レイトレースとマンデルブロ集合の画像を表示するだけの D88 を用意する。
HyPER-FRAME+ のサンプル（M149、`demos/hyperframe+`）と同じ題材で、物体の
配置と場所は変える。起動の方式は X88000 のビデオアートボード・サンプルに
ならう。

## 内容

`demos/vab/`（[README](../../../demos/vab/README.md)）:

- `raytrace.py`: 金色の鏡面球を、虹色 7 個の球の輪が囲む。テラコッタと
  クリームの市松の床、夕焼けの空、左からの低い光。320×200 で直接描く。
- `mandel.py`: 象の谷（中心 0.2852 + 0.0117i、幅 0.0072、6000 回）。
  紫・ばら色・橙・クリームの循環パレット。
- `build-d88.py`: IPL（Z80、スクリプト内の小さなアセンブラで組む）と、
  2 枚の画像をボードの形式（G6R5B5、4×4 の組織的ディザ）にした 128 KiB
  ずつを、ファイルシステムのない 2D ディスクに置く。`--check` で
  コミット済みのイメージと比べる。
- `vab-samples.d88.xz`: 許可リストは README。中身はすべてこのディレクトリ
  のソースから作られ、システムファイルを含まない。

IPL は拡張 RAM のカード位置を順に選んで B4h の読み戻しでボードを探し、
表示をオンにし（B4h = 47h）、N88-BASIC ROM のセクタ読み出し（369Ah）で
512 セクタをボードの 4 バンクに書く。キーで次の画像に切り替わり、2 枚目の
次は 1 枚目に戻る。ボードがなければメッセージを出して止まる。

## 確認

- `build-d88.py` を 2 回実行し、2 回目の `--check` で同じイメージ
  （SHA-256 `38eec379e1a1e5d4b72c963483e86cd30a345024eb300cfaf763f6b6f6b124fb`）。
- 実機では試していない。

## Gate

G160: 保守者が vaeg で `vab-samples.d88` を V2 モードで起動し、2 枚の画像が
キーで切り替わることを確認する。

## G160 記録

2026-11-22、保守者が G160 を passed とし、main へのマージを指示した。
対象は `6c70c25a`（`vab-samples.d88` SHA-256
`38eec379e1a1e5d4b72c963483e86cd30a345024eb300cfaf763f6b6f6b124fb`）。
リリースは承認されていない。

## 統合キャンペーン

保守者の判断で、両方とも実施しない（メディア/デモの回帰、実画面の自動
確認とも NOT_RUN）。エミュレータ本体の変更はない。
