# Phase 1.3 Rename Candidate Plan

## 目的

Phase 1.3 のファイル名・ディレクトリ名正規化を実施する前に、rename 候補、参照更新対象、維持候補、停止条件を整理する。

この文書では rename は実施しない。

## 前提状態

- Branch: `main`
- HEAD: `80c4bba Convert source tree to UTF-8`
- tracked files: `1065`
- uppercase paths: `1052`
- case-insensitive collisions: `0`
- proposed lowercase path collisions: `0`

未追跡の `AGENTS.md` と `refactor-instructions.md` は指示ファイルのため対象外。

## 基本方針

- ソース、ヘッダ、データ、platform directory は原則 lowercase に寄せる。
- `MAKEFILE.W32` は `Makefile.w32` に正規化する。
- `README.md` と `README.TXT` は慣例名として維持する。
- case-only rename は必ず一時名を経由した 2 段階 `git mv` で行う。
- rename と clang-format、rename とロジック変更は同じコミットに混ぜない。
- `.dsp`, `.dsw`, `.vcp`, `.vcw`, `.vcproj`, Makefile, RC, docs, HTML link の参照を追従更新する。
- `Win9x/np2.dsp` と `Win9x/np2.vcproj` は Windows build gate の台帳なので、rename 後も有効な参照に保つ。
- `WinCE/`, `MacOS9/`, `Mona/` は人間判断により directory ごと削除可。ただし rename commit には混ぜず、参照調査後に独立削除 commit とする。
- `Win9x/` と `Win9xC/` は directory 名を維持し、配下の大文字 file/directory はすべて lowercase にする。
- `sdl/` directory 自体は既に lowercase のため維持し、配下の大文字 file/directory は人間判断により小文字化可。
- root 直下の大文字ファイルは人間判断により小文字化可。ただし `README.md` は維持し、`README.TXT` は `README.txt` にする。
- `COMMON/` は `common/` にし、配下の大文字 file はすべて lowercase にする。
- `CPUCVA/` は現時点では directory 名を維持し、`CPUCVA/GVRAMVA.C` の拡張子のみ `GVRAMVA.c` にする。他の `CPUCVA/Z80*.h`, `Z80c.cpp`, `Z80diag.cpp`, `types.h` は現時点では維持する。
- `IO/` と `IOVA/` は directory 名を維持し、配下の大文字 file はすべて lowercase にする。
- `EMBED/` と `FDD/` はそれぞれ `embed/`, `fdd/` にし、配下の大文字 file/directory はすべて lowercase にする。
- `GENERIC/` は `generic/` にし、配下の大文字 file はすべて lowercase にする。
- `SOUND/` は `sound/` にし、配下の大文字 file/directory はすべて lowercase にする。
- `FONT/` と `LIO/` はそれぞれ `font/`, `lio/` にし、配下の大文字 file はすべて lowercase にする。
- `CBUS/` は directory 名を維持し、配下の大文字 file はすべて lowercase にする。
- `BIOS/`, `BIOSVA/`, `VRAM/` は directory 名を維持し、配下の大文字 file はすべて lowercase にする。
- `I286A/`, `I286C/`, `I286X/` はそれぞれ `i286a/`, `i286c/`, `i286x/` にし、配下の大文字 file はすべて lowercase にする。
- `VRAMVA/` は directory 名を維持し、配下の大文字 file はすべて lowercase にする。
- `CPUXVA/` は directory 名を維持し、配下の大文字 file はすべて lowercase にする。
- `NP2TOOL/` は `np2tool/` にし、配下の大文字 file は lowercase にする。`MAKEFILE.W32` は `Makefile.w32` にする。
- `CPUCVA/` は directory 名を維持し、`GVRAMVA.C` と `Z80*` files を lowercase にする。`types.h` は維持する。
- `ROMIMAGE/` は `romimage/` にし、配下の大文字 file はすべて lowercase にする。`MAKEFILE.W32` は `Makefile.w32` にする。
- `accessories/` は directory 名を維持し、配下の `.dsp`, `.dsw`, tool source を lowercase にする。
- `HLP/` は略称の `hlp/` ではなく、意味が明確な `help/` へ rename する。`nkf -w` で読めることを確認済みで、HTML の charset header は `UTF-8` にする。

