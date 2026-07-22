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
# M55 HOSTFAT integration

Status: implementation and automated validation complete; G55 PC-Engine and
standard human gate pending. G55 has not been declared passed.

## Identity

- Branch: `topic/m55-hostfat-integration`
- Starting and approved supplemental G54 SHA:
  `e0bafbaa3cc0b12f945e18c231c843fc17ff0392`
- Final and remote SHA: supplied in the handoff after the hosted-CI result is
  recorded

## Commits

1. `bf13d19` — `M55: define maximum FAT12 HOSTFAT geometry`
2. `ace68a7` — `M55: expand HOSTFAT to the FAT12 limit`
3. `a24ec58` — `M55: bind save states to HOSTFAT snapshots`
4. `15ed5c4` — `M55: harden snapshot names and file identity`
5. `0f4dfd1` — `M55: add persistent asynchronous HOSTFAT controls`
6. `1d87511` — `M55: document FAT12-max HOSTFAT workflow`
7. `57e3deb` — `M55: allow sanitizer time for FAT12-max checks`
8. Final report/CI-record commits: supplied in the handoff

## Files changed

- Geometry, image ownership, identity, and transport:
  `io/hostfat.c`, `io/hostfat.h`, `statsave.c`, `statsave.tbl`.
- Snapshot creation and frontend lifecycle:
  `sdl2/hostfat_snapshot.cpp`, `sdl2/hostfat_snapshot.h`,
  `sdl2/hostfat_manager.cpp`, `sdl2/hostfat_manager.h`, `sdl2/np2.c`,
  `sdl2/np2.h`, `sdl2/ini.c`, `sdl2/gui/gui.cpp`, `sdl2/selftest.c`, and
  `CMakeLists.txt`.
- Guest driver and deterministic checker:
  `tools/pc88va/hostfat/hostfat.asm`, `check_driver.py`, and `README.md`.
- Planning and user documentation: `docs/agents/ROADMAP.md`, the M55 task,
  `sdl2/README.md`, `sdl2/gui/README.md`, and this report.

No frozen-reference file, ROM, disk image, font, icon, cursor, or wave payload
changed.

## FAT12-max geometry

| Property | M55 value |
|---|---:|
| Logical sector | 2,048 bytes |
| Sectors per cluster | 16 |
| Cluster size | 32 KiB |
| Reserved sectors | 0 |
| FAT copies / sectors per FAT | 2 / 7 |
| Root entries / sectors | 128 / 2 |
| DOS-visible sectors | 65,360 |
| Backing sectors | 65,536 |
| DOS-visible capacity | 133,857,280 bytes (127.65625 MiB) |
| Data-cluster count used for FAT type | 4,084 |
| Highest allocatable cluster | `0FEFH` |
| Allocatable payload | 133,627,904 bytes (127.4375 MiB) |

The cluster count stays below the 4,085-cluster FAT16 cutoff. Allocation also
stops before FAT12's `0FF0H` reserved identifiers, so six geometrically present
tail clusters are not put in any file chain. The PC-Engine request packet
retains its 16-bit starting-sector contract. `HOSTFAT.SYS` advertises the same
BPB as the snapshot generator, and the generated-driver checker fails if the
geometry or cutoff changes.

The generated driver is 528 bytes with SHA-256
`74af84b10e2157e3c178e423d80469a81f1ad122bd82eb99520d69d44b6d82f4`.
Direct NASM plus GCC, Clang, ASan, and MinGW CMake outputs reproduced it
byte-identically.

Historical 128 MB SCSI MO support is plausibility evidence only. A physical
MO normally uses a SCSI host adapter and block-driver stack. HOSTFAT does not
emulate SCSI and needs no SCSI driver: `HOSTFAT.SYS` is itself the PC-Engine
block device. PC-Engine acceptance of its 2,048-byte logical sectors remains
an explicit G55 check.

## Persistent frontend and refresh lifecycle

`HOSTFAT` and `HOSTFATDIR` are persisted in `vaeg.cfg`. Emulate -> Configure
provides an enable switch, UTF-8 path input, and directory-only browser.
Enabling or rebuilding creates a complete candidate on an SDL worker thread.
Scan, copy, and image-preparation progress is synchronized through an SDL
mutex; the ImGui/event/render loop continues normally.

The currently mounted image is retained throughout a rebuild. File copying,
full-image SHA-256, and preparation of core-owned storage all occur on the
worker. The frontend commit is a constant-time pointer swap, followed by the
normal FDD-preserving guest reset. A failed build retains the prior mount and
reports the exact error. Disabling explicitly unmounts and resets. A build in
progress blocks another rebuild or unmount. Host changes are never synthesized
into a live mounted FAT view.

The command-line `--hostfat-dir` remains a session-only startup override. A
persistently enabled empty path is a clear startup error rather than a silent
unmounted boot.

