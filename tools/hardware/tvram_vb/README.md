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
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
-->

# M142 — TVRAM writes during VB: discriminating hardware measurements

[Issue #75](https://github.com/nakatamaho/vaeg/issues/75) remains **OPEN**.
This kit is synthetic, source-built MS-DOS software, **not a timing fix**.
No hardware run or causal explanation is implied by generating the disk.
The committed TSPSPR source and distribution are not changed by this tool.

## Current receipt and investigation

[Ten returned DONE screens](../../../docs/agents/reports/m142_tvram_vb_hardware.md)
provide all 240 A–X rows; no additional photographs or hardware runs are
currently requested. The missing-input blocker is lifted. The
[instruction-cost/SYNC follow-up](../../../docs/agents/reports/m142_tvram_vb_events.md)
uses the unchanged worker to locate extra-copy thresholds and distinguish a
prior 200-line selection from an unconditional VA hold. The cause remains
OPEN and correction NOT_IMPLEMENTED. The protocol below documents the kit;
it is not a renewed request to run every variant.

## Why chunk and entry controls matter

The published VA2 Q/S values, 602/20 versus 875/21, do not identify a memory
wait cost. Q counts **completed 128-byte chunks**, checking VB only after each
chunk; the final counted chunk can finish outside VB. Its entry waits for VB
set, not a low-to-high edge, and TVRAM mapping and interrupt masking precede
the poll. A shorter blank can therefore magnify chunk quantization and entry
phase/latency, as well as any real memory-arbitration difference. Row S is a
status-poll fraction, not an independent absolute-time measurement. Do not
turn the 1.45 count ratio into an unexplained wait or multiply all TVRAM costs.

Issue #74's separately observed VRTC/VB phase mismatch is another reason to
keep phase and throughput hypotheses separate. It is not proof of #75's cause.

The kit varies chunk size and whether entry accepts an already active VB or
requires a complete next VB, while retaining ordinary-word, byte, G1, short-
and long-blank controls. Regressions against chunk size can discriminate a
per-chunk overhead from a one-time entry loss; they do not alone prove a
physical bus signal, a specific TSP fetch, or an absolute cycle count.

## Build

```sh
python3 tools/hardware/tvram_vb/test_build.py
python3 tools/hardware/tvram_vb/build.py --output /path/outside/checkout/tvram-vb \
  --msdos4 /path/to/pinned/msdos4-pc88va-2hd.d88
```

The output directory must be new and outside the checkout. `--msdos4` is
optional; when supplied it must be the checksum-pinned FreeDOS-88VA
`msdos4-va.2` release. Only its vanilla system files/license and the generated
COMs/help are stored on the new disk. No ROM or private guest data is needed
to build. NASM and Python 3 are required. The manifest binds all nine COMs,
the unchanged source, assembler version and optional disk digest; generation
reports `hardware: NOT_RUN`.

## Run on the PC-88VA2, then the VA control

1. Boot `tvram-vb-msdos4.d88` in the normal 15 kHz native mode at the machine's
   standard clock. Record the model, clock, display setting and disk identity.
2. For the unchanged reference, run `TSPSPR 1`, press SPACE four times, and
   photograph screen 5 after `DONE`. The machine is identified from the ROM
   word at F000:FFFE, as in the original demo.
3. Reset/reboot between variants; each takes over the display/interrupt
   vectors and does not return to DOS. Run these with argument `1` initially:

   | Current-VB entry | New-edge entry | Bytes per counted chunk |
   |---|---|---:|
   | `V008A 1` | `V008E 1` | 8 |
   | `V032A 1` | `V032E 1` | 32 |
   | `V128A 1` | `V128E 1` | 128 |
   | `V256A 1` | `V256E 1` | 256 |

   Variants start screen 5 automatically. Photograph **every row** after DONE;
   both the variant ID and ROM-reported machine remain in the heading. Keep
   Q, R, S and W together with I–P/U (word-access controls), E–H/V (bytes) and
   T (SPRON). X is the long-SYNC status-poll control. Do not copy just Q/S.
4. Start with the VA2 stock reference and 32/128-byte A/E pairs. If those
   expose an entry effect, retain all observations before changing another
   parameter. Complete the other sizes and the VA control to distinguish a
   repeatable size/entry effect from a single-run fluctuation. Increase passes
   only for a demonstrated need; keep original photos/results unmodified.

A and E contain the same five-byte entry slot: five NOPs for A, or an
`IN/TEST/JNZ` wait-for-low loop for E, before the common wait-for-high loop.
The chunk immediate is always 16 bits; fixed-width labels and the common
screen-5 autostart keep code/data addresses identical across the eight
variants. The unchanged TSPSPR baseline has **no added entry slot/autostart**;
never relabel an A result as the original measurement. Changing entry policy
necessarily changes executed polling history: it is a controlled intervention,
not a direct oscilloscope observation.

## Acceptance boundary

A correction requires a demonstrated mechanism with predictions that survive
both machines, short/long blank and the non-Q controls. The returned hardware
results are now available and analyzed, but do not yet establish that mechanism.
Emulator runs only establish software execution/readout contracts, not hardware
timing. No fitted 1.45 slowdown, random phase adjustment or private-input lookup
is permitted. Missing photographs no longer block analysis; the unresolved
cause/correction requirement still blocks completion of this M142 item.
