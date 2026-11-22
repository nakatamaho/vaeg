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

# VA printer and serial file backends

These SDL2 command-line options are session-only and work with scripted
runs. With no capture options there are no host device files.

| Option | Behavior |
| --- | --- |
| `--printer-out PATH` | Append raw printer bytes. |
| `--printer-busy-frames N` | Assert printer busy for the first N executed guest frames (default 0). Works without a capture file. |
| `--serial-in PATH` | Replay bytes from a seekable binary file of fixed length. |
| `--serial-out PATH` | Append raw serial transmit bytes. |

The PC-88VA Technical Manual, chapter 2, §2.2 (I/O port map), specifies
printer data at `0010h` and XPSTB/PBSY in bit 0 of `0040h`. Capture
samples the `0010h` latch on the falling edge of active-low XPSTB. PBSY
reads as 0 when ready and 1 when busy. Busy is a status input, not a
capture filter: a guest that strobes despite busy still produces a byte.
Capture includes any guest startup strobes. There is no printer command
interpretation or print rendering.

Either serial option attaches the backend. CTS, DSR and DCD are asserted;
there is no host flow control. Input arrives through the `0020h`/`0021h`
USART and its existing PIT-driven receive/interrupt path, at most one byte
per configured character interval. It waits while RX is disabled or the
receive register is unread, rather than simulating overruns. TX-enabled
writes are appended immediately; this is not bit-level or baud-paced TX.
A missing input or exhausted file supplies no bytes. A missing output
discards transmit bytes. Cassette routing still takes precedence when
selected. No PTY backend is implemented.

Capture writes are unbuffered and are not rolled back by resets or state
loads. Likewise, file position and the startup busy countdown are not
saved or rewound by guest reset/state load. Keep the input unchanged during
the run and use distinct paths, including via symlinks and hard links.
Open errors fail startup; read/write/close failures fail the session at
normal exit.

## Synthetic guest check

Assemble `tests/va_prnaux.asm` with `nasm -f bin -o PRNAUX.COM`, place it
on an MS-DOS 4.0 VA disk, and run it in V3 mode on each model. Supply an
input file containing exactly `VArx-0123456789` (no newline) and fresh
captures. Expect `PRNAUX PASS` on screen, the input pattern exactly in the
serial capture, and the byte sequence `00h` through `FFh` in the printer
capture after any startup bytes (measure those with a boot-only control).
The COM uses INT 89h raw output (AH=03h) and INT 8Ah timed receive and
character transmit; it is not an AUX DOS driver test.

Real hardware comparison and exact strobe timing: **NOT RUN**.
