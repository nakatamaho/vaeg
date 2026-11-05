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

# HFTEST — HyPER-FRAME+ の最小 DOS 試験ツール

**M145 / 実験段階 / G145 NOT_READY。PC-9801 系が先、PC-88VA は後段。**
公開論文の byte 画素 I/O を独立実装した C89 プログラムである。
原本 JPGVHF は今回未入手のため、初期化の逆解析は BLOCKED。
メーカーソフトなしで電源投入直後から動く完成ドライバではない。

実機カードは動作未確認であり、カードの存在や健全性を自動判定できない。
初期化済み pixel mode、ポート所有権、ホスト・カード・配線の確認が前提となる。
**PC-88VA、別の Hyper/Super Frame 系、WSR-E、PC/AT では実行を許可しないこと。**
ビルドや mock の成功は実機確認ではない。

## 配置

既存 vaeg の root の README/build を置き換えず、本ディレクトリに独立配置した。
依頼の大文字 Markdown 名はリポジトリの命名規約に合わせて小文字にした。
DOS backend の名前は、実 DOS の 8.3 制約のため `hf_dos_io.c` ではなく
`hf_dos.c` とした。

| ファイル | 内容 |
|---|---|
| [docs/hyper_frame_plus_spec.md](docs/hyper_frame_plus_spec.md) | 根拠区分、画素 I/O、バンク・segment 算術、安全境界 |
| [docs/reverse_engineering.md](docs/reverse_engineering.md) | 入力台帳、未照合ランドマーク、MZ 検証・追跡手順 |
| [docs/test_plan.md](docs/test_plan.md) | ソフトウェア検証、実機条件、判定基準、未実行項目 |
| `src/hf_io.h`, `src/hf_io.c` | デバイス API、検査、復元、pattern、純粋なバンク計算 |
| `src/hf_dos.c` | 16-bit DOS の `inp/outp/outpw` と非ブロッキング DOS キー入力 |
| `src/hf_test.c` | 既定 no-I/O の CLI、明示的確認、結果出力 |
| `tests/hf_mock_io.*`, `tests/test_hf.c` | 画素モデル、全 I/O 記録、指定読値・失敗注入、決定的試験 |
| `tests/mock_main.c`, `tests/test_cli.py` | 実 CLI を mock のみにリンクする安全性試験 |
| `tests/test_reference.py` | 原本不要の MZ/hash/landmark 合格 fixture と単一変更負例 |
| `tests/test_dos_backend.py` | 自作 MZ と一致する生成命令の限定実行、I/O はすべて捕捉 |
| `verify_reference.py` | 原本を変更しない入力検証。欠落は BLOCKED |
| `build.bat`, `build_dos.py`, `Makefile` | DOS / host ビルドと検査 |

C ソース・コメント・CLI・テスト・スクリプトは英語、説明文書は日本語。
論文、原本 viewer、旧実験コード、ドライバ、画像、ROM は収録しない。
エミュレータ本体への実装、D88 作成、release 公開もこの成果物には含めない。

## ホストビルドと試験

C89 コンパイラ、make、Python 3 が必要。`OUT` は checkout 外に明示する。
この target は DOS backend をリンクせず、ホストの実 I/O を実行しない。

```sh
cd tools/hardware/hyperframe-plus
export PYTHONDONTWRITEBYTECODE=1
make OUT="$HOST_OUTPUT" test
make OUT="$SANITIZER_OUTPUT" \
  CFLAGS='-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer' test
```

`$HOST_OUTPUT/hftest_mock info` などで、実 CLI の無 I/O 動作を確認できる。
この mock EXE を DOS 用の完成物だと取り違えないこと。
元ディレクトリを削除する clean target は設けていない。

## 本物の DOS EXE を作る

### Open Watcom 1.9 を使用する場合の手順（今回未検証）

DOS 用 compiler と DOS/16-bit libraries を用意し、`WATCOM`、`INCLUDE`、
`LIB`、`PATH` をその環境に設定する。Git の batch 原本は LF なので、ネイティブ DOS に
持ち出す作業コピーだけを CRLF 化する（下記 helper は自動変換する）。
ソース root を `S:` として参照できる状態で、
**空の出力ディレクトリ**から次を実行する。`1234abcd` は実際の source ID に置換する。

```bat
CALL S:\BUILD.BAT S:\SRC 1234abcd
```

