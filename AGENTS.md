# AGENTS.md — vaeg (88VA Eternal Grafx)

## Filesystem access boundary

Do not read or write directories above `~/work` (`/home/maho/work`),
or files or directories outside that tree. Keep all file access within
`~/work` and its descendants, including access through symlinks and tools.
This restriction takes precedence over any paths or workflows below.

Locations inside the boundary used by the workflows below:

- task scratch: `~/work/pc88va/tmp/m<id>/`; set
  `TMPDIR=~/work/pc88va/tmp` for tools that create temporary files;
- the shared download cache:
  `~/work/pc88va/pc88va-private-docs/pc88va-utilities-cache/`, the
  default of the repository's tools (`VAEG_CACHE_DIR` overrides it);
- private inputs (system disks, ROMs, extra ROMs) are read from
  `~/work/pc88va/pc88va-private-docs/` and copied into task scratch when a
  tool needs a writable or renamed copy.

PC-88VA emulator derived from Neko Project II. Upstream (project-vaeg)
is abandoned; this fork is the living tree.

The active tree is the portable CMake build: C core, SDL2 frontend under
`sdl2/`, Dear ImGui GUI, and macOS / Linux / Windows-MinGW support. It
uses `cpu/upd9002/` for the main CPU instruction core, the built-in CPU
register model in `io/upd9002_regs.*`, the uPD70008-compatible main-CPU
adapter in `cpu/upd9002_upd70008.*`, the uPD780-compatible FDC instance in
`io/subsystem.cpp`, the shared suzukiplan-backed instruction implementation
in `cpu/z80_compat_*`, `sound/opngenc.c` for OPN generation (never define
`OPNGENX86`), and `memoryva/memoryva.c` for the VA memory layer.

## CPU role and file naming

The hardware-facing names are deliberately distinct:

- `cpu/upd9002_upd70008.*` is the uPD9002 main-CPU adapter's uPD70008-
  compatible mode. It is not the FDC CPU.
- `io/subsystem.cpp` owns the FDC CPU and exposes it as `UPD780C`.
- Non-VA PC-8801/8001 selections use an independent main `UPD780C`
  instance, with the same `Upd780ConfigureCore` instruction configuration
  as the FDC but separate state, bus and clock. The existing main-CPU
  bridge in `cpu/upd9002_upd70008.cpp` selects it from the applied ROM
  machine flags; VA/VA2 keep their separate uPD70008-compatible instance.
  VA firmware still supplies the existing board bootstrap/device bridge.
- `cpu/z80_compat_cpu.*`, `z80_compat_bus.h`, `z80_compat_registers.h`, and
  `z80_compat_state.*` are the common Z80 compatibility implementation and
  compatibility layer shared by those two adapters. `cpu/upd780/upd780_disasm.*`
  is the FDC-facing uPD780 disassembler. The remaining Z80 terminology refers
  only to instruction-set, vendor, and historical save-state compatibility; it
  does not identify a separate FDC device.

Keep the shared `z80_compat_*` backend separate from the hardware-specific uPD780C and
uPD70008 layers. When changing hardware-facing code, use the role-specific
uPD70008/uPD780 names above. The stable `UPD9Z80` save-state section name is
also retained for existing state-file compatibility.

The former frozen reference tier (`win9x/`, `i286x/`,
`cpuxva/memoryva.x86`, and `hlp/`) was removed from the current tree in
M57. Its exact G56 snapshot remains available at the annotated tag
`archive/frozen-win9x-i286x-g56` for behavior archaeology and provenance.
Normal fixes land in the active CMake/C/SDL2 tree.

## How work is organized

All work is milestone-driven. Read, in this order:

1. `docs/agents/ROADMAP.md`       — milestone sequence and gates
2. `docs/agents/CONVENTIONS.md`   — repo invariants and new-code rules
3. `docs/agents/tasks/M*.md`      — the single task file you were assigned

