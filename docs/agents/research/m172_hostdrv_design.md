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

# M172 — HOSTDRV design for MS-DOS 4.0 VA

This is a proposal, not an implemented interface. HOSTFAT remains the
read-only FAT12 snapshot solution for PC-Engine (M54/M55). M56's interface
question was whether PC-Engine forwards the public redirection calls into
MS-DOS-style INT 2Fh. HOSTDRV instead targets the DOS kernel that actually
contains the redirector dispatch. It gives DOS file operations, not sectors:
no BPB, guest partition, DMA controller or FAT emulation is involved.

## Kernel profile and entry conventions

Source references below are relative to `v4.0/src/` in [FreeDOS-88VA/MS-DOS,
release/msdos-va](https://github.com/FreeDOS-88VA/MS-DOS/tree/7a11162ce3433469d0c520ce2470a621c6f69d1b).
The offsets are a profile for this kernel, not an ABI for arbitrary binaries
reporting DOS 4.00. The VA machine I/O is under `v4.0/pc88va/` (including
Shift-JIS console handling in `VAIO.ASM:1256–1258`); the redirector call sites
are in the common kernel. The product must bind a named profile using the DOS
version, layout checks and a maintained source/build compatibility binding;
version alone is not sufficient. Do not scan arbitrary DOS memory for a match.

* INT 21h/AH=52h returns ES:BX at `SYSINITVAR` (`DOS/MISC.ASM:143–155`,
  `DOS/MSCONST.ASM:63–93`). Relative offsets: DPB head `+00h`, SFT chain
  `+04h`, CDS pointer `+16h`, FCB SFT chain `+1Ah`, physical disk table count
  `+20h`, CDS count (`LASTDRIVE`, not number of mounted drives) `+21h`.
* INT 21h/AX=5D06h takes DS:DX at an 11-word DPL; `+12h` UID and `+14h`
  PID are read and set even for this query, so supply a zero DPL for the local
  caller. Returns DS:SI at `SWAP_START`, CX total rounded span, DX length of
  the always-swap prefix. CX includes that prefix; do not add DX again.
  AX=5D0Bh is implemented as a special case (no DPL) and returns DS:SI at
  `SWAP_AREA_TABLE`: count word 2, then two `{far pointer, length word}`
  records, InDOS first, always second (bit 15 on length is a flag, not size).
  The `IFSFUNC` record beyond these two is not part of the returned list.
  See `INC/DPL.ASM:4–14`, `DOS/SRVCALL.ASM:64–128,259–270`,
  `DOS/MS_TABLE.ASM:691–701`, `DOS/MSINIT.ASM:573–595`.
* Base DOS handles 1100h by testing AL and returning AL=0, CF=0; no redirector
  is claimed by DOS itself. Other AH=11h calls to this fallback take the bad
  function path (`DOS/MS_CODE.ASM:480–509`). A resident handler claims 1100h
  with AL=FFh and CF clear, leaving AH unchanged (DOS also tests AL in
  `DOS/FCBIO.ASM:817–830`). Chain unrelated calls via a far jump with untouched
  registers and interrupt frame. For owned calls update CF in the saved
  FLAGS and IRET; errors normally return DOS code in AX. Do not alter a
  caller-pushed argument: it starts at entry `SS:SP+6` and DOS pops it.
  Preserve registers except documented outputs (especially DS on DOSGroup
  paths), and restore DF/IF through the saved frame. `CallInstall` itself
  loads AX and issues INT 2Fh, with optional caller saves, not an extra
  argument (`INC/DOSMAC.INC:541–554`). No recursive public INT 21h from the
  resident interrupt handler.

### Working storage

Hex offsets below are relative to the start of the SDA, derived from the
linked profile; regenerate and independently check them for the implementation
candidate. The start of the second range is `+01Ah`, and the total rounded
span is `078Ch`. `INC/CONST2.ASM:99–139` defines the first fields and the
range boundary; `INC/MS_DATA.ASM:18–46,79–121,179–187` defines the later
fields and end marker. This is the relevant subset, not a memory-copy spec.

| Offset | Field / interpretation |
|---|---|
| `000,001,002,003,004,006,007,008` | ErrorMode, InDOS, WPErr, locus, error word, action, class, far error pointer |
| `00C,010` | `DMAADD` far transfer/search pointer, `CurrentPDB` word (PSP) |
| `01A,01C,01E` | original `USER_IN_AX`, `PROC_ID`, `USER_ID` (words) |
| `09E,11E,19E` | `OPENBUF`, `RenBuf` (128 bytes each), `SEARCHBUF` (53 bytes) |
| `22B,237` | `NAME1`, `NAME2` (12/13 bytes; overlap the temporary device directory entry, not the canonical path) |
| `24C,24D,24E` | extended FCB flag, search attributes, open access |
| `27E,282,286` | `THISSFT`, `THISCDS`, `THISFCB` (far pointers) |
| `292,294,296` | `WFP_START`, `REN_WFP`, `CURR_DIR_END` (DOSGroup offsets; the last can be FFFFh) |
| `2D4,2D6,2D7,2DF,2E1` | extended-open action/flags word, active byte, I/O mode word, saved CX/BX |

Do not blindly dereference DS on every verb. Use the cached DOSGroup/SDA
profile for these globals (e.g. IOCTL sets DS to a device segment); follow
validated far pointers only for an owned call. Bounded-copy canonical path
inputs starting at WFP/REN_WFP rather than guessing the FCB base. DOS's
`DMAADD` is a software pointer, not permission to use a DMA device.

**CDS** (`INC/CURDIR.INC:17–43`, `DOS/MACRO.ASM:218–250,425–442`): 58h
bytes, path text 67 bytes at `00h`, flags `43h` (network/IFS `8000h`, in
use `4000h`, splice `2000h`, local `1000h`), device/DPB pointer `45h`,
network ID dword `49h`, user word `4Dh`, protected prefix boundary `4Fh`,
type `51h`, IFS header pointer `52h`, FS-specific bytes `56h`. DOS multiplies
the zero-based drive by the structure size. Provide a real resident
`IFSHEADER` with `IFSDISK|IFSREMOTE` and without `IFSUNC`/filter/custom share
flags; use `TYPE_NET_DRIVE=4`. Do not substitute a null pointer for the
header: `DOS/IOCTL.ASM:660–675,731–741` dereferences it for CDS and SFT
classification (`INC/IFSSYM.INC:26–43,380–381`). Keep a DOS-style `X:\`
root with boundary 2 as the first candidate; validate DOS's remote path
construction and CDS/DPB-pointer expectations before freezing install
fields. `DOS/PATH.ASM:214–295` updates the CDS path on a successful chdir;
validate the directory, do not independently maintain host working CWD.

**SFT** (`INC/SF.INC:15–19,25–112`): entries are 3Bh bytes after the
6-byte link/count header of each SFT block. Offsets: refs `00h`, mode
`02h` (FCB bit 15), attributes `04h`, flags `05h` (remote bit 15), device
pointer `07h`, serial/first cluster `0Bh`, packed time/date `0Dh/0Fh`, size
and position dwords `11h/15h`, seven FS-specific bytes `19h–1Fh`, padded
11-byte name `20h`, share link/UID/PID/MFT `2Bh/2Fh/31h/33h`, last cluster
`35h`, IFS header `37h`. Store a compact token and owner tag in FS storage;
validate it against an owned-SFT registry (never identify an SFT by the
remote bit alone). On successful open, initialize mode/metadata/flags and
IFS pointer and set `sf_UID` to the SDA `USER_ID` (not the PSP or a host UID;
`DOS/HANDLE.ASM:766–777`). Preserve DOS's busy/refcount transition. For
FCBs preserve the FCB mode bit and DOS's `PROC_ID` bookkeeping in `sf_PID`
(`DOS/OPEN.ASM:337`, `DOS/FCBIO.ASM:867,1005–1009`). Set the **word**
`sf_serial_ID` at `+0Bh` to a fresh nonzero serial for every successful SFT
allocation, including an FCB reopen after eviction. Never recycle a serial
while this DOS boot can retain an FCB copy, including across unload/reload
(use an emulator boot-lifetime allocator; fail open with DOS error 4 at
exhaustion, reset only when the DOS image is discarded). DOS copies and
checks that word, not the seven FS bytes (`DOS/FCBIO.ASM:424–425,1056–1058`).
Initialize both words of `sf_position` to zero on allocation. For FCBs,
thereafter leave `sf_LRU` and `sf_OpenAge` to DOS; do not use them as a saved
byte offset (`INC/SF.INC:59–60,89–90`, `DOS/FCBIO.ASM:505–584`). Reuse a
slot only after its final reference is retired; reseed all redirector-owned
fields, no leftover identity from a previous open.

Each valid owned `1106h` retires exactly one DOS reference even if detached,
transport is unavailable, or the host close fails. Decide ownership locally;
keep the slot record on force detach. Only the transition to zero drops the
local slot/token and attempts a host `CLOSE` if still live. Clear the local
owner/token on that transition, never underflow and never retire an unowned
reference. Host `CLOSE` consumes a valid token even on close error. If the
result is ambiguous, revoke the session (or leave it blocked pending host
revocation/reset if the port is unavailable); do not retry a close. For FCB
close, report error 6 on host failure so eviction can continue its cleanup;
ordinary close can report the mapped failure after retiring the reference.
DOS has already released the JFN and depends on this cleanup
(`DOS/HANDLE.ASM:131–152`, `DOS/CLOSE.ASM:54–90`); FCB eviction loops on
success and error 6 (`DOS/FCBIO.ASM:762–778`).

### Calls issued by this kernel

Numbers below are **hex**; many source operands are decimal. Inventory:
`multNET`/`MultNet` and `CallInstall` active sites, with `Installed=TRUE`
(`INC/DOSSYM.INC:4–7`, `INC/MULT.INC:83`). Path calls use THISCDS and WFP
unless specified; handle calls use the owned SFT (ES:DI where passed, or
THISSFT). Unowned calls always chain. Read-only disposition applies only
to owned calls.

| AX | Call site / when | Inputs, outputs and disposition |
|---|---|---|
| 1100 | `FCBIO.ASM:817–830`, FCB network check | AL=FFh installed; see above. |
| 1101, 1103, 1105 | `DIRCALL.ASM:83–90,210–246,309–317` rmdir, mkdir, chdir | Canonical path, THISCDS, CURR_DIR_END. Deny mutation; chdir validates directory with no cluster/ID assignment. |
| 1106, 1107 | `CLOSE.ASM:54–90,349–366` close, commit | ES:DI SFT. Close retires refs even on failure; commit leaves refs unchanged. Live read-only commit succeeds without host write; detached commit returns not ready. |
| 1108, 1109 | `DISK.ASM:124–192,422–502` read/write | ES:DI, DMAADD, CX count; return CX bytes, update SFT position (size also on write), ES:DI preserved as SFT; error CX=0 if no progress. Write CX=0 means truncate/extend at position, never a read-only no-op. |
| 110A | `LOCK.ASM:86–132,184–210` DOS 5Ch | ES:DI, BX original AX (BL=0 lock, 1 unlock), DS:DX to one pair of dwords (start,length), CX=1. Reject until locking implemented; the extended branches shown in comments are inactive. |
| 110C | `DINFO.ASM:41–68` capacity | THISCDS. Return AL sectors/unit, AH media ID, BX total units, CX sector bytes, DX free units. INT 21h/36h repacks these; do not expose host sector or partition fields. |
| 110E, 110F | `FINFO.ASM:47–100,203–215` set/get attributes/info | Set takes pushed attribute word (use low byte); deny. Get outputs AX attr, CX time, DX date, BX:DI high:low size. |
| 1111, 1113 | `RENAME.ASM:136–141`, `DELETE.ASM:138–143` | Rename uses WFP and REN_WFP, delete WFP; consult search/name state for wildcard semantics. Deny before touching host, including wildcard requests. |
| 1116 | `OPEN.ASM:63–173` open existing (direct or CDS) | Pushed access/sharing word, THISSFT preseeded with mode incl. FCB bit, SATTRIB. Initialize SFT without changing refs. Direct/UNC (`THISCDS.offset=FFFFh`) is unowned. |
| 1117, 1118 | `CREATE.ASM:114–162` create/truncate via CDS, direct create | Pushed create attribute word; SFT/path and DOS create state. Deny owned create; no UNC provider. |
| 1119, 111B | `ISEARCH.ASM:53–115` direct and CDS find-first | WFP, SATTRIB, CURR_DIR_END, DMAADD. CDS only: write 53 bytes: 21 private bytes starting with remote-marked drive byte, then 32-byte directory entry. |
| 111C | `ISEARCH.ASM:241–271`, `SEARCH.ASM:82–86,182–186` find-next | DMAADD points to copied search state; select by drive byte and owned cookie, not THISCDS or DTA address; update 53-byte result. No match/end AX=18 decimal. |
| 111D | `ABORT.ASM:54–96` process end | DOS closes JFNs first; DOSGroup process identity is available. Reap owned searches and reconcile FCB cache refs, never close an inherited/live JFT solely on PSP exit. Chain the broadcast. |
| 111E | `MACRO.ASM:103–178` assignment/redirector control | Pushed original public AX, original request registers (including DS:SI and ES:DI buffers where applicable); DOS has CDS lookup context on drive operations. No general network assign/enumerate API in v1; chain. |
| 111F | `MACRO.ASM:271–328` printer strings | Pushed public AX, caller buffers/registers. Chain; no printer provider. |
| 1120 | `MISC.ASM:247` DOS disk reset | Global flush, no owned target. Read-only no-op for HOSTDRV, chain. Write phase must define synchronization/error reporting separately. |
| 1121 | `HANDLE.ASM:502–539` seek from EOF on selected sharing modes | ES:DI, signed CX:DX displacement. Return position in DX:AX for DOS to store; other seeks do not go through us. Refresh size where possible; document cached EOF for paths that DOS handles locally. |
| 1122 | `CTRLC.ASM:633–652` process reset | Global redirector environment cleanup, despite legacy comment. No HOSTDRV global state to reset here; chain. |
| 1123, 1124, 1125, 1126 | `MACRO2.ASM:357–367`, `CTRLC.ASM:226–229`, `SRVCALL.ASM:272–277` | Spool name check **before** canonicalization (original DS:SI name, ES:DI output buffer; `MACRO2.ASM:313–327,357–408`), spool close, spool control (AX/BX saved by caller, DOS server spool call context), echo check. Chain, never mark HOSTDRV an SFT spool device. |
| 112B | `IOCTL.ASM:306–313,395–443` IFS IOCTL (including device control-string path) | SI:DX becomes DS:DX on dispatch; not a DS:SI device pointer. Preserve caller context when chaining; if explicitly owned, return invalid function. |
| 112D | `HANDLE.ASM:580–623` extended attributes by handle | ES:DI SFT, BL=XA type, original DOS call context. Return invalid function; no host extended attributes. Ordinary time query uses SFT fields (not this verb). |
| 112E | `OPEN.ASM:129–139,163–175`, `CREATE.ASM:115–124,151–152` extended open | Pushed create attrs, THISSFT/mode, WFP, EXTOPEN_FLAG (action input and result). See below. |
| 112F | `IFS.ASM:490–516` IFS control | Pushed public AX and pass-through request registers. No HOSTDRV IFS-control provider, chain unless explicitly addressed, then invalid function. |

No active DOS call sites found for the gaps in this table; reserved or
redirector-private verbs from other DOS versions are not obligations of
this kernel. A caller-specific register/ownership trace with synthetic DOS
programs is required in the read-only milestone, especially for FCB refcounts,
remote CDS shape, enumeration ownership and the rarely reached IFS paths.

Ordinary `5701h` sets packed times and dirties the SFT locally, without
issuing an AH=11h verb (`DOS/HANDLE.ASM:664–696`). DOS may report success
and a later `5700h` may read the updated SFT; read-only `1107h` and `1106h`
never propagate it to the host.

DOS 4 additions versus a 3.x redirector profile: `AX=6C00h` saves the
extended-open mode and action, passes creation attributes on the stack and
uses `EXTOPEN_FLAG` for the action returned in user CX (1 opened, 2 created,
3 replaced; `DOS/FILE.ASM:668–755,785–831`). Snapshot input before overwriting
it; read-only permits only an existing-file open with read access, returning
1. If the request would create/replace, deny it without side effects; when
no operation is requested for a missing/existing file, return the normal
file-not-found/file-exists result. Extended IFS query/EA/control paths do not
imply permission to forward arbitrary host IOCTLs. The DOS 4 long BPB fields
and widened sector bookkeeping support local volumes over 32 MiB
(`INC/BPB.INC:21–37`, `INC/MS_DATA.ASM:73–98`); the redirected capacity ABI
still uses 16-bit counts and SFT size/position are still 32-bit. Do not
wrap oversized files or invent FAT sector addresses. Validate FCB reads and
close/abort recovery separately from 6C00h and ordinary handle operations.

## Proposed wire protocol v1

The existing `io/np2sysp.c:139–207` hardware interface uses a rolling 07EDh
value and suffix matching on the 07EFh command stream (dispatch on last
character, not on NUL, despite the introductory comment). Its 07EDh reads
rotate, not consume, a dword. HOSTFAT (`check_hostfat`, `read_hostfat1`)
and SASI/SCSI use the same transport; none of their commands or replies
change. For HOSTDRV reserve the non-overlapping suffixes `hostdrv?1` and
`hostdrv!1`, check for existing-command substring conflicts, and reject
execution outside VA native mode. Send an initial zero to delimit the
request from prior traffic; don't rely on a trailing zero for dispatch.

* Emulator-only discovery: require explicit `HOSTDRV /VAEG` on the
  transient install command, used only in a VAEG guest started for HOSTDRV.
  No automatic probe or default startup invocation. Without that switch
  HOSTDRV does not touch either port. The switch is the operator's assertion
  that this is the emulator, not hardware detection. **Real-hardware safety
  is unproven** (including expansion boards); never use the switch on real
  hardware. After opt-in, send `NP2`, verify its exact bounded reply including
  NUL, then query `hostdrv?1` and require `HDR1\0`. This is capability
  discovery, not authentication. `QUERY` provides flags and generation;
  attachment still requires an explicitly enabled host mapping.
* Exclusive transaction: hold the TSR's non-reentrant guard and save FLAGS
  and mask guest IRQs from before the first port access through the last
  reply byte and response validation; restore original IF on all exits.
  Apply this to discovery and every packet. Synchronous foreground/BIOS
  traffic (including SASI/SCSI/IDE and HOSTFAT) cannot run concurrently on
  the same guest CPU without a callback, which is forbidden here. Only
  enable the transport in a VAEG profile that excludes other port masters
  and NMI/debugger/host-injected access during that window; gate on that
  execution guarantee, not on a claim that no other software uses the ports.
  Host UI and diagnostics cannot inject traffic mid-transaction. If the
  profile cannot guarantee exclusivity or the local guard is busy, do not
  touch the ports or spin: refuse installation or fail the owned operation
  (DOS 31); still retire DOS references on close.
* Execution: write `offset low, offset high, segment low, segment high`
  to 07EDh and send `hostdrv!1` to 07EFh. Read **four** reply bytes from
  07EDh: `4852xxxxh`, low word transport code (`0` completed, `1` invalid
  guest range, `2` malformed/unsupported packet, `3` disabled, `4` busy).
  The host replaces the entire dword on every command, even on failure.
  On code 0 require the response-only `HDS1` marker and echoed request
  sequence, version and opcode before accepting any result, even EOF or
  empty success. Rebuild the request with `HDR1` and clear output fields
  before every dispatch. An old rotating port dword cannot authenticate an
  untouched request. Validate generation and all verb-specific outputs;
  stale or inconsistent replies are DOS 31, not successes. Stage both
  packets, publish the complete response to guest RAM before publishing
  completed transport, and never retry an ambiguous state-changing verb.
  If resource lifetimes may have diverged (including a lost find-first
  reply), block further service and require host revocation or a full
  reset; DOS-side cleanup remains available.
* One guest-owned bounce buffer: 64-byte header followed by up to 4096
  bytes (4160 total), entirely within a single non-wrapping segment in
  ordinary writable conventional RAM. Host requires linear bounds and
  RAM-only mapping for the whole span (no IVT/BDA including VA low work
  area, banked/device windows, ROM or A20 aliases); prove the VA-specific
  RAM predicate before use. No embedded pointers. No DMA or host callbacks
  into DOS; guest copies chunks to/from the DOS transfer area and validates
  its 16-bit segment arithmetic. Host sees neither SDA nor SFT addresses.

All multibyte fields little-endian; offsets and sizes in bytes. Request
sets unused, output-only and reserved fields to zero. Responses echo input
fields unless the opcode specifies a replacement. Transport errors do not
write guest memory. On completed DOS failure, return the DOS status and
zero all output-only fields, with in/out fields unchanged (no new token).
Only bytes covered by the response payload length may be consumed; ignore
other payload bytes. No native C layout is serialized.

| Offset | Size | Meaning |
|---|---:|---|
| 0 | 4 | ASCII `HDR1` request, `HDS1` response (including DOS errors) |
| 4, 5 | 1 each | version 1, opcode |
| 6 | 2 | sequence, echoed, increment modulo 65536 per packet (synchronous, one buffer, no asynchronous replies) |
| 8 | 4 | mount generation (in/out, zero only for QUERY) |
| 12 | 4 | opaque file or search token (in/out, zero when not applicable) |
| 16 | 4 | absolute file offset or search ordinal (in) |
| 20 | 2 | requested bytes (READ/WRITE only, maximum 4096) |
| 22, 24 | 2 each | first/second path byte lengths (max 127 each) |
| 26, 28 | 2 each | request payload length; max response payload (each at most 4096) |
| 30 | 2 | mode / verb options (in, zero on unused bits) |
| 32 | 2 | DOS status (out, zero on success) |
| 34, 36 | 2 each | actual data count and response payload length (out) |
| 38 | 2 | capabilities (out: bit 0 read-only; bit 1 write verbs supported) |
| 40 | 4 | size (out) |
| 44, 46, 48 | 2 each | DOS attributes, packed time, packed date (in/out by verb) |
| 50 | 2 | DOS SFT serial (out, OPEN only) |
| 52–63 | 12 | reserved, zero |
| 64–4159 | variable | request path 1 then path 2 then data, no NUL; overwritten by response payload on success |

Only canonical relative DOS path components go on the wire, separated by
standalone `5Ch`. Strip the owned drive/assignment prefix in DOS; no UNC
or host paths. Set second-path length to zero for all read-only verbs.
Request payload length includes both paths and any data (not the header).
The response capacity must equal the table entry, even when returning less.
All lengths are exact; fields not named below must be zero. `M` means size,
attributes (`0037h` mask), packed time and date; directory size is zero.
All opcodes are hex. Only `QUERY` and `ATTACH` are legal before attach (the teardown acknowledgement
below is legal only for a previously attached installation).
`QUERY` always has request generation zero; other requests need the
current nonzero generation except `UNREGISTER`, as specified below. On enabled `QUERY`, return that generation
and capabilities (`0001h` in read-only v1); no enabled mapping returns
DOS 21. `ATTACH` uses the queried generation and does not change it on
success; a prior installation (including detached-but-resident) makes it
transport-busy. A stale generation or missing session returns DOS 21 on
session verbs; a bad or wrong-kind file token in a live session returns 6.
Owned stale searches map DOS 21 to DOS 18 at the redirector boundary.

| Opcode | Request fields beyond common header | Request length / response capacity | Success outputs beyond status |
|---|---|---|---|
| 00 QUERY | none | 0 / 0 | generation, capabilities |
| 01 STAT | path 1 (0–127 bytes; empty selects the root) | path length / 0 | M; empty path returns root directory metadata, never a file token |
| 02 SPACE | none | 0 / 16 | 16 bytes: total and caller-available bytes as LE u64s, available zero |
| 03 OPEN | path 1 (1–127 bytes), options | path length / 0 | file token, M and fresh nonzero DOS SFT serial; regular files only |
| 04 READ | file token, offset ≤ `7FFFFFFFh`, count 1–4096 | 0 / count | actual data count and same payload length, at most requested count; zero only at EOF |
| 05 CLOSE | file token | 0 / 0 | none; consumes the valid token even on DOS close error |
| 06 FIND_FIRST | path 1 (0–127 bytes), 11-byte padded wildcard template as data, options = DOS search attributes (`003Fh`) | path length + 11 / 11 | search token, M and 11-byte name payload at ordinal 0 |
| 07 FIND_NEXT | search token, ordinal 1–4096 | 0 / 11 | M and 11-byte name payload for that ordinal |
| 08 FSTAT | file token | 0 / 0 | M for the opened object |
| 09 FIND_RELEASE | search token | 0 / 0 | none; drop snapshot (already expired = no more files) |
| 0A ATTACH | none | 0 / 0 | none; acquire session and set resident-state guard |
| 0B DETACH | none | 0 / 0 | none; only after files/searches drained; invalidate generation, keep guard until guest teardown is finalized |
| 0C UNREGISTER | former installation generation, not current mount generation | 0 / 0 | none; only in detached-but-resident state, acknowledged by transient after completed guest teardown; clear guard |

`OPEN` options use mask `0077h`: access (bits 0–2) 0 read, 1 write,
2 read/write; share (bits 4–6) 0 compatibility, 1 deny all, 2 deny write,
3 deny read, 4 deny none. Other values are invalid. Read-only rejects
access 1 and 2; guest normalizes an FCB compatibility open to effective
read access before marshalling. Inheritance and FCB bookkeeping stay in
DOS. `FIND_FIRST` never matches volume labels; after masking out read-only
and archive, `08h` alone means labels only (no match). Otherwise hidden,
system and directory entries require the corresponding bit; read-only/archive
do not filter ordinary files (`DOS/DIR.ASM:160–191`,
`DOS/DIR2.ASM:794–812`). Find data count stays zero (not a file read).
`10h`–`19h` reserve `WRITE`, `SET_LENGTH`, `FLUSH`, `CREATE/OPEN_ACTION`,
`MKDIR`, `RMDIR`, `UNLINK`, `RENAME`, `SET_ATTR`, `SET_TIME` respectively;
reject these and all other unassigned opcodes in the read-only contract.

Search snapshots are immutable lists of metadata and names, sorted by DOS
name bytes. New searches and path lookups see live changes. The guest
constructs the 53-byte DOS result: in the private 21 bytes put `80h |`
zero-based drive at byte 0, ASCII `HD` at 1–2, LE generation at 3–6,
LE search token at 7–10, LE next ordinal at 11–12, and zero at 13–20;
then the 32-byte directory entry. DOS routes `111Ch` on the high bit alone
(`DOS/ISEARCH.ASM:262–270`); claim it only for that drive/signature and
validate cookie against current generation and a resident token/owner PSP
record. Use the cookie ordinal, not DTA address or a host cursor; advance
only after success. No match at first allocates nothing; absent ordinal,
expired/evicted token or detached owned cookie returns DOS 18. EOF does
not release or change the cookie: earlier copies still work until release.
Bound searches to 32 snapshots, 4096 raw entries per scan and 8 MiB total
snapshot storage (budget up to 64 bytes per result including overhead;
bound staging separately to one scan). Reject oversize scans. At the cap,
on a successful new search evict the oldest snapshot by creation order
and its guest owner record, applying the same fixed cap and ordering at
both ends. An ordinary EOF never removes the guest record. Release by owner on DOS
termination; successful uninstall explicitly drains all searches including
copies kept by a long-lived shell. Search cookies never block uninstall.
Use 64 file handles; error 4 on exhaustion, no live file eviction. Tokens
are monotonic nonzero, typed within the current generation and never reused
within it. Change generation on detach/reset; fail closed on counter wrap.
Zero-length DOS reads return locally; never issue a zero-count `READ`.

Handle SFTs own logical position; host uses positioned I/O. FCB transfers
use DOS's per-operation position, not a persistent byte offset in the FCB
cache entry: if DOS temporarily overlays position during record I/O, use
and advance that transfer value only, preserving DOS's save/restore of the
cache age words. Prove both FCB paths with caller traces before enabling. For DOS requests
larger than 4096, chunk, validate each count, and advance only by actual
bytes; stop at EOF or first error. A read with progress returns short success;
report a persistent error on the next operation, not a fabricated EOF.
Write phase must distinguish short success/disk-full and failure before any
write; transport failure after a mutation is ambiguous and must not be
retried automatically. Add bounded guest share-mode tracking keyed on actual
file identity in the host service, with explicit handling of DOS compatibility
and the FCB open-for-both convention; if FCB needs an effective read-only
mode, prove mode/refcount/cache semantics before enabling support. Host-side
operation permissions always override guest access requests.

Core error mapping (decimal DOS codes, see `INC/ERROR.INC`): 1 unsupported,
2 absent final file, 3 absent/invalid path prefix, 4 handle table full,
5 denied policy/rights or read-only access/mutations, 6 invalid file handle,
12 bad access mode, 17 cross-device, 18 no further match, 21 detached/not
ready, 31 general I/O/transport failure, 32 sharing conflict, 33 lock
conflict, 39 handle disk full when the DOS 4.0 write path can report it,
80 existing target for exclusive create. Adapt errors at the DOS verb boundary where
DOS expects a different code/short count; never copy errno or Win32 values
out. Verify both immediate and extended DOS error results. No invented
INT 24 dialog or retry on a malformed packet.

## Host policy

* Add a separate SDL2 folder picker, mount status and detach action, following
  HOSTFAT settings style. Store the root path locally, never in guest data,
  portable state, captures or telemetry. Default disabled; path persistence
  alone must not regrant access. Read-only host policy is immutable per mount.
  Require explicit writable opt-in and confirmation on each new writable
  session. No startup autorun or implicit directory choice. A guest drive
  letter is a separate DOS decision.
* Carry forward M55's explicit selected root, fail-closed containment and
  ordinary-file/directory eligibility. Do not inherit its snapshot identity,
  FAT/name limits, state rebind policy, pathname checks as authorization, or
  permissive cases for contained Windows reparses
  (`sdl2/hostfat_snapshot.cpp:539–569,1230–1237`). Reject all reparses,
  links, junctions, cross-mount traversal, alternate streams, special files,
  network/UNC exports, and regular files whose link count is not one at
  authorization. The grant is to an opened root object: renaming its host
  pathname does not retarget the grant. Existing file handles and immutable
  search results retain authority acquired inside the root; they cannot
  authorize new descendants. Every fresh directory/file authorization
  resolves from the root, not a previously pinned child that can move out.
  A new open of a path displaced outside the root must fail or resolve a
  different in-root object and then pass namespace identity checks.
  Privileged changes to kernel/filesystem semantics are not in scope.
  - **Linux:** use one root-relative `openat2` for the full path with
    `RESOLVE_BENEATH | RESOLVE_NO_SYMLINKS | RESOLVE_NO_XDEV` and
    `O_PATH | O_NOFOLLOW | O_CLOEXEC`. Deny ambiguous resolution including
    rename failures (no weaker retry). `fstat` the metadata descriptor and
    reject special files before opening for data or listing, including any
    symlink returned as the final object. Upgrade only that pinned object
    through a verified local procfs fd reference, not the original name;
    verify matching type, device, inode and regular-file link count again.
    Use `O_NONBLOCK` as defense in depth, not as a type check. Authorize
    a listing when its directory is opened; for each publishable result
    authorize metadata via a fresh root-relative lookup and confirm parent
    and entry identity, not via a child-relative open from the enumeration
    descriptor. Reject a scan whose directory cannot be revalidated. No
    portable `openat` walk fallback on a mutable directory graph.
  - **Windows:** use handle-relative `NtCreateFile`, one component at a
    time from the root, refusing reparses (`OBJ_DONT_REPARSE` and
    final-component reparse inspection). Keep each ancestor open without
    delete sharing through the final authorization, on a local filesystem
    verified to prevent both ancestor rename and replacement while held.
    If locking an ancestor fails, refuse the lookup. Open leaf metadata
    first (attribute access, no data rights); check disk type, reparse
    attributes/tag, volume, file identity, link count and directory versus
    ordinary-file type before obtaining any data handle. Use a same-object
    reopen (e.g. supported `ReOpenFile`) and verify identity/type again;
    never reopen through a stale absolute path. Disable the backend on
    providers where these guarantees cannot be established.
  - **macOS:** no plain `openat` walk on a mutable subtree: `O_NOFOLLOW`
    cannot prevent an ancestor from being moved out. Candidate restricted
    backend: root must be the top of a verified dedicated local filesystem,
    with no filesystem crossings (verify mount identity at every component),
    so a rename cannot move a pinned parent outside the root. Use
    descriptor-relative `openat` and no-follow checks throughout. For a leaf,
    require a non-data type probe and a proven same-object upgrade or a
    filesystem-enforced barrier against replacement between the no-follow
    type probe and `O_NOFOLLOW | O_NONBLOCK` data open. If that guarantee
    is unavailable, refuse the backend rather than substitute a precheck
    plus racy data open. Share POSIX code only where the guarantees match.
  - Fail closed if required APIs, trustworthy identity, local filesystem
    behavior, or no-atime guarantees are unavailable. For writes, decide
    separately how to exclude concurrent directory moves and hard-link
    creation through each mutation; read-only authorization at open time
    does not establish that a later write is still authorized.
* Reject `.`/`..`, empty interior components, absolute paths, embedded NUL,
  DOS/Windows device basenames, illegal standalone separators or colons, and
  trailing-dot/space ambiguity. Split Shift-JIS at character boundaries:
  a `5Ch` trail byte is not a slash. Wire encoding is VA Shift-JIS (DOS
  Japan code page 932), with lead bytes `81h–9Fh`, `E0h–FCh` and mapped
  trails `40h–7Eh`, `80h–FCh` (`DOS/DOSMES.ASM:250–257`,
  `DEV/COUNTRY/MKCNTRY.ASM:2320–2325`). Use a pinned CP932 mapping table,
  reject malformed/unmapped sequences and require exact round-trip for
  native spellings (no best-fit mappings). Fold ASCII `a`–`z` to uppercase;
  apply the VA `FILE_UCASE_TAB` (`SJIS_UCASE`, `DOS/DOSMES.ASM:119–135,157–162`)
  to high single bytes, and leave DBCS pairs byte-for-byte unchanged.
  Reject incompatible DOS NLS/DBCS profiles rather than change keys mid-mount.
  Match the padded 8+3 byte fields using `MetaCompare` semantics
  (`DOS/DIR.ASM:200–266`): literal folded bytes, `?` matches one single-byte
  position including padding, and a DBCS pair matches only the same pair
  or two `?` positions. DOS expands `*` before marshalling; no `*` on the
  wire and no host glob. Keep `E5h` canonical on the wire; escape the first
  directory-entry byte as `05h` at the DOS directory-entry boundary only.
  Convert host names strictly: Win32 UTF-16, macOS actual spelling and
  normalization, and Linux valid UTF-8 bytes. Hide invalid host encodings;
  alias valid Unicode names that cannot round trip. Use the real directory
  entry, never reconstruct a host lookup from a display normalization.
* Reserve all unambiguous round-trippable DOS 8.3 names initially, counting
  encoded bytes, not characters. Resolve fold collisions by a fixed order of
  raw Unicode spelling bytes; only the first eligible name keeps the native
  spelling. Assign the rest `PREFIX~n.EXT`, sanitizing ASCII uppercase prefix
  and extension, shortening prefix as decimal `n` grows, with a stable
  fallback prefix, rejecting DOS reserved names and checking the unified
  namespace. Sort by raw spelling encoded as UTF-8, with no Unicode
  normalization. An issued entry identity binds the pinned parent object,
  exact raw basename and pinned child object (volume/device plus file ID).
  Hold metadata identity references for issued names until retired so an
  ID reused after deletion cannot masquerade as the same object. Revalidate
  both parent and child on a fresh lookup. Replacement at the same raw
  spelling is a new identity; tombstone the old DOS name and give the new
  entry an unused spelling, even if it would normally keep its native name.
  Preserve an issued name through a case-only rename only if in the same
  parent, the child pin still matches, the spelling has the same DOS folded
  key (fold the raw name before aliasing), and association is unique. Otherwise
  treat it as removal plus insertion, including other renames. For names
  that cannot be DOS-folded, do not infer case-only continuity. Never
  resurrect an observed tombstone within the session. Bound alias, identity
  pin and tombstone storage; fail explicitly on exhaustion, not by reusing
  a spelling or relaxing identity checks. Fresh mounts of the same contents
  have the same initial mapping, not a promise of continuity across mounts.
  DOS-created names must be legal, round-trip and not shadow existing
  entries. Write phase must test case-only renames and alias races again.
* Use host mtime in local civil time, DOS 1980–2107 range clamped with
  two-second truncation; deterministic configured timezone for tests.
  Read-only mode never sets host times, including on `5701h`, commit or
  close. Require verified no-atime behavior for both reads and listings
  (e.g. authorized `O_NOATIME` on Linux or verified filesystem/volume
  policy), not a compensating time write. Do not follow links for metadata.
  Map directory, hidden (Windows flag or
  POSIX dot name), read-only (whole mount or file/access policy), and archive
  (Windows flag or deterministic POSIX default for regular files), system
  only where supported. No synthetic NT EA, ACL or executable bit mapping;
  no volume-label entry in v1. Reject unrepresentable file sizes rather than
  report a truncated size (start with signed 31-bit cap, expand only after
  validating DOS seek/arithmetic). Compute capacity using `fstatvfs` on
  POSIX or validated volume and quota-aware `GetDiskFreeSpaceExW` on Windows;
  floor available bytes into units and cap free ≤ total ≤ 65535. Use 512-byte
  sectors and the smallest power-of-two sectors/unit up to 64 that permits
  the result; saturate at that geometry beyond its range. Check guest 36h and
  COMMAND arithmetic at the cap, and reduce cap if needed. Read-only free=0.
* Serialize execution on the emulator thread; UI queues mount changes to a
  safe point between commands. Host handles stay with the opened object across
  pathname changes; fresh opens revalidate resolved current identities. No
  UI lock held during file I/O. Restrict v1 to local storage, bound request
  sizes, scans, and memory; slow host I/O can still stall the emulator. Never
  promise hard timeouts for synchronous filesystem calls. Reject nested entry
  without spinning and never leave an async response aimed at stale RAM.

### Resident-state and reset policy

| State | Host and DOS state | Save/load and reset |
|---|---|---|
| Installed | Session attached; guest TSR, CDS and possibly SFTs live | Reject save and load, even with zero open files. Any reset revokes host session and tokens. |
| Detached-but-resident | No host handles or searches, but TSR/CDS/SFTs may remain (also covers ambiguous install/teardown) | Same save/load guard. Owned I/O fails not ready, searches end, local closes still retire refs. No reattach into this TSR. |
| Uninstalled | Neither resident guest state nor an active session | Ordinary save/load allowed; no mapping authority serialized or restored. Reset keeps this state. |

Set the guard at `ATTACH`, before hooking/publishing the CDS. Installation
rollback also requires detach and confirmed teardown; ambiguity keeps the
barrier. On force detach or reset, close host handles, discard searches and
aliases, revoke the generation and block reuse. `machine/pccore.c:944–948`
(`CPU_RESETREQ` → `CPU_SHUT()`) must revoke but cannot clear the guard:
it is not the `hardwarereset` → `pccore_reset()` path at `:1020–1024`.
The latter may clear it only after discarding the guest DOS image and all
owned references. Treat guest warm/bootstrap paths (including software
reboot that bypasses machine reset) as revocation events before the new
boot consumes the channel; identify and exercise their hooks in the DOS
profile. If guest image disposal cannot be proved, retain the guard and
require a full machine reset for recovery. Do not assume every guest
restart reaches `pccore_reset()`. Quit releases all host state.

Apply the same guard to every save/load entry before output or live state
changes. Supported saves carry the uninstalled policy and DOS-boot SFT
serial and generation high-water marks (not authorization), but no live
session, root or token. Merge the marks by maximum on load, never roll
back into stale FCB or DTA copies in saved guest RAM. Reject active/resident
or unknown HOSTDRV state; never reopen or replay host state. Loading an
uninstalled save revokes pending authority; manual host enable and guest
install are required again.

## Guest lifecycle and acceptance

At install, require `/VAEG` and the exclusive transport profile, then
check channel, DOS profile, DOS not busy, valid swap/CDS/SFT bounds, free
drive slot below LASTDRIVE and no existing redirector (1100h AL must be
zero in first release); reject duplicates via a distinct private TSR
multiplex, not AH=11h. Default to highest truly unused slot above physical
and default drive; accept explicit override only for a free entry, no CDS
expansion. Save the exact original CDS and old vector. Allocate the resident
block, free the transient environment, and attach just before publishing the CDS
and hook. Failure rolls back both and releases the session before DOS
resumes. Retain only required paragraphs.
Target ≤12 KiB including 4160-byte bounce buffer, stack and tables. DOS
serializes normal calls, but INT 2Fh can be nested; use a private stack for
owned work after a guarded handoff, not a large stack allocation inside DOS.
Chain all unowned calls with the original stack/flags. No timer, keyboard,
console or FEP hooks. Without opt-in or valid discovery, exit non-resident
without changing vectors or CDS; a missing opt-in makes no port access.

Uninstall from a transient command only if top of the INT 2Fh chain, not
busy, no owned live file/FCB refs, not current default drive, and no
SUBST/JOIN/dependent CDS references. Verify the slot is still ours before
starting teardown; refuse if preconditions fail. Drain search snapshots
explicitly, then `DETACH` if still installed. With host authority revoked,
restore the original CDS and vector, free resident memory, and send
`UNREGISTER` with the former installation generation from a transient
bounce buffer. Do not clear the guard if any cleanup step cannot be proved;
a full machine reset is recovery. If revocation is ambiguous, stop before
unhooking and recover by full machine reset. Force detach alone is not an
uninstall. Process exit releases search ownership and reconciles owned FCB
refs through the same DOS close semantics; never drop a live inherited JFT
reference merely because its original PSP exits. Refuse using an existing
SUBST/JOIN drive or redirecting a drive they depend on; prove DOS rejects
new mappings against the remote drive or document the exact supported subset
before release. Check DOS COMMAND `DIR`, `CD`, `TYPE`, `COPY` to a local
volume, child execution and redirection, plus JFP/FEP entry, switching and
Shift-JIS trail-byte names without any change to console behavior. FCBs,
including find-first/next and process termination, are a distinct gate and
not inferred from handle tests.

1. **G172 (design):** review the guest profile, backing-root threat model,
   proposed wire contract and split; accept or amend explicitly. The
   nonresident probe covers discovery of kernel structures, not TSR operation.
2. **Read-only milestone:** prove the DOS CDS/SFT ABI and transport first
   with a small synthetic fixture, before folder UI and platform rollout.
   Trace handled and chained verbs; test unspecified opcode/masks/lengths,
   undispatched command with a stale success dword, wrong response marker
   and sequence, and busy/exclusivity refusal with no port I/O. Then test
   live additions/removals, copied DTA at EOF and interleaved searches,
   Japanese/collision/identity-replacement fixtures, reads, both seek paths,
   EXT open, FCBs per G172, JFP/FEP, drive conflict/uninstall, LASTDRIVE
   exhaustion, actual SUBST/JOIN utilities, and missing discovery. Exercise
   final and nonfinal close after force detach, process exit with an
   inherited handle, FCB eviction after a stale close, and slot reuse.
   Cover both reset paths and guest warm reboot, installed/detached
   save/load rejection, and uninstalled save/load without regrant. For
   each enabled backend test quotas, stale tokens, invalid guest ranges,
   link/reparse/special-file rejection and a parent moved before leaf open.
   Prove no host writes (including times) despite mutating DOS requests;
   confirm the DOS-local `5701h` behavior separately. Require the normal
   clean-build handoff and human acceptance.
3. **Write milestone:** separate opt-in and review, with anchored mutation
   APIs, identity/share/lock rules, guarded replacement and explicit partial
   write, truncate, flush and crash semantics. Test exclusive create,
   mkdir/rmdir, rename/delete, attr/time capability limits, zero-count
   writes, disk-full, concurrent removal/replacement/links, and repeated
   read-only regressions. Initially reject unsupported wildcard mutations
   until DOS semantics and safe effect boundaries are demonstrated; never
   fall back to a host glob or shell. Require write-specific acceptance and
   per-platform evidence before enabling each backend. No promise of host
   rollback, external-process locking, or portable active states.

## Maintainer decisions for G172

Maintainer decision (2026-11-27): (a)–(d) approved as recommended below.

| Decision | Recommendation |
|---|---|
| (a) FCB support in read-only phase | Yes, minimal: open/read/close, find-first/next and cache/exit cleanup. Gate on distinct FCB traces; no write FCB verbs. |
| (b) Read-only host backends | Linux and Windows first. Include macOS in the same milestone only if the POSIX implementation can share the required containment/type-before-data guarantees; otherwise defer macOS and leave it disabled. |
| (c) First implementation proof | Prove CDS/SFT and packet transport with the synthetic fixture before UI work or broad platform enablement. |
| (d) Drive exhaustion and dependency cases | Include LASTDRIVE exhausted (no drive, hook or attach side effects) and actual SUBST and JOIN utilities (reject using the remote drive or document a demonstrated safe subset); their network-drive paths are `CMD/SUBST/SUBST.C:422–438` and `CMD/JOIN/JOIN.C:371–381`. |

Deferred to the write-phase gate: decide whether `5701h` persistence is in
scope. If yes, define how commit/last close detect DOS's dirty SFT times,
convert and persist them through an authorized object, and report failures
without leaking DOS references. Otherwise explicitly exclude host timestamp
updates; never imply that DOS-local success persists them.
