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
EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.
-->

# Master plan: faithful NEON4 PC-88VA 640x400 port

## Scope

Rewrite the local PC-9801 `demos/NEON4_1_0` 16-colour geometric demo as a
16-bit NASM COM for the PC-88VA. Preserve the source's eight-scene order,
384-frame chapter timing, geometry families, palette intent, and scene-local
carrier/raster/grid elements. The required output mode is VA 640x400 4bpp.

The central conversion is operation-level, not numeric-register translation:
PC-98 GRCG/EGC/GDC-style line/span/rectangle work becomes SGP command-list
LINE/PATBLT/CLS work. The source reference and local hardware references are
read-only. Generated COM/D88 media remain outside Git.

## Step files

| Step | Detail file | Deliverable |
|---|---|---|
| 00 | `.plan/neon4-va-640-00-audit.md` | 640x400 evidence freeze and GDC/SGP conversion table |
| 01 | `.plan/neon4-va-640-01-video.md` | VA 640x400 G0 mode, framebuffer descriptors, palette and restore |
| 02 | `.plan/neon4-va-640-02-sgp.md` | SGP SET_WORK/CLS/LINE/PATBLT emitters and descriptor tests |
| 03 | `.plan/neon4-va-640-03-scenes.md` | All eight scenes in 640x400 coordinates and original timing |
| 04 | `.plan/neon4-va-640-04-input-build.md` | DOS ESC, NASM/CMake build, disposable D88 |
| 05 | `.plan/neon4-va-640-05-iterate.md` | Repeated VAEG launch, visual/dump correction loop, human gate |

## Things that may be changed

- Add `demos/neon4-va-640/` NASM source, build script and README.
- Add the audit report and plan files named above.
- Add one explicit CMake guest target for the COM.
- Generate disposable D88 images only under `/private/tmp`.

## Things that must not be changed

- Do not edit or delete `demos/NEON4_1_0/`.
- Do not add PC-98 ports (`007ch`, `007eh`, `04a0h-04aeh`, PEGC MMIO),
  GDC/EGC register probes, IRQ2/INT 0Ah hooks, or guessed VA ports.
- Do not modify emulator core, ROMs, disk images, fonts, or local reference
  documents to make the demo pass.
- Do not draw animated pixels with CPU loops. CPU builds SGP descriptors;
  SGP writes GVRAM.
- Do not claim SGP SCAN or polygon fill support that vaeg marks unresolved.

## Review gates

The audit and plan are saved before implementation. Each implementation step
must pass NASM and the relevant VAEG launch check. The final step explicitly
repeats build, D88 install, boot, screenshot/GVRAM observation, and source
correction until the COM launches and scene commands are observed. Real-board
validation remains a maintainer human gate.

## Adversarial plan review

| Risk | Countermeasure |
|---|---|
| Lower model silently falls back to 320x200 | Hard constants, report table, and a VAEG mode-state check require 640x400 |
| GDC ports are copied into the VA source | Closed port allowlist in Step 02; static grep rejects PC-98 addresses |
| SGP descriptor uses 160-byte pitch | Step 01 computes 320-byte pitch and two 128KiB G0 pages |
| PATBLT is mistaken for polygon fill | PATBLT is restricted to rectangle spans; triangle differences are documented |
| Command list is in GVRAM or missing SET_WORK | Static parser checks main-RAM labels and first command |
| DSA is changed outside VBLANK | Step 01 requires TSP status low-to-high polling before each DSA0 write |
| ESC works only through host injection | DOS function 06h path is retained and manual ESC is a gate item |
| A screenshot is taken after the finale and looks blank | Step 05 captures scene index/time and early scene frames as well as the final scene |

## Completion

Complete when the 640x400 COM builds, its command stream is accepted by a
fresh VAEG session, SGP trace/dump shows CLS/LINE/PATBLT against G0 pages,
the eight scenes advance in the original order, ESC restores DOS, and the
disposable D88 contains only the system files plus the new COM. Stop for the
maintainer's human gate before merging.
