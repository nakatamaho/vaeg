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

VA 主 CPU の互換モードと FDC CPU は、共通の Z80 互換バックエンド
（`cpu/z80_compat_cpu.*`、中身は同梱の suzukiplan コア）で動いている。
M139 は非 VA モデルの主 CPU 用に、さらに独立したインスタンスを追加した。
役割ごとに検査し、一方の期待値が保たれているだけで他方への変更を見逃さない。
ソフトウェアのモデル名や設定関数の共有は、実機の CPU 型番・挙動の実証とは区別する。

| 役割 | ハードウェア | コア設定 | 現在の挙動 |
|---|---|---|---|
| uPD70008 互換モード | `BRKEM` 後の uPD9002 メイン CPU（V1/V2 モード、CP/MVA） | `Upd9002Upd70008ConfigureCore`（`cpu/upd9002_upd70008_core_config.cpp`） | `kUpd9002` フラグプロファイル、実測した未定義命令の規則、未定義 `ED` ロード用の IVT 読み出し |
| FDC サブシステムの CPU | `vasubsys.rom` を実行する NEC uPD780C | `Upd780ConfigureCore`（`cpu/upd780/upd780_core_config.cpp`） | `kZilog` プロファイル |
| 非 VA 主 CPU（M139 のモデル上の役割） | 適用された PC-8801 / PC-8001 モデルの主 CPU。実機型番の測定ではない | 現在は `Upd780ConfigureCore` を共有する別インスタンス | `kZilog`、主 CPU 独自のバス・クロック・割り込み状態 |

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
| `vaeg_upd70008_zexall` | uPD70008 | 標準の ZEXALL、67グループ | 実機 VA2 のグループ CRC（`tests/z80_compat/upd9002_zexall_expected.txt`） | 全グループ一致 |
| `vaeg_upd70008_probes` | uPD70008 | M101/M102 のプローブプログラム | 実機 VA2 の出力ファイル | バイト単位の一致、先頭一致、レコード単位の一致（後述） |
| `vaeg_upd780_zexall` | uPD780C | 標準の ZEXALL | Zilog の標準 CRC | 全グループ OK |
| `vaeg_upd780_probes` | uPD780C | ZEX13S、ZEXIY、ZEXUND、ZEXED | Zilog の標準 CRC | 全グループ OK |
| `vaeg_upd9002_brkem_upd70008` | uPD70008 のアダプタ | 合成データ: 本番のブリッジ、裏レジスタの保存、互換モードの CPU 速度 | 資料と M103a の記録から書いた期待値 | 固定ケース |
| `vaeg_romless_tests`（`vaeg --selftest` の uPD780 部分） | uPD780C サブシステム | 合成データ: ステートの橋渡し、クロックとウェイト、HALT/IRQ の状態、IRQ アクノリッジ、スリープ経路、FDD 境界、EI の状態、交差配線した 8255 ポート | `tests/upd780/subsystem_integration.cpp` に書いた期待値 | 固定ケース |
| `vaeg_z80_compat_*` | 共通バックエンド | ラッパーの約束事、旧インターフェースの約束事、ステート形式、素のコアとのトレース差分、素のコアでの ZEX | 素のコアと、バックエンド自身の過去の出力（レベル 4） | 一貫性のみ |

ZEXDOC は実行しない。ZEXDOC と ZEXALL のソース（`external/zex/`）の違いは、各グループの
フラグマスク67箇所、期待 CRC、表題の文字列だけで、テストベクタは同一である。ZEXALL のマスクは
すべて `0FFh` なので、ZEXALL のグループ CRC が一致すれば、マスクを狭めた ZEXDOC の CRC も
（CRC の衝突を除いて）必ず一致する。実機の ZEXDOC 結果（`upd9002_zexdoc_expected.txt`）は
記録として残す。

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

外部レベルで検査しているもの: ZEXALL（ZEXDOC を含意）、ZEX13S、ZEXIY、ZEXUND、ZEXED での、設定した
バックエンドの Zilog 準拠（全グループ OK）と、`vaeg --selftest` による合成データでの
サブシステム結合テスト。

外部レベルで検査していないものと、その理由:

- 実際の uPD780C の挙動。測定がない。証拠を得るには、同じプローブプログラムを実機 VA の
  FDC 側 CPU で動かす必要があり、`vasubsys.rom` の環境で動くハーネスが要る。
- FDC ファームウェア（`vasubsys.rom`）そのものと、uPD765 やメイン CPU とのやりとり。NEC の
  ROM が必要なので、ローカルレベル A のみ。
- 実際のボード上での割り込み受け付けとタイミング、合成ケースを超える `HALT`/スリープ経路、
  サイクル数。

<a id="m139-fdc-upd780"></a>

## M140: FDC uPD780

この範囲は保守者の指示で M139（主CPU選択）から M140 へ分離した。
過去のコミット・結果・v1 プロトコルの `M139` 識別子は改変しない。
ZEX の再現可能な変更差分は `external/zex/patches/` に保存する。

