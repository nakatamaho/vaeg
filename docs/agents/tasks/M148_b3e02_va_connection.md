<!--
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
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

# M148 — PC-9801B3-E02 を PC-88VA につなぐ（手順と既存プローブの解析）

状態: **IN_PROGRESS**。ブランチ `milestone/m148-b3e02`（`origin/main`
1fb2fed0 から）。B0〜B3 のプローブと試験ディスクを用意した（§8）。
実機の段階はすべて未実行。コミットの接頭辞は `M148:`。

## 1. 目的

NEC PC-9801B3-E02（CL-GD5428、VRAM 1MB、C バス、マニュアルなし、
VA での実績なし）を PC-88VA の拡張スロットに挿し、

1. VA から I/O でチップに届くか、
2. VA の CPU（20 bit アドレスの uPD9002）から VRAM 窓に届くか、
3. 届くなら BLT を含めて画が出せるか

を実機で順に確かめ、通った段階だけ vaeg に実装する。
型番の正式表記は PC-9801B3-E02（「B03」ではない）。ID 値は
PC-9801-96 と共通の 60h。

## 2. 根拠の区分

- `[DOC-98]` PC-9800 の I/O 資料（ウィンドウアクセラレータ制御の章）
- `[DOC-VA]` PC-88VA テクニカルマニュアル 2.2「I/O ポートマップ」、
  3.3「ROM エリアのバンク切換え」、3.6「拡張スロットで拡張できるメモリ」
- `[CODE-NP2]` NP2kai `wab/cirrus_vga.c` ほか（作者自身が
  「この辺のマッピング本当にあってる？」と書いている。信頼度は低い）
- `[MEASURED]` vaeg の過去の実機計測（M144、M146）
- `[STATIC]` 本書のための既存プローブ 17 本の逆アセンブル解析
- `[INFERRED]` 上からの推論。実機で確かめるまで事実として扱わない

## 3. ボードの仕様（わかっていること）

### 3.1 I/O ポート `[DOC-98]` `[CODE-NP2]`

| 機能 | 本来の VGA | B3-E02 / -96 |
|---|---|---|
| Attribute index/data（書き） | 3C0h | 0C50h |
| Attribute data（読み） | 3C1h | 0C51h |
| Input Status 0（読み）/ Misc Output（書き） | 3C2h | 0C52h |
| Sleep Address | 3C3h | 0C53h |
| Sequencer index / data | 3C4h / 3C5h | 0C54h / 0C55h |
| Pixel Mask / Hidden DAC | 3C6h | 0C56h |
| DAC 読みアドレス / DAC State | 3C7h | 0C57h |
| DAC 書きアドレス | 3C8h | 0C58h |
| DAC data | 3C9h | 0C59h |
| Feature Control（読み） | 3CAh | 0C5Ah |
| Misc Output（読み） | 3CCh | 0C5Ch |
| Graphics Controller index / data | 3CEh / 3CFh | 0C5Eh / 0C5Fh |
| CRTC index / data（カラー） | 3D4h / 3D5h | 0D54h / 0D55h |
| Input Status 1 / Feature Control（カラー） | 3DAh | 0D5Ah |
| CRTC index / data（モノクロ） | 3B4h / 3B5h | 0B54h / 0B55h `[CODE-NP2]` |
| Input Status 1（モノクロ） | 3BAh | 0B5Ah `[CODE-NP2]` |
| POS102（Video Subsystem Enable） | 102h | 0902h |
| Setup / 102h アクセス制御 | 94h（46E8h 系） | 0904h |
| ボード制御 index / data | — | 0FA2h / 0FA3h |

- CRTC は Misc Output bit0 で 3Bx（0B54h）と 3Dx（0D54h）が切り替わる。
  リセット直後の Misc は 00h なので、**CR27（チップ ID）はまず 0B54h/0B55h
  で読む必要がある**。0D54h で読むなら先に Misc bit0 を 1 にする。
- 46E8h 系の起こし手順はこのボードでは使わない（NP2kai も登録していない）。

### 3.2 ボード制御 0FA2h/0FA3h `[DOC-98]`

