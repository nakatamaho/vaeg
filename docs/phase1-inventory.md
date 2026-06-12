# Phase 1.0 Inventory

## 目的

Phase 1 の意味変更を意図しない浄化に入る前に、変換・rename・整形の対象と除外候補を記録する。

この文書ではコード変換、ファイル名変更、整形は行わない。

## 前提状態

- Branch: `main`
- HEAD: `841e08b CVS から Git へ移行`
- tracked files: `1058`
- uppercase paths: `1050`
- case-insensitive collision: `0`
- zlib / inflate / deflate references: `0`
- writetag references: `0`

作業ツリーには Phase -1 baseline 用の変更がある。

## Windows Build Gate

- `Debug|Win32`: 成功
- 生成物: `bin\vaegd.exe`
- 結果: エラー 0、警告 47
- 実行確認: デモ動作、PC-Engine 動作
- `Release|Win32`: 成功
- 生成物: `bin\vaeg.exe`
- 結果: エラー 0、警告 42

詳細は `docs/windows-vc2008-baseline.md` を参照。

## 文字コード候補

全 tracked files を対象に、非 ASCII バイトと CP932 デコード可否を確認した。

```text
non_ascii: 451
cp932_ok: 402
cp932_ng: 49
```

`cp932_ng` の 49 件は、主に画像・音声・圧縮ファイルなどの binary asset。

代表例:

- `HLP/images/*.gif`
- `HLP/images/toolwinva.png`
- `HLP/vaeg.bmp`
- `Win9x/ICONS/*.WAV`
- `Win9x/ICONS/*.BMP`
- `Win9x/ICONS/*.ICO`
- `Win9xC/ICONS/*.BMP`
- `Win9xC/ICONS/*.ICO`
- `WinCE/W32/NP2.ICO`
- `WinCE/WCE/NP2.ICO`
- `MacOS9/mkres.lzh`

テキスト候補だけに絞ると以下。

```text
text_candidates: 1006
text_non_ascii: 405
text_cp932_ok: 402
text_cp932_ng: 3
```

テキスト候補の `cp932_ng`:

- `MacOS9/np2.proj`
- `MacOS9/np2.r`
- `MacOS9/np2classic.proj`

これらは Classic MacOS 系 resource/project であり、Phase 1.1 の初回 UTF-8 変換対象から除外する。

## 非 ASCII ファイル分類

内容と拡張子による機械分類:

| 分類 | 件数 | 代表例 |
|---|---:|---|
| source-or-header | 270 | `BIOS/BIOS.C`, `COMMON/CODECNV.C`, `Win9x/SCRNMNG.CPP` |
| html/help/assets | 95 | `HLP/about.html`, `HLP/common.css`, `HLP/images/*.gif` |
| other-or-binary | 39 | `CPUXVA/MEMORYVA.X86`, `MacOS9/mkres.lzh`, `ROMIMAGE/*.ASM` |
| project-file | 19 | `Mona/mona.dsp`, `Win9x/np2.dsp`, `WinCE/*.vcp` |
| text-doc | 13 | `README.TXT`, `README.md`, `Win9x/README.TXT` |
| table-or-include-data | 11 | `KEYSTAT.TBL`, `GENERIC/*.RES`, `I286C/*.MCR` |
| rc-resource | 4 | `Win9x/NP2.RC`, `Win9xC/np2.rc`, `WinCE/W32/NP2.RC`, `WinCE/WCE/NP2PPCV.RC` |

## Source / Header の文字コード

C/C++ source/header の集計:

```text
source/header total: 758
ascii-only: 488
cp932-ok with non-ascii: 270
cp932-ng: 0
```

Phase 1.1 で source/header を変換する場合、CP932 デコード不能な C/C++ ファイルは現時点では見つかっていない。

## 実行時文字列を含む可能性が高いファイル

初回変換時に特に注意する。

- `Win9x/NP2.RC`
- `Win9xC/np2.rc`
- `WinCE/W32/NP2.RC`
- `WinCE/WCE/NP2PPCV.RC`
- `KEYSTAT.TBL`
- `Win9x/*.CPP` の dialog/menu/message 系
- `HLP/*.html`

