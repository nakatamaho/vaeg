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

88VA Eternal Grafx Rel.260708 distribution README
================================================

This archive contains the portable SDL2/Dear ImGui build of 88VA
Eternal Grafx. Source archives are attached automatically by GitHub
Releases for the release tag; this archive contains one platform binary
package.

Disk images and guest WAV payloads are not included. ROMs are not
included and must be extracted from hardware you own.

Files supplied by the release package
-------------------------------------

Keep this relative layout after unpacking:

- vaeg or vaeg.exe
- assets/OFL.txt
- assets/NOTICE.md
- README-dist.txt

The Dear ImGui Japanese font and historical VAEG startup graphic are
embedded in the executable. OFL.txt and NOTICE.md document the font
license and provenance and must stay with the package.

Additional platform runtime files:

- Windows: SDL2 and the MinGW GCC, libstdc++, and winpthread runtimes are
  statically linked into vaeg.exe.
- Linux: SDL2 is a system dependency. Install your distribution's SDL2
  runtime package, for example libsdl2-2.0-0 on Debian/Ubuntu systems.
  Rel.260708 is built and tested on the GitHub Actions ubuntu-latest
  runner using the linux-ci-gcc preset and libsdl2-dev.
- macOS: the release preset statically links the pinned FetchContent SDL2;
  macOS system frameworks remain operating-system dependencies.

Files you must supply
---------------------

Create a romimage/ directory in the unpacked directory, or point the
BIOS path in vaeg.cfg at your ROM directory. A usable PC-88VA setup needs
the VA ROM set, including FONT.ROM, VAFONT.ROM, VADIC.ROM,
VAROM00.ROM, VAROM08.ROM, VAROM1.ROM, and VASUBSYS.ROM.

Supply your own disk images. You can mount floppy images from the GUI or
pass up to two image paths on the command line:

    ./vaeg disk1.d88 disk2.d88

On Windows:

    vaeg.exe disk1.d88 disk2.d88

VA configuration prerequisites
------------------------------

For VA booting, vaeg.cfg must select the VA machine, matching sound
hardware, and the VA clock domain:

    pc_model=88VA1
    SNDboard=100
    clk_base=3993600
    clk_mult=2

SNDboard=100 is the VA built-in YM2203/OPN. Use SNDboard=200 for a VA
with Sound Board II, or with pc_model=88VA2 for YM2608/OPNA. The GUI
selects these defaults when the boot model changes. If you reuse an
existing config, invalid combinations can stop at a V2 screen, leave
sound hardware unbound, or run in the wrong clock domain.

Config and state file locations
-------------------------------

The portable frontend normally stores writable files outside the unpacked
distribution directory:

- Linux: $XDG_CONFIG_HOME/vaeg or $HOME/.config/vaeg
- Windows: %APPDATA%\vaeg
- macOS: ~/Library/Application Support/vaeg

vaeg.cfg, vabkupmem.dat, and fixed GUI save-state slots normally live
there. An executable-local vaeg.cfg or existing vabkupmem.dat takes
priority over its user-state counterpart and is saved in place. Save
states and keyboard sidecars remain in the user directory.
Save-state files are tied to the architecture and build family that
created them; do not move them between old Win32 builds, portable
builds, different CPU architectures, or different operating systems.

macOS unsigned binary note
--------------------------

The macOS command-line binary is not code-signed. If Finder or Gatekeeper
adds quarantine metadata after download, remove it from the unpacked
release directory before running:

    xattr -dr com.apple.quarantine vaeg-rel260708-macos-arm64

You can also remove it from just the binary:

    xattr -d com.apple.quarantine ./vaeg