Do exactly ONE milestone task per session. Every milestone ends at a
HUMAN GATE unless the task file states the gate is machine-verifiable.
The standard human gate is: build from clean checkout, boot in V3 mode,
run the bundled VA demo, boot an OS and perform simple operations.
The clean-checkout build is the fully static MinGW build described under
"Clean-build handoff"; give the maintainer the absolute path of `vaeg.exe`.
Never begin milestone N+1 until the user states that gate N passed.
Always push the branch and report the exact commit SHAs when done.

### Cleanup after a passed human gate

When the maintainer states that a milestone's human gate passed, and after
the gate record, the `--no-ff` merge and the roadmap record are pushed,
remove that milestone's working directories. The gate pass is the approval
for these categories; no separate list is needed:

- the milestone's worktree and its local branch, after confirming that the
  branch is contained in `origin/main` and the worktree has no uncommitted
  changes;
- the temporary merge worktree;
- the static MinGW build directories of that milestone
  (`../vaeg-mingw-<short-sha>/`);
- its hardware handoff and private scratch directories
  (`~/work/pc88va/m<id>-hw/`, `~/work/pc88va/m<id>-private/`) and
  `~/work/pc88va/tmp/m<id>/`, after moving anything the maintainer asked
  to keep.

Never remove without explicit approval: `pc88va-private-docs/`, another
milestone's worktree or branch, the shared cache
`~/work/pc88va/pc88va-private-docs/pc88va-utilities-cache/` (it is
reused), the remote branch, and files the maintainer supplied (photos,
screenshots, media, archives). Report what was removed and what was kept.

## Demos stay buildable

Every milestone must leave every bundled demo buildable from a clean
checkout with the repository's own sources and tools. Before a milestone's
gate:

- build each demo under `demos/` (GLASS, NEON3, NEON4, the pseudo-sprite and
  wireframe sets, sgp-scan, the self-booting SGP disk) and check that the
  payloads on the committed `demos/disks/*.d88.xz` are byte-identical to the
  fresh build; if a source change alters a payload, rebuild the affected
  distribution, all-demos and all-demos-msdos4 disks in the same milestone;
- start every program of the all-demos disk in vaeg (PC-Engine and MS-DOS
  4.0) and confirm it draws, and run `MANDEL.BAS` from N88-BASIC;
- record the result in the task file.

`zundamon-orbit` is rebuilt with `demos/zundamon-orbit/build-from-psd.sh`
from the freely distributable "ずんだもん立ち絵素材2.3.psd" (a user-supplied
input, like the PC-Engine system disk); its profile tables are tracked under
`demos/zundamon-orbit/256/ida/` (M109).

## Media build regression before every gate

Every milestone gate includes this machine check, run locally from a
clean clone of the pushed commit (the PC-Engine and MS-DOS system disks
are private inputs, so hosted CI cannot run it). Downloads come from the
shared cache `~/work/pc88va/pc88va-private-docs/pc88va-utilities-cache/`
(the utility-media archives at its top, the CP/M archives in `cpm/`, and
`msdos4-va.2/dl/msdos4-pc88va-2hd.d88`, which
`build-all-demos-msdos4-disk.py` downloads from the FreeDOS-88VA
`msdos4-va.2` release when missing); a cache miss may download, a checksum
mismatch fails.

- Utility media: `tools/pc88va/build-utility-disk.sh` builds the utility
  FDD from PC-Engine 1.1, and `tools/pc88va/build-sasi-utility-disks.sh`
  builds both SASI images (PC-Engine 1.05 for the VA, 1.1 for the VA2)
  with that FDD as payload. In vaeg the FDD boots and each SASI image boots
  alone on its model.
