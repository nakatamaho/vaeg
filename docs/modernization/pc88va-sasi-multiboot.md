<!--
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
-->

# Build a PC-88VA SASI multiboot disk

The multiboot builder creates one new HDI for SASI target 0. It is separate
from the [utility disk builders](pc88va-utility-media.md).

## English

You need Python 3.10+, an SDL2 VAEG executable with `--debug-script`
(including `capture-tvram <id>`, added in M171, so build VAEG from M171 or
later) and `--screen-tvram-dump` support, and ROMs
for the selected machine. Supply 2HD D88s for PC-Engine 1.05, PC-Engine 1.1,
and the [FreeDOS-88VA MS-DOS 4.0 VA distribution](https://github.com/FreeDOS-88VA/MS-DOS/blob/release/msdos-va/docs/D5-dual-boot.md).
The builder requires **MS-DOS 4.0 VA with D5.1 or later**, compatible with
D5.1's 23-character labels and version 3 menu format. Older D5 installers
cannot store the default labels. The tool supplies and downloads no disks or ROMs.

Create a private scratch directory outside the checkout. It and the new
output path must be on the same filesystem, with hard-link support. The
builder retains session logs, floppy copies and the working HDI there even
on failure. Do not share those files. The original inputs are not modified.

```sh
python3 -B tools/pc88va/build-sasi-multiboot-disk.py \
  --pce105 "$PCE105_D88" --pce11 "$PCE11_D88" --msdos "$MSDOS_D88" \
  --vaeg "$VAEG" --roms "$ROMS" --work-dir "$SCRATCH" \
  --output "$SCRATCH/multiboot.hdi" --model va2 --size 40 --default msdos
```

Use `--model va` for a PC-88VA build session, or `va2` (default) for a
PC-88VA2. `--size` accepts `5`, `10`, `20`, or `40` MB (default `40`).
`--default` accepts `msdos` (default), `pce105`, or `pce11`.

The host creates an empty NEC FAT12 volume. VAEG runs both PC-Engine
`SYS C:` installs, both D5 `SYS C:` captures, and D5 `SYS C: /P` in the
guest; the host does not generate D5 state, IPLs or parked system entries.
Before publication the builder checks the FAT and capture state, boots
all three entries, and runs `SYS C: /L` and `CHKDSK C:`. It also checks that
one uninterrupted, no-input run shows the menu and then boots the default.
It then installs any caller-supplied startup scripts without executing them
and checks the final FAT and capture state again. The six options
are `--msdos-config`, `--msdos-autoexec`, `--pce105-config`,
`--pce105-autoexec`, `--pce11-config`, and `--pce11-autoexec`. Provide files
in the intended guest encoding and DOS CRLF line endings; bytes are copied
as-is. If absent, MS-DOS gets a minimal `AUTOEXEC.BAT` to suppress date and
time prompts. Other startup files are optional and default to absent.
The host preserves the source D88 directory timestamps for `SYS.COM` and
`CHKDSK.COM`. Supplied startup files use their local modification time;
the generated `AUTOEXEC.BAT` uses the build's local start time. FAT times
have two-second resolution. The guest's `SYS` writes the system files.

Attach the HDI as the first SASI disk, enable fixed-disk boot and eject
both floppies. Press `1` for `PC-Engine 1.05 (VA)`, `2` for
`PC-Engine 1.1 (VA2/3)`, or `3` for `MS-DOS 4.00`. Enter or about ten
seconds of guest time boots the last choice.
The PC-Engine banner can reflect the built-in version on the other machine;
use the menu entry to identify the disk version. Back up the HDI before
maintenance. Do not reorder the root or rename the parked system files.

If the first VA2 launch with fresh backup memory and `Main_RAM_Auto=false`
does not reach the menu, restart VAEG or keep the default `Main_RAM_Auto=true`.

## 日本語

Python 3.10 以降、`--debug-script` (M171 で追加した `capture-tvram <id>` を含むため、M171 以降の VAEG が必要) と
`--screen-tvram-dump` に対応した SDL2 版 VAEG、選択した機種の ROM が
必要です。PC-Engine 1.05、PC-Engine 1.1、FreeDOS-88VA 版の
**D5.1 以降の MS-DOS 4.0 VA** の 2HD D88 を用意してください。
D5.1 の 23 文字ラベルとバージョン 3 のメニュー形式への互換性が必要です。
旧 D5 のインストーラーは既定のラベルを保存できません。
ディスクや ROM の同梱・自動取得はありません。

上記のコマンド例を使います。`--work-dir` にはチェックアウト外の作業用
ディレクトリを指定してください。ハードリンクが可能な同一ファイルシステム上に
作業場所と新規の出力先を置いてください。元の入力は変更されません。作業ディスクや
ログは失敗時も残るため、公開しないでください。

`--model va` は PC-88VA、`--model va2` は PC-88VA2 (既定) で構築します。
`--size` は `5`、`10`、`20`、`40` MB から選びます (既定は `40`)。
初期選択は `--default msdos` (既定)、`pce105`、`pce11` で指定します。
ホストが空の NEC FAT12 ボリュームを作り、ゲスト内で各 `SYS C:` と D5 の
`SYS C: /P` を実行します。公開前に FAT、各項目の起動、`SYS C: /L`、
`CHKDSK C:` を確認します。さらに、キー入力なしの同じ実行中にメニューが表示され、
続いて既定の項目が起動することを確認します。起動ファイルの格納後に、最終イメージの
FAT と登録状態を再確認します。

各 OS の起動ファイルは `--msdos-config` / `--msdos-autoexec`、
`--pce105-config` / `--pce105-autoexec`、`--pce11-config` / `--pce11-autoexec`
で指定できます。ゲスト用の文字コードと CRLF 改行を使ってください。内容は
変換・実行せずに格納します。MS-DOS の `AUTOEXEC.BAT` を省略した場合は日時を
尋ねないための最小ファイルを入れます。他の起動ファイルは既定では作りません。
ホストがコピーする `SYS.COM` と `CHKDSK.COM` は、元の D88 のディレクトリにある
日時を保持します。指定した起動ファイルには元のファイルの更新日時、生成する
`AUTOEXEC.BAT` にはビルド開始日時を、いずれもローカル時刻で記録します。
FAT の時刻の分解能は 2 秒です。システムファイルはゲストの `SYS` が書き込みます。

HDI を先頭の SASI に接続し、固定ディスク起動を有効にしてフロッピーを両方とも
取り出します。`1` は `PC-Engine 1.05 (VA)`、`2` は `PC-Engine 1.1 (VA2/3)`、
`3` は `MS-DOS 4.00` です。
Enter またはゲスト内の約 10 秒の経過で前回の選択を起動します。異なる機種では
PC-Engine の起動表示が内蔵版になることがあるため、メニューの項目名で区別します。
保守作業の前に HDI をバックアップし、ルートの並び替えや退避したシステム
ファイルの名前変更はしないでください。

新しいバックアップメモリを使い、`Main_RAM_Auto=false` にした状態で VA2 を
初回起動すると、メニューが表示されないことがあります。VAEG を再起動するか、
既定の `Main_RAM_Auto=true` のまま使用してください。
