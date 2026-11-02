<!--
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
-->

# V1V2PORT — V3-mode I/O read probe (M144)

Checks on a PC-88VA/VA2 which ports the main unit drives in V3 mode: the
ports the technical manual marks V1/V2-only, the low bytes a WSR-E board
uses (E0h, E1h, E8h) and WSR-E's own ports, with 153h as a driven control.

```sh
tools/hardware/v1v2ports/build.sh OUTPUT.d88
```

builds `V1V2PORT.COM` and a bootable MS-DOS 4.0 disk (the msdos4-va.2
release image from the shared cache, checked by SHA-256). Keep the disk
outside the repository. Boot it on the machine (V3 mode) and run
`V1V2PORT`; the table appears on the screen and in `V1V2PORT.TXT`.

Each port is read four times followed by `MOV BL,55h` (column A) and four
times followed by `MOV CL,0AAh` (column B), with interrupts off. Nothing is
written to any port.

- `FLOAT`: A and B differ, each steady. Nobody drives the port; the value is
  what the bus last carried (prefetched code after the `IN`).
- `DRV xx`: all eight reads equal. A device answers, or the floating
  value happened to be equal (compare with the other ports).
- `UNSTABLE`: anything else.

In vaeg an undriven port reads FFh (the floating bus is not modelled), so
there `DRV FF` means "not decoded". On hardware M144 expects `FLOAT` for
the V1/V2-only ports and for E0h/E1h, and `DRV` for 153h.