- Demo disks: every `demos/*/build-d88.sh` distribution, the all-demos,
  all-demos-msdos4 and self-booting SGP disks build, and their payloads are
  byte-identical to the committed `demos/disks/*.d88.xz` (see "Demos stay
  buildable").
- Record in the task file the commit, the commands, their exit status and
  the SHA-256 of each produced image. Generated images stay outside Git
  except the distributable `.d88.xz` files already allowed.

A failure blocks the gate. Fix it in the milestone (and record it in the
bug-fix ledger), as in M110 when the utility FDD had stopped building.

## Local QA before human review

Maintainer direction during M132: automate reproducible gate checks as local QA
instead of repeatedly asking the human to execute them. Private-input tests run
locally, with explicit input bindings and out-of-tree generated evidence; they
must not upload private assets, identities or captures to hosted CI. Reusable
runners and content validators belong in the repository, without private paths.

A zero worker exit, a saved screenshot or a plausible nominal clock is not a
semantic PASS. Check guest-visible results, expected state transitions, persistence
and appropriate image/text contracts. Use stable failure codes and passing-fixture,
single-mutation negatives. Distinguish PASS, FAIL and BLOCKED/NOT_RUN; dummy video
must not stand in for actual presenter, window or fullscreen inspection. Preserve
and reuse unchanged expensive evidence under the identity rules below.

Prioritize machine/mode/reset, H/S execution, paste, cassette and actual state-file
round trips. Visual/layout checks are also machine-owned: compare final-client
screenshots against independently approved, frozen references at fixed settings,
size, DPI and backend, with narrowly bounded dynamic masks and region checks.
Do not assign routine overlap/clipping/fullscreen checks to a human merely because
they are visual. Human work is reference approval, intentional visual-change
approval, genuinely unautomatable interaction and final acceptance. Lack of a
real display blocks that target's capture automation; it does not transfer the
whole test back to the maintainer. A mapped SDL client on virtual X11 with a
window manager is a valid explicitly labeled software-integration target, not
SDL dummy output; never promote it to physical-GPU, Cocoa, Windows or true-HiDPI
coverage. Report exactly which checklist items an automated case covers; adding
a runner does not mark unimplemented cases passed, retroactively upgrade prior
manual checks, or authorize a milestone merge without its required gate acceptance.

## CI and expensive-test discipline

Do not use hosted CI as an iterative debugger. Before requesting a hosted CI
run, complete the applicable local builds, tests, static validators, repository
checks, and final-commit scope checks. Confirm that the intended commit chain
and generated-evidence layout are final.

Preserve expensive intermediate results in commits or untracked, task-specific
work directories. Record the evaluated commit, worker-binary digest, corpus
identity, comparison-contract identity, target-policy identity, exact command,
exit status, and output digests. Reuse a completed SST or other expensive run
when these identities are unchanged and the task contract permits reuse. A
documentation-only, report-only, evidence-only, or validator-only edit does
not by itself justify rerunning unchanged emulator behavior; prove that the
build-relevant inputs and worker digest are unchanged instead. Never rerun an
unchanged failing hosted job without first retrieving its exact log and
addressing or explaining the failure locally.

Push and run hosted CI only after local validation and commit ordering are
settled. Prefer one final hosted run for the review candidate. If a hosted-only
failure requires another attempt, preserve the prior results, make the
smallest justified correction, rerun the affected local check, and avoid
repeating unrelated jobs or profiles.

## BIOS analysis documentation — canonical placement

**Publish BIOS analysis documents under `docs/bios/`.** Do not place new
interrupt/service reference documents under `docs/modernization/`, in the
emulator-source `bios/` directory, or only in a private work directory.
Use lowercase Markdown filenames based on the interrupt number, for example
`docs/bios/bios_int80h.md`; keep AH subfunctions in that interrupt's document.
Manual chapter numbers such as `601` are source references, not canonical
filenames. Include conventional-memory work layouts when established, and
clearly separate manual specifications, static ROM findings, and runtime
verification or unresolved hypotheses.

Publish independently written analysis only. Private source paths, asset
filenames and hashes, raw disassemblies, manual copies, ROMs, disk images,
and raw captures remain outside Git under the existing private-asset policy.

## Permanent bug-fix ledger

`docs/modernization/bug-fixes.md` is the permanent correctness ledger for
this fork. Update it in the same milestone and normally in the same commit
whenever a change fixes incorrect guest-visible behavior, a portability
crash or data-corruption risk, persistence/media handling, or a regression.

Each entry must identify the symptom, affected scope, demonstrated root
cause, correction, verification, milestone/task, and commit. Do not record a
hypothesis as a root cause. Keep unresolved defects in the ledger's open
section until evidence and a tested correction exist. Feature additions and
cosmetic restorations do not belong there unless they also correct a concrete
defect. Evidence documents and fixing commits must be clickable Markdown
links; commit links use the repository's GitHub commit URL with the full SHA.
Release notes may summarize the ledger but do not replace it.

## Repository invariants (steady state since phase 1)

- Sources and documentation are UTF-8 without BOM. Never introduce CP932
  content.
- EOL is LF throughout the current tree. Tool-mandated project formats remain
  declared in `.gitattributes`; do not add new CRLF files without an explicit
  repository-policy exception.
- Tracked paths are lowercase except tool- or project-mandated names listed
  by `tools/repo/check_case.py`, including top-level `CHANGES*.md` release
  notes. New paths must otherwise be lowercase.
- Never modify binary payloads: `romimage/`, ROM images, fonts, icons,
  cursors, or wave data.  A narrowly scoped distribution exception permits
  committing a reproducibly generated `.d88.xz` only when its directory has
  an explicit allow-list, the image listing proves that every stored file is
  a source-built and freely distributable payload, and the compressed image
  round-trips byte-for-byte to its generated raw D88.  A disk may be bootable
  if it meets these conditions; a disk that needs PC-Engine, NEC system files,
  or other non-redistributable payloads to boot does not.  A maintainer-
  approved exception (M106) admits the MIT-licensed MS-DOS 4.0 for the
  PC-88VA of the FreeDOS-88VA project as system files of a bootable demo
  disk: only from the pinned release `msdos4-va.2` image (its published
  SHA-256), only the files a `pcengine_disk.py vanilla` copy keeps, with the
  release's `LICENSE.TXT` on the disk and its boot sector and files checked
  by hash.  The raw D88, PC-Engine system files, private media, ROMs, and
  source templates remain untracked and must never be committed.
- Treat private integration asset identities as sensitive. Tracked files must
  use neutral stable test identifiers; do not record private filenames,
  absolute paths, or hashes unless the maintainer explicitly authorizes that
  exact metadata. Keep raw private screenshots, logs, traces, and save files
  outside Git, and never stage, commit, or push them.
- Refer to commercial-machine ROMs only by MAME's published set and file
  names and SHA-1 values. Do not state who owns, dumped, or holds a ROM,
  where a ROM came from, or that it was checked on hardware; describe a
  ROM-based check as "with a ROM set matching MAME's `<set>`". Existing VA
  dump notes stay as historical records.
- The maintainer's private reference tree (`pc88va-private-docs/`) is
  read-only for agents, except its `pc88va-utilities-cache/` directory,
  where the repository's tools may store archives they download (nothing
  else: no work files, builds, or logs there). Otherwise never create,
  modify, or delete anything under it:
  no work directories, scratch output, builds, logs, screenshots, or
  generated media. This includes audit or disassembly scratch
  (`*-audit-*`) and milestone work trees (`m<id>-*`), which have been
  found there and removed. Put task work under a task-specific directory
  elsewhere (for example `~/work/pc88va/m<id>-private/` or
  `~/work/pc88va/tmp/m<id>/`).
