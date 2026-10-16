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
POSSIBILITY OF SUCH DAMAGE.
-->

# M110 — Menu reorganization (and the plan for further machines)

Status: **complete; G110 passed on 2026-10-15; merged to `main` at [6f15dc5](https://github.com/nakatamaho/vaeg/commit/6f15dc5fcc9c16668366ce54ba9dd578e36f134f)**

## Request

The maintainer asked (2026-10-14) to take stock of the SDL2/ImGui menu:
group what is used, remove what is unused or permanently greyed out, rename
エミュレート to 模擬, and prepare 模擬 for further machines (PC-8801 and
PC-8001mkII/SR families from user-supplied ROMs, M111 onwards). The menu
below was agreed item by item in the same session.

## Decisions

- [POLICY] Top-level menus, in this order:
  模擬 / FDD / HDD / Tape / 編集 / 画面 / Key / 音 / 状態 from the left,
  and 情報 alone at the right end of the menu bar (maintainer,
  2026-10-15); when the window is too narrow for both, 情報 follows the
  others instead of overlapping them.
- [POLICY] Labels are Japanese throughout (machine and format names such
  as OPNA, ymfm, SASI stay as written), except the top-level device menus
  FDD, HDD, Tape and Key (maintainer, 2026-10-15).
- [POLICY] Permanently unimplemented, always-greyed items are removed:
  FDD3/FDD4 open and eject, Rotate, Screen option, Mechanical keys, Serial
  option, MIDI option, the mouse-status line, the fixed Base clock combo
  in the settings window, and the "no N-BASIC ROM" placeholder.
  Items unavailable only for the moment (Rewind or Eject with no tape,
  for example) stay greyed out, so that items do not move.
- [POLICY] Options that never apply to the current machine are hidden, not
  greyed: OPN (VA only), ymfm fidelity (ymfm only), HDD (VA only).
- [POLICY] The Pause button goes; 一時停止 becomes an F12 assignment, and a
  paused machine shows that it is paused on screen.
- [POLICY] The uPD8087 is on by default on every VA model (no software for
  the original VA uses it, which the maintainer accepts); it is switched off
  in the settings window. The "VA2/VA3 + 8087" model entry goes.
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
- [POLICY] KEY_MODE and the tenkey overlay become one choice,
  カーソル割り当て: 通常 (the VA cursor keys) / テンキー 8/4/6/2 (today's
  tenkey overlay, for games that move with the keypad) / ジョイスティック
  (JoyKey-1) / マウス (Mouse key, kept only if it works when checked); they
  all redirect the cursor keys, so only one can apply. JoyKey-2 is removed:
  it feeds the PC-98 FM board's second joystick connector, which vaeg never
  reads, so it only disabled the keys.
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
  the menu says so. Each slot is saved with a thumbnail PNG of the
  guest frame at that moment (unprocessed, about 320x200): `slotN.sav` and
  `slotN.png`. The 保存/読み込み lists show each slot's date and time (空き
  when unused) and its thumbnail on hover, and 一覧… opens a window of the
  ten thumbnails, clicked to save or load. 名前を付けて保存… and
  ファイルから読み込み… are added. Slots are kept per software in
  `states/<software>/`, the name taken from the FDD1 image, else the SASI-1
  image, else the tape, else `common`. Directory names vaeg chooses are
  English (`states`, `screenshots`, `recordings`, `common`, `extra`).
  The software directory is the image's file name without its extension,
  kept as written, Japanese included (UTF-8; characters the host file
  system forbids are replaced), followed by a short hash of its full path
  so that equal names from different directories stay apart, for example
  `ゲームA-1a2b3c` (maintainer, 2026-10-15). It is fixed
  when a state of that software is first saved, so a disk swap during play
  keeps the slots together; loading a slot made with another disk keeps
  the existing disk-change warning. `vaeg-cache/` (librashader shader
  files), today created in the current directory, is also created in the
  executable's directory, so that a shortcut or command line starting vaeg
  elsewhere no longer scatters it (maintainer, 2026-10-15). Screenshots
  are not split per software: they all go into `screenshots/` with
  today's date-and-time names (maintainer, 2026-10-15); headless
  `--screenshot` keeps its explicit paths. There is no switch between
  per-software and common slots: slots are always per software (`common`
  with no medium); another software's slots are reached with
  ファイルから読み込み… (maintainer, 2026-10-15).
