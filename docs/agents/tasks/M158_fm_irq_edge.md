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

# M158 — 音源割り込みが、フラグの残ったタイマーで再要求される

Status: **IN PROGRESS; awaiting G158.**
Branch: `milestone/m158-stall-case`（origin/main
[a913388f](https://github.com/nakatamaho/vaeg/commit/a913388fde308347394aaebec2648481f9c8b948) から）;
commit prefix `M158:`.

## 目標

保守者の報告: あるプログラムが、タイトルでスペースを押してゲームに入ると、
画面がそのまま止まる（M155 より前のビルドでも同じ）。原因を特定して直す。

## 原因

- そのプログラムは、曲の処理を OPN のタイマー B による音源割り込み
  （8214 のレベル 4）で動かす。ゲームに入ると、BASIC ROM のディスク
  ルーチンでステージのデータを読み、曲のデータ領域にコピーする。読み込みの
  間は、割り込みの処理は ROM のものになり、曲の処理は動かない。タイマー B の
  フラグは立ったまま残る。
- 読み込みを終えると、プログラムは E6h=02h、E4h=05h で割り込みを許可し、
  しばらく後に新しい曲をセットする。
- OPN の /IRQ はレベル出力で、フラグが立っている間は出たままになる。
  フラグを消すのはレジスタ 27h のリセットビットだけである。8214 の要求は
  ラッチされ、受け付けで消える。そのため、フラグを残したままタイマーが
  何度溢れても、新しい要求にはならない。
- vaeg の `sound/fmtimer.c` は、フラグが立ったままでも、タイマーが溢れる
  たびに要求を出していた。そのため、許可した直後に音源割り込みが届き、
  曲の処理が、上書きされた領域の古い位置を読んだ。範囲外の表を引いて
  0 番地へ飛び、割り込み禁止のまま止まっていた。

## 修正

- `sound/fmtimer.c` `fmport_a`/`fmport_b`: /IRQ が出ていない状態から出る
  状態に変わるときだけ要求する。SINTM の解除時に /IRQ が出ていれば要求し
  直す `fmboard.c` の既存の処理は変えない。
  [70ffab6c](https://github.com/nakatamaho/vaeg/commit/70ffab6cbade0224f47a34de0395727546fb9e6f)。
- `sdl2/selftest.c`: FM IRQ edge 試験を追加した（フラグが残ったままの
  溢れ、片方だけのリセット、リセット後の再要求）。修正を外すと
  `M158_FM_IRQ_HELD` で失敗する。N80 FM mask 試験は、2 回目の溢れの前に
  フラグをリセットするように直した（この試験の目的は、マスクし直しても
  ラッチ済みの要求が消えないことの確認）。

## 確認

- `vaeg --selftest`（ダミー SDL）PASS。
- `tools/repo/check_case.py`、`check_encoding.py`、`check_eol.py`、
  `check_md_comments.py` PASS。

## Gate

G158: 保守者が、このブランチの MinGW ビルドで、報告のプログラムを VA と
PC-8801MA2 で起動し、スペースでゲームに入って止まらないことを確認する。
