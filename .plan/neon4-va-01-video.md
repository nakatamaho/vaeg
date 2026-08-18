<!-- Copyright (c) 2026 Nakata Maho; 2-clause BSD license. -->

# Step 01 — VA video bring-up

1. Create `demos/neon4-va/` with a lower-case, DOS-8.3 output basename
   `NEONVA.COM` and English NASM source.
2. Implement COM setup (`bits 16`, `org 100h`, `DS=CS`, `CLD`) and a small
   DOS error/status printer.
3. Save enough prior VA video state for a safe exit. Initialize the tested VA
   320x200 G0/G1 mode through `INT 8Fh`, initialize the palette, set G1 over G0
   composition, and enable graphics. Check every returned AX status.
4. Draw a patterned G0 background only during initialization. Use the known VA
   CPU aperture/write-mode sequence; this is bring-up code, not the final scene
   renderer.
5. Define two G1 4-bpp pages with the verified pitch/address relationship.
   Do not expose a page until its complete frame has rendered.
6. Add a bounded VBLANK transition wait and a teardown path that disables
   graphics, clears or restores the previous mode as supported, and returns to
   DOS.

Tests: NASM assembly; static inspection for PC-98 ports/`INT 0Ah`; VAEG launch
showing patterned G0 and an empty G1; ESC exits without a stuck display.