batch は `wcc -bt=dos -1 -ms -za -w4 -we -s` で 16-bit small model / 80186 を選ぶ。
backend にはさらに `-oi` を付け、port I/O intrinsic を命令として出す。
`wlink` の `system dos` と stack=8192 で `HFTEST.EXE` を生成する。
DOS/4GW のプログラムを生成する設定ではない。
`BUILD.OK` と `HFTEST.EXE` が必要で、`BUILD.ERR` があれば失敗。
`HF_DOS.LST` で word OUT が二つの byte OUT に置き換わっていないことも検査する。
MS-DOS 3 以降の console API を用いるが、今回 PC-9801 上の runtime は未検証。

### 今回確認したビルド

PATH 上に Open Watcom 1.9 の `wcc/wcl` はなかった。
代わりにリポジトリで固定済みの **Open Watcom v2 2026-08-01** 配布から、
DOS 用 `binw/wcc.exe`、linker、headers、16-bit libraries を隔離展開し、
既存のローカル DOSBox 0.74-3 でビルドした。システムへのインストールや
container の新設・切替は行っていない。**DOS MZ は BUILT、1.9 は NOT_TESTED。**

使用 archive の URL/hash は
[`tools/openwatcom/containerfile`](../../openwatcom/containerfile)を参照。
helper は同ファイルの SHA-256 と不一致なら実行前に停止する。
ダウンロードや既存 SDK の改変は helper 自体では行わない。

```sh
python3 -B build_dos.py \
  --archive "$OPENWATCOM_ARCHIVE" --dosbox "$LOCAL_DOSBOX" \
  --output "$NEW_DOS_OUTPUT" --build-id 1234abcd
```

出力は `$NEW_DOS_OUTPUT/run/hftest.exe`、構造・hash・source identity・
DOSBox 内の無 I/O smoke 結果は `$NEW_DOS_OUTPUT/build.json`。
使用した SDK は host build tool であり、配布する HFTEST に同梱しない。
archive の提供元・license 義務は、HFTEST の BSD license と別である。

任意の追加検査（Unicorn がある Python 環境）:

```sh
python3 -B tests/test_dos_backend.py \
  --exe "$NEW_DOS_OUTPUT/run/hftest.exe" \
  --listing "$NEW_DOS_OUTPUT/run/hf_dos.lst" \
  --output "$NEW_BACKEND_REPORT"
```

依存不足は exit 77 / NOT_RUN であって PASS ではない。
これは自作 EXE の一部の命令試験であり、PC-9801 やボード全体の emulation ではない。

## コマンドと安全確認

引数なし・`help`・`info` は **ボード IN/OUT ゼロ**。
綴り違い・余分な引数・確認キャンセル・EOF でも許可前の I/O はない。
確認を省略する `--yes` や自動 probe は設けていない。

| コマンド | 許可条件・内容 |
|---|---|
| `HFTEST off` | `PC98-HF` の入力が必要。D0=00 → EE=00 の停止候補。原本未照合と警告する |
| `HFTEST readback` | `PC98-HF` と `PIXEL-READY` が必要。二画素保存→相異なる RGB→照合→復元→再照合 |
| `HFTEST readback word` | 上記に加え `WORD-OUT`。座標を word OUT にする実験で、実機同値性は未確認 |
| `HFTEST pattern` | `PC98-HF`, `PIXEL-READY`, `DRAW-32`。32×32 のみを上書き |
| `HFTEST pattern full` | 最後の確認は `FULL-FRAME`。640×400 を上書き。小領域の確認より先に使わない |
| `HFTEST init` | BLOCKED。未確認の初期化を強制実行する裏口はない |
| `HFTEST banktest` | BLOCKED。実 backend の EC 出力も拒否。窓への memory access は実装しない |

pattern は行単位で進捗と非ブロッキング ESC 中断を扱う。
中断後も書いた範囲は残り、**元画像は復元しない**。
画面を消す目的で終了時に自動 `off` を付け加えることもしない。

結果は `HF_RESULT=...` と比較値、復元確認 mask、書込み画素数等を stdout に出す。
初回は live console と DOS リダイレクトによるテキスト記録を両方確保する。
ROM による機種自動判定や logfile の自動追記は実装していないので、
機種・カード・初期化条件・実行物 hash は試験記録に別途記入する。

exit 0 は help、送出完了、または限定 readback 成功であり、万能な実機 PASS ではない。
1 は比較不一致、2 は未対応/前提未確認/中断、3 は引数エラー、4 は I/O/復元失敗。
復元失敗時にも元の比較結果は `Primary=...` に残す。

**次に薦める実機実験は、条件を満たした PC-9801 で、既知の pixel mode からの
二画素 byte `readback` 一回のみ。** 原本・初期化・機材条件が確認できなければ
実行せず BLOCKED を維持する。具体的条件は [試験計画](docs/test_plan.md)を参照。
