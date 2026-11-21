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

# M154 — V1/V2 モードの音源割り込みが止まる

Status: **COMPLETE; G154 human gate PASSED on 2026-11-21.**
Branch: `milestone/m154-v2-fm-sound`（origin/main
[5f3c71e9](https://github.com/nakatamaho/vaeg/commit/5f3c71e95a5f2ad5ec0aee972a50f55eead3b761) から）;
commit prefix `M154:`.

## 目標

保守者の報告: V1/V2 モード（まず VA2/VA の V2）で、あるプログラムの FM
音源の曲が鳴らない（起動直後の音は出る）。原因を調べ、FM の曲が流れる
ようにする。

## 原因

- そのプログラムは、E6h に 00h を書いて VRTC など 0〜2 のレベルを禁止し、
  OPN のタイマー B（27h=3Ah）と E4h=05h で音源割り込み（レベル 4）を使う。
- vaeg の 8214 モデルは、E6h で許可されていた間にラッチされた VRTC の
  要求を、E6h で禁止された後も CPU に提示していた。直後の E4h の書き込みで
  その VRTC がベクタを取り、8214 は受け付け不可のままになる。プログラムの
  VRTC 処理は E4h を書き直さないので、以後タイマー B の要求（1.2 ms ごと）が
  一度も届かず、曲の処理が動かなかった。

## 修正

- `io/pic.c` `pic8214_select`: E6h のビットが 0 のレベル 0〜2 は、ラッチが
  あっても提示しない（X88000 `CPC88::IntVectChangeMain` と同じ）。ラッチは
  残し、許可への変化でラッチを消す既存の規則（M144a）は変えない。
- `sdl2/selftest.c` 8214 mode: 禁止されたラッチ済み VRTC と音源の要求が
  並ぶ場合を追加（修正を外すと
  `prohibited VRTC latch was dropped or offered` で失敗）。

## 確認

- `vaeg --selftest`（ダミー SDL）PASS、8214 mode ok。
- 修正コミット
  [bc0c9f64](https://github.com/nakatamaho/vaeg/commit/bc0c9f64c6e7735b097d60249f71ed5ee7f70463)。

## Gate

G154: 保守者が、このブランチの MinGW ビルドで、報告のプログラムを VA2 と VA
の V2 モードで起動し、FM の曲が流れることを確認する。

## G154 記録

2026-11-21、保守者が G154 を passed とした（FM の曲が鳴り、問題なし）。
ゲートで使ったビルドは `bc0c9f64`（Linux ホスト MinGW、
`Rel.20261121 (m154, bc0c9f64)`、SHA-256
`d093a61d3b89773c18d86bb8589623aad680df7eac3d99d07df30ab94723c74e`）。
統合キャンペーン（メディア/デモの回帰、実画面の自動確認）は NOT_RUN
（保守者の判断で見送り）。リリースは承認されていない。
