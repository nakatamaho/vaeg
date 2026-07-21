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
# HOSTFAT.SYS M54 prototype

`HOSTFAT.SYS` is an independently written PC-Engine CONFIG.SYS block driver.
It exposes the immutable read-only FAT12 snapshot created by vaeg's
`--hostfat-dir` option. It does not use INT 2FH redirector internals and never
sends guest writes to the host.

Build with CMake when NASM is available:

```sh
cmake --preset linux-release
cmake --build --preset linux-release --target hostfat_sys
python3 tools/pc88va/hostfat/check_driver.py \
  --input build/linux-release/guest/hostfat.sys
```

Or assemble directly:

```sh
nasm -f bin -o hostfat.sys tools/pc88va/hostfat/hostfat.asm
```

Copy the generated file to the PC-Engine boot disk and add:

```ini
DEVICE=HOSTFAT.SYS
```

Then start vaeg with a deliberately constrained ASCII 8.3 source tree:

```sh
vaeg --hostfat-dir /path/to/read-only-root
```

M54 snapshots are fixed at startup. Changes to the source directory are not
visible until vaeg is restarted with a newly generated snapshot. GUI,
persistence, refresh, and save-state identity are deferred to M55.

The prototype accepts at most 1024 entries and eight directory levels. Every
name must fit ASCII 8.3 using letters, digits, `_`, and `-`; lowercase is
folded to uppercase and folded-name collisions fail the entire mount. The
8 MiB geometry is fixed. The final FAT12-reserved cluster range is deliberately
left unused, so the maximum source payload is slightly smaller than 8 MiB and
also depends on directory and per-file cluster rounding.

## Private protocol

The driver uses the emulator-private channels that vaeg already reserves:

- `07EDH`: four-byte little-endian values;
- `07EFH`: command and response strings.

Writing `check_hostfat` to `07EFH` returns the two-byte signature `H1` only
when a compatible snapshot is mounted. For `read_hostfat1`, the driver first
writes the PC-Engine request-packet far pointer to `07EDH`, least-significant
byte first, then writes the command to `07EFH` and reads one result byte from
`07EDH`. Result 0 means success; all other results mean no guest bytes were
written. vaeg validates the complete request packet, command, unit, sector
count, LBA range, and destination range before copying. Guest write commands
are rejected inside `HOSTFAT.SYS` with write-protect status and never reach
this service.