- Never re-encode, re-wrap, or reformat lines you are not changing.

## Hard rules

- One concern per commit. Renames (`git mv`) go in rename-only commits;
  reference fixups go in a separate, immediately following commit.
- Deletions happen only from a list the user has explicitly approved.
- New files created in phase 2 carry a 2-clause BSD header
  `Copyright (c) 2026 Nakata Maho` (see CONVENTIONS.md §New code).
  Never alter copyright headers of existing files.
- Do not restore the M57-deleted reference-tier paths (`win9x/`, `i286x/`,
  `cpuxva/memoryva.x86`, or `hlp/`) unless a task explicitly requires it.
  Use tag `archive/frozen-win9x-i286x-g56` for historical comparison.
- Core code (root, `io/`, `sound/`, `cbus/`, `vram/`, `*va/`,
  `cpu/upd9002/`)
  stays C. C++17 is allowed under `sdl2/` (frontend + GUI) and, when an
  approved third-party CPU core requires it, for CPU backends and thin
  compatibility adapters under `cpu/`. C++ and STL types must not cross
  existing C-facing subsystem or state-save interfaces. Other newly written
  emulator-core code remains C99 unless an ADR separately approves it.
- Vendored third-party code lives under `external/` with the exact
  version recorded in `docs/agents/DECISIONS/`. Never hand-edit it.
