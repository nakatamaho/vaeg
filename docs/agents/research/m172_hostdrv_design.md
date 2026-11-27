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
remote bit alone). Initialize mode/metadata/flags and IFS pointer on open;
leave the busy/refcount transition to the open caller, but implement remote
close refcount handling and release on the last close (`DOS/OPEN.ASM:89–109`,
`DOS/CLOSE.ASM:54–90`). FCB cache uses SFT position differently and copies
network identity into the FCB (`DOS/FCBIO.ASM:658–753,1016–1060`); do not
assume each FCB operation is an independent JFT handle or that the full
seven-byte token makes the FCB round trip.

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
| 1106, 1107 | `CLOSE.ASM:54–90,349–366` close, commit | ES:DI SFT. Close decrements refs; commit leaves refs unchanged. Read-only commit succeeds without host write. |
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
| 1123, 1124, 1125, 1126 | `MACRO2.ASM:357–367`, `CTRLC.ASM:226–229`, `SRVCALL.ASM:272–277` | Spool name check (canonical name context), spool close, spool control (AX/BX saved by caller, DOS server spool call context), echo check. Chain, never mark HOSTDRV an SFT spool device. |
| 112B | `IOCTL.ASM:400–443` generic local-IFS IOCTL | DS:SI device context, ES:DI from caller, original AX/440Dh pushed. Remote IFS generally rejected before dispatch; if owned, return invalid function. |
| 112D | `HANDLE.ASM:580–623` extended attributes by handle | ES:DI SFT, BL=XA type, original DOS call context. Return invalid function; no host extended attributes. Ordinary time query uses SFT fields (not this verb). |
| 112E | `OPEN.ASM:129–139,163–175`, `CREATE.ASM:115–124,151–152` extended open | Pushed create attrs, THISSFT/mode, WFP, EXTOPEN_FLAG (action input and result). See below. |
| 112F | `IFS.ASM:490–516` IFS control | Pushed public AX and pass-through request registers. No HOSTDRV IFS-control provider, chain unless explicitly addressed, then invalid function. |

No active DOS call sites found for the gaps in this table; reserved or
redirector-private verbs from other DOS versions are not obligations of
this kernel. A caller-specific register/ownership trace with synthetic DOS
programs is required in the read-only milestone, especially for FCB refcounts,
remote CDS shape, enumeration ownership and the rarely reached IFS paths.

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

* Discovery: verify `NP2` with a bounded exact read including NUL, then
  query `hostdrv?1` and require exact `HDR1\0`. A missing signature,
  mismatched version or no selected root means no install. `QUERY` below
  provides flags and a mount generation. Attaching requires an explicitly
  enabled host mapping, not merely finding the emulator. This is capability
  discovery, not authentication.
* Execution: write `offset low, offset high, segment low, segment high`
  to 07EDh and send `hostdrv!1` to 07EFh. Read **four** reply bytes from
  07EDh: `4852xxxxh`, low word transport code (`0` completed, `1` invalid
  guest range, `2` malformed/unsupported packet, `3` disabled, `4` busy).
  The host replaces the entire dword on every command, even on failure.
  On code 0 require matching sequence/version and validate the response
  before publishing results to DOS. Stale transport and inconsistent
  replies are general failures, not successes. Stage request and response
  bytes in host memory, then publish only on completed valid transport.
* One guest-owned bounce buffer: 64-byte header followed by up to 4096
  bytes (4160 total), entirely within a single non-wrapping segment in
  ordinary writable conventional RAM. Host requires linear bounds and
  RAM-only mapping for the whole span (no IVT/BDA including VA low work
  area, banked/device windows, ROM or A20 aliases); prove the VA-specific
  RAM predicate before use. No embedded pointers. No DMA or host callbacks
  into DOS; guest copies chunks to/from the DOS transfer area and validates
  its 16-bit segment arithmetic. Host sees neither SDA nor SFT addresses.

All multibyte fields little-endian; offsets and sizes in bytes. Request
sets output-only and reserved fields to zero; response echoes fields not
listed as in/out or out. Protocol errors do not write to guest memory.
On DOS status failure count and returned-payload size are zero except for
write partial-progress as specified below; failed operations never return
a newly allocated token. No native C layout is serialized.

| Offset | Size | Meaning |
|---|---:|---|
| 0 | 4 | ASCII `HDR1` |
| 4, 5 | 1 each | version 1, opcode |
| 6 | 2 | sequence, echoed, increment per command |
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
| 50–63 | 14 | reserved, zero |
| 64–4159 | variable | request path 1 then path 2 then data, no NUL; overwritten by response payload on success |

