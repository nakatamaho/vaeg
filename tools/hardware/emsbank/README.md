<!--
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
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

# EMSBANK — EMS page frame and the system memory bank (M146)

Checks on a PC-88VA/VA2 with an EMS board whether the EMS page frame at
`C0000h` belongs to the bus slot bank of the system memory area
(`A0000h-DFFFFh`, port `153h` bits 3-0, SMBC), as the technical manual's
memory map and the EMM adapter EMMVA imply.

```sh
VAEG_OPENWATCOM_BACKEND=dosbox VAEG_OPENWATCOM_DOS_ARCHIVE="$OW_ARCHIVE" \
  tools/hardware/emsbank/build.sh OUTPUT.d88
```

builds `EMSBANK.COM` and a bootable MS-DOS 4.0 disk (the msdos4-va.2
release image from the shared cache, checked by SHA-256) whose
`CONFIG.SYS` loads `EMMVA01.SYS`, `SQEMM98.SYS` and `EMMVA02.SYS`. EMMVA
comes from the cached `emmva15a.lzh` (SHA-256 checked); `SQEMM98.SYS` is
built by `tools/openwatcom/build-sqemm98.sh`. Keep the disk outside the
repository. Boot it in V3 mode and run `EMSBANK`; the result appears on the
screen and in `EMSBANK.TXT`.

The program allocates one EMS page, maps it to physical page 0 and, with
interrupts off:

1. selects the bus slot (SMBC 0) and writes the marker `EMSb` at frame
   offset 0;
2. selects GVRAM (SMBC 4), reads the four bytes (`gvram-before`), writes
   zeros, reads them back (`gvram-written`) and restores them;
3. selects kanji ROM #1 (SMBC 8) and reads four bytes (`kanji`);
4. selects the bus slot again and reads the frame (`bus-again`).

`PASS` requires the marker in `bus` and `bus-again`, and in neither
`gvram-before` nor `kanji`. The caller's port `153h` value is restored.
`153h-after-map` shows the bank EMMVA02 leaves after INT 67h AH=44h.

In vaeg before M146 the frame replaced every bank at `C0000h-CFFFFh`
whenever an EMS target was selected, so `gvram-before` and `kanji` showed
the marker (`FAIL`). The machine and the corrected vaeg are expected to
`PASS`.
