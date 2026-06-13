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
- `makefile.w32` は `makefile.w32` に正規化する。
- `README.md` と `README.txt` は慣例名として維持する。
- case-only rename は必ず一時名を経由した 2 段階 `git mv` で行う。
- rename と clang-format、rename とロジック変更は同じコミットに混ぜない。
- `.dsp`, `.dsw`, `.vcp`, `.vcw`, `.vcproj`, makefile, RC, docs, HTML link の参照を追従更新する。
- `Win9x/np2.dsp` と `Win9x/np2.vcproj` は Windows build gate の台帳なので、rename 後も有効な参照に保つ。
- `WinCE/`, `MacOS9/`, `Mona/` は人間判断により directory ごと削除可。ただし rename commit には混ぜず、参照調査後に独立削除 commit とする。
- `Win9x/` と `Win9xC/` は directory 名を維持し、配下の大文字 file/directory はすべて lowercase にする。
- `sdl/` directory 自体は既に lowercase のため維持し、配下の大文字 file/directory は人間判断により小文字化可。
- root 直下の大文字ファイルは人間判断により小文字化可。ただし `README.md` は維持し、`README.txt` は `README.txt` にする。
- `common/` は `common/` にし、配下の大文字 file はすべて lowercase にする。
- `CPUCVA/` は現時点では directory 名を維持し、`CPUCVA/GVRAMVA.c` の拡張子のみ `GVRAMVA.c` にする。他の `CPUCVA/Z80*.h`, `z80c.cpp`, `z80diag.cpp`, `types.h` は現時点では維持する。
- `IO/` と `IOVA/` は directory 名を維持し、配下の大文字 file はすべて lowercase にする。
- `embed/` と `fdd/` はそれぞれ `embed/`, `fdd/` にし、配下の大文字 file/directory はすべて lowercase にする。
- `generic/` は `generic/` にし、配下の大文字 file はすべて lowercase にする。
- `sound/` は `sound/` にし、配下の大文字 file/directory はすべて lowercase にする。
- `font/` と `lio/` はそれぞれ `font/`, `lio/` にし、配下の大文字 file はすべて lowercase にする。
- `CBUS/` は directory 名を維持し、配下の大文字 file はすべて lowercase にする。
- `bios/`, `BIOSVA/`, `VRAM/` は directory 名を維持し、配下の大文字 file はすべて lowercase にする。
- `i286a/`, `i286c/`, `i286x/` はそれぞれ `i286a/`, `i286c/`, `i286x/` にし、配下の大文字 file はすべて lowercase にする。
- `VRAMVA/` は directory 名を維持し、配下の大文字 file はすべて lowercase にする。
- `CPUXVA/` は directory 名を維持し、配下の大文字 file はすべて lowercase にする。
- `np2tool/` は `np2tool/` にし、配下の大文字 file は lowercase にする。`makefile.w32` は `makefile.w32` にする。
- `CPUCVA/` は directory 名を維持し、`GVRAMVA.c` と `Z80*` files を lowercase にする。`types.h` は維持する。
- `romimage/` は `romimage/` にし、配下の大文字 file はすべて lowercase にする。`makefile.w32` は `makefile.w32` にする。
- `accessories/` は directory 名を維持し、配下の `.dsp`, `.dsw`, tool source を lowercase にする。
- `help/` は略称の `hlp/` ではなく、意味が明確な `help/` へ rename する。`nkf -w` で読めることを確認済みで、HTML の charset header は `UTF-8` にする。

## 集計

大文字を含む tracked path の主な内訳:

