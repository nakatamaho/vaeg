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

# M166 — GA-98NBIV（CL-GD5434）を PC-88VA で調べるプローブと vaeg の I/O・VRAM 窓

Status: **IN PROGRESS.** 実機での測定は保守者が行う。
Branch: `milestone/m166-ga98nbiv`（origin/main
[538877ee](https://github.com/nakatamaho/vaeg/commit/538877ee) から）;
commit prefix `M166:`.

## 目標

保守者の依頼（2026-11-24）: I・O DATA の GA-98NBIV（GA-98NB4、Cirrus Logic
CL-GD5434、4 MB）を PC-88VA / VA2 の汎用スロットで調べる。最初の到達点は
二つ: (1) I/O 疎通（Sequencer の SR06 に 12h を書いて読み戻し、CRTC の
CR27 & FCh = A8h で GD5434 を識別）、(2) VRAM 窓（E0000h の同じ番地から
二つのバンクに別々の値を置き、交互に読み戻し、元に戻す。E4000h の第 2 窓は
別に判定）。範囲は保守者の判断で「プローブと vaeg の窓まで」:

- MS-DOS 4.0（FreeDOS-88VA msdos4-va.3 preview 3）で動く V30 用のプローブ。
  モード `baseline`、`identify-word`、`identify-byte`、`window-read`、
  `window-test`。最初は読み取りと構成表示だけで、有効化・窓の設定・窓への
  書込みはそれぞれ明示して指定する。各 I/O を実行前に記録する。
- vaeg に GA-98NBIV の I/O（設定 `y=0`）と VRAM 窓を入れ、プローブを vaeg で
  通す。実機の結果はまだないので、推測した部分はそう書く。表示（カードの
  映像出力）は範囲外。

## 根拠

- PC-98 用の既存ドライバ（非公開の入力）の検出・初期化手順と窓の使い方:
  ポート 46E8h（Setup/Sleep）、xxE0h 系（VGA の 3C0h〜3CFh を (port & 0F)
  << 8 | 40E0h に、3D4h/3D5h を 54E0h/55E0h に、3DAh を 5AE0h に）、
  `OUT16 44E0h ← 1206h` で SR06 を解除、窓は E000:0000〜3FFF（GR09）と
  E400:0000〜3FFF（GR0A）、バンクは 16 KiB 単位。表示切替は 40E1h のビット 0。
- XFree86 3.3 の PC-98 版 Cirrus ドライバ（`cir_pc98.c`、`compiler.h` の
  `port_convert`: VGA ポート p を ((p << 8) & 7F00h) | E0h へ）。ワード単位の
  OUT（`outw(0x3ce, …)` など）も同じ変換で使う。
- Cirrus Logic, *Alpine Family Technical Reference Manual*（SR06、SR07、
  GR09〜GR0B、CR27）。Neko Project 21/W の GA-98NB 解析資料（I/O 変換の表、
  40E3h のリレー）。
- BNN『PC-88VA テクニカルマニュアル』: ポート 152h（メモリマップ
  レジスタ）の RBC03〜00（ROM0、E0000h〜EFFFFh）と RBC13〜10（ROM1、
  F0000h〜FFFFFh）は 8〜F で「バススロットに解放」。VA で E0000h を拡張
  スロットに出す方法は、これを使う。
