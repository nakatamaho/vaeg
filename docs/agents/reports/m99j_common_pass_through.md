# M99j common pass-through coverage

Status: PASS

The common layer now provides a caller-owned conversion buffer for explicit
RGB565/ARGB8888-to-RGBA8888 conversion. It preserves row origin, pitch, and
frame metadata without allocating in the conversion function. This is the
format-normalization seam used by native backends; it does not alter the
emulator's raw RGB565 framebuffer or raw capture API.

The focused pass-through test covers:

- RGB565 conversion with known red and green pixels;
- short destination pitch and short destination buffer rejection;
- 4:3 aspect viewport calculation at 1920x1080 and point mapping;
- unavailable-presenter fallback results;
- repeated shutdown and recovery calls on the unavailable presenter.

The test is registered as `vaeg_librashader_pass_through` and runs together
with the M99h frame-input, raw-capture, and presenter-state tests. The final
run passed 4/4 tests.

The implementation intentionally does not perform any device calls, shader
compilation, preset parsing, or allocation. Those operations belong to the
platform presenter lifecycle milestones.
