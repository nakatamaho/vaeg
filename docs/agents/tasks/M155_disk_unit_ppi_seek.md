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

# M155 — PC-8801 のディスクユニットを独自に使うプログラムが起動しない

Status: **IN_PROGRESS; G155 NOT_READY.**
Branch: `milestone/m155-boot-case`（origin/main
[a29aad7e](https://github.com/nakatamaho/vaeg/commit/a29aad7e77d4ef932d6cf93260f82301544a3308) から）;
commit prefix `M155:`.

## 目標

保守者の報告: あるプログラムが起動しない。PC-8801MH では動くはずなので、
まず PC-8801MH（`--extra-machine PC-8801MH`）で原因を調べ、動くように
する。VA/VA2 の V1/V2 モードも同じ所で止まっていた。

## 原因

そのプログラムのローダーは、ディスクユニット（PC-80S31 相当）の RAM に
自前のコードを置き、コマンド 0Dh で実行させて、8255 を両方向に使う
2 バイト単位の転送でデータを受け取る。止まる原因は二つあった。

1. **8255 のモード設定語。** 転送の前にディスクユニット側のコードは
   PC4 を 1 にし、転送後はモード設定語（91h）を書くだけで PC4 を下げる。
   8255 はモード設定語で出力ラッチをすべて 0 にするが、vaeg はラッチを
   保持していたため、本体は PC4 が 0 になるのを待ち続けた。
2. **シーク終了時の ST0。** ディスクユニット側のコードはヘッド 1 を指定して
   シークし、Sense Interrupt Status の ST0 を `AND FCh` して 20h と比べる。
   vaeg は Seek/Recalibrate の ST0 にヘッド番号を入れていた（24h）ため、
   リキャリブレートとシークを繰り返し続けた。uPD765 はシーク終了時の ST0
   にヘッド番号を入れない（X88000 `BuildST0` も同じ）。

## 修正

- `io/i8255.c`: モード設定語で出力ラッチ（A、B、C の出力ビット）を 0 に
  し、相手側へ伝える。リセット時は相手側へは伝えない。
  [f9e391ae](https://github.com/nakatamaho/vaeg/commit/f9e391ae4bc56d03b328088479224cb000c41f75)。
- `io/fdc.c`: Seek と Recalibrate の ST0 にヘッド番号を入れない。
  [804e8c9e](https://github.com/nakatamaho/vaeg/commit/804e8c9e361030a8ca46cdfe3866cbfdeb2330e9)。
- `sdl2/selftest.c`: 二つの 8255 を本体とディスクユニットのポートとして
  つないだ試験と、ヘッド 1 での Seek/Recalibrate の ST0 の試験を追加。
  それぞれ修正を外すと
  `a mode word did not reset the output latches`、
  `seek end ST0 carried the head address` で失敗する。

## 確認

- `vaeg --selftest`（ダミー SDL）PASS。
- `linux-ci-gcc` の CTest: 160 件中 159 件 PASS。失敗した
  `vaeg_m152_debug_script_parser`（`M152_PASTE_THEN_KEY`、キー入力が 31 フレーム目）
  は M155 の変更を外した状態でも 2 回とも失敗し、変更後は 1 回失敗・1 回成功した。
  M155 以前からのタイミング依存の失敗で、本マイルストーンでは扱わない。

## Gate

G155: 保守者が、このブランチの MinGW ビルドで、報告のプログラムを
PC-8801MH として起動し、動くことを確認する。
