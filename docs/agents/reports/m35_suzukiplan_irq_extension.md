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
# M35 suzukiplan Z80 IRQ extension evidence

## Status and entry state

The focused extension and all required tests are complete. G35 is not passed:
an approved immutable accessible upstream or fork commit is still required.
M36 is not authorized.

The vaeg M35 branch started as
`topic/m35-z80-upstream-extension` at
`bc0b7b631aee6ace5d1c35944e40d7ba098be745`. Tracked files were clean. The
following twelve untracked paths predated M35 and were left untouched:

```text
PC-Engine 1.05(86U13).d88
beep-bank.log
beep-head.log
beep-iptrace.log
beep-m17.log
beep-no-event.log
beep-no-pcm.log
beep-sound-off.log
beep.log
docs/modernization/PCEPAT.DOC
docs/tekumani/
his.txt
```

The upstream worktree was clean at release `1.10.0`, commit
`e3926769a790fab0af1c34a5540e317f8d4f0ddc`, repository
`https://github.com/suzukiplan/z80`. No core was copied into vaeg.

## Verified source result

The local upstream result is commit
`b4a0a5a238fecc280781e6fe5719faf0eafcd667`, tree
`8a606eb39332a6e79b69bb62d9dedca042b923dc`. It changes exactly four upstream
files:

```text
README.md
test/Makefile
test/test-interrupt-extension.cpp
z80.hpp
```

At that result:

- `z80.hpp:259-303` carries the normal and `Z80_NO_FUNCTIONAL` acknowledge
  callback forms and the separate external IRQ-line state.
- `z80.hpp:5981-6036` accepts either the legacy one-shot request or the level
  line, invokes the callback once after EI inhibition and IFF acceptance,
  preserves the external line, injects the IM0 byte through `opSet1`, ignores
  it for IM1 dispatch, and uses it for IM2.
- `z80.hpp:6022-6025` makes the IM2 low-byte/high-byte vector reads explicit;
  it no longer depends on C++11 function-argument evaluation order.
- `z80.hpp:6161-6173` initializes the new callback and line state.
- `z80.hpp:6386-6410` exposes callback setup/reset and inspectable/restorable
  `setIRQLine` and `isIRQLineAsserted` APIs.
- Existing users that only call `generateIRQ` retain its one-shot behavior;
  `test/test-interrupt-extension.cpp` includes an acceptance and cancellation
  regression for that path.

The public register state remains sufficient for architectural state, HALT,
and `execEI` import/export. The only additional restorable state is the
external level-sensitive IRQ line, now available through its setter/getter.
M35 found no evidence requiring a broader state API.

## Corrected source contradiction

Before this milestone, `docs/agents/DECISIONS/ADR-0011-z80-migration.md:35`
claimed `Copyright (c) 2021-2023 Yoji Suzuki`. At the exact selected base,
`LICENSE.txt:3` and `z80.hpp:6` both state `Copyright (c) 2019 Yoji Suzuki`.
ADR-0011 now records the source-backed year. This is a documentation
correction; the MIT-license decision and verified license hash are unchanged.

The base and result use the same `LICENSE.txt` Git blob,
`a4cbbf62b0edaf761ef48556c7a2e50bb3b4817f`, and the same SHA-256,
`ca7261ecf96ab7fea40c4c66aeb644710d210bd71418d285a9dd0098a7bddff1`.
The extension does not modify the license file or its MIT notice.

## Focused coverage

The new test source runs the same 21 cases in normal `std::function` and
`Z80_NO_FUNCTIONAL` builds. It verifies:

- inspect and restore of the external IRQ level;
- DI blocking, EI delay, and a callback byte changed by the inhibited
  instruction before acceptance;
- deassertion before acceptance and persistence across a later EI;
- exact-once acknowledge at acceptance, HALT wake, and NMI non-acknowledge;
- IM0 raw `RST 08h`, `NOP`, `LD A,A` (`0x7f`), and `CALL` without fetching or
  advancing PC for the supplied first byte;
- CB, ED, DD, FD, DDCB, and FDCB continuation bytes fetched from current PC;
- IM1 acknowledge with ignored byte and IM2 callback/vector read order; and
- unchanged legacy one-shot acceptance and cancellation when the new APIs are
  not used.

## Commands and results

All upstream commands ran in a clean checkout or worktree derived from the
exact base. The vaeg repository checks ran against the staged evidence change.

