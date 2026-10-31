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

# M142 — Claude引継ぎ：現状とBUPDATEのPython代替

## 引継ぎの依頼

以下の二点を引き継ぐ。**この文書を作成した段階ではPython代替は未実装**。

1. **進行中のM142を継続する。** 特に #75 は原因未確定・未修正であり、
   周辺テストやビルドの完了をもってhuman gate準備完了としない。
2. **BUPDATEのためにDOSBoxを起動する処理を、小さなホスト側Pythonスクリプトに
   置き換える。** まず既存媒体レシピが使うBDF差分適用に限定する。
   汎用DOS実行環境やBDIFF差分生成機能を新設する依頼ではない。

作業ブランチは `topic/m142-fixes`。この引継ぎ直前のpushed HEADは
[`468fa6affd816c87ad2073aa2b04ea47a4c5aeb1`](https://github.com/nakatamaho/vaeg/commit/468fa6affd816c87ad2073aa2b04ea47a4c5aeb1)。
引継ぎ文書追加後のHEADとは区別する。次のマイルストーンを開始せず、
現在のブランチと未コミット差分を確認してから再開する。

最初に `AGENTS.md` → [ROADMAP](../ROADMAP.md) →
[CONVENTIONS](../CONVENTIONS.md) → [M142タスク](../tasks/M142_runtime_portability.md)
を読む。以下はその置き換えではない。

## 1. M142の現状

**M142 IN_PROGRESS / #75 OPEN / correction NOT_IMPLEMENTED / G142 NOT_READY。**
正式なhuman gate依頼、受理、merge、release、hosted CIは行っていない。
現在のEXEは途中チェックポイントであり、ユーザーへの追加写真・中間EXE試験・
漠然とした環境準備依頼は出さない。必要になった場合だけ、対象と最小操作を特定する。

| 項目 | 完了したこと | 残っていること |
|---|---|---|
| FPS | 100超を含む計測・桁数・書式のテスト | 報告された実際の過クロック条件と表示幅の再現。132→32の桁落ちは実証されていない |
| メモリ既定値 | 未指定EMS/BMSをOFF、明示設定維持、実状態保存・復元・再起動 | GUIでの編集・永続化の受入確認 |
| クロック | メニューを4/8/10/12/20 MHzに限定、既存設定と主CPU/FDC分離を維持 | GUI選択・機種/モード/周辺クロックの実行確認 |
| #75 | 実機10画面・240行の転記、対応ソフト実行、Q/R/Wトレース解析 | 物理的な制限要因の特定と根拠のある修正 |
| #76 | DISK v1固定形式、同一ホスト旧形式、パス再解決、実状態の同一ホスト復元 | 実際のWindows↔Linux状態交換。ネイティブ実行と旧状態を含む確認 |
| Windows | fresh clone/application objectsの完全なMacPorts MinGW cross-build、監査 | Windowsでの実行・presenter/HiDPI等。cross-buildは実行証明ではない |
| デモ/媒体 | 新規9媒体のビルド整合、34描画ケース、MANDEL進行、SGP自起動メニュー | utility FDDとVA/VA2用SASIの生成・各単独起動 |

### 読むべき既存報告

- [総合実装・進捗](m142_runtime_progress.md)
- [状態保存・CTest・Windowsビルド・gate準備](m142_gate_preparation.md)
- [新規デモ/媒体再生成・ライブ描画](m142_demo_media_regression.md)
- [実機10画面と対応ソフト観測](m142_tvram_vb_hardware.md)
- [VBイベント・Q/R/W・REPアラインメント](m142_tvram_vb_events.md)
- [TSP参照資料](../../modernization/upd72022-tsp.md)
- [恒久バグ修正台帳](../../modernization/bug-fixes.md)

最新のローカルCTest集計は **150 PASS / 16 SKIP / 0 FAIL、失敗を保存したうえでの
focused recovery/reuse込み**。二度目の全件一括成功ではない。
デモ結果も初回の失敗と修正後の再利用を区別している。成功済みの高価な試験を、
文書・検証器・ホスト側媒体ツールだけの変更で無条件にやり直さない。

### #75：次に解くべき本題

VA2の短いVBについて、32/128/256-byte単位のQコピー回数はソフトで **7/3/2**、
実機では概ね **7/2/1**。一方、GVRAMのRは **13/5/3** が対応し、
長いblankのWは128/256-byte条件の **8/4** が対応している。
通常転送行の誤差は主に数%で、全体を1.45倍遅くする根拠にはならない。

- Q/R/Wは命令コスト・VBポーリング・REP再dispatch・メモリアクセスを分けて検討する。
- 同じ命令コストと位相のまま共通VB終端だけを早める案は、256-byte Qと
  128-byte Rを同時に満たせない35-clockの不整合が既に示されている。
  これはその仮定下の反証であり、物理原因の証明ではない。
- RのREP継続にある追加コストは奇数番地とprefetchで説明済み。
  それだけでは #75 を解決せず、REPの起動コストを勝手に削除する根拠にもならない。
- 新しいVAラベルのS=20と過去のVA S=86の不一致は未解決。
  200-line SYNCは代替条件として確認されたが、過去の操作履歴は確定していない。
  既存のVA holdを仮説だけで削除しない。
- トレースのIN位置は命令開始点であり、正確なI/Oバスエッジではない。
  IRQ entryに架空のクロックを加えず、partial windowを完全区間に混ぜない。

直前の読み取り調査では、BNN技術マニュアル §3.5のVALT制限が「1水平期間あたり」
であることも確認した。`0148h` bits6–4の0は制限なし、1–7は4–28アクセス、
制限超過は次の水平期間まで待つという記述。ただし、原資料の画像照合や
実機の設定・grant timingを新たに測定した結果ではない。
既存VA2状態3プロファイルの再読では `videova.txtmode=03h`、VALT=0。
したがって、**このソフト再現にVALT制限が有効だったとは言えず、原因認定しない**。
この小調査もエミュレータの変更・新規ゲスト実行・#75修正を含まない。

関連コードは `io/tsp.c`、`io/videova.c`、`memoryva/memoryva.c`、
`memoryva/gvramva.c`、`machine/pccore.c`、`machine/nevent.c`、
`cpu/upd9002/upd9002_{core,mn,rp}.c`、`demos/tsp-sprites/tspspr.asm`。

### 手元の成果物と証拠の所在

`SCRATCH` はリポジトリからの相対位置 `../tmp/m142`。存在・identityを再確認して使う。
privateな入力名・パス・ハッシュ・生ログはGitに記録しない。

| 場所（`$SCRATCH/`配下） | 用途 |
|---|---|
| `media-qa-r1/validation.json`、`media.json`、`guest-results.json` | 新規媒体9件・描画34件・補助ビルド等の検証とbinding |
| `gate-prep-r1/state-run-r3/manifest.json` | 実状態3プロファイル、再起動、拒否テスト。初回検証失敗は別ログで保存 |
| `gate-prep-r1/ctest-all.log`、`ctest-recovery.log` | 初回失敗とfocused recovery |
| `tvram-vb-hw-intake-01/` | 実機10画面、転記、対応ソフト実行 |
| `tvram-vb-events-r1/`、`tvram-vb-controls-r1/`、`tvram-vb-controls-r2/` | Q/SYNC条件とコスト解析。失敗・未完了の試行も含む |
| `tvram-vb-rw-r1/comparison.json`、`tvram-vb-values-r1/` | R/W対照と24行の実際のreturn観測 |
| `tvram-vb-arbitration-r1/observations.json`、`layout.json` | 直前のVALT/構造体layout再読。既存状態の再利用のみ |
| `tvram-vb-build-r3/manifest.json` | ソース由来の実機測定キット |
| `mingw-gate-prep/output/` | 現在のWindows EXE、BUILD-REPORT.md、監査 |
| `mingw-sdk/`、`native-ffmpeg-stage/` | 再利用可能な隔離SDK。削除・全再構築しない |

成果物のソースは混同しない：

- 旧Q/R/Wトレース：`30bceba24c102eb3ee60bdfcf90e0e4b93836b3c`。
- 現在のnative worker：`0d02bc849b5c4fb4f45d21c13716735e77578d37`、
  `$SCRATCH/native/sdl2/vaeg`、SHA-256
  `24f53861e4a81f0914d489c353921113cc81e0aa1f730f00b9a9b07deea70700`。
- 媒体再生成のfresh clone：`33dcb8ba9091cdf93eab8e1399478d5508c3e593`。
- Windows EXE：`ea62eda1eb2d1d3abed11f72af2e5e7899868652`、
  `$SCRATCH/mingw-gate-prep/output/vaeg.exe`、SHA-256
  `d9c3bbdc36d88e5cc23a3587f2cbe7870594af2c55634dfe97e4e6f32b2b132a`。
  表示は `Rel.20261030 (M142-dev, ea62eda1)`。後続文書のSHAに付け替えない。

macOS display preflightはactive display=0、screen capture不可、accessibility未信頼。
SDL dummyの描画成功をCocoa/final-client/layoutのPASSにしない。
Windows実行targetも未確保。MHはmaintainerが修正済みと報告済みで再調査しない。
G139/G140の受理範囲、過去のrelease/tag、M141の受理記録は書き換えない。

## 2. BUPDATEのPython代替

### 依頼範囲と、重要な現状訂正

実際のutility FDD生成は、次のコマンドが **exit 1** で止まっている。

```sh
tools/pc88va/build-utility-disk.sh --source "$SYSTEM_01" --output "$UTILITY_FDD"
# error: required host command is missing: dosbox
```

ただし、**DOSBox用途はBUPDATEだけではない**。BUPDATEを置換する依頼自体は進めるが、
それだけでutility全体のDOSBox依存・全媒体gateが解消したとは報告しない。

| 現在DOSBoxを呼ぶ処理 | 役割／今回の切り分け |
|---|---|
| `build-utility-disk.sh` のBUPDATE | LHA、PCPLUS、TSCLVAのBDF適用。今回の置換対象 |
| `scsi-support.sh` のBUPDATE | PCPLUSのBDF適用。同じPython実装を共用 |
| `stage-development-tools.sh` のSASI側BUPDATE | FD98→VA用のBDF適用。同じPython実装を共用 |
| `build-utility-disk.sh` のWSP | MSE、BMS、K-Launcherの更新。BDFとは別方式で残る |
| `build-utility-disk.sh` のDIET | EXE/COM圧縮。FDD容量に関わるので勝手に省かない |
| `stage-softlib-additions.py` のJWasm | SASI用VA3DDEMO/PCODEのソースビルド。BUPDATEとは別に残る |

さらに `tools/openwatcom/build-sqemm98.sh` と `build-view480.sh` は
container経由のビルドであり、現M142方針では未実行。DOSBoxの代替とは別の障害。
この引継ぎはcontainer使用や依存全部の大規模置換を承認するものではない。
残る依存は列挙し、別途必要な最小対応として扱う。

### 実装の出発点

- 小さなPython標準ライブラリのツール、例えば `tools/pc88va/bupdate.py` を新設する。
  これは候補名であり、既に存在するコマンドではない。
- 既存レシピが取得・SHA-256固定している **BDIFF/BUPDATE 1.28** を調べる。
  `bdiff128.lzh` には `BDIFF.DOC`、`BDIFF.EXE`、`BUPDATE.EXE`、`MODTIME.EXE`
  が入っている。キャッシュ済み。書式・checksum・オプションの意味を確認し、
  文書を読んで独立実装する。BDFの書式や `-x -i -o` の意味を推測で決めない。
- 配布アーカイブは既存のshared cacheから読み、展開・解析・生成は
  task scratchに行う。キャッシュに作業ファイルを置かない。
- 差分生成、完全CLI互換、DOSプログラム実行は不要。実際のBDFで使われる構文を
  正しく処理し、未対応構文は明示的に拒否する。
- TSCLVAはDOCとSYSの**複数ファイル**を生成する。単一入力・単一出力だけという
  前提にしない。CLIはpatchと入力/出力directoryを明示できるものがよい。
- lha/lhasaの小文字展開と既存uppercase aliasにも注意する。大小文字だけで
  同一ファイルを二重に処理したり、aliasから出力先外に書き込んだりしない。
- 原本・キャッシュは不変。stageコピーだけを処理する。入力の一致、範囲、
  checksum等を検査してから出力し、不正patchで中途半端な更新を残さない。
  patch内のパス脱出・絶対パス・衝突等を拒否し、外部コマンドを実行しない。
- 出力をhash lookupや既成のpatched binaryで置き換えるのではなく、実際に差分を適用する。
  binary/DOCのバイト列を再エンコードしない。新規追跡ソースはUTF-8/LF、BSD-2-Clause。

### 置き換える既存呼び出しと正解の基準

以下は現在のDOS側コマンド。行番号ではなくコマンドで該当箇所を探す。
`lha255b@` は既存呼び出しの綴りで、入力BDFは `LHA255B@.BDF`。

```text
build-utility-disk.sh:
  bupdate -x -i -o lha255b@
  bupdate -x -i -o pcplus.bdf
  bupdate -x -i -o tsclva.bdf
scsi-support.sh:
  bupdate -x -i -o pcplus.bdf
stage-development-tools.sh (SASI additions):
  bupdate -x -i -o fd98.bdf
```

既存スクリプトが固定している**適用直後・DIET前**の期待値を維持する。
公開utility package由来の既存値であり、private入力のhashではない。

| BDF | 出力 | SHA-256 |
|---|---|---|
| `LHA255B@.BDF` | `LHA.EXE` | `0794c20ce820c687687fe49285758f026765f4fad9ffb2ff4d78e6a46f7fb452` |
| `PCPLUS.BDF` | `PCPLUS.SYS` | `f86d03201a2fa6c0dab13345df55f3bb929f41ec3c7c6d03efb4dbd7935f1b06` |
| `TSCLVA.BDF` | `TSCLVA.DOC` | `6712aa90951acfe03890259b415e0c9389a4c1394d2471eed641f5febe0b612d` |
| `TSCLVA.BDF` | `TSCLVA.SYS` | `574e8e5a1a5ae72f6d6ae0198f7aa760fb70e6de379d7bd10b1ec0cd2f6eb74e` |
| `FD98.BDF` | `FD98.COM` | `b62f83c33a094380e56b3f0324c4c784587736186417988e1caa03ec84326107` |

package URL・固定archive hash・入力の配置は上記3本のshell scriptを正とする。
既存の出力hash検査は削除・更新して帳尻を合わせない。
ホストでのBUPDATE実行を置き換えても、媒体に入れるゲスト用 `BUPDATE.EXE` は別物であり、
その同梱を削る依頼ではない。WSP/DIETが残るFDDからDOSBox必須検査だけを消さない。

### 検証・受入条件

1. 合成データのpassing fixtureを先に作り、実装した構文・複数出力をテストする。
   truncation、入力不一致、checksum不一致、未対応構文、パス脱出などは
   一度に一変異だけ与え、意図した安定error codeを厳密に確認する。
   別の失敗が目的の検査を覆っていないこと、原本が不変であることも確認する。
2. 4種類の実BDFから上記5出力を**新規生成**し、既存期待hashと一致させる。
   private入力不要のunit testと、cache依存のローカル結合試験を分離する。
   再検証のために新たなDOSBoxインストールを必須にしない。
3. 3本の呼び出し側を共通実装へ接続し、BUPDATE処理だけのDOSBox依存を除く。
   shell構文・focused tests・repository checksを行う。残依存と未実行工程は明記する。
4. utility FDD・両SASIが生成できる段階になったら、新しい出力先で実際に生成し、
   payload/hash、FDD起動、VA/VA2各SASIの単独起動を確認する。
   BUPDATE単体成功をこれらのPASSに昇格させない。
5. 具体的な媒体処理不具合を修正した場合は恒久バグ修正台帳も更新する。
   コミットは `M142:`、one concern、pushして正確なSHAを報告する。

## 共通の作業境界と再開順

- ファイルアクセスは `~/work` 以下だけ。`TMPDIR` はその下の `pc88va/tmp`、
  `PYTHONDONTWRITEBYTECODE=1`。private treeはshared download cacheを除いてread-only。
- 現M142ではColima/containerへのfallback、システムport変更、global rustup変更をしない。
  既存の隔離SDK/ツールを再利用する。不要なbuild tree・実験variantを増やさない。
- BUPDATE用Pythonや文書だけの変更なら、変更されないvaeg worker・SDK・高価なCPU試験・
  Windows EXEを再ビルドしない。エミュレータ本体を変更した場合は別途検証し、pushed source
  のfresh cloneからrecording/shaders ON・bundled encoders OFFのWindows handoffを更新する。
- 削除は承認リストなしに行わない。privateの写真・ROM・入力媒体・生キャプチャをcommitしない。
- まずBDFの調査と小さなPython代替を完了し、残依存を明示する。
  続いてM142の未完了項目、とりわけ #75 の原因特定・修正へ戻る。
  **#75が未解決なら、周辺検証をいくら追加しても「human gateまで完遂」ではない。**

文書・ツール変更の基本チェック（追加したfocused testsは別途実行）：

```sh
python3 tools/repo/check_encoding.py --expect utf8
python3 tools/repo/check_eol.py
python3 tools/repo/check_case.py
python3 tools/repo/check_md_comments.py
python3 tools/qa/milestone_ids.py --root . --selftest --discover --audit
git diff --check
git diff --cached --check
```
