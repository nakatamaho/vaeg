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
EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.
-->

# M90: VA EMS board

## Status

M90 is implementation-complete at candidate
[`624e74a`](https://github.com/nakatamaho/vaeg/commit/624e74a6560effe324acb6d11c5422043547ba66)
on `main`. G90 remains a human gate.

The predecessor is the G89-integrated `main` commit
[`c65853c`](https://github.com/nakatamaho/vaeg/commit/c65853cfd2f5ff5318c1a11fec384961037bfdbb).

## Delivered behavior

- `Device / EMS Board...` appears immediately below I/O Bank Memory.
- EMS is disabled by `ExMemory=0`; enabled capacities are 1 through 13MB in
  1MB units, with a clean-configuration default of 1MB.
- Applying a changed capacity persists it and resets the guest. Cancel and an
  unchanged value do not reset the guest.
- The retained `08E1H`, `08E3H`, `08E5H`, `08E7H`, and `08E9H` interface is
  attached to both VA and compatibility I/O dispatch.
- The four 16KB windows remain at `C0000H`, `C4000H`, `C8000H`, and `CC000H`;
  target zero restores ordinary memory mapping.
- EMS and I/O Bank Memory remain independent configuration mechanisms.
- The supplemental-disk builder retains EMMVA15A and RDEMS152 archives,
  installs the three redistributable SYS files, and installs their manuals.
  A compatible EMM manager remains separately supplied by the user.

## Commit chain

1. [`27fd023`](https://github.com/nakatamaho/vaeg/commit/27fd0238f03fa92f223164fd8c31248be79de9e4)
   defines the M90 task and ROADMAP entry.
2. [`34945a6`](https://github.com/nakatamaho/vaeg/commit/34945a6fdef115acbb4600694848ae4ccfc521ca)
   connects the EMS board, adds GUI/configuration handling, and adds the
   ROM-less mapping/configuration selftest.
3. [`624e74a`](https://github.com/nakatamaho/vaeg/commit/624e74a6560effe324acb6d11c5422043547ba66)
   installs EMMVA/RDEMS in the supplemental-disk workflow and documents the
   required guest stack.

## Validation

| Check | Result |
|---|---|
| UTF-8, LF, and path-case repository checks | PASS; 0 violations/findings |
| `git diff --check` | PASS |
| macOS MacPorts configure/build | PASS; `vaeg` built successfully |
| Linux Debug configure/build | PASS |
| Linux CI GCC configure/build and CTest | PASS; 81/81 tests passed, 1 expected external-SST skip |
| ROM-less `--selftest` | PASS; BMS and EMS lifecycle checks included; all tests passed |
| ROM-less `--smoke` | PASS in documented reduced-scope mode |
| MinGW cross configure/build | PASS; PE32+ x86-64 GUI executable |
| Supplemental builder shell syntax | PASS |
| Supplemental media generation | PASS; 35 files, 951,863 payload bytes, 324,608 bytes free |
| Source-media protection | PASS; disposable source copy was unchanged before/after |
| Reproducibility | PASS; two independently generated outputs were byte-identical |
| Binary payload audit | PASS; no generated D88, ROM, font, or other binary payload is tracked |

The evaluated macOS executable has SHA-256
`b61beb0eab3ed3d6841d4fc837b73eaccde6b3765846a55f1e8140fe20953ece`.
The evaluated MinGW executable at `build/mingw-cross/sdl2/vaeg.exe` has
SHA-256
`28543f97e9a07717460ec468cbed00c13aa0fcd467b6e5a7672d24e8a4a81c51`.

The generated supplemental media and all raw validation logs remain outside
Git. The maintainer-supplied source media was used only through a disposable
copy; its private identity is not recorded here. The builder creates a
data-only supplemental disk and does not create or edit `CONFIG.SYS`.

## G90 human gate

From a clean checkout and clean configuration:

1. Complete the standard V3-mode, bundled-demo, OS-boot, and simple-operation
   gate.
2. Confirm EMS Board appears below I/O Bank Memory, defaults to 1MB, accepts
   1 through 13MB, persists, and resets only when applying a change.
3. Supply a compatible EMM manager and load `EMMVA01.SYS`, that manager, then
   `EMMVA02.SYS`; verify configured capacity and distinct data in multiple
   16KB pages.
4. Load `RDEMS.SYS` after the EMM stack and verify RAM-disk read/write.
5. Enable I/O Bank Memory concurrently and verify both mechanisms.
6. Disable EMS Board and verify normal V3/OS operation.

G90 passes only when the maintainer explicitly reports that this gate passed.
