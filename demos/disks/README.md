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
