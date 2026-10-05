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


# M103j - Cassette tape (vaeg extension)

Status: **in progress**

Series: V1/V2 mode (ADR-0016), closing milestone. Branch
`topic/m103j-cassette` off `main` at
`98f7eb066c33a24f322658789883d49cb7fcfed8` (M103a–M103i merged). Commit
prefix `M103j:`.

## Goal

Let V1/V2 BASIC load and save programs and data on cassette tape images
(`LOAD "CAS:..."`, `BLOAD`, `SAVE "CAS:..."`), as on a PC-8801
(maintainer request). The VA has no cassette interface, so this is a
vaeg extension, like the N mode.

## Background

- `[VA-TM]` Ports 20h/21h are the µPD8251 for RS-232C only; port 30h
  output defines only bit 0 (80CM) and bit 1 (MCM). The PC-8801 shares
  the 8251 with the cassette and selects it, and switches the motor,
  with port 30h bits 2–5. A real VA cannot load from tape.
- `[ROM]` The VA's N-88 BASIC ROM still contains the PC-8801 cassette
  code: writing polls TXRDY (21h bit 0) and outputs to 20h (7FD0h);
  reading takes bytes from a buffer filled by the 8251 receive interrupt
  handler (3167h: `IN 21h` RXRDY, `IN 20h`); `MOTOR` sets port 30h bit 3
  through the shadow at E6C0h (7F20h–7F4Bh), and port 30h bits 4–5
  select the 8251 channel.
- `[X88000]` (behaviour reference only, ADR-0016): channel 00 = CMT
  600 baud, 01 = CMT 1200 baud, 1x = RS-232C; bytes arrive while the
  motor is on and receive is enabled; T88 tape images carry tagged data,
  blank, space and mark blocks.

## Scope

1. **Tape images**: load raw byte images (`.cmt`) and T88 (`.t88`);
   save to a raw image.
2. **8251 cassette channel** in 88 mode: port 30h bits 3–5 (motor,
   channel), receive at the selected baud rate with RXRDY and its 8214
   interrupt, transmit to the save image. RS-232C and V3 unchanged.
3. **Frontend**: a cassette menu (set, rewind, eject, save image) and a
   command-line option for headless tests.
4. **Tests**: a romless round trip of the 8251 path, and a BASIC `SAVE`
   then `LOAD` through a tape image.

## Gate G103j (human)

Standard V3 gate unchanged, plus: a BASIC program saved to tape loads
back; the maintainer's tape images load in V1/V2 BASIC.

## Implementation progress

- Task started.
