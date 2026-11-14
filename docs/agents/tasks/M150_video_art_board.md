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

# M150 — PC-8801-17 ビデオアートボード

Status: **COMPLETE; G150 human gate PASSED on 2026-11-14.**
Branch: `milestone/m150-video-art-board`（origin/main
[2ef14f6d](https://github.com/nakatamaho/vaeg/commit/2ef14f6d) から）;
commit prefix `M150:`.

## 目標

NEC PC-8801-17 ビデオアートボード（VAB）のエミュレーションを vaeg に
加え、X88000 のビデオアートボード・サンプル
（<https://quagma.sakura.ne.jp/manuke/vab_sample.zip>、`shiori.d88`）を
表示できるようにする（保守者の依頼）。

## 根拠

- `[X88000]` X88000 1.5.3 のソース（PDS、
  <https://quagma.sakura.ne.jp/manuke/x88src.html>）。
  `PC88Z80Main.cpp`（ポート B4h/B5h、E2h/E3h、拡張 RAM カード 1 の
  読み書き）と `X88ScreenDrawer.cpp`（`DrawScreenVAB`）。ソースは
  Git の外（`~/work/pc88va/tmp/m150/dl/`）に置き、仕様だけを独立に
  書き起こした。
- 実機の VAB は手元にない。挙動は X88000 の実装に従う。

## 仕様（X88000 1.5.3 の実装）

| 項目 | 内容 |
|---|---|
| RAM | 128 KiB。拡張 RAM のカード 1（E3h の bit3–2 = 01b）、bit1–0 で 32 KiB バンク。E2h の bit0 で 0000h–7FFFh の読み出し、bit4 で書き込みに出る（通常の拡張 RAM と同じ切り替え） |
| B4h（モード 1） | ラッチ。カード 1 が E3h で選ばれているときだけ読み戻せ、それ以外は FFh |
| B5h（モード 2） | 同上。表示には使われない |
| 表示 | `(B4h & 41h) == 41h` のとき、画面全体が VAB の 320×200 に置き換わる |
| 画素 | 1 画素 1 ワード（リトルエンディアン）、オフセット 0 から行順。bit15–10 = G（6 bit）、bit9–5 = R（5 bit）、bit4–0 = B（5 bit） |

X88000 は 320×200 を縦横 2 倍にして 640×400 で表示する（インタレース
表示でなければ奇数行は前の行の複製）。

## 実装

- `io/vab.c` / `io/vab.h`: ボードモデル。B4h/B5h のラッチ、カード選択の
  判定、RAM オフセット、表示条件、RGB565 への合成。
- `memoryva/memoryva.c`: 88 モードの拡張 RAM の読み書きで、カード 1 が
  選ばれ VAB が有効なら VAB の RAM（`vab_ram`）へ振り向ける。無効なら
  従来どおり主 RAM の拡張 RAM 領域。
- `vram/scrndrawva.c`: 表示条件を満たすとき、VAB の画像で画面全体を
  置き換える（最近傍の拡大、ガード列を考慮した番地）。
- 設定 `VideoArtBoard`（既定 OFF）。GUI の「デバイス」メニューに
  「PC-8801-17 ビデオアートボード」（リセット不要。無効時は空きスロット
  と同じ振る舞い）。
- 状態保存: `VAB`（ラッチ 2 バイト）と `VABRAM`（128 KiB）。
- CTest `vaeg_vab`（32 checks）: 無効時、B4h/B5h の読み戻し条件、カード
  とバンク、表示条件、画素形式と 2 倍拡大、リセット。

適用範囲: VAB は PC-8801 のバス用ボードで PC-88VA には挿さらないが、
vaeg では 88 互換のセッション全般（M113 の PC-8801 機種と V1/V2 モード）
で有効にできる。既定は OFF。

## 確認

- vaeg の headless 実行（`--extra-machine PC-8801MA`、`VideoArtBoard=true`、
  `shiori.d88` を FDD1）で、約 50 秒後に X88000 のサンプル画像
  （320×200、65536 色）が表示されることを画面写真で確認した。
  `VideoArtBoard` を無効にすると、同じディスクは N88-BASIC の空の画面
  （ファンクションキー表示のみ）で止まる。
- CTest `vaeg_vab` 32 checks / 0 failures、`vaeg_hfplus` 126 checks /
  0 failures。
- サンプルのディスクイメージと X88000 のソースは Git に入れない。

## Gate

G150: 保守者が vaeg（MinGW ビルド）で `shiori.d88` を PC-8801 機種で
起動し、画像が表示されることを確認する。

## G150 記録

2026-11-14、保守者が G150 を passed とし、main への merge を指示した。
ゲートで使ったビルドは `c9018afc`（MinGW、
`Rel.20261114 (m150, c9018afc)`）。統合キャンペーン（メディア/デモの
回帰）は実施していない（保守者の指示による）。リリースは承認されていない。
