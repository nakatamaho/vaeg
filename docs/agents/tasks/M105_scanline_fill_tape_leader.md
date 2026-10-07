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


# M105 - 15 kHz scanline fill and the tape leader sound

Status: **in progress**

Series: V3 series (scanline fill) with a V1/V2 cassette follow-up
(maintainer decision 2026-10-07). Branch
`topic/m105-scanline-fill-tape-leader` off `main` at
`2508ccf47e6a10d9c2b67d1fad62b5a3c7522491`. Commit prefix `M105:`. The CPU
speed gap found in M104 moves to M106.

## Goal

1. **15 kHz scanline fill.** At 15 kHz (non-interlaced) vaeg shows each
   line on the even raster of a pair and leaves the odd raster black, as a
   CRT shows its scanline gaps. An LCD that accepts 15 kHz doubles each
   line instead (maintainer observation, M104 photographs). Add a monitor
   setting: gaps (CRT, default, the current picture) or filled (LCD), saved
   in the configuration. Interlaced modes and 24.8 kHz are unchanged.
2. **Tape leader sound** (vaeg cassette extension, C10). When the motor
   starts for reading a raw or T88 image, play 1200 Hz for one second and
   then 2400 Hz for one second before the data, and hold the data back for
   those two seconds so that BASIC prints `Found` after the sound, as the
   maintainer remembers a PC-8801 ("low, then high, then Found"). With
   fast load (`CMT_Fast`) there is no leader sound and no delay. Saving is
   unchanged. `[POLICY]` (maintainer memory; no document at hand).

## Gate G105 (human)

Standard V3 gate unchanged, plus: at 15 kHz the new setting fills the
scanline gaps and the default keeps them; loading a tape without fast load
plays low then high before `Found`; with fast load it loads at once.

## Implementation progress

