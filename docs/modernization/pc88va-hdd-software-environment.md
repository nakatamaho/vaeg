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
# PC-88VA HDD Software Environment

This note summarizes the external PC-88VA HDD environment recipe centered
on PCEPAT, MSE, and PCPLUS. It is a source-reference note for future vaeg
SASI/HDD workflow work. It does not add, mirror, or redistribute any
third-party binaries.

## Source Notes

The main setup recipe is the TomoRetroPC article "PC-88VA HDD internal
environment setup memo":

- https://tomoretropc.blogspot.com/2019/04/pc-88va-hdd.html

The article assumes a SASI HDD boot environment already exists, then adds
the PC-Engine compatibility support and MS-DOS tool environment. It points
to PC88.gr.jp for most PC-88VA software and to "Madoushi no Atochi" for
MSE 3.52.

The old "Madoushi no Atochi" site is best read through the Internet
Archive:

- https://web.archive.org/web/20071017185024/http://hp.vector.co.jp/authors/VA015636/

The archived page exposes MSE 3.52a and a 3.52a-to-b diff archive. The
PC88.gr.jp link used by the article for PCEPAT is:

- http://www.pc88.gr.jp/softlib/index.php?action=list_file&anum=2&gnum=330

That PC88.gr.jp page is PCEPAT, not MSE. It lists `PCEPAT.COM` for V3
mode PC-Engine environments.

## Core Components

The article's baseline `CONFIG.SYS` is:

```dos
FILES = 20
BUFFERS = 20
DEVICE = A:\PCEPAT.SYS
DEVICE = A:\MSE352B.COM
DEVICE = A:\PCPLUS.SYS
```

The drive letter in the article is the booted VA environment. The exact
letter can differ if the HDD/FDD boot layout differs.

`PCEPAT.SYS` is produced by running `PCEPAT.COM` inside PC-Engine. The
article describes it as a PC-Engine bug-fix and function-extension layer.

`MSE352B.COM` is the MS-DOS application emulator for PC-Engine. The
archived MSE package provides 3.52a, and the 3.52b form is produced by
applying the archived diff with WSP. The included MSE documentation says
MSE can be loaded either as a command-line resident program or as a
`DEVICE=` line in `CONFIG.SYS`.

Useful MSE-side tools in the archived package include:

- `MSET.COM`: changes MSE interrupt/emulation settings after boot.
- `ALIAS.COM`: inspects, adds, or removes MSE aliases.
- `MSECUST.COM`: embeds aliases and mode rules into an MSE executable.
- `MSE350.DEF`: an example custom definition file.

`PCPLUS.SYS` is built from the PCPLUS archives referenced by the article
(`PCP108` plus its patch). The article treats it as another PC-Engine
extension layer.

## Support Tools

The recipe also needs DOS utilities, which run through MSE:

- LHA 2.13 for LZH extraction.
- BDIFF/BUPDATE for BDF diffs.
- WSP for WUP diffs, including the MSE 3.52a-to-b patch.
- K-Launcher as a two-pane file manager.
- PMD and VA-specific generated PMD players for music playback tests.

The article explicitly warns that archive extraction is safer on the
PC-88VA side than on Windows. The practical reason is timestamp
preservation: some diff tools can reject inputs whose file dates changed
during host-side extraction.

## Suggested HDD Layout

A practical emulator-side HDD image should keep the boot and tool layers
simple:

```text
A:\
  CONFIG.SYS
  AUTOEXEC.BAT
  PCEPAT.SYS
  MSE352B.COM
  PCPLUS.SYS
  PCENGINE.COM

A:\BIN\
  LHA.EXE
  BUPDATE.EXE
  WSP.COM
  MSET.COM
  ALIAS.COM
  MSECUST.COM
  KLL.COM

A:\TMP\
```

`AUTOEXEC.BAT` should at least set a DOS-like tool path and a temporary
directory:

```dos
PATH A:\BIN
SET TMP=A:\TMP
```

K-Launcher extension execution may also need `COMSPEC` pointed at
PC-Engine, matching the TomoRetroPC note:

```dos
SET COMSPEC=A:\PCENGINE.COM
```

## vaeg Implications

This recipe is useful for guest-side development and validation, but it is
not a host-file-transfer solution by itself. MSE lets DOS tools run inside
the PC-88VA environment; it does not make a host directory visible to the
guest.

For vaeg, the practical workflow target is:

1. Create or obtain a SASI-compatible HDD image.
2. Boot a PC-88VA DOS environment and install system files to the HDD.
3. Install PCEPAT, MSE 3.52b, PCPLUS, and the DOS utility set inside the
   HDD image.
4. Use the HDD image as the stable guest development environment.
5. Prefer guest-side archive extraction and diff application when file
   timestamps matter.

The longer-term ergonomic improvement is still a dedicated file-exchange
path, either through a restored host-drive bridge or through explicit HDD
image management tools. The PCEPAT/MSE setup complements that work but
does not replace it.

The SDL2 GUI parity list currently keeps HardDisk open/remove support as a
later item. When that lands, this document is the expected software
environment to test against.

## Distribution Caution

Do not vendor these third-party archives or generated binaries into the
vaeg repository. Keep them as user-supplied software, documented by URL
and checksum if a release workflow ever needs reproducible external
inputs.
