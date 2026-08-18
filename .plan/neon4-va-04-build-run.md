<!-- Copyright (c) 2026 Nakata Maho; 2-clause BSD license. -->

# Step 04 — build and run integration

1. Add `demos/neon4-va/build.sh` (or the repository-equivalent portable
   script) that accepts `NASM` and an output directory, creates the directory,
   and assembles `NEONVA.COM` without hard-coded local paths.
2. Add a CMake custom target only if it does not force private media into the
   build. Keep the source list explicit and make NASM absence a clean disabled
   target, matching existing guest targets.
3. Write `demos/neon4-va/README.md` with exact build command, output path,
   VAEG launch command using a disposable boot disk, expected screen, ESC
   behavior, and known limitations. Use neutral fixture names and no absolute
   maintainer paths.
4. Assemble the COM and inspect its size, symbols/listing, and all I/O
   constants. Keep generated COM outside Git unless requested.

Tests: build the guest target from a clean CMake build; run repository encoding,
EOL, case, diff, and unreferenced checks; verify the README command works from
the repository root.

