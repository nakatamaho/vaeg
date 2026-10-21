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

# M100s11 — post-recording MP4 export

## Assignment and gate

The maintainer requested background post-stop compression on macOS, Linux and
Windows, preferring OS encoding on macOS/Windows. G100s10 was explicitly accepted
and integrated before this task. This is the single new implementation milestone.
**G100s11 OPEN / HUMAN GATE.** No runtime success follows from a cross-build.

## Contract

- Keep the finalized guest-time FFV1/PCM MKV unchanged. Conversion is optional,
  never substitutes realtime recording, and runs after successful publication,
  including each completed Native split segment.
- Background H.264/AAC MP4, bounded worker/queue and conversion buffers, progress,
  success/failure/cancellation reporting; cancel outstanding conversions on exit.
- Preserve source timestamps (not wall time), full picture/aspect and audio.
  Even video dimensions, up to 1280x720, stereo 48 kHz AAC, no alpha in the upload.
- macOS AVFoundation/VideoToolbox and Windows Media Foundation encode through OS
  APIs; do not bundle their encoder implementations. Prefer those backends.
- Linux software encoding is an explicitly opted-in local-only static FFmpeg
  build using libx264 and native AAC. Default OFF; never build/publish it in CI or
  releases. Respect `AGENTS.md` local-codec policy, including license review.
- Final `.mp4` beside the MKV. Existing output (including symlinks) is an error,
  not an implicit overwrite. Work only inside an exclusively created temporary
  directory, publish without replacement, then clean up only this job's generated
  temporary file/directory. No guest media, user asset or original MKV is deleted.
- Guard default-OFF, explicit local enablement, known CI environment rejection,
  public/release preset rejection and static FFmpeg encoder allow-list. Static
  codec archives are inspected; lack of DLL imports is not sufficient evidence.

## Validation required before gate

- Focused passing fixtures and single-mutation negative tests with stable codes
  for export, no-overwrite, cancellation, missing backend and build policy.
- macOS native actual MP4 export/decode and MKV digest preservation; Linux private
  static export/decode plus normal build/export-OFF behavior; Windows strict
  MinGW compilation/clean static handoff and native Windows runtime human check.
- Recording ON/OFF regressions and applicable frontend selftests; repository
  encoding/EOL/case/Markdown/whitespace checks.
- Standard clean pushed-commit static MinGW handoff with executable/report paths
  and SHA-256. Demo/media/private-input gates remain required; mark unavailable
  checks NOT_RUN rather than inferring success from synthetic fixtures.
- No hosted CI iteration. Preserve task-specific logs, fixtures and expensive
  local dependency builds outside Git; document evaluated commit and worker
  identity. Do not publish private or codec-enabled binary/dependency artifacts.

## Status

Implementation in progress. No MP4 export, license/distribution or new runtime
acceptance is claimed yet.
