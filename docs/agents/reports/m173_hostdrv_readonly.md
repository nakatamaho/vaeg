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
read-only DOS profile probe and a development TSR with install/removal,
owned path checks, ordinary DOS handles, enumeration, capacity and minimal
read-only FCB dispatch. The archived reference tier and its provenance are
unaffected.

| Check | Result | Scope |
|---|---|---|
| `vaeg_hostdrv_wire` (CTest) | PASS | Exact request header lengths and masks; unassigned opcodes; reply marker, sequence and generation; stale success dword with unchanged request; staged publication; nested dispatch refusal; VA address arithmetic. |
| Strict C99 build and UBSan run | PASS | Same wire fixture. |
| MinGW compilation and PE import audit | PASS | Clean static Linux-host build of `4329ff209de1a9e6763a63cf45f63e24301ccf21`. Windows HOSTDRV is disabled in that revision. Not a G173 handoff and no Windows execution. |
| `vaeg_hostdrv_fcb_fixture_build` (CTest) | PASS | NASM-only build of `HDFCBTST.COM`; no FCB behavior exercised by this check. |
| Linux release-preset build and `vaeg_hostdrv_transport` (CTest) | PASS | Synthetic dispatcher through the actual shared ports: exact discovery, default disabled, completed query, stale dword after an unmatched command, A20-alias rejection, compatibility-mode refusal, and existing HOSTFAT transport regression. |
| Guest transaction exclusivity and installed-profile proof | NOT RUN | The port fixture runs with no guest executing; it does not exercise a TSR guard or prove guest IRQ, NMI, or debugger exclusion. |
| `vaeg_hostdrv_abi_fixture_build` (CTest) | PASS | NASM-only build of `HDABI.COM`. |
| Full redirector verb and FCB traces | NOT RUN by public checks | The TSR implements ordinary and extended opens, FCB open/read/close, shared search dispatch and FCB exit reconciliation; NASM checks do not prove the DOS call paths. |
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
| Full installed guest lifecycle and operation checklist | OPEN | See remaining gates below. |
| G173 static MinGW handoff | NOT RUN | No candidate. |

