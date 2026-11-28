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

# PC-88VA BIOS解析資料 — INT B9H / 大容量フロッピーディスク（2TD）BIOS

対象: VA2 ROM（VA3と共通）、テクマニ第2章・5.2、公開資料
版: 静的解析 第1版（M174）

## 0. この資料の位置づけ

INT B9HはテクマニとBNN資料に載っていない割り込みで、PC-88VA3の
3.5インチ2TD（9.3Mバイト）ドライブ（ROM内の呼び名はBD0/BD1）を扱う。
VA2とVA3は同じROMを使う（N88-BASIC V3はどちらも3.1）ため、
このBIOSはVA2のROMに入っている。VAのROMには無い。

[ROM確認] は実際のバイト列・到達可能な分岐から確認した事項。
[文書仕様] は公開資料・マニュアルの記載。
[推定/未確認] は意味や範囲を確定できていない事項。
実機での測定はしていない。

ROMの番地は「ROM0 bankN:offset」（E0000H窓、ポート152Hで選ぶ）と、
システムROMの「F000:offset」で示す。

## 1. 公開資料による2TDの仕様

[文書仕様] Oh!88 No.3 p.22 表1、PC-88VA2/3ユーザーズガイド pp.30-31:

| 項目 | 値 |
| --- | --- |
| 容量 | アンフォーマット12.5Mバイト、フォーマット時9.3Mバイト |
| トラック | 480（240シリンダ×2面）、406TPI |
| セクタ | 512バイト×38セクタ/トラック、MFM |
| 転送速度・回転数 | 1300KBPS、360RPM |
| 追従 | サーボ情報によるトラッキング |
| 互換 | 3.5インチ2DD/2HDは読めるが書けない |
| FORMAT C: の表示 | 全ディスク容量 9297920 バイト |

ユーザーズガイドでは、5.25インチのシステムディスクから起動すると
2TDがC:になる。FORMAT C: と SYS C: で作った2TDから起動すると、
2TDがA:、ドライブ1がB:、ドライブ2がC:になる。

## 2. 入口

[ROM確認] システムROMの割り込みベクタ表（F000:0DD9、3バイト単位で
ベクタ番号と入口offset）に次の登録がある。入口はAL=ROM0 bank、
BP=E000H、DX=offsetを積んで 0040:2F10 のbank切替え中継へ飛ぶ。

| ベクタ | 入口 | 行き先 | 役割 |
| --- | --- | --- | --- |
| B9H | F000:0475 | bank5 E000:C2D3 | 2TD BIOSサービス |
| 0BH | F000:038C | bank5 E000:C2D9 | IR3割り込み（1F0H側コントローラ） |
| 0DH | F000:039A | bank5 E000:C2D6 | IR5割り込み（1E0H側コントローラ） |

初期化 bank5 E000:C2D0 はシステムROMの起動処理から呼ばれ、
0040:3F0F〜のワークを消し、0040:0CC8 にフックを置き、
0040:3F51 の6個のfarフックを空処理（RETF）にする。

[ROM確認] 同じ表には B8H〜BFH の他の非公開割り込み（BAH, BBH, BDH,
BFH）もあるが、2TDとは関係しない。

## 3. 呼び出し規約

[ROM確認] サービス入口（bank5 C328）はAHのbit7をマスクして機能番号
00H〜0BHを選び、0CH以上はAH=02H（パラメータエラー）。結果はAHと
CFで返す。処理中は 0040:0068 のbit2を立てる。AH=0EH/0FHの終了では
回復処理（C620）を呼ぶ。

| レジスタ | 内容 |
| --- | --- |
| AH | 機能番号、bit7=1でリトライなし |
| AL | セクタ数（READ/WRITE）、ディスクモード（09H） |
| BH | ドライブ（0=BD0, 1=BD1） |
| CX | 論理トラック番号（2TDは0〜479） |
| DH | 開始セクタ |
| DL | セクタ長コード（2DD/2HD） |
| ES:BP | バッファ |

## 4. 機能

