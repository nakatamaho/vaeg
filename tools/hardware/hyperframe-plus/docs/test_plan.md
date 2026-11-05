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
EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
-->

# 試験計画・ソフトウェア検証記録

## 判定の意味

- **PASS**: 指定した契約・対象について期待結果が一致した。
- **FAIL**: その契約に不一致があった。直ちに実機カード故障とはしない。
- **BLOCKED / INCONCLUSIVE**: 前提・入力・所有権等が未確認で、結論を出せない。
- **NOT_RUN**: 実施していない。

終了コード 0、MZ signature、画像らしき出力、mock 成功だけで
`[HARDWARE-TESTED]` にしない。M145 の gate はこの試験表だけでは承認されない。

## 実行済みのソフトウェア試験

生成物と完全ログは checkout 外の M145 scratch に保存する。
`$HOST_OUTPUT` 等は再利用可能な明示的出力先 binding で、原資料の場所ではない。
最終 source ID・EXE identity は build report と task checkpoint に記録する。

| 試験 | 実際の対象 | 結果 |
|---|---|---|
| strict C89 host | Apple clang 21.0.0 / arm64、`-std=c89 -pedantic -Wall -Wextra -Werror -O2` | PASS、下記 6 群 / 2050815 assertions |
| CLI | 同じ CLI を mock のみとリンク | PASS、22 cases |
| 入力 validator | 原本不要の小さい合格 MZ fixture と一変更負例 | PASS、9 tests |
| ASan/UBSan | 同じ host tests、`-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer` | PASS、検出報告なし |
| DOS build | 固定 Open Watcom v2 2026-08-01 の DOS compiler / small model / 80186 / `-za` | BUILT、実 MZ、relocation/entry 範囲正常 |
| 生成 I/O 命令 | 自作 DOS backend の WDIS listing | PASS、EC / EE / EF の実命令を検査 |
| 生成 backend 限定実行 | MZ 内と完全一致する leaf code、Unicorn 2.1.4、最大 256 instructions/case | PASS、25 cases、全 IN/OUT 捕捉 |
| DOS smoke | DOSBox 0.74-3、dummy video、ボード操作を許可しない経路のみ | PASS、6 cases、guest exit と結果文字列を検査 |
| JPGVHF 初回入力 | 当時は原本欠落 | 歴史的 BLOCKED、`HFRE_INPUT_MISSING` / exit 2 を保持 |
| 三原本の後続解析 | DOC、J1 MZ/hash と J1/H1/D1 の限定制御フロー | J1 入力 exit 0、限定 trace 53 cases PASS、下記参照 |
| trace 検証器 | 原本不要の合格 fixture と一変更負例 | 12 tests PASS（Unicorn 2.1.4） |
| Open Watcom 1.9 | compiler が PATH 上になかった | NOT_TESTED |
| PC-9801 本体・HyPER-FRAME+ | 機材での実行なし | NOT_RUN |
| PC-88VA/V30、VA D88 | 本段階の対象外 | NOT_RUN |

実行コマンド（このツールのディレクトリで実行）:

```sh
make OUT="$HOST_OUTPUT" test
make OUT="$SANITIZER_OUTPUT" \
  CFLAGS='-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer' test
python3 -B build_dos.py --archive "$OPENWATCOM_ARCHIVE" \
  --dosbox "$LOCAL_DOSBOX" --output "$NEW_DOS_OUTPUT" --build-id 00000000
python3 -B tests/test_dos_backend.py \
  --exe "$NEW_DOS_OUTPUT/run/hftest.exe" \
  --listing "$NEW_DOS_OUTPUT/run/hf_dos.lst" --output "$NEW_BACKEND_REPORT"
python3 -B verify_reference.py --references "$REFERENCE_DIR" \
  --output "$NEW_INPUT_REPORT"
```

`00000000` は開発検証用ラベルであり、commit ではない。
引渡し build は確定した source commit の先頭 8 桁で作り直す。
後続の文書・報告だけの変更では、source file hash が変わらないことを照合し、
既に得た挙動試験を再利用する。SDK や実行物を Git/CI に載せない。

