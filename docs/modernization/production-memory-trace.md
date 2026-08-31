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
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.
-->
# Production-Memory CPU Trace

## Root cause

At public base commit `ade337c2d1f2ec0106a04361e1dd22a9995cb9b7`, the
normal production build (`VAEG_Z80_COMPAT_INTEGRATION_TRACE=OFF`,
`VAEG_ENABLE_TESTS=OFF`) compiled, while trace enabled with tests disabled did
not. `io/subsystem.cpp` exposed `Clock::now()` only inside
`VAEG_UPD780_INTEGRATION_TESTING`, although the trace-enabled subsystem uses
that method independently of tests. Enabling tests happened to supply the
declaration but also compiled `VAEG_UPD9002_SSTS_TESTING`, including the flat
uPD9002 test-memory seam. Such a build cannot establish production memory
addresses or contents.

The correction makes `Clock::now()` unconditionally public, removes the CMake
requirement that tests imply trace, and keeps the two features independent.
Production trace records the backend selected by the actual instruction fetch;
it does not read guest memory a second time. Test-enabled trace retains its
established fixture format and test-memory behavior.

## Build-mode contract

| Mode | Trace | Tests | uPD9002 memory path | Result |
| --- | --- | --- | --- | --- |
| P0 | off | off | production | normal build and selftest |
| P1 | on | off | production | bounded production trace |
| T0 | off | on | test as designed | existing test build |
| T1 | on | on | test as designed | existing traced test build |

`tools/qa/production_trace.py verify-matrix` parses `compile_commands.json`
for all four modes. It also inspects the P1 executable's exported symbols to
prove that production memory and bounded trace are linked while the flat
test-memory control symbol is absent. This is structural evidence; a source
filename grep alone is not accepted.

## Observation contract

A P1 instruction record reports a monotonic step, VAEG clock position, model,
production-memory marker, logical `CS:IP`, physical instruction-fetch address,
already-fetched opcode, general and segment registers, `SP`, FLAGS, and IF.
The opcode passed to the trace is the value already returned by
`upd9002_memoryread()`, so tracing does not add a device-visible memory access.
The end record reports post-instruction state and consumed/remain clocks.

With `--fdctrace`, FDD records retain their existing format and receive a
separate CPU-step correlation record. No disk or firmware-specific marker is
compiled into VAEG.

Use a strict instruction bound and explicit stop:

```sh
vaeg --trace-cpu 100000 --trace-cpu-output cpu.trace --trace-cpu-stop ...
```

`--trace-cpu-output` opens a separate trace file and fails closed if it cannot
be opened. `--trace-cpu-stop` exits after the requested instruction record and
emits `stop reason=trace-limit`; reaching the limit is not reported as guest
success. `--production-trace-capability` prints a stable public capability
record and is valid only as a standalone option.

When production trace is compiled out, the P0 executable remains
byte-identical to the same base built before this correction. When trace is
compiled in but not enabled at runtime, ROM-free QA compares the same synthetic
architectural checkpoint with and without tracing. Two clean P1 builds and two
identical P1 trace projections must also match byte-for-byte.

This capability is an emulator observation tool. A VAEG result is not a real
PC-88VA hardware result, and private firmware or disk evidence does not belong
in this repository or its CI.
