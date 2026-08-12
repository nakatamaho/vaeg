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
[`b437811`](https://github.com/nakatamaho/vaeg/commit/b4378111319fe0a82e31abebc4b4749df2083dc0)
on `topic/m90-va-ems-board`. G90 remains a human gate.

The predecessor is the G89-integrated `main` commit
[`5b4a22b`](https://github.com/nakatamaho/vaeg/commit/5b4a22ba4e8a4fc7ef44f3d2dfcfe4c1001cde97).

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

1. [`9daab85`](https://github.com/nakatamaho/vaeg/commit/9daab85c6ee534958575f9772c50e8d5863e4aed)
   defines the M90 task and ROADMAP entry.
2. [`bb3a2ab`](https://github.com/nakatamaho/vaeg/commit/bb3a2abb3ca794c7bb3718159ddcc9ee9cdbfc9a)
   connects the EMS board, adds GUI/configuration handling, and adds the
   ROM-less mapping/configuration selftest.
3. [`b437811`](https://github.com/nakatamaho/vaeg/commit/b4378111319fe0a82e31abebc4b4749df2083dc0)
   installs EMMVA/RDEMS in the supplemental-disk workflow and documents the
   required guest stack.

## Validation

| Check | Result |
|---|---|
| UTF-8, LF, and path-case repository checks | PASS; 0 violations/findings |
| `git diff --check` | PASS |
| macOS MacPorts configure/build | PASS |
| ROM-less `--selftest` | PASS; BMS and EMS lifecycle checks included; all tests passed |
| ROM-less `--smoke` | PASS in documented reduced-scope mode |
| MinGW cross configure/build | PASS; PE32+ x86-64 GUI executable |
| Supplemental builder shell syntax | PASS |
| Supplemental media generation | PASS; 35 files, 951,863 payload bytes, 324,608 bytes free |
| Source-media protection | PASS; disposable source copy was unchanged before/after |
| Reproducibility | PASS; three independently generated outputs were byte-identical |
| Binary payload audit | PASS; no generated D88, ROM, font, or other binary payload is tracked |

The evaluated macOS executable has SHA-256
`765ff93e6204a79084cb9e6e19923e12036d3274a69ae5ae32d519bf3dc8f13e`.
The evaluated MinGW executable at `build/mingw-cross/sdl2/vaeg.exe` has
SHA-256
`71d07fe703b54e34d96acd2e8624f6eb6aefbb0eb503943226e0cb53ed31b78f`.

The generated supplemental media and all raw validation logs remain outside
Git. The maintainer-supplied source media was used only through a disposable
copy; its private identity is not recorded here.

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
