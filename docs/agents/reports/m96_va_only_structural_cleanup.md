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
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.
-->

# M96 VA-only structural cleanup report

## M96 status

M96a was report-only and passed G96a. M96b1 now removes only the proven-dead
EGC/GRCG residue. No ROM, disk, font, icon, cursor, or wave payload was
modified. The working branch is `topic/m96-va-only-structural-cleanup`.

The task file was absent at the evaluated baseline. This commit adds the
tracked task index at
[`docs/agents/tasks/M96_va_only_structural_cleanup.md`](../tasks/M96_va_only_structural_cleanup.md)
and registers M96 in `ROADMAP.md`.

## 1. Baseline confirmation

| Item | Result |
| --- | --- |
| Evaluated commit | `dfe50a1420c075040c12b96f00c315b5987a846a` |
| Branch before M96a | `main` |
| M96 branch | `topic/m96-va-only-structural-cleanup` |
| `git status --short` | Only pre-existing untracked `.codex-write-test` and maintainer-local `docs/98io/`, `docs/cpmva/`, `docs/disks/`, `docs/roms/`, `docs/tekumani/` |
| Submodules | None reported by `git submodule status` |
| Task file before M96a | Missing |
| CMake presets | `linux-debug`, `linux-release`, `linux-ci-gcc`, `linux-ci-clang`, `linux-asan`, `linux-ci-asan`, `mingw-release`, `mingw-cross`, `mingw-ci`, `macos-release`, `macos-macports`, `macos-asan`, `macos-ci` |

The local reference trees are not included in the report or source comments.
They remain outside the tracked M96 tree.

## 2. Production ownership inventory

This is a directory-level ownership map derived from the explicit CMake source
lists at the baseline. It is not a deletion verdict.

| Directory or tier | Baseline ownership |
| --- | --- |
| `common/`, `generic/` | Shared utility, text, profile, geometry, and information helpers used by active targets |
| `cpu/upd9002/` | Main uPD9002 instruction, memory, state, diagnostics, and DMA implementation; CPU adapter changes are excluded by M96 S4 |
| `cpu/upd780/`, `cpu/z80_compat_*` | uPD780 FDC-facing disassembly and shared compatibility backend used by production adapters; excluded by S4 |
| `diagnostics/` | Active host-side uPD9002 diagnostics |
| `io/` | Native VA built-in I/O, DMA, FDC, serial, timer, memory-control, SGP/TSP, and shared lifecycle callbacks |
| `cbus/` | Live PC-88VA C-bus ownership and device lifecycle for SASI, SCSI, MPU98II, and BMS; protected by S1 |
| `sound/` | VA FM/OPN/OPNA, PSG, rhythm, ADPCM, beep, sound ROM, and shared synthesis/input support |
| `fdd/` | FDD media, geometry, motor, XDF, D88, and SASI/SCSI backing support |
| `font/` | Character data, conversion, and host backing; `fontrom` remains in `mem[]` under S3 |
| `bios/` | VA ROM loader, simulated BIOS fallback, FDD bootstrap helpers, and SASI/SCSI BIOS service code; M96d/e audit only |
| `machine/` | Reset, clock, event, keyboard, state-save, timing, and machine policy |
| `memoryva/` | Authoritative VA CPU memory decoder and GVRAM mapping |
| `vram/` | Active VA raster/text/sprite/graphics conversion plus generic residue candidates under M96b2 |
| `sdl2/` | Portable host frontend, GUI, media, keyboard, selftest, pacing, and sound integration |
| `external/` | Vendored third-party ImGui, ymfm, SDL2, libarchive, zlib, and xz dependencies; not M96 cleanup scope |
| `romimage/` | Read-only ROM/resource source and generated payload inventory; protected by S2 |

## 3. Reset and bind lifecycle

### Cold reset (`machine/pccore.c`)

The observed order is:

```text
iocore_reset()
cbuscore_reset()
fmboard_reset(pccore.sound)
upd9002_memorymap_va()
iocore_build()
iocore_bind()
cbuscore_bind()
fmboard_bind()
```

`iocore_create()` occurs during machine construction and
`iocore_destroy()` during termination. `fmboard_reset(usesound)` also occurs
during state-load reconstruction before the callback tables are rebuilt.

### State-load reconstruction (`machine/statsave.c`)

The observed order is:

```text
iocore_reset()
cbuscore_reset()
fmboard_reset(pccore.sound)
... state sections are restored ...
upd9002_memorymap_va()
iocore_build()
iocore_bind()
cbuscore_bind()
fmboard_bind()
```

No ordering change is made in M96a. The three tiers are currently distinct:
`iocore` owns the canonical CPU-visible 16-bit VA map and built-in bindings;
`cbuscore` owns live C-bus device reset/bind ownership; `fmboard` owns the
selectable VA OPN/OPNA sound-board lifecycle. M96c must preserve this
distinction unless a behavior-backed alternative is demonstrated.

