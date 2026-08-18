<!-- Copyright (c) 2026 Nakata Maho; 2-clause BSD license. -->

# Step 00 — Audit and freeze

1. Record `git rev-parse HEAD`, branch, clean tracked status, and the fact that
   `demos/NEON4_1_0/` is local reference material.
2. Read the source files named in the audit and map each PC-98 backend to its
   VA replacement or explicit non-goal.
3. Read `docs/98io/` and `docs/tekumani/` for graphics, SGP, VBLANK, keyboard,
   text, and Music BIOS evidence. Do not invent a port from a matching number.
4. Record the original eight scene order, frame lengths, coordinate range,
   palette families, and which scene-specific carriers/grids are retained.
5. Review the previous simplified prototype only as a rejected design. Do not
   copy its checkerboard or INT 82h input path.

Output: update the audit report. No source implementation changes in this step.
