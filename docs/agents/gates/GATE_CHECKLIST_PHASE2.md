# GATE_CHECKLIST_PHASE2 — human verification per gate

General rule: the agent's build logs and smoke runs never substitute
for these steps. A gate passes only when the user states it in the
conversation/PR.

## G7 (M7, machine + review)
- [ ] Agent pasted gcc AND clang full build logs, zero errors
- [ ] tools/repo checkers clean
- [ ] Review m7_source_list.md and every "minimal fix" commit — each
      must be individually justified and behavior-preserving
- [ ] LEGACY v141 build unaffected (statement in PR; spot-build if in doubt)

## G8 (M8, human, Linux) — frontend gate (PC-98 scaffold)
- [ ] Clean checkout, `cmake --preset linux-release`, build
- [ ] Window opens; machine starts (with ROMs if present; defined
      error message without — no crash)
- [ ] Keyboard reaches the guest; audio initializes; clean exit
- [ ] `--smoke` exits 0 headless
- [ ] Tag `portable-pc98`

## G9 (M9, human, Linux) — STANDARD VA GATE
- [ ] Clean checkout build, VA machine
- [ ] V3 mode boot
- [ ] Bundled VA demo runs
- [ ] OS boot; DIR; launch a program
- [ ] Anomalies cross-checked against LEGACY v141 on Windows with the
      same media before filing as a port bug
- [ ] Tag `portable-va`

## G10 (M10, human, Linux)
- [ ] Both ADRs (backend, font) were decided by the user BEFORE code
- [ ] Full walk of the must-have set with a real disk image
- [ ] Japanese labels render; no guest font ROM access
- [ ] GUI typing does not leak into the guest and vice versa
- [ ] GUI-PARITY.md status column is honest (stubs visible, not hidden)

## G11 (M11, human, Windows + macOS)
- [ ] Standard VA gate executed on real Windows (MinGW build)
- [ ] Standard VA gate executed on real macOS
- [ ] Non-ASCII (Japanese) path to a disk image mounts on Windows
- [ ] GUI must-have walk on both

## G12 (M12, machine + review)
- [ ] Green run URL, all three OSes, on the fork
- [ ] ctest passing; codecnv wave-dash test present
- [ ] Invariant checkers wired as a required job
- [ ] No ROM/disk-image material anywhere in CI

## G13 (M13, human)
- [ ] Deletion list approved item by item (win9x/, i286x/ decisions
      made explicitly)
- [ ] `pre-retirement` tag pushed before any deletion
- [ ] Clean-clone builds on all three OSes following docs only
- [ ] Standard VA gate one final time per OS
- [ ] Tag `phase2-complete`
