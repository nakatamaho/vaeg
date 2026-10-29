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

# M142 — Runtime settings, MH reset, TVRAM investigation and portable states

## Assignment and scope

Base: `75ad3b9129038c3c54a834283ac709b8c02a6ce9`.

The maintainer assigns A–F and additionally requires the Windows handoff to
build on macOS using MacPorts MinGW, **without Colima**. Establish that build
first; if it cannot be built, stop and report the precise blocker. Otherwise
report results together at the end. Do not install/change system ports or
reuse a default container daemon. Keep generated SDKs and evidence out of Git
and within the work-tree boundary. Earlier scratch/builds have been deleted;
old reports do not substitute for available, identity-bound evidence.

- **A** Investigate Information → FPS above 100 FPS, including formatting,
  measurement and actual bar width. Do not assume `32.0` means `132.0`.
- **B** Fresh settings omit EMS and bank memory; explicit saved choices survive.
- **C** Add 2, 4, 6, 8, 10 and 12 MHz clock choices and numeric higher choices
  instead of multiplier-only labels. Preserve machine/FDC clock separation.
- **D** Reproduce and correct the MH demo's delayed reset on non-VA machines
  using the maintainer-provided unchanged private fixture and BASIC
  `LOAD "DEMOMH",R`. Do not conflate this with the separate MA red-screen stop.
- **E** Investigate [#75](https://github.com/nakatamaho/vaeg/issues/75): identify
  what limits VA2 VB-time TVRAM writes on hardware before modeling it. Preserve
  other TSPSPR rows. Q/S observations (28.0 VA, 30.1 VA2, 41.7 emulated VA2)
  are not a demonstrated root cause or permission for a fitted slowdown.
- **F** Correct [#76](https://github.com/nakatamaho/vaeg/issues/76): a
  host-independent DISK record, same-host legacy loading, GUI-equivalent path
  remapping and positive/negative selftests.

## Verification and gate

Focused builds/tests and stable-code single-mutation negatives precede the
integration suite. Record evaluated commits, commands, exits, worker and output
identities outside Git; public summaries use neutral fixture identifiers.
Run encoding, EOL, case, Markdown, milestone audit/selftest and whitespace
checks. Respect existing formatting debt without unrelated reformatting.
No hosted iterative debugging or private-asset upload.

The final gate requires the normal clean pushed-commit static Windows handoff,
recording ON, bundled local encoders OFF, system-DLL import audit, embedded
identity, plus demo/media rebuild/payload and live guest-content regression
specified in AGENTS.md. MacPorts is the maintainer-approved alternative to the
container for this task; do not silently omit shaders or recording to obtain
a successful build. Windows execution is distinct from cross-compilation.
The standard human acceptance is still required; prior G139/G140 acceptances
are not reopened. A hardware-dependent item remains BLOCKED, not PASS, if no
causal measurement can be obtained.

## Progress

- MacPorts GCC/G++ 15.2.0 compile and statically link a C++17 thread/mutex
  probe; only Windows system DLL imports. This is a toolchain prerequisite,
  **not** a successful vaeg build.
- Implementation and full verification pending. G142 **NOT_READY**.
