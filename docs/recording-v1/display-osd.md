# OSD composition and Displayed acquisition

## D1. OSD extraction before native recording

REC08 inventories true guest-area overlays and separates semantic snapshotting
from their rendering. Inspect video/framebuffer diagnostics, frame counters,
messages, any drive/activity text, and status placement. Classification is based
on actual rendering, not a label such as "status". Preserve all existing enabled
OSD classes; do not implement only the easiest overlay and claim coverage.

Build one immutable frame-associated snapshot on the GUI/state-owning thread.
Copy strings and values, preserve ordering, clipping, alpha convention, enabled
flags and anchors. No references into transient ImGui draw data may escape.
Keep wall-clock-derived diagnostics as the values actually sampled; do not claim
such OSD produces bitwise-identical movies across host runs. Tests use an injected
fixed snapshot and controlled time.

For Native, reuse an existing CPU text/primitive renderer or implement a small
adapter for the **actual existing OSD primitive subset**. Reuse authorized font
assets/atlas data. An SDL software renderer over an owned surface is acceptable
if it avoids a live GPU/window requirement and is tested. Do not implement a
complete ImGui software renderer or downscale the full displayed window to obtain
native OSD. Live overlay rendering must retain its normal visual behavior.

Native OSD layout uses canonical image coordinates and a defined unit scale.
Anchor to guest viewport corners/edges, clip to its raster, and keep readable
existing font metrics. Host-side menu/status strips are not resized and inserted
into the native image. Alpha blending occurs once, with documented integer
rounding or the existing renderer's rule; tests cover zero, full and partial alpha.

## D2. Displayed frame contract

The acquisition call belongs after the final in-client overlay/GUI pass and
before Present/swap/presentation. Associate the image with the guest frame token
that caused that render. Reuse the actual final composition once, including
letterbox/menu/status regions, not merely the CRT's guest-image output texture.

If a presenter already exposes screenshot capture, inspect its scope and ordering
before reusing it. Do not change screenshot semantics as a side effect. The
existing QA/dummy rendered-screen path is not proof of real Displayed acquisition.
An unsupported presenter must return a stable capability error, not black pixels.

Synchronous readback is a permitted v1 implementation and must have measured
costs. Do not promise asynchronous behavior merely because a copy command queues
GPU work. Later rings/fences are a separate optimization after golden correctness.

## D3. SDL fallback

Use `SDL_RenderReadPixels` on the current completed main render target before
`SDL_RenderPresent`, on the renderer owner. Query actual renderer output pixels,
not window logical size. Respect pitch and explicitly convert channel order.
SDL documents this operation as slow [P7]. Restrict readback to active recording
or explicit tests. A software SDL window is valid; the dummy reconstruction path
is a unit fixture, not a platform Displayed acceptance result.

## D4. D3D11

Inspect the actual final backbuffer/target format and composition sequence.
Copy completed final output into a matching staging resource with CPU-read access,
then map only when GPU completion permits. Copy rows using `RowPitch`, not assumed
packed width. Resolve multisampling when required by the actual source and test
that path; reject unsupported format cases rather than reinterpret them.

Release/unmap on all paths. Keep immediate-context operations on its owner.
A synchronous baseline may wait, but must not hold locks needed by event handling
or the encoder. Verify distinct corner colors, channel order, row padding,
status/menu regions, resize, and device removal. A Wine build or SDL-D3D renderer
is not evidence for VAEG's separate native D3D11 presenter. [P8]

## D5. OpenGL

Read the actual final framebuffer with the correct read buffer before swap. Save
and restore read-FBO/read-buffer selection and all modified pixel-pack state,
including pack-buffer binding. With a nonzero pack buffer, the destination argument
is an offset rather than a CPU address; never inherit such state accidentally.
Use an explicit supported format/type, normalize bottom-up rows to top-down,
respect row packing, and check GL errors without clearing unrelated diagnostics.
Preserve context ownership and sRGB behavior; do not add a second gamma transform.
A PBO/fence ring is optional after the synchronous path is correct. [P9]

## D6. Metal

Trace the actual completed GUI/CRT composition. Ensure the color attachment is
stored before a blit/read; a discarded attachment is not a capture source.
Framebuffer-only drawables cannot be assumed readable. When needed, render once
to an appropriate intermediate final texture and copy the identical result to
the drawable and a CPU-visible staging buffer. Prove parity before claiming that
this still represents Displayed output.

Commit the command buffer before waiting/observing completion. Do not access a
buffer until completion, or wait on work that requires the waiting thread's next
unsubmitted operation. Respect bytes-per-row/format/storage-mode requirements;
managed storage on supported Intel configurations needs explicit synchronization
where required. Retain resources until completion and handle failed command
buffers/drawable loss. Do not call `getBytes` blindly on a private texture. [P10]

## D7. Geometry and temporal effects

Start with the actual final drawable dimensions. Never silently resize it to an
arbitrary 720p/1080p capture target. Disable avoidable geometry/presenter-changing
settings while Displayed recording is active; unavoidable changes finalize the
current file before acquisition with a new descriptor. No automatic segmentation.
Ensure original resizability/settings are restored after stop or failure.

During active recording the scheduler requests each guest frame's composition,
even if normal host pacing would skip it. The same CRT result goes to capture and
the window. UI-only redraws may occur but do not advance the movie's guest-time
sequence. Do not advance temporal shader history twice to make both Native and
Displayed screenshots. Fixed synthetic shader uniforms can be used for parity
tests; actual CRT output need not be bitwise identical across GPU vendors.

## D8. Evidence

Each backend has a readback milestone and a separate end-to-end milestone.
Readback compares normalized bytes to the **same target** populated by a known
fixture. End-to-end proves an actual VAEG client image, active CRT, OSD, host status
region, audio, seek, and source selection. Record backend/device/driver/library
versions for public synthetic tests. Keep private images and identities outside
tracked reports. A missing runtime remains unverified, not skipped-as-PASS.
