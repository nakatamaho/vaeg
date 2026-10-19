# Timing and audio integration

## T1. Why this is a separate workstream

The inspected public `sdl2/soundmng.c` callback calls `sound_pcmlock()`. The public
`sound/sound.c` also has callback-related generation and playback-buffer-limited
production, plus a conditional wave-recording path. These are integration clues,
not authorization to turn on or duplicate legacy WAV recording. REC00 must trace
actual local call sites and lock ownership. Sources: `sources.md` P4 and P5.

A tap at the SDL callback is not sufficient for guest-time recording. It can
contain host-driven padding, miss samples when output is disabled, or generate
samples according to callback demand rather than the movie timeline.

REC10 first extracts a tested production seam without changing existing call
order, waveform, clipping, buffer lengths, or clocks. REC11 then implements a
recording-only guest-time production path, sharing the same synthesis operations.
Do not combine these into a broad sound rewrite. Do not edit ymfm internals,
frequency constants, resampler quality, or sound-chip register semantics.

## T2. Clock domain

Record a descriptor for the authoritative guest clock: source, units, rational
frequency, wrap behavior, reset behavior, CPU-speed relationship, and video-boundary
relationship. Extend wrapping counters safely before arithmetic; never subtract
an unextended CPU counter and treat the result as globally monotonic time.

Use an exact internal time representation, such as integer ticks plus a rational
seconds-per-tick. Distinguish emulated hardware clock changes from host pacing
changes. The latter do not alter recorded duration. The former terminate the
current recording in v1 unless a separately approved clock-continuity contract
already exists. Never use the frontend's nominal FPS for encoding.

## T3. Sample grid and epochs

Let `F` be the positive integer output sample rate. Let `A` be the guest-time
origin of the canonical generated audio sample grid. Sample `m` belongs to time
`A + m/F`. For recording boundaries `T0 <= t < T1`, select samples by their grid
times, not by callback arrival:

```text
m0 = ceil((T0 - A) * F)
m1 = ceil((T1 - A) * F)
accepted sample indices = [m0, m1)
accepted sample count = m1 - m0
audio origin relative to movie = A + m0/F - T0
```

Compute this with checked integer/rational arithmetic. If audio begins on the
recording boundary by construction, the origin is zero; otherwise it is less
than one sample period. Preserve that bounded offset in timestamp calculation.
Do not reset chip phase or rewrite PCM to force zero offset. Audio codec PTS uses
integer sample counts; add the common-epoch offset during the single conversion
to muxer stream time. Tests must cover a start in the middle of a block.

Retain fractional sample debt across adjacent intervals. For any partition of
`[T0,T1)`, the sum of its sample counts must equal the formula for the whole
interval. Do not independently round `F * frame_duration` for every frame. For
nonintegral samples-per-frame, correct successive block sizes naturally alternate.

## T4. Recording entry and exit: do not reset the sound chip

Before changing producer ownership, synchronize with the old producer/callback
using existing approved locks. Inventory already-generated samples, reserved
playback data, generator cursor, and any callback-generated look-ahead. Map that
valid data to the same sample grid. Drain/use known look-ahead or wait for guest
time to catch up; do not synthesize it twice, discard it silently, or reset the
chip to hide a phase mismatch. A bounded arming state is acceptable. If the
existing code cannot establish a trustworthy cursor, implement and test the
missing accounting first; do not fake a zero cursor at start.

During recording, one guest-time owner produces each sample once. Immutable
blocks fan out to recording assembly and a separate playback FIFO. The device
callback can consume or produce playback-only underrun silence but cannot advance
synthesis or the movie cursor. On playback FIFO overflow, the device-monitor
policy may drop stale monitoring data, with counters; it may not change recording
sample ownership. Distinguish this from the lossless recording queue.

Synthesis must still advance during application mute, after which recorded samples
are zeroed. Device failure/absence must not stop guest-time production. If the
current mixer is initialized only after successful device creation, separate mixer
initialization from the device sink within REC11 and prove lifecycle behavior.
Do not implement a second independent mixer for headless recording.

At exit, stop accepting recording work, drain/finalize it, and transfer the
already-current generator state back to the playback scheduling policy without
replaying, skipping, or reinitializing guest sound state. Verify repeated start/
stop cycles and compare sound-register state before and after the transition.

## T5. Complete capture intervals

Recommended small-v1 coordinator: stage one frame at boundary `ti`, then pair it
with all PCM samples whose grid times fall in `[ti, ti+1)` when the next boundary
is known. Enqueue an owned `CaptureSlice` containing sequence, endpoints, image,
OSD snapshot identity, and the matching sample range. It is normal to have a
one-interval staging delay; this is not A/V drift.

A slice is the atomic acceptance unit for producer-side completeness. It prevents
audio and video queues from deadlocking while waiting for one another. A shared
budget covers queued slices, the active slice, pending readback buffers, and known
codec scratch allocations. An implementation with separate queues is acceptable
only if it proves equivalent boundedness, watermarking, and cancellation.

Each nonzero interval must have its real completed image, even if pixel content
is unchanged. A genuine displayed frame may repeat unchanged pixels; inventing a
frame to cover a failed acquisition is not allowed. Do not skip intermediate guest
compositions when the host frontend would normally frame-skip.

For normal stop while running, finish at the next complete interval boundary. For
stop while paused at a boundary, close at that already-known boundary and discard
only a zero-duration unaccepted candidate. Never advance emulation solely to
flush. A capture failure ends at the last complete accepted boundary and reports
an interrupted/failed recording, not a fully successful requested duration.

## T6. Container rescaling

Keep an internal ledger of exact endpoints and cumulative sample counts. After
`avformat_write_header`, read each stream's actual time base. Calculate the
quantized start and end from the same absolute epoch, then their difference for
duration. Avoid accumulating rounded durations or applying rescaling twice.
Use codec PTS and muxer PTS in separately named variables/types.

For the tested FFmpeg Matroska implementation, the video/audio stream time base
may be 1 ms. This bounds representable packet positions; it does not reduce the
PCM payload's sample rate or justify dropping samples. `ffprobe` container
`duration`, `nb_frames`, or `avg_frame_rate` alone is not the timing oracle.

Acceptance uses exact decoded counts and event positions. Let `qv` and `qa` be
actual stream timestamp quanta in seconds. Flash/impulse alignment must remain
within `qv + qa + 1/F` of the exact intended boundary, without a slope over time.
Any source-latency convention found in REC00 must be resolved and documented;
do not expand tolerance by one frame simply to make a wrong association pass.

## T7. Required focused tests

Use rational frame intervals whose samples-per-frame are nonintegral; the fixture
rate is synthetic and must not be labelled the actual VA refresh rate. Exercise
split blocks, silence, mute/unmute, irregular producer chunking, delayed callback,
no callback, delayed encoder, fast-forward, pause, and sample-grid-aligned and
unaligned start/stop. Compare exact frame IDs and PCM bytes between production
speeds. Run a multi-hour virtual clock test without writing a multi-hour movie,
and a smaller genuinely encoded multi-marker fixture.

The recording-off baseline must be unchanged by the production-seam refactor.
Recording-on guest instruction/register outcomes must match the same scripted
input run without recording; permitted differences are host scheduling and device
monitoring behavior, not guest hardware behavior.
