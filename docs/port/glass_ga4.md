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

# GLASS ORBIT GA-4 TSP vertical-blank polling proof

GA-4 proves only that the GLASS payload can observe the documented TSP
vertical-blank status bit and makes its updates conditional on successive
low-to-high observations. It retains GA-2's G0 640 by 200 single-plane 4bpp
mode, GA-3's diagnostic palette, and CPU aperture. It writes a static page of
palette-index-1 pixels once, then changes palette entry 1 at word port `0302h`
after each observed VB edge. It does not submit SGP work, alter TSP `SYNC`,
modify framebuffer descriptors, select a second page, or make a PC-88VA timing
or frame-rate claim.

## VB contract

The TSP status port is `0142h`; its bit 6 is `VB`, the vertical-blanking
period. The payload performs only byte input from that port:

```text
wait until VB = 0
wait until VB = 1
write palette entry 1 at `0302h`
```

This low-to-high sequence avoids treating a long assertion of VB as more than
one update. Because rewriting the entire 64 KiB page can extend beyond the
blanking period and cause a split frame, GA-4 does not rewrite GVRAM inside the
loop. The page remains `1111h`; the single palette entry changes colour. The
single word `OUT` avoids the visible split caused by a 64 KiB GVRAM rewrite and
keeps the repeated update independent of Graphics BIOS call state. A bounded
diagnostic polling loop reports the `47E4h` failure marker instead of reporting
success if either level is not observed. The loop bound is not a hardware timing
specification.

Evidence: `[VA-TEKU:2.TXT TSP status-port diagram, 0142h bit 6]`,
`[VA-TEKU:4.TXT §4.5 palette word ports 0300h--031Eh]`, and
`[SRC:io/tsp.c:tsp_i142]`. The local Technical Manual is maintainer-local and
is cited here, never from a source comment.

## Two bounded captures

Each run begins from a fresh boot and captures `2000:0200`, the program's
loop checkpoint after a completed update. The loop does not define a stack
pointer value as part of that checkpoint, so it is not an acceptance condition.
The two M74 scripts select different appearances of that checkpoint:

| Capture | Completed low-to-high observations | Palette entry 1 value | Expected RGB sample |
|---|---:|---:|---|
| `ga4-vb1` | 1 | `001Fh` | `(0, 0, 255)` |
| `ga4-vb5` | 5 | `FC1Fh` | `(0, 254, 255)` |

The checker rejects a missing checkpoint, wrong `4744h` success marker, wrong
count in `BX`, unexpected uniform-background geometry, black separator rows,
or equal RGB samples. It therefore establishes that four additional VB-gated
updates occurred between the two independently bounded observations. It does
not infer elapsed time between them.

The 640 by 200 result appears as 200 coloured rows with black intervening
rows in VAEG's 640 by 400 compositor capture. This is an emulator capture
observation, not a real-hardware raster-timing claim.

## Local execution

`build-ga4.sh` creates the raw `GLASSG4.BIN` payload and local `GLASSP4.COM`
loader. `build-ga4-bootable-d88.sh` creates a caller-selected local bootable
disk. `run-vaeg-ga4.sh` runs two fresh VAEG boots and invokes
`tools/verify-ga4-capture.py`. The D88, ROMs, logs, and captures are private
integration artifacts and are not committed.

`tools/repo/find_unreferenced.py --report` classifies the two GA-4 assembly
sources as locally owned NASM sources. `build-ga4.sh` and the loader establish
their build reachability; they are not deletion candidates.