- Run the machine checks named in your task file (`tools/repo/*.py`,
  cmake builds) and paste their output into the PR description.

## Local-only statically linked H.264/AAC encoders

Maintainer policy: H.264/AAC encoder implementations may be statically linked
**only into explicitly opted-in, maintainer-local builds** for personal use.
This permission is not a patent clearance, a redistribution authorization, or
an exemption from the selected implementations' copyright/license obligations.
It does not authorize publishing a codec-enabled executable.

- **Maintainer-local personal builds MUST have MP4 encoding enabled.** On Linux,
  explicitly configure `VAEG_ENABLE_RECORDING=ON`, `VAEG_STATIC_FFMPEG=ON`,
  `VAEG_ENABLE_LOCAL_MP4_ENCODERS=ON`, and
  `VAEG_LOCAL_MP4_ACKNOWLEDGEMENT=LOCAL_ONLY_NO_DISTRIBUTION` using the reviewed,
  isolated local SDK. Do not silently substitute an encoder-OFF build when its
  dependencies are missing: report the local build as blocked instead.
- On macOS/Windows, a personal local build MUST enable recording/OS-native MP4
  support (`VAEG_ENABLE_RECORDING=ON`), using AVFoundation/Media Foundation.
  Keep `VAEG_ENABLE_LOCAL_MP4_ENCODERS=OFF` there: that option is Linux-only and
  must not replace the preferred OS encoder with bundled implementations.
- **Every non-personal-local build MUST explicitly set
  `VAEG_ENABLE_LOCAL_MP4_ENCODERS=OFF`.** This includes all CI, public/release,
  packaging, clean-build handoff and distribution builds, even when executed on
  the maintainer's own machine. Classify by intended use, not physical location.
  The OS-API adapter exception below does not permit bundled static encoders.
- The local-codec cache default must remain **OFF** as a fail-safe; the mandatory
  personal Linux ON setting above is an explicit build instruction, not automatic
  detection. Installed encoders, inherited environment or dependency caches must
  never silently enable it. Normal Native/Displayed FFV1/PCM recording remains
  independent of this option.
- **Never enable this option in any CI job**, including PR, branch, scheduled,
  test, debug, release, or self-hosted CI jobs. Configuration/build entry points
  must reject local-codec enablement in CI rather than silently producing a
  codec-enabled target. Do not upload such binaries, libraries, SDKs, symbols,
  source bundles, or dependency caches as CI artifacts.
- **Never include these statically linked H.264/AAC encoder implementations in
  a public release.** Release presets, packaging scripts and clean-build handoff
  helpers must explicitly keep the option OFF and reject conflicting local
  enablement. A codec-enabled local build must not be copied to release staging,
  the usual release destination, GitHub Releases, or other public downloads.
