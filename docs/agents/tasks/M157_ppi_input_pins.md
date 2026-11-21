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

# M157 — 8255 の入力ポートが相手の出力を取り逃す

Status: **COMPLETE; G157 human gate PASSED on 2026-11-21.**
Branch: `milestone/m157-hang-case`（origin/main
[20c8cba0](https://github.com/nakatamaho/vaeg/commit/20c8cba010cc21569364c6d648e1b4776fda53f6) から）;
commit prefix `M157:`.

## 目標

保守者の報告: あるプログラムが、VA（V2 モード）と PC-8801MA2 のどちらでも、
ゲームが始まって間もなく固まる。原因を特定して直す。

## 原因

- そのプログラムは、オブジェクトの絵をタイマー割り込み（1/600 秒）で少しずつ
  ディスクユニットから受け取る。受け取りの初めに、本体は PPI を入力
  （モード 93h）に切り替えてから、ポート A/B で 4 バイトのヘッダ（書き込み先と
  量）を読む。
- ディスクユニット側は、本体が切り替える前にヘッダをポート A/B に出している
  ことがある。vaeg の 8255 は、相手が書いた瞬間に受け側が入力だったときしか
  値を記録しなかったため、本体は古い値を読み、データを別のアドレスに書いた。
- オブジェクトの大きさの表が古いままになり、その幅（130 バイト）で切り詰めた
  幅 65 バイトが、スプライト描画の自己書き換えコード（60 バイトまで）の範囲を
  超えて、飛び先がループになって固まっていた。

## 修正

- `io/i8255.c`/`.h`、`io/subsystem.*`、`io/subsystemif.*`: 入力のポート
  A/B は、相手がそのポートを出力として駆動していれば相手の出力ラッチを読む
  （接続の覗き見コールバック）。保存される 8255 の状態は変えない。
  [d8f700b1](https://github.com/nakatamaho/vaeg/commit/d8f700b11c4a35e332810336f8508da754b5b041)。
- `sdl2/selftest.c`: 相手が先に A/B を出し、後で受け側が入力に切り替えて
  読む試験を追加（修正を外すと
  `an input port missed data the peer drove before the mode change` で失敗）。

## 確認

- `vaeg --selftest`（ダミー SDL）PASS。

## Gate

G157: 保守者が、このブランチの MinGW ビルドで、報告のプログラムを VA と
PC-8801MA2 で遊び、固まらないことを確認する。

## G157 記録

2026-11-21、保守者が G157 を passed とした。ゲートで使ったビルドは
`d8f700b1`（Linux ホスト MinGW、`Rel.20261121 (m157, d8f700b1)`、SHA-256
`65b3a1e7c383b3c79f8024b760a72aff6eb3c25b8dd0a976e9615917dd3553ce`）。
リリースは承認されていない。

## 統合キャンペーン

保守者の指示で、メディア/デモの回帰だけを実施した（実画面の自動確認は
NOT_RUN、保守者の判断）。結果は
[M157 integration QA](../reports/m157_integration_qa.md)。デモ媒体 9/9、
全デモの描画 34/34、MANDEL.BAS、自己起動 SGP、ユーティリティ FDD と SASI の
起動 4/4 が PASS。
