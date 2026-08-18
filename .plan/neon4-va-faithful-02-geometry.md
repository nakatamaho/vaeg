<!-- Copyright (c) 2026 Nakata Maho; 2-clause BSD license. -->

# Step 02 — Faithful eight-scene geometry

1. Convert authored 640x400 coordinates to 320x200 with signed divide-by-two
   before storing endpoints. Reject any endpoint outside the page.
2. Implement reusable SGP LINE emitters for horizontal, vertical, shallow,
   steep, and all reverse-direction edges. Emit `LINE`, mode, start-dot,
   width, height, pitch, and even address in exactly that order.
3. Preserve scene durations and call order from `SCENE4_256.INC`:
   seed/tetra; cage/solid; satellites; morph; carrier transfer; ribbon;
   perspective grid; finale corona/shutter.
4. Quantize original 0..255 colours to the VA 16-entry palette while retaining
   hue family and brightness ordering. Draw far geometry first and near edges
   last.
5. Clear the hidden page with SGP CLS, emit all current scene lines, wait SGP,
   wait VBLANK, then exchange DSA. No dirty CPU rasterizer is permitted.
6. Add a host/static check that every descriptor address is inside the selected
   320x200 page and every command list begins with SET_WORK.

Do not add sound or text until the geometry is visually correct.
