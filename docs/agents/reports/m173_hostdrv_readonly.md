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
namespace and a Linux dispatcher with a tested lifecycle adapter enabled
explicitly by `--hostdrv-dir`. Stage 3 has guest packet validation and the
read-only DOS profile probe and a development TSR with install/removal and
owned path checks. The archived reference tier and its provenance are
unaffected.

| Check | Result | Scope |
|---|---|---|
| `vaeg_hostdrv_wire` (CTest) | PASS | Exact request header lengths and masks; unassigned opcodes; reply marker, sequence and generation; stale success dword with unchanged request; staged publication; nested dispatch refusal; VA address arithmetic. |
| Strict C99 build and UBSan run | PASS | Same wire fixture. |
| MinGW compilation and PE import audit | PASS | Wire fixture and static cross-build of the transport checkpoint; not a G173 handoff and no Windows execution. |
| `vaeg_hostdrv_fcb_fixture_build` (CTest) | PASS | NASM-only build of `HDFCBTST.COM`; no FCB behavior exercised by this check. |
| Linux release-preset build and `vaeg_hostdrv_transport` (CTest) | PASS | Synthetic dispatcher through the actual shared ports: exact discovery, default disabled, completed query, stale dword after an unmatched command, A20-alias rejection, compatibility-mode refusal, and existing HOSTFAT transport regression. |
| Guest transaction exclusivity and installed-profile proof | NOT RUN | The port fixture runs with no guest executing; it does not exercise a TSR guard or prove guest IRQ, NMI, or debugger exclusion. |
| `vaeg_hostdrv_abi_fixture_build` (CTest) | PASS | NASM-only build of `HDABI.COM`. |
| Full redirector verb and FCB traces | NOT RUN by public checks | The transient fixture exercises only a bounded subset of the ABI; production handle and search dispatch are outstanding. |
| `vaeg_hostdrv_linux` (CTest) and strict C++17/UBSan build and run | PASS | Synthetic filesystem: selected-root rename, moved parent and replacement identities, symlink/hardlink/FIFO rejection, signed file-size bound, listing quota, positioned read/EOF, and unchanged file and directory timestamps. |
| `vaeg_hostdrv_names`, `vaeg_hostdrv_linux_namespace` (CTest), strict C++17/UBSan | PASS | Pinned encoding, DBCS separators and wildcard width, invalid encodings and names, native reservation and collisions, case-only carry-over, tombstones, live changes and pinned reads across replacement. |
| Shift-JIS alias amendment and namespace scan-cap regression (CTest) | PASS | Kana/kanji and mixed aliases, half-width characters, unmappable input, boundary truncation and suffix growth; one added entry forces a previously valid scan to fail without retiring its name ledger. |
| `vaeg_hostdrv_linux_service` (CTest) | PASS | Synthetic packets through `hostdrv_exchange`: generation and residency, DOS metadata and capacity, positioned reads, typed tokens, read sharing, exhaustion, immutable ordinal replay, eviction and high-water mark merging. No production port binding. |
| Linux release emulator target and HOSTDRV CTest label | PASS | Application compile/link and 8/8 focused tests at the dispatcher checkpoint. This is not a handoff build. |
| Synthetic lifecycle and `vaeg_state_storage` (CTest) | PASS | Real CPU shutdown-request and full machine reset hooks with synthetic residency; pre-output/pre-mutation save/load refusal including HOSTFAT override, exact errors for malformed policy/reserved/version, pending grant revocation, and independent maximum of both marks. Existing state-storage regression passed. |
| Linux lifecycle adapter (CTest) | PASS | The actual Linux wire dispatcher revokes on the lifecycle seam, keeps residency on shutdown, clears it on proven disposal and revokes an uninstalled grant on load. This does not enable the transport. |
| Native firmware-entry revocation (CTest) | PASS | Synthetic native far jumps exercise the actual machine loop, both reset-vector spellings, model-specific entry selection, and neighboring/wrong-model negative targets. Revocation precedes target execution; residency and live RAM are retained. No firmware or guest bootstrap is executed by this check. |
| Linux session option | Implemented | See the activation checkpoint below; not a redirector acceptance result. |
| Product warm bootstrap and Windows backend | NOT RUN by public checks | Remaining lifecycle and platform gates. |
| `vaeg_hostdrv_com_build` (CTest) | PASS | 8086 NASM build of the development `HOSTDRV.COM`; not a runtime check. |
| Full installed guest lifecycle and operation checklist | OPEN | Development TSR does not yet offer handle or search verbs. |
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

## Shared-port activation profile

The VA DOS machine-I/O source (`VAIO.ASM`, `VAHDD.INC`, `VAHDINIT.INC`,
`VASPEED.ASM`) has no identified 07EDh/07EFh client. The source audit of
VAEG guest tools identifies the HOSTFAT driver; the emulated option BIOS
sources (`idebios.asm`, `sasibios.asm`, `scsibios.asm`) also use the command
port. These are not independent masters. Native instruction execution owns
the channel, and the FDC subsystem has a separate I/O bus. The frontend
and frame/PC diagnostic script have no runtime operation that injects a
channel read or write; direct selftest I/O is a separate run. No native
NMI source was identified. The activation decision is for this execution
profile, not a claim that all guest programs or firmware avoid the ports.

The driver must refuse a busy transaction or an entry with TF set before
port access, and hold its guard with IF masked through reply validation.
It must not call DOS, firmware or a guest callback in that window. Keep
busy refusal for unknown clients; the audit is not authority to override
it. Re-audit before enabling any independent port master, port-injection
interface or native NMI source.