- [POLICY] Sound volume (maintainer, 2026-10-15): today's "Master
  volume" only writes one value into the FM, SSG, ADPCM, rhythm, BEEP and
  seek volumes, and the tape volume is separate, so it is not a master.
  全体の音量 becomes a real master applied to the final mix (tape, seek and
  PCG included), a new configuration key defaulting to 100 % so that the
  current sound is unchanged. 個別の音量 keeps the balance: 音源 (FM, SSG,
  ADPCM and rhythm together, as their ratio is fixed by the chip and the
  board on hardware; no separate FM slider), BEEP, テープ, シーク音 and
  later PCG. "Sound on/off" (SDL output paused,
  the guest unaffected, as `--mute`) becomes ミュート, checked when silent,
  placed right below 全体の音量; no F12 assignment for it.
- [POLICY] History (maintainer, 2026-10-15; to be dropped again if it
  proves a nuisance in use):
  - File history per drive: FDD1 履歴 and FDD2 履歴 (10 each), SASI-1,
    SASI-2 and each SCSI ID (5 each), Tape (10). An image is recorded when
    it is inserted (menu or drag and drop), whether or not the guest is
    reset; inserting it again moves it to the top. Missing files are shown
    greyed; each list ends with 履歴を消す.
  - 最近の起動 in 模擬 (10): recorded at a reset (リセット, a 起動 choice,
    power-on), the media in FDD1, FDD2, the hard disks and the tape with
    the machine and boot mode as one set; nothing is recorded with no
    medium; the same set moves to the top. Choosing an entry inserts that
    set, selects the machine and mode, and resets. Disks inserted after the
    boot (a second disk during play) are not added to the set.
  - File-selection windows list the 5 most recent directories.
  - Media given on the command line or by headless scripts are not
    recorded. Histories are stored in the configuration file.
- Dialog windows fit the vaeg window (maintainer, 2026-10-15: the FDD
  file browser has to be enlarged by hand on some hosts, and at the default
  1x window it does not quite fit). Cause: the file browsers and other
  dialogs open at fixed pixel sizes (620x420 and similar, with
  `ImGuiCond_FirstUseEver`, no saved layout) at ImGui's default position,
  whatever the window size and the GUI font scale, so at 1x (640x422) they
  run past the window's edges. Every dialog is sized in font units,
  clamped to the window's work area (at most 95 % of it), centred, and
  given each time it appears; the file list takes the remaining height, so
  the path field and the buttons stay visible at 1x. A dialog the user
  resizes keeps that size until vaeg exits.
- [POLICY] Compatibility (maintainer, 2026-10-15): configuration files
  and state files need not stay compatible. Configuration keys may be
  renamed, merged or dropped without migrating old values (defaults apply),
  and a new vaeg need not write keys an older one reads. State files of
  earlier builds need not load (a clear message instead of a crash), and
  the existing `np2sdl.S00`-`S09` slots are not moved. Command-line options
  and the headless tests keep working.

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
 ├─ 最近の起動 ▶  (10 sets: media, machine and boot mode; choosing one resets)
 ├─ 実行速度  [slider] 10-400 %, 最大 = No Wait
 ├─ 実行速度を 100% に戻す
 ├─ CPU クロック ▶ (current MHz)  ● 機種の既定 / PC-8801 標準速度相当 /
 │                 PC-8801 高速相当 / ── / 2倍 / 4倍 / 8倍 / 任意…
 ├─ 機種の構成 ▶
 │   ├─ メインメモリ容量 ▶
 │   ├─ バックアップメモリへ自動反映
 │   ├─ HDD から起動
 │   ├─ I/O バンクメモリ…
 │   ├─ EMS ボード…
 │   └─ PCG (PCG-8100 / PCG-8800)  (M115+, 8001/8801 only)
 ├─ 詳細設定…  (8087 on/off and clock, SGP speed, HOSTFAT)
 └─ 終了

FDD           FDD1 開く…, FDD1 履歴 ▶, FDD1 取り出し (with its contents),
              FDD2 開く…, FDD2 履歴 ▶, FDD2 取り出し (with its contents),
              新しい FDD イメージ ▶ 2HD / 2D (320 KB) / 2DD (640 KB) / ── /
              2D (360 KB) / 2DD (720 KB)
HDD           SASI-1, SASI-2 開く…/履歴 ▶/外す (as today, with their contents),
              SCSI ▶ (heading shows how many are in use) ID 0-6 ▶ 開く…/履歴 ▶/外す
              (each with its contents), 新しい SASI/SCSI イメージ… (VA only)
