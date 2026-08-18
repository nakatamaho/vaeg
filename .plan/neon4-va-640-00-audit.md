# Step 00 - audit and evidence freeze

1. Confirm the working branch is based on `origin/main` and that the local
   NEON4 source and hardware-reference directories remain untracked/read-only.
2. Read `NEON4_16.ASM`, `VIDEO4_LOW.INC`, `GEOM4_LOW.INC`,
   `GEOM4_CORE.INC`, and `FRAME_RENDER4_LOW.INC`; list every primitive that
   writes pixels or selects a page.
3. Read `docs/98io/io_disp.txt`, `io_pmc.txt`, `io_egc.txt`, and `io_agdc.txt`
   as PC-9801 comparison evidence only.
4. Read `docs/tekumani/606GRP.TXT`, `4.TXT`, `607ADVG.TXT`, and the tracked
   VA video/SGP reconstruction reports. Extract the 640x400 mode bits,
   framebuffer pitch, page capacity, command opcodes and VBLANK interface.
5. Record the conversion and hostile-review tables in
   `docs/modernization/neon4-gdc-sgp-640-audit.md`.
6. Do not create a COM or modify source in this step.
