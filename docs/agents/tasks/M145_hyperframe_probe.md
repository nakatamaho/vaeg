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

G145 は **NOT_READY**。原本の条件付き制御フロー、初期状態・SW1・I/O 所有権、
実機結果が未資格である。引渡しやビルド成功は gate 承認ではなく、後段の媒体作成や
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