| AH | 名称（解析上） | 内容 |
| --- | --- | --- |
| 00H | INITIALIZE | メモリスイッチ B1FC7H のbit2-1でドライブ数（1=1台、2=2台、他=無し）。コントローラ検出、ワーク初期化 |
| 01H | READ DATA | 2TDモードはコマンドB0H（0040:3EE4 bit0でC0H） |
| 02H | WRITE DATA | コマンドF0H、0040:3EE4 bit1でE0H（書込み後ベリファイ）。2DD/2HDモードでは常にエラー02H。書込み禁止はエラー05H |
| 03H | （FORMAT TRACK相当） | 常にエラー02H。物理フォーマット機能は無い |
| 04H | READ ID | 2TDはコマンド90H、ID（C,H,R,N）をDMAでバッファへ。2DD/2HDはμPD765の4AH |
| 05H | RECALIBRATE | 2TDは58H、2DD/2HDはμPD765の07H |
| 06H | SENSE | 2TDは30H（結果をAL）、2DD/2HDはSENSE DRIVE STATUS |
| 07H | CHECK READY | SENSE結果のbit1（2DD/2HDはST3のbit5） |
| 08H | CHECK MEDIA CHANGE | ドライブ状態ポートのbit1（交換ラッチ）。立っていればRECALIBRATEし、ドライブ制御ポートのbit1をパルスしてCF=1 |
| 09H | SET DISK MODE | AL=0:2DD、1:2HD、2:2TD（3以上はエラー）。0040:3EE0+ドライブへ格納。初期値FFHは2TDとして扱う |
| 0AH | RECOVER WORK | 3EE4=00H、3EE5=08H（リトライ8回）、3EE6=1EH |
| 0BH | GET BOOK | DS:DX = 0040:3EE0（ワーク先頭）を返す |

## 5. ハードウェア（ドライバから読み取ったもの）

[ROM確認] ドライバはコントローラ記述子（20バイト）を2つ持つ。

| 記述子 | 有効フラグ | ポート | IRQ / ベクタ | DMA |
| --- | --- | --- | --- | --- |
| bank5 D890 | 00H | 1E0H〜1EEH（偶数） | IR5 / 0DH | ch1 |
| bank5 D8A4 | FFH | 1F0H〜1FEH（偶数） | IR3 / 0BH | ch3 |

[文書仕様] テクマニ5.2ではIR3=UINT0（Bus #24）、IR5=UINT1（Bus #25）。
公開されている拡張記事では「DMA3=2TD、DMA1=未使用」とあり、
内蔵2TDは1F0H側と推定する。同記事のINT番号表記（INT0=外部、
INT1=内部）とは対応が逆で、表記の流儀は[未確認]。

ポート（1E0H側。1F0H側も同じ並び）:

| ポート | 役割 |
| --- | --- |
| 1E0H | コントローラの有無。00Hなら有り |
| 1E2H | 制御。bit7-6=モード、bit5=書込みストローブ、ドライブ初期化でbit1とbit(2+ドライブ)をパルス。読み出しbit7で割り込み処理の方式を選ぶ |
| 1E4H | 2DD/2HD: μPD765メインステータス。2TD: 引数の書込みと結果の読出し |
| 1E6H | 2DD/2HD: μPD765データ。2TD: ステータス読出し（bit7=使用中）とコマンド書込み |
| 1E8H / 1EAH | ドライブ0/1の制御。bit7-6=媒体（2TD=00H、2HD=C0H、2DD=80H）、bit1=交換ラッチのクリア、bit0はノットレディで落とす |
| 1ECH / 1EEH | ドライブ0/1の状態。bit0=書込み禁止、bit1=媒体交換 |

コントローラの有無の判定（bank5 C5EA）: 1E0Hと1F0Hの両方が00Hなら
BD0=1E0H側ドライブ0、BD1=1F0H側ドライブ0。片方だけなら、その
コントローラのドライブ0/1がBD0/BD1。どちらも無ければエラー。

モードbit7が変わるとき、ドライバはタイマ（0040:0CBF）で48H単位待つ。
2TDと互換モードの切り替えに時間がかかるとみられる[推定]。

DMAはVA本体のμPD71071（160H〜16FH）。モード04H（I/O→メモリ）
で読み込み、08H（メモリ→I/O）で書き込み。

### 5.1 2DD/2HD（互換モード）

[ROM確認] μPD765の手順。SPECIFYは 03 BF 14（2DD）、03 7F 28（2HD）。
割り込みではSENSE INTERRUPT STATUS（08H）を発行し、ST0のNRで
ドライブ制御ポートのbit0を落とす。

### 5.2 2TDモードのコマンド

