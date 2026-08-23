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
EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.
-->

# GLASS ORBIT GA-5 SGP clear proof

## Scope

GA-5 proves one intentionally small equivalence in VAEG: an SGP command list
containing `SET_WORK`, `SET_COLOR`, `CLS`, and `END` clears the selected G0
page to the same visible result as a dedicated GA-5 CPU reference. It does not
establish an SGP execution-time model or real PC-88VA conformance.

The guest sets G0 to 640x200, packed 4bpp, with the same video-BIOS setup and
palette used by GA-2. The SGP command list is stored in main RAM. Its physical
address is sent to the SGP command address ports, execution is started, and
the guest waits until the SGP busy status is clear.

## Command contract

The list is deliberately limited to this sequence:

| command | parameters | purpose |
| --- | --- | --- |
| `SET_WORK` | physical address of a zeroed 58-byte work area | establish required work-area ownership without assuming an undocumented field layout |
| `SET_COLOR` | `5555h` | select palette index 5 in all four packed nibbles |
| `CLS` | G0 physical base `200000h`, `7D00h` words | clear 640 x 200 x 4bpp page |
| `END` | none | finish the command list |

`7D00h` is 32,000 words, the derived size of one 640 x 200 packed-4bpp page.
The SGP register operations follow the command submission and busy-polling
contract recorded in [the VA video contract](va_video_contract.md) and the
existing focused SGP tests. The local Technical Manual is comparison evidence;
no untracked documentation path is cited from source code.

## Independent checks

After the busy status becomes idle, the guest maps the G0 CPU aperture and
compares every one of the `7D00h` words with `5555h`. It reports success only
when all words match. This is a completed-GVRAM readback, not a timing
measurement.

The human-facing SGP program makes no further GVRAM writes after that readback:
its completed image is a uniform palette-index-5 clear. The headless runner
uses a separately compiled CPU-reference program that performs the same
`5555h` word fill, then:

1. runs a fresh GA-5 CPU reference capture;
2. runs `GLASSP5` from a fresh local bootable D88;
3. validates the guest success marker and `7D00h` count;
4. validates the 640 x 200 physical display pattern in the composed viewport;
5. compares the complete 640 x 400 composed guest viewport with the GA-5 CPU
   reference.

The host-side viewport comparison is visual-output evidence. It is not a
substitute for the guest's direct GVRAM readback and is not a raw-memory dump.

## Reproduction

The runner takes a local bootable 2HD template, a VAEG binary, a local ROM
directory, and a new output directory:

```text
demos/glass-orbit/run-vaeg-ga5.sh \
  SOURCE_BOOTABLE_2HD.d88 VAEG ROM_DIRECTORY OUTPUT_DIRECTORY
```

It creates the bootable test D88 locally. That image contains local boot
infrastructure and is never a repository artifact.

## Result and boundary

The VAEG functional run passed all of these checks:

| check | result |
| --- | --- |
| guest SGP completion and all-word readback | PASS (`7D00h` words) |
| GA-5 visible 640 x 200 pattern | PASS |
| GA-5 CPU and GA-5 SGP composed viewport equality | PASS |

This result shows emulator-side CPU/SGP functional equivalence for this exact
clear operation. It remains `hardware_pending` until a PC-88VA run is compared
with an authoritative hardware observation.
