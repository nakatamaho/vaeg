<!--
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
-->
# PC-88VA SCSI Support Disk

This document is an independently written summary of the PC-88VA SCSI setup
described by the original software documentation and the PC88.gr.jp forum.
It does not reproduce the forum post or package manuals. The repository does
not contain or redistribute PC-Engine, PCPLUS, SCHD, VBUFF, or SCFORM
binaries.

## Preservation Scope

VAEG does not currently emulate the PC-9801-55-compatible SCSI interface.
This work adds no SCSI hardware emulation and does not claim that the
generated disk can use a SCSI target under the current emulator.

The purpose is preservation: record the public package locations and exact
download identities, retain an independently written English setup procedure,
and provide a reproducible way to assemble the software while the public
downloads or a verified local cache remain available. A generated disk also
keeps the original package manuals together for use with real PC-88VA
hardware or a future SCSI-capable implementation.

## Sources and Software

The primary setup note is the PC88.gr.jp forum topic
[Connecting a SCSI hard disk to the PC-88VA][forum-501]. The required public
packages are:

- [PCPLUS 1.08][pcplus], which supplies the PC-9801-55-compatible SCSI BIOS
  service used by the other software.
- [The PCPLUS 1.08 correction][pcplus-patch], which fixes its DMA-mask setup.
- [BDIFF/BUPDATE 1.28][bdiff], used only on the host under DOSBox to apply
  that correction reproducibly.
- [SCHD 1.55T][schd], the PC-Engine block-device driver for SCSI hard disks
  and magneto-optical media.
- [VBUFF 1.02][vbuff], which changes the maximum logical-sector buffer size
  recorded in a PC-Engine system disk's IPL.
- [SCFORM 1.24][scform-topic], an interactive SCSI initialization and
  partitioning utility distributed as the forum attachment
  `SCF124.LZH`.

Every downloaded archive and the resulting patched `PCPLUS.SYS` are checked
against fixed SHA-256 values before the disk is created.

The package manuals remain the authority for command details, hardware
limitations, and redistribution conditions. The generated disk retains the
relevant original manuals under `A:\DOC`.

## Building the Disk

[`tools/pc88va/scsi-support.sh`](../../tools/pc88va/scsi-support.sh) takes a
user-supplied PC-Engine 1.1 D88 system disk and produces a new bootable D88.
The source image and generated image are local artifacts and must not be
added to Git.

On Debian or Ubuntu, install the host-side dependencies with:

```sh
sudo apt-get install curl dosbox lhasa python3 coreutils
```

Build a disk for a SCSI target whose ID is zero:

```sh
tools/pc88va/scsi-support.sh \
  --source /path/to/user-supplied-pcengine-1.1.d88 \
  --output /path/to/pc88va-scsi-support.d88
```

Use `--scsi-id 0..7` when the target does not use ID 0:

```sh
tools/pc88va/scsi-support.sh \
  --source /path/to/user-supplied-pcengine-1.1.d88 \
  --output /path/to/pc88va-scsi-id-3.d88 \
  --scsi-id 3
```

The destination must not already exist. `--cache DIR` selects an alternate
download cache. The default is the normal user cache under
`vaeg/pc88va-scsi-support`. A cached file with the wrong checksum is rejected
and is not silently replaced.

The builder first uses
[`create-vanilla-system-disk.sh`](../../tools/pc88va/create-vanilla-system-disk.sh)
to retain only the PC-Engine 1.1 IPL and required system files. It then
installs the SCSI drivers, utilities, and original documentation:

```text
A:\
  AUTOEXEC.BAT
  CONFIG.SYS
  PCPLUS.SYS
  SCHD.SYS
  ENGINEIO.SYS
  PCENGINE.SYS
  ADVGBIOS.SYS
  PCENGINE.COM

A:\BIN\
  SCFORM.COM
  VBUFF.COM

A:\DOC\
  PCPLUS.DOC
  PCPLUS.TXT
  SCSI55.TXT
  SCHD.DOC
  SCHD.LOG
  SCHD.TXT
  SCFORM.DOC
  SCFORM.LOG
  VBUFF.DOC
  VBUFF.LOG
```

`AUTOEXEC.BAT` adds `A:\BIN` to the command path. For target ID 0, the
generated `CONFIG.SYS` is:

```dos
FILES = 20
BUFFERS = 10
DEVICE = A:\PCPLUS.SYS
DEVICE = A:\SCHD.SYS -I0
```

PCPLUS must load before SCHD. The value after `-I` is the target's SCSI ID
and is generated from `--scsi-id`.

## Interface-Board Setup

Read `A:\DOC\SCSI55.TXT` before configuring a PC-9801-55 or compatible
board. Its PC-88VA-specific guidance includes these points:

- Use the board's standard I/O ports `0CC0h`, `0CC2h`, and `0CC4h`.
- Select an interrupt that does not conflict with installed hardware.
  The document identifies the PC-88VA's `INT0` through `INT3` choices and
  notes existing 2TD and SASI assignments.
- Programmed I/O is the normal transfer mode. Only DMA channels 0 and 3 are
  exposed to expansion slots, and those channels can conflict with SASI and
  2TD hardware. The forum note additionally warns that a board supporting
  only bus-master transfer is unlikely to work.
