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
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
-->

# GLASS ORBIT GA-1 bare entry proof

GA-1 proves only the approved bare-payload register boundary. It does not set
a graphics mode, call the VA Graphics BIOS, access TSP/SGP/GVRAM, or produce a
visual GLASS frame.

## Contract

The local-only `GLASSP1.COM` scaffold embeds the raw `GLASSG1.BIN` image,
copies it to `2000:0000`, and performs a far jump to that address. The raw
payload establishes its own execution state:

| Item | Required value |
|---|---|
| Image origin | `org 0` |
| Entry | `CS:IP = 2000:0000` |
| Data segments | `DS = ES = 2000h` |
| Stack segment and pointer | `SS:SP = 2000:F000h` |
| Direction flag | clear |
| Interrupt flag at idle | set |
| Idle capture location | `2000:0100` before `HLT` |
| Marker | `AX = 4741h` |

The COM file is a local VAEG/PC-Engine launch mechanism only. The raw payload
does not invoke DOS and does not return to the COM scaffold.

The first payload call remains `glass_geometry_step`, preserving the P0
geometry/data closure. Its successful return is represented by the marker at
the fixed idle location.

## Local VAEG proof

Use `run-vaeg-ga1.sh` with a local bootable 2HD template, a VAEG executable,
a local VA ROM directory, and a new output directory. The runner creates a
local bootable disk, invokes `GLASSP1` through the standard guest keyboard
path, waits for the fixed bare-payload idle PC using the M74 debug harness,
and captures registers plus a screen image.

`tools/verify-ga1-capture.py` fails closed unless the captured registers match
the table above, the debugger recorded the idle event, and the capture screen
exists. The screen is a harness artifact only; GA-1 has no graphics output to
inspect.

The bootable D88, ROMs, screenshots, logs, and capture directories are local
integration artifacts. They are not committed.