## 4. Dispatch inventory

| Dispatch or selector | Baseline finding | M96a classification |
| --- | --- | --- |
| `PCMODEL_VA` | Same value as `PCMODEL_VA1`; `pccore.model` is set once while `model_va` selects VA1/VA2 | `DEFER_INSUFFICIENT_EVIDENCE`; planned M96f configuration/state audit |
| `PCMODEL_VA1` / `PCMODEL_VA2` | Live VA1/VA2 runtime selection in memory, ROM, SGP, GUI, and selftests | `RETAIN_LIVE` |
| `SUPPORT_PC98*`, `PC9821*`, `PC9801*` | No active production selector found in the baseline source search; prior removal is recorded as `ALREADY_ABSENT` (M72/M91 where applicable) | `ALREADY_ABSENT` |
| `iocore16` | Only the unreferenced `io/iocore16.tbl` artifact remains; canonical VA map is `io/iocore.c` | `DEFER_TO_M96b3` |
| `CPU_ITFBANK` | Only writer is inactive `io/necio.c`; read guard remains in `biosfunc()` and the CPU state field is serialized | `DEFER_TO_M96b3/M96g` |
| `biosfunc()` | Live NOP side-channel entry and physical simulated-BIOS cases remain | `RETAIN_LIVE`; isolated M96d/e audit |
| `BIOS_SIMULATE` | Always-defined simulated BIOS path in `bios/bios.c` | `DEFER_TO_M96e` |
| `VAEG_FIX`, `VAEG_EXT` | No active selector remains in production CMake; historical evidence is retained in reports | `ALREADY_ABSENT` / M96h evidence check |

## 5. Two-reviewer debate

Both reviews inspect the same baseline and treat S1-S4 as settled facts.

| Item | Reviewer A - reduction advocate | Reviewer B - preservation advocate | Arbiter | Evidence |
| --- | --- | --- | --- | --- |
| `io/egc.*`, `cpu/upd9002/egcmem.*` | Not in production CMake; tool reports them unreferenced | Names and implementation could represent inherited graphics behavior; verify VA ownership and stale includes before deletion | `DELETE` in M96b1 | Explicit CMake lists; `find_unreferenced.py`; includes in each other only |
| `io/vramcompat.h` | No production include or symbol use | Header may be historical contract for the EGC family | `DELETE` candidate, pending M96b proof | No active include; no CMake entry |
| `vram/vram.c`, `vram/vram.h` | Generic state is only consumed by dead EGC code and stale includes | `bios.c`, `pccore.c`, and `statsave.c` include `vram.h`; prove no state symbols are used | `DELETE` in M96b2 | `rg` finds no `vramop`, `VOP_*`, or `MEMWAIT_*` in those production units |
| `vram/palettesva.c`, `vram/palettesva.h` | `palettesva.c` was the only active CMake entry and both files contain no implementation | No production include or symbol use | `DELETE` in M96b2 | Explicit `VAEG_VA_SOURCES` entry removed with the empty files |
| `io/dipsw.*`, `io/printif.*`, `io/fdd320.*`, `io/cpuio.*` | No CMake entry or runtime registration | DIP switch data itself is live in `NP2CFG`; do not confuse dead implementation files with live switch state | `DELETE` candidate, pending M96b3 | Active code accesses `np2cfg.dipsw` directly; candidate files are not built |
| `io/necio.*` | No production entry; sole `CPU_ITFBANK` writer is inactive | CPU state layout and legacy BIOS guard still need a separate state-aware decision | `DEFER_INSUFFICIENT_EVIDENCE` for state; source `DELETE` candidate | `CPU_ITFBANK` read/write census; state assertions in `upd9002_state.c` |
| `io/iocore16.tbl` | Unreferenced obsolete table | It may be a historical word-port specification, not the active map | `DELETE` candidate, pending M96b3 | No production include; `io/iocore.c` owns active map |
| `oprecord.c` | Not built and guarded by undefined `SUPPORT_OPRECORD` | Out-of-tree operation-record compatibility may still be intentional | `DEFER_INSUFFICIENT_EVIDENCE` | M72 explicitly left `SUPPORT_OPRECORD` for focused audit |
| `common/wavefile.*` | Not built and guarded by undefined `SUPPORT_WAVEREC` | Recording is an optional historical interface; verify all supported definitions first | `DEFER_INSUFFICIENT_EVIDENCE` | M72 left `SUPPORT_WAVEREC` for focused audit |
| `io/np2vasup.*` | Stub is unreferenced by production CMake | Header explicitly says it is retained for out-of-tree users | `RETAIN_COMPATIBILITY` | File comment and no active selector |
| `generic/unasm*`, `cmndraw*`, `dipswbmp*`, `common/mimpidef.*` | Self-contained, not built, and unreferenced | They may serve offline tools; deletion must not remove tool workflows accidentally | `DELETE` candidates / tool audit required | CMake and reference census |
| `sound/tms3631*`, `bios/rsbios.h` | No active CMake or symbol path | Legacy names alone are not enough; check generated/tool consumers | `DELETE` candidates | Production search has no consumers |
| `cbus/scsibios.res` | Payload is not included; `cbus/sasibios.res` is the live SASI payload | C-bus itself is live under S1; deleting one unused payload must not touch device code | `DELETE` candidate | `cbus/sasiio.c` includes `sasibios.res`, not `scsibios.res` |
| `fdd/fdd_mtr.res` | Payload is not included; active motor sound loads external WAV files | FDD motor behavior is live and must remain | `DELETE` candidate | `fdd/fdd_mtr.c` uses WAV names, not the resource |

