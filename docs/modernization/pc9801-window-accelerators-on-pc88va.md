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

# PC-9801 用ウィンドウアクセラレータを PC-88VA で使う — 調査のまとめ（M148）

2026-11-12、M148 で一時クローズ。経過と全ログの要約は
[`docs/agents/tasks/M148_b3e02_va_connection.md`](../agents/tasks/M148_b3e02_va_connection.md)
（§6 判定欄、§8、§9）、プローブは [`demos/b3e02/`](../../demos/b3e02/README.md)
と [`demos/wgnv/`](../../demos/wgnv/README.md)。

根拠の区分: `[MEASURED]` VA 実機（以下すべて PC-88VA、F000:FFFE = FFFFh）
での計測、`[DOC-VA]` PC-88VA テクニカルマニュアル、`[DOC-98]` PC-9800 の
I/O 資料（ウィンドウアクセラレータ制御）、`[DOC-USER]` 保守者が示した
2000 年の掲示板の書き込み、`[DRIVER]` メルコ配布の Windows 95 ドライバの
独自の読解、`[INFERRED]` 推論。

## 1. 結論

- **NEC PC-9801B3-E02（= PC-9801-96 相当、CL-GD5428、C バス）**
  - VA の I/O からチップには届く。ボード制御 reg03 bit0 = 1 と起こし
    手順で CL-GD5428（CR27 = 9Bh）のレジスタを読み書きできる。
    `[MEASURED]`
  - VA の CPU から VRAM に届く経路は見つからなかった。ボードの VRAM 窓は
    F00000h〜F6FFFFh（1MB より上）だけで、20 bit の VA からは原理的に
    当たりにくい。残る可能性（スロットの A20〜A23 が 1 で、窓が VA の
    40000h/60000h に重なる）は判定できなかった。
  - **実用（画面表示）には至らない。vaeg には実装していない。**
- **メルコ WGN-V2/V4（S3 Trio64V+、C バス）**
  - ドライバに「E バンク」モード（窓 E0000h、32KB）がある。`[DRIVER]`
    E0000h は VA でも 152h bit3 でスロットに出せるので、**B3-E02 より
    見込みが高い。** 実機試験用のプローブ（WV0/WV1）は用意したが未実行。

## 2. PC-88VA 側について分かったこと

| 事項 | 内容 | 区分 |
|---|---|---|
| I/O 0000h–0FFFh のスロット出力 | 0FA2h/0FA3h、0902h、0C5xh、0B54h/0B55h がボードに届く | `[MEASURED]` |
| 空きスロットの I/O | FFh（M144 と一致） | `[MEASURED]` |
| 空きスロットのメモリ | 153h SMBC = 0 の B0000h/A0000h、152h bit3 の E0000h、1D0h = 01h〜FFh の 80000h–9FFFFh は、コードを変えて読んでも FFh。M143 のメモ（直前のバス値が見える）とは違う | `[MEASURED]` |
| MS-DOS 4.0 実行中の 152h | 02h（ROM エリア 0 のバンク 2）。bit3 を立てて E0000h をスロットに出し、元に戻せる | `[MEASURED]` |
| 153h | 41h。SMBC = 0 にして戻せる | `[MEASURED]` |
| RAM バンク制御 1D0h | 書き込み専用。1〜255 で 80000h–9FFFFh がスロットに出る。バンク番号がボードにどう伝わるかは資料に記載なし。上位アドレス線には出ていない（B3-E02 の窓は全バンクで見えず） | `[DOC-VA]` `[MEASURED]` |
| スロットの A20〜A23 | 未確定（テスタなし）。0 なら 1MB より上の窓には当たらず、1 なら主 RAM 00000h/20000h/40000h/60000h に重なる | — |
| 時計 | 試験機は電池切れで、電源投入ごとに 2000-01-31 00:00 から始まる | 保守者 |

## 3. PC-9801B3-E02

### 3.1 ボード制御と起こし