- The board ROM normally occupies `0DC000h-0DCFFFh`. The PCPLUS note says
  that this range is otherwise unused on a PC-88VA, but it must not overlap
  an EMS page frame. Disabling the board ROM is supported, and the forum
  note recommends doing so when practical.

These are software-configuration notes, not electrical-installation
instructions. Follow the interface board and target-device manuals for
termination, cabling, and hardware switch settings.

## Logical-Sector Buffer

An unmodified PC-Engine system normally handles logical sectors no larger
than 1024 bytes. SCFORM 1.24 can create a partition of roughly 1 through
64 MB with that logical-sector size.

Larger partitions require a larger PC-Engine buffer. To change the system on
drive A to a 2048-byte maximum logical sector, boot the support disk and run:

```dos
VBUFF -D1 -B11
```

VBUFF drive numbering is `0` for the current drive, `1` for A, `2` for B,
and so on. VBUFF changes a value in the selected system disk's IPL, so make
a backup first and select the drive containing the PC-Engine system files,
not the SCSI target. Reboot after changing it. A larger buffer also consumes
more guest memory.

VBUFF is deliberately not run by `AUTOEXEC.BAT`: 1024-byte partitions need
no IPL change, and altering the wrong boot disk would be an unsafe default.

## Initializing and Partitioning the Target

SCFORM writes SCSI disk metadata and can destroy existing partitions and
files. Back up the target before running it.

For a partition of at most approximately 64 MiB, start the interactive
formatter with:

```dos
SCFORM
```

After applying the 2048-byte VBUFF setting, start SCFORM with its
logical-sector-size extension:

```dos
SCFORM /S
```

The SCFORM 1.24 manual defines the option as `-S` and demonstrates the
slash form in its `SCFORM /MS` example. The forum topic says `-B` at this
point, but `-B11` belongs to VBUFF; SCFORM uses `/S` to expose the
2048-byte choice. The option expands the choices—it does not select the
2048-byte value automatically.

In SCFORM's menus:

1. Select the target by SCSI ID.
2. Initialize the device only if its current contents may be discarded.
3. Allocate a region, selecting a logical-sector size supported by the
   current VBUFF setting.
4. Leave no more than four desired regions active; SCHD exposes the first
   four active regions.
5. Exit and reboot before using the new drive.

SCFORM's documented approximate partition ranges are 1-64 MB with
1024-byte logical sectors, 65-128 MB with 2048-byte logical sectors, and
129-256 MB with 4096-byte logical sectors. Prefer the smallest logical
sector size that can represent the required partition.

## Driver Overrides and Limitations

SCHD normally discovers the cylinder count and blocks per cylinder. If the
target does not report usable geometry, add `-C<number>` for the cylinder
count or `-S<number>` for blocks per cylinder to its `CONFIG.SYS` line. When
both are supplied, SCHD gives `-S` precedence. This SCHD `-S` option is
unrelated to SCFORM's `/S` logical-sector option.

The historical setup has these important limitations:

- It cannot boot PC-Engine from the SCSI disk. Keep the generated floppy as
  the boot medium.
- The forum reports file corruption after roughly 30 MiB of use with some
  old PC-Engine system disks. It does not identify the first known-safe
  release. This builder intentionally requires the documented PC-Engine 1.1
  layout, but that structural check is not proof against the reported
  historical defect.
- SCHD rejects a partition whose logical-sector size exceeds the buffer
  recorded in the boot system's IPL.
- A larger logical-sector buffer consumes additional conventional memory.
- Geometry overrides should be copied from authoritative target-device
  specifications. Guessing `-C` or `-S` values can make the disk
  inaccessible.

## Verification

The builder verifies downloads, the patched PCPLUS result, the source
PC-Engine 1.1 filesystem layout, and the generated FAT12 structure. Current
VAEG cannot perform the functional checks below because it has no SCSI
interface emulation. They require real PC-88VA hardware or a future
SCSI-capable implementation:

1. Boot the generated disk in V3 mode.
2. Confirm that PCPLUS loads before SCHD and that SCHD reports the intended
   SCSI ID.
3. Run `SCFORM` only on a disposable or backed-up target.
4. Reboot, confirm that each active region receives a drive letter, then
   create, read, and delete test files.
5. For a large partition, cross the 30 MiB usage point with nonessential test
   data before trusting the environment.

[forum-501]: http://www.pc88.gr.jp/forum/viewtopic.php?t=501
[pcplus]: http://www.pc88.gr.jp/softlib/index.php?action=list_file&anum=2&gnum=378
[pcplus-patch]: http://www.pc88.gr.jp/softlib/index.php?action=list_file&anum=2&gnum=451
[bdiff]: http://www.pc88.gr.jp/softlib/index.php?action=list_file&anum=2&gnum=328
[schd]: http://www.pc88.gr.jp/softlib/index.php?action=list_file&anum=2&gnum=448
[vbuff]: http://www.pc88.gr.jp/softlib/index.php?action=list_file&anum=2&gnum=452
[scform-topic]: http://www.pc88.gr.jp/forum/viewtopic.php?t=502
