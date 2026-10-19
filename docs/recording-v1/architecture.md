# Architecture and ownership

## A1. Boundaries

Keep the existing C core C99. Use C++17 only in the frontend area already allowed
by the repository. A small C-compatible tap/control interface may cross the
boundary; no STL, exception, AVFrame, AVPacket, SDL_Renderer, or GPU object may
leak into sound/video core contracts or saved state. Do not create a new broad
engine framework, service locator, generic media plugin system, or process manager.

Proposed ownership, subject to mapping existing equivalent files in REC00:

```text
C guest video/audio production
    -> small C-compatible callbacks and exact timestamps
C++ frontend recording coordinator
    -> owned video + OSD snapshot + matching PCM CaptureSlice
bounded lossless queue
    -> recorder worker, sole owner of FFmpeg and output I/O
    -> partial Matroska file -> finalized Matroska file

final presenter composition -> readback on presenter-owning thread
    -> owned Displayed pixels -> same coordinator
```

Native composition reads the canonical frame before presentation and uses a
recording-owned destination for OSD. The live renderer, screenshot subsystem,
and recording do not share writable frame storage.

## A2. Small data contracts

Define only what the implementation needs. At minimum:

- `RecordingSource`: NativeVideo or DisplayedVideo.
- `VideoDescriptor`: width, height, explicit channel layout, bytes per pixel,
  row stride, orientation, and source-generation identity.
- `AudioDescriptor`: effective rate, channel order/count, sample representation.
- `GuestTime`: exact checked rational/tick-domain value, not host milliseconds.
- `OsdSnapshot`: owned content, enabled-layer list, anchors, clipping and frame ID.
- `CaptureSlice`: sequence, interval endpoints, owned video buffer, audio sample
  start/count/data, and immutable descriptor-generation identities.
- `RecorderStatus`: lifecycle, source, accepted/written counts, guest duration,
  queued bytes/high-water mark, stable error code, and public-safe message.

These are conceptual names, not instructions to create parallel versions of
existing repository types. `file-layout.md` supplies suggested private modules.
Frame/audio storage must outlive the worker's last access. Prefer move-owned
buffers or a bounded pool; do not queue pointers into SDL textures, sound
buffers, ImGui transient allocations, or mapped GPU staging memory.

## A3. State machine

```text
Idle -> Preparing -> Armed -> Recording -> Stopping -> Finalizing -> Idle
                  \-> Canceled -> Idle
any active state -> Failed -> acknowledged cleanup -> Idle
```

Preparation validates codecs, destination, descriptors, backend capability and
memory budget before mutating recording state. Arming finds a trustworthy common
guest boundary and reconciles generated audio look-ahead. Preparing/Armed must
not falsely display Recording or claim a final output file already exists.

Only the coordinator initiates session transitions. The worker posts completion
and errors. Repeated Stop requests are harmless. A second Start returns a stable
busy error, never an implicit close/reopen. Failed state retains the error and
partial-file disposition until shown to the user. Teardown must not erase the
only failure evidence.

## A4. Thread rules

The presenter and GUI stay on their owning thread/context. The encoder worker
must never call SDL rendering, ImGui, OpenGL, D3D11 immediate-context, or Metal
presentation functions. FFmpeg calls and output I/O stay on that one worker.
Audio callbacks do not encode, allocate unbounded memory, access the UI, wait for
video, or block on the recording queue.

The coordinator assembles complete intervals and waits for queue capacity only
at an emulation scheduling safe point with **no sound, guest-state, GUI, or GPU
resource locks held**. Pump stop/cancel/window events while waiting without
recursively advancing emulation or recording another frame. The worker must be
able to make progress without acquiring a lock held by that wait.

Use condition variables with predicates and explicit cancellation/error wakeups,
not polling loops. Close producer admission before draining. Cancel/stop wakes
all waiting participants. Do not join the worker from a lock it may need, and do
not detach a worker that still references application state. A blocked OS storage
call is not guaranteed interruptible; describe this honestly and never use forced
thread termination as a correctness mechanism.

## A5. Bounded memory and backpressure

Set a conservative checked byte budget and a frame/interval count bound. Derive
the maximum admitted frame size, queued count and staging allocations at start.
Account for stride, PCM, OSD snapshots, the active slice, and backend readback.
FFV1 scratch memory is additional and must be estimated/measured, not hidden
behind a claim that queue bytes equal total memory.

The default must admit at least one interval for a supported recording size.
A too-large requested drawable is rejected clearly before recording. Allocation
failures are recoverable errors, not silent downscaling. Do not grow queues when
the encoder slows down. Successful lossless recording can slow host execution;
there is no v1 promise of real-time 4K CRT + FFV1 throughput.

The recording-off fast path must not allocate snapshots/readback surfaces, start
workers, change frame skipping, or synthesize extra audio. A cheap disabled check
is acceptable. A compiled-OFF build must not reference FFmpeg symbols.

## A6. Testability without fake integration claims

Use a fake sink for queue/lifecycle tests, deterministic pixel/OSD/audio fixtures
for encoder tests, and a narrow readback adapter for each GPU backend. Dependency
injection is only for these boundaries. No test switch may silently replace a
production Displayed source with Native or manufacture a PASS for unavailable
hardware. Production capability reports and tests must distinguish unsupported,
not built, no device/context, and readback failure.

Failure injection is private to test targets. Define stable error codes and test
one controlled failure at a time, starting from a valid fixture. Do not copy a
large history-validation framework into this feature. Existing repository checks
still apply, but timing/content unit tests must not require private media or Git
history to exercise their intended invariant.
