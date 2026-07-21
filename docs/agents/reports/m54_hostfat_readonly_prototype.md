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
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
-->
# M54 read-only HOSTFAT prototype

Status: implementation plus local and hosted validation complete; G54
PC-Engine human gate pending. This report does not declare G54 passed and no
M55 work has begun.

The final report/remote SHA is supplied in the handoff because this file
cannot contain the SHA of the commit that contains itself.

## Identity and scope

- Branch: `topic/m54-hostfat-readonly-prototype`
- Starting SHA: `e5741b83fe12f121bb2eb955cbd7b8f0d2af61f2`
- Pre-report implementation SHA: `b661490322a998a20ca9881442c6a06c8c312958`
- Hosted implementation run:
  [build 29813271492](https://github.com/nakatamaho/vaeg/actions/runs/29813271492)
  (result recorded below)
- Final and remote SHA: supplied in the G54 handoff

The worktree began clean at the stated starting SHA. M54 adds only a
session-scoped, read-only host-folder snapshot and the guest block driver that
reads it. HOSTFAT remains disabled unless `--hostfat-dir` is supplied. No M55
GUI, persistence, refresh, save-state identity, long-name mapping, or writable
behavior is present.

## Commits before this report

1. `f34f7b5d685ca60d8abe936800fea2a89ff0d2ba` —
   `M54: define read-only HOSTFAT milestones`;
2. `567e04d2fc1ccd288d26bf927b4f052290c3f308` —
   `M54: build deterministic read-only FAT snapshots`;
3. `f79b677c1e48071779349a4ac3b404ed291f821a` —
   `M54: expose a fail-closed HOSTFAT sector service`;
4. `2263cb36c58bad5c7075fb0d997fe28009d714ce` —
   `M54: add the PC-Engine HOSTFAT block driver`;
5. `752a791ca812ea5201655cca7f83c9629f7b5913` —
   `M54: add HOSTFAT fail-closed regression coverage`;
6. `ef4d9e8054fc9f98eaf6465295cd9c1932e4f32e` —
   `M54: record private I/O channel correction`;
7. `2f527cca552a92fdbbf49c45acb35240fc40f0f4` —
   `M54: harden snapshot race and link rejection`;
8. `b661490322a998a20ca9881442c6a06c8c312958` —
   `M54: reject cross-platform hard links`.

## Files changed

```text
BUILD.md
CMakeLists.txt
docs/agents/ROADMAP.md
docs/agents/tasks/M54_hostfat_readonly_prototype.md
docs/agents/tasks/M55_hostfat_integration.md
docs/modernization/bug-fixes.md
io/hostfat.c
io/hostfat.h
io/np2sysp.c
sdl2/README.md
sdl2/cliopts.c
sdl2/cliopts.h
sdl2/hostfat_snapshot.cpp
sdl2/hostfat_snapshot.h
sdl2/np2.c
sdl2/selftest.c
tools/pc88va/hostfat/README.md
tools/pc88va/hostfat/check_driver.py
tools/pc88va/hostfat/hostfat.asm
```

No frozen-reference file, ROM, disk image, font, icon, wave payload, or
accepted uPD9002 golden was changed.

## Snapshot architecture

`--hostfat-dir <path>` is parsed into the session options only. After normal
configuration loading and CLI validation, but before `pccore_init()` and the
first machine reset, the frontend scans the source and builds an 8 MiB image
in temporary host memory. `hostfat_mount_image()` commits a private copy only
after the complete candidate image succeeds. A failed rebuild leaves an
already accepted image intact. The mounted image survives ordinary guest
reset and is freed on frontend shutdown or startup-error unwind.

The FAT12 geometry is fixed and matches the RDBMS block-driver convention:

| Field | Value |
|---|---:|
| bytes per sector | 1024 |
| sectors per cluster | 2 |
| reserved sectors | 0 |
| FAT copies | 2 |
| sectors per FAT | 7 |
| root entries | 128 |
| total sectors | 8192 |
| media descriptor | `F0H` |

Both FAT copies are generated from the same packed table. Allocation starts
at cluster 2, uses deterministic depth-first order after each directory is
sorted by its folded 11-byte DOS name, and never allocates FAT12 values
`FF0H`--`FFFH` as data clusters. Dates, times, attributes, volume label,
directory ordering, unused bytes, and cluster padding are deterministic.

M54 accepts only ASCII letters, digits, `_`, and `-` in 8.3 names. Lowercase
ASCII folds to uppercase. Unsupported names and folded collisions reject the
whole candidate rather than being omitted. The scan also rejects symbolic
links, detectable hard links, non-regular special files, more than 1024
entries, more than eight directory levels, unrepresentable sizes, and images
that exceed the fixed capacity. File type, size, modification time, and link
identity are checked before and after copying; a second bounded-buffer read
must reproduce the captured bytes before commit. This includes empty files
and detects ordinary same-size modification races.

The source path itself is neither stored in the image nor printed in the
snapshot summary. Generated snapshot and driver binaries remain untracked.

## Emulator-private transport

M54 uses vaeg's existing emulator-private channels and does not claim physical
PC-88VA hardware ports:

- `07EDH`: four-byte little-endian value channel;
- `07EFH`: command/response string channel;
- `check_hostfat` -> `H1` only while an M54 image is mounted;
- request far pointer via `07EDH`, then `read_hostfat1` via `07EFH`;
- one-byte result via `07EDH` (`0` success, nonzero no transfer).

The service reads the full 18-byte PC-Engine request packet and validates the
packet span, length, unit, command, nonzero transfer count limit, LBA interval,
destination interval, and mounted state before its first guest-memory write.
Zero-sector reads succeed without writing. Invalid packet, unit, command,
count, LBA, destination, and unmounted cases are deterministic and leave the
destination unchanged.

The audit also demonstrated and corrected a pre-existing binding defect:
`np2sysp_bind()` had attached both value and string callbacks to `07EFH`, and
only to the generic table. The value callbacks now use `07EDH`, the string
callbacks remain on `07EFH`, and both are attached to the generic and VA I/O
tables. The permanent correctness ledger records the evidence and fix.

## PC-Engine block driver

`tools/pc88va/hostfat/hostfat.asm` is an independently written non-IBM-format
CONFIG.SYS block driver. It implements initialization, media check, BuildBPB,
read, removable query, write/write-with-verify rejection, and deterministic
unknown-command handling. Writes return the PC-Engine write-protect status
inside the driver and never invoke the host service. Initialization installs
one unit after an `H1` probe and prints:

```text
HOSTFAT read-only drive ready
```

Without a mounted image it installs zero units and prints an unavailable
message. NASM is optional for emulator builds; when present, CMake builds and
structurally checks `guest/hostfat.sys`. A clean configure with NASM forced
unavailable also succeeded and disabled only the guest-driver target.

Two independent NASM invocations produced byte-identical files:

- size: 455 bytes;
- SHA-256: `b2d6a44445290a08ac413b79019f0c9388179d680e0f47fef2fcda7b20ae747a`.

The generated binary is not committed.

## Automated evidence

The ROM-less M54 coverage verifies:

- root files plus one subdirectory, FAT-copy identity, FAT chains, directory
  entries, file bytes, fixed volume metadata, and reserved-cluster exclusion;
- byte-identical image regeneration;
- ASCII folding, folded-name collision, invalid name, symbolic/hard link,
  excessive depth, entry-count, and capacity rejection;
- transactional preservation of the accepted image after rejected rebuilds;
- default-disabled and explicit CLI parsing;
- generic and VA `07EDH`/`07EFH` signature probes;
- a valid sector read and rejection of malformed length, unit, command,
  count, LBA, destination, and unmounted requests without destination changes;
- mount retention across `np2sysp_reset()` and explicit unmount behavior;
- deterministic assembly and structural validation of `HOSTFAT.SYS`.

The M52 BMS and M54 transport tests run after the accepted eight-step M42 CPU
trace has been exhausted. This prevents test setup I/O from contaminating the
frozen trace; the trace golden itself was not changed.

## Validation commands and results

All commands below exited 0 unless a deviation is stated.

```text
cmake --preset linux-ci-gcc -B .../m54-linux-ci-gcc --fresh
cmake --build .../m54-linux-ci-gcc --parallel 4
ctest --test-dir .../m54-linux-ci-gcc --output-on-failure

cmake --preset linux-ci-clang -B .../m54-linux-ci-clang --fresh
cmake --build .../m54-linux-ci-clang --parallel 4
ctest --test-dir .../m54-linux-ci-clang --output-on-failure

cmake --preset linux-ci-asan -B .../m54-linux-ci-asan --fresh
cmake --build .../m54-linux-ci-asan --parallel 4
ASAN_OPTIONS=detect_leaks=0 ctest --test-dir .../m54-linux-ci-asan \
  --output-on-failure
```

Each tests-enabled profile reported 35 passed, one external-dataset test
skipped because no external SingleStepTests path was configured, and zero
failed out of 36. The M42 graph, constructor, trace, 156-case harness, ABI and
state fixtures; M43 accepted sidecars; M44 state boundary; M45 native
execution; M46 normalization; M48 522-case diagnostic atomicity; and M49--M51
guards all passed unchanged.

The first sanitizer CTest invocation used LeakSanitizer defaults and failed
because this managed execution environment runs tests under ptrace. It was
rerun with only leak detection disabled. ASan and UBSan remained enabled; the
full suite then passed. Existing documented sound and vendored Z80 shift
diagnostics remain outside M54 and did not fail the configured suite.

```text
cmake --preset linux-release -B .../m54-linux-release --fresh
cmake --build .../m54-linux-release --parallel 4
SDL_AUDIODRIVER=dummy SDL_VIDEODRIVER=dummy .../vaeg --selftest
SDL_AUDIODRIVER=dummy SDL_VIDEODRIVER=dummy .../vaeg --smoke
```

The tests-disabled production cache records `VAEG_ENABLE_TESTS=OFF`.
`readelf --dyn-syms` found no HOSTFAT or M54 audit-only exported symbol. The
ROM-less selftest and headless smoke run passed.

```text
cmake --preset mingw-cross -B .../m54-mingw-cross --fresh
cmake --build .../m54-mingw-cross --parallel 4
WINEDEBUG=-all WINEPREFIX=... wine64 .../vaeg.exe --selftest
```

The MinGW build and Wine ROM-less selftest passed, including snapshot link and
file-stability checks and the VA transport. Direct NASM output compared equal
to CMake output, and `check_driver.py` accepted the header, BPB, protocol
markers, size, and digest.

```text
cmake --preset linux-ci-gcc -B .../m54-linux-no-nasm --fresh \
  -DVAEG_NASM_EXECUTABLE=VAEG_NASM_EXECUTABLE-NOTFOUND
python3 tools/repo/check_encoding.py
python3 tools/repo/check_eol.py
python3 tools/repo/check_case.py
python3 tools/repo/find_unreferenced.py
git diff --check
git diff --name-only <start> -- win9x i286x cpuxva/memoryva.x86 hlp romimage
```

The optional-NASM configure succeeded. Encoding, EOL, and case checks passed;
the unreferenced scan exited 0 with the same established 70 findings and no
M54 source; `git diff --check` passed; and the frozen/payload path diff was
empty.

## Hosted CI

[Run 29813271492](https://github.com/nakatamaho/vaeg/actions/runs/29813271492)
completed successfully at the pre-report implementation SHA. All seven jobs
passed: Ubuntu GCC, Ubuntu Clang, Ubuntu ASan/UBSan, Windows MSYS2 MinGW64,
macOS FetchContent SDL2, standalone Z80 conformance, and repository
invariants. The final handoff supplies the later report-only remote SHA.

## Deviations and remaining risks

- The external pinned SingleStepTests corpus was not configured locally; its
  committed M43 identity/sidecar checks passed, and hosted CI is authoritative
  for the normal repository matrix.
- LeakSanitizer cannot run under the managed ptrace wrapper, so only its leak
  detector was disabled; ASan/UBSan instrumentation remained active.
- Automated tests prove the FAT image and private transport, but cannot prove
  PC-Engine's live drive-letter assignment, request scheduling, or INT 83H
  message display. Those are deliberately the G54 human gate.
- The prototype is a fixed startup snapshot. GUI configuration, persistent
  selection, refresh semantics, save-state identity, broader deterministic
  name mapping, and final containment UX are deferred to gated M55.
- No claim is made that a hostile process with concurrent write access cannot
  deliberately race every metadata and duplicate-content check. M54 detects
  ordinary source mutation and fails transactionally; users should select a
  stable source tree for snapshot creation.

## G54 human-review checklist

- [ ] Build from a clean checkout and create a host folder containing only
  ASCII 8.3 files plus one ASCII 8.3 subdirectory.
- [ ] Build `HOSTFAT.SYS`, put it on a PC-Engine system disk, and add
  `DEVICE=HOSTFAT.SYS` to CONFIG.SYS.
- [ ] Start vaeg with `--hostfat-dir <folder>` and confirm
  `HOSTFAT read-only drive ready`.
- [ ] Identify the assigned drive; run DIR in its root and subdirectory, TYPE
  a text file, and copy files to a writable disk or RAM disk.
- [ ] Compare the copied bytes with the source files.
- [ ] Attempt create, overwrite, and delete; confirm write-protect and confirm
  the host folder remains byte-for-byte unchanged.
- [ ] Reset the guest and confirm the same snapshot remains readable.
- [ ] Start without `--hostfat-dir`; confirm zero-unit/unavailable behavior and
  unchanged V3 boot, bundled VA demo, OS, RDBMS, and MSE operation.

G54 passes only when the maintainer explicitly says so. Do not begin M55
before that approval.
