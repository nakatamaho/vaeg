# M7 warning inventory

Build log sources:

- `/tmp/m7_gcc_build.log`
- `/tmp/m7_clang_build.log`

Command:

```text
cmake --build build/linux-debug --clean-first
```

Result: gcc build passed.

## GCC warning counts

| Directory | Warning category | Count |
|---|---:|---:|
| bios | `-Wunused-const-variable=` | 4 |
| font | `-Wsizeof-pointer-memaccess` | 1 |
| i286c | `-Wunused-but-set-variable` | 3 |
| io | `-Wunused-const-variable=` | 10 |
| sound | `-Wunused-but-set-variable` | 1 |
| sound/getsnd | `-Wunused-but-set-variable` | 8 |
| sound/vermouth | `-Wunused-but-set-variable` | 1 |

Total gcc warnings: 28.

## Clang warning counts

Commands:

```text
CC=clang CXX=clang++ cmake -B build/clang -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build/clang
```

Result: clang build passed.

| Directory | Warning category | Count |
|---|---:|---:|
| bios | `-Wunused-but-set-variable` | 1 |
| bios | `-Wunused-const-variable` | 4 |
| font | `-Wsizeof-array-decay` | 1 |
| font | `-Wsizeof-pointer-memaccess` | 1 |
| i286c | `-Wunused-but-set-variable` | 3 |
| io | `-Wunused-const-variable` | 10 |
| sound | `-Wunused-but-set-variable` | 2 |
| sound/getsnd | `-Wunused-but-set-variable` | 8 |
| sound/vermouth | `-Wunused-but-set-variable` | 1 |

Total clang warnings: 31.

## Compile-blocking source deviations

These were required to compile the M7 plain PC-98 C core with gcc. No
warning-only fixes were made.

| File | Reason |
|---|---|
| `io/pic.c` | `iocoreva.h` is VA-only; guarded include with `SUPPORT_PC88VA` for the non-VA PC-98 core build. |
| `io/np2sysp.c` | Local command string `str_np2` conflicted with `strres.h` extern `str_np2` under modern C; renamed local static symbol only. |
| `pccore.c` | `biosva.h` is VA-only; guarded include with `SUPPORT_PC88VA` for the non-VA PC-98 core build. |
| `pccore.c` | Debug/breakpoint callbacks are only declared under `VAEG_EXT`; guarded their use for the non-debug C core build. |
| `pccore.c` | Single-step loop called `i286x_step`/`v30x_step`; non-VA C core now calls `i286c_step`/`v30c_step`, while VA builds keep the x86 path. |
