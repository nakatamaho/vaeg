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
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.
-->

# M106a — Backport the hardware SGP LINE fix and rebuild demo distributions

## Authorization and scope

The maintainer requests a new main-based demo-fix PR and rebuilt disks after
reporting broken lines in all three wireframe colour depths. Backport only the
LINE correction and its required demo/build dependencies from the independently
worked M106 branch; do not merge its boot loaders, panel, utility media, BIOS
analysis or MS-DOS distribution work.

Hardware evidence already recorded in that branch establishes LINE
`HD=0800h`, `VD=0400h`, contrary to the Technical Manual table. The maintainer
confirmed GLASS and all three SGPWIRE variants on a PC-88VA2. No new hardware
measurement is claimed. BITBLT/PATBLT direction decoding remains unchanged.

The emulator and every affected demo source must change together. Rebuild the
wireframe, GLASS, NEON4, 65536-colour pseudo-sprite and NEON3 distributions,
then the aggregate non-bootable all-demos distribution. NEON3's approved
ADR-0017 source dependency is needed to rebuild rather than patch its COMs.
Keep unrelated demo payloads byte-identical and all raw media out of Git.

## Validation and gate

- Build the emulator and run the SGP selftest and applicable local CTests.
- Prove the four raw direction tests fail with the old LINE mapping.
- Rebuild all three SGPWIRE variants twice; compare with extracted disk files.
- Verify every rebuilt disk's complete listing, payload source identity and
  XZ round-trip before placement in the allow-listed `demos/disks/` directory.
- Verify aggregate payloads against component distributions.
- Run encoding, EOL, case, archive-policy and staged-diff checks.
- Push the final main-based branch and open a PR with exact commit identities
  and validation results. Human visual acceptance remains the maintainer's
  gate; do not claim new runtime/hardware acceptance or request iterative CI.

## Provenance

