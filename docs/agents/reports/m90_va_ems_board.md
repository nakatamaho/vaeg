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
[`34fb837`](https://github.com/nakatamaho/vaeg/commit/34fb8376f988b9c121ade3774f0b038b2d110fb4)
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
  target zero restores ordinary memory mapping. Native VA memory accesses now
  reach the selected EMS pages as well as compatibility-mode accesses.
- EMS and I/O Bank Memory remain independent configuration mechanisms.
- The supplemental-disk builder retains EMMVA15A and RDEMS152 archives,
  installs their three redistributable SYS files, builds the PC-88VA
  `SQEMM98.SYS` manager with pinned Open Watcom, and installs the manuals and
  licenses.
- The generated root `CONFIG.SYS` loads `EMMVA01.SYS`, `SQEMM98.SYS`,
  `EMMVA02.SYS`, and `RDEMS.SYS -P40 -A` in that order. The supplemental
  media retains the PC-Engine 1.1 IPL and four fixed system files and boots
  that complete stack directly.
- SQEMM98 initialization and diagnostic messages use PC-Engine Text BIOS
  `INT 83H/AH=02H`. The generated binary contains no IBM video BIOS `INT 10H`
  and no DOS `INT 21H/AH=09H` string-output path. It preserves the PC-Engine
  loader's caller state and uses validated built-in defaults rather than
  interpreting the incompatible request-header command-tail field.

## Commit chain

1. [`27fd023`](https://github.com/nakatamaho/vaeg/commit/27fd0238f03fa92f223164fd8c31248be79de9e4)
   defines the M90 task and ROADMAP entry.
2. [`34945a6`](https://github.com/nakatamaho/vaeg/commit/34945a6fdef115acbb4600694848ae4ccfc521ca)
   connects the EMS board, adds GUI/configuration handling, and adds the
   ROM-less mapping/configuration selftest.
3. [`624e74a`](https://github.com/nakatamaho/vaeg/commit/624e74a6560effe324acb6d11c5422043547ba66)
   installs EMMVA/RDEMS in the supplemental-disk workflow and documents the
   required guest stack.
4. [`235e5cb`](https://github.com/nakatamaho/vaeg/commit/235e5cbaf4f49ddd4e6c64f9c44425f7306bae07)
   records the initial EMS validation and G90 handoff.
5. [`ef699ea`](https://github.com/nakatamaho/vaeg/commit/ef699ead3a43b57e5b51f616e277beb9e536851f)
   adds the reproducible PC-88VA SQEMM98 build and validator, routes messages
   through PC-Engine BIOS, and adds the complete stack and `CONFIG.SYS` to the
   supplemental media workflow.
6. [`f5a7fab`](https://github.com/nakatamaho/vaeg/commit/f5a7fab35f77fa57313dbe28ef4ee7465236ae8b)
   records the independently reproduced SQEMM98 and supplemental-media
   identities.
7. [`a4b6170`](https://github.com/nakatamaho/vaeg/commit/a4b6170824dbfb8d602ca1ade19c60778a6eff1c)
   retains the boot-only PC-Engine layout, fixes native VA EMS page-frame
   access, and makes SQEMM98 safe for the PC-Engine device-loader contract.
8. [`34fb837`](https://github.com/nakatamaho/vaeg/commit/34fb8376f988b9c121ade3774f0b038b2d110fb4)
   gives the EMS selftest fresh I/O tables so it does not reuse tables that
   the preceding state-save test destroyed.

## Validation

| Check | Result |
|---|---|
| UTF-8, LF, and path-case repository checks | PASS; 0 violations/findings |
| `git diff --check` | PASS |
| macOS MacPorts configure/build | PASS; `vaeg` built successfully |
| Linux Debug configure/build | PASS |
| Linux CI GCC configure/build and CTest | PASS; 81/81 tests passed, 1 expected external-SST skip |
| Hosted CI | PASS; [run 31675702844](https://github.com/nakatamaho/vaeg/actions/runs/31675702844), all 9 jobs succeeded |
| ROM-less `--selftest` | PASS; BMS and EMS lifecycle checks included; all tests passed |
| ROM-less `--smoke` | PASS in documented reduced-scope mode |
| MinGW cross configure/build | PASS; PE32+ x86-64 GUI executable |
| SQEMM98 Open Watcom assembly/link | PASS; 13,253-byte `EMMXXXX0` character device |
| SQEMM98 reproducibility | PASS; two independent builds have SHA-256 `eb3d443d7c12b6eb204e03a7ebac4b68a69d4690d0899ca93a37ad7a546d4930` |
| SQEMM98 structural validation | PASS; entry points, caller-state preservation, zeroed memory-test offset, one `INT 83H`, no `INT 10H`, and no DOS `AH=09H` output |
| SQEMM98 validator negative check | PASS; the earlier caller-state-unsafe build is rejected as `SQEMM98_CHECK_CALLER_STATE` |
| Supplemental builder shell syntax | PASS |
| Supplemental media generation | PASS; four fixed system files plus 39 payload files, 222,208 bytes free |
| Supplemental `CONFIG.SYS` | PASS; four expected CRLF lines extracted from the generated D88 |
| Supplemental SQEMM98 identity | PASS; D88 copy matches the independently generated driver byte-for-byte |
| Source-media protection | PASS; disposable source copy was unchanged before/after |
| Reproducibility | PASS; two independently generated outputs were byte-identical |
| Headless PC-Engine boot | PASS; SQEMM98 tested all EMS pages, initialized 1.0MB, and RDEMS registered a 640KB `C:` drive |
| Headless RAM-disk operation | PASS; copied the 114-byte `CONFIG.SYS` to `C:` and read back all four lines |
| Binary payload audit | PASS; no generated D88, ROM, font, or other binary payload is tracked |

The evaluated macOS executable has SHA-256
`e8af96a4c746482ef8ca12a84e141d943bdc7bf4872ea1b6001fab350ad35478`.
The evaluated MinGW executable at `build/mingw-cross/sdl2/vaeg.exe` has
SHA-256
`ea040180b484fb2a1b7ec49f2a8571330d5aaa7cabeb1765aed2bc98391c7476`.

The first hosted run after the bootable-media correction,
[31674777821](https://github.com/nakatamaho/vaeg/actions/runs/31674777821),
exposed a test-fixture defect: the state-save selftest destroyed the global
I/O tables, then the EMS selftest called `emsio_bind()` on their stale base
pointers. Linux GCC/Clang segfaulted and ASan identified a heap-use-after-free
in `getextiofunc`; Windows compatibility failed on the same unit-test step.
The final candidate builds isolated I/O tables inside the EMS test and
destroys them on cleanup. The three affected local CTests, the complete 81-test
suite, a macOS ASan/UBSan selftest, and all nine jobs in the final hosted run
passed after that correction.

The generated supplemental media and all raw validation logs remain outside
Git. The maintainer-supplied source media was used only through a disposable
copy; its private checksum is not recorded here. The canonical generated
output has SHA-256
`477d7cd94ccc2a70ffac9e47d8c9cb3f59ba8d1de8183c53239e92cbc96e0cc9`.

The headless boot used a disposable copy of that generated disk. PC-Engine
displayed EMMVA01, SQEMM98, EMMVA02, and RDEMS messages; SQEMM98 reported a
successful full-page memory test and 1.0MB capacity. RDEMS registered its
640KB RAM disk as `C:` because `B:` remains the second floppy. Guest commands
listed `C:`, copied root `CONFIG.SYS` to `C:\M90TEST.TXT`, listed the new
114-byte file, and read all four driver lines back. The disposable D88 was
unchanged after the run.

## G90 human gate

From a clean checkout and clean configuration:

1. Complete the standard V3-mode, bundled-demo, OS-boot, and simple-operation
   gate.
2. Confirm EMS Board appears below I/O Bank Memory, defaults to 1MB, accepts
   1 through 13MB, persists, and resets only when applying a change.
3. Boot the generated supplemental disk and verify SQEMM98's PC-Engine BIOS
   messages, configured capacity, and distinct data in multiple 16KB pages.
4. Verify `RDEMS.SYS` loads after SQEMM98 and supports RAM-disk read/write.
5. Enable I/O Bank Memory concurrently and verify both mechanisms.
6. Disable EMS Board and verify normal V3/OS operation.

G90 passes only when the maintainer explicitly reports that this gate passed.
