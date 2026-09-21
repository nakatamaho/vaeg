# M100r11 - REC10 recording-v1 audio production seam

Status: MACHINE_GATE_PASS
Feature: REC10
Scope: legacy-preserving sound generation, look-ahead and PCM packing seam

This wrapper assigns REC10 of recording-v1 to legal repository ID M100r11.
Read `docs/recording-v1/milestones/rec10-audio-seam.md` and its named
references together with the applicable root instructions before acting.

The implementation is limited to extracting the existing stereo S32 generator
dispatch, reserved-tail look-ahead calculation and S16 saturation/silence
packing into a small C99 operation. It must preserve callback order, sample
lengths, saturation, sound-chip state, SDL callback behavior and recording-OFF
behavior. It must not add recording integration, a new sample-rate algorithm,
host callback capture, an ymfm change or a chip reset. The current authorized
recording-v1 run may continue after machine evidence is recorded; this does
not waive later platform, human or release validation.
