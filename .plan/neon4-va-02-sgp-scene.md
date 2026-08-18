<!-- Copyright (c) 2026 Nakata Maho; 2-clause BSD license. -->

# Step 02 — SGP NEON scene

1. Port only geometry/state data from `GEOM4_CORE.INC` and one visually
   representative NEON chapter. Remove PC-98 renderer fields, bank selectors,
   and planar colour assumptions.
2. Define a command buffer with a conservative maximum size, a stable 58-byte
   work area, and per-frame counters. Build commands in main RAM.
3. For each frame emit `SET_WORK`, `SET_COLOR`, `CLS` for the hidden G1 page,
   then painter-ordered `SET_DESTINATION` + `LINE` commands and `END`.
   Initialize the destination descriptor as 4-bpp, 160-byte pitch, even SGP
   address, and explicit start dot. Use the tracked SGP LINE bit definitions;
   include an asymmetric-line self-test before relying on all octants.
4. Program the command pointer with word writes to `0500h` and `0502h`, write
   the VA GVRAM mode required by the tested path, start at `0506h`, and poll
   busy until END. Never patch an active list.
5. Wait for the VA VBLANK interval, exchange the completed G1 page, and update
   the animation phase only after ownership is unambiguous. Preserve G0 and
   G1 composition and painter order.
6. Compare one static frame against a CPU-side reference only for geometry
   placement; do not use a CPU pixel renderer in the COM.

Tests: assemble; run under VAEG; verify lines, overlap order, no stale page
contents, no byte access to word SGP address ports, and no PC-98 I/O constants.

