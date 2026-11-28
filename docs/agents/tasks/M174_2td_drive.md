<!--
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.
-->

# M174 — PC-88VA3 の 2TD（9.3M バイト）ドライブ

Status: **IN PROGRESS（ROM 解析済み、実装前）.**
Branch: `milestone/m174-2td`（origin/main
[d0ef1ea1](https://github.com/nakatamaho/vaeg/commit/d0ef1ea11c9987d50eef775808d5ebd2642bd362) から）;
commit prefix `M174:`.

## 目標

保守者の指示（2026-11-28）: PC-88VA3 の 2TD ドライブに対応する。VA3 の
実機は無いので、ハードウェアは ROM のドライバから組み立てる（想像を含む）。

VA2 と VA3 は同じ ROM を使い、2TD の BIOS（INT B9H、ROM 内の呼び名
BD0/BD1）は VA2 の ROM に入っている。解析は
[INT B9H 解析資料](../../bios/bios_intb9h.md) にまとめた。vaeg は BIOS を
作らず、ROM のドライバが期待するハードウェアを模擬する。

保守者が決めたこと:

- 2TD ドライブで 3.5 インチ 2DD/2HD を読む機能は後の milestone に回す。
- イメージ形式は拡張 D88（480 トラックを持てるように広げる）。
- ドライブは 2 台まで対応する。

## 範囲

1. **2TD コントローラ**（新しい C99 のコア、`io/` 配下）
   - ROM のドライバの 2TD モードのコマンド（20H, 30H, 40H, 58H, 68H, 90H,
     B0H/C0H, F0H/E0H）、パケットの受け渡し（1E4H/1E6H）、割り込み、
     DMA（VA の μPD71071）、制御・ドライブ・状態ポート、有無の判定
     （1E0H/1F0H が 00H）、書込み禁止、媒体交換ラッチ。
   - コントローラは 2 つまで: 1F0H 側（IR3, DMA3、内蔵とみる）と
     1E0H 側（IR5, DMA1、外付け）。1 つのコントローラにドライブ 2 台まで。
   - 2DD/2HD の互換モード（μPD765 の手順）は今回は作らない。そのモードに
     切り替えられたら、ドライブはノットレディを返す。
   - フォーマットのコマンドは作らない（ROM にも無い）。
2. **拡張 D88**
   - 480 トラックを持てるようにトラック表を広げ、2TD の媒体の値を足す。
     既存の D88 の読み書きと区別できる形にする。形式は
     `docs/modernization/` に文書化する。
   - 新しいディスクは「セクタ ID だけがあり中身が空」（工場出荷の状態）で
     作れるようにする。使えるようにするのはゲストの FORMAT（ユーザーズ
     ガイドの手順）。
3. **設定と UI**
   - 2TD のドライブ数（0/1/2）と、2 台のときのつなぎ方の設定。
     メモリスイッチ B1FC7H の bit2-1 をこの設定に合わせる（起動ディスクの
     ビットと同じ同期のしかた）。
   - メディアメニューに BD0/BD1、コマンドラインに 2TD のイメージ指定。
   - 新しい 2TD イメージを作るメニュー。
   - 機種の選択肢は増やさない（VA3 = VA2 + 2TD）。

## 推測で作る部分

ROM のドライバが見ないビット、コマンド 20H の引数の意味、90H の結果、
タイミング（1300KBPS、360RPM を目安）、モード切替えの待ち時間、
物理的な ID の並び。これらは文書に「推測」と明記する。

## 決めること（保守者）

- 2 台のときのつなぎ方の既定:
  (a) コントローラ 2 つ（1E0H 側が BD0 で起動に使われる、1F0H 側が BD1）、
  (b) 1F0H 側のコントローラにドライブ 2 台（BD0 = 内蔵側ドライブ 0）。

## 確認

- 単体テスト: コントローラのコマンド、パケット、割り込み、DMA、
  書込み禁止、媒体交換、拡張 D88 の読み書きと往復、範囲外の扱い。
  失敗時は安定したエラーコードで判定する。
- ROM の設定画面で「大容量FDのドライブ数」を 1 台・2 台に変えられる。
- PC-Engine 1.1 で `FORMAT C:` が 9297920 バイトで終わり、`SYS C:` 後に
  2TD から起動して 2TD が A: になる（ユーザーズガイドの手順）。
  2 台のとき両方のドライブで読み書きできる。
- 自作の MS-DOS 4.0 用プログラム（合成データのみ）で INT B9H の読み書きを
  行い、バイト単位で一致する。
- 既存の FDD・SASI・SCSI の動作が変わらない。

## Gate

G174: 保守者が MinGW ビルドで、VA2 の機種に 2TD ドライブをつなぎ、
FORMAT・SYS・2TD からの起動・2 台での読み書きを確認する。