| reg | 内容 |
|---|---|
| 00h | ID。60h = PC-9801-96 / PC-9801B3-E02。bit7-6・bit3 チップ ID（01xx0xxxb = CL-GD5428）、bit5-4 バス（10b = C バス）、bit2-0 ボード版数 |
| 01h | VRAM 窓アドレス。80h = F20000h、A0h = F00000h、C0h = F40000h、E0h = F60000h。10h = 0B0000h は内蔵機の表にしかなく、-96 での 10h は `[CODE-NP2]` のみ |
| 03h | bit1 リレー（1 = アクセラレータ出力）、bit0 メモリマップト I/O。-96 での意味は `[CODE-NP2]` のみ |
| 04h | DIP スイッチ（読み）。bit7 インタレース、bit6 機種設定、**bit4 = 1 で 15MB モード**、bit3-2 16MB モードの窓（F00000h/F20000h/F40000h/F60000h）、bit1-0 15MB モードの窓（E00000h/E20000h/E40000h/E60000h） |

窓は 64KB で、チップ側の GR09/GR0A（オフセット 0/1）と GR0B（二重バンク・
16KB 粒度）でバンクを切り替える。

### 3.3 VA 側の経路

- `[MEASURED]`（M144）VA の I/O は 16 bit 全デコード。未接続の I/O を
  読むと FFh。B3-E02 のポートはすべて VA のシステムエリア 0000h–0FFFh
  にあるが、下位 8 bit だけで本体と衝突する、という心配はいらない。
  ただし、**スロットへ 0000h–0FFFh の I/O サイクルが出るか**は未確認。
- `[DOC-VA]` 153h の SMBC（bit3-0）= 0 で A0000h–DFFFFh（システムメモリ
  エリア）がバススロットになる（M146 で EMS ページフレームとして使用）。
  **よって B0000h–BFFFFh はスロットへ出せる。** 以前のメモの「B 領域を
  出す手段は見つかっていない」は誤り。153h の bit6（SMM、V3 = 1）と bit4
  （GMSP）は保つこと。
- `[DOC-VA]` 152h bit3 = 1 で E0000h–EFFFFh（ROM エリア 0）、bit7 = 1 で
  F0000h–FFFFFh（ROM エリア 1）がスロットになる。F 側は割り込みベクタ
  処理や BIOS が動かなくなるので使わない。
- `[DOC-VA]` RAM エリアでスロットに出せるのは 40000h–9FFFFh のみ。
- `[MEASURED]`（M143/M144）未接続の**メモリ**読み出しは直前のバス値
  （命令コードのバイト）を返す。I/O と違って FFh ではない。
  **書いた直後に同じ番地を読んで一致しても RAM の証拠にならない。**
- **未確定で、成否を決める点**: VA のスロットが C バスの A20–A23 を
  どう扱っているか（0 に固定か、未接続でボード側のプルアップにより 1
  か）。0 なら届く窓は 0B0000h（reg1 = 10h）だけで、F0x0000h には届かない。
  1 なら F00000h 系の窓が VA の 00000h–6FFFFh（主 RAM）に重なり、
  主 RAM と同時に駆動されて危険。**ソフトで調べる前に、スロットの
  A20–A23 端子の電位をテスタで測ってもらう**のが最も安全で確実。

## 4. 既存プローブ 17 本の解析 `[STATIC]`

保守者から渡された MS-DOS 4.0（msdos4-va.2）起動ディスク 17 枚。どれも
ソースはなく、`.COM` 1〜2 本と標準の AUTOEXEC.BAT・CONFIG.SYS・
README.TXT・LICENSE.TXT だけを含む。実行結果（写真・ログ）は渡されて
いない。抽出物と逆アセンブルは Git の外
（`~/work/pc88va/tmp/m148/extract/`、`dis/`、`code/`）にある。

### 4.1 共通部

- 引数が ` /ENABLE` でなければ説明を出して終了コード 1。
- F000:FFFE が FFFFh（VA）でなければ何もせず終了コード 2。VA2 では
  意図的に実行しない。
- 表示は DOS の AH=09h。結果の 16 進は 2 桁の文字列に埋めている。
- ボードの前提確認は、0FA2h を読んで 00h、0FA3h を読んで 60h。
- 「起こし」は全プログラム共通で `OUT 0904h,00h` → `OUT 0902h,01h` →
  `OUT 0904h,20h`。その後 0C5Eh（GC index）が 0Fh になることを、
  後のプログラムは前提にしている。
- 0FA2h = 03h、0FA3h = 03h（リレー＋MMIO）を書く。

