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

# PC-88VA解析資料 — INT 91H / 16Bit memory・I/Oアクセス

V1/V2スーパーバイザ用のnative accessサービス。テクマニ第2章の分類、
BNN CPU節のCALLN仕様、VA/VA2 ROM、PC-Engine 1.1を静的照合した初版。
[索引](index.md)、[INT90Hとmode境界](bios_int90h.md)も参照。
新たなguest実行、I/Oアクセス、ROM変更は行っていない。

## 1. 呼び出しとレジスタ契約

**文書仕様**: V1/V2のメインCPU・uPD70008互換modeから
`CALLN 91H`（encoding `ED ED 91`）でnativeへ移り、native handlerのIRETで戻る。
Z80系命令だけでは表せないnative segment付きメモリと16-bit I/O addressにアクセスする。
FDC CPU/uPD780C向けのサービスではない。
通常のAH別BIOSやINT21HのDOS APIとは呼び出し契約が違う。

「16Bit」は転送幅が常にwordという意味ではない。
メモリはHL=16-bit segmentとDE=16-bit offset、I/OはDE=16-bit port。
転送幅はA bit0でbyte/wordを別に選ぶ。

| compatible register | 文書上の入力/出力 | native handlerでの対応 |
|---|---|---|
| A | 00H～07Hの機能番号 | ALをzero-extendしたBXでtable選択 |
| HL | メモリのsegment、I/Oでは使用しない | 入力BXをDSへ設定 |
| DE | メモリoffset / I/O port | DX、メモリ時はhelperでBXへcopy |
| BC | 書込/OUT値、読込/IN結果 | CX→AX、読取系でAX→保存CX |

例: HL=0040H、DE=00C2Hならnative addressは0040:00C2、物理計算上004C2H。
実際のROM/RAM overlay、銀行切替、書込可否は機械のmemory設定に依存する。

| Aのbit | 0 | 1 |
|---|---|---|
| bit2 | memory | I/O |
| bit1 | read / IN | write / OUT |
| bit0 | byte | word |

**ROM確認**: 上位bitsをANDで捨てたり、00H～07Hか検査したりする処理はない。
Aをそのまま2倍して8-word表を引く。上位bitsをflagとして混ぜてはいけない。
不正番号は表外のcode bytesをpointerとして使用し得る。安全なエラー戻りは未確認。

## 2. 個別登録と8機能

[INT90H](bios_int90h.md)と同じ6-record個別IVT表が91Hを登録する。
VAは表097DH・初期化0B51H、VA2は表0F5EH・初期化13EDH。
91H×4=0244HなのでIVTのoffset/segmentは0000:0244 / 0246。

| 対象 | 91H登録値 | 内部8-word表 |
|---|---|---|
| VA | F000:1640 | F000:1669 |
| VA2 | F000:24B0 | F000:24D9 |

以下は**文書仕様とROM確認を別列**にしたもの。

| A | 文書仕様 | VA helper | VA2 helper | 調査ROMの処理 |
|---|---|---|---|---|
| 00H | memory byte read | F000:1679 | F000:24E9 | C←byte、B維持 |
| 01H | memory word read | F000:167F | F000:24EF | **byte書込、BC維持（文書と不一致）** |
| 02H | memory byte write | F000:1685 | F000:24F5 | byte書込、BC維持 |
| 03H | memory word write | F000:168A | F000:24FA | word書込、BC維持 |
| 04H | I/O byte IN | F000:168F | F000:24FF | C←byte、B維持 |
| 05H | I/O word IN | F000:1692 | F000:2502 | BC←word |
| 06H | I/O byte OUT | F000:1695 | F000:2505 | byte出力、BC維持 |
| 07H | I/O word OUT | F000:1697 | F000:2507 | word出力、BC維持 |

byte read/INはnative ALだけを更新するため、BCの上位Bは入力値を維持する。
BC全体をzero-extendされたbyte値だと思って扱わない。byte write/OUTはCを使う。
word write/OUTはBC全体を使い、native AXのAL=C、AH=Bとして処理する。

I/O helperはそれぞれnative `IN AL,DX` / `IN AX,DX` /
`OUT DX,AL` / `OUT DX,AX`。88互換の8-bit port範囲へマスクする処理はない。
これはINT90HのCRTC/DMA等のsoftware変換とは別のnative I/O経路。
I/O readも装置状態を消費し得るため、無害な観測APIとは限らない。
word I/Oのbus分割、hardware trap、割り込みタイミングは今回未実測。

## 3. 重要: A=01Hは資料どおりのword readではない

**静的に確定した相違**:

- BNNのbit定義では01H=memory/read/word。
- VAのtable第1項は167FH、VA2は24EFH。
- そのhelperはDXをBXへcopyした後、VA:1681H / VA2:24F1Hで
  bytes `88 07` を実行する。これはnative `MOV [BX],AL`、**byte store**。