- Keep codec-enabled builds, dependency stages and caches outside Git and
  separate from ordinary/CI/release build trees. Do not commit or push encoder
  archives or local codec-enabled artifacts. Rebuild the normal release target
  in a fresh, option-OFF tree; never relabel or reuse a local codec-enabled
  executable or its application objects as a release build.
- Before implementing this option, add fail-closed configuration/packaging
  checks and focused tests for default-OFF, local explicit enablement, CI
  rejection and release rejection. Release dependency/configuration audits
  must prove that no H.264/AAC encoder implementation was pulled in through a
  static FFmpeg archive or another dependency. Absence of codec DLL imports is
  **not** evidence that static encoder code is absent. Keep the ordinary static
  FFmpeg encoder allow-list restricted to FFV1/PCM; do not expand it for this
  local-only feature.
- Pin and document the exact local encoder implementations and their licenses
  before linking them (for example, libx264's GPL/commercial licensing is
  separate from codec patents). Do not claim that private use, static linking,
  or an external/OS API automatically resolves all patent/license obligations.

M100s11 implements this boundary with `cmake/mp4_policy.cmake`, public-preset
markers, configure/pre-build static archive audits and focused negative tests.
See `docs/agents/DECISIONS/ADR-0018-post-recording-mp4.md`. Policy-only script-mode
test fixtures may simulate local settings, but must never configure/build a
codec-enabled vaeg target or compile/link encoder implementations in CI.

macOS/Windows OS-API export adapters may be built in normal CI/release profiles:
they call encoders already provided by the OS, rather than bundling encoder
implementations. Do not interpret this distinction as patent clearance or an
approval of the OS/application's license terms for every use or distribution.

## Validator design and negative-test isolation

Design new evidence and repository validators as three independently callable
layers:

1. content validation for schemas, counts, hashes, classifications, and
   semantic evidence;
2. protected-history validation for explicitly enumerated approved artifacts;
3. Git-topology validation for evaluated/evidence commit ordering and
   evidence-only commit scope.

The production entry point may compose all three layers, but focused tests
must call the smallest relevant layer. Do not make a content-mismatch test
depend on the current Git history, and do not make a commit-topology test
depend on a large real evidence corpus. Exercise Git-topology checks in a
purpose-built temporary repository.

Every fail-closed negative test must:

- start from a fixture that passes all checks relevant to that test;
- apply exactly one controlled mutation;
- assert a stable, machine-readable error code for the intended invariant,
  not merely a nonzero exit status or message substring;
- fail if an unrelated validator masks the intended failure;
- retain one final integration test that composes every layer.

Use explicit protected artifact manifests or closed path prefixes. Never
protect an open-ended parent directory when later milestones legitimately add
new children beneath it. Generators must write out of tree and be validated as
generated output before repository-placement or commit-scope checks run.
Generation must not require the future evidence commit to exist.

Keep stable error codes separate from explanatory text, for example
`M60D_FRAME_SET_MISMATCH` versus
`M60D_EVIDENCE_COMMIT_SCOPE`. A test for one code must not pass when it
receives the other. Do not add production bypass flags merely to make a
negative test reach its intended branch; isolate the underlying validator
function instead.

## Build reference

- Active: CMake >= 3.20. `cmake --preset linux-debug` etc. SDL2 is
  discovered via find_package/pkg-config by default, with the
  ADR-0006-pinned FetchContent path only where a preset opts into it.
  Dear ImGui is vendored under `external/imgui`.
- Archived reference: the former VS2017 v141/Win32 tree is available only at
  tag `archive/frozen-win9x-i286x-g56`; it is not a current build target and
  has no CI or compile-guarantee coverage.

### Colima and Docker storage