### 4.2 一覧（実験 ID 順）

| ID | プログラム | やっていること | 書き込み | 後始末 |
|---|---|---|---|---|
| M04-G | B3WIDE | ID 確認、0C5E が FFh であること、起こし前後の 0C5E/0C54/0C5C | 起こし 3 回 | 起こしは戻さない |
| M04-H | B3COMBO | 上に reg3 = 03h を加えた前後比較 | reg3、起こし | reg3 = 00h、index 00h |
| M04-I | B3CHECK | 上＋GC index の 04h→08h→04h 選択読み戻し | reg3、起こし、GC index | GC index を 0Fh に戻す |
| M04-J | B3MULTI | 上＋Sequencer index 02h→04h→02h | 同上＋SR index | 同上 |
| M05-A | B5STATE | 153h、reg0/01/04 の読み取り、GC 00–08、SR 02/04 のスナップショット | reg3、起こし、index | reg3、index |
| M05-B | B5WINDOW | reg01/reg04 を初期・reg3 = 03h・起こし後で比較 | reg3、起こし | reg3、index |
| M05-C | B5LOW | reg1 = 10h、153h SMBC = 0 で B0000/B4000/B8000/BFFFF を読む | reg1、153h | 153h、reg1 = 00h |
| M05-F | B5CFG | SR02 = 0Fh、GR04 = 00h、GR05 &= FCh の後、B0000 に書いて読む | チップ、reg1、153h、B0000 | 全部戻す |
| M05-G | B5MISC | 上＋Misc Output（0C52h）bit0 = 1 | 同上＋Misc | 同上 |
| M05-H | B5ROM | 153h SMBC = 1 と 0 で A000–DFFF の 4KB ごとのマップと B0000 を比較（ボードを触らない） | 153h | 153h |
| M05-I | B5COMB | B5MISC と同じ組み合わせ＋SMBC = 0 の A000–DFFF マップ | 同上 | 同上 |
| M05-J | B5COMB2 | 上＋SR06 = 12h（解錠）、SR07 &= 0Fh、GR06 読み | 同上＋SR06/SR07 | 同上 |
| M06 | B5EWIN | SR06 解錠、GR09 = GR0A = 00h、Misc bit0、152h bit3 で E0000–EFFFF を読み、E0000/E7FF0 に書く | チップ、152h、E 窓 | 152h、GR、Misc |
| M06 | B5ECWIN | 00ECh = C0h の前後で 80000/8FFFF/90000/9FFFF を比較（ボードを触らない） | **80000h–9FFFFh の主 RAM**、ECh | ECh だけ |
| M06 | B5EWIN2 | GR を触らずに E 窓のマップと E0000/E7FF0 の書き読み | SR06、152h、E 窓 | 152h、reg3 |
| M06 | B5EWIN3 | E0000 に 00h–FFh を書いてすぐ読む 256 回、GR09 = 1/0 で E0000 を比較 | SR06、GR09、152h、E 窓 | 152h、GR09 |
| M06 | B5EWIN4 | 書かずに GR09 = 0–3 で E0000 の 16 バイトを読む | SR06、GR09、152h | 152h、GR09 |
| M06 | B5EWIN5 | GR06 = 01h、SMBC = 0 で A0000 に 256 値を書いて読む | チップ、GR06、153h、A0000 | 全部戻す |

### 4.3 実行されたことが推測できる点 `[INFERRED]`

後のプログラムほど前提確認が厳しくなっている（M05 以降は ID = 60h、
起こし後の 0C5E = 0Fh、SR06 の 12h 読み戻しを要求）。作者がそれを前提に
次を作ったのなら、実機で

- 0FA2h/0FA3h が届き ID = 60h が読めた、
- 起こしの後 0C5Eh が FFh から 0Fh に変わった、
- SR06 に 12h を書いて 12h が読めた、

と考えられる。ただし結果は渡されておらず、未確認として扱う。
VGA の GC index の上位 2 bit は予約で、0Fh という値は「チップが応答した」
とも「index レジスタにたまたま残った値」とも読めるので、CR27 が読める
までチップ到達の決め手にはしない。

### 4.4 既存プローブの欠陥

結果の解釈に効くもの:

