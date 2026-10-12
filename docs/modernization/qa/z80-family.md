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

# Z80 互換 CPU の QA

2つのハードウェア上の役割が、共通の Z80 互換バックエンド（`cpu/z80_compat_cpu.*`、中身は
同梱の suzukiplan コア）で動いている。両者は別の CPU なので別々に検査する。一方の役割への
変更が、もう一方の期待値が保たれていることで見逃される、ということが起きないようにするためである。

| 役割 | ハードウェア | コア設定 | 現在の挙動 |
|---|---|---|---|
| uPD70008 互換モード | `BRKEM` 後の uPD9002 メイン CPU（V1/V2 モード、CP/MVA） | `Upd9002Upd70008ConfigureCore`（`cpu/upd9002_upd70008_core_config.cpp`） | `kUpd9002` フラグプロファイル、実測した未定義命令の規則、未定義 `ED` ロード用の IVT 読み出し |
| FDC サブシステムの CPU | `vasubsys.rom` を実行する NEC uPD780C | `Upd780ConfigureCore`（`cpu/upd780/upd780_core_config.cpp`） | `kZilog` プロファイル |

本番のコードと役割ごとの検査は、同じ設定関数を呼ぶ。3つのレベルと比較相手の優先順位は
[QA の概要](README.md) を参照。

## 1. 比較相手

| 役割 | 利用できる最上位の比較相手 | 出どころ |
|---|---|---|
| uPD70008 互換モード | 実機 PC-88VA2 の出力（レベル 1） | M101 の ZEXDOC/ZEXALL とプローブ（`docs/agents/reports/m101_zexall_qa/pc88va2/`）、M102 のプローブ（`docs/agents/reports/m102_zexund_qa/run*_pc88va2/`） |
| uPD780C | 実機の記録なし。Zilog Z80（レベル 3） | ZEX が持つ標準 CRC |

実機の uPD780C は一度も測定していない。したがって FDC 側 CPU の検査が示すのは、設定した
バックエンドが Zilog と一致することであって、uPD780C と一致することではない。NEC の NMOS
Z80 系は `SCF`/`CCF` の非公開ビット X/Y が Zilog と異なると報告されているが、uPD780C が
そうであるかは未検証である（`cpu/z80_compat_cpu.h`）。

## 2. 外部レベルの検査

| CTest | 役割 | 実行するもの | 比較相手 | 判定 |
|---|---|---|---|---|
| `vaeg_upd70008_zexdoc`、`vaeg_upd70008_zexall` | uPD70008 | 標準の ZEXDOC/ZEXALL、各67グループ | 実機 VA2 のグループ CRC（`tests/z80_compat/upd9002_zex*_expected.txt`） | 全グループ一致 |
| `vaeg_upd70008_probes` | uPD70008 | M101/M102 のプローブプログラム | 実機 VA2 の出力ファイル | バイト単位の一致、先頭一致、レコード単位の一致（後述） |
| `vaeg_upd780_zexdoc`、`vaeg_upd780_zexall` | uPD780C | 標準の ZEXDOC/ZEXALL | Zilog の標準 CRC | 全グループ OK |
| `vaeg_upd780_probes` | uPD780C | ZEX13S、ZEXIY、ZEXUND、ZEXED | Zilog の標準 CRC | 全グループ OK |
| `vaeg_upd9002_brkem_upd70008` | uPD70008 のアダプタ | 合成データ: 本番のブリッジ、裏レジスタの保存、互換モードの CPU 速度 | 資料と M103a の記録から書いた期待値 | 固定ケース |
| `vaeg_romless_tests`（`vaeg --selftest` の uPD780 部分） | uPD780C サブシステム | 合成データ: ステートの橋渡し、クロックとウェイト、HALT/IRQ の状態、IRQ アクノリッジ、スリープ経路、FDD 境界、EI の状態、交差配線した 8255 ポート | `tests/upd780/subsystem_integration.cpp` に書いた期待値 | 固定ケース |
| `vaeg_z80_compat_*` | 共通バックエンド | ラッパーの約束事、旧インターフェースの約束事、ステート形式、素のコアとのトレース差分、素のコアでの ZEX | 素のコアと、バックエンド自身の過去の出力（レベル 4） | 一貫性のみ |

