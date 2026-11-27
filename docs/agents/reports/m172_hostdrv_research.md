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
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
-->

# M172 — HOSTDRV research report

Scope: source audit and design proposal, plus a nonresident diagnostic.
No HOSTDRV implementation or emulator changes. The archived reference tier
and its provenance are unaffected.

- [Design and source references](../research/m172_hostdrv_design.md)
- [Task and review gate](../tasks/M172_hostdrv_design.md)
- [Diagnostic assembly](../../../tools/pc88va/hostdrv/hdprobe.asm)
  and [NASM checker](../../../tools/pc88va/hostdrv/check_probe.py)

## Reproducing the build check

With NASM on PATH, set `SCRATCH` to an existing out-of-tree working directory:

```sh
python3 tools/pc88va/hostdrv/check_probe.py "$SCRATCH/hdprobe.com"
```

The checker assembles 8086-compatible flat COM code with warnings treated as
errors, checks the size and completion marker, and prints its digest. It
requires a new output filename. No emulator executable is built by the check.

`HDPROBE` prints the raw DOS version word, AX and carry after 1100h (carry
set on entry), the 5D06h pointer and size words on success, the 5D0Bh table
pointer on success, and the 52h list-of-lists and CDS pointers with the CDS
count. The stride is a source-derived constant and is labeled as such; it is
not a runtime measurement. It installs no vectors and terminates via 4Ch.
5D06h uses a zeroed local DPL; it is not a side-effect-free peek in the DOS
server interface sense. Run at a foreground DOS prompt, not in a server
session. The version guard prevents the private calls on non-4.00 DOS but
is not a general compatibility check for other kernels claiming 4.00.

The design deliberately leaves FCB cache semantics, precise CDS install
shape and platform containment APIs as implementation acceptance items.
G172 is review of the proposal, not acceptance of a file-serving product.
