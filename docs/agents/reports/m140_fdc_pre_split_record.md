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

# M140 — inherited FDC preparation record

This is the verbatim pre-split FDC section from M139 at
[d483636a70c427953bfa13cce0b27ca27216829e](https://github.com/nakatamaho/vaeg/commit/d483636a70c427953bfa13cce0b27ca27216829e).
Its M139/G139 names, source SHAs, protocol identifiers and results are historical,
not a new M140 evaluation or gate acceptance. Current scope/status is in
[M140](../tasks/M140_fdc_upd780_conformance.md). Private captures remain outside Git.

## Additional authorization: physical FDC uPD780 conformance and speed

The maintainer explicitly notes that the uPD780 available for this physical
measurement is in the FDC. Running CP/MVA on the uPD9002, a main-CPU guest on
an emulated non-VA model, or a flat-memory host runner does **not** measure
that physical uPD780. Keep the existing software-role selection and its
validation separate from silicon identification and physical measurements.

Deliverables, all still open unless an explicit result below says otherwise:

1. Preserve the stock 67-group ZEXALL host baseline with the production
   `Upd780ConfigureCore`, pinned corpus and expectation identities. This tests
   the current Zilog-model contract, not NEC hardware. Preserve/reuse matching
   completed expensive evidence rather than rerunning unchanged workers.
2. Establish a safe RAM upload, launch, result retrieval and recovery protocol
   on the actual FDC. The current emulator maps 8 KiB ROM at 0000h–1FFFh and
   16 KiB RAM at 4000h–7FFFh; this is source evidence, not an independently
   verified physical memory specification. The prior RAM-upload observation
   in [M103i](M103i_v1_mode.md#v1-a-demonstration-disk-io-error-at-its-second-bload-real-va-too)
   is a lead, not a complete execution/recovery contract. Respect live firmware
   work areas, stack and mailbox ownership. Do not patch or replace ROMs, and
   do not conduct drive writes during a diagnostic takeover. A main-CPU loader
   may save returned records only after the normal FDC service is restored.
3. Build a reproducible FDC-resident ZEXALL adaptation, or explicitly classified
   split variants if necessary. Stock ZEXALL assumes writable low memory,
   CP/M vectors/stack and the fixed `msbt` state address. Verify every generated
   read/write/fetch and stack footprint fits the usable FDC map. Reassembly or
   relocation can change register/operand addresses, undocumented flags and
   CRCs: never classify a relocated/stock CRC difference alone as a NEC rule.
   Use byte-identical target images on hardware and host, retain stock/ported
   coverage correspondence, independent Zilog reference checks and all 67
   group outcomes. Preserve ZEX's GPL notices; do not modify the vendored
   original or silently reduce masks/vectors to obtain a match.
4. Add focused flag, undocumented-opcode and boundary probes where ZEXALL has
   known gaps. Use explicit initial states and complete final-state records;
   observed crashes/incomplete output are not successful group comparisons.
5. Measure speed with separate bounded, RAM-resident kernels. Report ZEXALL
   elapsed time separately from instruction timings. Keep upload, CRC/log
   output and disk I/O outside the timed interval; calibrate completion and
   notification overhead with the same empty path. Use several iteration
   counts, repetitions and timer resolution/error bounds to separate slope
   from fixed overhead. Include register/ALU, memory, taken/untaken branch,
   stack and prefix cases, without uncontrolled device accesses. Use an
   independently specified board timer/reference, not the emulated CPU's
   nominal MHz or host wall time. Check main H/S/CPU-speed changes alter the
   observer overhead rather than being mistaken for FDC-clock changes.
6. Document observations separately for physical FDC uPD780, physical uPD9002,
   stock Zilog expectations and host models. Reduce each demonstrated mismatch
   to an isolated regression before changing suzukiplan. Work in an out-of-tree
   upstream checkout and reproduce vendored changes through the pinned
   provenance/patch procedure; never hand-edit `external/`. Preserve Zilog and
   uPD9002 behavior and keep any NEC rule per-instance. FDC measurements alone
   do not identify every non-VA main-CPU silicon variant or authorize silently
   applying newly found FDC quirks to another role through the shared helper.

Physical-FDC execution, physical reference approval and hardware recovery remain
**BLOCKED / not established**. No real uPD780 result, speed figure or upstream
correction is claimed. The executable harness is **implemented** and its mapped
host/ROM-backed software integration is described below, not hardware accepted. See the
[Z80-family QA contract](../../modernization/qa/z80-family.md#m139-fdc-upd780)
for the comparison and timing distinctions. This extension does not remove the
existing red-screen, media-regression or clean-build gate requirements.

### Implemented FDC controller, mapped images and speed kernels

The maintainer subsequently requested implementation and supplied private input
`fdc-cpm-01`. [Build/run instructions](../../../tools/hardware/upd780/README.md)
and [canonical monitor ABI analysis](../../bios/fdc_monitor.md) accompany the
new reproducible tool. It adds `FDCONE.COM`, `FDCPRB.COM`, `FDCZEX.COM`, help
and the pinned GPL license to a **copy**, preserving all original files.
The existing main-CPU ZEXALL remains untouched and is not FDC evidence.

- The real monitor transport uses commands 0Bh/0Ch/0Dh and its output/command
  service vectors, not test-only RAM injection. Save 4000h–7EFFh, upload the
  bounded image and zero the rest of that diagnostic range for deterministic
  initial RAM, stop motors during execution, receive records, restore and
  byte-verify the saved range, then permit BDOS log creation. ROM and the live
  7Fxx work/monitor-stack area are not replaced. An unknown-state error halts
  without disk I/O and requires reset. The main controller owns its own stack
  and restores original SP/default DMA on ordinary return.
- Full mapped ZEX retains all 67 groups, flag masks and scan vectors. The
  explicit descriptor-base operand change 0339h → 7339h avoids an FDC-ROM
  write; this and relocation change the comparison contract. Every actual
  CRC is printed alongside `STOCK=`, never relabeled as stock conformance.
  `FDCONE` is separately labeled NEG-only smoke, not full coverage.
- The AF probe records requested/readback AF and full BIT/AND/SCF/CCF results
  for 256 input pairs. Further focused undocumented-opcode/boundary cases,
  independent mapped Zilog controls and physical NEC rules remain open.
- Eleven separate timing bodies use 60000×N iterations, N=1/4/16, two
  repetitions, plus an empty control: 67 timing records. They cover register,
  ALU, memory, loop branch, stack pair, indexed BIT, SCF/CCF and NOP/NOP8.
  This is not a complete independent taken/untaken opcode matrix. Both VRTC
  edges are counted on the main CPU; no reference Hz or physical MHz is
  guessed. Affine slopes, a branch baseline and explicit nonuniform-edge
  quantization bounds separate loop/notification overhead. An independently
  approved physical timer contract is still required for absolute conversion.

The mapped worker, private-input runner and content/allocator negatives are
independently callable. Software integration checks actual upload/launch,
completion, verified recovery, persisted complete records, unchanged existing
file contents and the post-return DIR/prompt; zero exit or a screenshot alone
is insufficient. Current exact identities/results are recorded in the follow-up
validation record. These additions do not resolve the broader blocked G139.

### FDC implementation software-validation record

Evaluated hardware-tool source:
[aa76276241476d317a323c428256336d769117fc](https://github.com/nakatamaho/vaeg/commit/aa76276241476d317a323c428256336d769117fc).
The final private handoff is generated from that pushed commit and is
byte-identical to the tested working image, all five added files and all three
subsystem BINs. Its image/private-input digests, exact commands, assembler
identity and raw captures remain outside Git. No ROM, original input media or
vendored instruction implementation is changed.

Commands (neutral explicit bindings):

```sh
python3 tools/hardware/upd780/build.py --source-disk "$PRIVATE_INPUT" \
  --output-dir "$HANDOFF" --cache-dir "$ARCHIVE_CACHE"
cmake --build "$BUILD" --target vaeg_upd780_fdc_probe_runner
ctest --test-dir "$BUILD" -R '^vaeg_upd780_fdc_' --output-on-failure
"$BUILD/vaeg_upd780_fdc_probe_runner" "$HANDOFF/fdczex.bin"
python3 tools/hardware/upd780/integration.py --worker "$WORKER" \
  --roms "$STAGED_ROMS" --image "$WORKING_IMAGE" \
  --build-report "$RECIPE" --output "$CASE_OUTPUT" \
  --case probe --standard false
# Also probe/standard=true, smoke/standard=true, and an existing-log case.
```

Build and final focused CTest exit 0: **2/2 PASS**, 2.79 s; the content and
allocator suite has **16 passing tests**. The payload integration checks the
256-row independent Zilog AF formula, all 67 exact model-T timing rows, NEG
smoke and a passing mapped-write fixture with one unmapped-address mutation.
Content negatives isolate missing/duplicate/corrupt rows, completion/recovery,
inactive/nonlinear timers, invalid timer reference and terminal contracts;
allocator checks include second-half directory ownership, multi-extent text,
existing-name refusal and one geometry mutation. Error-code assertions use the
smallest content/map layer and do not depend on current Git history.

The production-configured mapped full ZEX worker exits 0 after **all 67 actual
CRC records** and `Tests complete`: 104.40 s host wall time, model clock
`46736000002`. This is **not 67 stock-CRC PASSes or physical timing**. No
out-of-map access occurs. Its output is byte-identical to the earlier exploratory
mapped worker's output; the production-configured worker was evaluated once
because that worker identity changed. The unchanged stock baseline above is
not rerun.

| Public software identity | SHA-256 |
| --- | --- |
| Production mapped worker | `4f8a14266448b76d82490d87dd48cc4733f42e8b46ff77b135647f9f49541fce` |
| Full mapped ZEX payload | `e674749a5caf28e83dc00cd8a1f2265f2db672e271516b90e3768a0039783e57` |
| AF/speed payload | `ac9bd9245aed4b9c767b5a878197770d4f861c13564d452e8569140f7cc8ef09` |
| NEG-only payload | `f854b305b214259b813ed736a4b8b10404bd121feb5bfdefd479c7d544446f7b` |
| Mapped full-ZEX stdout | `ee6271acc2df49b4e773227a728a06afa552d0eaaf5f72eaed1e54503073ee6d` |

Private-input integration uses the unchanged development application worker
`710171ed4aaf4ecab06d4224895b7c17ebf6230118b2254b1d7ffd07abad7bdd`
from the earlier implementation record. No emulator execution source changes
in this hardware-tool increment; a changed tool/document does not justify
repeating unchanged emulator builds or the earlier expensive tests. This
worker's embedded pre-commit identity remains explicitly **not a clean gate
candidate**. Target: VA2, 88-mode CP/M, CPU_Speed=100, isolated config/home,
no second FDD/SASI, SDL dummy video/audio, a ROM set matching MAME's `pc88va2`.

| Software case | Semantic result |
| --- | --- |
| AF/speed, standard=true (S) | Complete 256 AF/67 timing rows, zero model AF differences; recovered, saved, existing files unchanged, healthy DIR/prompt; PASS after contract-only revalidation below |
| AF/speed, standard=false (H) | Same checks PASS; no physical timing claim |
| NEG smoke | One explicitly classified CRC (`d638dd6a`), recovered/persisted, existing files unchanged, healthy DIR/prompt; PASS after terminal-contract revalidation below |
| Existing-log refusal | Refusal before takeover; all files/log bytes unchanged and DIR/prompt healthy; PASS |

S/H fitted slopes differ by at most **0.10 VRTC edges per 60000-iteration N
unit**, within the analytic 8/15-edge comparison bound. This checks observer
coupling in this software target at CPU_Speed=100; other CPU-speed settings,
physical H/S behavior, a physical reference frequency, real uPD780 speed and
physical recovery remain NOT_RUN. No dummy output is promoted to physical or
presenter/window coverage.

Development failures are preserved, not relabeled or discarded:

- The first mapped full run exposed the stock `LD r,n` descriptor's ROM write;
  the declared 0339h → 7339h operand port removes it without dropping a group,
  opcode vector or flag mask. This is a portability adaptation, not a NEC rule.
- A prototype borrowed the caller's main stack and produced unhealthy
  post-return DIR output. The final image-backed private stack and common DMA
  cleanup give a healthy return in the semantic cases. No untraced exact
  overwrite address or emulator root cause is asserted.
- An initial two-edge middle-residual contract incorrectly treated the two
  VRTC transitions as uniform. The independent nonuniform-phase derivation is
  ±2 edges per measurement, four-edge middle residual, **tighter** repeat
  bound 8/15. An analytic passing boundary fixture plus one-edge negative
  exercises that bound. The S run's original FAIL and exact original validator
  source are retained; unchanged digest-bound captures are revalidated rather
  than rerunning the guest.
- The initial terminal checker used BASIC's F3C8h layout instead of the
  fixture's F300h CP/M layout. Both original rejection records remain; the
  fixed-layout passing/one-mutation test and the same unchanged captures
  establish the corrected DIR/prompt verdict. Revalidation is a new report,
  not an overwrite of original failed evidence.
- Initial setup used a legacy assembler helper that placed a compiled helper
  in the shared archive cache. The new builder isolates compilation/executables
  in task output and uses the shared cache for locked archives only. The
  initially generated cached executable is retained pending explicit cleanup
  approval; no shared-cache deletion or private-input modification is performed.

Final repository checks exit 0: encoding, LF, case (0 findings), Markdown
comments (0 findings), 49-case strict milestone selftest, milestone audit,
new C++ clang-format and staged diff checks. The prior whole-manifest baseline
formatting failure is not promoted to PASS. No hosted CI or source-unchanged
full-emulator/media rerun is requested. The standard G139 remains BLOCKED by
the later private guest stop, physical/independent conformance work and the
uncompleted standard demo/media/clean-MinGW acceptance requirements.

### Stock host ZEXALL baseline result

The existing role runner is byte-identical to the preserved M138 runner
(SHA-256 below), but no reusable completed local role-ZEXALL run was found.
A bounded run is therefore performed once for this newly requested baseline.
Evaluated source: `24ad2f0c6e3a1d3c4a75bd90cca075121b00f331`;
subsequent scope/report edits do not change its build inputs or worker.

```sh
python3 tests/z80_compat/fetch_zex.py --output-dir "$CORPUS"
"$BUILD/vaeg_z80_compat_role_zex_runner" --role upd780 \
  --expect tests/z80_compat/upd780_zexall_expected.txt \
  --max-seconds 600 "$CORPUS/zexall.cim"
```

Both exit 0. The pinned stock corpus completes with **67/67 groups OK**,
`Tests complete`, and `PASS expectations=67 clocks=46740000000`.
The clocks are the runner's model accounting, not physical FDC timing.
Host wall time is 98.25 s and is **not** a uPD780 speed measurement.

| Evidence identity | SHA-256 |
| --- | --- |
| Host role runner | `40dc2655e2b139e0b0ba37fcf761fe8d48fc520287b21fd530f436fdd5b25aaa` |
| Stock ZEXALL corpus | `fbb1bb5d46f61c33ea6841a71f2b23c49b9b62410ce6ed4e57b7d9b2e7b437e0` |
| uPD780 role expectation contract | `82e2be848c4fdb4c7278d6cb504338d38c268b2b39d15639dbfd4fca8d3230c2` |
| Standard output | `f827ef78cbdbbf5dc35000a44e9fc74ac5662010ec61cce63f5dd50786529f67` |
| Standard error (empty) | `e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855` |

The exact command, worker/corpus/contract identities, flat-64-KiB host target
policy, exit status, duration and output digests are retained in task scratch.
This is **PASS for the current host Zilog-model contract only**. FDC-mapped
execution was subsequently implemented and software-validated as described
above; physical uPD780 matching and physical speed remain NOT_RUN/BLOCKED. No real-machine difference has been inferred from it.

End of the verbatim pre-split record; current authority is the M140 task.