## 集計

大文字を含む tracked path の主な内訳:

| グループ | 件数 | 方針 |
|---|---:|---|
| `Win9x/` | 135 | directory 名を維持し、配下 file/directory はすべて lowercase。人間承認済み |
| `HLP/` | 96 | `help/` へ。HTML charset header は `UTF-8`。人間承認済み |
| `WinCE/` | 73 | 削除候補。人間承認済み。rename ではなく独立削除 commit |
| `IO/` | 60 | directory 名を維持し、配下 file はすべて lowercase。人間承認済み |
| `MacOS9/` | 54 | 削除候補。人間承認済み。rename ではなく独立削除 commit |
| `SOUND/` | 54 | `sound/` へ。配下 file/directory はすべて lowercase。人間承認済み |
| `Mona/` | 52 | 削除候補。人間承認済み。rename ではなく独立削除 commit |
| `sdl/` 配下 | 52 | 配下の大文字 file/directory を小文字化。人間承認済み。`sdl/` 自体は維持 |
| `Win9xC/` | 51 | directory 名を維持し、配下 file/directory はすべて lowercase。人間承認済み |
| `ROMIMAGE/` | 41 | `romimage/` へ。配下 file はすべて lowercase。`MAKEFILE.W32` は `Makefile.w32`。人間承認済み |
| `CBUS/` | 38 | directory 名を維持し、配下 file はすべて lowercase。人間承認済み |
| `GENERIC/` | 37 | `generic/` へ。配下 file はすべて lowercase。人間承認済み |
| `IOVA/` | 36 | directory 名を維持し、配下 file はすべて lowercase。人間承認済み |
| `EMBED/` | 30 | `embed/` へ。配下 file/directory はすべて lowercase。人間承認済み |
| `COMMON/` | 28 | `common/` へ。配下 file はすべて lowercase。人間承認済み |
| `I286A/`, `I286C/`, `I286X/` | 73 | `i286a/`, `i286c/`, `i286x/` へ。配下 file はすべて lowercase。人間承認済み |
| `VRAM/`, `VRAMVA/` | 38 | directory 名を維持し、配下 file はすべて lowercase。人間承認済み |
| `BIOS/`, `BIOSVA/` | 23 | directory 名を維持し、配下 file はすべて lowercase。人間承認済み |
| `FDD/` | 17 | `fdd/` へ。配下 file はすべて lowercase。人間承認済み |
| `FONT/`, `LIO/` | 22 | `font/`, `lio/` へ。配下 file はすべて lowercase。人間承認済み |
| `CPUCVA/`, `CPUXVA/` | 11 | `CPUCVA/` は維持し `GVRAMVA.C` と `Z80*` files を lowercase。`types.h` は維持。`CPUXVA/` は directory 名を維持し、配下 file はすべて lowercase |
| `NP2TOOL/` | 7 | `np2tool/` へ。配下 file は lowercase。`MAKEFILE.W32` は `Makefile.w32`。人間承認済み |
| root uppercase files | 18 | `pccore.c`, `common.h` などへ。`README.md` は維持、`README.TXT` は `README.txt` |

拡張子の主な内訳:

| 拡張子 | 件数 | 方針 |
|---|---:|---|
| `.H` | 344 | `.h` へ |
| `.C` | 250 | `.c` へ |
| `.CPP` | 133 | `.cpp` へ |
| `.X86` | 35 | `.x86` へ。アセンブリ内容は変更しない |
| `.RES` | 27 | `.res` へ。include 側の参照更新必須 |
| `.INC` | 17 | `.inc` へ |
| `.S` | 17 | `.s` へ |
| `.TBL` | 12 | `.tbl` へ |
| `.TXT` | 12 | 個別判断。`README.TXT` は維持 |
| `.ASM` | 11 | `.asm` へ |
| `.MCR` | 9 | `.mcr` へ。clang-format 対象外のまま |
| `.RC` | 5 | `.rc` へ。resource compile 確認必須 |
| `.W32` | 2 | `Makefile.w32` へ |

## 候補一覧

