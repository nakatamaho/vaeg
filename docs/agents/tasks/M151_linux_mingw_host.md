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

# M151 — Linux ホストの MinGW-w64 によるクリーンビルド

Status: **IN_PROGRESS; G151 NOT_READY.**
Branch: `milestone/m151-linux-mingw-host`（origin/main
[b1ad18d0](https://github.com/nakatamaho/vaeg/commit/b1ad18d0f67adf6f550d4a089341432198b1abf4) から）;
commit prefix `M151:`.

## 目標

Linux での静的 MinGW クリーンビルド（ゲートの「build from clean
checkout」）を、Docker なしで、ディストリビューションの MinGW-w64
（`/usr/bin/x86_64-w64-mingw32-*`）で行えるようにする（保守者の依頼）。

## 変更

- `tools/release/build-mingw-macports.py` を Linux x86_64 でも動くように
  した。ホストごとに Rust のホスト triple、既定のツールチェーン prefix
  （macOS `/opt/local`、Linux `/usr`）、レポートの表記を選ぶ。
  `x86_64-unknown-linux-gnu` の Rust 1.88.0（rustc、cargo、rust-std）は
  公式の SHA-256 で固定した。
- Linux では `x86_64-w64-mingw32-gcc` と `g++` のスレッドモデルが posix
  であることを要求し、そうでなければ `MACPORTS_THREAD_MODEL` で止まる。
  win32 モデルには録画が使う `std::thread` がない。
- CMake プリセット `mingw-macports` を prefix `/usr` で再利用する。
  ツールチェーン設定と依存 SDK（librashader、FFmpeg 8.1.2）、エンコーダ
  監査、インポート監査、ビルド識別は macOS と同じ。
- `AGENTS.md` と `BUILD.md` の Linux の標準手順をこのスクリプトに変え、
  コンテナ版 `build-mingw-static-container.sh` は明示の依頼時の選択肢と
  して残した。
- テスト `tests/frontend/test_mingw_macports.py` にホスト選択と
  スレッドモデル判定（正常系と単一変更の負例、エラーコード照合）を追加。

## 確認

- `test_mingw_macports.py` 5 tests OK。
- Linux x86_64（posix の MinGW-w64 GCC 13）で main `b1ad18d0` の
  クリーンクローンから `vaeg.exe` を生成。表示
  `Rel.20261115 (main, b1ad18d0)`、FFmpeg エンコーダ監査と Windows
  システム DLL インポート監査 PASS（`libwinpthread-1.dll` 依存なし）。
- Windows 上での実行は NOT_RUN。

## Gate

G151: 保守者が、このブランチの先端から Linux ホスト手順で作った
`vaeg.exe` を Windows で起動し、標準の human gate（V3 モードで起動、
同梱 VA デモ、OS 起動と簡単な操作）を行う。