| グループ | 件数 | 方針 |
|---|---:|---|
| `Win9x/` | 135 | directory 名を維持し、配下 file/directory はすべて lowercase。人間承認済み |
| `help/` | 96 | `help/` へ。HTML charset header は `UTF-8`。人間承認済み |
| `WinCE/` | 73 | 削除候補。人間承認済み。rename ではなく独立削除 commit |
| `IO/` | 60 | directory 名を維持し、配下 file はすべて lowercase。人間承認済み |
| `MacOS9/` | 54 | 削除候補。人間承認済み。rename ではなく独立削除 commit |
| `sound/` | 54 | `sound/` へ。配下 file/directory はすべて lowercase。人間承認済み |
| `Mona/` | 52 | 削除候補。人間承認済み。rename ではなく独立削除 commit |
| `sdl/` 配下 | 52 | 配下の大文字 file/directory を小文字化。人間承認済み。`sdl/` 自体は維持 |
| `Win9xC/` | 51 | directory 名を維持し、配下 file/directory はすべて lowercase。人間承認済み |
| `romimage/` | 41 | `romimage/` へ。配下 file はすべて lowercase。`makefile.w32` は `makefile.w32`。人間承認済み |
| `CBUS/` | 38 | directory 名を維持し、配下 file はすべて lowercase。人間承認済み |
| `generic/` | 37 | `generic/` へ。配下 file はすべて lowercase。人間承認済み |
| `IOVA/` | 36 | directory 名を維持し、配下 file はすべて lowercase。人間承認済み |
| `embed/` | 30 | `embed/` へ。配下 file/directory はすべて lowercase。人間承認済み |
| `common/` | 28 | `common/` へ。配下 file はすべて lowercase。人間承認済み |
| `i286a/`, `i286c/`, `i286x/` | 73 | `i286a/`, `i286c/`, `i286x/` へ。配下 file はすべて lowercase。人間承認済み |
| `VRAM/`, `VRAMVA/` | 38 | directory 名を維持し、配下 file はすべて lowercase。人間承認済み |
| `bios/`, `BIOSVA/` | 23 | directory 名を維持し、配下 file はすべて lowercase。人間承認済み |
| `fdd/` | 17 | `fdd/` へ。配下 file はすべて lowercase。人間承認済み |
| `font/`, `lio/` | 22 | `font/`, `lio/` へ。配下 file はすべて lowercase。人間承認済み |
| `CPUCVA/`, `CPUXVA/` | 11 | `CPUCVA/` は維持し `GVRAMVA.c` と `Z80*` files を lowercase。`types.h` は維持。`CPUXVA/` は directory 名を維持し、配下 file はすべて lowercase |
| `np2tool/` | 7 | `np2tool/` へ。配下 file は lowercase。`makefile.w32` は `makefile.w32`。人間承認済み |
| root uppercase files | 18 | `pccore.c`, `common.h` などへ。`README.md` は維持、`README.txt` は `README.txt` |

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
| `.TXT` | 12 | 個別判断。`README.txt` は維持 |
| `.ASM` | 11 | `.asm` へ |
| `.MCR` | 9 | `.mcr` へ。clang-format 対象外のまま |
| `.RC` | 5 | `.rc` へ。resource compile 確認必須 |
| `.W32` | 2 | `makefile.w32` へ |

## 候補一覧

