<!-- Copyright (c) 2026 Nakata Maho; 2-clause BSD license. -->

# Step 00 — baseline and source freeze

1. Confirm `git status --short`, branch, and `git rev-parse HEAD`. Preserve all
   pre-existing untracked paths. Work only on `topic/neon4-va-port`.
2. Inventory `demos/NEON4_1_0/` and classify every include as graphics,
   geometry, input, timing, text, or sound. Record that the original directory
   is an untracked PC-9801 reference and is not edited.
3. Read the PC-98 and VA source documents named in the audit. Decode CP932 to a
   temporary path only; never rewrite the evidence files.
4. Verify the available NASM executable with `nasm -v` or the repository's
   configured NASM path. Check the existing SGP demo build command.
5. Establish the initial VA contract: 320x200, 4-bpp single-plane, G0
   background, G1 hidden/display pages, SGP command list in main RAM, `SET_WORK`
   first, word writes to `0500h/0502h`, SGP busy polling, VBLANK page exchange,
   ESC exit.

Deliverable: the audit report and this plan. Do not change emulator code or the
original NEON source in this step.

