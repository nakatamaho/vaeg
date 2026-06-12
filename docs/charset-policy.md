# Charset Policy

## Source Files

All text source files are stored as UTF-8.

Line endings are preserved. Git automatic line-ending conversion is disabled by `.gitattributes`.

## Legacy Windows Backend

The legacy Windows backend uses narrow Win32 strings. Runtime strings are treated as Shift_JIS / CP932.

When adding or changing build systems, configure the compiler so UTF-8 source literals are emitted as Shift_JIS for this backend.

Required intent:

- MSVC: UTF-8 source charset, Shift_JIS execution charset.
- GCC / Clang: `-finput-charset=UTF-8 -fexec-charset=CP932`.

Visual C++ 2008 does not support the modern `/source-charset` and `/execution-charset` options. The current VC2008 baseline is retained as a build gate, and resource/source charset behavior must be verified by Windows builds.

## SDL2 / Cross-Platform Backend

Future SDL2 and cross-platform backends should use UTF-8 runtime strings.

## Boundaries

Use `COMMON/CODECNV.C` and `COMMON/CODECNV.H` for explicit conversion between Shift_JIS and UTF-8 at platform boundaries.

Do not silently replace undecodable bytes. Stop and review any file that cannot be decoded exactly.