- 続くSTCは結果copyの内部マーカー。そのAXは入力BCのままなので、戻りBCも入力のまま。
- word load `MOV AX,[BX]` ではなく、DE+1から上位byteを読む処理もない。

すなわち、この初期登録handlerに入ればHL:DEへCを書き込む命令を実行する。
相手がRAMなら変更し得る。ROM/書込保護mappingでは実際の変更が阻止される場合がある。
**A=01Hを読取専用の試験だと思って呼んではいけない。**

8-word tableをdataとして解読してから、各helperを命令境界で切り出した。
さらに `88 07` の解釈を別のnative decoderで確認した。
両機種で同じbyte storeなので、VA2だけの差として扱わない。

なぜこの相違が存在するかは未確定。ROM実装の誤りと断定したり、
文書どおりのword loadへエミュレータを補正したりはしていない。
実機の対象ROM/versionや実行後の結果は今回測定していない。
OSが91Hのvectorを差し替えた場合は、その後のhandlerを別途調べる必要がある。

## 4. native frameと戻り値

**ROM確認**: native AX/BX/DX/CX/SI/DI/BP/ESをpushし、BP=SP、さらにDSをpushする。
DS←入力BX、BX←zero-extend(A)×2、AX←入力CXとしてhelperを間接near CALLする。

| native BPからの位置 | 保存内容 | compatible側の意味 |
|---|---|---|
| -02H | DS | 元のnative DS |
| +00H | ES | 元のnative ES |
| +02H | BP | compatible SP |
| +04H | DI | IY |
| +06H | SI | IX |
| +08H | CX | BC、読取結果の書戻し先 |
| +0AH | DX | DE |
| +0CH | BX | HL |
| +0EH | AX | 元のAはAL。native AHをFと混同しない |
| +10H | IP | CALLNの次、compatible PC+3 |
| +12H | CS | CALLN前のcompatible code segment |
| +14H | FLAGS | compatible Fを含む復帰frame |

読取/IN helperはSTC、入口はJNCで保存CXへ結果を書き戻すか決める。
書込/OUT helperにはSTCがなく、合法番号のtable選択時のSHLがCF=0にするので
保存CXを変更しない。01HはSTCがあるが、そのAXは入力BCのまま。

このCFは**内部copy制御**であって、公開success/error statusではない。
handlerは保存FLAGSを変更せず、IRETで復帰する。
A、DE、HL、IX/IY、compatible SPもpush/popで復元する。
「native STCだから戻ったZ80のC flag=1」という読み方は誤り。
全flagや割り込み条件の実機一致をこの静的調査だけで保証するものではない。

frameの固定分はCALLNの3 words＋handlerの9 words=24 bytes。
near CALL中はreturn addressの2 bytesも使う。割り込みやhardware trap等の追加frameは
含まないので、これを安全なstack確保量の保証として使わない。
compatible SPはnative BPに対応し、native SS:SPとは別のstack管理である。

## 5. ワーク領域と検証範囲

この短いROM handlerには0040H固定ワークやGetBook、buffer割当てはない。
保存状態はnative stack上、メモリ対象はcallerがHL:DEで指定する。
INT90Hの0040:00C0等の管理RAM、TVRAM/GVRAM、DMA imageとは所有者を区別する。
callerは対象segment/offset、I/O装置状態、native stackを適切に用意する必要がある。
入口/helperにはCLI/STIや独自lockもない。atomicityの契約は未確認。

**既存の実機測定（今回の新規試験ではない）**:
[M103a ALTPRB記録](../agents/reports/m103a_altregs_qa/README.md)では、
VA2のCALLN91H **A=00H memory byte read**でF000:13EDHを読み、BC=0033Hを確認している。
裏register、IX/IY/SP/DE/HLとFの保存もその条件で測定された。
[M103a task](../agents/tasks/M103a_alternate_register_storage.md)に既存IRET/F修正の説明がある。
この証拠をA=01H、word I/O、全mode/全ROMの保証へ拡張しない。
新しいエミュレータ修正や、その既存修正の再実行は本資料の作業ではない。

## 6. PC-Engine側の候補

OS本体、IO driver、拡張描画driverのCALLN91H/INT91Hとvector置換候補を調べた。
今回、91HのOS置換は確定できていない。全driverの不存在証明ではない。

OS本体のraw `CD 91` 候補はファイル91C3Hにあったが、
命令境界は91C2Hの `PUSH 91CDH`（encoding `68 CD 91`）だった。
91CDHはその近くの復帰先で、周辺はfar制御移譲を組み立てている。
即値の途中をINT91Hと解読するのは誤り。
`ED ED 91` の直接byte署名もこれらのファイルでは得られていないが、
動的生成、generic vector設定、別driverを排除する材料にはならない。

今後の課題は読取を装う01Hの安全な実機照合計画、起動後の最終vector、
word I/O/境界address/割り込み条件とregister保存の実測。
01H、write/OUT、不正A、状態を消費するI/Oへの実行probeは行っていない。