`Win9x/NP2.RC` は `#pragma code_page(932)` を含むため、Phase 1.1 初回では変換対象から除外する。

## 大文字 Path

tracked path のうち大文字を含むもの:

```text
1050
```

代表例:

- `PCCORE.C`, `PCCORE.H`
- `COMMON/`
- `I286C/`
- `I286X/`
- `CPUXVA/`
- `CPUCVA/`
- `IOVA/`
- `VRAMVA/`
- `Win9x/`
- `WinCE/`
- `MacOS9/`
- `ROMIMAGE/`
- `NP2TOOL/`

Phase 1.3 では候補一覧と参照元を提示し、承認後に段階的に rename する。一括 rename は行わない。

## makefile / Makefile

確認結果:

- `NP2TOOL/MAKEFILE.W32`
- `ROMIMAGE/MAKEFILE.W32`
- `Win9x/Makefile`
- `sdl/Makefile.win`
- `sdl/Makefile.zau`

Phase 1.3 では `MAKEFILE.W32` は `Makefile.w32` 形へ正規化候補とする。

## Visual Studio 系ファイル

tracked `.dsp` / `.dsw`:

- `Mona/mona.dsp`
- `Mona/mona.dsw`
- `Win9x/np2.dsp`
- `Win9x/np2.dsw`
- `Win9xC/np2c.dsp`
- `Win9xC/np2c.dsw`
- `WinCE/np2.dsp`
- `WinCE/np2.dsw`
- `accessories/bin2txt.dsp`
- `accessories/bin2txt.dsw`
- `accessories/lzxpack.dsp`
- `accessories/lzxpack.dsw`
- `sdl/sdlw32s.dsp`
- `sdl/sdlw32s.dsw`

tracked `.sln` / `.vcproj`:

- なし

Phase -1 で追加した未追跡 project:

- `Win9x/np2.vcproj`

`.dsp` ごとの `SOURCE=` 件数:

| DSP | SOURCE 件数 | 判定 |
|---|---:|---|
| `Win9x/np2.dsp` | 261 | vaeg の既存 source list 台帳。削除禁止 |
| `sdl/sdlw32s.dsp` | 186 | SDL/win32s 系 frontend 台帳 |
| `WinCE/np2.dsp` | 182 | WinCE frontend 台帳 |
| `Win9xC/np2c.dsp` | 169 | Win9xC frontend 台帳 |
| `Mona/mona.dsp` | 138 | Mona frontend 台帳 |
| `accessories/bin2txt.dsp` | 4 | tool 台帳 |
| `accessories/lzxpack.dsp` | 4 | tool 台帳 |

## clang-format 候補と除外候補

候補:

- C/C++ source/header: `758` files

初回除外候補:

- `*.MCR`
- `*.TBL`
- `*.RES`
- `*.STR`
- `*.X86`
- `*.S`
- `*.ASM`
- `*.RC`
- `*.INC`
- `HLP/`
- `ROMIMAGE/`
- `MacOS9/`
- `WinCE/`
- `Mona/`

拡張子ベースで対象判定しきれないため、Phase 1.7 では適用前に対象ファイル一覧を明示する。

## 追加確認コマンド

```sh
git ls-files | wc -l
git ls-files | grep -E '[A-Z]' | wc -l
git ls-files | grep -Ei '(^|/)makefile'
git ls-files | grep -Ei '\.(dsp|dsw|sln|vcproj)$'
git grep -in "zlib\|inflate\|deflate"
git grep -in "writetag"
```

case-insensitive collision check:

```sh
python3 - <<'PY'
import subprocess, collections, sys
paths = subprocess.check_output(["git", "ls-files"], text=True).splitlines()
groups = collections.defaultdict(list)
for path in paths:
    groups[path.lower()].append(path)
collisions = {key: values for key, values in groups.items() if len(values) > 1}
for key, values in sorted(collisions.items()):
    print("CASE COLLISION:")
    for value in values:
        print(f"  {value}")
sys.exit(1 if collisions else 0)
PY
```

## 次に進めるか

Phase 1.0 の inventory は作成済み。次に進む場合は Phase 1.1 の UTF-8 変換計画を、対象・除外・検証コマンド込みで提示してから実施する。
