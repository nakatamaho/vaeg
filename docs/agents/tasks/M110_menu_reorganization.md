<!--
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
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

# M110 — Menu reorganization (and the plan for further machines)

Status: **in progress**

## Request

The maintainer asked (2026-10-14) to take stock of the SDL2/ImGui menu:
group what is used, remove what is unused or permanently greyed out, rename
エミュレート to 模擬, and prepare 模擬 for further machines (PC-8801 and
PC-8001mkII/SR families from user-supplied ROMs, M111 onwards). The menu
below was agreed item by item in the same session.

## Decisions

- [POLICY] Top-level menus, in this order:
  模擬 / FDD / ハードディスク / テープ / 編集 / 画面 / キー / 音 / 状態 / 情報.
- [POLICY] Labels are Japanese throughout (machine and format names such
  as OPNA, ymfm, SASI stay as written).
- [POLICY] Permanently unimplemented, always-greyed items are removed:
  FDD3/FDD4 open and eject, Rotate, Screen option, Mechanical keys, Serial
  option, MIDI option, the mouse-status line, the fixed Base clock combo
  in the settings window, and the "no N-BASIC ROM" placeholder.
  Items unavailable only for the moment (Rewind or Eject with no tape,
  for example) stay greyed out, so that items do not move.
- [POLICY] Options that never apply to the current machine are hidden, not
  greyed: OPN (VA only), ymfm fidelity (ymfm only), ハードディスク (VA only).
- [POLICY] The Pause button goes; 一時停止 becomes an F12 assignment, and a
  paused machine shows that it is paused on screen.
- [POLICY] The uPD8087 is on by default on every VA model (no software for
  the original VA uses it, which the maintainer accepts); it is switched off
  in the settings window. The "VA2/VA3 + 8087" model entry goes. A saved
  `NDP8087=false` keeps precedence.
- [POLICY] Speed has two controls in 模擬 that act differently:
  実行速度 (10-400 %, 最大 = No Wait, back to 100 %) slows frames, timers
  and sound together; CPU クロック changes the CPU alone, frames, timers and
  music keeping real time, which is what matching a PC-8801's speed needs.
  CPU クロック is a list of presets with the current MHz in its heading:
  機種の既定 (from the machine and boot mode, the default), PC-8801
  標準速度相当, PC-8801 高速相当, 2倍, 4倍, 8倍, and 任意… (a slider that
  replaces today's CPU 速度 10-100 % and the settings window's CPU
  multiplier). On the VA's V1/V2 mode the uPD9002 runs the Z80-compatible
  instructions with its own cycle counts and waits, so the PC-8801 presets
  are defined as "close to the machine" and calibrated from hardware
  measurements later; on the PC-8801 machines (M113) they are 4 MHz and
  8 MHz. A clock away from the default is shown by the CPU-clock display.
- [POLICY] KEY_MODE becomes カーソルキーの使い方: 通常 / ジョイスティック
  (JoyKey-1) / マウス (Mouse key, kept only if it works when checked).
  JoyKey-2 is removed: it feeds the PC-98 FM board's second joystick
  connector, which vaeg never reads, so it only disabled the keys; a saved
  2 becomes 通常.
- [POLICY] New FDD images in the order 2HD, 2D (320 KB), 2DD (640 KB), then
  after a separator 2D (360 KB), 2DD (720 KB).
- [POLICY] Recording (not implemented yet) will go into 画面 next to the
  screenshots, with an F12 assignment.
- [POLICY] Screen rotation is planned (maintainer). Its greyed placeholder
  goes now; when implemented it appears in 画面 as 画面の回転 ▶ (0°, 90°
  left, 90° right, 180°).
