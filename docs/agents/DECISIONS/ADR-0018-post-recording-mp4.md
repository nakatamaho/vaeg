<!--
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
-->

# ADR-0018 — optional post-recording MP4, OS-first and local-only software codecs

Status: implementation decision for M100s11; human/runtime gate OPEN.

## Decision

Keep finalized FFV1/PCM MKV as the immutable archive. Decode that local file on a
background worker and export a separate H.264/AAC MP4 with original media time.
The software-codec option is independent of Native/Displayed acquisition and is
not realtime H.264 recording or an audio-host-clock adapter.

Use AVFoundation (VideoToolbox/system AAC) on macOS, and Media Foundation system
H.264/AAC on Windows. Those encoder implementations are not linked into vaeg.
Windows loads Media Foundation from System32 at conversion time; its absence
must not prevent startup or MKV recording. Hardware acceleration is preferred
by the OS where available; do not claim a particular device/encoder was selected
without runtime evidence. Ordinary CI/release builds contain these API adapters,
not bundled H.264/AAC encoder implementations. This is not patent clearance or
an assurance about the OS vendor's licenses covering every application/use.

Linux's optional software backend requires explicit `VAEG_ENABLE_LOCAL_MP4_ENCODERS`
(default OFF), static FFmpeg and `VAEG_LOCAL_MP4_ACKNOWLEDGEMENT=LOCAL_ONLY_NO_DISTRIBUTION`.
It uses exactly libx264 and FFmpeg native AAC. Known CI environments and all
CI/release/clean-handoff presets reject enablement, even if the public-build
cache boolean is overridden. Normal static FFmpeg permits FFV1/PCM encoders
only; configure-time and pre-build archive audits reject extra registrations.
The local option requires the exact additional libx264/AAC registrations. Resolve
libx264 to an exact static archive as well; `--static` pkg-config alone is not
proof that `-lx264` did not select a shared library.

## Local dependency identities and license boundaries

No encoder source/library is vendored or committed. Locally built dependency
stages, executable/object files, SDKs and caches must never be uploaded to CI,
release staging or public downloads. Public recipes retain their minimal LGPL
FFV1/PCM codec configuration. The local-only development profile is pinned to:

- FFmpeg **8.1.2**, <https://ffmpeg.org/releases/ffmpeg-8.1.2.tar.xz>, SHA-256
  `464beb5e7bf0c311e68b45ae2f04e9cc2af88851abb4082231742a74d97b524c`.
  FFmpeg defaults to LGPL-2.1-or-later; enabling GPL libx264 changes the resulting
  local FFmpeg build's license. Disable nonfree/version3/autodetection/network.
- x264 **0.164.3108 / git31e19f9**, Ubuntu 24.04 arm64 package
  `libx264-dev` **2:0.164.3108+git31e19f9-1**, upstream
  <https://www.videolan.org/developers/x264.html>. The published arm64 package
  SHA-256 is `213534d13ff7afd55155ae022d972faa6d2c343fcebb1e4517f37d7193b1b6d0`.
  Package copyright identifies **GPL-2-or-later** and separate commercial licensing.
  This profile uses the GPL package locally, not a commercial license.
- AAC is FFmpeg's native `aac` encoder, not libfdk-aac or a downloaded AAC SDK.

These are public upstream/package identifiers, not private integration assets.
The permission to compile locally does not authorize distributing the combined
executable, settle compatibility of every source license for redistribution, or
settle patents in any jurisdiction. Re-review any different implementation,
version, package or intended distribution. No binary or codec licensing approval
is supplied by this ADR.

## Resource, lifecycle and publication rules

One conversion at a time, four pending paths maximum; a full queue is an explicit
error and leaves the MKV. Each successful nonempty published split is eligible.
Record the UI choice at recording start. GUI recreation does not kill conversion;
application exit cancels/joins pending work (MKVs remain). A running recording
and its guest-time contract are independent of export success.

Use bounded decoded frames, nearest-neighbor resize to even dimensions fitting
1280x720 without cropping the source, BT.709 limited-range NV12 where required,
stereo 48 kHz resampling and 128 kbit/s AAC. Retain source PTS/durations, not elapsed
host time. macOS needs independent video/audio producers: use two read-only file
passes instead of an unbounded demux queue or a single-producer readiness deadlock.
End each input independently before waiting for asset-writer completion.

Work inside an exclusively created sibling temporary directory. Publish only
after successful encoder/muxer close, using no-replacement MoveFileEx on Windows
or atomic hard-link publication on POSIX. On filesystems without hard-link support,
fail explicitly rather than substitute an unsafe overwrite-capable operation.
Clean only the job's generated file/directory, never recursively delete unknown
contents. An existing destination, including a dangling symlink, is an error.
The archive and any existing destination remain untouched on failure/cancel.