M140 には uPD780 の実機 ZEXALL、差分の文書化と suzukiplan への反映、速度測定を追加する。
**今回の実機 uPD780 は FDC 側で測る。** 主 CPU での CP/MVA 実行や、非 VA 主 CPU の
モデル実行を uPD780 実測とは呼ばない。現在の `--role upd780` ランナーも、本番設定関数を
使うだけで、64 KiB の平坦なメモリと CP/M 代替環境を持つホスト上の検査である。
FDC の物理的なメモリマップ、ROM、8255、ウェイト、実時間を再現するハーネスではない。

### 実行環境と再配置の対照

現行 `io/subsystem.cpp` のモデルは、0000h–1FFFh が ROM、4000h–7FFFh が RAM。
これはソースで確認できるモデルの仕様であり、実機仕様の独立確認は別途必要である。
[M103i の RAM 転送調査](../../agents/tasks/M103i_v1_mode.md) は導入経路の手掛かりだが、
安全な起動・結果回収・通常 FDC サービスへの復帰まで確立したものではない。
既存のファームウェア作業域とスタックを無視して「16 KiB 全部が空き」としてはならない。
診断中はドライブ書き込みを行わず、ROM は変更しない。

標準 ZEXALL は低位メモリ、CP/M の入口・スタック、固定アドレスの `msbt` を使う。
RAM 常駐版では全生成ケースのアクセス先と自己書き換え・スタックの収まりを検査する。
アドレスが変わると、レジスタ値、非公開フラグ、CRC も変わり得る。
そのため **再配置版の CRC と標準版の CRC の違いを、そのまま NEC の差として扱わない**。

- 標準版 67 グループの基準結果を保存する。
- 実機とホストでは同一の FDC 用イメージを使用し、プログラム・比較契約を固定する。
- 標準版とのベクタ・マスク・グループ対応を示し、分割・未実行・変更した範囲を明記する。
- 再配置版の独立した Zilog 対照も確認する。比較対象を生成する suzukiplan 自身だけの
  一致を、ハードウェア一致や独立した正しさの根拠にはしない。
- 差を小さなプローブへ縮約し、初期状態と全結果を照合してから CPU 規則を決める。
  ZEX の GPL 通知を維持し、外部ソースを手編集したり、フラグマスクを弱めたりしない。

### 速度測定の契約

ZEXALL 総所要時間は、検査ルーチン・CRC 計算・出力も含むので、命令速度とは別に記録する。
命令用には、RAM 常駐の独立した反復カーネルを使う。

- 転送、ログ、CRC 集計、ディスク I/O は計測区間の外。開始・完了通知も同じ経路の
  空カーネルで校正し、単純な所要時間をすべて命令時間へ換算しない。
- 複数の反復数と繰り返しを使い、固定費と反復ごとの傾きを分ける。
  タイマーの分解能、ばらつき、誤差範囲を結果とともに記録する。
- レジスタ・ALU、メモリ、分岐の成立／不成立、スタック、プレフィックスを分ける。
  制御していない I/O や割り込みが混入したケースを独立分類する。
- 独立した基準タイマー／クロックを確定する。ホストの壁時計、エミュレータが数えた
  T-state、表示上の公称 MHz を実機計測値の代わりにしない。
- 主 CPU の H/S・倍率変更による観測や通信の固定費と、FDC の実行速度を区別する。
  RAM 上の計測から ROM フェッチやバス全体のタイミングまで保証しない。

実機向けの[転送・起動・回収・復元ツールと実行イメージ](../../../tools/hardware/upd780/README.md)
は実装済み。ROM-backed ソフトウェア統合で復元後のログ保存・既存ファイル保持・DIR 復帰を検査する。
RAM の退避後、診断域の未使用部分もゼロ初期化し、同一 BIN のホスト対照と初期メモリを揃える。
67 群の再配置 ZEX、NEG 専用スモーク、256 組の AF 観測、独立した速度カーネルを区別する。
速度は 60000×N（N=1/4/16）、2反復、空経路を含む67件。VRTC の両遷移は等間隔ではないため、
1件の計数位相誤差±2辺、中央点残差4辺、反復傾き差8/15辺という解析上限を使う。
CP/M の TVRAM 契約は F300h/120-byte rows であり、BASIC の F3C8h と混同しない。

**実機**での起動・復帰、物理基準タイマーの承認、CRC/速度測定は **未測定／BLOCKED**。
実機の CRC、速度値、NEC 差分の修正はまだ報告しない。
現状とホスト基準の進捗は [M140](../../agents/tasks/M140_fdc_upd780_conformance.md) に記録する。

### suzukiplan への反映

実機との差を示す最小ケースと回帰テストを付け、固定した上流 checkout で修正する。
`external/suzukiplan-z80/provenance.txt` と ADR-0011 の再現可能なパッチ手順に従い、
vendored コードを直接手編集しない。必要な NEC 規則はインスタンス単位とし、
Zilog と uPD9002 の既存結果を維持する。FDC の特性を、共有設定関数を通じて別の主 CPU
型番・リビジョンへ無条件に広げない。現段階では差が未測定なので、上流修正も未着手。

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