## Snapshot identity and save states

Each complete image receives a SHA-256 identity; the existing short FNV-1a
digest remains display-only. A new version-1 `HOSTFAT` state section is 36
bytes: format version, mounted flag, two required-zero bytes, and the 32-byte
identity. It contains no host path or private source metadata.

State preflight applies this policy before any live subsystem is imported:

- mounted and matching identity: accept;
- saved mounted image but current image missing or different: reject with a
  clear compatibility error;
- saved unmounted state while an image is mounted: reject as a mismatch;
- legacy state without a HOSTFAT section: accept only when HOSTFAT is not
  mounted.

The selftest changes both CPU IP and guest memory before attempting a
mismatched load and verifies that both remain unchanged after rejection. The
M44 transactional CPU286/UPD9002 adapter, opaque residue policy, section
sizes, and CPU_SHUT `FLAGS 0000` behavior are unchanged.

## Name mapping and host-path safety

Valid unique ASCII 8.3 names are retained after uppercase folding. Longer,
spaced, and valid Unicode UTF-8 names receive deterministic hash-derived 8.3
aliases with deterministic collision resolution. DOS device names do not pass
through as literal short names. Invalid UTF-8, control characters, excessive
depth/count, and data that cannot fit reject the entire rebuild.

The builder canonicalizes the root, rejects symlinks and Windows reparse
points, requires every traversed entry to remain below that root, and accepts
only regular files and directories. POSIX device/inode identity or Windows
volume/file identity, type, size, and last-write metadata are checked before
and after each copy. Hard-link aliasing and source replacement during a build
fail closed. A partial candidate is never mounted.

## Automated validation

Unless noted as an expected environmental result, every command below exited
zero.

```text
cmake --preset linux-ci-gcc -B /tmp/vaeg-m55-final-gcc --fresh
cmake --build /tmp/vaeg-m55-final-gcc --parallel 4
ctest --test-dir /tmp/vaeg-m55-final-gcc --output-on-failure

cmake --preset linux-ci-clang -B /tmp/vaeg-m55-final-clang --fresh
cmake --build /tmp/vaeg-m55-final-clang --parallel 4
ctest --test-dir /tmp/vaeg-m55-final-clang --output-on-failure
```

GCC and Clang each passed 35 tests, skipped only the unavailable external
SingleStepTests corpus test, and had zero failures out of 36. These runs cover
the accepted M42--M54 dispatch, trace, ABI, state, REP+0F, protected-state,
rename, Z80, pacing, I/O-bank, and HOSTFAT gates. In particular, final dispatch
and accepted provenance artifacts, the 522-case diagnostic-stop suite, state
payload fixtures, and embedded M43 CI/full expectation summaries remained
unchanged.

```text
cmake --preset linux-ci-asan -B /tmp/vaeg-m55-final-asan --fresh
cmake --build /tmp/vaeg-m55-final-asan --parallel 4
ASAN_OPTIONS=detect_leaks=0 \
  ctest --test-dir /tmp/vaeg-m55-final-asan --output-on-failure
```

The first ASan/UBSan matrix ran concurrently with GCC and Clang. All tests
except the existing trace-equivalence wrapper passed; that wrapper reached its
old 180-second timeout because it invokes the now larger complete selftest
three times. M55 raises only that test timeout to 420 seconds. With no competing
builds, the unchanged test passed under ASan/UBSan in 207.85 seconds. The
standalone ROM-less suite had already passed in 68.69 seconds. LeakSanitizer
was disabled for the managed environment; address and undefined-behavior
instrumentation remained enabled.

```text
cmake --preset mingw-cross
cmake --build --preset mingw-cross --parallel 4
WINEDEBUG=-all \
WINEPREFIX=/home/maho/vaeg/build/m54-wine-prefix \
  wine64 build/mingw-cross/sdl2/vaeg.exe --selftest
```

The MinGW build and Wine ROM-less selftest passed, including the Windows
file-identity code path, asynchronous build/commit, state identity, FAT12
generation, and transport. Reparse-point rejection is compiled on Windows;
the selftest exercises it when the host permits creation of a test link. No
Wine-only failure occurred.

```text
python3 tools/pc88va/hostfat/check_driver.py \
  --input build/linux-debug/guest/hostfat.sys
python3 tools/repo/check_encoding.py
python3 tools/repo/check_eol.py
python3 tools/repo/check_case.py
python3 tools/repo/find_unreferenced.py
cmake --preset linux-ci-gcc -B /tmp/vaeg-m55-final-no-nasm --fresh \
  -DVAEG_NASM_EXECUTABLE=VAEG_NASM_EXECUTABLE-NOTFOUND
git diff --check
git diff --name-only e0bafbaa3cc0b12f945e18c231c843fc17ff0392 \
  -- win9x i286x cpuxva/memoryva.x86 hlp romimage
```