役割ごとの検査は `compat-conformance` ジョブで動く。プローブプログラムは z80asm 1.8 で
ビルドし、z80asm がなければプローブの検査はスキップを報告する。

## 3. uPD70008 互換モード: Zilog と異なる点と、その検査方法

このモードは Z80 の上位互換（μPD780 の命令セットに `CALLN`/`RETEM` を加えたもの）であって
8080 モードではない（`docs/modernization/upd9002-upd70008-mode.md` §2.4）。ただし、フラグの
計算と未定義命令の領域が Zilog Z80 と異なる。規則 R1〜R19 の定義は
`docs/modernization/uPD9002-zex-results.md` §4 にある。

| 規則 | 挙動（要約） | 実機 VA2 との照合に使うもの |
|---|---|---|
| R1 | F のビット5/3は常に0として読め、保存もできない | ZEXALL の CRC、FLAGPRB P7（完全一致） |
| R2〜R3 | AND と BIT は H をクリアする | ZEXALL の CRC、FLAGPRB P1/P2 |
| R4 | RLCA/RRCA/RLA/RRA は A と C だけを変える | ZEXALL の CRC、FLAGPRB P3 |
| R5 | LDI/LDD のブロック転送のフラグ | ZEXALL の CRC、FLAGPRB P4 |
| R6〜R7 | 16ビット ADD/ADC/SBC の H の規則 | ZEXALL の CRC（ZEXDOC はグループ1〜4で H を隠す）、FLAGPRB P5/P6 |
| R8〜R11 | CPL/SCF/CCF/DAA | DAA/CPL/SCF/CCF の全入力ダンプ（4種 × 16384レコード、完全一致）、DAADUMP、ZEX13S |
| R12〜R13 | レジスタオペランドの `DD CB`/`FD CB`: BIT はレジスタを調べ、RES/SET はレジスタだけを変える | CBPRB、全1024通り × 入力2組（run 2 と run 3、完全一致） |
| R14 | 未定義 `ED 00`〜`3F`、`74`/`75`/`77` がネイティブ側のページ0からワードを読む | EDPRB3（1変数ずつ変化）、EDPRB4（`C` の全256値）、ED00/E200 の先頭一致、e264 のレコード |
| R15 | `ED 4C` は `RETN` の重複 | ED00/E200 の先頭一致 |
| R16 | `ED 64`/`65`、`6C`/`6D` の RRD/RLD は `(HL+1)` にも書く | S200（完全一致）、e264 のレコード |
| R17〜R18 | `ED 7C`〜`A7` の1回分と、`AD`〜`AF`/`B4`〜`B7` の繰り返しのワード転送 | S200、e264 のレコード |
| R19 | `ED BC`〜`EC` はオペランドを1バイト消費し、目に見える効果はない | e264 のレコード |
| IY の半分 | `FD 24/25/2C/2D` は IX の半分と同じに振る舞う | ZEXIY（完全一致） |

R14 の検査では、実機 run 8 から再構成した IVT ページを CP/M ランナーが役割設定に渡す。
エミュレータ上では同じ命令がゲストの実際のページ0を読むので、両者が一致するのは、ゲストが
該当するベクタを書き換えていない間だけである（run 10 では RAM 上にフックされた3つのベクタが
異なった）。

### ZEXDOC/ZEXALL だけでは確かめられないこと

- グループ CRC が覆うのは ZEX が実行する状態空間だけで、その外については何も言えない
  （`uPD9002-zex-results.md` C5）。
- ZEXDOC のマスクは、グループ1〜4で H を、BIT のグループで S と P/V を隠す（C1）。
- グループ29〜31は Y/X について縮退している（C2）。F がビット5/3を立てた状態で入ることが
  ないので、これらのビットの保存は検査されない（C4）。
- `iyh`/`iyl` のグループは実際には `DD` プレフィックスの命令を実行しており、本物の IY の
  半分は検査されない（C3）。