| 現在のパス | 提案するパス | 種別 | 理由 | 参照箇所 | リスク | 実施判断 |
|---|---|---|---|---|---|---|
| `common/` | `common/` | directory | makefile が既に `../common` を参照。人間承認済み | `Win9x/makefile`, `sdl/makefile.*`, `.dsp`, `.vcp`, `.vcproj`, docs | 中 | 実施 |
| `common/*` | lowercase under `common/` | file | 人間判断により配下 file はすべて lowercase | makefile, source, `.dsp`, `.vcp`, `.vcproj` | 中 | 実施 |
| `common.h` | `common.h` | file | include は lowercase 方針 | makefile, source, `.dsp`, `.vcp`, `.vcproj` | 低 | 実施 |
| `pccore.c` / `pccore.h` | `pccore.c` / `pccore.h` | file | makefile が `pccore.c/h` を参照 | makefile, `.dsp`, `.vcp`, `.vcproj`, source | 低 | 実施 |
| `NEVENT.*`, `TIMING.*`, `STATSAVE.*`, `KEYSTAT.*`, `BREAKPOINT.*`, `OPRECORD.*` | lowercase | file | root core files を include/build 参照に合わせる | makefile, `.dsp`, `.vcp`, `.vcproj`, source | 低 | 実施 |
| `bios/`, `BIOSVA/` | 維持 | directory | 人間判断により directory 名は維持 | makefile, `.dsp`, `.vcp`, `.vcproj`, docs | なし | 維持 |
| `bios/*`, `BIOSVA/*` | lowercase under `bios/`, `BIOSVA/` | file | 人間判断により配下 file はすべて lowercase | makefile, `.dsp`, `.vcp`, `.vcproj`, docs | 中 | 実施 |
| `CBUS/` | 維持 | directory | 人間判断により directory 名は維持 | makefile, `.dsp`, `.vcp`, `.vcproj` | なし | 維持 |
| `CBUS/*` | lowercase under `CBUS/` | file | 人間判断により配下 file はすべて lowercase | makefile, `.dsp`, `.vcp`, `.vcproj` | 中 | 実施 |
| `IO/`, `IOVA/` | 維持 | directory | 人間判断により directory 名は維持 | makefile, `.dsp`, `.vcp`, `.vcproj`, source | なし | 維持 |
| `IO/*`, `IOVA/*` | lowercase under `IO/`, `IOVA/` | file | 人間判断により配下 file はすべて lowercase | makefile, `.dsp`, `.vcp`, `.vcproj`, source | 中 | 実施 |
| `i286a/`, `i286c/`, `i286x/` | `i286a/`, `i286c/`, `i286x/` | directory | 人間判断により小文字化可 | makefile, `.dsp`, `.vcp`, `.vcproj`, source | 中 | 実施 |
| `i286a/*`, `i286c/*`, `i286x/*` | lowercase under `i286a/`, `i286c/`, `i286x/` | file | 人間判断により配下 file はすべて lowercase | makefile, `.dsp`, `.vcp`, `.vcproj`, source | 中 | 実施 |
| `CPUCVA/GVRAMVA.c` | `CPUCVA/GVRAMVA.c` | file | 人間判断により拡張子のみ小文字化可 | `Win9x/np2.dsp`, `Win9x/np2.vcproj` | 低 | 実施 |
| `CPUCVA/z80.h`, `CPUCVA/z80c.cpp`, `CPUCVA/z80c.h`, `CPUCVA/z80diag.cpp`, `CPUCVA/z80if.h` | lowercase under `CPUCVA/` | file | 人間判断により `z80*` にする | source, `.dsp`, `.vcproj` | 中 | 実施 |
| `CPUCVA/types.h` | 維持 | file | 既に lowercase | source, `.dsp`, `.vcproj` | なし | 維持 |
| `CPUXVA/` | 維持 | directory | 人間判断により directory 名は維持 | `Win9x/np2.dsp`, `Win9x/np2.vcproj` | なし | 維持 |
| `CPUXVA/*` | lowercase under `CPUXVA/` | file | 人間判断により配下 file はすべて lowercase | `Win9x/np2.dsp`, `Win9x/np2.vcproj` | 中 | 実施 |
| `VRAM/` | 維持 | directory | 人間判断により directory 名は維持 | makefile, `.dsp`, `.vcproj`, source | なし | 維持 |
| `VRAM/*` | lowercase under `VRAM/` | file | 人間判断により配下 file はすべて lowercase | makefile, `.dsp`, `.vcproj`, source | 中 | 実施 |
| `VRAMVA/` | 維持 | directory | 人間判断により directory 名は維持 | `Win9x/np2.dsp`, `Win9x/np2.vcproj`, source | なし | 維持 |
| `VRAMVA/*` | lowercase under `VRAMVA/` | file | 人間判断により配下 file はすべて lowercase | `Win9x/np2.dsp`, `Win9x/np2.vcproj`, source | 中 | 実施 |
| `sound/` | `sound/` | directory | makefile が `../sound` を参照。人間承認済み | makefile, `.dsp`, `.vcp`, `.vcproj`, source | 中 | 実施 |
| `sound/*` | lowercase under `sound/` | file/directory | 人間判断により配下 file/directory はすべて lowercase | makefile, `.dsp`, `.vcp`, `.vcproj`, source | 中 | 実施 |
| `fdd/`, `embed/` | `fdd/`, `embed/` | directory | makefile と include 参照に合わせる。人間承認済み | makefile, `.dsp`, `.vcp`, `.vcproj`, source | 中 | 実施 |
| `fdd/*`, `embed/*` | lowercase under `fdd/`, `embed/` | file/directory | 人間判断により配下 file/directory はすべて lowercase | makefile, `.dsp`, `.vcp`, `.vcproj`, source | 中 | 実施 |
| `generic/` | `generic/` | directory | makefile と include 参照に合わせる。人間承認済み | makefile, `.dsp`, `.vcp`, `.vcproj`, source | 中 | 実施 |
| `generic/*` | lowercase under `generic/` | file | 人間判断により配下 file はすべて lowercase | makefile, `.dsp`, `.vcp`, `.vcproj`, source | 中 | 実施 |
| `font/`, `lio/` | `font/`, `lio/` | directory | 人間判断により小文字化可 | makefile, `.dsp`, `.vcp`, `.vcproj`, source | 中 | 実施 |
| `font/*`, `lio/*` | lowercase under `font/`, `lio/` | file | 人間判断により配下 file はすべて lowercase | makefile, `.dsp`, `.vcp`, `.vcproj`, source | 中 | 実施 |
| `Win9x/`, `Win9xC/` | 維持 | platform directory | 人間判断により directory 名は維持 | docs, `.dsp`, `.dsw`, `.vcproj`, makefile, RC, scripts | なし | 維持 |
| `Win9x/*`, `Win9xC/*` | lowercase under `Win9x/`, `Win9xC/` | file/directory | 人間判断により配下 file/directory はすべて lowercase | docs, `.dsp`, `.dsw`, `.vcproj`, makefile, RC, scripts | 高 | 実施 |
| `WinCE/` | 削除 | platform directory | 人間判断により directory ごと削除可 | `.dsp`, `.dsw`, `.vcp`, `.vcw`, RC, docs | 高 | 別 commit で削除 |
| `MacOS9/` | 削除 | platform directory | 人間判断により directory ごと削除可 | project/resource files, docs | 中 | 別 commit で削除 |
| `Mona/` | 削除 | platform directory | 人間判断により directory ごと削除可 | `.dsp`, `.dsw`, source, docs | 中 | 別 commit で削除 |
| `sdl/*` 大文字 path | lowercase | platform descendants | `sdl/` 配下の大文字 file/directory は人間判断により小文字化可 | `sdl/makefile.*`, `sdl/sdlw32s.dsp`, source | 中 | 実施 |
| `help/` | `help/` | docs/assets directory | `hlp` より意味が明確な directory 名へ正規化。HTML charset header は `UTF-8` | HTML help launcher/docs, HTML internal links | 中 | 実施 |
| `romimage/` | `romimage/` | source/data directory | 人間判断により小文字化可。内容変更なし | `romimage/makefile.w32`, docs | 中 | 実施 |
| `romimage/*` | lowercase under `romimage/` | file | 人間判断により配下 file はすべて lowercase | `romimage/makefile.w32`, docs | 中 | 実施 |
| `np2tool/` | `np2tool/` | directory | 人間判断により小文字化可 | docs, tool build command | 低 | 実施 |
| `np2tool/*` | lowercase under `np2tool/` | file | 人間判断により配下 file は lowercase | docs, tool build command | 低 | 実施 |
| `np2tool/makefile.w32` | `np2tool/makefile.w32` | file | makefile 慣例へ正規化 | docs, tool build command | 低 | 実施 |
| `romimage/makefile.w32` | `romimage/makefile.w32` | file | makefile 慣例へ正規化 | docs, ROMIMAGE build command | 中 | 実施 |
| `accessories/*` | lowercase under `accessories/` | file | 人間判断により `.dsp`, `.dsw`, tool source を lowercase | docs, tool project files | 低 | 実施 |
| `README.txt` | `README.txt` | file | 人間判断により lowercase extension へ正規化 | docs | 低 | 実施 |
| `README.md` | 維持 | file | 慣例名 | docs | なし | 維持 |