1. **VRAM 窓の判定方法が成り立たない。** B5EWIN3・B5EWIN5・B5CFG ほか
   は、書いた直後に同じ番地を読んで一致すれば RAM としている。未接続の
   メモリは直前のバス値を返すので、何もつながっていなくても一致しうる。
   画面の「mism=0000: the window is fully functional RAM」は根拠にならない。
2. **Misc Output の RAM Enable（bit1）を一度も立てていない。**
   B5MISC/B5COMB/B5COMB2/B5EWIN は bit0（CRTC を 3Dx へ）だけを立てる。
   リセット直後の VGA は bit1 = 0 で CPU から VRAM へのアクセスを受け
   付けないので、窓の試験はすべて不成立になりうる。
3. **CR27（チップ ID）を一度も読んでいない。** チップ到達の最も確かな
   証拠がない。
4. **B5EWIN4 は 16 進に変換せず生のバイトを表示文字列に書き込む。**
   24h（`$`）があればそこで表示が切れ、制御コードは画面を乱し、ROM の
   テキスト BIOS は印字可能 ASCII 以外を出さない。「GR09 の 4 つのダンプ
   が違えば VRAM」という判定は写真から読めない。マップの見出し
   「E000-E7FF」も誤りで、実際は E000–EFFF を 4KB ごとに 16 個調べている。
5. **表示されるが一度も代入されない欄がある。** B5EWIN3 の
   「Pattern pre/rb」と「RBC … restored rb」、B5EWIN4 の「RBC restored rb」
   は常に 00 と表示される。
6. **B5ECWIN は MS-DOS が使っている主 RAM（80000h–9FFFFh）に割り込み
   許可のまま書き込み、8000:0000 を 5Ah のまま残す。** 9FFF:000F は
   COMMAND.COM の常駐しない部分がある最上位付近で、戻った後にハング
   しうる。B3-E02 とは関係のない実験でもある。
7. **異常終了時の後始末の誤り。** B5EWIN3/B5EWIN4 の失敗経路は SR06 を
   戻すとき index 06h を書かずに 0C55h へ書く。B5EWIN4 は GR09 を戻す
   つもりで Sequencer（0C54h）の index 09h（スクラッチパッド）に書く。
8. 成功経路で起こし（0902h/0904h）、SR06 の解錠、reg3 を戻さない
   プログラムが多い（「電源を切る」前提）。連続して実行すると前の状態
   を引き継ぐ。
9. GR0B（二重バンク・16KB 粒度）を書いていないので、GR09/GR0A の粒度
   は既定の 4KB のまま。B5EWIN の「GR0B rb」は表示だけ。
10. 窓 E0000h（152h bit3）を使う M06 群は、ボードの窓をどこに置くか
    （reg1、DIP の 15MB モード）を設定していない。既定の F00000h の窓が
    E0000h に現れる理由が説明されていない。
11. 結果をファイルに残さず、写真から手で書き写す前提になっている。

vaeg での予想（未実行）: 今の vaeg には B3-E02 がなく 0FA3h は FFh なので、
ID を確かめるものは書き込みなしで終了コード 3 で止まる。ID を確かめない
B5ROM と B5ECWIN は最後まで走る（B5ECWIN は主 RAM を書き換える）。

## 5. 新しい手順

### 5.1 プローブの共通規則

- MS-DOS 4.0（msdos4-va.2）上の `.COM`。ソースを
  `demos/b3e02/tests/*.asm`（NASM）に置き、`build.sh` で起動ディスクを
  作る。ディスクは Git の外。
- 機種を F000:FFFE で判定して画面とログに出す（FFFFh = VA、FFFEh =
  VA2/VA3、FFFDh = VA-91 付き）。最初は VA に限る。
- **結果は A:\B3LOG.TXT に追記する**（日付、機種、プログラム名と版、
  全読み値の 16 進、段階コード、後始末の結果）。画面にも同じものを
  16 進で出す。生のバイトは表示しない。
- 書いたものはすべて元の値に戻し、戻したことを読み戻しで確かめる
  （index レジスタも含む）。起こしの状態も最後に戻す。
- 主 RAM（00000h–9FFFFh）には書かない。窓を開けている間は `cli`。
- メモリの試験は「未接続でもたまたま一致する」ことを排除する形にする:
  (a) 2 つ以上の番地に異なる値を書いてから順に読む、(b) 書きと読みの
  間に別の番地（例えば DS 上の変数）への読み書きを挟む、(c) GR09 で
  バンクを替えると内容が替わり、戻すと元に戻ることを見る。
