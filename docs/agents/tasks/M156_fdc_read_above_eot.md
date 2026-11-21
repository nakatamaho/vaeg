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

# M156 — EOT より大きい R から始まる複数セクタ読み込み

Status: **IN_PROGRESS; G156 NOT_READY.**
Branch: `milestone/m156-boot-case`（origin/main
[a5a362c5](https://github.com/nakatamaho/vaeg/commit/a5a362c59eb4ceb10f1d1e8507756497ae92170f) から）;
commit prefix `M156:`.

## 目標

保守者の報告: あるプログラムが、タイトルで Enter を押すと音は鳴るが中央の
プレイ画面が描かれず、キーも効かない。原因を調べて直す。

## 原因

- そのプログラムはディスクユニットに自前の読み込みコードを置き、1 トラック
  分（N=3 の 5 セクタ）を `READ DATA`（MT/MF）で読む。セクタ番号は
  11h〜15h で、コマンドの EOT は 05h。読む量はバイト数で数え、数え終わると
  TC で止める。
- vaeg の `inc_fdcR` は R を増やした後に `R > EOT` でトラックの終わりと
  していたため、R=11h から始まる読み込みは 1 セクタ目で終わり、ヘッド 1 では
  EN、ヘッド 0 では MT でヘッド 1 のセクタ 1 に移って No Data を返した。
- その結果、読み込んだデータが本体のコードの上に置かれ、Enter の後に
  本体が 0 番地付近へ飛んで暴走していた。M155 の変更前から同じで、M155 による
  退行ではない。

## 修正

- `io/fdc.c` `inc_fdcR`: 直前に読んだセクタの R が EOT と等しいときに
  トラックの終わりとする（uPD765、MAME upd765 と同じ）。R が EOT 以下から
  始まる読み込みの結果は変わらない。
  [22a7cdbc](https://github.com/nakatamaho/vaeg/commit/22a7cdbc9d4bc0e6c766d0220ab448d4e9020298)。
- `sdl2/selftest.c`: R=2、EOT=1 で 2 セクタを読んでから TC で止める試験を
  追加（修正を外すと `a read starting above EOT ended after one sector` で
  失敗）。

## 確認

- `vaeg --selftest`（ダミー SDL）PASS。

## Gate

G156: 保守者が、このブランチの MinGW ビルドで、報告のプログラムのプレイ画面が
描かれ、キー操作できることを確認する。
