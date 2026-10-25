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

# M137 — N mode on VA/VA2 boots as an option

Status: **G137 human gate passed on 2026-10-25.**

Branch `milestone/m137-nmode-option`; commit prefix `M137:`.

## Request

The maintainer (2026-10-25): under 起動 > PC-88VA2/VA3 and PC-88VA, N mode
is selectable only while it is enabled; while it is disabled the I/O port
must not open it, so N mode cannot be reached ("feature not available").
詳細設定 turns N mode on and off for the VA. While it is off, the 起動
entries show a tooltip that it can be turned on in 詳細設定.

## Changes

- [fe888000](https://github.com/nakatamaho/vaeg/commit/fe88800024cab9b84f623fe386923756587f7686):
  - `V1V2_NModeEnable` (off by default, as the VA has no N-BASIC).
  - `romva_load_n80_rom()`: with N mode off no N-BASIC ROM is loaded, so
    port 31h RMODE keeps the VA's own banks and an N boot does not start N
    mode; "N mode: feature not available" is logged. The PC-8801/8001
    machines from `extra/` keep their own N-BASIC.
  - 起動 > PC-88VA2/VA3, PC-88VA: N items greyed while off, with the tooltip
    "N モード: feature not available / 模擬 > 詳細設定… の「N モード」で
    on/off できます" (a greyed N also when no ROM is found); a recent VA N
    boot is greyed likewise.
  - 詳細設定: "N モード (PC-88VA / VA2)" section; turning it off also leaves
    the N boot mode of a VA; a change resets the guest.
  - Selftest `N-BASIC ROM files` checks that no ROM loads with N mode off
    (the mutation removing the check is caught).
- [3f436e13](https://github.com/nakatamaho/vaeg/commit/3f436e13b4fc51217b9a3a08f99a17da6ebd738d):
  README; V1/V2 decision C9.

## Gate G137 (human)

Standard V3 gate, plus: with N モード off the VA's N items are greyed and
show the tooltip, and N-BASIC cannot be reached; with it on, N boots as
before; the PC-8801/8001 machines' N mode is unaffected.

## Local verification

- `--selftest`: exit 0.
- Linux release, VA2 with `BootMode=2` and an N-BASIC ROM in the ROM
  directory: `V1V2_NModeEnable=false` logs "N mode: feature not available"
  and does not start N-BASIC; `true` starts N-BASIC (PC-8001 BASIC 1.8).
- `linux-ci-gcc` ctest: 119 of 119.
- Media build regression reused: the media builders are unchanged since
  [bd781886](https://github.com/nakatamaho/vaeg/commit/bd78188638dfcd3bed41c696434fc245277adedd)
  (recorded in M134); since then only `demos/z80-timing` (M135) was added,
  which builds (`Z80TIME.COM`, exit 0) and is on no committed disk.

## Clean-checkout build

`tools/release/build-mingw-static-container.sh --commit
bee07c447602f0bcb8060c4d7ed0c4cda7e085f8`: commit embedded, Windows system
DLL imports only; `vaeg.exe` SHA-256
`e756fe70c56157776b94c8e0f88ae5c582084ac56e03a20611891008a0785491`.

## G137 passed

The maintainer stated **G137 passed** on 2026-10-25 with the static Windows
build of `bee07c447602f0bcb8060c4d7ed0c4cda7e085f8`.
