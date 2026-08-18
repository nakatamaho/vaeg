<!--
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
-->

# NEON4 PC-88VA port master plan

## Goal

Create a separately buildable PC-88VA guest demo derived from the scene and
music intent of `demos/NEON4_1_0`, replacing PC-9801 GRCG/EGC/PEGC, VRAM
windows, PC-98 VSYNC IRQ, text VRAM, and direct sound probing with verified VA
interfaces. The original untracked PC-98 directory remains unchanged.

The first deliverable is a launchable 16-bit `.COM` using a 320x200 4-bpp VA
G0/G1 composition and SGP `LINE`/`CLS` command lists. It is not a claim that
all original 640x400 PEGC scenes are already ported.

## Evidence and review

The hardware difference table and adversarial review are in
[`docs/modernization/neon4-pc98-va-port-audit.md`](../docs/modernization/neon4-pc98-va-port-audit.md).
The execution steps are split below; each step names its inputs, outputs,
allowed files, invariants, and test.

| Step | File | Outcome |
|---|---|---|
| 0 | `.plan/neon4-va-00-baseline.md` | Freeze source/reference inventory and tool contract |
| 1 | `.plan/neon4-va-01-video.md` | Build VA mode, palette, G0 background, and safe teardown |
| 2 | `.plan/neon4-va-02-sgp-scene.md` | Render a static and animated NEON wire/solid scene with SGP LINE/CLS |
| 3 | `.plan/neon4-va-03-input-sound.md` | Add ESC and optional VA Music BIOS adapter without harming graphics |
| 4 | `.plan/neon4-va-04-build-run.md` | Add reproducible NASM/CMake build and launch instructions |
| 5 | `.plan/neon4-va-05-iterate.md` | Build VAEG, launch the COM, capture failures, and iterate until launchable |

## Adversarial review of this plan

The plan was reviewed against the original NEON4 source, the PC-98/VA
documents, the existing working SGP demo, and the current VAEG source. The
following attacks changed the plan before implementation:

| Attack | Risk found | Plan correction |
|---|---|---|
| A “port” could silently become a wholesale rewrite with no NEON identity | Geometry/music intent could be lost while the result is merely an SGP test | Step 02 explicitly reuses one NEON geometry chapter and records which data is retained or deferred |
| Direct VA addresses might be guessed from the manual alone | A wrong GVRAM page or word-port width can hang real hardware | Step 00 requires comparison with the already-running VA SGP demo; Step 02 forbids byte writes to word SGP address ports |
| VA graphics BIOS and direct DSA page exchange could race | BIOS-owned window state and SGP-owned drawing state are separate | Step 01 checks every BIOS status; Step 02 keeps page ownership explicit and waits for SGP then VBLANK |
| “Save/restore video” could overwrite a user’s pre-existing state | The available video-state bytes and write-mode latch are not a generic PC-98 API | Step 01 treats restoration as a verified helper, not an invented snapshot format; a failure becomes a documented blocker |
| The Music BIOS might not be present in every ROM set | A sound failure could mask a graphics failure | Step 03 is graphics-first, optional, and has a no-sound path; it requires `INT 8Bh` initialization before any playback |
| The VAEG LINE direction conflict could make only some edges correct | Symmetric test lines can hide reversed octants | Step 02 requires an asymmetric self-test and records the selected bit profile |
| VAEG success could be reported as real-PC-88VA success | Emulator timing/BIOS behavior is not a hardware measurement | Step 05 separates launchability, VAEG observation, and the human hardware gate |
| Adding the entire untracked reference tree would pollute the port | COM/MID/S98 payloads and PC-98 code would become accidental deliverables | The port is a small separate `demos/neon4-va/` directory; the source reference stays untouched |

The remaining deliberate risk is the exact original-VA behavior of LINE
direction and any Music BIOS coverage not present in the selected ROM. The
plan stops and records evidence if either is required for a claim; it does not
invent a compatibility shim.

## Global implementation rules

- Do not edit or delete `demos/NEON4_1_0/`, `docs/98io/`, or
  `docs/tekumani/`; they are reference material in this task.
- Do not add a PEGC/GRCG/EGC compatibility layer, guessed VA port, CPU pixel
  loop, or PC-98 `INT 0Ah` handler.
- Keep all command lists and the 58-byte SGP work area in main RAM.
- Use word I/O for SGP command-pointer ports `0500h` and `0502h`; keep
  `SET_WORK` before drawing; poll SGP busy before page ownership changes.
- Use only interfaces supported by the current VAEG tree or documented in the
  audit. If a call returns an error, report the exact call and stop guessing.
- New source, scripts, and docs use the repository 2-clause BSD header. Keep
  NASM symbols/comments/diagnostics in English.
- Preserve the original scene geometry and musical intent where possible, but
  replace hardware-specific data structures instead of retaining misleading
  names.
- Generated COMs and disk images stay outside Git unless explicitly requested;
  never modify ROM or disk payloads.

## Completion definition

The port is complete for this plan when `NEONVA.COM` assembles, VAEG builds,
the program starts in the documented run procedure, shows the VA background and
animated NEON geometry, exits on ESC, restores the video state, and the final
report separates verified VAEG behavior, hardware-document facts, and open
hardware questions. Full original feature parity is a follow-up, not a hidden
acceptance condition.