The [design's FCB note](../research/m172_hostdrv_design.md) now distinguishes
HOSTDRV's carry/error result from the separate MS-DOS 4.0 `sf_OpenAge`
restore bug ([FreeDOS-88VA MS-DOS B4](https://github.com/FreeDOS-88VA/MS-DOS/blob/847a33c51265be9bb7fce38bd08bbc5738a83e6f/docs/ROADMAP.md#follow-ups)).
The profile is unchanged. The [FCB fixture](../../../tools/pc88va/hostdrv/hdfcbtest.asm)
checks AL status and extended errors 31 (one injected read fault) and 5
(write denial), then opens five FCBs in a four-entry cache, checks reuse
with a fresh serial, closes the stale FCB and checks that the replacement
still reads at byte offsets 0 and 65536 before closing the remaining FCBs.
It also checks FCB find-first/next and intentionally leaves a final FCB
open for DOS exit cleanup. It does not assert that uncorrected DOS preserved
its age. The read-error case requires the synthetic fault described in the
fixture.

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

`--hostdrv-dir PATH` is explicit, session-only and Linux-only.
Windows rejects the option before opening a folder. The approved Windows
path requires handle-relative, non-reparse authorization with ancestor
rename/replacement protection, followed by a verified same-object data
or listing handle with no access-time updates. Microsoft documents
[per-handle suppression with `LastAccessTime = -1`](https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/ns-wdm-_file_basic_information);
that alone does not establish containment or provider behavior. No
Windows backend is enabled, and a MinGW compile/import audit cannot
establish either guarantee. The frontend
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

## Ordinary handles and directory checkpoints

- `hdfiles.inc` allocates ordinary network SFTs from the DOS table, tracks
  ownership and host tokens, and implements chunked reads, EOF seek, commit
  without writes and close with local reference retirement. Uninstall
  refuses while any owned handle remains. `HDFILE.COM` exercises a
  reproducible 5120-byte input, handle duplication, EOF and write denial.
- `hdsearch.inc` implements immutable ordinal replay with PSP ownership,
  bounded eviction, process-exit release, and explicit uninstall draining.
  Capacity reports 512-byte sectors and zero free units for the read-only
  mapping. All 14 HOSTDRV CTests passed; COM checks assemble only.
- Extended open follows the approved design amendment: after a read-only
  open succeeds, `112Eh` writes action 1 to the validated saved INT 21h
  register frame, not to the INT 2Fh frame. `HDFILE.COM` checks 6C00h with a
  sentinel CX, then distinguishes missing-file failure from denied creation.
  The Linux build and all 14 HOSTDRV CTests passed; the fixture CTest is
  assembly-only.

## COMMAND TYPE/COPY and upper RAM qualification

The COMMAND source calls `5702h` for its code-page XA query before
transferring file data (`CMD/COMMAND/TCMD1B.ASM`, `COPY.ASM`). For a remote
SFT, `DOS/HANDLE.ASM` dispatches that call to `112Dh` with BL=2 and routes
carry-set returns to `SYS_RET_ERR`. The approved amendment now implements
an empty list for owned `5702h`: return saved CX=2 for the size query; with
at least two bytes of capacity, also store a zero entry count in the caller's
buffer. Set-XA remains denied. No host XA access is involved. `HDFILE.COM`
checks the size and list forms, adjacent sentinels, a destination outside
conventional memory, and set-XA denial. All 14 HOSTDRV CTests passed; the
fixture CTest checks assembly only.

The saved DOS INT 21h frame can reside above `80000h`, which caused the
redirector's saved-frame validation to reject otherwise valid extended
opens and XA queries before any file transfer. The approved
[guest-write amendment](../research/m172_hostdrv_design.md#m173-amendments-empty-xa-and-upper-dos-output-ram)
separates CPU writes from host direct memory access: CPU stores now follow
DOS's mapping with span and resident-overlap checks; host copies stay in the
validated bounce window below `80000h`. There is no new guest interface.
The host-internal decoder query retains its synthetic BMS-unbound, bound
bank-zero, nonzero selector and short-RAM checks.

## FCB dispatch

`1116h` distinguishes a zero-refcount compatibility FCB entry in the FCB
SFT chain from a reserved entry in the ordinary chain. A successful FCB
open preserves the DOS FCB mode bit and process field, installs read-only
access and uses the service's nonzero allocation serial. The READ offset
comes from the DOS-supplied per-operation position. Final close retires the
local reference even if the host close fails; the FCB error 6 disposition
allows DOS eviction to continue. `111Dh` reconciles only matching owned FCB
entries by `PROC_ID`, does not retire ordinary handles by PSP, and chains
the broadcast.

## Final local verification scope

- Linux release-preset build and 17 focused CTests passed, including the
  HOSTDRV contracts, NASM fixture builds, debug harness and state storage.
  Repository case, encoding and EOL checks passed.
- `HDBMS.COM` is a forced high-load fixture: it checks the refusal exit
  status and exact preservation of INT 2Fh and the CDS table. Its CTest
  assembles it; it does not cover execution.
- Debug script `reset` invokes the frontend Reset menu's full-reset path
  at a frame boundary. It is not a normal VA guest warm-reboot shortcut.
  The latter remains a distinct gate item; a scripted frontend reset
  does not substitute for it.

## Next steps and remaining gates

1. Complete the task's installed behavior checklist: JFP/FEP, SUBST/JOIN,
   NLS changes while resident, copied-DTA replay, and no-port-I/O assertions
   for busy and TF refusal. A busy or TF-set abort chains without performing
   FCB reconciliation; this exceptional lifecycle path still needs coverage.
2. Complete the guest lifecycle gate. The [native entry hook](https://github.com/nakatamaho/vaeg/commit/f013cd0f0dccf8d28d69facf6ddc2352593c8ddf)
   revokes resident authority at a native fetch of `FFFF0h` or the
   model-specific bootstrap entry (`F16A0h` on VA, `F1F90h` on VA2).
   It preserves the save/load guard. This is not a general reboot
   detector. In particular, a software bootstrap before attachment is
   not currently distinguished from the initial cold boot and remains a
   gate item. Verify both normal reset paths and the DOS warm-bootstrap
   paths, including old tokens/cookies and the retained guard.
3. Run the Windows NTFS fixture and Windows guest check against the Windows
   adapter checkpoint below. A compile does not establish effective access-time
   suppression or ancestor rename/replacement exclusion. macOS stays disabled.
   The source milestone is `M173-dev`.

## Windows NTFS preflight

A minimal MinGW executable returned status 0 through WSL2 interop. The
checkout and task scratch are on ext4, so this only verifies that Windows
executables can launch; it does not exercise local NTFS. The proposed NTFS
scratch is outside the repository's filesystem access boundary and was not
created or accessed. Ancestor rename/replacement exclusion, reparse and
leaf-type rejection, and timestamp preservation across reads and listings
were NOT RUN at preflight.

## Windows adapter checkpoint

[`b13c9785`](https://github.com/nakatamaho/vaeg/commit/b13c978503de146982834533e6d2fd5eb4bd57bb)
limits the Windows session option to fixed local NTFS volumes. The backend
checks each component using a root-relative native metadata open and holds
ancestors without delete sharing. Ancestor handles request directory traverse
access because attribute-only opens do not enforce sharing. The backend
rejects reparses and ineligible file types before same-object reopen, then
verifies identity and suppresses access-time updates on the I/O handle before
any read or listing. If Windows denies the `FILE_WRITE_ATTRIBUTES` right
required by its suppression API, the upgrade fails closed; the adapter does
not set a persistent timestamp. The DOS namespace and wire service are
shared with Linux.

Linux HOSTDRV CTests: 15 passed. The standalone Windows fixture cross-compiles
statically with system DLL imports only. The clean static MinGW application
build of the adapter checkpoint passed the encoder archive and PE import
checks. NTFS fixture and Windows guest execution: **NOT RUN**.
It checks ancestor locking and replacement, fresh path identity, special and
reparse rejection, timestamp preservation, and CP932 names. A timestamp pass
with updates disabled is evidence for that system mode only; record the
reported setting and do not infer enabled-mode coverage. The fixture does
not change the system policy and removes only its own entries.

## G173 Windows checklist — pending runtime checks

On an eligible volume, start
`vaeg.exe --hostdrv-dir <folder>`, start MS-DOS 4.0 VA, and run
`HOSTDRV /VAEG`. Check `DIR`, `CD`, `TYPE` and `COPY` to a DOS-local
volume, including Japanese names. Add a file on the host and check that
it appears on a fresh guest directory search without reinstalling.
Windows execution and this checklist are NOT RUN. Run the remaining
normal VA guest warm-reboot route separately. Neither a static build
nor a successful refusal is a G173 acceptance result.
