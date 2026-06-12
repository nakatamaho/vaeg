# Windows / Visual C++ 2008 Baseline Build

## 目的

リファクタリング前の Windows / Visual C++ 2008 ビルドを、以後の変更に対する build gate として記録する。

## 対象リポジトリ状態

- Branch: `main`
- HEAD: `841e08b CVS から Git へ移行`
- 作業開始時の未追跡ファイル: `AGENTS.md`, `refactor-instructions.md`
- VS2008 baseline 用に追加・変更したファイル:
  - `Win9x/np2.vcproj`
  - `Win9x/np2rc.h`
  - `Win9x/NP2.RC`
  - `Win9x/DD2.CPP`
  - `Win9x/SCRNMNG.CPP`
  - `I286X/MEMORY.X86`
  - `I286X/EGCMEM.X86`
  - `Win9x/x86/OPNGENG.X86`
  - `Win9x/x86/MAKEGRPH.X86`
  - `Win9x/np2.dsp`
  - `Win9x/Makefile`

## Windows 環境

- Windows 実機で確認。
- Visual Studio: Visual C++ 2008 Express / Visual Studio 9.0。
- `devenv.exe`: なし。
- `VCExpress.exe`: `C:\Program Files (x86)\Microsoft Visual Studio 9.0\Common7\IDE\VCExpress.exe`
- Resource Compiler: `Microsoft (R) Windows (R) Resource Compiler Version 6.1.6723.1`
- NASM: `C:\Program Files\NASM\nasm.exe`
- NASM は PATH 経由で `nasm` として実行。
- DirectDraw は `ddraw.lib` へ追加リンクせず、`ddraw.dll` を実行時ロードする。

## 使用した Project / Solution

- Project: `Win9x/np2.vcproj`
- 元ファイル: `/mnt/c/Users/maho/vaeg-orig/Win9x/np2.vcproj`
- Solution: 未追加。必要なら Visual Studio 2008 が project から solution を生成できる。

## Baseline 用の環境差吸収

以下は既存挙動の変更を意図したものではなく、VC2008 build gate を再現するための最小修正。

- `Win9x/NP2.RC`: `afxres.h` 依存を外し、`np2rc.h` を include。
- `Win9x/np2rc.h`: MFC resource header なしで resource compile するための最小 resource 定義。
- `Win9x/DD2.CPP`, `Win9x/SCRNMNG.CPP`: `DirectDrawCreate` を link-time import せず、`LoadLibraryA("ddraw.dll")` と `GetProcAddress` で実行時解決。
- `I286X/MEMORY.X86`, `I286X/EGCMEM.X86`, `Win9x/x86/OPNGENG.X86`, `Win9x/x86/MAKEGRPH.X86`: NASM / COFF で外部 alias が未解決シンボルとして残る箇所を `equ` から `%define` へ変更。
- `Win9x/np2.dsp`, `Win9x/Makefile`: `nasmw` / 固定パスを使わず `nasm` を使う。

## 実行した Build

Visual Studio 2008 で `Debug|Win32` を rebuild。

```text
Project: np2
Configuration: Debug|Win32
Output: ..\bin\vaegd.exe
Build log: obj\dbg\BuildLog.htm
Result: 1 succeeded, 0 failed, 0 skipped
Errors: 0
Warnings: 47
```

## 生成物

- `bin\vaegd.exe`
- `bin\vaegd.pdb`
- `obj\dbg\BuildLog.htm`

ファイルサイズ、タイムスタンプ、hash は未記録。次回 Windows 環境で確認する。

## 実行確認

- `bin\vaegd.exe` の起動を確認。
- デモ投入で動作確認済み。
- PC-Engine 動作確認済み。

## Release Build

PowerShell から `tools\windows\build_vc2008.cmd Release` を実行。

```text
Project: np2
Configuration: Release|Win32
Output: ..\bin\vaeg.exe
Build log: obj\rel\BuildLog.htm
Script log: obj\vc2008\release.log
Result: 1 succeeded, 0 failed, 0 skipped
Errors: 0
Warnings: 42
```

最低成功条件である `bin\vaeg.exe` 生成は満たした。

## 既知の警告

- NASM: `label ... alone on a line without a colon might be in error [-w+label-orphan]`
- MSVC: `C4996` (`strcat`, `strcpy`, `sprintf`, `vsprintf`, `fopen`)
- MSVC: `C4101` unused local variables
- MSVC: `C4244` narrowing conversion

現時点では baseline 阻害要因ではないため、挙動変更を避ける目的で修正しない。

## 再現手順

PowerShell または Command Prompt から:

```bat
cd /d C:\Users\maho\vaeg
tools\windows\build_vc2008.cmd Debug
```

Release 確認:

```bat
cd /d C:\Users\maho\vaeg
tools\windows\build_vc2008.cmd Release
```

両方を確認する場合:

```bat
cd /d C:\Users\maho\vaeg
tools\windows\build_vc2008.cmd all
```

## 次に進めるか

`Debug|Win32` は build / run 済み。`Release|Win32` は build 済み。Phase 1 以降では Windows build gate を「Debug 成功、Release 成功」として扱う。
