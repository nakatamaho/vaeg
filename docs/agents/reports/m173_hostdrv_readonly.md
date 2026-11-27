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

**Not ready for G173.** Stage 1 has the wire, shared-port and transient
DOS CDS/SFT fixtures. Stage 2 has the Linux file authorization primitives;
there is no enabled emulator service. The archived reference tier and its
provenance are unaffected.

| Check | Result | Scope |
|---|---|---|
| `vaeg_hostdrv_wire` (CTest) | PASS | Exact request header lengths and masks; unassigned opcodes; reply marker, sequence and generation; stale success dword with unchanged request; staged publication; nested dispatch refusal; VA address arithmetic. |
| Strict C99 build and UBSan run | PASS | Same wire fixture. |
| MinGW compilation and PE import audit | PASS | Wire fixture and static cross-build of the transport checkpoint; not a G173 handoff and no Windows execution. |
| `vaeg_hostdrv_fcb_fixture_build` (CTest) | PASS | NASM-only build of `HDFCBTST.COM`; no FCB behavior exercised by this check. |
| Linux release-preset build and `vaeg_hostdrv_transport` (CTest) | PASS | Synthetic dispatcher through the actual shared ports: exact discovery, default disabled, completed query, stale dword after an unmatched command, A20-alias rejection, compatibility-mode refusal, and existing HOSTFAT transport regression. |
| Guest transaction exclusivity and installed-profile proof | NOT RUN | The port fixture runs with no guest executing; it does not exercise a TSR guard or prove guest IRQ, NMI, or debugger exclusion. |
| `vaeg_hostdrv_abi_fixture_build` (CTest) | PASS | NASM-only build of `HDABI.COM`. |
| Full redirector verb and FCB traces | NOT RUN by public checks | The transient fixture exercises only a bounded subset of the ABI; there is no product TSR. |
| `vaeg_hostdrv_linux` (CTest) and strict C++17/UBSan build and run | PASS | Synthetic filesystem: selected-root rename, moved parent and replacement identities, symlink/hardlink/FIFO rejection, signed file-size bound, listing quota, positioned read/EOF, and unchanged file and directory timestamps. |
| Linux service integration and Windows backend policy and functional checks | NOT RUN | No DOS namespace, service binding, or Windows adapter. |
| `HOSTDRV.COM`, guest operations, reset, state and lifecycle checks | NOT RUN | No installed redirector. |
| G173 static MinGW handoff | NOT RUN | No candidate. |

The [design's FCB note](../research/m172_hostdrv_design.md) now distinguishes
HOSTDRV's carry/error result from the separate MS-DOS 4.0 `sf_OpenAge`
restore bug ([FreeDOS-88VA MS-DOS B4](https://github.com/FreeDOS-88VA/MS-DOS/blob/847a33c51265be9bb7fce38bd08bbc5738a83e6f/docs/ROADMAP.md#follow-ups)).
The profile is unchanged. The [FCB fixture](../../../tools/pc88va/hostdrv/hdfcbtest.asm)
checks AL status and extended errors 31 (one injected read fault) and 5
(write denial), then opens five FCBs in a four-entry cache, checks reuse
with a fresh serial, closes the stale FCB and checks that the replacement
still reads before closing the remaining FCBs. It does not assert that
uncorrected DOS preserved the age. Execution requires the synthetic
redirector endpoint with the one-shot fault described in the fixture.

## Transient DOS ABI fixture

[`HDABI.COM`](../../../tools/pc88va/hostdrv/hdabi.asm) requires a free CDS
above the physical drives and the default drive, and no prior redirector.
It installs and restores an in-process CDS and INT 2Fh hook without issuing
port I/O, and emits `HDABI PASS` only after its checks. It covers the
DOS-maintained CDS working directory, ordinary open and missing-file error,
CDS/SFT remote classification, duplicated reference counts, positioned
synthetic read, nonfinal close and final error close after simulated detach,
slot reuse with a fresh serial, and replay from a copied public DTA after
EOF. Its CDS has a synthetic device descriptor because the DOS 4409h path
reads `SDEVATT` through `curdir_devptr` even on remote drives. DOS 3Fh maps
redirector status 21 to immediate status 5 while preserving 21 for 5900h
(`DOS/MS_CODE.ASM:395–437`, `DOS/MS_TABLE.ASM:64–80`).

## Next steps

1. Continue Stage 2 from `6b64f99c378a70087014899f2237bf90cec3a798`:
   `sdl2/hostdrv_linux.*` implements root-relative `openat2`, metadata-only
   eligibility checks, verified procfs reopen with mandatory `O_NOATIME`,
   positioned reads, capacity, and bounded raw directory scans. `enumerate`
   returns names and inode hints, not authorized DOS entries. For each
   published entry, revalidate the directory and entry from the root,
   compare the enumeration identity, and only then retain identity pins.
   The before/after parent checks are not an atomic directory snapshot;
   review concurrent rename and replacement behavior before enabling service.
2. Add the pinned CP932 mapping and DOS namespace (including byte-aware
   wildcard matching, stable aliases, tombstones and bounded identity storage).
   Implement immutable searches, bounded file handles/sharing, timestamps,
   DOS error mapping and the generation/serial lifecycle. Bind the opt-in
   only on profiles with proven transport exclusivity; add reset/revocation
   and save/load guards before exposing the session option. Then complete the
   Windows anchored metadata/reopen adapter and static MinGW build; keep
   macOS disabled unless the full policy can be established.
3. Implement `HOSTDRV.COM`, using the transient ABI fixture as a starting
   point for CDS/SFT handling, not as install/ownership policy. Verify all
   verb and lifecycle cases, including the separate FCB probe and caller
   traces, under the approved design. Add guest-side packet validation and
   exercise the no-port-access busy/refusal path separately from the host
   dispatch guard.
4. Complete the task's checklist and clean static MinGW candidate handoff.
   Windows execution remains G173; no acceptance is recorded here.