The driver checker passed. Encoding and EOL checks were silent; case checking
reported zero findings. The unreferenced scan retained the established 70
findings and did not list the new manager. Diff checking passed, and the
frozen/binary-payload comparison was empty. The optional-NASM configuration
also passed while disabling only the generated guest-driver target.

```text
cmake --preset linux-release -B /tmp/vaeg-m55-final-release --fresh
cmake --build /tmp/vaeg-m55-final-release --parallel 4
SDL_AUDIODRIVER=dummy SDL_VIDEODRIVER=dummy \
  /tmp/vaeg-m55-final-release/sdl2/vaeg --selftest
SDL_AUDIODRIVER=dummy SDL_VIDEODRIVER=dummy \
  /tmp/vaeg-m55-final-release/sdl2/vaeg --smoke
readelf --dyn-syms --wide /tmp/vaeg-m55-final-release/sdl2/vaeg
```

The tests-disabled cache records `VAEG_ENABLE_TESTS=OFF`. The ROM-less
selftest and headless smoke run passed. Dynamic-symbol filtering found no M55,
HOSTFAT selftest, snapshot-test, or audit seam exported by the production
binary. The release executable SHA-256 is
`2ae3fc9c56de91a2a27e644381b1019d90fba8680030190a28074033d0848563`;
the MinGW executable SHA-256 is
`47d9eeb990f6a62e652622b8dc1c1f00c3f57c170b3b539bdf58ff191de69c21`.

## Production isolation and hosted CI

- Tests-disabled Linux release and ROM-less smoke: passed.
- Dynamic symbol inspection: passed; no M55-only test/audit seam exported.
- Hosted Linux GCC, Linux Clang, ASan/UBSan, Windows MinGW, macOS,
  standalone Z80, and repository checks: pending final record and URL.

## Deviations and residual risks

- The ASan trace timeout was raised from 180 to 420 seconds because the test
  deliberately runs the complete 128 MiB snapshot suite three times. No test
  content, dispatch artifact, or expected output changed.
- An atomic rebuild temporarily retains the old 128 MiB image while creating
  and preparing its replacement. Peak host memory can approach 384 MiB; this
  is bounded and avoids exposing a partial image or blocking the UI during the
  bulk copy/hash.
- Automated tests prove the BPB is internally FAT12 and that every LBA fits the
  private 16-bit request contract. Only the G55 PC-Engine run can prove that
  this OS/driver combination accepts 2,048-byte logical sectors and reports the
  intended capacity.
- Real uPD9002 REP+0F semantics remain unknown and unchanged from the accepted
  M48 fail-closed policy. No CPU, timing, memory, DMA, interrupt, I/O, or
  protected-state policy was altered.

## G55 human gate

- [ ] From a clean checkout of the reported SHA, build Linux and/or Windows
  release plus `HOSTFAT.SYS`; verify the driver is 528 bytes with SHA-256
  `74af84b10e2157e3c178e423d80469a81f1ad122bd82eb99520d69d44b6d82f4`.
- [ ] Put that `HOSTFAT.SYS` on a copy of the PC-Engine boot disk and retain
  `DEVICE=HOSTFAT.SYS` in `CONFIG.SYS`.
- [ ] In Configure, enable HOSTFAT, browse to a test folder, press OK, and
  confirm progress while menus/window/input remain responsive.
- [ ] Confirm the successful commit resets the guest, prints the HOSTFAT ready
  message, and reaches the OS prompt without hanging.
- [ ] Run `DIR` and confirm PC-Engine accepts 2,048-byte sectors and reports
  approximately 127.66 MiB total capacity.
- [ ] `DIR`/`TYPE` a root file, subdirectory file, long/spaced name, and Unicode
  host name; record the deterministic 8.3 aliases.
- [ ] Copy a file larger than 32 KiB from HOSTFAT to writable guest media and
  compare its complete bytes with the host source.
- [ ] Add or change a host file and confirm it is invisible before rebuild;
  rebuild, observe the progress/reset, then confirm it appears.
- [ ] Save and load with the same snapshot and continue operation.
- [ ] Change/rebuild the snapshot, then load the old state and confirm a clear
  mismatch error with the running guest left unchanged.
- [ ] Confirm create, overwrite, rename, and delete on HOSTFAT return
  write-protect and leave the host tree unchanged.
- [ ] Disable HOSTFAT and confirm unmount/reset; restart vaeg and confirm the
  enabled/path setting persists when re-enabled.
- [ ] Repeat core HOSTFAT checks on both Windows and Linux.
- [ ] Complete the standard V3 boot, bundled VA demo, OS boot, keyboard, FDD,
  Sound Board II, save/load, reset, and no-test-seam checks.

The maintainer must explicitly state that G55 passed. No later milestone is
started by this report.
