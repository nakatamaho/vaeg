<!--
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
-->

# M108 - CPU conformance checks per role

Status: **complete; G108 passed on 2026-10-15; merged to `main` by [PR #28](https://github.com/nakatamaho/vaeg/pull/28) and [PR #29](https://github.com/nakatamaho/vaeg/pull/29)**

Maintainer-requested QA milestone based on `main`; commit prefix `M108:`.
No dependency on the pending G106a or G107b gates.

## Scope

Replace the hosted uPD9002 SST ratchet with checks that are simple to
verify and maintain, one per CPU role, so that the roles can diverge later
without touching each other's expectations:

1. uPD9002 native mode: `tools/qa/sst_v20.py` runs the SingleStepTests V20
   corpus and compares it with a committed per-form list of failing records
   (`tests/ssts/v20/baseline_ci.json`). Regressions and stale entries fail
   with stable codes; `update` regenerates the baseline. No commit, history,
   support-map, or milestone identity is used.
2. uPD9002 uPD70008-compatible mode (BRKEM): ZEXDOC/ZEXALL through the role's
   production core configuration, compared with the real PC-88VA2 CRCs, and
   the M101/M102 probe programs (DAA/CPL/SCF/CCF dumps, FLAGPRB, ZEX13S,
   ZEXIY, CBPRB, EDPRB, EDPRB2/2S, EDPRB3, EDPRB4) compared with the recorded
   real PC-88VA2 output files.
3. FDC uPD780C: ZEXDOC/ZEXALL and ZEX13S/ZEXIY/ZEXUND/ZEXED through the
   subsystem's production core configuration, compared with the stock Z80
   CRCs.

The role core configuration moves into `Upd9002Upd70008ConfigureCore` (flag
profile and the native-side IVT reader of the undefined ED opcodes) and
`Upd780ConfigureCore`, called by production, by the role ZEX runner, and by
`vaeg_cpm_runner --role`. The CP/M runner moves from the bare external core
to the shared backend; its profile outputs stay byte-identical.
Behaviour is unchanged: the uPD70008-compatible mode keeps the uPD9002 flag
profile and the uPD780C keeps the Zilog profile.

The hosted `upd9002-ssts` job runs the new V20 check instead of the G65m
ratchet and the M60a-M60d evidence tests. Following the maintainer-approved
deletion list, M108 then removes the G43-G70 SST history data under
`tests/ssts`, the 29 validators that only served it, and
`upd9002_m66_identity.py` with its g66b inventory (695 files); the removed
state is commit `884b7dca97b6576007baa235b81a2597fa5f0c22`. The g70 dispatch
tables stay because `upd9002_protected_deletion.py` reads them.

The V20 check leaves the documented target-absent forms (0F31/33/39/3B,
6C-6F, 63) unjudged; every judged record passes. The QA levels and the
per-CPU boundaries of what the external checks do and do not establish are
documented in `docs/modernization/qa/README.md`,
`docs/modernization/qa/upd9002-native.md`, and
`docs/modernization/qa/z80-family.md`.

## Gate G108 (machine-verifiable)

```sh
python3 tools/qa/sst_v20.py selftest
python3 tools/qa/sst_v20.py validate --baseline tests/ssts/v20/baseline_ci.json
# z80asm 1.8 on PATH for the probe programs
cmake --preset linux-ci-gcc -DVAEG_SSTS_V20_ROOT=<verified V20 checkout> \
  -DVAEG_ZEX_ARTIFACT_DIR=<verified ZEX inputs>
cmake --build --preset linux-ci-gcc
ctest --test-dir build/linux-ci-gcc --output-on-failure \
  -R '^vaeg_(sst_v20_|z80_role_probes_selftest|upd70008_(zex|probes)|upd780_(zex|probes))'
ctest --test-dir build/linux-ci-gcc --output-on-failure
python3 tools/repo/check_encoding.py --expect utf8
python3 tools/repo/check_eol.py --enforce
python3 tools/repo/check_case.py
git diff --check
```

### G108

- Passed on 2026-10-15 (maintainer approval of the machine-verifiable gate
  above); the work had been merged to `main` by PR #28 and PR #29.