Only canonical relative DOS path components go on the wire, separated by
standalone `5Ch`; empty path denotes the root only for directory operations.
Strip the owned drive/assignment prefix in DOS before marshalling; no UNC,
local host paths or host names hidden in tokens. `FIND_FIRST` takes the
parent path and an 11-byte padded DOS wildcard template as data (question
marks match DOS 8.3 positions, not arbitrary UTF-8 bytes, and DBCS pairs
must be handled with the VA DOS rules). Option masks and attribute
filtering follow the kernel profile; size/length must agree exactly with
the opcode and no reserved bit is accepted. Proposed opcode assignments:

| Opcode | Request → successful response |
|---|---|
| 00 QUERY | no payload → generation and capabilities |
| 01 STAT | path → type/metadata in attributes, time, date and size |
| 02 SPACE | no payload → 16 LE bytes: total and caller-available host bytes as u64s (read-only advertises zero available) |
| 03 OPEN | path + DOS access/share bits → token and metadata |
| 04 READ | file token + offset + count → actual bytes (0 means EOF), payload length equals count returned |
| 05 CLOSE | file token → empty; invalid/stale token is not a close of another file |
| 06 FIND_FIRST | parent path + 11-byte pattern, attributes → search token, first result |
| 07 FIND_NEXT | search token + ordinal → that result |
| 08 FSTAT | file token → current metadata (e.g. remote seek from EOF) |
| 09 FIND_RELEASE | search token → empty; only used when copied DTA state is no longer needed, e.g. process exit |
| 0A ATTACH | generation from QUERY → reserve the only guest session; fail if already attached |
| 0B DETACH | current generation → end session only if no live file/search resources; invalidate generation |
| 10–19 (hex), reserved for writes | `WRITE`, `SET_LENGTH`, `FLUSH`, `CREATE/OPEN_ACTION`, `MKDIR`, `RMDIR`, `UNLINK`, `RENAME`, `SET_ATTR`, `SET_TIME`, in order; no dispatch until write milestone specifies transaction semantics |

Find results return the padded 11-byte guest name as response payload plus
metadata in the header; guest constructs the DOS directory entry and search
state. Host snapshots one filtered, sorted directory search at FindFirst;
new searches and path lookups see live changes. Use immutable entries and
indexed ordinals, not a cursor tied to a guest memory address, so copying
DTA bytes works. No durable enumeration handles. Bound searches to 32
snapshots and 4096 entries per directory, with a total byte budget; reject
oversized directories (never silently hide later entries), and reclaim oldest
searches under pressure with deterministic no-more-files for stale searches.
Keep their owner PSP in guest state and reap on DOS termination. Use 64
open host handles initially; return too-many-open-files on exhaustion, never
evict a live file. Tokens are monotonic nonzero per session, typed and scoped
to current generation, never host pointers or descriptor numbers. Fail closed
on wrap. Do not reuse an SFT slot's token; generation changes on detach/reset.

SFT owns the logical position; host uses positioned I/O. For DOS requests
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
* Reuse M55's fail-closed canonical root, link/reparse and special-file policy,
  not merely its pathname comparison. Pin an opened root; only directories
  and regular files. Linux/macOS: descriptor-relative directory walks with
  no-follow on each component and final object and post-open `fstat`, strict
  handling of cross-filesystem descent; Linux may use `openat2` containment
  and no-cross-mount policy, with a tested fail-closed fallback. Windows:
  wide-path namespace internally, open root/children with handle-relative
  traversal that cannot follow reparses; verify type, root/volume identity
  and link count on opened handles, never authorize a mutation via a string
  checked before an unanchored reopen. Reject links, junctions, mount crossings,
  alternate data streams, devices, UNC/network exports and ambiguous backends
  that cannot establish these contracts. Deny known multiply linked files
  (and unsupported link-count checks); writes must account for concurrent hard
  link creation, not assume a single check proves exclusive ownership.
  Pinning a parent also does not prevent another process moving that parent
  outside the root. The write milestone must enforce its concurrency model
  (for example, a dedicated root whose directory graph other writers cannot
  move), or refuse writes where it cannot. Do not claim that a prior ancestry
  check protects later mutations.