確定 source は
[`d321baa646c24da99d36afc2fb545a5633a82056`](https://github.com/nakatamaho/vaeg/commit/d321baa646c24da99d36afc2fb545a5633a82056)。
同じ command の `--build-id d321baa6` と新しい出力先で exit 0。
`HFTEST.EXE` は **17312 bytes**、SHA-256
`b8501af0d237cde9d7dd551d02acc6eccd0cd5e08329c6ead380c593e102c775`。
MZ header 48 bytes、relocation 4、entry 0000:122Ah、overlay 0 を検査した。
最終 EXE で上記六件の DOS smoke と 25 件の限定命令実行を再確認した。
開発版との byte 差は八文字の build label のみで、source/worker identity が
不変の host 試験は保持した証拠を再利用した。これは実機 PASS や G145 承認ではない。

### 六つの C 試験群

1. **画素順序・読値**: 各成分で D7→D6→D5→D4 を出し、D8/DA/DC の
   byte データを一つずつ扱う。端点 (639,399) の高位 byte も照合。
   backend は read script を注入でき、指定した値・順序を検査する。
2. **拒否条件**: 負座標、x=640、y=400、RGB=256、NULL、容量不足、
   権限不足・不正権限値を拒否。ボード操作件数がゼロであることを検査する。
3. **二画素 readback**: 150 I/O の合格 fixture を先に実行し、
   各 I/O 一箇所だけを失敗させた 150 負例で、正しい status と cleanup を検査する。
   保存失敗ならデータ OUT ゼロ。比較失敗でも両方の復元を試行する。
   復元の silent drop、復元読値の一 bit 破損、アドレス alias、固定 FF も区別する。
   データ後にアドレスを進める別モデルでも成功し、保持を仮定しないことを確認する。
4. **word と停止**: 座標 word transaction と byte transaction を別記録にする。
   word は追加権限必須。D0=00→EE=00 の順、第一出力失敗後の第二出力試行を検査。
   これは停止列の実機上の正しさではなく送出契約の検査である。
5. **bank 算術**: mock の六値 C0–C5、204 行の 511/512 境界、区間分割、
   205 行・最終画素、全 256000 画素の segment:offset を照合。
   mapping 先の実メモリは一度も dereference しない。
6. **pattern**: 進捗・開始前中断・行境界中断・範囲拒否・書込み失敗。
   640×400 の合成画素を全照合し、3840000 byte OUT、IN/word OUT ゼロを確認する。

### CLI と DOS の検査範囲

CLI の 22 cases は help/info/default、承認済み off/readback/word/pattern、
各確認の一文字違い、EOF、改行欠落、埋込み NUL、余分な引数・綴り違い、
full の追加確認・開始前中断、init/banktest の拒否を含む。
正しいケースを通してから一項目だけを変更し、exit、`HF_RESULT`、操作数を照合する。

DOS smoke は default/info=0、init/banktest=2、off の空入力取消=2、
unknown=3 の六件。DOSBox worker の exit 0 だけでなく、batch の guest ERRORLEVEL と
各ファイルの `HF_RESULT` / build banner を検査した。
**この DOSBox は PC-9801/ボード/実表示器の代役ではない。**

限定命令試験は `dos_in8_`, `dos_out8_`, `dos_out16_` の code を実 MZ に照合してから、
Watcom の register calling convention で実行する。
byte/word の port、値、幅、返り値、読値格納先と、EC・不正ポート・不正 word 範囲の
無出力拒否を検査する。OS、モニタ、バス、待ち時間は対象外。

### 開発途中の不一致も保存

初回 CLI 試験は、リダイレクトした入力の改行が echo されず、
`HF_RESULT` が確認プロンプトと同じ行になるため FAIL した。
確認関数で改行を出し、行の完全性も検査するよう修正した。

初回 DOS 命令検査は、`inp/outp/outpw` が library call になり、
backend object に期待する opcode がないため FAIL した。
SDK header の intrinsic 条件を確認し、backend のみ `-oi` を明示して再検査した。
それぞれの失敗ログを残し、成功へ書き換えていない。

また、List 1 が一成分ごとの API であることに合わせ、RGB の各成分で
座標を再選択する実装にした。以前の単一選択・RGB 連続転送の開発生成物は
最終版ではなく、保持を仮定しない最終実装で検査を更新した。

## 原本三件の逆ASMを裏付ける後続検証

[操作経路の保存記録](reverse_engineering.md)を技術説明の集約先とする。
J1/H1/D1 の原 archive を保全し、member の hash と CP932 DOC を確認した。
J1 の archive/EXE/DOC は既提示 hash と一致し、MZ の header/entry/relocation と
二点の明示 byte landmark の検証は exit 0 / `BYTE_CHECK_PASS`。
遠隔側との identity 照合は認証で BLOCKED。解析したのはローカルに存在したコピーである。
初回の入力欠落、今回の認証失敗、51-case 開発 trace、最終 53-case をすべて保持する。

原本の code/data 境界、caller、option/global、分岐を静的に追跡した後、
`trace_reference.py` で **53 cases** を限定実行した（exit 0）。
`instructions` の合計 5393136 は stub 境界を含む code-hook 訪問数で、実機命令数ではない。
全 IN/OUT を捕捉し、方向・port・幅・値・順序を期待列と完全比較した。
返り値、出力寸法/global、読出し buffer、外部関数 stub の順序も該当ケースで検査した。
対象は J1 の初期化 17、word 縦更新 12、byte 縦更新 2、停止/終了 4、行描画 2、
D1 F1 検出 4 と leaf 7、H1 prefix/書戻し 5。機械生成・ソフト動作の一致のみである。

```sh
python3 -B verify_reference.py --references "$REFERENCE_DIR" --output "$NEW_INPUT_REPORT"
python3 -B tests/test_trace.py
python3 -B trace_reference.py --bindings "$PRIVATE_BINDINGS" --output "$NEW_TRACE_OUTPUT"
```

Unicorn 2.1.4 の環境で 12 fixture tests も exit 0。
各負例は先に合格する独立命令 fixture/content から一項目だけを変え、
`HFRE_TRACE_MISMATCH`、`HFRE_READ_SCRIPT`、`HFRE_EXECUTION_RANGE`、
`HFRE_RETURN_MISSING`、`HFRE_UNEXPECTED_INTERRUPT` の該当コードを照合する。
初回 IN 不一致 fixture の ctypes 補助警告は、整数の callback return と
停止エラー通知を分離して修正した。元の警告ログも保持し、最終検査は警告なし。
content と worker を分離し、最後に合成検査を行う。Git history/private corpus を
負例の前提にしない。Unicorn がなければ worker 試験は SKIP/NOT_RUN、campaign は exit 77。

入力・runner/validator・Unicorn library・比較契約・出力の digest は Git 外の
report/manifest に保持する。完全な EXE/COM entry、DOS/BIOS、実機 I/O、
H1 の全 VRAM/palette、D1 の全 dispatcher、J1 の EMS round trip は実行していない。
block-scroll I:7B64 等の静的所見を、この 53 ケースに含めたとは報告しない。

HFTEST の `src/`、DOS build inputs、既配布 MZ は変更していない。
`d321baa6` と source digest が不変であることを照合し、前節の C89/sanitizer/DOS
結果は再利用する。今回それらや vaeg 本体、全 demo/media/presenter を再実行したとの
主張はしない。原本 trace の PASS は G145 や `init`/bank aperture の許可ではない。

## 実機前提チェック — 現在すべて未資格/未実施

- 本体型番・CPU・DOS 版・メモリ構成・常駐プログラム・拡張カードを記録する。
  Windows、memory manager、EMS/UMB、別デバイスの占有を未知のまま放置しない。
- カードの製品名、基板 revision、刻印、SW1 の物理位置を記録する。
  switch の意味は manual 等で確認し、写真だけから推定して変更しない。
- ジャンク・動作未確認という状態を維持し、ソフトウェアの無応答を故障確定にしない。
- 接点、装着、固定は電源 OFF・電源プラグを抜いて確認する。
  無理に押す、曲げる、削る、固定ねじで引き込むことはしない。
- モニタ・ケーブル・pinout・同期方式・レベル・切替経路の適合を確認する。
  **24.83 kHz 級を含む必要周波数への実際の対応**を表示器の仕様で確認する。
  他機種での映像経験や単なる端子の一致を根拠にしない。
  未対応・不明なら映像出力実験を開始しない。
- I/O 所有権と初期 pixel mode の成立を独立に確認する。
  特に VA の低位 system-area ports を PC-98 と同じ空き領域とみなさない。
- 使用 EXE の完全 hash、source commit、build banner、各 command、
  電源再投入境界、stdout、guest ERRORLEVEL、写真を対応させる。
  private な実機記録は Git/hosted CI の外へ保存する。

## 条件成立後の段階的な実機計画

### 0. 無 I/O の起動

`HFTEST info` で DOS への正常復帰と build identity を確認する。
表示は対象指定であり、ROM 等による実機自動識別ではない。
これだけで card PASS を付けない。

### 1. 必要なら元 viewer を別枠の基準にする

これは**今すぐ実行する指示ではない**。J1 原本と DOC は今回照合したが、対象カード、
初期状態、機材の互換性が確認できる PC-9801 実機でのみ別途判断する。
候補操作は `JPGVHF -m6 TEST.JPG`、画像は自作の小さな synthetic JPEG とする。
原文の ESC による表示維持と、他の終了キーによる出力停止の説明は確認した。
命令側の停止列の呼出し有無も一致したが、実際の映像・画像保持はまだ確認していない。
HyPER-FRAME III の `-3` 対応から「+ と同一」とは結論しない。
本作業では viewer 全体は実行していない。

### 2. 次に薦める実機実験: 二画素 byte readback 一回

**上記前提と既知 pixel mode が成立した場合に限る。**
(0,0)/(1,0) を保存し、12/34/56 と A9/CB/ED を書いて比較する。
完全な stdout と ERRORLEVEL を記録し、復元確認が両方成立したか確認する。

- 一致・復元確認成立: この二画素・byte 操作・初期状態についてのみ PASS。
- 比較不一致: 試験 FAIL。未装着、初期化不足、port conflict、fault 等の原因は未確定。
- 復元不一致: FAIL。続行せず、残存状態が未知であることを記録する。
- 初期化・所有権が不明: BLOCKED。確認文字列を入力して強行しない。

### 3. 小 pattern、停止候補、後続段階

次に進む承認が得られた場合だけ、32×32 pattern と適合表示器での観察を行う。
期待する色・位置・端・周辺の変化を記録する。単に何か映っただけでは PASS にしない。
原本照合後の `off` は別 command として前後の表示を比較する。
全画面、word、初期化はそれぞれ別の実験であり、小領域 PASS に含めない。

`init` は原本の限定制御フローを確認しても、冷間起動・機材・初期状態等の
実機前提が資格を得るまで BLOCKED。
`banktest` は窓の予約と memory map を証明できるまで BLOCKED。
PC-88VA は、別途 bus/port/enable/CPU/表示の適合を調べた後の後段である。
