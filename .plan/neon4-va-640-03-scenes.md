# Step 03 - scene conversion

1. Preserve `SCENE4_256.INC`'s scene order and 384-frame scene boundaries:
   seed, facet, material, morph, raster transfer, surface wave, grid arrival,
   and finale.
2. Keep the authored 640x400 coordinate space; do not divide coordinates by
   two. Clamp endpoints to 0..639 and 0..399 before descriptor emission.
3. Map `line_set16` and `hline_set16` calls to `emit_line`; map rectangle
   fills to `emit_fill_rect`; preserve painter order and palette family.
4. Where the original scan converter fills a non-rectangular face, emit its
   edge plus SGP horizontal spans or explicitly mark the face as wireframe.
   Never substitute a CPU pixel loop.
5. Keep the background black. Emit carrier/raster/perspective-grid geometry
   only in the source scene that owns it; do not add a checkerboard.
6. Build and inspect an early seed frame, a middle material frame, the grid
   scene, and the finale separately so a late black finale is not mistaken for
   an initialization failure.
