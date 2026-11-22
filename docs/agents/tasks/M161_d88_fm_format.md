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

# M161 — D88 のフォーマットで FM の密度が記録されない

Status: **COMPLETE; G161 human gate PASSED on 2026-11-22.**
Branch: `milestone/m161-d88-fm-format`（PR
[#84](https://github.com/nakatamaho/vaeg/pull/84) の
[64e55e11](https://github.com/nakatamaho/vaeg/commit/64e55e11b5ea739fa4d32713153611d15f766664) から。その親は
origin/main [3c2c9cd0](https://github.com/nakatamaho/vaeg/commit/3c2c9cd047e48d6c28a01d84eb226c2bd3d9dbb2)）;
commit prefix `M161:`.

## 目標

保守者の指示で PR #84 を取り込む。FDC の FORMAT TRACK を FM（MF = 0）で
実行しても、D88 のセクタに FM の印が付かず、すべて MFM として記録されて
いた。そのため、FM でフォーマットしたトラックを FM で READ ID / READ DATA
すると失敗した（PR #84 の報告では、1 トラック目が FM のディスクを
DISKCOPY すると、コピー先の検査で失敗する）。

## 原因

`fdd/fdd_d88.c` の `fdd_formating_d88` は、セクタヘッダを 0 で埋めたあと
密度のバイト（`mfm_flg`）を設定していなかった。D88 では 40h が FM、00h が
MFM で、READ ID / READ DATA はこのバイトと FDC の MF ビットを比べている。

## 修正

- `fdd_formating_d88`: `mfm_flg` を FDC の MF ビットの反転にする（MF が
  40h の MFM なら 00h、MF が 0 の FM なら 40h）。PR #84 の
  [64e55e11](https://github.com/nakatamaho/vaeg/commit/64e55e11b5ea739fa4d32713153611d15f766664)。
- `sdl2/selftest.c`: D88 format density 試験。空の 2D イメージの 1 トラックを
  FM で、もう 1 トラックを MFM でフォーマットし、それぞれ同じ密度の
  READ ID では読め、反対の密度では読めないことを確かめる。修正を外すと
  `M161_D88_FORMAT_FM_NOT_FM` で失敗する。

## 確認

- `vaeg --selftest`（ダミー SDL）PASS。修正を外すと上の試験で失敗する。
- `git clang-format` で変更行の整形に差分なし。

## Gate

G161: 保守者が PR #84 の取り込みを指示した（2026-11-22）。CI の結果と、
保守者の指示による統合キャンペーンの結果を確認してから main にマージする。

## G161 記録

2026-11-22、保守者が PR #84 のマージを指示し（「84マージできます」）、
G161 を passed とした。PR #84 の GitHub CI は 20 本すべて PASS。
リリースは、保守者の指示による Rel.20261122 の準備で別に扱う。

## 統合キャンペーン

保守者の指示で両方を実施した。結果は
[M161 integration QA](../reports/m161_integration_qa.md)。評価したのは
main（M160 まで）に M161 をマージした候補 `1e3b7216`。全デモの描画
34/34、MANDEL.BAS、自己起動 SGP、ユーティリティ FDD と SASI の起動 4/4 が
PASS。実画面の自動確認（仮想 X11）は、速度画面 10 ケースを撮影（OCR は
NOT_RUN）、レイアウト 32/32 一致（APPROVAL_REQUIRED）。同じ候補の
`linux-ci-gcc` CTest は 160 本すべて PASS（1 本は無効）。最初の CTest は、
止めたつもりのキャンペーン用スクリプトが同じビルドで並行して実行していて
結果が混ざったため、単独で実行し直した。