- 終了コード: 0 = 合格、1 = 引数、2 = 機種、3 = ボードなし、
  10h 台以降 = 段階ごとの失敗。
- vaeg では同じディスクを、ボードなしとボードありのモデルで回帰試験に
  する（`smoke` 相当）。

### 5.2 段階

**B0（ボードなし、読みだけ）** — VA と VA2 で、0C50h–0C5Fh、0D54h/0D55h、
0D5Ah、0B54h/0B55h、0B5Ah、0FA2h/0FA3h、0FACh、0902h、0904h を、
直後の命令を変えて 2 通りに読む。合格: 全部 FFh（駆動なし）。
SMBC = 0 の B0000h、152h bit3 の E0000h を読み、ボードなしの値を記録する
（メモリは直前のバス値が出るので、比較の基準として残す）。

**B0.5（テスタ）** — スロットの A20–A23 端子の電位（電源投入後、
MS-DOS のプロンプトで）。0 V なら 0B0000h 窓へ進む。5 V または浮いている
なら、F0x0000h 窓が主 RAM に重なるので reg1 を変える試験はしない。

**B1（ボード装着、ID）** — 0FA2h に 00h を書いて読み戻し、0FA3h を読む。
0FA2h に 01h/04h を書いて reg1・reg4（DIP）を読む。合格: ID = 60h、
index の読み戻しが一致。書くのは index だけ。

**B2（起こしとチップ ID）** — 起こし（0904h = 00h、0902h = 01h、
0904h = 20h）の前後で、0C5Ch（Misc）、0C54h、0C5Eh、0B54h/0B55h の CR27、
SR06 の解錠（12h を書いて 12h、他の値で 0Fh）を読む。
合格: **CR27 が 98h〜9Bh**（CL-GD5428）、SR06 が切り替わる。
CR27 は Misc bit0 = 0 なら 0B54h、1 なら 0D54h で読む。最後に全部戻す。
B2 に通らなければここで止め、不成立と記録する。

**B3（チップ設定のスナップショット）** — 起こし後の SR00–SR1F、
GR00–GR0E、CR00–CR27、Misc を読むだけ。BIOS がないボードなので、
電源投入直後のレジスタ値（クロック、メモリ設定 SR0F/SR17 など）を
知ることが窓の試験の前提になる。

**B4（窓 0B0000h）** — B0.5 で A20–A23 = 0 のときだけ。
reg1 = 10h、Misc bit1（RAM Enable）= 1、SR04/GR05/GR06 で線形の
パックドピクセル（SR07 bit0 = 1）、GR0B bit0 = 1（二重バンク）を設定し、
153h SMBC = 0 で B0000h–BFFFFh を使う。5.1 の方法で、GR09/GR0A を
動かして VRAM 1MB 全域にパターンを書いて読み戻す。合格: 誤り 0、かつ
バンクを替えると内容が替わる。
reg1 = 10h が -96 系で効かない（10h は資料では内蔵機のみ）場合は、
A0000h（GR06 のメモリマップ = 00b/01b）をチップ側の VGA 窓として
試す（B5EWIN5 の考えを、上の方法でやり直す）。

**B5（表示）** — リレー（reg3 bit1）で出力を切り替え、640×480 8bpp の
既知のモードを CRTC・シーケンサ・DAC に設定して色帯を出す。写真で確認。

**B6（BLT）** — 画面内の塗り（ROP 0/FFh）と画面→画面コピー、
システムメモリ→画面（BLT モードの system source）を試し、所要時間を
タイマで測る。

各段階は保守者が実機で通したと言うまで次へ進まない。vaeg 側は B2 が
実機で通るまで実装しない（B0 の判定には今の vaeg の「未接続 = FFh」で
足りる）。

### 5.3 vaeg への実装（B2 以降）

- `cbus/b3e02.c`（C99）。0FA2h/0FA3h、0902h/0904h、0C50h–0C5Fh、
  0B54h/0B55h/0B5Ah、0D54h/0D55h/0D5Ah を iocore に登録。ID 60h。
- 実機で確かめた段階だけを再現する。窓は B4 の結果に合わせ、153h
  SMBC = 0（または 152h bit3）のときだけ見えるようにする。
