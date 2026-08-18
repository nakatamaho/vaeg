<!-- Copyright (c) 2026 Nakata Maho; 2-clause BSD license. -->

# Step 01 — VA video bring-up

1. Create a separate lower-case DOS 8.3-compatible source directory.
2. Initialize the verified 320x200 4bpp G0/G1 mode through `INT 8Fh`, set the
   16-colour palette, compose G1 over G0, and set transparency/write mode using
   only the tested VA path.
3. Clear G0 to black through the VA CPU aperture only during initialization.
   Do not create a checkerboard or claim it is NEON4 background.
4. Reserve two G1 pages and implement save/restore of the video mode and DSA.
5. Validate with an empty SGP `SET_WORK`/`CLS` list and a static black screen.

No PC-98 graphics port, text segment, or CPU pixel animation is allowed.