## Root File Approval

以下の root 直下ファイルは人間判断により小文字化可。

- `breakpoint.c` -> `breakpoint.c`
- `breakpoint.h` -> `breakpoint.h`
- `calendar.c` -> `calendar.c`
- `common.h` -> `common.h`
- `debugsub386.c` -> `debugsub386.c`
- `keystat.c` -> `keystat.c`
- `keystat.h` -> `keystat.h`
- `keystat.tbl` -> `keystat.tbl`
- `nevent.c` -> `nevent.c`
- `nevent.h` -> `nevent.h`
- `oprecord.c` -> `oprecord.c`
- `pccore.c` -> `pccore.c`
- `pccore.h` -> `pccore.h`
- `README.txt` -> `README.txt`
- `statsave.c` -> `statsave.c`
- `timing.c` -> `timing.c`

`README.md` は維持する。

## 参照更新対象

rename 実施時に、少なくとも以下の 38 ファイル群を参照更新対象に含める。

- `Win9x/makefile`
- `sdl/makefile.win`
- `sdl/makefile.zau`
- `np2tool/makefile.w32`
- `romimage/makefile.w32`
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
- `Win9x/np2.rc`
- `Win9x/np2res.rc`
- `Win9xC/np2.rc`
- `WinCE/W32/np2.rc`
- `WinCE/WCE/np2.rc`
- `WinCE/WCE/NP2PPCV.RC`

