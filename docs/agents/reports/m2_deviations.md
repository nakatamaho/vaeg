# M2 Deviations

Scope: VS2017 / PlatformToolset v141 project creation from the M1 VS2008 project.

No shared source files were edited in M2. The VS2008 `Win9x/np2.vcproj` and
`Win9x/np2.sln` files were not modified.

## Environment

| File | Line | Reason |
|---|---:|---|
| `Win9x/np2_v141.sln` | 1 | Created a separate VS2017 solution instead of editing `Win9x/np2.sln`, so the VS2008 solution remains untouched and buildable. |
| `Win9x/np2_v141.sln` | 5 | Points the VS2017 solution at the new `Win9x/np2.vcxproj` while retaining the M1 project GUID. |
| `Win9x/np2.vcxproj` | 2 | Created an MSBuild C++ project by translating the M1 `Win9x/np2.vcproj`; this Linux workspace has no local `devenv`, `msbuild`, or v141 toolchain. |
| `docs/agents/reports/m2_warnings.txt` | 1 | Full v141 warning capture could not be generated locally because the required Visual Studio tooling is unavailable here; no warning fixes were made. |

## Source Deviations

None.

## Project Deviations

| File | Line | Reason |
|---|---:|---|
| `Win9x/np2.vcxproj` | 21 | Leaves `WindowsTargetPlatformVersion` unset in the Globals section so VS2017 can select an installed Windows SDK instead of requiring SDK `10.0`. |
| `Win9x/np2.vcxproj` | 31 | Sets `PlatformToolset` to `v141` for `Release|Win32`. |
| `Win9x/np2.vcxproj` | 37 | Sets `PlatformToolset` to `v141` for `Trace|Win32`. |
| `Win9x/np2.vcxproj` | 43 | Sets `PlatformToolset` to `v141` for `WaveRec|Win32`. |
| `Win9x/np2.vcxproj` | 49 | Sets `PlatformToolset` to `v141` for `Debug|Win32`. |
| `Win9x/np2.vcxproj` | 32 | Sets the VS2017 character set to MultiByte for `Release|Win32`, matching M1 `_MBCS`. |
| `Win9x/np2.vcxproj` | 38 | Sets the VS2017 character set to MultiByte for `Trace|Win32`, matching M1 `_MBCS`. |
| `Win9x/np2.vcxproj` | 44 | Sets the VS2017 character set to MultiByte for `WaveRec|Win32`, matching M1 `_MBCS`. |
| `Win9x/np2.vcxproj` | 50 | Sets the VS2017 character set to MultiByte for `Debug|Win32`, matching M1 `_MBCS`. |
| `Win9x/np2.vcxproj` | 98 | Carries `_MBCS` into the `Release|Win32` preprocessor definitions. |
| `Win9x/np2.vcxproj` | 127 | Carries `_MBCS` into the `Trace|Win32` preprocessor definitions. |
| `Win9x/np2.vcxproj` | 156 | Carries `_MBCS` into the `WaveRec|Win32` preprocessor definitions. |
| `Win9x/np2.vcxproj` | 185 | Carries `_MBCS` into the `Debug|Win32` preprocessor definitions. |
| `Win9x/np2.vcxproj` | 105 | Adds `/source-charset:.932 /execution-charset:.932` for `Release|Win32`; also adds `/Zc:strictStrings-` as a legacy-code conformance relaxation. |
| `Win9x/np2.vcxproj` | 134 | Adds `/source-charset:.932 /execution-charset:.932` for `Trace|Win32`; also adds `/Zc:strictStrings-` as a legacy-code conformance relaxation. |
| `Win9x/np2.vcxproj` | 163 | Adds `/source-charset:.932 /execution-charset:.932` for `WaveRec|Win32`; also adds `/Zc:strictStrings-` as a legacy-code conformance relaxation. |
| `Win9x/np2.vcxproj` | 193 | Adds `/source-charset:.932 /execution-charset:.932` for `Debug|Win32`; also adds `/Zc:strictStrings-` as a legacy-code conformance relaxation. |
| `Win9x/np2.vcxproj` | 106 | Sets `ConformanceMode` to `false` for `Release|Win32`, enabling permissive mode for legacy sources. |
| `Win9x/np2.vcxproj` | 135 | Sets `ConformanceMode` to `false` for `Trace|Win32`, enabling permissive mode for legacy sources. |
| `Win9x/np2.vcxproj` | 164 | Sets `ConformanceMode` to `false` for `WaveRec|Win32`, enabling permissive mode for legacy sources. |
| `Win9x/np2.vcxproj` | 194 | Sets `ConformanceMode` to `false` for `Debug|Win32`, enabling permissive mode for legacy sources. |
| `Win9x/np2.vcxproj` | 120 | Translates the M1 non-incremental linker behavior for `Release|Win32` to the MSBuild `LinkIncremental=false` form. |
| `Win9x/np2.vcxproj` | 149 | Translates the M1 non-incremental linker behavior for `Trace|Win32` to the MSBuild `LinkIncremental=false` form. |
| `Win9x/np2.vcxproj` | 178 | Translates the M1 non-incremental linker behavior for `WaveRec|Win32` to the MSBuild `LinkIncremental=false` form. |
| `Win9x/np2.vcxproj` | 208 | Translates the M1 non-incremental linker behavior for `Debug|Win32` to the MSBuild `LinkIncremental=false` form. |

