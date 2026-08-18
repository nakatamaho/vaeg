<!-- Copyright (c) 2026 Nakata Maho; 2-clause BSD license. -->

# Step 05 — Repeat VAEG launch until usable

1. Create a disposable copy of the local boot disk and install the COM.
2. Launch a fresh VAEG with the VA ROM directory, bounded screen capture, and
   a script that types the DOS command after boot.
3. If the display is uniform, the command does not return, ESC is ignored, or
   a scene is wrong, classify the first divergence as guest code, VAEG
   behavior, or unverified hardware assumption.
4. Fix only the demonstrated guest/port issue, rebuild the COM and VAEG, and
   repeat the launch. Keep each correction separate and rerun validators.
5. Inspect all eight scene transitions and verify no checkerboard is present
   unless it is an intentional scene-7 grid element.
6. Request the human gate only after launch, animation, DOS ESC, teardown, and
   D88 contents are all verified. Do not call a screenshot a hardware pass.