加えて、以下を全体 grep で追従確認する。

- `#include` の path
- `.RES` / `.res` include 参照
- HTML の `href` / `src`
- docs 内の path 記述
- `tools/windows/build_vc2008.cmd`

## 既知の注意点

- `CPUCVA/z80c.cpp`, `CPUCVA/z80c.h`, `Win9x/debuguty/*`, `IOVA/subsystem.cpp` などに `Z80*.h` の大文字 include が残っている。`CPUCVA/Z80*.h` rename と同時に `z80*.h` へ更新する。
- `Mona/win32s/SDL*.h`, `sdl/win32s/SDL*.h`, `sdl/fontmng.c` には SDL 系の大文字 include がある。リポジトリ内 stub と外部 SDL header の区別を確認してから更新する。
- `MacOS9/` と `Mona/` は削除候補のため、配下の platform-specific header/source は rename 対象外。削除 commit で directory ごと取り除く。
- `Win9x/np2.vcproj` は Phase -1 の VC2008 build gate で使うため、rename 後も Debug / Release build gate を必ず実行する。
- `romimage/` と `font/` は資産・互換 ROM 生成に関係するため、ファイル内容は変更しない。
- `help/` は内部 link が lowercase で揃っている。help viewer や docs から `help/` directory を参照している箇所を更新し、`help/` へ directory rename する。HTML の `charset=UTF-8` が残っていれば `charset=UTF-8` に更新する。

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
git grep -n "PCCORE\|common\|i286c\|Win9x\|WinCE\|MacOS9\|MAKEFILE\.W32"
```

Windows build gate:

```bat
tools\windows\build_vc2008.cmd Debug
tools\windows\build_vc2008.cmd Release
```

## 停止条件

以下に該当したら rename を止めて確認する。

- `.dsp`, `.vcp`, `.vcproj` の path 更新方針が一意に決まらない。
- `help/` の HTML link または help project の参照更新が曖昧。
- `.RES` / `.res` の include 側参照が曖昧。
- `WinCE/`, `MacOS9/`, `Mona/` 削除後に stale reference が残り、削除してよい参照か判断できない。
- platform directory rename が 1 コミットでレビュー不能な規模になる。
- Windows build gate が rename 後に失敗し、原因を機械的 rename に限定できない。

## 推奨判断

まず `Win9x/np2.vcproj` を含む build gate を維持するため、Phase 1.3 は一括 rename ではなく分割実施する。

最初の rename commit は core shared directories に限定し、`Win9x/`, `Win9xC/`, `Mona/`, `help/`, `romimage/` 周辺は後続 commit に分けるのが妥当。

`WinCE/` と `MacOS9/` は rename せず、stale reference check を行った上で独立削除 commit にする。