Tape          読み込み・録音…, 履歴 ▶, 巻き戻し, 取り出し, 録音を止める, Fast load (off by default)
編集          画面の文字をコピー, 貼り付け, 貼り付けの取り消し
画面          スクリーンショットを保存 (and 加工前), 録画 (future),
              モニタ ▶ 24 kHz / 15 kHz (リセット), デジタル RGB (8色),
                       15 kHz 走査線: 隙間あり / 埋める, V1/V2 表示: VA 標準 / PC-8801 風,
              描画方式 ▶ (and CRT 設定…), エフェクト ▶, 拡大方法 ▶,
              ウィンドウサイズ ▶, 縦横比の補正, 全画面表示, 前面パネル,
              フレームスキップ ▶, フォント ▶, 画面の回転 ▶ (future)
Key           カーソル割り当て ▶ 通常 / テンキー 8/4/6/2 / ジョイスティック / マウス,
              ホストの配列 ▶, かな入力 ▶,
              F12 の割り当て ▶ (adds 一時停止; 録画 later), キーの割り当て…,
              マウスを捕捉, コントローラポート ▶, 連射ボタン
音            全体の音量 [slider], ミュート,
              個別の音量 ▶ 音源 (FM/SSG/ADPCM/リズム) / BEEP / テープ / シーク音 /
                          PCG 音源 (M115+, PCG on only),
              FM 音源ボード ▶ (OPN only on the VA) / OPNA,
              FM の方式 ▶ ymfm (default, listed first) / NP2, ymfm の精度 ▶ (ymfm only),
              サンプリング周波数 ▶, バッファ ▶, シーク音 (on/off)
状態          保存 ▶, 読み込み ▶ (date, time and thumbnail per slot), 一覧…,
              名前を付けて保存…, ファイルから読み込み…
情報          display toggles (FDD, FPS, CPU/SGP clock, frame, video and FB info),
              layer toggles (text, sprite, graphics 0/1), About
