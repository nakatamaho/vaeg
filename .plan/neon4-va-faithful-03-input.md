<!-- Copyright (c) 2026 Nakata Maho; 2-clause BSD license. -->

# Step 03 — Input and optional sound

1. Implement the original DOS nonblocking keyboard path:
   `INT 21h`, `AH=06h`, `DL=FFh`; if a character is available, exit only for
   ASCII ESC (`AL=1Bh`). Other keys are consumed and animation continues.
2. Restore DSA, graphics mode, GVRAM write mode, and DOS cursor state on exit.
3. Audit `611MUSIC.TXT` before adding any sound. If the VA Music BIOS queue
   format and ROM service are verified, add an optional AH=00 initialization
   and a short BIOS-played motif. Otherwise leave sound disabled and document
   the unresolved boundary; never probe PC-98 OPNA/OPL3 ports.
4. Verify ESC from a real DOS keyboard as well as a headless command script.

Sound must never be required for graphics startup.