## Linux activation checkpoint

`--hostdrv-dir PATH` is explicit, session-only and Linux-only. The frontend
binds the Linux service on the emulator thread after the startup machine
reset. Reset and load revoke authority and do not replay the option.
Unsupported platforms fail explicitly. Keep full-path `openat2`
authorization on every Linux lookup: parent identity and timestamps detect
changes, not an atomic snapshot, and do not provide containment.

`hdwire.inc` adds the guarded 8086 discovery and transaction primitives.
The caller owns the bounce buffer and supplies an opcode-specific validator
that runs before interrupt restoration. The nonresident `HDQUERY.COM`
fixture exercises the busy branch and validates a QUERY response; it does
not install a drive. The Linux release-preset build, nine ROM-free HOSTDRV
CTest entries (including three NASM build checks), and state-storage CTest
passed. Encoding, EOL and case checks passed.

## Guest reply and DOS profile checkpoint

- `hdpacket.inc` prepares zeroed request headers and validates successful
  and failed DOS replies against a captured request inside `hd_exchange`.
  Validation covers generation, token and serial, lengths and metadata.
  The caller still owns transaction serialization, search name handling,
  and latching after ambiguous replies.
- `HDREPLY.COM` compares passing synthetic replies with single-bit
  mutations, without port access. `HDQUERY.COM` uses the same packet layer.
- `hdprofile.inc` implements structural eligibility for the named
  [MS-DOS 4.0 VA `msdos4-va.2` profile](https://github.com/FreeDOS-88VA/MS-DOS/tree/f8cd7e812a2408f8dd1fe4f83687920017affb1e):
  SDA extent, swap-table agreement, InDOS pointer,
  CDS bounds and ordinary/FCB SFT chains. `HDPROF.COM` probes eligibility
  and checks default and explicit CDS selection without publishing a CDS,
  attaching, or hooking INT 2Fh. This is not binary authentication or
  SUBST/JOIN dependency verification.
- The 11 HOSTDRV CTest entries passed. The assembly fixture CTests are
  build checks only. Case, encoding and EOL validators passed.

## Resident entry and CDS dependency checkpoint

- `hdresident.inc` preserves the caller register image and interrupt frame
  for chained requests and switches owned work to a guarded private stack.
  The worker can return through IRET or chain an untouched broadcast. The
  wrapper checks busy and the saved TF before dispatch; the eventual CLOSE
  worker must honor the no-port-I/O flag while still retiring a reference.
  No product classifier or worker is wired yet.
- `hdprofile.inc` excludes a free CDS slot referenced by a SUBST/JOIN CDS
  and refuses ambiguous drive prefixes. The transient fixture includes a
  passing synthetic table and isolated SUBST, JOIN and malformed-prefix
  mutations. Actual utility interoperability remains a separate gate.
- The Linux release-preset target built and all 12 HOSTDRV CTest entries
  passed. The new `HDRES.COM` CTest is a NASM build check only. The
  fixture constructs synthetic interrupt frames; it neither installs a
  hook nor touches the ports.

## Development TSR checkpoint

- `HOSTDRV /VAEG [X:]` checks existing redirectors, DOS profile and free
  CDS eligibility before touching the channel. Confirmed ATTACH publishes
  the saved CDS replacement and INT 2Fh hook. `HOSTDRV /VAEG /U` checks the
  private multiplex, vector, CDS and dependencies, then detaches, restores
  the DOS state, frees the block and acknowledges teardown via UNREGISTER.
  An ambiguous DETACH latches refusal; no retry or vector removal follows.
- The installer checks DOS's active/system CP932 and the DOSGroup filename
  casing and DBCS tables. The resident path handler rechecks the table
  bytes, classifies by the exact CDS, and issues STAT for chdir and get
  attributes. It denies its selected path mutations without a host call.
  It does not claim handle, search or FCB verbs. The teardown preconditions
  must grow as those resource tables are added.
- The Linux release-preset target built; 13 HOSTDRV CTest entries and the
  state-storage check passed. `vaeg_hostdrv_com_build` assembles the COM
  but does not run it.

## Next steps and remaining gates

1. Extend the resident classifier and worker with owned SFT allocation,
   positioned reads, seek, reference retirement, capacity and extended open.
   Then add search lifetimes, teardown draining, and the minimal FCB paths.
   Check NLS changes while resident, and extend failure latching to new
   resource-creating or resource-consuming transactions. Exercise all task cases,
   including busy and TF refusal with a no-port-I/O assertion, the FCB
   probe and distinct FCB caller traces. The product checklist is not yet
   complete; fixture status cannot substitute for it.
2. Complete the guest lifecycle gate. The [native entry hook](https://github.com/nakatamaho/vaeg/commit/f013cd0f0dccf8d28d69facf6ddc2352593c8ddf)
   revokes resident authority at a native fetch of `FFFF0h` or the
   model-specific bootstrap entry (`F16A0h` on VA, `F1F90h` on VA2).
   It preserves the save/load guard. This is not a general reboot
   detector. In particular, a software bootstrap before attachment is
   not currently distinguished from the initial cold boot and remains a
   gate item. Verify both normal reset paths and the DOS warm-bootstrap
   paths, including old tokens/cookies and the retained guard.
3. Implement the Windows anchored metadata/reopen adapter and prove the
   local filesystem and no-atime requirements. Keep macOS disabled unless
   it meets the entire policy. Update the declared source milestone for
   the actual M173 candidate, complete the task checklist and produce the
   clean static MinGW handoff. Windows execution remains G173; no
   acceptance is recorded here.