## NASM Custom Builds

All 34 M1 NASM custom build steps are carried into the v141 project. The commands
still use `nasm` on `PATH`, start from `$(ProjectDir)`, and emit COFF objects
under `$(IntDir)`.

| File | Line | Configuration | Reason |
|---|---:|---|---|
| `Win9x/np2.vcxproj` | 473 | `.\x86\PARTS.X86` `Release|Win32` | Carries over the M1 NASM object build step. |
| `Win9x/np2.vcxproj` | 478 | `.\x86\PARTS.X86` `Trace|Win32` | Carries over the M1 NASM object build step. |
| `Win9x/np2.vcxproj` | 483 | `.\x86\PARTS.X86` `WaveRec|Win32` | Carries over the M1 NASM object build step. |
| `Win9x/np2.vcxproj` | 488 | `.\x86\PARTS.X86` `Debug|Win32` | Carries over the M1 NASM object build step. |
| `Win9x/np2.vcxproj` | 495 | `..\I286X\DMAP.X86` `Release|Win32` | Carries over the M1 NASM object build step. |
| `Win9x/np2.vcxproj` | 500 | `..\I286X\DMAP.X86` `Trace|Win32` | Carries over the M1 NASM object build step. |
| `Win9x/np2.vcxproj` | 505 | `..\I286X\DMAP.X86` `WaveRec|Win32` | Carries over the M1 NASM object build step. |
| `Win9x/np2.vcxproj` | 510 | `..\I286X\DMAP.X86` `Debug|Win32` | Carries over the M1 NASM object build step. |
| `Win9x/np2.vcxproj` | 517 | `..\I286X\EGCMEM.X86` `Release|Win32` | Carries over the M1 NASM object build step and `NP2ASM.INC` dependency. |
| `Win9x/np2.vcxproj` | 523 | `..\I286X\EGCMEM.X86` `Trace|Win32` | Carries over the M1 NASM object build step and `NP2ASM.INC` dependency. |
| `Win9x/np2.vcxproj` | 529 | `..\I286X\EGCMEM.X86` `WaveRec|Win32` | Carries over the M1 NASM object build step and `NP2ASM.INC` dependency. |
| `Win9x/np2.vcxproj` | 535 | `..\I286X\EGCMEM.X86` `Debug|Win32` | Carries over the M1 NASM object build step and `NP2ASM.INC` dependency. |
| `Win9x/np2.vcxproj` | 543 | `..\I286X\MEMORY.X86` `Release|Win32` | Carries over the M1 NASM object build step and `NP2ASM.INC` dependency. |
| `Win9x/np2.vcxproj` | 549 | `..\I286X\MEMORY.X86` `Trace|Win32` | Carries over the M1 NASM object build step and `NP2ASM.INC` dependency. |
| `Win9x/np2.vcxproj` | 555 | `..\I286X\MEMORY.X86` `WaveRec|Win32` | Carries over the M1 NASM object build step and `NP2ASM.INC` dependency. |
| `Win9x/np2.vcxproj` | 561 | `..\I286X\MEMORY.X86` `Debug|Win32` | Carries over the M1 NASM object build step and `NP2ASM.INC` dependency. |
| `Win9x/np2.vcxproj` | 569 | `.\x86\OPNGENG.X86` `Release|Win32` | Carries over the M1 NASM object build step. |
| `Win9x/np2.vcxproj` | 574 | `.\x86\OPNGENG.X86` `Trace|Win32` | Carries over the M1 NASM object build step with the `.cod` listing output. |
| `Win9x/np2.vcxproj` | 579 | `.\x86\OPNGENG.X86` `WaveRec|Win32` | Carries over the M1 NASM object build step with the `.cod` listing output. |
| `Win9x/np2.vcxproj` | 584 | `.\x86\OPNGENG.X86` `Debug|Win32` | Carries over the M1 NASM object build step. |
| `Win9x/np2.vcxproj` | 591 | `.\x86\CPUTYPE.X86` `Release|Win32` | Carries over the M1 NASM object build step. |
| `Win9x/np2.vcxproj` | 596 | `.\x86\CPUTYPE.X86` `Trace|Win32` | Carries over the M1 NASM object build step. |
| `Win9x/np2.vcxproj` | 601 | `.\x86\CPUTYPE.X86` `WaveRec|Win32` | Carries over the M1 NASM object build step. |
| `Win9x/np2.vcxproj` | 606 | `.\x86\CPUTYPE.X86` `Debug|Win32` | Carries over the M1 NASM object build step. |
| `Win9x/np2.vcxproj` | 613 | `.\DCLOCKD.X86` `Release|Win32` | Carries over the M1 NASM object build step. |
| `Win9x/np2.vcxproj` | 618 | `.\DCLOCKD.X86` `Trace|Win32` | Carries over the M1 NASM object build step. |
| `Win9x/np2.vcxproj` | 623 | `.\DCLOCKD.X86` `WaveRec|Win32` | Carries over the M1 NASM object build step. |
| `Win9x/np2.vcxproj` | 628 | `.\DCLOCKD.X86` `Debug|Win32` | Carries over the M1 NASM object build step. |
| `Win9x/np2.vcxproj` | 635 | `.\x86\MAKEGRPH.X86` `Release|Win32` | Carries over the M1 NASM object build step and `NP2ASM.INC` dependency. |
| `Win9x/np2.vcxproj` | 641 | `.\x86\MAKEGRPH.X86` `Trace|Win32` | Carries over the M1 NASM object build step and `NP2ASM.INC` dependency. |
| `Win9x/np2.vcxproj` | 647 | `.\x86\MAKEGRPH.X86` `WaveRec|Win32` | Carries over the M1 NASM object build step and `NP2ASM.INC` dependency. |
| `Win9x/np2.vcxproj` | 653 | `.\x86\MAKEGRPH.X86` `Debug|Win32` | Carries over the M1 NASM object build step and `NP2ASM.INC` dependency. |
| `Win9x/np2.vcxproj` | 661 | `..\CPUXVA\MEMORYVA.X86` `Release|Win32` | Carries over the M1 NASM object build step and `NP2ASM.INC` dependency. |
| `Win9x/np2.vcxproj` | 669 | `..\CPUXVA\MEMORYVA.X86` `Debug|Win32` | Carries over the M1 NASM object build step and `NP2ASM.INC` dependency. |

## Verification Performed Here

```text
Win9x/np2.vcxproj: XML parse OK
Win9x/np2.vcxproj: CRLF only
Win9x/np2_v141.sln: CRLF only
Win9x/np2.vcxproj: 261 project items
Win9x/np2.vcxproj: 9 CustomBuild inputs
Win9x/np2.vcxproj: 34 per-configuration NASM Command elements
Win9x/np2.vcxproj: PlatformToolset v141 in all 4 configurations
Win9x/np2.vcxproj: WindowsTargetPlatformVersion is not pinned
Win9x/np2.vcxproj: /source-charset:.932 and /execution-charset:.932 in all 4 configurations
Win9x/np2.vcxproj: _MBCS in all 4 C/C++ configurations
Win9x/np2.vcproj and Win9x/np2.sln: unchanged in M2
```

Build status: not run in this Linux workspace because VS2017/v141 and VS2008
tooling are unavailable. VS2008 re-verification is not required here because M2
made no shared-source edits and did not modify the VS2008 project files.