| 現在のパス | 提案するパス | 種別 | 理由 | 参照箇所 | リスク | 実施判断 |
|---|---|---|---|---|---|---|
| `COMMON/` | `common/` | directory | Makefile が既に `../common` を参照。人間承認済み | `Win9x/Makefile`, `sdl/Makefile.*`, `.dsp`, `.vcp`, `.vcproj`, docs | 中 | 実施 |
| `COMMON/*` | lowercase under `common/` | file | 人間判断により配下 file はすべて lowercase | Makefile, source, `.dsp`, `.vcp`, `.vcproj` | 中 | 実施 |
| `COMMON.H` | `common.h` | file | include は lowercase 方針 | Makefile, source, `.dsp`, `.vcp`, `.vcproj` | 低 | 実施 |
| `PCCORE.C` / `PCCORE.H` | `pccore.c` / `pccore.h` | file | Makefile が `pccore.c/h` を参照 | Makefile, `.dsp`, `.vcp`, `.vcproj`, source | 低 | 実施 |
| `NEVENT.*`, `TIMING.*`, `STATSAVE.*`, `KEYSTAT.*`, `BREAKPOINT.*`, `OPRECORD.*` | lowercase | file | root core files を include/build 参照に合わせる | Makefile, `.dsp`, `.vcp`, `.vcproj`, source | 低 | 実施 |
| `BIOS/`, `BIOSVA/` | 維持 | directory | 人間判断により directory 名は維持 | Makefile, `.dsp`, `.vcp`, `.vcproj`, docs | なし | 維持 |
| `BIOS/*`, `BIOSVA/*` | lowercase under `BIOS/`, `BIOSVA/` | file | 人間判断により配下 file はすべて lowercase | Makefile, `.dsp`, `.vcp`, `.vcproj`, docs | 中 | 実施 |
| `CBUS/` | 維持 | directory | 人間判断により directory 名は維持 | Makefile, `.dsp`, `.vcp`, `.vcproj` | なし | 維持 |
| `CBUS/*` | lowercase under `CBUS/` | file | 人間判断により配下 file はすべて lowercase | Makefile, `.dsp`, `.vcp`, `.vcproj` | 中 | 実施 |
| `IO/`, `IOVA/` | 維持 | directory | 人間判断により directory 名は維持 | Makefile, `.dsp`, `.vcp`, `.vcproj`, source | なし | 維持 |
| `IO/*`, `IOVA/*` | lowercase under `IO/`, `IOVA/` | file | 人間判断により配下 file はすべて lowercase | Makefile, `.dsp`, `.vcp`, `.vcproj`, source | 中 | 実施 |
| `I286A/`, `I286C/`, `I286X/` | `i286a/`, `i286c/`, `i286x/` | directory | 人間判断により小文字化可 | Makefile, `.dsp`, `.vcp`, `.vcproj`, source | 中 | 実施 |
| `I286A/*`, `I286C/*`, `I286X/*` | lowercase under `i286a/`, `i286c/`, `i286x/` | file | 人間判断により配下 file はすべて lowercase | Makefile, `.dsp`, `.vcp`, `.vcproj`, source | 中 | 実施 |
| `CPUCVA/GVRAMVA.C` | `CPUCVA/GVRAMVA.c` | file | 人間判断により拡張子のみ小文字化可 | `Win9x/np2.dsp`, `Win9x/np2.vcproj` | 低 | 実施 |
| `CPUCVA/Z80.h`, `CPUCVA/Z80c.cpp`, `CPUCVA/Z80c.h`, `CPUCVA/Z80diag.cpp`, `CPUCVA/Z80if.h` | lowercase under `CPUCVA/` | file | 人間判断により `z80*` にする | source, `.dsp`, `.vcproj` | 中 | 実施 |
| `CPUCVA/types.h` | 維持 | file | 既に lowercase | source, `.dsp`, `.vcproj` | なし | 維持 |
| `CPUXVA/` | 維持 | directory | 人間判断により directory 名は維持 | `Win9x/np2.dsp`, `Win9x/np2.vcproj` | なし | 維持 |
| `CPUXVA/*` | lowercase under `CPUXVA/` | file | 人間判断により配下 file はすべて lowercase | `Win9x/np2.dsp`, `Win9x/np2.vcproj` | 中 | 実施 |
| `VRAM/` | 維持 | directory | 人間判断により directory 名は維持 | Makefile, `.dsp`, `.vcproj`, source | なし | 維持 |
| `VRAM/*` | lowercase under `VRAM/` | file | 人間判断により配下 file はすべて lowercase | Makefile, `.dsp`, `.vcproj`, source | 中 | 実施 |
| `VRAMVA/` | 維持 | directory | 人間判断により directory 名は維持 | `Win9x/np2.dsp`, `Win9x/np2.vcproj`, source | なし | 維持 |
| `VRAMVA/*` | lowercase under `VRAMVA/` | file | 人間判断により配下 file はすべて lowercase | `Win9x/np2.dsp`, `Win9x/np2.vcproj`, source | 中 | 実施 |
| `SOUND/` | `sound/` | directory | Makefile が `../sound` を参照。人間承認済み | Makefile, `.dsp`, `.vcp`, `.vcproj`, source | 中 | 実施 |
| `SOUND/*` | lowercase under `sound/` | file/directory | 人間判断により配下 file/directory はすべて lowercase | Makefile, `.dsp`, `.vcp`, `.vcproj`, source | 中 | 実施 |
| `FDD/`, `EMBED/` | `fdd/`, `embed/` | directory | Makefile と include 参照に合わせる。人間承認済み | Makefile, `.dsp`, `.vcp`, `.vcproj`, source | 中 | 実施 |
| `FDD/*`, `EMBED/*` | lowercase under `fdd/`, `embed/` | file/directory | 人間判断により配下 file/directory はすべて lowercase | Makefile, `.dsp`, `.vcp`, `.vcproj`, source | 中 | 実施 |
| `GENERIC/` | `generic/` | directory | Makefile と include 参照に合わせる。人間承認済み | Makefile, `.dsp`, `.vcp`, `.vcproj`, source | 中 | 実施 |
| `GENERIC/*` | lowercase under `generic/` | file | 人間判断により配下 file はすべて lowercase | Makefile, `.dsp`, `.vcp`, `.vcproj`, source | 中 | 実施 |
| `FONT/`, `LIO/` | `font/`, `lio/` | directory | 人間判断により小文字化可 | Makefile, `.dsp`, `.vcp`, `.vcproj`, source | 中 | 実施 |
| `FONT/*`, `LIO/*` | lowercase under `font/`, `lio/` | file | 人間判断により配下 file はすべて lowercase | Makefile, `.dsp`, `.vcp`, `.vcproj`, source | 中 | 実施 |
| `Win9x/`, `Win9xC/` | 維持 | platform directory | 人間判断により directory 名は維持 | docs, `.dsp`, `.dsw`, `.vcproj`, Makefile, RC, scripts | なし | 維持 |
| `Win9x/*`, `Win9xC/*` | lowercase under `Win9x/`, `Win9xC/` | file/directory | 人間判断により配下 file/directory はすべて lowercase | docs, `.dsp`, `.dsw`, `.vcproj`, Makefile, RC, scripts | 高 | 実施 |
| `WinCE/` | 削除 | platform directory | 人間判断により directory ごと削除可 | `.dsp`, `.dsw`, `.vcp`, `.vcw`, RC, docs | 高 | 別 commit で削除 |
| `MacOS9/` | 削除 | platform directory | 人間判断により directory ごと削除可 | project/resource files, docs | 中 | 別 commit で削除 |
| `Mona/` | 削除 | platform directory | 人間判断により directory ごと削除可 | `.dsp`, `.dsw`, source, docs | 中 | 別 commit で削除 |
| `sdl/*` 大文字 path | lowercase | platform descendants | `sdl/` 配下の大文字 file/directory は人間判断により小文字化可 | `sdl/Makefile.*`, `sdl/sdlw32s.dsp`, source | 中 | 実施 |
| `HLP/` | `help/` | docs/assets directory | `hlp` より意味が明確な directory 名へ正規化。HTML charset header は `UTF-8` | HTML help launcher/docs, HTML internal links | 中 | 実施 |
| `ROMIMAGE/` | `romimage/` | source/data directory | 人間判断により小文字化可。内容変更なし | `ROMIMAGE/MAKEFILE.W32`, docs | 中 | 実施 |
| `ROMIMAGE/*` | lowercase under `romimage/` | file | 人間判断により配下 file はすべて lowercase | `ROMIMAGE/MAKEFILE.W32`, docs | 中 | 実施 |
| `NP2TOOL/` | `np2tool/` | directory | 人間判断により小文字化可 | docs, tool build command | 低 | 実施 |
| `NP2TOOL/*` | lowercase under `np2tool/` | file | 人間判断により配下 file は lowercase | docs, tool build command | 低 | 実施 |
| `NP2TOOL/MAKEFILE.W32` | `np2tool/Makefile.w32` | file | Makefile 慣例へ正規化 | docs, tool build command | 低 | 実施 |
| `ROMIMAGE/MAKEFILE.W32` | `romimage/Makefile.w32` | file | Makefile 慣例へ正規化 | docs, ROMIMAGE build command | 中 | 実施 |
| `accessories/*` | lowercase under `accessories/` | file | 人間判断により `.dsp`, `.dsw`, tool source を lowercase | docs, tool project files | 低 | 実施 |
| `README.TXT` | `README.txt` | file | 人間判断により lowercase extension へ正規化 | docs | 低 | 実施 |
| `README.md` | 維持 | file | 慣例名 | docs | なし | 維持 |

