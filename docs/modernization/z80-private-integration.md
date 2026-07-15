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
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.
-->
# M39 private Z80 integration manifest

This file defines tests but contains no ROM, disk image, restricted screen,
derived payload, or private trace. For each run, record the exact vaeg commit,
host, native/cross/Wine classification, core choice, private ROM-set identifier
and hash, disk identifier and hash, and evidence directory in a maintainer-only
log. Suggested failure evidence paths below are outside this repository.

Build separate trees from the same commit with
`VAEG_Z80_CORE=legacy` and `VAEG_Z80_CORE=suzukiplan`. Run uninstrumented
first. Repeat the relevant case with `VAEG_Z80_INTEGRATION_TRACE=ON` and
`--fdctrace` only when event evidence is needed.

## Test definitions

| Stable ID | Mode and private identifiers | Core selection | Startup and action | Expected screen/system and FDD result | Evidence and save point | Pass criteria / failure evidence path |
|---|---|---|---|---|---|---|
| `m39-boot-va-v3` | VA/V3; record owned ROM-set ID/hash | Run both separately | Cold boot to system menu or BASIC | Normal prompt/menu; no repeated reset or FDD timeout | Record boot completion and representative `f4`/IRQ/DRQ trace | Both reach the same usable state; failures: `private-evidence/m39/m39-boot-va-v3/<core>/` |
| `m39-demo` | VA mode; record legal bundled-demo ID/hash if available | Run both separately | Cold boot and launch the bundled legal demo | Demo reaches its expected stable scene and remains responsive | Record startup and completion state; no save required | Same visible completion and no subsystem stall; failure path uses stable ID/core |
| `m39-os-operation` | VA/V3; record ROM and expendable OS disk IDs/hashes | Run both separately | Boot OS, list a directory, open/read a file | Directory and file data are correct; no retry loop | Capture final listing plus FDD command/status and IRQ/DRQ sequence | Results and transferred data agree; failure path uses stable ID/core |
| `m39-reset-repeat` | Same assets as boot case | Run both separately | Boot, reset, and boot again at least three times | Every boot reaches the same state | Record each completion and any failed iteration | No intermittent loader, WAIT, IRQ, or FDD failure |
| `m39-fdd-read` | VA/V3; record readable disk ID/hash | Run both separately | Read known sectors through the guest | Operation completes once without timeout | Trace command/status, port `0xf4`, DRQ/IRQ, byte count, and final data hash | No missing/extra/reordered event and identical data hash |
| `m39-fdd-write` | VA/V3; record expendable disk ID and before hash | Run both separately | Write known data, flush/eject normally, then read back | One completed command and correct readback | Trace `0xf4`, command/status, DRQ/IRQ; record after hash | No timeout/retry/corruption; failure path uses stable ID/core |
| `m39-loader` | VA/V3; record representative boot-loader disk ID/hash | Run both separately | Cold boot through loader to its expected destination | Loader completes without repeated command or hang | Record final state and FDD event ordering | Both complete with matching device-visible result |
| `m39-loader-timing` | VA/V3; record known timing-sensitive loader ID/hash | Run both separately | Repeat cold boot at least three times | Each run completes | Compare eventual events, not `Exec()` slice numbers; preserve `f4` trace | Any timeout, transfer failure, IRQ/DRQ change, or corruption blocks G39 |
| `m39-sleep-va` | VA idle firmware; record ROM-set ID/hash | Run both separately | Reach normal VA idle path, wait, then assert actual ATN/8255 wake | Sleep engages and guest resumes | Trace live/public PC, memory `7f67`, IN `fe`, `Wait(true)`, drained clocks, wake, `Wait(false)` | Constants remain `1732`; no missed sleep or stuck WAIT |
| `m39-sleep-sorcerian` | Record Sorcerian asset ID/hash if lawfully available | Run both separately | Reach Sorcerian idle path and use actual wake source | Sleep engages and guest resumes | Same trace fields; memory marker is not required | Constant remains `700e`; unavailable asset is recorded, not substituted |
| `m39-state-legacy-new` | Same build family; record ROM/disk IDs | Save with legacy, load same file with suzukiplan | Save only after frame/`Exec()` return during ordinary operation | New selection resumes the same next operation | Record decoded state, next instruction/effect, and final guest state | No revision error, altered transfer, or guest-visible divergence |
| `m39-state-new-new` | Same build family; record ROM/disk IDs | suzukiplan only | Save and reload after frame return | Same next instruction and result | Record register/state summary and device result | Architectural/HALT/WAIT/IRQ/clock state resumes correctly |
| `m39-state-fdd` | Record expendable disk ID/hash | Run legacy-to-new and new-to-new | Save while FDD activity is logically in progress between CPU calls | Loaded operation completes once with correct data | Record save boundary, command/status, `f4`, IRQ/DRQ, and final data hash | No replay, loss, reorder, timeout, or corruption |
| `m39-state-halt` | Record ROM/system path | Run both separately; new-to-new reload | Save at reproducible HALT/idle boundary and load | Guest wakes/resumes normally | Record HALT state, wake source, next instruction/effect | No stuck or premature wake |
| `m39-state-wait` | Record ROM/system path | Run both separately; new-to-new reload | Save after `Wait(true)` and `Exec()` return, load, then apply actual wake | WAIT drains clocks, remains stopped, then resumes | Record WAIT bits, clock balance, wake, and next PC | No execution during WAIT and no failed wake |

## M39 execution record

No private ROM or disk asset was available to the agent checkout. The four
pre-existing untracked paths named by the maintainer were not opened, hashed,
copied, modified, or used. Therefore no private-system result is claimed.

| Stable IDs | Core | Asset identifier | Result | FDD / IRQ / DRQ | SLEEP_HACK / WAIT | State load | Failure artifacts |
|---|---|---|---|---|---|---|---|
| `m39-boot-va-v3` through `m39-state-wait` | `legacy` | unavailable in agent workspace | NOT RUN | NOT RUN | NOT RUN | NOT RUN | none |
| `m39-boot-va-v3` through `m39-state-wait` | `suzukiplan` | unavailable in agent workspace | NOT RUN | NOT RUN | NOT RUN | NOT RUN | none |

G39 remains a human gate. A successful boot alone is insufficient. Any
repeatable missing, duplicated, reordered, or permanently divergent FDD
effect; transfer timeout; data corruption; changed IRQ/DRQ sequence; failed
SLEEP_HACK; stuck WAIT; or changed transfer completion after load blocks the
gate and must retain its private evidence outside Git.
