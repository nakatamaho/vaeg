<!-- Copyright (c) 2026 Nakata Maho; 2-clause BSD license. -->

# Step 04 — Build and D88 packaging

1. Add a portable NASM build script accepting `NASM` and an output directory.
2. Add an explicit CMake guest target only; never embed private media paths.
3. Build the COM with `/opt/local/bin/nasm`, CMake Linux Debug, and the
   repository-supported MinGW preset.
4. Copy the boot disk to `/private/tmp`, add only the generated COM using
   `tools/pc88va/pcengine_disk.py install`, list the D88 directory, and record
   the artifact SHA-256 outside Git.
5. Do not commit the D88, ROMs, the source reference, screenshots, or logs.