- 非公開のレジスタ形式の `DD CB`/`FD CB` も、未定義の `ED` 命令も実行しない。

これらの抜けは、上の表のプローブがそれぞれ実機 VA2 の出力に対して埋めている。

## 4. uPD70008 互換モード: 外部レベルで検査していないもの

| 対象 | 理由 | 検査されている場所 |
|---|---|---|
| 未定義 `ED 54`/`55`、`AC`、`EE`〜`FC`（互換モードを抜ける）、`5C`（停止）、`5D`（異常） | 症状しか観察されておらず、vaeg もエミュレートしていない | なし |
| 入口で `BC = 0` のときの `ED` ワード転送 | 測定していない。vaeg は `LDIR` と同様に65536ワードを転送する | なし |
| `IM` の重複（`ED 4E/66/6E/76/7E`）と `LD A,R` | 設計上プローブから外している | 実機との照合なし |
| `IN`/`OUT` 系 | CP/M ランナーは I/O を拒否する。INPRB は実機専用 | ローカルレベル A のみ |
| モード遷移の `BRKEM`/`BRKEM2`、`CALLN`、`RETEM`、互換モード中に受け付けた割り込みのネイティブ側での処理、`RETI` での復帰 | バックエンドではなく、アダプタとネイティブ側の担当 | `vaeg_upd9002_brkem_upd70008`（合成データ）、ローカルレベル A |
| `CALLN`/`RETI` のキャリーの問題（`upd9002-upd70008-mode.md` §3.2） | 実機で未解決 | 未解決 |
| モードをまたぐ HALT/スタンバイの規則（§3.5） | 機械からの割り込みが必要 | ローカルレベル A |
| 互換モードでの Z80 割り込みモード `IM 0/1/2` | 実機で `[UNKNOWN]`（§3.5） | 未解決 |
| セグメントベース（コード/データの変換）と実際の IVT | アダプタのつなぎ込み部分 | `vaeg_upd9002_brkem_upd70008`、ローカルレベル A |
| サイクル数とバスタイミング | 実機のタイミング記録がない | なし。`docs/modernization/z80-compat-cycle-deltas.md` はバックエンドと素のコアの差分だけを記録している |
| ZEXUND と ZEXED の完全な結果 | 実機での実行がファイルを閉じなかった（画面写真のみ） | 未解決 |

## 5. uPD780C: 検査しているものと、していないもの

外部レベルで検査しているもの: ZEXDOC、ZEXALL、ZEX13S、ZEXIY、ZEXUND、ZEXED での、設定した
バックエンドの Zilog 準拠（全グループ OK）と、`vaeg --selftest` による合成データでの
サブシステム結合テスト。

外部レベルで検査していないものと、その理由:

- 実際の uPD780C の挙動。測定がない。証拠を得るには、同じプローブプログラムを実機 VA の
  FDC 側 CPU で動かす必要があり、`vasubsys.rom` の環境で動くハーネスが要る。
- FDC ファームウェア（`vasubsys.rom`）そのものと、uPD765 やメイン CPU とのやりとり。NEC の
  ROM が必要なので、ローカルレベル A のみ。
- 実際のボード上での割り込み受け付けとタイミング、合成ケースを超える `HALT`/スリープ経路、
  サイクル数。

## 6. 結果の読み方

- `vaeg_upd70008_*` の合格が意味するのは、列挙したすべてのプログラムとレコードについて、
  エミュレートした uPD70008 互換モードが実機 PC-88VA2 の出力を再現する、ということである。
  第4節の内容は含まない。
- `vaeg_upd780_*` の合格が意味するのは、実行した状態空間で FDC 側 CPU が Zilog Z80 として
  振る舞う、ということである。uPD780C を検証したことにはならない。
- 2つの役割の照合リストを入れ替えると両方のプローブ検査が失敗するので、検査は2つの役割を
  区別できている。

更新手順とファイル形式: `tools/qa/z80_role_probes.py`（照合リスト
`tests/z80_compat/upd70008_probes.json`、`upd780_probes.json`）と
`tests/z80_compat/*_zex*_expected.txt`。