- [POLICY] Saved states, screenshots and (future) recordings go into
  `states/`, `screenshots/` and `recordings/` in the directory holding
  `vaeg.exe` (the executable's directory from `SDL_GetBasePath`, not the
  current directory), next to the ROMs and `vaeg-cache/`; if that directory
  is not writable, the user directory (where `vaeg.cfg` lives) is used and
  the menu says so. The existing `np2sdl.S00`-`S09` slots are moved into
  `states/` at first start. Each slot is saved with a thumbnail PNG of the
  guest frame at that moment (unprocessed, about 320x200): `slotN.sav` and
  `slotN.png`. The 保存/読み込み lists show each slot's date and time (空き
  when unused) and its thumbnail on hover, and 一覧… opens a window of the
  ten thumbnails, clicked to save or load. 名前を付けて保存… and
  ファイルから読み込み… are added. Slots are kept per software in
  `states/<software>/`, the name taken from the FDD1 image, else the SASI-1
  image, else the tape, else 共通. The name is the image's file name made
  safe for the host file system, followed by a short hash of its full path
  so that equal names from different directories stay apart. It is fixed
  when a state of that software is first saved, so a disk swap during play
  keeps the slots together; loading a slot made with another disk keeps
  the existing disk-change warning. `vaeg-cache/` (librashader shader
  files), today created in the current directory, is also created in the
  executable's directory, so that a shortcut or command line starting vaeg
  elsewhere no longer scatters it (maintainer, 2026-10-15). Screenshots
  are not split per software: they all go into `screenshots/` with
  today's date-and-time names (maintainer, 2026-10-15); headless
  `--screenshot` keeps its explicit paths. Under discussion: a
  ソフトごと/共通 switch for the state slots.
- Existing configuration keys stay readable and are still written, so an
  older vaeg reads a new configuration file; command-line options and the
  headless tests keep working.

## Agreed menu

```
模擬
 ├─ リセット
 ├─ 起動 ▶
 │   ├─ PC-88VA2/VA3 ▶ ● 自動 (ディスクで判別)
 │   │                  ○ V3                         (M111)
 │   │                  ○ V2 H / V2 S
 │   │                  ○ V1 H / V1 S
 │   │                  ○ N  (vaeg extension; when an N-BASIC ROM is present)
 │   ├─ PC-88VA      ▶   (same)
 │   ├─ PC-8801MA    ▶ V2 / V1 H / V1 S / N          (M112+, when its ROMs are present)
 │   ├─ PC-8801MC    ▶   (same)                      (M112+)
 │   ├─ PC-8001mkII  ▶ N / N80                       (M112+)
 │   └─ PC-8001mkIISR▶ N / N80 / N80SR               (M112+)
 ├─ 実行速度  [slider] 10-400 %, 最大 = No Wait
 ├─ 実行速度を 100% に戻す
 ├─ CPU クロック ▶ (current MHz)  ● 機種の既定 / PC-8801 標準速度相当 /
 │                 PC-8801 高速相当 / ── / 2倍 / 4倍 / 8倍 / 任意…
 ├─ 機種の構成 ▶
 │   ├─ メインメモリ容量 ▶
 │   ├─ バックアップメモリへ自動反映
 │   ├─ ハードディスクから起動
 │   ├─ I/O バンクメモリ…
 │   ├─ EMS ボード…
 │   └─ PCG (PCG-8100 / PCG-8800)  (M115+, 8001/8801 only)
 ├─ 詳細設定…  (8087 on/off and clock, SGP speed, HOSTFAT)
 └─ 終了

FDD           FDD1/FDD2 開く…/取り出し (with their contents),
              新しい FDD イメージ ▶ 2HD / 2D (320 KB) / 2DD (640 KB) / ── /
              2D (360 KB) / 2DD (720 KB)
ハードディスク SASI-1/2, SCSI IDs 開く…/外す, 新しい SASI/SCSI イメージ… (VA only)
テープ        読み込み・録音…, 巻き戻し, 取り出し, 録音を止める, Fast load (off by default)
編集          画面の文字をコピー, 貼り付け, 貼り付けの取り消し
画面          スクリーンショットを保存 (and 加工前), 録画 (future),
              モニタ ▶ 24 kHz / 15 kHz (リセット), デジタル RGB (8色),
                       15 kHz 走査線: 隙間あり / 埋める, V1/V2 表示: VA 標準 / PC-8801 風,
              描画方式 ▶ (and CRT 設定…), エフェクト ▶, 拡大方法 ▶,
              ウィンドウサイズ ▶, 縦横比の補正, 全画面表示, 前面パネル,
              フレームスキップ ▶, フォント ▶, 画面の回転 ▶ (future)
キー          カーソルキーの使い方 ▶ 通常 / ジョイスティック / マウス,
              テンキー代わり, ホストの配列 ▶, かな入力 ▶,
              F12 の割り当て ▶ (adds 一時停止; 録画 later), キーの割り当て…,
              マウスを捕捉, コントローラポート ▶, 連射ボタン
音            音を出す, FM 音源ボード ▶ (OPN only on the VA) / OPNA,
              FM の方式 ▶, ymfm の精度 ▶ (ymfm only), サンプリング周波数 ▶,
              バッファ ▶, テープの音量, PCG 音源の音量 (M115+, PCG on only), シーク音
状態          保存 ▶, 読み込み ▶ (date, time and thumbnail per slot), 一覧…,
              名前を付けて保存…, ファイルから読み込み…
情報          display toggles (FDD, FPS, CPU/SGP clock, frame, video and FB info),
              layer toggles (text, sprite, graphics 0/1), About
```

