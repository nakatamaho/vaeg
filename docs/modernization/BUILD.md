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
# Modernized Build Recipes

These are the G11 maintainer recipes for the portable SDL2/ImGui build.
The legacy Visual Studio projects remain reference artifacts and are not
covered here.

## Windows Native: MSYS2 MINGW64

This is the supported Windows runtime configuration. Build and run the
native Windows binary from an MSYS2 MINGW64 environment, then distribute
`vaeg.exe`. The executable statically links SDL2 and the MinGW gcc,
libstdc++, and winpthread runtimes.

Open the MSYS2 MINGW64 shell, then install the build prerequisites:

```sh
pacman -S --needed mingw-w64-x86_64-cmake mingw-w64-x86_64-ninja \
  mingw-w64-x86_64-gcc mingw-w64-x86_64-pkgconf
```

Configure and build:

```sh
cmake --preset mingw-release
cmake --build --preset mingw-release
```

The output is `build/mingw-release/sdl2/vaeg.exe`. The preset builds a
GUI-subsystem executable. For a console while debugging, configure a
separate tree with `-DVAEG_WINDOWS_CONSOLE=ON`.

The historical VAEG icon is embedded both as a Windows executable resource
and as the cross-platform SDL window icon. No adjacent icon file is required.

## macOS Release

The release preset fetches pinned SDL2 and links it statically:

```sh
cmake --preset macos-release
cmake --build --preset macos-release
```

The output is a plain `build/macos-release/sdl2/vaeg` binary. macOS
system frameworks remain dynamic operating-system dependencies. The
historical VAEG icon data is embedded and supplied to SDL for the running
application. The plain executable is not an application bundle, so Finder
does not assign it a bundle document icon; that would require a separately
audited `.icns` asset and an app-bundle packaging milestone.

## macOS Development: MacPorts

The M11 baseline is MacPorts under `/opt/local`, not Homebrew. Install:

```sh
sudo port install cmake ninja libsdl2 pkgconfig
```

Configure and build:

```sh
cmake --preset macos-macports
cmake --build --preset macos-macports
```

The MacPorts preset sets `CMAKE_PREFIX_PATH=/opt/local` and
`PKG_CONFIG_PATH=/opt/local/lib/pkgconfig`. A plain binary is accepted for
G11; no app bundle is required.

## Linux To Windows Cross Check

The agent-side MinGW cross check uses the pinned SDL2 FetchContent path
from ADR-0006:

```sh
cmake --preset mingw-cross
cmake --build --preset mingw-cross
```

The staged output is `build/mingw-cross/sdl2/vaeg.exe`. SDL2 and the
MinGW runtimes are static, so the executable has only Windows system DLL
imports. This preset remains a link-check tier; Windows release artifacts
should be produced from the supported MSYS2 MINGW64 native configuration.

Linux executables also contain the same icon data and set it through SDL for
window-manager and task-switcher use. The project does not currently install
a desktop entry or an icon-theme payload, so launchers do not receive a
system-wide application icon from this unpacked-binary distribution.

## GitHub Actions CI

The M12 workflow is `.github/workflows/build.yml`. It covers:

- Ubuntu with system SDL2 from `apt`, in separate gcc and clang jobs.
- Ubuntu ASan/UBSan smoke. The known UBSan backlog messages documented in
  `../agents/reports/ubsan_backlog.md` may appear, but sanitizer process
  failures still fail the job.
- Windows on `windows-latest` through MSYS2/MINGW64 with pinned static
  FetchContent SDL2.
- macOS on `macos-latest` with `VAEG_FETCH_SDL2=ON`. The maintainer
  baseline remains MacPorts, but MacPorts is impractical on hosted
  runners; this CI job deliberately exercises the pinned FetchContent path
  from ADR-0006.

CI has no ROMs or disk images by design. `--smoke` therefore runs in a
reduced-scope ROM-less mode on hosted runners: SDL initialization, core
loop, one GUI frame, and clean exit. If a complete VA ROM set is present,
the same smoke command enables the uniform-screen detector and logs that
mode. Behavioral ROM and disk validation remains a human gate.

`VAEG_ENABLE_TESTS=ON` enables the ROM-less ctest suite. Current tests are
the codecnv wave-dash round-trip, ini read/write round-trip, and statsave
save/check/load on stable sections. M9 did not produce a memoryva fixture,
so the memoryva fixture test is intentionally skipped.

CI uploads build artifacts for all three operating systems. The Windows
artifact is the standalone `vaeg.exe`; its import audit rejects SDL2 and
MinGW runtime DLL dependencies.

ROMs are deliberately absent from CI and release artifacts. Users place the
VA unsuffixed ROM set or the MAME-compatible VA2/VA3 `*_va2.rom` set beside
the executable, together with the extra `vasubsys.rom`, using ROMs extracted
from hardware they own. Runtime size/CRC32/SHA-1 checks warn on differences
from MAME. No ROM placeholder is packaged.

## VA Configuration Prerequisites

Before running the VA checklist, ensure the portable `vaeg.cfg` selects
the VA machine and matching sound hardware:

```ini
pc_model=88VA1
SNDboard=100
clk_base=3993600
clk_mult=2
```

`SNDboard=100` is the VA built-in YM2203/OPN. A VA with Sound Board II
uses `SNDboard=200`; `pc_model=88VA2` also uses `SNDboard=200` for its
YM2608/OPNA. Missing or stale values can produce a V2 halt, an FM-timer
wait with no sound hardware bound, or the wrong clock domain. See
`../../sdl2/README.md` for the full state directory and warning policy.

## G11 Verification Notes

G11 passed on the maintainer-verified native Windows and macOS builds.
The macOS build used the SDL renderer backed by Metal. Retina output was
not blurry at integer display scales, and writable state files were
created under `~/Library/Application Support/vaeg` as required by
ADR-0005.

## Known Issues

US physical host keyboards do not yet have a dedicated mapping mode for
the JIS PC-88VA guest layout. On all host platforms with a US keyboard,
the known symptoms are swapped `@` / `"` positions, no direct `:` key, and
unreachable `-`, `^`, and yen-key positions. The numeric keypad path is
not affected. A keyboard-layout mapping mode is tracked as a later GUI
parity item.

State-save (`statsave`) files are not portable across architectures or
builds. This was established during the M11 LLP64 audit: runtime pointer
and handle-size fixes are safe for process memory, but serialized emulator
state should be treated as tied to the exact architecture/build family
that wrote it.

VA1 mode has an inherited V3 BASIC execution defect: commands including
`BEEP`, `LIST`, and `FILES` can leave the guest running in a repeated
text-display path and make it appear frozen. VA2/VA3 mode is not known to be
affected. The problem also reproduces in the original VAEG and predates the
SDL2 display-effects work. Disabling host sound, suppressing BEEP PCM
registration, and suppressing the BEEP event did not prevent it, so this is
not currently classified as an audio-backend defect.
