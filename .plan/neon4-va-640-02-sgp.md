# Step 02 - SGP conversion

1. Place the command list and 58-byte work area in main RAM. Compute their
   physical addresses once at startup.
2. Emit `SET_WORK` as the first record of every list and `END` as the last.
3. Implement `emit_line` for original line and horizontal-span operations:
   use LINE-specific `HD=0800h`, `VD=0400h`, 4bpp descriptor mode 1,
   320-byte destination pitch, and a page-relative GVRAM address.
4. Implement `emit_fill_rect` as SET COLOR + 1bpp all-one SET SOURCE + 4bpp
   SET DEST + PATBLT COPY. This is the SGP equivalent of the original
   GRCG/EGC solid rectangle path.
5. Implement SGP CLS for page erase. No `REP STOS`, Bresenham pixel loop, or
   CPU VRAM write may remain in the animated path.
6. Add a static command parser or listing check for opcode order, word writes
   to ports 0500h/0502h/0506h, and no byte writes to the word SGP pointer.
