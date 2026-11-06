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
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.
-->
# X88000互換機能の調査 — vaegとの比較

## Scope and evidence

M144aの**調査記録**。実装・修正の提案を採用した記録ではない。
調査対象のvaegはmain
[`dc023c9b7ce62c7334d797ba1934ad95b51fd3fd`](https://github.com/nakatamaho/vaeg/tree/dc023c9b7ce62c7334d797ba1934ad95b51fd3fd)。
[タスク／検証記録](../agents/tasks/M144a_x88000_compat_review.md)。

証拠の区分を次のように分ける。

- **HISTORY**: X88000作者サイトの変更履歴。
- **SOURCE**: 特定版のソースから確認できる実装。
- **DERIVED**: その式・状態代入から導いた例。エミュレータ実行ではない。
- **HISTORICAL**: 既存vaegタスクに記録された過去の検証。今回の再実行ではない。
- **UNKNOWN / NOT_RUN**: 実機仕様、ゲスト再現、未確認の条件。

X88000はPC-8801エミュレータであり、VA/VA2の互換ハードウェアそのものではない。
[ADR-0016](../agents/DECISIONS/ADR-0016-v1v2-mode-series.md)のとおり、相違があるだけで
VA側の不具合とは断定しない。特にPC-8801機種選択時とVA2/V2Hは、主CPUもVA側の
橋渡しも異なる。今回、ROM・媒体・実機は使用せず、X88000もvaegも実行していない。
持ち越し中のデモのシーン切り替え時のごみについて、原因特定や修正効果は主張しない。

## Reference versions

1. [作者サイト／変更履歴](https://quagma.sakura.ne.jp/manuke/x88000.html)。
   指定された2016-08-10/1.5.1、2018-08-10/1.5.3、2008-10-09/1.4.0、
   2001-10-09/`1, 3, 2`を確認。以下では最後の版名を1.3.2と表記する。
2. [作者のソース公開ページ](https://quagma.sakura.ne.jp/manuke/x88src.html)から取得した
   [1.5.3ソース](https://quagma.sakura.ne.jp/manuke/x88_1_5_3_src.tar.gz)。SHA-256:
   `66bc0b69d5a394e2195b5e277093eebcc045fbb3930c03637aa106394ae83cf3`。
   既存ADRの参照と一致。公開ページと同梱`X88000Src.txt`にPDSの表明がある。
3. [公開ミラーの固定版](https://github.com/nakatamaho/x88000/tree/1606677251c49c5d980953e13b42be5812ea99e7)
   の`x88_1_5_1_src/`を1.5.1の比較対象とした。ミラーを1.5.3とは扱わない。
4. 作者サイトで過去版名から組み立てた1.5.1、1.4.0、1.3.2の`*_src.tar.gz`
   URLはいずれもHTTP 404だった。**過去版が他所にも存在しないという主張ではない**。
   1.4.0/1.3.2の導入当時の細部は、履歴と現存1.5.3実装を分けて記す。

行番号は原本から作ったUTF-8/LFの作業用ビューによる。原本の保存・変換記録・
ファイル別ハッシュ・比較差分はGit外に保存し、第三者ソースはリポジトリへ追加しない。

選定した10ファイルを1.5.1と1.5.3で比較すると、正規化ビュー上で
`PC88Z80Main.cpp/.h`、`PC88Crtc.cpp/.h`、`PC88Pcg.cpp/.h`、`PC88.cpp/.h`、
`PC88Z80Sub.cpp`の9ファイルは同一。`X88ScreenDrawer.cpp`の差は、属性数取得の追加、
属性数上限2箇所、行末加算の4変更に限定される。これは**選定ファイルの比較**であり、
全配布物の同一性や1.5.0以前からの変更点を証明するものではない。

## Summary

| ID | 履歴項目 | X88000で確認したこと | vaeg mainの状態 | 判断の限界 |
| --- | --- | --- | --- | --- |
| X01 | 1.5.1 / 32H GVAM切り替え | GVAMが変わる両方向で独立アクセス先を3=メインRAMへ | GVAM更新はあるが、独立プレーン選択は保存されたまま | **SOURCE差あり**。VAでの期待動作・デモとの関係は未検証 |
| X02 | 1.5.1 / E6H許可時の要求クリア | タイマ/VRTC/USARTの禁止→許可で各要求を消去 | 禁止中の新規要求は捨てるが、許可への変更で既存要求を消さない | **SOURCE差あり**。要求を保持する場所も異なり、単純な移植は不可 |
| X03 | 1.5.1 / モノクロ反転色 | グラフィックスと文字を合わせたドットをセル色／背景色で反転 | 反転後のセル色と文字ラスタを合成する実装がある | 通常の可視セルでは**式が対応**。全モード・過渡フレームの同等性は未検証 |
| X04 | 1.5.1 / START DISPLAY DM | コマンドbit 0で全体反転、文字反転・カーソルと組み合わせる | TSP SYNCのRVを通常文字・3301互換文字の両方で使用 | **実装済み**。今回ゲストの20h/21h操作を再実行してはいない |
| X05 | 1.5.3 / 属性数変更 | 描画側の固定20組／40バイト加算を設定値へ変更 | TSP EMULに属性数があり、描画側も使用。行ピッチは別の画面表から取得 | **主要処理は実装済み**。3301設定→ROM→EMUL＋ピッチ更新の通し確認は残る |
| X06 | 1.4.0 / PCG-8800 | 2バンク文字RAM、00h–03h、8253の3音源 | **M115/M115aで実装済み**。書き込み条件と音源は後に意図的変更あり | 既存の人間受入あり。全8253動作の完全再現とは主張しない |
| X07 | 1.4.0 / 海外版モード | `optfont`による追加8×8フォント選択。PCG初期化／ROMコピーにも作用 | 対応する独立設定・追加フォント合成はない | この機能としては**未実装**。機種別ROMフォントの利用とは別 |
| X08 | 1.3.2 / ウェイト | M1、通常メモリ、表示DMA、GVRAMを条件別に遅延 | VA用実測命令コスト・WCU等はあるが、X88000と同じ選択式はない | **別方式／同等性未確立**。PC-8801用ウェイトをVAへ一括追加しない |

## X01 — GVAM and independent access

**SOURCE**: 1.5.3 `PC88Z80Main.cpp:2482–2502`、1.5.1の
[`WriteIO_32`](https://github.com/nakatamaho/x88000/blob/1606677251c49c5d980953e13b42be5812ea99e7/x88_1_5_1_src/PC88Z80Main.cpp#L2482-L2502)
も同じ。32h bit 6の新旧を比較し、変化した場合だけ`m_nGVRamBank=3`にする。
独立→拡張、拡張→独立の両方が対象。同じGVAM値の再書き込みでは初期化しない。
その後、独立なら`MemBankGVRamMode`、拡張なら`MemBankGVRamModeEx`でアクセス関数を選ぶ。

「メインRAMに選択」は**アクセス先の選択状態**を指す。RAM/VRAMを消す操作でも、
画面を非表示にする操作でもない。PC-8001用`WriteIO_33_N80`には同じ3への代入がなく、
32hの結果を33hへ広げて解釈しない。

vaegの
[`sysp_o032`](https://github.com/nakatamaho/vaeg/blob/dc023c9b7ce62c7334d797ba1934ad95b51fd3fd/io/sysportva.c#L57-L68)
は`memoryva_88_alu.gvam`を更新するが、`memoryva_88_plane`を変更しない。
独立選択は
[`memctrlva_o05c`](https://github.com/nakatamaho/vaeg/blob/dc023c9b7ce62c7334d797ba1934ad95b51fd3fd/io/memctrlva.c#L17-L26)
が5Ch–5Fhから設定する。リセット時は3だが、GVAM切り替え時の再選択はない。
[`n88_gvram_selected` / `n88_gvram_ex_selected`](https://github.com/nakatamaho/vaeg/blob/dc023c9b7ce62c7334d797ba1934ad95b51fd3fd/memoryva/memoryva.c#L771-L785)
はGVAMと独立選択を別々に参照する。

**DERIVED**: 独立モードで5Dhによりplane 1を選び、32hのGVAMを0→1→0とした場合、
X88000の最終選択は3、vaegは1。拡張中に独立選択を行ってからGVAMを1→0とする場合も、
X88000はその切り替えで3へ戻す。ALU自体の動作や35hのGAM条件とは分ける。

**UNKNOWN**: この差をVA/VA2のどちらにどう適用すべきか。後続の最小試験は、
同値書き込み、両方向の切り替え、拡張中の選択変更、35hのアクセス許可の組み合わせ。
今回ゲストメモリ破損や表示ごみの原因とは判定していない。

## X02 — E6H unmask transition

**SOURCE**: 1.5.3 `PC88Z80Main.cpp:2971–2976`および
[`SetTimerInterruptMask`等](https://github.com/nakatamaho/x88000/blob/1606677251c49c5d980953e13b42be5812ea99e7/x88_1_5_1_src/PC88Z80Main.h#L732-L784)。
E6hのbit 0=タイマ、bit 1=VRTC、bit 2=USART。**各ビットは1で許可、0で禁止**。
内部の`*InterruptMask` booleanは逆極性（trueが禁止）。書き込みはリセット引数をtrueにする。

要求を消す条件は「旧mask=true、次mask=false」。従って、E6hの対応ビットが
0→1になったときだけ消す。1→1の再書き込み、1→0での禁止、他のビットの変更では
その要求を消さない。対象はTimer/Crtc/Usartそれぞれの要求フラグであり、CPUのIFFや
割り込みレベル設定を一律にクリアする実装ではない。その後ベクタを再評価する。

vaegの
[`pic8214_request` / `pic8214_oe6`](https://github.com/nakatamaho/vaeg/blob/dc023c9b7ce62c7334d797ba1934ad95b51fd3fd/io/pic.c#L92-L195)
では、level 0/1/2をE6h bit 2/1/0に対応させる。
禁止中に到着した要求は最初からラッチしない。一方、`pic8214_oe6`はマスクを代入し、
必要なら600Hzタイマを起動するだけで、旧マスクとの比較、`irr`のクリア、
`pic8214_update`の呼び出しをしない。

**DERIVED**: 許可中にVRTC要求が入り、未受付のままE6hで禁止→許可とした場合、
X88000の対応要求は許可への変更で消える。vaegの`irr`の対応ビットはE6h書き込みだけでは
消えない。`pic8214_select`は既にラッチされた`irr`を参照し、現在のE6hマスクを再照合
していない。禁止中に**新しく**発生した要求を捨てることとは別の差である。

ただしvaegはVAの8214→8259接続、互換CPUへの`pending`、要求受付時の解除を持つ。
X88000のデバイス別要求フラグと同じ保存構造ではない。後続確認では、タイマ/VRTC/USART
別、到着と許可変更の順序、E4h、IFF、受付前後、既にofferedの`pending`を分ける。
今回、物理VAの消去条件や既存ソフトへの影響は検証していない。

## X03 — Monochrome reverse colour

**SOURCE**: 1.5.3 `X88ScreenDrawer.cpp:550–694, 983–1060`。
[`DrawGraphics`](https://github.com/nakatamaho/x88000/blob/1606677251c49c5d980953e13b42be5812ea99e7/x88_1_5_1_src/X88ScreenDrawer.cpp#L549-L693)
はモノクロのGVRAMドットと反転属性からセル色／背景色の表を作る。
[`DrawText`](https://github.com/nakatamaho/x88000/blob/1606677251c49c5d980953e13b42be5812ea99e7/x88_1_5_1_src/X88ScreenDrawer.cpp#L982-L1059)
は反転モノクロの文字ドットを背景色で上書きする。単に文字の色番号だけをXORするのではない。

vaegの
[`makeline_3301`](https://github.com/nakatamaho/vaeg/blob/dc023c9b7ce62c7334d797ba1934ad95b51fd3fd/vram/maketextva.c#L406-L556)
は文字反転と全体反転のXORでfg/bgを交換し、交換後のfgを`linecolor`へ保存する。
[`scrndrawva_compose_raster`](https://github.com/nakatamaho/vaeg/blob/dc023c9b7ce62c7334d797ba1934ad95b51fd3fd/vram/scrndrawva.c#L202-L218)
は多プレーン1bpp合成時、GVRAMドットありなら`textcolorraster`、なしなら
`textraster`を選ぶ。色を作る層の配置は違うが、対応する処理は既に存在する。

**DERIVED**: 可視の通常セル、カーソル・スプライト・線属性なし、合成対象の
GVRAMドットをG、文字ドットをT、反転の有効値をRとする。C/Bはそれぞれセルの
前景色／背景色という**役割**であり、双方のパレット番号そのものの一致ではない。

| R | GまたはT | X88000の色の役割 | vaeg合成の色の役割 |
| --- | --- | --- | --- |
| 0 | 0 | B | B |
| 0 | 1 | C | C |
| 1 | 0 | C | C |
| 1 | 1 | B | B |

この範囲では、グラフィックスと文字をORしたドットが反転する式になり、
「モノクロ反転機能がない」という状態ではない。ただし1.5.0の原本を取得していないので、
変更履歴にいう修正前の誤色の詳細を差分から復元したとは主張しない。

**HISTORICAL / UNKNOWN**: テキスト非表示時はVA/VA2とPC-8801で意図的に処理を分ける
既存方針がある（[M133](../agents/tasks/M133_hidden_text_graphics_colour.md)、
[M131の実機による訂正](../agents/tasks/M131_guest_start_display_corruption.md)）。
X88000のTVRAM overlay無効時の処理へ一律に合わせてはならない。
200/400ライン、文字属性のモノクロ／カラー、各パレット、反転カーソル、隠し文字、
シーン途中の属性変更まで同等とは未確認。デモの一瞬の表示についての新しい証拠はない。

## X04 — START DISPLAY DM and global reverse

**SOURCE**: 1.5.3 `PC88Crtc.h:610–619`、
[`WriteCommand`](https://github.com/nakatamaho/x88000/blob/1606677251c49c5d980953e13b42be5812ea99e7/x88_1_5_1_src/PC88Crtc.h#L610-L619)
のSTART DISPLAYはbit 0を`m_bReverseDisp`に保存する。20hなら解除、21hなら反転。
同時に表示開始、カウンタ、最終属性を初期化する。`RatchText`は文字反転・カーソル反転に
さらに全体反転を加える。非透過属性の描画分岐にも適用がある。

vaegには独立したPC-8801 CRTCモデルを同じ形で置くのではなく、VAファームウェアの
3301互換処理がTSPへ橋渡しする。
[`textva_global_reverse`](https://github.com/nakatamaho/vaeg/blob/dc023c9b7ce62c7334d797ba1934ad95b51fd3fd/vram/maketextva.c#L118-L132)
はTSP SYNC第2バイトbit 3のRVを読み、通常文字と3301互換文字の双方で属性反転とXORする。
現在はモノクロやPC-8801機種でもこの反転を除外していない。
今回固定したX88000の`RatchText`にも、`IsReverseDisp`を色モードで除外する条件はない。
過去記録の参照エミュレータ一般に対する評価と、この固定版の観察は同一視しない。

**HISTORICAL**: `sdl2/selftest.c`の`test_tsp_sync_reverse`と
[M139の反転訂正記録](../agents/tasks/M139_rv_all_machines.md)が既にある。
従ってDM相当の全体反転は**実装済み**。ただし内部RVの描画試験は、ゲストが20h/21hを
書いてROM経由で正しい時点に更新される通し試験の代わりにはならない。
表示開始／停止、個別反転、カーソル、同フレーム中の変更を含めた今回の再実行はNOT_RUN。

## X05 — Variable attribute count

**SOURCE**: 1.5.3 `PC88Crtc.cpp:336–338`、1.5.1でも同じ
[`WriteParam`](https://github.com/nakatamaho/x88000/blob/1606677251c49c5d980953e13b42be5812ea99e7/x88_1_5_1_src/PC88Crtc.cpp#L306-L346)
はRESETの第5パラメータから
`(値 & 1Fh)+1`で1〜32組を保存する。この保存処理自体は比較した1.5.1にもある。
1.5.3の変更は`X88ScreenDrawer.cpp:334,352,360,416`の**利用側**だった。

- 属性組を使い切る条件の固定19/20を、設定数−1／設定数へ変更。
- 1行の80文字の後、固定40バイト進める代わりに属性数×2バイト進める。
- 例えば属性1組なら次行は82バイト先、20組なら120、32組なら144。
  属性数を保存するだけでは後半2点は直らない。

vaegの
[`exec_emul`](https://github.com/nakatamaho/vaeg/blob/dc023c9b7ce62c7334d797ba1934ad95b51fd3fd/io/tsp.c#L255-L267)
はEMUL第3パラメータ＋1を`emul_attrs`に保存する。
`makeline_3301`はその組数を使い（作業配列の上限32）、文字数に続く属性ペアを消費する。
20組の固定ループではない。一方、
[`maketextva_raster` / `maketextva_3301_text`](https://github.com/nakatamaho/vaeg/blob/dc023c9b7ce62c7334d797ba1934ad95b51fd3fd/vram/maketextva.c#L786-L889)
は**別に設定されるTSP画面表のピッチ**を使う。EMULの属性数だけで次行を計算し直す
設計ではない。2文字の隠し先頭とローカルbyte/wordアドレス変換もあるため、
X88000の`80+2N`をVA画面表の生の値と直接比較しない。

従って「CRTC属性数変更がまったく未実装」は不正確。**描画側は可変対応済み**であり、
残る確認点はゲストの3301 RESET設定からVA ROMがEMULと画面表を整合して更新するか。
既存`test_tsp_3301_emulation`は標準20組を中心にしたテストなので、これだけで1〜32組の
全条件をPASSにはしない。後続では1/2/19/20/21/32組、先頭属性の列0／非0、
行をまたぐ属性、画面終端、40/80桁を、文字列と色の両方で確認する。

## X06 — HAL PCG-8800

**SOURCE**: 1.5.3 `PC88Pcg.cpp:101–234`（
[1.5.1の同一処理](https://github.com/nakatamaho/x88000/blob/1606677251c49c5d980953e13b42be5812ea99e7/x88_1_5_1_src/PC88Pcg.cpp#L101-L234)
）と`X88000.cpp:1341–1351`。
2個の1KiB文字RAM、コード下半分／上半分ごとの表示元、書き込みバンク、
00h–03hのデータ／アドレス／制御、0Ch–0Fhの8253カウンタを持つ。
初期RAMとROMコピーには現在選択中の文字フォントを使用する。

vaegの[`io/pcg88.c`](https://github.com/nakatamaho/vaeg/blob/dc023c9b7ce62c7334d797ba1934ad95b51fd3fd/io/pcg88.c)
に対応実装がある。V1/V2の88モードかつPCG設定ONで有効、03hのバンク機能、
8×8グリフ、3音、音量、`PCG88`状態保存、GUI/設定の接続もある。

| 項目 | X88000 1.5.3 | 現在のvaeg |
| --- | --- | --- |
| 文字RAMへの書き込み | 00h/01h/02h書き込みの末尾で書き込みビットを判定 | 02h書き込み時だけ判定（M115aの意図的方針） |
| ROMコピー・初期RAM | 通常／海外版の選択フォント | 読み込んだ8×8 ROMフォント |
| MSBのみのカウンタ設定 | case 2からcase 3へfall-through | MSB設定後に終了（M115からの意図的差） |
| 音源クロック | 4,000,000 Hzをカウンタで割る | 3,993,600 Hz |
| 音を出す条件 | ゲート、カウンタ0または256超など | ゲート、mode 3、カウンタ266超。0は65536 |
| 波形 | X88000の音声出力経路 | 奇数周期のduty、ゲート立上りで位相再開、PolyBLEP |
| 初期設定 | 参照版ではPCG OFF | mainはPCG ON。明示的OFFは保存して尊重 |

**HISTORICAL**: [M115](../agents/tasks/M115_pcg.md)、[M115a](../agents/tasks/M115a_pcg_quasi88.md)
は受入済み。文字書き込み、バンク、音程／3音、後者の雑音改善と余分なドット解消が
記録されている。[M142](../agents/tasks/M142_runtime_portability.md#pcg-and-va-n-mode-startup-defaults)
でデフォルトONへ変更されたため、古いM115の「デフォルトOFF」を現在仕様として転記しない。

結論は**既に取り込み済み、しかもその後に意図的な相違を導入済み**。
8253の全モード／読み返し／出力端子まで完全再現するという意味ではない。
今回、音声出力や保存復元を新しく実行したとは主張しない。1.4.0原本との差分は未取得。

## X07 — International mode is an optional font

**SOURCE**: `X88Resource.cpp`は海外版モードを`International Mode`と表示し、
`X88Option.cpp:747–770`は設定名`optfont`を`SetOptionFont`に接続する。
ホストUIの英語化は変更履歴でも別項目であり、この機能とは違う。

1.5.3 `PC88.cpp:334–352`、1.5.1でも同じ
[`CPC88::Initialize`](https://github.com/nakatamaho/x88000/blob/1606677251c49c5d980953e13b42be5812ea99e7/x88_1_5_1_src/PC88.cpp#L334-L352)
内の読み込みは次の流れ。

1. 通常のANKフォント2048バイト（256字×8行）を追加フォント用バッファへコピー。
2. 任意の追加フォント`optfont.rom`があれば2048バイトを読む。
3. 各文字の8バイトのうち1バイトでも非ゼロなら、**その文字の8バイト全部**を置換。
   8バイトともゼロなら通常フォントを残す。各ラスタの非ゼロ部分だけの合成ではない。
4. `DrawText`はPCG有効ならPCG、そうでなければ追加フォント、ひらがな、通常の順で選ぶ。
   セミグラフィックスの指定はさらに別の描画フォントを選ぶ。
5. `GetCurTextFontRom`は追加フォント選択を参照するため、PCGの初期RAMとROMコピーにも
   影響する。単なる画面フィルタではない。

`OptionFont`の参照箇所を追った範囲では、このスイッチによるキーボード配列変更、
英語BASIC ROMの切り替え、CPUやI/Oの海外モデル化は見つからない。
従って**海外機全体のエミュレーション**と呼ぶのは広すぎる。
追加ROMの中身やどの実機版に対応するかは今回調べておらず、配布・真正性も主張しない。

vaegは`cgromva_ank8`と機種別の外部ROM適用経路を持ち、PCGも読み込んだフォントを使う。
しかし`optfont`相当の独立設定、上記の文字単位フォールバック合成、PCGと連動した
追加フォント選択はない。一般のフォント読み込みや英語キーボード設定を、
この機能の実装済み証拠にはしない。**この機能としては未実装**。
将来追加するなら、合成ゼロ文字、PCGとの優先順、リセット時点、保存復元と、
フォントのライセンス／私有入力境界を別途仕様化する。

## X08 — Wait emulation

### X88000 source

**HISTORY**: 1.3.2の説明は「V1-Sモード等の速度が実機に近づきました」。
以下の詳細は**現存1.5.3の処理**であり、2001年版そのものを実行・復元した結果ではない。
`waitemu`は初期値OFF。高速ROM処理、旧機種互換、ホストの待ち／描画間引きとは別設定。

`CPC88::Reset`（`PC88.cpp:407–427`）が主CPU・サブCPUへ設定を渡し、主CPUの選択から
CRTCのwaitを設定する。
[`CPC88Z80Main::Reset`](https://github.com/nakatamaho/x88000/blob/1606677251c49c5d980953e13b42be5812ea99e7/x88_1_5_1_src/PC88Z80Main.cpp#L734-L882)
（1.5.3 `PC88Z80Main.cpp:734–762`）はON時に次のフラグを選ぶ。

| BASICモード | M1 wait | TVRAM DMA wait | GVRAM wait |
| --- | --- | --- | --- |
| N88 V2 | OFF | OFF | OFF |
| N80 V1 | ON | ON | ON |
| N80 V2 | OFF | ON | OFF |
| その他のH | OFF | OFF | OFF |
| その他のS（N88 V1Sなど） | ON | ON | ON |

通常メモリの追加待ちは別条件`baseclock==8 && !Clock8MHzH`。
通常アクセス用とM1用の関数表を分け、`*_Wait`が`AddExecClock(1)`を加える。
fast TVRAMのM1はwaitemu ONなら全モードで待つ特例もある。
「どのアクセスにも常に同じ待ちを足す」というモードではない。
主CPUのI/Oハンドラ内には、このwaitemu用の`AddExecClock(1)`追加は見つからない。
命令自身のIN/OUTサイクルや周辺機器の待ち状態とは区別する。

[`PC88Crtc.h`の`PassClockVWait`](https://github.com/nakatamaho/x88000/blob/1606677251c49c5d980953e13b42be5812ea99e7/x88_1_5_1_src/PC88Crtc.h#L528-L591)
では次の表示競合を近似する。

- TVRAMはDMA開始かつ表示開始で有効。文字行境界ごとに`120*6`または`120*10`を加算。
  後者はDMA側のwait指定による。可変属性数に比例する式にはなっていない。
- GVRAMはグラフィックス有効かつCPUがGVRAMアクセス状態で有効。
  TVRAM DMA待ちのactive／inactive、15/24kHz相当、20/25行で追加率を切り替え、
  百分率の端数を繰り越す。
  定数はL20=120/164、L25=112/170、H20=304/365、H25=263/337。
  これはソース内の追加率であり、実機に対する実測比の引用ではない。
- FDCサブCPUも別にM1 waitを選ぶ。`PC88Z80Sub::Reset`ではwaitemu ONかつ
  2HD-ROM型でない場合にM1用の読み取りへ1を加える。主CPUの8MHz化とは別の条件。

**参照自身の注意点**: `PC88.h:436–465`では、複数命令版
`ExecuteMultiOpCodeMainVWait`は`PassClockVWait`の戻り値を実行時計へ足すが、
1命令版`ExecuteOneOpCodeMainVWait`は戻り値を使わない。CRTC内部の更新は双方で行われるが、
この差のため両経路の全デバイス時間が同じだと仮定できない。今回実行して影響を測っては
いない。参考実装の式を絶対的な実機仕様として移植しない理由の一つである。

### vaeg source

- [`CompatCounter` / `MainCpuBridge`](https://github.com/nakatamaho/vaeg/blob/dc023c9b7ce62c7334d797ba1934ad95b51fd3fd/cpu/upd9002_upd70008.cpp#L384-L437)
  は命令コストとnative waitを別勘定にする。
  VA/VA2のuPD70008互換モードは[M135](../agents/tasks/M135_compat_mode_timing.md)の
  実測クラス別コストを使い、WCU／デバイス待ちを重ねる。
- 非VAのPC-8801/8001選択時は**独立した主uPD780C**を使用し、
  [命令時間はコア側を採用](https://github.com/nakatamaho/vaeg/blob/dc023c9b7ce62c7334d797ba1934ad95b51fd3fd/cpu/upd9002_upd70008.cpp#L575-L593)。
  VA向けM135命令表をそのまま使うのではない。バスとVAファームウェアの橋渡しは共用する。
- [`Read8` / `Write8`](https://github.com/nakatamaho/vaeg/blob/dc023c9b7ce62c7334d797ba1934ad95b51fd3fd/cpu/upd9002_upd70008.cpp#L705-L778)
  は既存メモリ経路の待ちを回収する。VAでは整列したword busの共有を考慮するが、
  主uPD780Cでは8bit busとしてその共有をしない。
- [`pccore_compat_clocks`](https://github.com/nakatamaho/vaeg/blob/dc023c9b7ce62c7334d797ba1934ad95b51fd3fd/machine/pccore.c#L231-L254)
  はS設定で命令コストを伸ばす一方、既にnative単位で計上したwaitを再倍率化しない。
  M143の実行速度表示やF11はこの待ち設定を追加する機能ではない。
- nativeのGVRAM待ちとTVRAM word待ちは存在するが、
  [`n88_gvram_ex_read/write`等](https://github.com/nakatamaho/vaeg/blob/dc023c9b7ce62c7334d797ba1934ad95b51fd3fd/memoryva/memoryva.c#L771-L840)
  は互換ウィンドウから配列を直接読む／書く。この経路にX88000の`PassClockVWait`
  相当の表示DMA競合モデルはない。呼び出し元のWCU等もあるため、これを
  「互換メモリアクセスの待ちは全部ゼロ」と言い換えてはいけない。
- FDCの`Subsystem::SetWait`は外部WAIT線を扱うもので、X88000のROM型に応じた
  M1ごとの+1と同じ意味ではない。名称だけで対応済みとは判定しない。

結論は、**待ちの仕組みはあるが、X88000のwaitemuと同じモデルではない**。
特に非VAのPC-8801 V1SにおけるM1／表示DMA／GVRAMの等価性は未確立。
一方、X88000用の+1や追加率をVAへ足すと、既存の実測命令時間・WCUと二重計上する恐れがある。
具体的な不足量・原因・修正値は今回選定しない。

## Follow-up priority and non-claims

実装を許可された場合の順序案であり、着手指示ではない。

1. **X01/X02**: 小さな状態遷移試験で差を固定し、PC-8801とVA/VA2の期待値を分離する。
   回帰テストの失敗を確認してから、必要なら最小の訂正を検討する。
2. **X05**: 可変属性数は処理済み部分が大きい。まずROM→EMUL→ピッチの通し確認。
   デモが実際に何組へ変更しているかは、別途取得した実行トレースが必要。
3. **X03/X04**: 通常の色式・RVは既にある。非表示／反転／属性carry／カーソル／
   同フレーム変更の境界を観測し、既存の実機受入方針を壊さない。
4. **X08**: N88 V1H/V1S/V2、VA/VA2とPC-8801主CPU、コード／データ／M1、
   RAM／TVRAM／GVRAM、表示DMA有無を分けて測定する。ホストFPSから待ちを逆算しない。
5. **X06/X07**: PCGは既存の意図的差を維持。海外フォントは新機能として別途仕様を決める。

## Subsequent PC-8001 implementation work

以下は上記の静的調査とは別の、後続の実装進捗である。メンテナの追加指示により
PC-8001mkII/SRのグラフィックス・ROMバンク・モード切り替え・状態保存を優先する。
`io/pc8001_video.c/.h`に、N80/SR ROMソース・オフセット判定、8000h–BFFFhの
GVRAM窓、mkIIの640ドット1bit/320ドット2bit形式、SRの640ドット3プレーンと
320ドット2ページの透明色／優先ページ合成を追加した。

現段階ではテストターゲットだけに接続している。2,879,495 assertions、ASan/UBSan、
7種類の単一変異検出を通過したが、**エミュレータ本体での起動・描画・SAV復元は未完了**。
既存のVAベースNモードを、未完成のN80SR経路へ切り替えてはいない。
[進捗と残作業](../agents/tasks/M144a_x88000_compat_review.md#pc-8001mkiisr-implementation-checkpoint)。
PC-PR201/PDFとFDDクロック確認はこの作業中は保留。

その後、ROM読み込み・実I/Oディスパッチ・メモリ読出し・GVRAM窓・SAVファイルへの
接続を追加した。`e8.rom`はメンテナ承認の**X88000互換の任意入力名**であり、
MAME標準名や実機検証済み仕様とは表記しない。SRの40 KiB ROMはN80 ROMと
別バッファで保持し、31h/33h/71hによって参照先だけを切り替える。
`N80BANK`は機種とバンク状態を保存し、不正状態・別機種への復元はロード前に拒否する。
合成ROMでの実ファイルSAV往復と不正状態の拒否、GVRAMの8000h–BFFFhアクセスは通過。
ただし**ゲスト命令のI/Oトラップ経路、実画面への接続、媒体からの起動は未完了**。
[本体接続チェックポイント](../agents/tasks/M144a_x88000_compat_review.md#production-n80-rombanking-checkpoint)を参照。

続くチェックポイントでは、独立した主CPUがmkII/SRのバンク制御を直接扱う
限定的なI/O経路を追加した。VAトラップの設定自体は変更せず、未実装の文字・
パレット・CRTC制御は従来のトラップ経路へ残す。実際の即値／ED／INI・OUTI命令と
機種・モード対照からなる360ケースを確認し、トラップ例外を除く単一変異も検出した。
これは前段の「ゲストI/O未確認」を対象ポート・命令形式に限って更新するもので、
BASIC起動や描画をPASSにするものではない。
[ゲストI/Oチェックポイント](../agents/tasks/M144a_x88000_compat_review.md#guest-instruction-io-checkpoint)。

カラー表示の後続実装では、mkIIの4色、SRの2ページ／8色出力を本体ラスタへ接続した。
52h–5Bhのデジタルパレット・表示制御をゲストI/O経路へ追加し、命令試験は720ケースに
拡大した。SRのページ優先、透明色0、文字を背面へ置く33h bit 3は最終`vabitmap`画素で
検査する。文字・ページ表示禁止、背景色、パレットと表示制御のSAV復元も合成入力で
確認する。これはモノクロ固有属性、SRフォント、実媒体起動やホスト画面の確認までを
意味しない。[カラー接続の範囲](../agents/tasks/M144a_x88000_compat_review.md#n80-colour-raster-and-composition-checkpoint)。

続いてモノクロのプレーンORと反転前の文字合成、文字表示禁止時の色保持を接続した。
SRの字形は`font80sr.rom`内0800hからの8行字形を33h bit 4で選び、PCG優先と
通常ANKの内容を維持する。MAMEの別ファイル`fonthira.rom`の用途を同一視しない。
SR ROMが揃う場合のみ、最初の起動引き渡しでSRを選ぶようにした。
完全フォントを優先し、欠ける場合には`fonthira`を第2バンクの代替として
受け付ける。通常ANKを置き換えず、PCGの優先順位を維持する。
ゲストRAM上のモード遷移ルーチンと、必要な状態セクション・進行・復元後の
同値性を検査する仕組みを追加した。状態比較の契約はEVENT/BEEPを除外するため、
全状態や音声の同値性を保証するものではない。

### 属性と画素比較のテスト仕様

[`n80_attributes.py`](../../tools/qa/n80_attributes.py)は、文字コード65の
通常ANK／セミグラフィックス、32種類の装飾属性、40／80桁、点滅位相、
全画面反転をパラメータとする入力生成と画素検査を提供する。矩形の契約は
51×21ドットと周辺の余白を対象とする。これはツールの仕様であり、
実行結果の報告ではない。

[`n80_media_survey.py`](../../tools/qa/n80_media_survey.py)は、明示的な計画と
独立した設定を扱う実行補助ツールである。終了コード0は`UNASSESSED`とし、
互換性PASSに変換しない。Git内への出力と途中のworker変更を拒否する。

以下の非主張は元の調査チェックポイントに関するもの。
今回の結果は**SOURCE_REVIEW**であって、実機・ゲスト・画面・音声のPASSではない。
新しいビルド、ROM／媒体実行、描画キャプチャ、実機I/O、修正コミットは存在しない。
未修正の表示ごみは既存台帳のままOPEN/DEFERRED。静的な相違だけで根本原因として
bug-fix台帳へ追加したり、X88000からの移植が必要と断定したりしない。