- 設定項目（既定 OFF）。状態保存に新しいセクションを追加し、既存の
  形式は変えない。
- 5.2 の各プローブを、ボードなし・ありの両方で CTest にする。

## 6. 判定欄（実機）

| 日付 | 機種 | 段階 | 結果 | ログ |
|---|---|---|---|---|
| 2026-11-12 | VA | B0 | PASS（27 ポートすべて FF、終了コード 0）。B0000h（153h: 41h→40h→41h）・E0000h（152h: 02h→0Ah→02h）の窓はどの番地も 2 通りとも FFh。ボードを挿していたかは要確認（8.2） | B3LOG 1 |
| | | B0.5 | 未実施 | |
| 2026-11-12 | VA | B1 | PASS。ID 60h（chip 40h、bus 10b、版 0）、index 00h/01h/03h/04h とも読み戻し一致。reg01 = 00h、reg03 = 00h、reg04（DIP）= 00h（16MB モード、窓 F00000h） | B3LOG 1 |
| 2026-11-12 | VA | B2 | FAIL（終了コード 10h）。reg03 = 00h のまま起こし。起こしの前後とも 0902h/0904h/0C5xh/0B54h/0D54h、CR27、SR06 がすべて FF | B3LOG 1 |
| 2026-11-12 | VA | B3 | 停止（CR27 = FFh、終了コード 10h） | B3LOG 1 |
| | | B4 | | |

## 7. 出典

- PC-9800 I/O 資料「ウィンドウアクセラレータ制御」: 0FA2h/0FA3h
  （reg00–04）、0FAAh/0FABh、0FACh、0902h、0904h、0C50h–0C5Fh、
  0D54h/0D55h/0D5Ah、Sequencer・GC・CRTC の index 表
- PC-88VA テクニカルマニュアル 2.2（152h/153h の RBC・SMBC・SMM・GMSP）、
  3.3（ROM エリア・システムメモリエリア・RAM エリアのバンク切換え）、
  3.6（拡張スロットで拡張できるメモリ）
- NP2kai `wab/cirrus_vga.c`、`wab/cirrus_vga_extern.h`（CIRRUS_98ID_96 =
  60h）、`wab/wab.c`（0FACh）
- vaeg M144（16 bit 全デコード、未接続 I/O = FFh）、M146（SMBC = 0 の
  バススロット）、M143（未接続メモリは直前のバス値）
- 保守者提供の既存プローブ 17 本（M04-G〜M06）の逆アセンブル

## 8. B0〜B3 のプローブ（実装記録）

`demos/b3e02/`（NASM、MS-DOS 4.0 の `.COM`）。共通部は `b3e02.inc`、
ディスクは `build.sh` が msdos4-va.2 の vanilla 複製に入れて作る
（Git の外）。使い方と書き込む範囲は `demos/b3e02/README.md` と、
ディスク上の `B3E02.TXT`。

| プログラム | 段階 | 内容 |
|---|---|---|
| `B3E0.COM` | B0 | 3.1 の全ポートを 6 通りのコードで読む（`FF` / `D:xx` / `F:…`）。`cli` のまま 153h SMBC = 0 で B0000h、152h bit3 で E0000h の 4 番地を 2 通りで読み、152h/153h を戻して読み戻す。VA・VA2 とも実行 |
| `B3E1.COM` | B1 | 0FA2h に 00h/01h/03h/04h を書いて読み戻し、0FA3h を読む。ID・窓・リレー/MMIO・DIP を分解して表示。元の index を戻す |
| `B3E2.COM` | B2 | 起こしの前後で 0902h/0904h/0C52h/0C54h/0C5Ch/0C5Eh/0B54h/0D54h、CR27（0B55h と 0D55h の両方、判定は Misc bit0 で選んだ側）、SR06 の 12h/0Fh。最後に起こしを戻し（0904h = 00h、0902h = 元の値、0904h = 20h）、GC index が起こし前の読み値に戻ることを確かめる |
| `B3E3.COM` | B3 | 起こしと CR27 確認の後、SR06 を解錠して SR00–1F、GR00–3F、CR00–3F を読む（本書 5.2 の GR00–0E・CR00–27 を含む上位集合）。再施錠し、起こしを戻す |

