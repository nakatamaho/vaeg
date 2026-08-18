<!-- Copyright (c) 2026 Nakata Maho; 2-clause BSD license. -->

# Step 05 — VAEG launch/fix loop and gate

1. Build VAEG with the supported local preset and build `NEONVA.COM` with the
   exact NASM command recorded in the README.
2. Launch a fresh VAEG session with a disposable PC-88VA boot disk and the
   demo disk or mounted COM. Record only neutral command/output facts.
3. If the program fails before video setup, classify the failure as DOS/COM
   entry, BIOS call, or unsupported instruction. Fix only the first demonstrated
   divergence; do not guess a new port or change VAEG timing to make it pass.
4. If video setup succeeds but the scene is blank or corrupted, inspect the
   command list, SGP busy state, GVRAM address/pitch, and page ownership in that
   order. Rebuild and rerun after each focused fix.
5. If the scene renders but tears, verify the wait sequence and that SGP never
   writes the displayed page. Keep sound disabled while diagnosing graphics.
6. Repeat build, launch, ESC exit, and teardown checks until the COM is
   launchable and visually recognizable. Then run the optional sound path.
7. Update the audit/report with actual VAEG results, unresolved hardware
   questions, and the exact candidate commit. Request the human gate only after
   a clean-checkout build and manual visual/ESC check.

Stop conditions: an unknown hardware semantic, a required emulator change, or
a private fixture unavailable. Record the blocker rather than inventing a
register value or claiming a hardware pass.

