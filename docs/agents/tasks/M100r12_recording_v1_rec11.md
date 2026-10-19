# M100r12 - REC11 recording-v1 guest-time audio producer

Status: MACHINE_GATE_PASS
Feature: REC11
Scope: guest-time recording audio ownership and playback-only device consumer

This wrapper assigns REC11 of recording-v1 to legal repository ID M100r12.
Read `docs/recording-v1/milestones/rec11-audio-tap.md` and its named
references together with the applicable root instructions before acting.

The implementation uses the REC10 C99 generation seam exactly once from
guest-time `sound_sync` while recording is armed. It copies final S16 stereo
samples into a bounded owned FIFO with sample indices and epoch, and makes the
SDL device callback consume playback data only. Existing non-recording
`sound_pcmlock` look-ahead remains unchanged. SDL device absence is not
application mute and no device is required to initialize the sound core.

The mapped machine checks and evidence are recorded in
`docs/agents/reports/recording-v1/rec11-report.md`. This is not human or
release validation and does not claim a live MKV, presenter, VLC or
cross-platform runtime result.
