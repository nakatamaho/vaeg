# NEON4 P5-2 status

P5-2 integrates `scene4_solid_primitives` (SIGNAL SEED) with the 320x200
packed 8bpp VA SGP backend.  The same source can select the previously gated
Facet Assembly scene with `NEON4_P5_SCENE=1`.  The original geometry remains
in logical 640x400 coordinates; the backend halves X and Y at primitive entry.

## Scope

The stage-8 payload renders the selected scene repeatedly until ESC.  Each
iteration advances the original logical scene frame, waits for a VBLANK edge,
clears the draw page, and submits one SGP command batch.  The raster carrier
uses the generic span backend when EGC is disabled.  Face spans still use the
initial word-oriented CLS path; exact one-pixel endpoint handling is
intentionally reserved for the next P5 slice.

The first VAEG run exposed an address-calculation regression: the imported
`config4_256.inc` planar constants redefine `BYTES_PER_LINE` as 80.  The VA
packed backend restores the physical 320-byte pitch immediately after the
scene includes.  Without that override, every SGP row address was compressed
and the scene appeared as a band near the top of the display.

## Backend invariant

The original NEON4 low-colour helpers use `DI` as a private flag while the
scene is building raster assets.  The P5 SGP command cursor is stored in
`p5_list_offset` instead.  This prevents `n4_story_raster_panel` from writing
command words to offset 1 of the payload and was required for the scene to
reach the normal ESC wait path.

## Verification

The following local checks were completed:

* `NEON4_P5_SCENE=0 demos/neon4/build_p5.sh /tmp/N4P5.COM` — PASS.
* `NEON4_P5_SCENE=1 demos/neon4/build_p5.sh /tmp/N4P5-facet.COM` — PASS.
* `demos/neon4/build_p4.sh sgp /tmp/N4P4-regress.COM` — PASS (10,309 bytes).
* VAEG stage-8 scene 0 — PASS: the frame-loop keyboard checkpoint was reached
  at two guest frames and the rendered captures have different SHA-256 values,
  proving that `scene_frame` advances.  The guest crop contains the SIGNAL
  SEED carrier and tetrahedral geometry (non-black bounds 172x149 at 640x400
  presentation scale).
* VAEG stage-8 scene 1 — PASS previously; the selector remains available for
  the Facet Assembly regression.
* A temporary one-span check — PASS: a logical span x=100..500, y=100 is
  centered at the expected physical row after the 320-byte pitch correction.

The current payload SHA-256 is recorded outside the repository with the D88
capture because generated disk images are distribution artifacts, not source
inputs.

The captures are kept outside the repository under `/private/tmp` because
generated disk images and screenshots are not repository artifacts.

## Not yet verified

* exact 1-pixel endpoint writes for packed 8bpp spans;
* CPU-reference/SGP pixel equality for the scene;
* scenes 0 and 2--7;
* 640x200 variant;
* OPNA integration;
* real PC-88VA hardware.
