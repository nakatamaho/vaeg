<!-- Copyright (c) 2026 Nakata Maho. SPDX-License-Identifier: BSD-2-Clause -->
# M100s5 — ScreenCaptureKit window adapter checkpoint

Contract: [OS-assisted plan](../../recording-v1/os-assisted-plan.md). Single actual
main-window filter, runtime/SDK guards, explicit consent, owned bounded buffers,
source timestamp units, cursor exclusion and shadow policy. Never substitute
monitor capture or infer verified decoration coverage from successful compilation.
Do not alter global deployment minimum, private assets or the renderer.

Checks: ON/OFF build, noninteractive capture contract test, focused Werror and
sanitizer. Public titled-window runtime probe is opt-in and must preflight consent
without requesting permission. Denial/unavailable runtime is NOT_RUN/BLOCKED,
not PASS. Resize/Retina/revocation/window lifetime and decorated coverage require
real runtime evidence before enabling the frame choice.

Evidence and handoff: [status](../../recording-v1/os-assisted-status.md).
Sequential task-local progress authorized; LOCAL_ONLY, no routine approval stop.
