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
```

The builders accept a caller-selected raw D88 output path for local use, but
write the compressed companion here. Raw D88 files, bootable validation disks,
source templates, and private media remain outside the repository.

To make one local bootable disk containing every distribution above, use
[`tools/pc88va/build-all-demos-bootable-disk.py`](../../tools/pc88va/build-all-demos-bootable-disk.py)
with a user-supplied PC-Engine system disk:

```sh
python3 tools/pc88va/build-all-demos-bootable-disk.py \
  --source "/path/to/PC-Engine 1.1.d88" \
  --output /private/tmp/vaeg-all-demos-bootable.d88
```

The builder extracts the five `.d88.xz` images and installs them as
`A:\GLASS`, `A:\NEON3`, `A:\N4_16`, `A:\N4_65536`, `A:\SPRT16`,
`A:\SPRT256`, `A:\SPRT655`, `A:\WIRE16`, `A:\WIRE256`, and `A:\WIRE655`.
The supplied system disk provides the IPL and boot files, so the result is a
bootable PC-Engine D88. The source and raw output remain local artifacts and
are not committed.
