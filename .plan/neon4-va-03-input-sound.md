<!-- Copyright (c) 2026 Nakata Maho; 2-clause BSD license. -->

# Step 03 — input and optional VA sound

1. Add ESC handling using the documented VA keyboard BIOS path already used by
   the repository's guest demos. Do not install a PC-98 STOP/IRQ handler.
2. Add a VA Music BIOS adapter only after graphics works. Initialize the Music
   BIOS queue/work area through `INT 8Bh` as required by `611MUSIC.TXT`.
   Keep the graphics loop independent of sound and make the adapter a no-op if
   initialization reports unavailable hardware.
3. Port a short melody from the existing NEON data to a supported VA play mode.
   Do not claim simultaneous FM6+SSG or rhythm support unless the exact BIOS
   call and runtime result are observed. Do not write `0188h`/`018Ah` directly.
4. Ensure sound stop/queue cleanup executes on ESC and on every graphics error.

Tests: assemble with and without sound path; run VAEG graphics-only; run with
Music BIOS present if available; confirm ESC exits in both cases.

