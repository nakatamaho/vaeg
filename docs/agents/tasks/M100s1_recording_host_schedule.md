<!-- Copyright (c) 2026 Nakata Maho. SPDX-License-Identifier: BSD-2-Clause -->
# M100s1 — recording host schedule checkpoint

Assigned recording-only extension of the integrated recorder. Contract and legal
checkpoint mapping: [OS-assisted plan](../../recording-v1/os-assisted-plan.md).
Implement monotonic nanosecond/60Hz schedule and bounded latest owned image
admission without changing archive clocks or existing CLI behavior. Do not
claim a playback clock adapter or live real-time recording from these primitives.

Machine checks: focused host schedule test; recording ON/OFF builds and recording
CTest labels; focused Werror/ASan/UBSan; encoding/EOL/case and whitespace checks.
Evidence and handoff: [status](../../recording-v1/os-assisted-status.md).
Sequential task-local progress authorized; LOCAL_ONLY, no routine approval stop.