## 6. Reachability freeze for M96b candidates

The following is preliminary evidence, not permission to delete before G96a.
`find_unreferenced.py` reported 71 items, including protected ROM generators,
guest tools, and the candidates below. M96b must add one final row per deleted
file with build, include, symbol, runtime, hardware, and historical evidence.

| Candidate group | CMake entry at baseline | Active include/symbol evidence | Initial decision |
| --- | --- | --- | --- |
| EGC/GRCG (`io/egc.*`, `cpu/upd9002/egcmem.*`, `io/vramcompat.h`) | Absent | Only candidate-to-candidate includes; no active VA registration | `DELETE` in M96b1 |
| Generic VRAM (`vram/vram.*`) | `vram.c` and `vram.h` removed; prior includes were stale | No production use of `vramop`, `tramupdate`, `vramupdate`, `VOP_*`, or `MEMWAIT_*` outside deleted EGC code | `DELETE` in M96b2 |
| PC-98 I/O (`io/dipsw.*`, `io/necio.*`, `io/cpuio.*`, `io/fdd320.*`, `io/printif.*`, `io/iocore16.tbl`) | Absent | No runtime registration; `np2cfg.dipsw` and `CPU_ITFBANK` uses are separate live/state paths | `DELETE` or defer by per-file proof |
| NP2 utility residue | Absent | `oprecord` and `wavefile` are macro-guarded; `np2vasup` explicitly retained as stub | Defer macro-guarded items; retain compatibility stub; audit others |
| Generated legacy payloads | Absent | `scsibios.res` and `fdd_mtr.res` have no production include | `DELETE` candidate; no C-bus/FDD code deletion |

`romimage/` files reported by the tool are read-only and are not M96b
deletion candidates.

### 6.1 M96b1 B1 deletion proof

The B1 files were outside every production CMake source list. A complete
tracked-source search found no production include, symbol reference, callback
registration, reset/bind path, or state-save entry after the earlier portable
VA migration. The only remaining references were candidate-to-candidate
includes and historical reports. The EGC/GRCG implementation therefore has
no runtime route in the current VA product and was removed as a group.

| File | CMake absent | Include absent | Symbol absent | Runtime absent | Hardware classification | Last historical reference | Decision |
| --- | --- | --- | --- | --- | --- | --- | --- |
| `io/egc.c` | Yes | Yes | Yes outside deleted group | Yes: no `egc_bind`/`egc_reset` registration | Obsolete unreachable legacy EGC port handler | `b04c6203` (`M86: update machine core references`) | `DELETE` |
| `io/egc.h` | Yes | Yes | Yes outside deleted group | Yes | Obsolete unreachable legacy EGC state contract | `2baf50de` (`M4: lowercase all tracked paths`) | `DELETE` |
| `cpu/upd9002/egcmem.c` | Yes | Yes | Yes outside deleted group | Yes: no VA memory-map route | Obsolete unreachable legacy EGC memory engine | `b04c6203` (`M86: update machine core references`) | `DELETE` |
| `cpu/upd9002/egcmem.h` | Yes | Yes | Yes outside deleted group | Yes | Obsolete unreachable legacy EGC memory interface | `484cf94d` (`M51: move uPD9002 core sources`) | `DELETE` |
| `io/vramcompat.h` | Yes | Yes | Yes | Yes | Obsolete compatibility-only GRCG/EGC state declarations | `2fe49c94` (`M88: remove retired non-VA display backends`) | `DELETE` |

This deletion does not remove the live VA GVRAM, GDC, SGP, TSP, or C-bus
boundaries. The VA renderer remains under `vram/*va.c` and the VA memory
decoder remains under `memoryva/`.

### 6.2 M96b2 B2 deletion proof

