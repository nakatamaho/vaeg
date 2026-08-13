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
[`ef699ea`](https://github.com/nakatamaho/vaeg/commit/ef699ead3a43b57e5b51f616e277beb9e536851f)
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
  installs their three redistributable SYS files, builds the PC-88VA
  `SQEMM98.SYS` manager with pinned Open Watcom, and installs the manuals and
  licenses.
- The generated root `CONFIG.SYS` loads `EMMVA01.SYS`, `SQEMM98.SYS`,
  `EMMVA02.SYS`, and `RDEMS.SYS -P40 -A` in that order. The supplemental
  media remains data-only, so this is an HDD-install template rather than a
  claim that the D88 is bootable.
- SQEMM98 initialization and diagnostic messages use PC-Engine Text BIOS
  `INT 83H/AH=02H`. The generated binary contains no IBM video BIOS `INT 10H`
  and no DOS `INT 21H/AH=09H` string-output path.

## Commit chain

1. [`27fd023`](https://github.com/nakatamaho/vaeg/commit/27fd0238f03fa92f223164fd8c31248be79de9e4)
   defines the M90 task and ROADMAP entry.
2. [`34945a6`](https://github.com/nakatamaho/vaeg/commit/34945a6fdef115acbb4600694848ae4ccfc521ca)
   connects the EMS board, adds GUI/configuration handling, and adds the
   ROM-less mapping/configuration selftest.
3. [`624e74a`](https://github.com/nakatamaho/vaeg/commit/624e74a6560effe324acb6d11c5422043547ba66)
   installs EMMVA/RDEMS in the supplemental-disk workflow and documents the
   required guest stack.
4. [`ef699ea`](https://github.com/nakatamaho/vaeg/commit/ef699ead3a43b57e5b51f616e277beb9e536851f)
   adds the reproducible PC-88VA SQEMM98 build and validator, routes messages
   through PC-Engine BIOS, and adds the complete stack and `CONFIG.SYS` to the
   supplemental media workflow.

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
| SQEMM98 Open Watcom assembly/link | PASS; 13,258-byte `EMMXXXX0` character device |
| SQEMM98 reproducibility | PASS; two independent builds have SHA-256 `8d76f4ca63444343cf75f6106037dbbc475abfde3c5f839ec56ccdfcf5383f46` |
| SQEMM98 output-path validation | PASS; one `INT 83H`, no `INT 10H`, no DOS `AH=09H` output |
| SQEMM98 validator negative check | PASS; injected `INT 10H` rejected as `SQEMM98_CHECK_IBM_VIDEO` |
| Supplemental builder shell syntax | PASS |
| Supplemental media generation | PASS; 39 files, 967,925 payload bytes, 306,176 bytes free |
| Supplemental `CONFIG.SYS` | PASS; four expected CRLF lines extracted from the generated D88 |
| Supplemental SQEMM98 identity | PASS; D88 copy matches the independently generated driver byte-for-byte |
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
copy; its private identity is not recorded here. The canonical local output
has SHA-256
`0625f935e9ccaeadf88194cea1cbcf000e204284aa327a9b2878bafcb9ea7c12`.

A bounded boot attempt used a disposable vanilla PC-Engine 1.1 system disk
with the EMS stack and a small `INT 67H` allocation/map probe. It reached the
PC-Engine prompt, but the probe reported that no EMM was installed. A control
disk with the already validated HOSTFAT driver likewise reached the prompt
without loading its root `CONFIG.SYS`. This demonstrates that the minimal
vanilla-disk setup is not a valid runtime driver-loading oracle; it is not an
SQEMM98 pass or failure. Guest-visible SQEMM98 messages and EMS/RDEMS operation
therefore remain explicitly assigned to G90 on the maintainer's HDD software
environment.

## G90 human gate

From a clean checkout and clean configuration:

1. Complete the standard V3-mode, bundled-demo, OS-boot, and simple-operation
   gate.
2. Confirm EMS Board appears below I/O Bank Memory, defaults to 1MB, accepts
   1 through 13MB, persists, and resets only when applying a change.
3. Copy the generated stack to the boot drive, merge the supplemental
   `CONFIG.SYS`, and verify SQEMM98's PC-Engine BIOS messages, configured
   capacity, and distinct data in multiple 16KB pages.
4. Verify `RDEMS.SYS` loads after SQEMM98 and supports RAM-disk read/write.
5. Enable I/O Bank Memory concurrently and verify both mechanisms.
6. Disable EMS Board and verify normal V3/OS operation.

G90 passes only when the maintainer explicitly reports that this gate passed.
