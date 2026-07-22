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

`HOSTFAT.SYS` is a clean-room PC-Engine CONFIG.SYS block driver distributed
under the repository's two-clause BSD terms. It exposes the immutable
read-only FAT12 snapshot created by vaeg's `--hostfat-dir` option. It does not
use INT 2FH redirector internals and never sends guest writes to the host.

The implementation was authored from the factual
[clean-room contract](../../../docs/agents/research/m54_hostfat_cleanroom_spec.md)
under the recorded
[input attestation](../../../docs/agents/research/m54_hostfat_cleanroom_attestation.md).
The final source fixes the NASM CPU level at 8086 so a V30 cannot encounter
80386 `0F 8x` near conditional-jump encodings. The generated-driver checker
fails if such an encoding or an unexpected command-dispatch edge appears.

The driver explicitly accepts PC-Engine's open and close lifecycle requests
as successful no-ops. These notifications bracket operations such as COPY;
all actual file data still arrives through the read-only sector path. Write
and write-with-verify requests continue to return write-protect.

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

The accepted clean-room output is 528 bytes with SHA-256
`c036b88178f058295eaeedae8c9dffd0bcf13addb13449c307b2fba921a8f675`.

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

Files and directories preserve the host's local last-write time in FAT
date/time fields. FAT has no timezone and stores seconds in two-second units;
earlier odd seconds are therefore displayed rounded down. Values outside
FAT's representable range clamp to `1980-01-01 00:00:00` or
`2107-12-31 23:59:58`.

The snapshot accepts at most 1024 entries and eight directory levels. Every
name must fit ASCII 8.3 using letters, digits, `_`, and `-`; lowercase is
folded to uppercase and folded-name collisions fail the entire mount. The
backing snapshot is fixed at 128 MiB. The driver advertises 65,360 logical
sectors of 2048 bytes (127.65625 MiB) with 16 sectors per cluster. PC-Engine
therefore counts 4084 data clusters and selects FAT12 rather than FAT16. The
remaining 176 backing sectors are inaccessible through the guest service.
The final FAT12-reserved cluster identifiers are deliberately left unused, so
allocation stops at cluster `0FEFH`: at most 127.4375 MiB of cluster payload
is available before directory and per-file 32 KiB rounding.

This 2048-byte-sector geometry deliberately approaches the practical FAT12
limit while retaining the driver's 16-bit sector number. Historical PC-88VA
SCSI MO support is not used by HOSTFAT and does not remove the need to verify
this BPB with PC-Engine at G55.

## Private protocol

The driver uses the emulator-private channels that vaeg already reserves:

- `07EDH`: four-byte little-endian values;
- `07EFH`: command and response strings.

Writing `check_hostfat` to `07EFH` returns the two-byte signature `H1` only
when a compatible snapshot is mounted. For `read_hostfat1`, the driver first
writes the PC-Engine request-packet far pointer to `07EDH`, least-significant
byte first, then writes the command to `07EFH` and reads one result byte from
`07EDH`. Result 0 means success; all other results mean no guest bytes were
written. The non-IBM PC-Engine packet is 22 bytes, with its transfer pointer,
sector count, and starting sector at offsets `0EH`, `12H`, and `14H`.
vaeg validates the complete request packet, command, unit, sector
count, LBA range, and destination range before copying. Guest write commands
are rejected inside `HOSTFAT.SYS` with write-protect status and never reach
this service.