Create a dedicated directory under the current working directory for Colima
and Docker runtime storage, for example `$PWD/build/container-runtime/`.
Keep Colima configuration, VM disks, and Docker image/container data there;
configure `COLIMA_HOME` and any underlying VM storage paths accordingly.
Docker images must reside in that local VM/daemon storage, not a shared default
Colima environment or an unrelated Docker daemon. A local Dockerfile or bind
mount alone does not satisfy this storage requirement. Keep generated runtime
files out of Git. Do not move or delete existing environments without approval.

Select vaeg's dedicated daemon per process with `DOCKER_HOST` or an explicitly
selected, project-specific `DOCKER_CONTEXT`. Never run `docker context use`,
change the persisted global current context, or overwrite another project's
context definition. `DOCKER_CONTEXT` takes precedence over `DOCKER_HOST`:
when selecting a Unix socket with `DOCKER_HOST`, unset inherited
`DOCKER_CONTEXT` in the child process, not in the user's login-shell setup.
Fail if the dedicated runtime/socket is unavailable; never fall back to the
shared/default daemon. Start Colima with `--activate=false`; newly created
runtimes must use project-specific profile names to avoid context-name
collisions. Do not rename or recreate an existing runtime without approval.

On the maintainer's macOS setup, `~/work/tmp/vaeg-container` is a local-only
wrapper for the existing vaeg runtime. It sets `COLIMA_HOME` and `DOCKER_HOST`
for its child command, clears inherited context/TLS overrides, and leaves the
global context untouched. Use it when present, for example:

```sh
~/work/tmp/vaeg-container docker info
~/work/tmp/vaeg-container tools/release/build-mingw-static-container.sh --commit <full-sha>
```

The wrapper stays outside Git. On other setups, apply the same process-local
selection to that checkout's dedicated runtime; do not assume this local
wrapper exists. Verify that `docker context show` is unchanged after use.

### Clean-build handoff

Whenever a clean build is needed, including the "build from clean
checkout" step of a human gate, produce a fully static MinGW-w64 build of
the pushed commit with:

```sh
tools/release/build-mingw-static-container.sh --commit <full-sha>
```

The script clones the commit into a new `../vaeg-mingw-<short-sha>/`
directory, builds in a pinned Ubuntu 24.04 container with the `mingw-cross`
preset (static SDL2, LibArchive, zlib, xz, librashader, and MinGW runtimes),
verifies that the commit and date are embedded, and audits the PE imports
down to Windows system DLLs. Never reuse an existing build tree for this.
Hand the maintainer the absolute path of the resulting `vaeg.exe`, its
SHA-256, and the absolute path of `BUILD-REPORT.md` in the same `output/`
directory.

### Maintainer release handoff

When the maintainer says to copy release builds to "the usual place", the
destination is:

`/mnt/c/Users/maho/Dropbox/Documents/Emulators/PASOCON/NEC PC-88VA/vaeg_new/`

- Copy `build/linux-release/sdl2/vaeg` there as `vaeg`.
- Copy the MinGW release executable there as `vaeg.exe`; its source is
  `build/mingw-release/sdl2/vaeg.exe` for a native build or
  `build/mingw-cross/sdl2/vaeg.exe` for the Linux cross-build.
- Verify each copied executable against its build artifact with `cmp` or
  SHA-256.

The destination is outside `~/work`, so under the filesystem access
boundary agents do not write there: put the two executables in
`~/work/pc88va/release-handoff/` as `vaeg` and `vaeg.exe`, verify them as
above, and give the maintainer their absolute paths and SHA-256 values for
the copy.

This is a maintainer-local directory outside Git. Unless explicitly requested,
replace only `vaeg` and `vaeg.exe`; do not alter or inventory its ROMs, disk
images, configuration, save data, or other private integration assets.

## Commit messages

UTF-8, LF, English subject, canonical `M<id>:` prefix. The ID is an integer
or an integer followed by one lowercase letter and an optional nonzero decimal
suffix, such as `M58:`, `M60a:`, or `M62b1:`.