```

## Scope of M110

Implement the agreed menu for the VA models: the reorganization, the
removals, Pause as an F12 assignment with an on-screen indication, the
speed controls, 8087 on by default, the cursor-key setting, the FDD image
order, Japanese labels, the directories and state slots above. 起動
offers the VA rows with 自動, V2 H/S, V1 H/S and N (today's choices); V3
and the other machines come later. Update the READMEs that name menu paths
(historical task files and the bug-fix ledger keep their wording).

Checks: the 8087-on default re-runs the all-demos programs and an OS boot
on the VA and the VA2; states save and load under the new layout and an
old state file is refused cleanly; `--model` and the headless scripts keep
working; every menu item is
exercised once.

## Later stages (planned, not part of M110)

- M111: 起動 as machine ▶ boot mode for the VA, including V3, V2 and V1
  forced at boot without writing the backup-memory file (method from the
  ROM's behaviour), and N moved into it. Configuration keys `Machine=` and
  `BootMode=` replace `model`, `v1v2_nmode`, `v1v2_standard` (no migration
  needed).
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
- X88000 (public domain) supports HAL Laboratory's PCG-8800 and has a
  PC-8001mkII/SR emulation mode (maintainer, 2026-10-15), so it is the
  reference for both M114 and M115. Its source is not on the build machine
  yet; it is to be obtained, and its version and the evidence of its
  public-domain status recorded, before anything is ported.
- M115: PCG-8100 (8001) and PCG-8800 (8801): character redefinition and
  the 8253 three-voice tone generator, ported from X88000, which is in the
  public domain. Ported files carry the BSD header required for new files
  plus a note naming X88000 and its public-domain status; the version and
  the evidence of that status are recorded in `docs/agents/DECISIONS/`
  before porting.
- Floppy-disk timing against hardware: issue #33.

## Progress

- Menu bar reorganized as agreed (起動 with the VA rows, the speed controls,
  機種の構成, モニタ under 画面, Key, 音, HDD with SCSI folded, 情報 at the
  right, Pause as an F12 assignment with an on-screen 再開, greyed
  placeholders removed, cursor-key choice saved as `cursor_keys`); every top
  menu and the main submenus checked in a headless window at 1x.
  [8cecfd66](https://github.com/nakatamaho/vaeg/commit/8cecfd6663a9a6937348e76f880a1328309c02b9).
- Dialogs fit the window: sizes in font units clamped to 95 % of the work
  area, centred; file lists leave the path field and buttons visible at 1x;
  the settings window scrolls above fixed buttons.
  [9e3ffc78](https://github.com/nakatamaho/vaeg/commit/9e3ffc78dde6f07749aa18755a14779ff0a0f5cf).
- 8087 on by default. All 17 all-demos programs under PC-Engine 1.1 and
  MS-DOS 4.0, on the VA and the VA2 (68 runs), drew and returned without
  CPU errors; ZUNDAORB the same with the 8087 on and off (it must be started
  from its own directory). [ac79b8f3](https://github.com/nakatamaho/vaeg/commit/ac79b8f39ed02a4f8da509d57509b9b172a321d9).
- AGENTS.md: cleanup after a passed human gate
  ([f57f5a7e](https://github.com/nakatamaho/vaeg/commit/f57f5a7e93d3c6964fed768845beb9e21b274ed4)) and the media
  build regression before every gate
  ([6562f6f0](https://github.com/nakatamaho/vaeg/commit/6562f6f06d8ca75e3baecb3a912f8ce187b46579)).
- Utility FDD build fixed (V480PAT kept on the HDD and the SGP disk only;
  ledger entry). [dd7a661e](https://github.com/nakatamaho/vaeg/commit/dd7a661e23164385b3995091db35b827d41bd3cc).
- Media build regression at `6562f6f0`, clean clone, shared cache:
  `build-utility-disk.sh` (PC-Engine 1.1) exit 0, FDD SHA-256 `60a08e32…`,
  4096 bytes free, boots in vaeg; `build-sasi-utility-disks.sh` exit 0, VA
  `03b71a45…`, VA2 `a3054775…`, each boots alone on its model with
  `\BIN\V480PAT.COM`; the five `build-d88.sh` distributions,
  `build-from-psd.sh`, the SGP boot disk, all-demos and all-demos-msdos4
  exit 0 with payloads identical to the committed disks (zundamon differs
  only in 14 directory-date bytes; the SGP boot `.xz` is byte-identical).
- Files next to the executable: `screenshots/` and `vaeg-cache/` through
  `file_getappdir` (user directory when not writable; selftest
  `application directory`). [7b733b3a](https://github.com/nakatamaho/vaeg/commit/7b733b3a22970da676c2897b06ff697a793f289e).
- State slots per software with thumbnails, 一覧…, 名前を付けて保存… and
  ファイルから読み込み…; PNG read/write through UTF-8 paths (selftest
  `state thumbnail`, two mutations caught). Deviation from the plan: the
  software is the medium at the last reset (the booted one), not at the
  first save; a disk swap during play still keeps the slots together.
  Checked in vaeg: save into `states/ゲームA-a008ab/`, reset, load returns
  to the saved screen. [0c1f0f12](https://github.com/nakatamaho/vaeg/commit/0c1f0f12da9f46debc5a827bfad7f365a81ce551).
- 全体の音量 on the final mix (`master_volume`), 個別の音量, ミュート below
  it (selftest `master volume`). [05a08f72](https://github.com/nakatamaho/vaeg/commit/05a08f72a84417335c88c1885f3953a1948bd653).
- CPU クロック presets, applied without a reset; the multiplier left the
  settings window. [fd368432](https://github.com/nakatamaho/vaeg/commit/fd3684325aa0085c8a71eff4ff8d00f89f65a5dc).
- Histories (FDD, SASI, SCSI, tape), 最近の起動 and 最近のフォルダ.
  Deviation: kept in `vaeg-history.txt` next to `vaeg.cfg` rather than in
  the configuration file (lists of paths do not fit its fixed-size keys).
  Boots are recorded at GUI resets only. [9e1123f8](https://github.com/nakatamaho/vaeg/commit/9e1123f826d95d19d79de2403d8aa009d9a4ea8c).
- カーソル割り当て > マウス kept: the cursor keys move the mouse the VA
  reads (selftest `cursor keys as mouse`, fails with the key path removed).
  [3b9f10b2](https://github.com/nakatamaho/vaeg/commit/3b9f10b266f031785e6e864269b98c7c798bbdba).
- Documentation names the new menu paths. [0afb3314](https://github.com/nakatamaho/vaeg/commit/0afb33148033d232e6acf3ed54e1ef999a2f0d7d).
- Not done in M110 (M111): 起動 offers today's V2/V1 H/S and N choices;
  the separate 自動 entry and forced V3 come with the boot-mode work.
- Since `6562f6f0` (media regression) and `ac79b8f3` (all-demos runs) no
  file under `demos/`, `tools/`, `cpu/`, `io/`, `machine/`, `memoryva/`,
  `vram/`, `sound/`, `fdd/` or `bios/` changed, so those results stand for
  the gate commit.

### G110

- Passed on 2026-10-15 (maintainer), with the Windows build
  `ce09e6be...1d76` (SHA-256, static MinGW) from
  [9097d308](https://github.com/nakatamaho/vaeg/commit/9097d308e47f77d49b5139724999fc9b852cbc99).
