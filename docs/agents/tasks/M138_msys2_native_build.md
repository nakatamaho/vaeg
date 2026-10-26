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

# M138 — Windows-native (MSYS2) build fixes

Status: **implemented; G138 human gate pending.**

Branch `milestone/m138-msys2-build`; commit prefix `M138:`.

## Request

A report relayed by the maintainer (2026-10-25): the MSYS2 MINGW64 build
stopped. rustup could not be used with MSYS2's standard Rust (it had to be
removed and `mingw-w64-x86_64-rustup` installed), and
`build-static-librashader.py` failed reading a file as Shift_JIS.

## Findings

- `[CODE]` `build-static-librashader.py` used `Path.read_text()`,
  `write_text()` and `subprocess.check_output(text=True)` without an
  encoding, so Python used the locale code page (cp932 on Japanese
  Windows) for UTF-8 license files and Cargo output.
- `[CODE]` `build-mingw-static-librashader.sh` requires exactly Rust 1.88.0
  and the BUILD instructions use `rustup`, which MSYS2's own Rust package
  does not provide.

## Changes

- [f4ff0629](https://github.com/nakatamaho/vaeg/commit/f4ff0629355882f67f63979643e336fc5d8f617b): UTF-8 for every
  text read, write and command output of `build-static-librashader.py`.
- [2d5a88b9](https://github.com/nakatamaho/vaeg/commit/2d5a88b9a1ad331397c205a5ff58e15b9afbcd58): BUILD.md and
  README: remove `mingw-w64-x86_64-rust`, install `mingw-w64-x86_64-rustup`.

## Gate G138 (human)

The reporter's MSYS2 MINGW64 build (Japanese Windows) follows BUILD.md and
completes `tools/release/build-mingw-static-librashader.sh` and the
`mingw-release` build; standard V3 gate with the result.

## Local verification

- No local Rust or MSYS2 here. The script compiles (`py_compile`); no text
  read, write or command output without an encoding remains.
