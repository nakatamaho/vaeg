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
# M168 — TSP sprite-pattern address alias

**G168: pending.** On 2026-11-25 the maintainer approved M168 implementation
and candidate verification before G167, based on `538877ee`. This exception
neither accepts G167 or G168 nor authorizes a merge.

## Evidence and scope

[PRJ_SP PR #2](https://github.com/mazone-ma3/PRJ_SP/pull/2) documents the
symptom (filled rectangles instead of packed-colour patterns) and corrects
its SPDA encoding. The [documented VA mapping](../../modernization/upd72022-tsp.md#9-address-byte-order-and-host-io)
puts CPU byte offsets `0000h–7FFFh` at TSP words `0000h–3FFFh` and CPU byte
offsets `8000h–FFFFh` at TSP words `8000h–BFFFh`. In particular the CPU byte
`AA00h` maps to `9500h`, not `5500h`. The predecessor's 16-bit translation
made both words fetch `AA00h`: `5500h * 2 = AA00h`, while
`(9500h * 2 + 8000h) & FFFFh = AA00h`. The startup IBF waiter is separate.

The native VA/VA2/VA3 sprite renderer treats descriptors **starting** at
`4000h–7FFFh` as fetching `FFh` per pattern byte with the VA-91 disabled.
A packed sprite therefore uses palette index F; a monochrome sprite uses
its foreground index. For the modeled all-one monochrome row, foreground
index 0 is transparent even with the background-color bit set: it neither
replaces a lower-priority pixel nor registers a collision. Valid pattern
fetches, line budget and row advancement are unchanged.
This affects only sprite pattern fetch; CPU TVRAM, text and descriptor
fetch, and TSP command timing are unchanged. No state-file change.

The common behavior across `4000h–7FFFh`, all rows of an affected descriptor,
monochrome, and across native models is an extrapolation of the reported
packed-colour symptom, not a measured address map or a claim about electrical
open-bus behavior. VA-91 and V1/V2 paths retain predecessor behavior, as does
`C000h–FFFFh` (legacy alias, not endorsed as hardware decode). Other valid or
legacy bases retain linear byte advancement even at boundaries; this does
not define hardware behavior for unsupported crossing patterns. The wider
VA2/VA3 TVRAM aperture alone does not establish TSP SPDA decoding.

## Verification and gate

- ROM-less `--selftest` checks the exact alias pair with nonzero data,
  low and attribute quarters, endpoints, row contents and both pixel modes;
  it also pins the unchanged VA-91, V1/V2 and upper unknown-range fallbacks.
  An overlap test checks that the missing monochrome row with foreground 0
  preserves an opaque sprite behind it and does not report a collision.
  The predecessor plus the new test fails at `5500h` with
  `M168_SPDA_FALSE_ALIAS`. The existing line-limit/collision test remains
  applicable.
- [`SPDAPROB`](../../../demos/tsp-sprites/README.md#m168-spda-probe)
  is the optional source-built, machine-labeled synthetic diagnostic. The
  default TSPSPR binary is byte-identical before and after the probe change.
- G168 requires independent review, the standard clean static MinGW handoff
  and human VA gate, plus a probe comparison on available hardware. Check
  that `0800h` and `9500h` are outlines, record whether `5500h` fills and
  record `C800h` without assuming a result. Record model and monitor setting;
  separate unavailable hardware variants from tested ones.
- On the candidate emulator, compare the old and corrected PRJ_SP binaries
  in the same gameplay scene and with the same VA2 and DOS settings. The TSP
  startup-wait fix must be present in both. Expect filled rectangles from
  the old encoding and the intended sprites from the corrected encoding.
  Keep the game disks and comparison captures out of the repository.
- Full media/demo and actual presenter campaigns are not implicit in this
  candidate verification. Ask the maintainer for decisions before merge.