Original core correction:
[cc915511](https://github.com/nakatamaho/vaeg/commit/cc9155117e433ce333a2451ffcf1325202209aba).
Original demo re-encoding:
[ef036c12](https://github.com/nakatamaho/vaeg/commit/ef036c12154d3ae4919da2c744dce600f286d01f).
Prior wireframe hardware acceptance:
[2ba2c8d1](https://github.com/nakatamaho/vaeg/commit/2ba2c8d1b9530f48b04aab1c3629863bb89369cf).
Raw private evidence is not imported into this PR.

## Local validation record

Initial evaluated LINE implementation commit:
`387900c26f94ce6086b9459a5c810ed927934d29` (its subsequent media/documentation
changes do not affect LINE). Later test-fixture corrections are evaluated
separately below; emulator-core LINE inputs and generated demo payloads are
unchanged. Restored Linux worker SHA-256:
`3da151a42128d2e11cd4c4670776f1ae430865af9c71a638f257ec362bd27f41`.
No instruction-corpus or target-policy change is part of this task.

```text
cmake --preset linux-debug -DVAEG_ENABLE_TESTS=ON -DVAEG_Z80_COMPAT_INTEGRATION_TRACE=ON
cmake --build --preset linux-debug -j 8
  PASS
ctest --test-dir build/linux-debug --output-on-failure -j 8
  105 passed, 2 skipped, 1 failed (pre-existing ROADMAP M106 missing G106)
ctest --test-dir build/linux-debug -R milestone_id_selftest --output-on-failure
  PASS after adding the explicit G106 identifier and the G106a task row
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy build/linux-debug/sdl2/vaeg --selftest
  selftest: all tests passed
```

The two skipped tests require external SST and private BASIC fixtures. All
106 applicable tests pass across the full run and focused roadmap retest.
The temporary old-mapping mutation changed only the LINE direction aliases;
its rebuilt worker exited 1 at `selftest: SGP manual commands failed`.
Restoring the aliases and rebuilding returns the complete selftest to PASS.
No mutation is committed. BITBLT/PATBLT alias definitions are unchanged.

```text
python3 tools/repo/check_encoding.py --expect utf8: 0 violation(s)
python3 tools/repo/check_eol.py --enforce: 0 violation(s)
python3 tools/repo/check_case.py: 0 finding(s)
python3 tests/z80_compat/test_check_zex_archive.py: 11 tests, OK
git diff --cached --check: exit 0, no output
```

### Generated-media contract

Public tracked data disks supply geometry only; no private system template is
used. Every changed COM is freshly assembled with NASM from this branch.
GLASS, NEON4 (both), the 65536-colour sprite and the three wireframes differ
from the previous payloads only in direction-immediate bytes (`04`/`08`);
NEON4 has two emitters. All seven retained 16/256-colour sprite COMs rebuild
byte-identically. NEON3 needs the accepted ADR-0017 vendor sources because its
old private build inputs are lost: the replacement preserves 57,405-byte COM
sizes but also carries previously unpublished M97 text/loop fixes and the
public city geometry. It is not claimed to be a two-byte-only change.

The unchanged, already approved ZUNDAMON distribution is reused byte-for-byte;
its private input is not needed for this LINE correction. No system file or
private payload is added to any disk. Full listings contain only:

| Disk | Complete payload listing |
|---|---|
| glass-orbit | `GLASS.COM` |
| neon3-distribution | `NEON200.COM`, `NEON400.COM` |
| neon4-distribution | `16/NEON4.COM`, `65536/NEON4.COM` |
| sgp-pseudo-sprite | `16/SGPD_7A.COM`, `16/SGPD_7B.COM`, `16/SGPD_7C.COM`, `16/SGPD_7D.COM`, `16/SGPD_7S.COM`, `256/SGP256S.COM`, `256/SGP256T.COM`, `65536/SGP655S.COM` |
| sgp-wireframe | `16/SGPWIRE.COM`, `256/SGPWIRE.COM`, `65536/SGPWIRE.COM` |
| zundamon-orbit (unchanged) | `ZUNDAMON.BIN`, `ZUNDAORB.COM` |
| all-demos | The same 18 files, under `GLASS`, `NEON3`, `NEON4`, `SPRITE`, `WIRE`, `ZUNDAMON` |

For all seven archives: exact file-set/payload comparison and XZ raw-byte
round-trip PASS. Two complete out-of-tree generations with fixed FAT timestamp
2024-01-01 give identical archives, raw images and manifests. Three wireframe
COMs assembled twice also match. Outputs are validated before placement.

Task-local generator and logs are preserved outside Git in the M106a work
area. Exact generator command: `python3 /tmp/vaeg-demofix-media/rebuild.py`;
repeat command: `DEMOFIX_RUN=run2 python3 /tmp/vaeg-demofix-media/rebuild.py`.
Generator SHA-256:
`e6f87e8b790d7a9a3b444126a56bd8e4927a34a7e71415cd973bbd2ada749a9b`.
Both commands exit 0; `cmp` of their manifests exits 0. The manifest records
raw/compressed digests and every public payload's size and digest.

| Rebuilt `.d88.xz` | SHA-256 |
|---|---|
| glass-orbit | `2be78d85b8869dc9b0d1ab879c5e400288022525a09fd40bcdcafd142b86c298` |
| neon3-distribution | `6483685e76b335ed0bac9338d49e44869b2287bd891bb67f3cdcb35d5f0f47e1` |
| neon4-distribution | `359377ac1138e89504355bae51145f997d8d1298587707e30294e530bf31a2f6` |
| sgp-pseudo-sprite | `0aa747e3f92e34f829f00a7a5849fc417da478bf3d957e7a151e1b819ec29bf0` |
| sgp-wireframe | `d45d3711fea14419fd602e74a65717c3f7d8d521c27f80fc18ab1aaf8a6ba8bd` |
| all-demos | `5f18748de06ca5559e7e415bebee486edefe72dc7f284b4651c1fd5a7b217375` |

No raw disk, private screenshot, ROM, OS payload or other unrelated binary is
staged. Archived reference-tier behavior and provenance are unaffected.
This session claims no new ROM/OS runtime or real-machine acceptance.
Human visual acceptance of the PR remains pending.

### CI prerequisite fixtures (separate commits)

ASan before push reproduced a pre-existing heap-use-after-free at
`iocore_inp8`, called by `test_monitor_switch` after `test_v1v2_memory_switch`
freed the I/O table via `pccore_term`. The monitor, cassette and port-40h
selftests now own live machine lifetimes. Backport:
[179f6a10](https://github.com/nakatamaho/vaeg/commit/179f6a10ae3f3b1b2720af54ec4b2aa17fefd324).
The EOL fixture now writes ASCII bytes to avoid Windows text-mode CRLF
translation; two focused repository tests pass. Neither correction changes
normal guest execution or the rebuilt media.

Evaluated ASan fixture commit: `f80158d04e496542596f6e9a38284a55a4a30eca`.
Worker SHA-256:
`a775eb673516942b8f1df472abb2ebd76f1b1d08cf15f9237ae70af528f5e771`.

```text
cmake --preset linux-ci-asan
cmake --build --preset linux-ci-asan -j 8
ASAN_OPTIONS=detect_leaks=0 ctest --test-dir build/linux-ci-asan --output-on-failure -j 8
  106 completed: 104 passed, 2 skipped; tool deadline interrupted tests 46/108
ASAN_OPTIONS=detect_leaks=0 ctest --test-dir build/linux-ci-asan -R 'vaeg_upd9002_m60a_evidence_static|vaeg_m75_transfer_info_compiled' --output-on-failure
  PASS, 2/2 (test 108 completed in this focused run)
ASAN_OPTIONS=detect_leaks=0 ctest --test-dir build/linux-ci-asan -I 46,46 --output-on-failure
  PASS, trace equivalence 194.72 seconds (extended deadline)
```

Thus all 106 applicable ASan/UBSan CTests pass, with the same two
external/private-fixture skips. Completed results were retained rather than
rerunning the entire suite. The focused remaining-test command also ran the
M60a evidence static check, which passed. No expensive SST campaign rerun or
hosted CI debugging was requested. The unchanged disks retain their recorded
generator/COM/manifest identities.
