<!-- Copyright (c) 2026 Nakata Maho. SPDX-License-Identifier: BSD-2-Clause -->
# M100s2 — hardware H.264 / Matroska checkpoint

Contract: [OS-assisted plan](../../recording-v1/os-assisted-plan.md). Reuse the
existing libav boundary, seekable exclusive Matroska I/O and PCM encoder; retain
FFV1 tests and default behavior. First locally executable provider is macOS
VideoToolbox. Hardware-required open plus public encode/decode are mandatory;
provider listing is insufficient. Do not silently fall back or treat this as
live emulator A/V integration, distribution approval or another OS's proof.

Checks: H.264 config/decoder probe, production H.264/PCM public MKV fixture,
independent verifier, path collision preservation, archive ON/OFF regressions,
focused Werror/ASan/UBSan, repository encoding/EOL/case and whitespace checks.
Evidence and handoff: [status](../../recording-v1/os-assisted-status.md).
Sequential task-local progress authorized; LOCAL_ONLY, no routine approval stop.

Maintainer-requested build follow-up: the separate FFV1-enabled MinGW artifact,
minimal LGPL FFmpeg build, C/C++ header portability correction, checks and
runtime/distribution limits are recorded in
[MinGW FFV1 build evidence](../../recording-v1/mingw-ffv1-build.md). This enables
the existing archive path, not the unfinished Windows real-time H.264 provider.

Further maintainer feedback requests automatic dated output under `recordings/`,
clockwise video rotation, Japanese start/stop/settings and a RECORDING lamp left
of KANA/CAPS. Implemented with RGB rotation and panel-position tests; see
[follow-up evidence](../../recording-v1/recording-ui-followup.md). The separately
reported movie-only red tint remains OPEN pending affected-player/source
comparison; exhaustive RGB565 and independent Windows-produced public MKV
checks pass, but are not a private-guest reproduction or a human Windows gate.