## Root File Approval

以下の root 直下ファイルは人間判断により小文字化可。

- `BREAKPOINT.C` -> `breakpoint.c`
- `BREAKPOINT.H` -> `breakpoint.h`
- `CALENDAR.C` -> `calendar.c`
- `COMMON.H` -> `common.h`
- `DEBUGSUB386.C` -> `debugsub386.c`
- `KEYSTAT.C` -> `keystat.c`
- `KEYSTAT.H` -> `keystat.h`
- `KEYSTAT.TBL` -> `keystat.tbl`
- `NEVENT.C` -> `nevent.c`
- `NEVENT.H` -> `nevent.h`
- `OPRECORD.C` -> `oprecord.c`
- `PCCORE.C` -> `pccore.c`
- `PCCORE.H` -> `pccore.h`
- `README.TXT` -> `README.txt`
- `STATSAVE.C` -> `statsave.c`
- `TIMING.C` -> `timing.c`

`README.md` は維持する。

## 参照更新対象

rename 実施時に、少なくとも以下の 38 ファイル群を参照更新対象に含める。

- `Win9x/Makefile`
- `sdl/Makefile.win`
- `sdl/Makefile.zau`
- `NP2TOOL/MAKEFILE.W32`
- `ROMIMAGE/MAKEFILE.W32`
- `Win9x/np2.dsp`
- `Win9x/np2.dsw`
- `Win9x/np2.vcproj`
- `Win9xC/np2c.dsp`
- `Win9xC/np2c.dsw`
- `WinCE/np2.dsp`
- `WinCE/np2.dsw`
- `WinCE/np2hpc.vcp`
- `WinCE/np2hpc.vcw`
- `WinCE/np2hpc_full.vcp`
- `WinCE/np2hpc_full.vcw`
- `WinCE/np2ppc.vcp`
- `WinCE/np2ppc.vcw`
- `WinCE/np2ppc_full.vcp`
- `WinCE/np2ppc_full.vcw`
- `WinCE/np2ppcv.vcp`
- `WinCE/np2ppcv.vcw`
- `WinCE/np2sig3.vcp`
- `WinCE/np2sig3.vcw`
- `Mona/mona.dsp`
- `Mona/mona.dsw`
- `sdl/sdlw32s.dsp`
- `sdl/sdlw32s.dsw`
- `accessories/bin2txt.dsp`
- `accessories/bin2txt.dsw`
- `accessories/lzxpack.dsp`
- `accessories/lzxpack.dsw`
- `Win9x/NP2.RC`
- `Win9x/NP2RES.RC`
- `Win9xC/np2.rc`
- `WinCE/W32/NP2.RC`
- `WinCE/WCE/NP2.RC`
- `WinCE/WCE/NP2PPCV.RC`

