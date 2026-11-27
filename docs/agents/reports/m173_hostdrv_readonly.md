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

# M173 — HOSTDRV read-only progress

**Not ready for G173.** This is the isolated wire component of stage 1;
it is not a guest driver or an enabled emulator service. The archived
reference tier and its provenance are unaffected.

| Check | Result | Scope |
|---|---|---|
| `vaeg_hostdrv_wire` (CTest) | PASS | Exact request header lengths and masks; unassigned opcodes; reply marker, sequence and generation; stale success dword with unchanged request; staged publication; nested dispatch refusal; VA address arithmetic. |
| Strict C99 build and UBSan run | PASS | Same wire fixture. |
| MinGW compilation | PASS | Wire fixture only, no Windows execution or clean emulator handoff. |
| Shared-channel exclusivity, discovery, port I/O and writable mapping | NOT RUN | The wire fixture does not drive the actual ports or prove the execution profile. |
| DOS CDS/SFT fixture and caller traces | NOT RUN | No redirector fixture wired to the DOS ABI yet. |
| Linux/Windows backend policy and functional checks | NOT RUN | Backends not implemented. |
| `HOSTDRV.COM`, guest operations, reset, state and lifecycle checks | NOT RUN | No installed redirector. |
| G173 static MinGW handoff | NOT RUN | No candidate. |

The [design's FCB note](../research/m172_hostdrv_design.md) now distinguishes
HOSTDRV's carry/error result from the separate MS-DOS 4.0 `sf_OpenAge`
restore bug ([FreeDOS-88VA MS-DOS B4](https://github.com/FreeDOS-88VA/MS-DOS/blob/847a33c51265be9bb7fce38bd08bbc5738a83e6f/docs/ROADMAP.md#follow-ups)).
The profile is unchanged. The pending FCB case must check an injected read
error and a write denial, including the program-visible FCB status and
extended error, then check fresh FCB opens, closes and cache eviction without
assuming that the uncorrected DOS kernel preserved the age.