| 事項 | 内容 | 区分 |
|---|---|---|
| 0FA2h/0FA3h | index/data。index は読み戻せる | `[MEASURED]` |
| reg00 | 60h（-96/B3-E02）。chip 01xx0xxxb = CL-GD5428、bus 10b = C バス、版 0 | `[MEASURED]` `[DOC-98]` |
| reg01 | 電源投入時 00h（窓を開いていない）。書いた値は保持される | `[MEASURED]` |
| reg03 | 電源投入時 00h。**bit0 = 1 がチップと 0902h の I/O 応答の条件**（資料の名前は「メモリマップト I/O」）。bit1 はリレー（動作音を確認） | `[MEASURED]` |
| reg04 | 00h。bit3–2 = 00b が DIP SW1/SW2 = ON/ON（F00000h）に合う | `[MEASURED]` `[DOC-USER]` |
| DIP | SW1/SW2 で窓 F00000h/F20000h/F40000h/F60000h。SW3〜SW8 は意味なし。**15MB モードはない** | `[DOC-USER]` |
| 起こし | reg03 bit0 = 1 の後、0904h = 00h、0902h = 01h、0904h = 20h。0902h は 0904h = 00h の間読める（00h→01h）。戻すと（0904h = 00h、0902h = 00h、0904h = 20h）VGA ポートは FF に戻る | `[MEASURED]` |
| reg03 bit0 = 0 | 起こしても 0902h、0C5xh、CR27 はすべて FF | `[MEASURED]` |

### 3.2 起こした直後のチップ（BIOS なし、電源投入時の値）

`[MEASURED]`（SR06 の 12h は解錠のために書いた値）:

```
misc=00 feat=00 stat0=10 dacstate=00 sr-idx=07 gr-idx=0F cr-idx=3F (CRTC 0B54h)
SR00  00 00 00 00 00 0F 12 00 00 00 00 66 5B 45 7E 44
SR10  00 00 00 00 00 00 F0 39 00 01 00 3B 2F 30 33 1C
GR00  0F 0F 0F 1F 03 73 0F 0F FF 00 00 00 FF 00 00 00
GR10  FF FF 00 00 00 00 00 00 00 00 00 00 00 00 00 00
GR20  FF 07 00 00 FF 0F FF 0F 24 24 04 00 24 24 04 00
GR30  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
CR00  FF FF FF FF FF FF FF FF 7F FF 3F 7F FF FF FF FF
CR10  FF 00 FF FF 7F FF FF 6F FF 00 00 00 23 22 21 20
CR20  1F 1E FF 00 00 51 1F 9B FF FF FF FF FF FF FF FF
CR30  FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF
```

- CR27 = 9Bh（CL-GD5428）。SR06 は 12h を書くと 12h、00h で 0Fh。
- SR00 = 00h（シーケンサはリセット中）、SR07 = 00h（拡張モードなし）。
- `[INFERRED]` SR0B–0E/SR1B–1E は VCLK0–3、SR1F = 1Ch は MCLK の標準
  初期値に見える。SR0F = 44h、SR17 = 39h の意味は未確認。CR1C–CR21 は
  index とともに 1 ずつ減る値を返す。
- 設定（SR00/02/04/07、Misc、GR01–0B）は書いた値で読み戻せ、元に戻せる。

### 3.3 VRAM の窓（いずれも不成立）

| 段階 | 方法 | 結果 |
|---|---|---|
| B4a | reg01 = 10h（0B0000h、資料では内蔵機のみ）を SMBC = 0 の B0000h で。チップの VGA 窓（GR06 = 05h）を SMBC = 0 の A0000h で。2 バンクのパターン試験 | 両方とも全部 FFh |
| B4b | 1D0h = 01h〜FFh の全バンクで、reg01 = 00h/E0h/10h の 80000h、88000h、90000h を読む | 全部 FFh、差なし |
| B4c L | BLT で VRAM を塗り、候補の窓で CPU が読んだ後の CR22（ラッチ）を見る | CR22 は最初の BLT 以降、対照も含め常に 00h。BLT が動いたか確かめられず**判定不能** |
| B4c M | reg01 = 40h〜67h（-85/-91 の低位窓の形）で C0000h–E7FFFh の 4KB ブロック | 全部 FFh、差なし |

`[INFERRED]` 窓の番地は上位 4 bit が E/F の 24 bit アドレスなので、
VA の A0000h–EFFFFh（SMBC、152h で出せる範囲）には A20〜A23 がどちらでも
当たらない。既存の古いプローブ（E0000h、A0000h/B0000h の窓試験）は、
ボードの窓を使う限り成り立たなかった。残る経路は A20〜A23 = 1 での
VA 40000h/60000h だが、そこは VA 内蔵 RAM で CPU にデータは戻らない。

