# Step 01 - VA 640x400 video

1. Create a 16-bit COM entry with `org 100h`, save the current VA mode and
   palette, and use `INT 8Fh` function 0 with `BX=A000h`, `CL=4`, `CH=0`.
2. Initialize the 16-entry palette and compose G0 alone (`CX=0003h`).
3. Configure single-plane GVRAM and FB0 with `FBW=320`, `FBL=399`,
   `DSH=400`, `DSP=0`, and DSA0 page offsets 0 and 20000h.
4. Define SGP addresses 200000h/220000h and a 64000-word CLS. Do not use
   G1 or a 160-byte pitch in this 640x400 version.
5. Add teardown that restores the saved mode, palette-related state, memory
   map and write mode before DOS exit.
6. Build a minimal black/page-grid diagnostic before adding scene geometry.