* Reject `.`/`..`, empty interior components, absolute paths, embedded NUL,
  DOS/Windows device basenames, illegal standalone separators or colons, and
  trailing-dot/space ambiguity. Split Shift-JIS at character boundaries:
  a `5Ch` trail byte is not a slash. Use one pinned reversible VA Shift-JIS
  conversion and DOS case-folding table (at minimum ASCII fold outside DBCS
  pairs; don't use host locale). Convert host names strictly: Win32 UTF-16,
  macOS filesystem names with their actual spelling/normalization, Linux byte
  names interpreted as valid UTF-8 only. Hide invalid host encodings; alias
  valid Unicode names that cannot round trip in Shift-JIS. Do not reconstruct a
  host pathname from a display normalization; look up the real directory entry.
* Reserve all unambiguous round-trippable DOS 8.3 names initially, counting
  encoded bytes, not characters. Resolve fold collisions by a fixed order of
  raw Unicode spelling bytes; only the first eligible name keeps the native
  spelling. Assign the rest `PREFIX~n.EXT`, sanitizing ASCII uppercase prefix
  and extension, shortening prefix as decimal `n` grows, with a stable
  fallback prefix, rejecting DOS reserved names and checking the unified
  namespace. Allocate deterministically in sorted order. Within a mounted
  session pin issued spellings/aliases to entry identities; keep tombstones
  so stale aliases cannot retarget unrelated files, even when a new native
  name collides. New names only consume free spellings. Bound tombstone and
  alias maps and fail explicitly on exhaustion. On a fresh mount the same
  directory contents give the same initial mapping; this is not a promise
  across host renames/remounts. DOS-created names must be legal, round-trip
  losslessly and not shadow existing entries. Write phase tests case-only
  renames and host alias races before enabling them.
* Use host mtime in local civil time, DOS 1980–2107 range clamped with
  two-second truncation; deterministic configured timezone for tests.
  Do not follow links for metadata. Map directory, hidden (Windows flag or
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
* Save/load is disabled while a HOSTDRV session is attached (even with no
  open handles: CDS, aliases and search state are also external). Tag new
  state metadata with detached/unsupported attachment policy; reject attached
  state before writing any partial file or changing live machine state. No
  reopening, replay or rollback of host state. Reset/quit closes handles,
  discards search/alias state and invalidates generation; force detach
  invalidates outstanding tokens and produces deterministic guest errors.
  Ordinary detach requires the guest to uninstall first. A state from a
  detached instance must not turn access back on.

## Guest lifecycle and acceptance

At install, check channel, profile, DOS not busy, valid swap/CDS/SFT bounds,
free drive slot below LASTDRIVE and no existing redirector (1100h AL must
be zero in first release); reject duplicates via a distinct private TSR
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
console or FEP hooks. A guest without the device signature changes no
vectors/CDS and exits non-resident (real hardware or disabled mapping).

Uninstall from a transient command only if top of the INT 2Fh chain, not
busy, no owned live file/FCB/search state, not current default drive, and
no SUBST/JOIN/dependent CDS references. Verify that the slot is still ours
before ending the host session, restoring the original CDS and vector and
freeing memory; otherwise
refuse without change (reboot is the recovery). Refuse using an existing
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
2. **Read-only milestone:** implement the resident subset, transport, host
   policies and UI. Static/ABI checks plus synthetic caller traces for
   handled and chained verbs; test live additions/removals, copied and
   interleaved searches, Japanese/collision fixtures, reads, both seek
   paths, EXT open, FCBs, JFP/FEP, drive conflict/uninstall and missing
   channel, quota and stale tokens, invalid guest ranges, link/reparse and
   renamed-component negatives. Check positive and negative outcomes on
   Linux, macOS and Windows; no backend enabled without its containment
   tests. Prove no writes despite mutating requests and no state portability
   leak. Require the normal clean-build handoff and human acceptance.
3. **Write milestone:** separate opt-in and review, with anchored mutation
   APIs, identity/share/lock rules, guarded replacement and explicit partial
   write, truncate, flush and crash semantics. Test exclusive create,
   mkdir/rmdir, rename/delete, attr/time capability limits, zero-count
   writes, disk-full, concurrent removal/replacement/links, and repeated
   read-only regressions. Initially reject unsupported wildcard mutations
   until DOS semantics and safe effect boundaries are demonstrated; never
   fall back to a host glob or shell. Require write-specific acceptance and
   Windows/macOS/Linux evidence before enabling each backend. No promise
   of host rollback, external-process locking, or portable active states.
