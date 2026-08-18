# Step 05 - repeated VAEG launch and human gate

1. Launch a fresh VAEG session with the disposable D88 and capture an early
   frame before scene 0 advances, a scene 4 frame, and the grid scene.
2. If the screen is black, trace SGP start/status and command fetch first;
   distinguish mode rejection, missing SET_WORK, invalid pitch/address,
   empty command list, and late-finale capture before changing source.
3. If geometry is shifted or torn, inspect descriptor dot/address/pitch and
   DSA0/VBLANK order; do not change emulator timing or graphics semantics.
4. Repeat build, D88 installation and VAEG launch after each justified fix.
5. Confirm DOS ESC manually, video restoration, original eight-scene order,
   no permanent checkerboard, SGP LINE/PATBLT/CLS trace, and 640x400 mode.
6. Stop and request the maintainer human gate. Do not merge to `main` until
   that gate is explicitly passed.