加えて、以下を全体 grep で追従確認する。

- `#include` の path
- `.RES` / `.res` include 参照
- HTML の `href` / `src`
- docs 内の path 記述
- `tools/windows/build_vc2008.cmd`

## 既知の注意点

- `CPUCVA/Z80c.cpp`, `CPUCVA/Z80c.h`, `Win9x/DEBUGUTY/*`, `IOVA/SUBSYSTEM.CPP` などに `Z80*.h` の大文字 include が残っている。`CPUCVA/Z80*.h` rename と同時に `z80*.h` へ更新する。
- `Mona/win32s/SDL*.h`, `sdl/win32s/SDL*.h`, `sdl/FONTMNG.C` には SDL 系の大文字 include がある。リポジトリ内 stub と外部 SDL header の区別を確認してから更新する。
- `MacOS9/` と `Mona/` は削除候補のため、配下の platform-specific header/source は rename 対象外。削除 commit で directory ごと取り除く。
- `Win9x/np2.vcproj` は Phase -1 の VC2008 build gate で使うため、rename 後も Debug / Release build gate を必ず実行する。
- `ROMIMAGE/` と `FONT/` は資産・互換 ROM 生成に関係するため、ファイル内容は変更しない。
- `HLP/` は内部 link が lowercase で揃っている。help viewer や docs から `HLP/` directory を参照している箇所を更新し、`help/` へ directory rename する。HTML の `charset=Shift_JIS` が残っていれば `charset=UTF-8` に更新する。