| Command | Result |
|---|---|
| `make -C test` before modification | PASS; complete pre-change upstream suite |
| `make test-interrupt-extension test-interrupt-extension-no-functional` in `test/` | PASS; 21/21 normal and 21/21 `Z80_NO_FUNCTIONAL` |
| `make` in `test/` | PASS; every existing target plus both new focused targets |
| `make cpm` in `test-ex/` | PASS; CP/M exerciser built with Clang and `Z80_NO_FUNCTIONAL` |
| `./cpm -e -n zexdoc.cim` | PASS; every group `OK`, exit 0, 46,748,863,012 emulated clocks |
| `./cpm -e -n zexall.cim` | PASS; every group `OK`, exit 0, 46,748,863,012 emulated clocks |
| `git diff --check` | PASS; no whitespace errors |
| `git clang-format --diff e3926769a790fab0af1c34a5540e317f8d4f0ddc` | PASS; no changed-range formatting diff |
| `git am /home/maho/vaeg/docs/agents/reports/m35_suzukiplan_irq_extension.patch` from the exact base | PASS; resulting tree `8a606eb39332a6e79b69bb62d9dedca042b923dc` exactly matches the tested result |
| `python3 tools/repo/check_encoding.py` in vaeg | PASS; exit 0 |
| `python3 tools/repo/check_eol.py` in vaeg | PASS; exit 0 |
| `python3 tools/repo/check_case.py` in vaeg | PASS; exit 0, 0 findings |
| `python3 tools/repo/find_unreferenced.py` in vaeg | PASS; exit 0, 69 unreferenced paths reported and no M35 path |
| `git diff --cached --check` in vaeg | PASS; the generated patch is treated as patch data by its exact-path `-diff` attribute |

The focused build used the upstream warning policy with Clang 21.1.8,
C++11, `-Wall`, the upstream additional warnings, and `-Werror`. ZEX used the
upstream C++17 CP/M harness. These are public upstream test inputs; no private
ROM or disk-image data was used.

M35 changes only vaeg documentation, provenance, and the generated patch; it
does not change a vaeg build target. The task therefore requires no vaeg CMake
build or CTest run, and none is claimed as M35 evidence.

## Publication and patch provenance

The preferred maintainer fork URL,
`git@github.com:nakatamaho/z80.git`, returned `Repository not found`. No GitHub
CLI or API token was available, so creating an accessible fork or pull request
was not possible in this environment. No write was attempted against the
official `suzukiplan/z80` repository.

The fallback artifact is the directly applicable
[format patch](m35_suzukiplan_irq_extension.patch):

| Item | Value |
|---|---|
| Base commit | `e3926769a790fab0af1c34a5540e317f8d4f0ddc` |
| Tested local result | `b4a0a5a238fecc280781e6fe5719faf0eafcd667` |
| Result tree | `8a606eb39332a6e79b69bb62d9dedca042b923dc` |
| Patch SHA-256 | `d8624085139ef4e7b400b918b2b498e79bea1af4a1942e4ac935545846e746a4` |
| License | Unchanged MIT, Copyright (c) 2019 Yoji Suzuki |
| Upstream issue/PR | None; publication unavailable |

The `.patch` file is generated third-party commit evidence and intentionally
retains standard `git format-patch` metadata rather than a vaeg BSD header.
The vaeg-authored evidence document carries the repository BSD-2-Clause
header. Its exact path is marked `-diff` in `.gitattributes` so an outer vaeg
diff does not reinterpret whitespace-significant upstream context as vaeg
whitespace errors; the artifact remains LF text and is hash-verified above.

## Facts, hypotheses, and recommendation

Verified facts:

- The focused extension satisfies the required IRQ line, acknowledge timing,
  IM0/IM1/IM2, state-access, and dual-callback-build contracts in its tests.
- The complete existing upstream suite, ZEXDOC, and ZEXALL remain green.
- The MIT license is unchanged.
- The format patch applies directly to the exact approved base and recreates
  the tested tree.
- vaeg contains only evidence, not a vendored or integrated core, and no
  guest-visible behavior was changed.

Hypotheses or externally unresolved items:

- Upstream acceptance is unknown because no issue or pull request could be
  published.
- The future immutable fork commit is expected to have the tested tree, but
  its repository and commit ID do not yet exist.

**Recommendation: TECHNICAL GO; HOLD AT G35 FOR IMMUTABLE PUBLICATION.** The
patch is suitable for review and publication as a minimal MIT fork commit.
G35 must remain unpassed, and M36 must not begin, until the maintainer supplies
or approves an immutable accessible commit and explicitly passes the gate.

## Unresolved risks

- No immutable accessible upstream/fork commit or pull request exists yet;
  the local commit alone cannot satisfy G35.
- The focused patch has been exercised on this Linux/Clang host, not on every
  future vaeg build host. Cross-platform backend conformance belongs to the
  later vendor/conformance milestone after G35.
- This milestone intentionally does not integrate the core with vaeg, so
  guest-system interrupt behavior remains for the later differential and
  integration gates.