[ROM確認] パケット（0040:3F29）: [0]フラグ、[1]コマンドと引数の
バイト数n、[2]結果のバイト数、[3]コマンド、[4..]引数。
送出（bank5 D534）: 1E6Hのbit7が0になるのを待ち、引数n-1バイトを
1E4Hへ書いてから、コマンドを1E6Hへ書く。完了は割り込みで知らせ、
1E6Hから最初の結果（bit6-5の種別、正常は40H）、残りを1E4Hから読み、
1E6Hへ08Hを書いて受け取りを知らせる。コマンドの下位ビットはドライブ。

| コマンド | 引数→結果 | 内容（解析上） |
| --- | --- | --- |
| 20H | 8→1 | 初期化時の設定・検出。引数 5F 82 00 01 26 0C 00 C8 [意味は未確認] |
| 30H | 0→2 | ドライブ状態 |
| 40H | 0→6 | 転送状態（2〜3バイト目が残り数、ビッグエンディアン） |
| 58H | 0→2 | RECALIBRATE |
| 68H | 2→2 | SEEK |
| 90H | 2→3 | READ ID（IDはDMA） |
| B0H / C0H | 6→8 | READ（引数 H, FEH, C, H, R, セクタ数） |
| F0H / E0H | 6→8 | WRITE / 書込み後ベリファイ |

フォーマットのコマンドは見当たらない。2TDモードのセクタ長は512バイト固定。

### 5.3 媒体パラメータ表

[ROM確認] bank5 D8B8（最終論理トラック（ワード）、EOT、GPL、DTL）:

| モード | 最終トラック | セクタ/トラック |
| --- | --- | --- |
| 0: 2DD | 159 | 16 / 9 / 5（N=1/2/3） |
| 1: 2HD | 159 | 26 / 15 / 8 |
| 2: 2TD | 479 | 38 |

## 6. ワーク

[ROM確認] 0040:3EE0 からの主な領域（解析上の役割）:

| 0040内offset | 内容 |
| --- | --- |
| 3EE0 / 3EE1 | BD0/BD1のディスクモード（初期値FFH） |
| 3EE4 | 読み書きの修飾（bit0=読込み変種、bit1=書込み後ベリファイ） |
| 3EE5 | リトライ回数（8） |
| 3EE6 | 1EH |
| 3F08 | 有効ドライブのビット |
| 3F15 / 3F17 | BD0/BD1の記述子 |
| 3F19 / 3F1A | BD0/BD1のコントローラ内ドライブ番号 |
| 3F29〜 | 2TDコマンドパケット |
| 3F51〜 | 6個の拡張farフック |

## 7. FORMATと2TDディスクの配置

[ROM確認] ROM内蔵FORMAT（bank2 DC80〜DFB0）はBDに対して
物理フォーマットをしない。2TDモードを設定し、トラック0セクタ1へ
ブートセクタを書いて読み戻し、全トラックを読んで検査し（不良は
クラスタ単位で記録）、FAT2組とルートディレクトリを書く。
[文書仕様] ユーザーズガイドでも、VA3添付の3.5インチデータディスクは
「このままでは使えない」とし、FORMAT C: で使えるようにする。

FORMATの表（bank2 DE09）による2TDディスクの配置:

| 項目 | 値 |
| --- | --- |
| メディアバイト | E9H |
| 論理セクタ | トラック×38＋セクタ−1、全18240セクタ |
| ブートセクタ | 論理セクタ0 |
| FAT | FAT12、7セクタ×2組（1〜7、8〜14） |
| ルートディレクトリ | 15〜75（61セクタ、976エントリ） |
| データ領域 | 76から、1クラスタ8セクタ（4Kバイト）、最大クラスタ番号08DFH |

データ領域は2270クラスタ×4096バイト＝9297920バイトで、
ユーザーズガイドのFORMAT表示と一致する。

## 8. 設定画面

[ROM確認] PCキーで入る初期設定1（ROM0 bank6）に
「大容量FDのドライブ数：なし／１台／２台」があり、説明は
「Ｖ３モードで使用する大容量フロッピーディスクの数を設定します
（Ｖ３のみ）」。値はメモリスイッチ B1FC7H のbit2-1に入り、INT B9H
の初期化が読む。設定画面も1E0H/1F0Hを読んでいる。コントローラが
無い構成ではこの項目は変更できない[推定]。

## 9. 未確認

- 2TDコマンド20Hの引数、90Hの詳細、使われない結果ビットの意味。
- 2TDコントローラの実際のLSIとタイミング。
- システムROMのF000:8C00〜9300（DOS用BDドライバ）と
  F000:216A〜2304（2TDからの起動とみられる）の詳細。
- 公開記事のINT番号表記とROMの割り当ての対応。
