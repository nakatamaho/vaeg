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
DOS CDS/SFT fixtures. Stage 2 has pinned CP932 names, a Linux identity-based
namespace and an isolated Linux dispatcher with a tested lifecycle adapter;
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
| `vaeg_hostdrv_names`, `vaeg_hostdrv_linux_namespace` (CTest), strict C++17/UBSan | PASS | Pinned encoding, DBCS separators and wildcard width, invalid encodings and names, native reservation and collisions, case-only carry-over, tombstones, live changes and pinned reads across replacement. |
| Shift-JIS alias amendment and namespace scan-cap regression (CTest) | PASS | Kana/kanji and mixed aliases, half-width characters, unmappable input, boundary truncation and suffix growth; one added entry forces a previously valid scan to fail without retiring its name ledger. |
| `vaeg_hostdrv_linux_service` (CTest) | PASS | Synthetic packets through `hostdrv_exchange`: generation and residency, DOS metadata and capacity, positioned reads, typed tokens, read sharing, exhaustion, immutable ordinal replay, eviction and high-water mark merging. No production port binding. |
| Linux release emulator target and HOSTDRV CTest label | PASS | Application compile/link and 8/8 focused tests at the dispatcher checkpoint. This is not a handoff build. |
| Synthetic lifecycle and `vaeg_state_storage` (CTest) | PASS | Real CPU shutdown-request and full machine reset hooks with synthetic residency; pre-output/pre-mutation save/load refusal including HOSTFAT override, exact errors for malformed policy/reserved/version, pending grant revocation, and independent maximum of both marks. Existing state-storage regression passed. |
| Linux lifecycle adapter (CTest) | PASS | The actual Linux wire dispatcher revokes on the lifecycle seam, keeps residency on shutdown, clears it on proven disposal and revokes an uninstalled grant on load. This does not enable the transport. |
| Native firmware-entry revocation (CTest) | PASS | Synthetic native far jumps exercise the actual machine loop, both reset-vector spellings, model-specific entry selection, and neighboring/wrong-model negative targets. Revocation precedes target execution; residency and live RAM are retained. No firmware or guest bootstrap is executed by this check. |
| End-to-end warm bootstrap with an attached service, Linux session option, Windows backend | NOT RUN | No production binding; firmware-entry coverage and the guest transaction profile are not yet proven for activation. |
| `HOSTDRV.COM` and installed guest lifecycle/operations | NOT RUN | No product redirector. |
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

1. Finish Stage 2 bootstrap proof after the [native entry hook](https://github.com/nakatamaho/vaeg/commit/f013cd0f0dccf8d28d69facf6ddc2352593c8ddf).
   The machine loop revokes a resident session on a native fetch at physical
   `FFFF0h`, or at the model's firmware disk-bootstrap entry (`F16A0h` on
   VA, `F1F90h` on VA2). The hook intentionally retains residency and is
   not a general warm-reboot detector. Prove the supported VA and VA2 DOS
   warm-bootstrap paths end to end with an attached service and a stale
   file token/search cookie; verify the new boot cannot consume the
   previous authority and state storage remains guarded. Neither INT 19h
   nor reaching the reset vector is sufficient proof of a new DOS boot.
   Also distinguish initial startup (which must allow the explicit grant)
   from a later bootstrap before install (which must revoke an unconsumed
   grant). If the supported transitions cannot be identified reliably,
   keep discovery disabled and seek an approved profile clarification.
2. Prove the port exclusivity profile (IRQ masking, no NMI, other port
   masters or injected port access). Include the native trace-flag path:
   IF masking alone does not exclude a guest INT 1 trap. Exercise the guest
   no-port-access refusal separately. Keep discovery disabled until the
   profile and lifecycle hooks are verified; then bind `--hostdrv-dir` on
   the emulator thread. The session option must grant only after startup
   reset and never regrant on reset/load. Retain full-path `openat2`
   authorization on every Linux lookup: parent identity and timestamps
   detect changes, not an atomic snapshot, and do not provide containment.
   Then implement the Windows anchored metadata/reopen adapter and static
   MinGW build; keep macOS disabled unless it meets the entire policy.
   Update the declared source milestone for the actual M173 candidate.
3. Implement `HOSTDRV.COM`, using the transient ABI fixture as a starting
   point for CDS/SFT handling, not as install/ownership policy. Verify all
   verb and lifecycle cases, including the separate FCB probe and caller
   traces, under the approved design. Add guest-side packet validation and
   exercise the no-port-access busy/refusal path separately from the host
   dispatch guard.
4. Complete the task's checklist and clean static MinGW candidate handoff.
   Windows execution remains G173; no acceptance is recorded here.