## Scope of M110

Implement the agreed menu for the VA models: the reorganization, the
removals, Pause as an F12 assignment with an on-screen indication, the
speed controls, 8087 on by default, the cursor-key setting, the FDD image
order, Japanese labels, and the configuration compatibility above. 起動
offers the VA rows with 自動, V2 H/S, V1 H/S and N (today's choices); V3
and the other machines come later. Update the READMEs that name menu paths
(historical task files and the bug-fix ledger keep their wording).

Checks: the 8087-on default re-runs the all-demos programs and an OS boot
on the VA and the VA2; a state file saved without the 8087 still loads;
`--model` and the headless scripts keep working; every menu item is
exercised once.

## Later stages (planned, not part of M110)

- M111: 起動 as machine ▶ boot mode for the VA, including V3, V2 and V1
  forced at boot without writing the backup-memory file (method from the
  ROM's behaviour), and N moved into it. Configuration keys `Machine=` and
  `BootMode=`, migrated from `model`, `v1v2_nmode`, `v1v2_standard` and the
  V1 bit.
- M112: extra ROMs. The VA ROMs and wave files stay where they are (next
  to `vaeg.exe`, or the `--roms` directory, then the current directory);
  extra ROMs go below it in `extra/<machine>/`, named after the machine
  (`extra/PC-8801MA/`, `extra/PC-8801MC/`, `extra/PC-8001mkII/`,
  `extra/PC-8001mkIISR/`). Files are identified by SHA-1 and ROM type
  only, whatever their names (as `n80rom_identify_sha1` does); unknown
  dumps are listed and not used. The N-BASIC ROM is also read from
  `extra/`, and still from the old place. Only machines whose ROMs are
  complete appear under 起動.
- M113, M114: PC-8801 and PC-8001mkII/mkIISR machines (order may be
  swapped), reusing the VA's V1/V2 machinery. Keyboard: the VA keyboard
  (FH/MH-compatible layout and the existing PC-8801 key matrix); keys a
  8001 lacks are ignored. Hard disks stay attached but hidden and unused
  outside the VA. The front-panel lamps follow the machine.
- M115: PCG-8100 (8001) and PCG-8800 (8801): character redefinition and
  the 8253 three-voice tone generator, ported from X88000, which is in the
  public domain. Ported files carry the BSD header required for new files
  plus a note naming X88000 and its public-domain status; the version and
  the evidence of that status are recorded in `docs/agents/DECISIONS/`
  before porting.
- Floppy-disk timing against hardware: issue #33.
