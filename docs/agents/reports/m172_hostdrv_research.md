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

## G172 review revision

The revised design requires opt-in emulator-only discovery, exclusive port
transactions, and a response-only marker with an echoed sequence. It
specifies exact v1 packet lengths and fields, copied-DTA retention, SFT
identity and close cleanup, and a save/load guard that survives host detach
and CPU-only reset. The host design now separates root-relative authorization
from already-open object lifetime, rejects special files before data open,
and defines VA code page 932 folding, wildcard and alias identity rules.
Read-only service never writes host timestamps; `5701h` can still change
the DOS SFT locally.

The [G172 decision list](../research/m172_hostdrv_design.md#maintainer-decisions-for-g172)
recommends minimal FCB support, Linux and Windows first with conditional
macOS support, synthetic CDS/SFT and transport proof before UI, and gate
coverage for LASTDRIVE exhaustion and actual SUBST/JOIN utilities. These
are recommendations, not accepted gate decisions. Real-hardware discovery
safety is unproven, and the precise CDS install shape, platform guarantees,
and DOS lifecycle traces still need implementation evidence.

Revision checks: repository case, encoding, EOL and Markdown-comment
validators, diff whitespace check, and the unchanged NASM probe build.
No emulator or HOSTDRV implementation was built for this design revision.
