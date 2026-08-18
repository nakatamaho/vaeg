# Step 04 - input, build and disposable media

1. Poll DOS with `INT 21h`, `AH=06h`, `DL=FFh`; exit only when `AL=1Bh`.
   Restore video state on both normal and initialization-failure paths.
2. Add a NASM build script and a CMake custom target with a DOS 8.3 basename.
3. Build with `/opt/local/bin/nasm`, Linux CMake, MinGW cross CMake, and the
   repository self-test. Keep unrelated pre-existing formatter failures out
   of the diff.
4. Copy the boot-only D88 to `/private/tmp`, install only the new COM, list
   the image, and compare the embedded COM with the build output.