### 3.4 判定を残したまま閉じる点

- スロット A20〜A23 の電位（テスタで測れば確定）。
- ボードの映像出力（リレー = 1）をマルチスキャンのモニタで見れば、BLT と
  VRAM の動作、VA 40000h/60000h への書き込みが届くかを確かめられる（B3E7
  として構想、未作成）。

## 4. メルコ WGN-V2/V4（S3 Trio64V+）

`[DRIVER]` メルコの WGN-V シリーズ Windows 95 ドライバ（Ver 2.012）の
表示ドライバと仮想ドライバ（E バンク版を含む）、設定ファイルの独自の読解。
ドライバ本体・逆アセンブルは Git の外にあり、ここには結果だけを書く。

- ボードの検出: ボード位置 n = 0, 2, …, 0Eh について 51E1h+n が C3h/C4h、
  59E1h+n が 04h/F4h。以後のポートは 53E0h/53E1h、54E0h/54E1h、
  55E0h/55E1h（それぞれ +n）。奇数番地が index、偶数番地が data の組として
  使われている。
- 窓のモード: `SYSTEM.INI` の `[DISPLAY] VRAM-MAP=2` で「E バンク」
  （PC-9801FA、エプソン機向け）。E バンクでは窓が E0000h、32KB。通常は
  F00000h、64KB。`IO-WAIT` はボードレジスタ 10h（0 または 10h）。
- VGA/S3 のレジスタはポートではなく窓の中にメモリマップされる。窓内の
  オフセットは本来のポート番号そのまま（3C0h〜3DAh）。S3 拡張ポート
  （9AE8h など）は通常モードで同じ下位 16 bit、E バンクで下位 14 bit。
- 54E0h の data の bit3–2 が窓の中身を選ぶ: 00b = レジスタ、10b = VRAM。
  ドライバはレジスタに触る前に 00b、後に 10b に切り替える。
- PC-98 では E0000h が 16 色モードの第 4 プレーンと重なるので、ドライバは
  ポート 6Ah で 16 色モードを切る。VA では 152h bit3 で代わりになるので
  6Ah は使わない（VA で 6Ah に何があるかは未確認）。
- E バンクの初期化手順（ボードレジスタの書き込み、窓オフセット 0004h への
  03h、Misc = 23h、CR38/CR39 の解錠、CR2E/CR2D = 11h/88h の確認、SR08、
  SR11）は [`demos/wgnv/tests/wv1.asm`](../../demos/wgnv/tests/wv1.asm) の
  先頭の注釈に、VA で試す形で書いた。
- S3 の描画エンジンは 8514/A 互換で、ピクセル転送ポートを使えば窓なしでも
  I/O で画素を転送できる（チップ一般の性質。このボードでのポート配置は
  未確認）。

プローブ（未実行）:

| プログラム | 内容 |
|---|---|
| `WV0.COM` | 読むだけ。ボード位置 n ごとに 51E1h、59E1h、53E0h–55E1h、E0000h 窓の 8 バイト |
| `WV1.COM` | E バンクの初期化、CR2E/CR2D、レジスタ値、窓の VRAM にパターンを書き、レジスタ表示へ切り替えて戻した後に読み戻す。ボードは設定されたまま残るので、終了後に電源を入れ直す |

## 5. vaeg への実装について

- B3-E02 は I/O の振る舞い（ID 60h、reg03 bit0 による応答の有効化、
  起こし、CR27 = 9Bh、SR06、§3.2 の初期値）までは実機で確かめたが、
  VRAM に届かないので VA のソフトから使い道がない。実装しない。
- WGN-V は WV0/WV1 の実機結果を見てから判断する。

## 6. 記録の所在（Git の外）

- 実機ログ: `~/work/pc88va/tmp/m148/B3LOG.va1*.TXT`（6 回分）
- 古いプローブ 17 本の抽出物と逆アセンブル: `~/work/pc88va/tmp/m148/`
- WGN-V ドライバの展開物と逆アセンブル、InstallShield 3 の展開
  スクリプト: `~/work/pc88va/tmp/m148/wgn/`