本書 5.2 からの追加: B3E2 と B3E3 のオプション `/MMIO`・`/RELAY` は、
起こしの前にボード reg 03h の bit0（MMIO）・bit1（リレー）を立て、
最後に元の値へ戻して読み戻す。既存プローブ（4.1、B3COMBO 以降）が
起こしの前に reg03 = 03h を書いていたことに対応する `[INFERRED]` 変種
（8.2）。合否の基準は変えない。

起こしの戻し方（0902h = 00h）と、0902h が 0904h = 00h の間だけ読める
という扱いは `[INFERRED]`。B2 のログの WAKE/AFTER 行で確かめる。

### 8.1 机上確認（ハードウェアなし）

`demos/b3e02/smoke.py`（Unicorn、最小 DOS、本書の仮定に基づく偽の
ボード。計測ではない）で、制御の流れ、表示が印字可能な ASCII だけで
あること、ログが画面と同じであること、後始末（152h/153h、ボード
index、reg 03h、POS102、SR06、VGA の index）を確かめた。

| 実行 | 終了コード |
|---|---|
| B3E0（ボードなし、VA / VA2） | 00 |
| B3E0（ボードあり） | 10（駆動あり） |
| B3E0（浮いたバスの模擬） | 11 |
| B3E1（ボードなし / VA2 / ボードあり / 浮き / 不正な引数） | 03 / 02 / 00 / 03 / 01 |
| B3E2（ボードなし / あり / `/MMIO` / チップ無応答 / VA2） | 03 / 00 / 00 / 10 / 02 |
| B3E3（ボードあり / チップ無応答 / ボードなし） | 00 / 10 / 03 |
| B3E2（reg03 = 03h が必要な模擬: なし / `/MMIO` / `/RELAY` だけ必要な模擬で `/RELAY` / `/MMIO /RELAY`） | 10 / 10 / 00 / 00 |
| B3E3（reg03 = 03h が必要な模擬: なし / `/MMIO /RELAY`） | 10 / 00 |

vaeg での実行は未実施（今の vaeg にはボードがなく、B3E0 は 00、
B3E1〜B3E3 は 03 になるはず）。

### 8.2 VA 実機 1 回目（2026-11-12、build 19c0519d）

ログ（`B3LOG.TXT`）は Git の外（`~/work/pc88va/tmp/m148/B3LOG.va1.TXT`）。
結果は §6。

- `[MEASURED]` ボード制御 0FA2h/0FA3h は VA のスロットに届く: I/O
  0000h–0FFFh のサイクルはスロットに出ている（3.3 の未確認点の一部が
  解消）。ID 60h、DIP = 00h（16MB モード、窓 F00000h）。
- `[MEASURED]` reg03 = 00h のまま起こし（0904h = 00h、0902h = 01h、
  0904h = 20h）をしても、0902h（setup 中を含む）、0904h、0C5xh、CR27、
  SR06 はすべて FF。チップは応答しない。
- `[STATIC]` 既存プローブ B3COMBO 以降は、起こしの**前に** 0FA3h reg03 =
  03h（リレー＋MMIO）を書いており、その後のプログラムは起こし後の
  0C5Eh = 0Fh を前提にしている（4.3）。今回の B2 との違いはこの書き込み
  だけである。`[INFERRED]` reg03 の bit0 か bit1 がチップの I/O 応答の
  条件である可能性が高い。次は B3E2 `/MMIO`、`/RELAY`、`/MMIO /RELAY`
  の順に試し、どの bit が要るかを切り分ける。
- `[MEASURED]` 153h SMBC = 0 の B0000h と 152h bit3 の E0000h は、どの
  番地もコードを変えた 2 通りとも FFh。M143 のメモ（未接続のメモリは
  直前のバス値）と違う。ボードを挿していた場合はボードが FFh を返した
  可能性もあるので、B0 がボードなしだったかを確認してから扱う。
- 152h の入りの値は 02h（ROM エリア 0 のバンク 2）。B3E0 は bit3 を
  立てて 0Ah にし、02h に戻した。
- 時計は 2000-01-31 00:00 台。B3E0 から B3E1 まで 33 秒なので、同じ起動
  の中で続けて実行したように見える。その場合 B0 の時点でボードが
  挿してあり、0FA2h/0FA3h が B0 では FF、B1 の書き込み前の読みでは
  00h/60h と食い違う。B0 がボードなしだったか、要確認。
