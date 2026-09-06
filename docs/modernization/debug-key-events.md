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
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.
-->

# Explicit debug key transitions

The FreeDOS PC-88VA M11 task authorizes this narrow extension of the accepted
M74 debug harness, starting at
`7463f9501d84701f50f3243d5067b6a9dfd0c2e7`. This is not a reopening of the
historical VAEG M11 platform-port task or a new hardware gate.

`debug-script 1` accepts two additive commands:

```text
wait-frame 120
key-down 29
wait-frame 125
key-up 29
```

The argument is a decimal guest make code in 0 through 127, as used by
`kbdinject_keydown`/`kbdinject_keyup`. It is neither ASCII nor a host SDL
scancode. Callers choose keys using the public keyboard mapping inventory.
The existing guest-frame and PC-ordinal waits determine execution; no host
elapsed-time pacing is added. Consecutive commands execute in order at the
same checkpoint. A modifier is an ordinary explicit transition. Repeated
press testing uses make, break, then make, not typematic timing assumptions.
Operators must explicitly release their keys before a completed session.

The event log retains its existing four-column schema and adds `key-down`
and `key-up` event names with guest frame, neutral ID `-`, and guest code.
Malformed, missing, extra or out-of-range arguments fail parsing. Overlap
with active host-paced paste fails execution instead of silently delaying a
transition. Existing `input-line` and `enter` behavior is unchanged.

A script without `trace` commands may run alongside the existing bounded
causal observer. Startup instruction tracing remains incompatible with debug
scripts, and a script containing any `trace` command is rejected before
machine initialization when causal tracing is requested. This prevents two
instruction-trace sessions from competing for the same session state while
allowing normal event scheduling plus observational ownership checks.

All injection goes through the same `kbdinject` -> `keystat` -> `keyboard_send`
path as normal frontend input. This extension does not write guest RAM,
firmware queues or registers, bypass a device, install a special mapping,
alter interrupt processing, or read additional guest memory. It adds no
default-on behavior and no core globals. Private scripts, configurations,
input identities, firmware-derived addresses and captures remain local-only.

The selftest covers accepted endpoints and invalid arguments. The existing
ROM-free harness integration adds two byte-identical complete event-log
trials with press/release/modifier/repeated-press ordering. That smoke test
uses a single completed frame because ROM-free `--smoke` intentionally stops
after one frame; it does not claim multi-frame firmware keyboard behavior.
Private milestone qualification must establish actual device/firmware/input
causality independently with production memory and testing seams disabled.

## Local validation boundary

P0/P1 (testing off, tracing off/on) and T0/T1 (testing on, tracing off/on)
compile natively on macOS. Their selftests preserve the identical production
architectural checkpoint. The T0 suite has 82 tests and T1 has 87; each passes
its available ROM-free cases, with the external SST corpus test explicitly
skipped locally. The MinGW cross-build also compiles. Encoding, EOL, case,
diff checks and formatting of the debug-harness source/header pass.
Whole-tree formatting has pre-existing violations at the pinned baseline;
unrelated source formatting is not changed by this feature. Exact-head hosted
CI remains a separate requirement, including the external corpus job.

No core, CPU, memory, device, archived-reference, ROM payload or persistence
semantics are changed. No release artifact is copied to a user installation.
