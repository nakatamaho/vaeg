<!--
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

# 8087 BASIC test

`fputest.bas` is an ASCII N88-日本語BASIC V3.1 program that checks the 8087
through BASIC: ATN, TAN, SIN, COS, EXP, LOG, SQR and a division, each in
double precision against the exact value (computed with mpmath at 200 bits).
A value is `OK` when its relative error is below 1E-14 (double precision
gives about 1E-16).

## Run

On a VA2/VA3 profile with the 8087 enabled (`NDP8087=true`), start BASIC
with `/87` and run it, or paste it (編集 → 貼り付け) after `BASIC /87`:

```text
BASIC /87
LOAD "FPUTEST.BAS"
RUN
```

Every line should read `OK` and the last line `failed: 0`. Without `/87`
BASIC uses its own decimal arithmetic, so the program also runs there but
does not test the 8087.

## Results in vaeg (2026-10-17)

- Before M123: `ATN(74/75)` is `NG` (error 9.7E-06), the inaccurate
  SoftFloat FPATAN series; the other values show errors of 1E-16 to 4E-16
  from the 53-bit rounding of the precision-control mapping.
- M123, host x87 on and off (`NDP8087HostFPU`): `failed: 0`.
