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

# M145 — HyPER-FRAME+ PC-9801 DOS 検証と後段の VA 調査

Status: **IN_PROGRESS; G145 NOT_READY.**
Branch: `milestone/m145-hyperframe-probe`; commit prefix `M145:`.
Base: accepted main at
[cb4edc1a05221971c5a048236eeeb172d2abf73e](https://github.com/nakatamaho/vaeg/commit/cb4edc1a05221971c5a048236eeeb172d2abf73e).

## Assignment and separation from WSR-E

The maintainer explicitly paused WSR-E and requested a separate milestone
trying Hyper Frame+ on the VA. This authorizes switching the active task,
not accepting or merging M143. Its source and evidence remain on the
[separate WSR-E branch checkpoint](https://github.com/nakatamaho/vaeg/commit/5fe759705dec089b471c8ee40bd6506c76f34997),
with G143 NOT_READY. The maintainer suspects WSR-E seating after comparison
with Hyper Frame+; this is an unconfirmed observation, not an established
root cause of the preceding FF reads. Do not request further WSR-E operations
as part of this task, force insertion, or remove retained M143 work/evidence.

This branch starts from accepted main, not the unaccepted WSR-E implementation.
Hyper Frame+ is a different board family. Never apply the Cirrus SR06/CR27,
WSR-E 7Ah handshake, rotary address assumptions or wake sequence to it.
Previously supplied board photographs show Digital Arts markings and custom
logic/DAC parts, not a basis for importing the WSR-E port map.

## 今回の依頼による優先対象の変更

本チェックポイントでは、まず **PC-9801 系の 16-bit real-mode DOS** 用に、
独立した C89 の最小試験ツールを作ることが明示的に依頼された。
PC-88VA/V30 は後段の実現可能性調査であり、互換性成立を仮定しない。
文書・報告は日本語、コード・コメント・CLI・テスト・build script は英語とする。

公開論文と JPGVHF 原本を別々に評価し、原本が未入手・深い初期化解析が未完了でも、
根拠のある画素 I/O と mock の実装まで停止させない。
一方、未確認の初期化や bank aperture 書込みは実行しない。
既定 help/info、拒否、キャンセルはボード I/O ゼロとし、操作には明示的確認を要求する。
この変更は WSR-E の再開・受入れではない。

## 当初の VA/V3 目標（後段へ保留）

1. Establish the Hyper Frame+ identification, port map, read side effects,
   switch settings and any required enable sequence from its own sources.
   Keep Hyper Frame, Hyper Frame+ and Super Frame variants distinct.
2. Independently write the smallest VA/V3 MS-DOS 4.0 diagnostic justified
   by that evidence: `cpu 186`, machine ROM word/banner, explicit program
   and build identity, bounded execution and a separate log file. Default
   operation must not write unknown device ports or scan arbitrary I/O.
3. Keep any justified write experiment an explicit opt-in, document port
   ownership and residual state, and never restore unknown FF as a saved
   value. Do not promise complete restoration without readback evidence.
4. Build a separate pinned MS-DOS 4.0 D88, retaining its system files,
   AUTOEXEC and license. No automatic probe at boot; no supplied commercial
   drivers, images or manuals on the distributable probe disk.
5. Validate instruction/I/O/log contracts with isolated controls and actual
   DOS execution where meaningful. Preserve input/output identities, command
   exits, negative outcomes and missing coverage. Synthetic success and an
   emulator's absent-device reads are not hardware validation.
6. Obtain and analyze the returned hardware log with exact COM identity.
   A negative identification is a recorded result, not proof of absence,
   faulty seating, board damage or VA bus incompatibility.

If a safe board-specific access contract cannot be established, stop before
producing an invented probe and report the exact missing input. Do not run
whole multi-board detectors or original installers on the host or VA.
Independent analysis may inspect private supplied software and perform bounded
synthetic execution of identified routines, with no live hardware access.

## Private evidence and hardware handling

Original photos, archives, converted documents, disassembly, returned media,
raw captures and their identities remain outside Git under task-specific
scratch. Tracked analysis uses neutral source IDs and independently written
summaries; publicly available references have pinned provenance.

All card changes require power off and unplugging first. Do not force,
bend, grind, or use mounting screws to pull a card into alignment. The user
chooses installation and switch changes; software observations do not prove
mechanical contact or power integrity.

## 本チェックポイントの範囲と gate

対象は PC-9801 DOS の byte 画素アクセス、二画素保存/照合/復元、小 pattern、
追加確認付き全画面/word 実験、明示的停止候補、純粋な bank 算術とソフトウェア試験。
汎用 graphics driver、実 bank mapping、BLT、PCM、性能、エミュレータへの board 実装、
VA 用 D88、release 公開は対象外。原本確認がない初期化は実行不可とする。

G145 は **NOT_READY**。後続解析で原本の限定制御フローを確認したが、
初期状態・冷間初期化・SW1・I/O 所有権・実機結果は未資格である。引渡しやビルド成功は gate 承認ではなく、後段の媒体作成や
リポジトリ全体の gate checks を免除しない。full media/demo/presenter campaign、
新しい vaeg application build、M143/main の merge は行わない。

## 当初の調査記録（優先対象変更前）

以下は最初の VA 計画時点の記録であり、今回の P1 論文確認・自作ツールの状態は後述する。

- A supplied multi-board DOS image viewer explicitly lists HyperFrame/+ as
  built-in support, separately from its external Cirrus/WAB driver. This is
  a candidate reference for static analysis, not a reason to execute the
  whole viewer or assume PC-98 BIOS calls work on the VA.
- Static analysis of that viewer (neutral reference **D1**) follows the MZ
  load/relocation layout, the PC-98 graphics dispatch and three framebuffer
  candidate paths. The paths use the low 00D0h-family ports but have different
  register and pixel-data protocols. Their exact assignment to Hyper Frame,
  Hyper Frame+, Super Frame variants and the other listed board is not yet
  qualified. A shared address is not a shared device protocol.
- These detection paths already include initialization and pixel-data writes;
  they are not harmless presence queries. One also writes outside the
  00D0h-family range. Do not copy them wholesale or label an arbitrary raw
  port scan as a board identification test. Full-program paths use PC-98 BIOS
  and machine work areas that must not be transplanted to VA.
- The VA manual places low 0000h-00FFh ports in system area 0, not the
  1000h-FEFFh user area used in the preceding WSR-E work. Do not assume that
  absence of a vaeg handler proves a real port is available or side-effect
  free. No actual VA access to these candidate ports was made here.
- Exact Hyper Frame+ port/read-side-effect and SW1 contracts: **OPEN**.
  A Hyper Frame+ manual's I/O/SW1 pages, dedicated documented sample source,
  or dedicated setup/display software would allow the family mapping to be
  checked without guessing. The supplied photographs alone do not establish
  those meanings.
- New probe/media production is **BLOCKED pending that qualification**;
  control tests and hardware results are **NOT_RUN**. No new D88, board
  implementation or live original-software execution is claimed.

## 当初のチェックポイント検証

Documentation-only scope: roadmap plus this task. Encoding/EOL/case/Markdown
comment checks and staged `git diff --check` return 0 (zero reported
violations/findings); milestone-ID selftest/discovery/audit returns 0 with
49 checks passed, 225 tasks, 177 reports and 166 roadmap rows. Command output
is retained in task scratch. No application/probe build or behavioral rerun
is required or claimed for this planning/static-analysis checkpoint.

## PC-9801 ツールの実装・検証チェックポイント

成果物は [`tools/hardware/hyperframe-plus/`](../../../tools/hardware/hyperframe-plus/README.md)。
[暫定仕様](../../../tools/hardware/hyperframe-plus/docs/hyper_frame_plus_spec.md)、
[解析記録](../../../tools/hardware/hyperframe-plus/docs/reverse_engineering.md)、
[試験計画・結果](../../../tools/hardware/hyperframe-plus/docs/test_plan.md)を追加した。
既存 vaeg の root build を置換せず、DOS backend は 8.3 の `hf_dos.c` に分離した。

- 公開 P1 論文の p.65、List 1/2 を確認。各 RGB 成分の前で座標を再選択し、
  address hold/auto-increment を仮定しない。bank 境界は y=204、x=512。
- 指定された JPGVHF archive/EXE/DOC、変換文書、メモ、旧 C 二本は今回未検出。
  依頼中の hash/命令 offset は期待値であり、独立確認済み `[BINARY]` ではない。
  原本の MZ、分岐、caller、特殊座標を解析したとは報告しない。
- 入力 validator は `HFRE_INPUT_MISSING` / BLOCKED / exit 2。
  原本を得た場合の hash・MZ・relocation・短い byte landmark 検証器と
  独立 fixture tests を実装した。byte match だけで control-flow PASS にしない。
- C89 core、DOS と mock backend、英語 CLI、build.bat と host Makefile を実装。
  `init` と `banktest` は実行不可。実 DOS backend でも EC を拒否する。
  `off` の候補列は原本未照合という警告と確認付きであり、停止成功の証明ではない。
- 二画素比較は両方の保存後に開始し、不一致や I/O failure でも両方の復元を試す。
  復元後の readback も照合するが、全 device state の復元とは呼ばない。
- Apple clang 21.0.0、strict C89/Werror: 六群、2050815 assertions PASS。
  CLI 22 cases、validator 9 tests PASS。ASan/UBSan も同じ試験が PASS。
- Open Watcom 1.9 は PATH 上になく未検証。固定 v2 2026-08-01 の DOS compiler を
  既存ローカル DOSBox 0.74-3 で使用し、16-bit / small / 80186 / C89 の MZ を生成。
  MZ の構造と生成 IN/OUT の byte/word 命令を検査し、無 I/O 六ケースの guest exit と
  結果文字列を確認した。開発 build label `00000000` は commit ではない。
- 実 MZ と一致する自作 backend の限定命令実行は 25 cases PASS。
  Unicorn 2.1.4 で I/O を捕捉し、命令幅・許可ポート・拒否を確認した。
  これを PC-9801 やカードの実機確認に昇格しない。
- 初回の CLI 行区切り FAIL と intrinsic 命令検査 FAIL は原因を修正し、
  失敗ログも保存。単一座標選択だった開発版も残し、最終各成分再選択版で再検査した。
- PC-9801 runtime、実カード、映像、初期化、bank window、VA、D88 は NOT_RUN。
  当初の multi-board viewer や JPGVHF 全体は実行していない。
  コード生成・fixture 成功は `[HARDWARE-TESTED]` ではない。

具体的な command、範囲、PASS/FAIL/BLOCKED の定義は上記試験記録に記載。
原資料・page image・SDK・raw log・EXE は task scratch に保持し、Git へ追加しない。
ソフトウェアの追加だけであり、既存エミュレータの挙動修正や既知 defect の完了を
主張しないため、bug-fix ledger へ架空の実機修正を記録しない。

Repository checks は `check_encoding.py --expect utf8`、`check_eol.py`、
`check_case.py`、`check_md_comments.py`、staged `git diff --check` がすべて exit 0。
`milestone_ids.py --root . --selftest --discover --audit` は exit 0、49 checks、
225 tasks / 177 reports / 166 roadmap rows。Markdown の相対 link、新規ファイルの
BSD header、コード/スクリプトの ASCII、開発 QA と最終 source の build-input hash
一致も確認した。新しい batch の source は限定した `.gitattributes` 指定で LF を保ち、
DOS 用の作業コピーだけを CRLF 化する。元の source・資料は変更しない。

### 確定 source の DOS 引渡し

評価 source は
[`d321baa646c24da99d36afc2fb545a5633a82056`](https://github.com/nakatamaho/vaeg/commit/d321baa646c24da99d36afc2fb545a5633a82056)。
この commit の clean な対象 source を、新しい出力ツリーで `--build-id d321baa6`
としてビルドした（exit 0）。生成 `HFTEST.EXE` は **17312 bytes**、SHA-256 は
`b8501af0d237cde9d7dd551d02acc6eccd0cd5e08329c6ead380c593e102c775`。
MZ header は 48 bytes、relocation 4、entry 0000:122Ah、overlay 0。

最終 EXE で無 I/O DOS smoke 六件と、実 MZ に一致する backend の限定実行 25 件を
再確認した（各 exit 0）。開発版との EXE 差分は build label の八 bytes だけ。
host C/CLI/validator の評価済み source と worker の digest は保持・照合して再利用した。
後続変更はこの引渡し記録だけであり、再び同じ挙動試験を回したとは報告しない。

checkout 外の handoff に EXE、BSD license、build/限定命令検査 JSON、
日本語 BUILD-REPORT を配置し、コピー先と build artifact の hash を照合した。
記録は worker digest、source/比較契約 hash、command/exit、出力 digest を含む。
原資料・SDK・失敗ログ・開発版は保持。hosted CI、release 公開、main merge、
full media/demo/presenter campaign は実施しない。**実機は NOT_RUN、G145 NOT_READY。**

## 三原本の逆ASM・操作経路の保存チェックポイント

利用可能になった三つの archive と原本 DOC を保全し、maintainer の
「逆ASMから使い方を習得し、詳しい document として散逸を防ぐ」という追加依頼を受けた。
[逆ASM・操作経路の保存記録](../../../tools/hardware/hyperframe-plus/docs/reverse_engineering.md)
を集約先とし、README・暫定仕様・試験計画から参照する。
原資料名/hash、転送元、raw 逆ASM、DOC 全文、trace は私的台帳に保持する。
参照 ID は J1（専用 JPEG viewer）、H1（HF から本体へのディザ変換）、
D1（従来の multi-board viewer）を継続使用し、M143 の別 D1 と混同しない。

- 遠隔側の hash 照合は SSH 認証で BLOCKED。手元に存在した提供コピーを解析した。
  J1 は既提示 archive/EXE/DOC の三 hash と一致。原本 validator は exit 0 /
  BYTE_CHECK_PASS。D1 本体も先行 M145 解析のものとバイト一致した。
- J1 の caller/option globals と I:54CF の初期化を追跡した。
  EE/D0/D3 の出力だけでなく、条件付き画素ゼロ出力、Y=0191 の八点、
  Y=0193 の 32 点、幅依存位置更新がある。c1 の分岐は縮小後の
  `w<640 OR h<400`。-3 でも特殊座標初期化自体は省かない。
- D0=E5 は単成分描画中の経路で E1 に戻す。停止関数は D0=00→EE=00、
  正常終了時の ESC はそれを呼ばない。幅 638/639 の縦更新式と byte/word の
  命令幅、実 offset、caller、未確認の bit 定義も記録した。
- H1 は既存 HF 画素を本体画面へ変換する道具であり、JPEG loader や無変更 probe
  ではない。D0/D3 の保存・切替・書戻し、EE の残存、ソフト座標 zoom を区別した。
- D1 の HF-family 検出は制御値と画素を書き換える。D0=FF 同士は最初の gate を通り、
  画素照合失敗側は旧値を出力しない。別 protocol への fallback と本体依存も記録し、
  全検出の移植を正当化しない。行読出しの実格納順 G,R,B も限定実行で確認した。
- 独立した `trace_reference.py` は原本 hash を binding して閉じた経路だけを実行し、
  全 I/O と必要な値・状態・外部関数 stub の順を照合する。Unicorn 2.1.4 で
  **53 cases PASS / exit 0**、code-hook 訪問数合計 5393136（stub 境界を含む）。
  原本なしの合格 fixture / 一変更負例 **12 tests PASS / exit 0**。
  最初の IN 不一致負例の ctypes 補助警告は、整数 return と停止通知の分離で解消した。
  警告ログ、51-case 開発記録、最終 53-case の両方を残す。
- 以上は原本ソフトウェアの `[BINARY]` / 限定 trace 所見。
  全 EXE/COM entry、PC-9801 runtime、実カード、画像、EMS round trip は NOT_RUN。
  HFTEST の C/DOS build inputs は d321baa6 と不変で、既配布 MZ と過去の
  C89/sanitizer/DOS 検証を再利用する。新しい DOS/vaeg build、D88、全 media/demo、
  hosted CI、release/main merge、WSR-E 再開は行わない。
- `init` と `banktest` は維持。正式な register 定義、冷間状態、SW1、所有権、
  timing、VA 互換性は未解決であり、**G145 NOT_READY**。

再現 command、比較契約、入力/worker/出力 digest は checkout 外の manifest に保存。
原本と解析証拠は長期保管用の私的 bundle も作り、通常の task scratch cleanup と分離する。
この保存依頼に属する原本・解析記録は、将来の gate 後も自動廃棄の対象にしない。
既存 emulator の guest-visible 修正ではないため、bug-fix ledger へ架空の修正を追加しない。
Repository checks は `check_encoding.py --expect utf8`、`check_eol.py`、
`check_case.py`、`check_md_comments.py`、`milestone_ids.py --root . --selftest
--discover --audit`、staged/unstaged `git diff --check` がすべて exit 0。
違反/所見 0、milestone selftest 49、225 tasks / 177 reports / 166 rows。
Python syntax/header/ASCII、相対 link/anchor、九ファイルだけの scope も確認した。
出力・exit と最終 commit の source identity を private report に保持する。

## VA 向け EC 非使用構成と基板写真の追跡

maintainer から VA 診断 D88 の依頼があったが、続く system-port 競合の指摘を受け、
最初に提案した D0/D3 の read-only probe は実装前に撤回した。
IN を無害とみなさず、今回は「配線をさらに追い、EC 非使用の最小構成を絞る」
調査だけを進める。詳細は
[基板 decode 調査](../../../tools/hardware/hyperframe-plus/docs/board_decode.md)に集約した。

- 表裏写真 B1/B2 を原本とバイト照合して保全し、切出しと裏面反転で対応を確認した。
  遠隔側の hash/転送は未実施。先の標準 Python の Pillow import 失敗後、既存の
  別 Python/Pillow 環境で作成した。新しい package installation はない。
- SW1 直下の RA1 に対応する四つのパッドから、四本の線を IC1/LISA の領域内の
  ビア列まで視認した。それ以降のピン対応・ASIC 機能は確定できていない。
  SW1 を I/O アドレス設定と断定しない。
- SW1 と別に J0〜J6、IC2 の 74HC273、IC3 の 74HC85、IC27/28 と IC29/30 を
  調査対象として識別した。部品の近接だけで接続や EC decode を確定しない。
  `/CLR`・clock・比較入力/出力・decoder enable へ確認箇所を絞った。
  J3 の見た目の差は故障判断やキャップ追加の指示ではない。
- 既存の 53-case trace 全件、原本、runner/validator、Unicorn libraries の hash と
  集計を照合し、その限定 trace の EC アクセスがゼロであることを再集計した（exit 0）。
  七つの座標/RGB byte ポートによる転送経路は候補として残るが、冷間表示には
  D0/D3/EE・特殊座標・本体 BIOS 依存などの別調査が必要。原本実行は再走していない。
- EC は VA の漢字 ROM ポートと重なる。M144 の V3 読出し結果を、未測定の OUT
  動作や D0/D3 の衝突証明に拡大しない。00D0 と 01D0 も同一視しない。
- EC を書かない自作プログラムでも、power-on 時や firmware/OS の EC 書込み後に
  HF+ の窓が有効なら、既存 RAM と競合する可能性が残る。実装済み RAM の構成、
  HF+ の reset/decode/output-enable、I/O を残した窓無効化の可否は未確定。
- **スイッチ変更、導通測定、実機 I/O、診断プログラム/D88 作成は NOT_RUN。**
  写真から解けない ASIC 内部や隠れた配線を、VA の未知ポート走査で埋めない。
  結論は「EC 非使用の転送方式は候補、VA 実行可否は BLOCKED」である。
  `init` / `banktest`、VA 実行禁止、G145 NOT_READY、WSR-E の休止は維持する。

文書だけの追記で、HFTEST の DOS 入力が d321baa6 とバイト一致し、引渡し EXE の
SHA-256 も既存記録と一致することを確認した。新しい build/behavioral campaign、
full media/demo、hosted CI、release/merge は行わない。写真・原本・資料と hash 台帳は
Git 外の独立した長期保管 bundle に保存し、先行する三原本 bundle の記録は変更しない。
Repository checks は encoding/EOL/case/Markdown、milestone selftest/discovery/audit、
staged/unstaged diff check を使用し、command/output/exit を私的報告へ保持する。
