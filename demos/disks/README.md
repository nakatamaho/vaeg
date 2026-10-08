# Demo distribution disks

This directory is the canonical location for reproducible, non-bootable
`.d88.xz` distribution images produced by demo `build-d88.sh` scripts.

The allow-listed distribution images are:

```text
glass-orbit.d88.xz
neon3-distribution.d88.xz
neon4-distribution.d88.xz
sgp-pseudo-sprite.d88.xz
sgp-wireframe.d88.xz
zundamon-orbit.d88.xz
all-demos.d88.xz
```

plus one bootable image, `all-demos-msdos4.d88.xz`, under the maintainer-
approved MS-DOS 4.0 exception of `AGENTS.md` (below).

The builders accept a caller-selected raw D88 output path for local use, but
write the compressed companion here. Raw D88 files, bootable validation disks,
source templates, and private media remain outside the repository.

To make one non-bootable distribution disk containing every demo, use
[`tools/pc88va/build-all-demos-distribution-disk.py`](../../tools/pc88va/build-all-demos-distribution-disk.py):

```sh
python3 tools/pc88va/build-all-demos-distribution-disk.py \
  --source "/path/to/pcengine110-bootonly.d88" \
  --output /private/tmp/vaeg-all-demos.d88
```

This writes the raw image outside Git and the compressed companion as
`demos/disks/all-demos.d88.xz`.  The aggregate archive is built from the six
component distribution archives; it is not an input to the bootable builder.

To make one local bootable disk containing every component distribution, use
[`tools/pc88va/build-all-demos-bootable-disk.py`](../../tools/pc88va/build-all-demos-bootable-disk.py)
with a user-supplied PC-Engine 1.05/1.1 system disk or a PC-88VA MS-DOS 2.0
or 4.0 system disk of the FreeDOS-88VA project
(<https://github.com/FreeDOS-88VA/MS-DOS/releases>, for example
`msdos4-pc88va-2hd.d88`):

```sh
python3 tools/pc88va/build-all-demos-bootable-disk.py \
  --source "/path/to/PC-Engine 1.1.d88" \
  --output /private/tmp/vaeg-all-demos-bootable.d88
```

The builder extracts the six component `.d88.xz` images and installs them as
`A:\GLASS`, `A:\NEON3`, `A:\NEON4\16`, `A:\NEON4\65536`,
`A:\SPRITE\16`, `A:\SPRITE\256`, `A:\SPRITE\65536`, `A:\WIRE\16`,
`A:\WIRE\256`, `A:\WIRE\65536`, and `A:\ZUNDAMON`.
The supplied system disk provides the IPL and boot files, so the result is a
bootable PC-Engine or MS-DOS D88; of an MS-DOS disk only the system files,
`CONFIG.SYS`, `AUTOEXEC.BAT`, `LICENSE.TXT` and `README.TXT` are kept. The
per-demo `build-bootable-d88.sh` scripts accept either kind of system disk as
well. The source and raw output remain local artifacts and are not committed.

## Bootable MS-DOS 4.0 disk with every demo

`all-demos-msdos4.d88.xz` boots by itself: its system is the FreeDOS-88VA
project's MS-DOS 4.0 for the PC-88VA, built from Microsoft's MIT-licensed
MS-DOS source release (not supported by Microsoft or NEC; release
<https://github.com/FreeDOS-88VA/MS-DOS/releases/tag/msdos4-va.1>, image
`msdos4-pc88va-2hd.d88`, SHA-256
`9e4baf0e4098d2c6c2e3810fec0a524f1f7a2540ddc9e183241d1cd2941faa2e`). The MIT
licence is `A:\LICENSE.TXT` on the disk. Rebuild it with
[`tools/pc88va/build-all-demos-msdos4-disk.py`](../../tools/pc88va/build-all-demos-msdos4-disk.py):

```sh
python3 tools/pc88va/build-all-demos-msdos4-disk.py \
  --source /path/to/msdos4-pc88va-2hd.d88 --output /tmp/all-demos-msdos4.d88
```

The builder refuses any other source image, gives directory entries a fixed
time stamp (the same inputs give the same bytes), checks that the image
holds only the release's boot sector and kept MS-DOS files plus the demo
files of the six component distributions, and checks the `.xz` round trip.
The image listing:

```text
A:\IO.SYS, A:\MSDOS.SYS, A:\COMMAND.COM          MS-DOS 4.0 system
A:\AUTOEXEC.BAT, A:\CONFIG.SYS                    MS-DOS start-up files
A:\LICENSE.TXT, A:\README.TXT                     MS-DOS release notices
A:\GLASS\GLASS.COM
A:\NEON3\NEON200.COM, NEON400.COM
A:\NEON4\16\NEON4.COM, A:\NEON4\65536\NEON4.COM
A:\SPRITE\16\SGPD_7A.COM ... SGPD_7D.COM, SGPD_7S.COM
A:\SPRITE\256\SGP256S.COM, SGP256T.COM
A:\SPRITE\65536\SGP655S.COM
A:\WIRE\16\SGPWIRE.COM, A:\WIRE\256\SGPWIRE.COM, A:\WIRE\65536\SGPWIRE.COM
A:\ZUNDAMON\ZUNDAMON.BIN, ZUNDAORB.COM
```

Write the image out with `xz -dc demos/disks/all-demos-msdos4.d88.xz >
/tmp/all-demos-msdos4.d88` and boot it in drive 1. At the `A:\>` prompt,
change into a demo's directory and run it, for example `cd sprite`,
`cd 16`, `sgpd_7a`; the zundamon demo needs its own directory current
(`cd zundamon`, `zundaorb`) to find its atlas.
