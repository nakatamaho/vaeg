<!--
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
-->

# Intelligent FDC monitor: RAM diagnostics interface

M139 independently written analysis, not a NEC manual reproduction.
This is a subsystem protocol, not a main-CPU BIOS interrupt ABI.

## Evidence boundaries

- **Manual specification:** not established here.
- **Static ROM findings:** monitor family represented by MAME's `pc88va2`
  `vasubsys.rom`, published SHA-1
  `a9375aa480f85e1422a0e1385acb0ea170c5c2e0`. See the
  [published reference and MAME source link](../agents/tasks/M18_rom_layout.md#mame-checksum-reference).
  The following work addresses are specific to that family, not a promise
  about every intelligent FDD ROM or board revision.
- **Emulator runtime:** a ROM set matching MAME's `pc88va2` supports upload,
  execution, return to command service, RAM readback and subsequent CP/M disk
  operations in the M139 local software-integration cases. Exact evidence and
  evaluated tool identities are in the [task](../agents/tasks/M139_upd780_or_upd9002_check.md).
- **Physical hardware:** not yet measured by M139. Do not promote the source
  model, synthetic monitor or main-CPU CP/M execution to physical FDC evidence.

## Byte transport

Both ends use 8255 mode `91h`: A input, B output, upper C output and lower C
input. A and B are cross-wired. The main CPU therefore writes data at FDh and
reads data at FCh; FEh reads C and FFh performs mode/bit-set-reset operations.

Upper-C output bits are DAV (4), RFD (5), DAC (6), ATN (7); the corresponding
signals from the other side appear in lower C. A sender waits for peer RFD,
places a byte on B, asserts DAV, waits for DAC, drops DAV and waits for DAC to
clear. A receiver raises RFD, waits for DAV, drops RFD, reads A, raises DAC,
waits for DAV to clear and drops DAC. A command wakes the monitor with ATN;
ATN is cleared once RFD is observed, before sending its command byte.
Unexpected ATN during a monitor transport wait can abandon the old command.
It is not a safe asynchronous abort for arbitrary RAM code which is computing
without consulting the monitor.

## Commands used by the diagnostic

All multi-byte parameters are sent high byte first. These are static ROM
findings verified through emulator execution, not independently established
manual specifications.

| Command | Parameters after command byte | Effect |
| --- | --- | --- |
| 0Bh | address high/low, count high/low | Return `count` bytes beginning at address |
| 0Ch | address high/low, count high/low, data | Store bytes beginning at address |
| 0Dh | entry address high/low | Transfer execution to RAM entry; no ordinary caller return address is supplied |

The diagnostic uses nonzero counts only. Zero-count and wrapping transfers are
not part of its supported contract.

`RST 18h` sends A through the monitor byte transport. The diagnostic preserves
registers around this call where needed. `JP 002Ah` returns to monitor command
service, which establishes its own stack. A RAM program must not simply `RET`
from the command-0Dh entry.

## Subsystem memory and ownership

| FDC address | Role in this monitor/model |
| --- | --- |
| 0000h–1FFFh | ROM; never written or replaced by this diagnostic |
| 4000h–6FFFh | Bounded diagnostic image, including self-modifying ZEX state |
| 7000h | Timing-kernel scratch byte |
| 7339h | Ported ZEX `LD r,n` group's scratch operand; original 0339h would be ROM |
| below 7EFFh | Downward diagnostic stack, initially SP=7EFFh |
| 7F00h–7FFFh | Live firmware work and monitor stack; not general free RAM |
| 7F09h | Motor-active latch; clear when stopping motors through F8h so the next monitor command performs normal motor-start sequencing |

The main controller first saves **4000h–7EFFh**, then uploads the payload and
zeros the rest of that diagnostic range to match the mapped host initial RAM.
After receiving its explicit completion byte and return to monitor service,
it restores that range and reads it back byte-for-byte **before** any BDOS
file write. It does not claim to restore every CPU register or the entire
7Fxx work area. The motor latch is intentionally cleared by the diagnostic;
normal command service resumes motor handling. An independent default-DMA and
CP/M-main-stack lifecycle is also required on the controller side.

The program is a temporary FDC takeover: normal disk command service is
unavailable while its RAM program executes. No drive write commands occur
inside that interval. Motors are stopped during the computation. Timeout,
transport failure, log overflow or timer wrap is an unknown-state failure:
the main controller reports a stable code and halts without attempting disk
I/O. Recovery then requires a machine reset, not an unverified warm boot.

## Timing scope

The separate RAM kernels signal start/end outside their loops. The main CPU
polls **both transitions** of system port 40h bit 5 (VRTC), without disk or
console operations during the timed computation. An empty notification path,
branch baseline, multiple loop counts and repetitions separate fixed overhead
and loop-control cost from body cost. This measures board-level RAM-resident
execution relative to VRTC; it is not an intrinsic-cycle or ROM-wait test.
The physical reference frequency must be independently identified. Host wall
time, source-model T counts and displayed nominal MHz do not establish it.