## 分割案

1. core shared directories: `common`, root core files, `bios`, `cbus`, `io`, `fdd`, `font`, `lio`, `sound`, `vram`, `i286c`
2. VA-specific directories: `biosva`, `cpucva`, `cpuxva`, `iova`, `vramva`, `win9x` build gate references
3. platform descendants: approved lowercase descendants under `Win9x/`, `Win9xC/`, and `sdl`
4. approved legacy platform deletion: remove `WinCE/`, `MacOS9/`, and `Mona/` in an independent commit after stale reference checks
5. docs/assets/tools: `help`, `romimage`, `np2tool`, `embed`, `generic`, `accessories`
6. remaining extension-only file renames and stale reference cleanup

分割時も各コミットは independently revertable にする。

## 検証計画

各 rename commit 後に実行する。

```sh
python3 - <<'PY'
import subprocess
from collections import defaultdict

paths = subprocess.check_output(["git", "ls-files"], text=True).splitlines()
by_lower = defaultdict(list)
for path in paths:
    by_lower[path.casefold()].append(path)
collisions = {key: vals for key, vals in by_lower.items() if len(vals) > 1}
assert not collisions, collisions
print("case-insensitive collisions: 0")
PY

git ls-files | grep -E '[A-Z]'
git grep -n "PCCORE\|COMMON\|I286C\|Win9x\|WinCE\|MacOS9\|MAKEFILE\.W32"
```

Windows build gate:

```bat
tools\windows\build_vc2008.cmd Debug
tools\windows\build_vc2008.cmd Release
```

## 停止条件

以下に該当したら rename を止めて確認する。

- `.dsp`, `.vcp`, `.vcproj` の path 更新方針が一意に決まらない。
- `HLP/` の HTML link または help project の参照更新が曖昧。
- `.RES` / `.res` の include 側参照が曖昧。
- `WinCE/`, `MacOS9/`, `Mona/` 削除後に stale reference が残り、削除してよい参照か判断できない。
- platform directory rename が 1 コミットでレビュー不能な規模になる。
- Windows build gate が rename 後に失敗し、原因を機械的 rename に限定できない。

## 推奨判断

まず `Win9x/np2.vcproj` を含む build gate を維持するため、Phase 1.3 は一括 rename ではなく分割実施する。

最初の rename commit は core shared directories に限定し、`Win9x/`, `Win9xC/`, `Mona/`, `HLP/`, `ROMIMAGE/` 周辺は後続 commit に分けるのが妥当。

`WinCE/` と `MacOS9/` は rename せず、stale reference check を行った上で独立削除 commit にする。