The generic VRAM files had no production definition or consumer after the
EGC removal. `bios/bios.c`, `machine/pccore.c`, and `machine/statsave.c` had
only stale `vram.h` includes; no code in those units referenced the generic
state, wait macros, or operation flags. The two VA palette files contained no
implementation and were not included anywhere. Their one CMake source entry
was removed together with the files. Comment-only stale includes in the VA
renderer were removed as part of the same cleanup.

| File | CMake absent | Include absent | Symbol absent | Runtime absent | Hardware classification | Last historical reference | Decision |
| --- | --- | --- | --- | --- | --- | --- | --- |
| `vram/vram.c` | Yes | Yes | Yes | Yes: no `vram_initialize` route | Obsolete generic VRAM state backing | `88399555` (`M72: remove inactive PC-9821 guarded code`) | `DELETE` |
| `vram/vram.h` | Yes | Yes after removing three stale includes | Yes: no `VRAM_T`, `VOP_*`, or `MEMWAIT_*` consumer | Yes | Obsolete generic VRAM/EGC compatibility declarations | `e5941339` (`M92: format active emulator core sources`) | `DELETE` |
| `vram/palettesva.c` | Entry removed from `VAEG_VA_SOURCES` | Yes | Yes: comment only | Yes | Empty obsolete VA palette translation residue | `71a4a3bd` (`M89: merge VA source directories`) | `DELETE` |
| `vram/palettesva.h` | N/A | Yes | Yes: comment only | Yes | Empty obsolete VA palette header residue | `71a4a3bd` (`M89: merge VA source directories`) | `DELETE` |

## 7. Retained legacy-looking names

These names are not deletion evidence. Their baseline disposition is:

| Name | Why it remains or is deferred |
| --- | --- |
| `mpu98ii` | Live PC-88VA C-bus MPU98II device selected through `cbuscore` |
| `fontpc98`, `fontv98` | Font conversion/backing paths in the active VA font subsystem; require behavior evidence before removal |
| `sasiio`, `scsiio`, `scsicmd` | Live PC-88VA C-bus storage devices under S1 |
| `np2sysp` | Active shared system-policy/I/O support in the production map; name is historical, path is live |
| `cbus` | Live PC-88VA expansion-bus lifecycle and ownership tier under S1 |

## 8. `mem[]` and state-save baseline

M96a does not change storage or state format. The reset path currently clears
main/HMA and legacy compatibility ranges, and also clears a font range at
`FONT_ADRS`. The exact live layout and state-section sizes are deferred to
M96e/M96g after producer/consumer evidence. In particular, `fontrom` remains
inside `mem[]` and no `mem[]` bound is changed here.

## 9. Comment evidence map

No source comments were changed in M96a. Hardware-facing comment evidence is
therefore empty for this stage. Later source changes must add one row per
edited comment here, using a tracked M96 report section and never an
untracked `docs/tekumani/` path.

## 10. Validation at baseline

| Check | Result |
| --- | --- |
| `git diff --check` | PASS |
| `python3 tools/repo/check_encoding.py --expect utf8` | PASS, 0 violations |
| `python3 tools/repo/check_eol.py --enforce` | PASS, 0 violations |
| `python3 tools/repo/check_case.py` | PASS, 0 findings |
| `python3 tools/repo/clang_format.py` | FAIL on four pre-existing M95 UI lines in `sdl2/np2.c` and `sdl2/scrnmng.c`; no M96 source changed |
| `python3 tools/repo/find_unreferenced.py --report` | PASS, report-only: 488 sources, 417 reached, 71 unreferenced |
| `cmake --preset linux-debug` | PASS |
| `cmake --build --preset linux-debug -j4` | PASS, 179/179 |
| `build/linux-debug/sdl2/vaeg --selftest` | PASS, all selftests passed |
| `ctest --preset linux-debug` | SKIP: no test preset named `linux-debug` |
| `ctest --test-dir build/linux-debug` | SKIP: no tests registered in this non-test build |
| `CCACHE_DISABLE=1 cmake --fresh --preset mingw-cross` | PASS |
| `CCACHE_DISABLE=1 cmake --build --preset mingw-cross -j4` | PASS, 725/725; warnings are pre-existing |

The MinGW build used the repository-supported `mingw-cross` preset. No
source-changing M96 work was included in either build.

## 11. Corrections against earlier reports

M96a does not yet make the M88 or M84a corrections. The required producer /
consumer checks are registered for M96b6 and remain open.

## 12. Human gates

| Gate | Evaluated commit | Result | Maintainer statement |
| --- | --- | --- | --- |
| G96a | `fd1214e3584b5cc21e1076f6f1ce0f956de72cc8` | **PASS** | Maintainer: human gate passed |
| G96b-G96i / G96 | Not reached | **PENDING** | Blocked by staged gate protocol |

M96b is now proceeding after the maintainer recorded G96a as passed. The
combined M96b gate remains pending until B1-B4 validation is complete.
