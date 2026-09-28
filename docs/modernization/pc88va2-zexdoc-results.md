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
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.
-->
# ZEXDOC results on NEC µPD9002, PC-88VA2 V2 compatibility mode

## ZEXDOC

| Field | Value |
| --- | --- |
| Machine | NEC PC-88VA2 |
| Mode | V2 compatibility mode (Z80 emulation) |
| CPU identified for this mode | NEC µPD9002 |
| Test suite | ZEXDOC, 67 standard test groups |
| Result | 55 OK, 12 ERROR |
| Transcription | Cross-checked against five photographs and arranged in standard test order |

This is a reported real-machine result from PC-88VA2. It is not a VAEG run.
The photographs are not included in the repository.

| # | ZEXDOC test | Result | Expected CRC | Found CRC |
| ---: | --- | --- | --- | --- |
| 1 | `<adc,sbc> hl,<bc,de,hl,sp>` | OK | — | — |
| 2 | `add hl,<bc,de,hl,sp>` | OK | — | — |
| 3 | `add ix,<bc,de,ix,sp>` | OK | — | — |
| 4 | `add iy,<bc,de,iy,sp>` | OK | — | — |
| 5 | `aluop a,nn` | ERROR | `48799360` | `12987d59` |
| 6 | `aluop a,<b,c,d,e,h,l,(hl),a>` | ERROR | `fe43b016` | `4f797917` |
| 7 | `aluop a,<ixh,ixl,iyh,iyl>` | ERROR | `a4026d5a` | `6d7337f5` |
| 8 | `aluop a,(<ix,iy>+1)` | ERROR | `e849676e` | `4142bbf8` |
| 9 | `bit n,(<ix,iy>+1)` | ERROR | `a8ee0867` | `5a40ec5a` |
| 10 | `bit n,<b,c,d,e,h,l,(hl),a>` | ERROR | `7b55e6c8` | `9ae7950f` |
| 11 | `cpd<r>` | OK | — | — |
| 12 | `cpi<r>` | OK | — | — |
| 13 | `<daa,cpl,scf,ccf>` | ERROR | `9b4ba675` | `6096b6aa` |
| 14 | `<inc,dec> a` | OK | — | — |
| 15 | `<inc,dec> b` | OK | — | — |
| 16 | `<inc,dec> bc` | OK | — | — |
| 17 | `<inc,dec> c` | OK | — | — |
| 18 | `<inc,dec> d` | OK | — | — |
| 19 | `<inc,dec> de` | OK | — | — |
| 20 | `<inc,dec> e` | OK | — | — |
| 21 | `<inc,dec> h` | OK | — | — |
| 22 | `<inc,dec> hl` | OK | — | — |
| 23 | `<inc,dec> ix` | OK | — | — |
| 24 | `<inc,dec> iy` | OK | — | — |
| 25 | `<inc,dec> l` | OK | — | — |
| 26 | `<inc,dec> (hl)` | OK | — | — |
| 27 | `<inc,dec> sp` | OK | — | — |
| 28 | `<inc,dec> (<ix,iy>+1)` | OK | — | — |
| 29 | `<inc,dec> ixh` | OK | — | — |
| 30 | `<inc,dec> ixl` | OK | — | — |
| 31 | `<inc,dec> iyh` | OK | — | — |
| 32 | `<inc,dec> iyl` | OK | — | — |
| 33 | `ld <bc,de>,(nnnn)` | OK | — | — |
| 34 | `ld hl,(nnnn)` | OK | — | — |
| 35 | `ld sp,(nnnn)` | OK | — | — |
| 36 | `ld <ix,iy>,(nnnn)` | OK | — | — |
| 37 | `ld (nnnn),<bc,de>` | OK | — | — |
| 38 | `ld (nnnn),hl` | OK | — | — |
| 39 | `ld (nnnn),sp` | OK | — | — |
| 40 | `ld (nnnn),<ix,iy>` | OK | — | — |
| 41 | `ld <bc,de,hl,sp>,nnnn` | OK | — | — |
| 42 | `ld <ix,iy>,nnnn` | OK | — | — |
| 43 | `ld a,<(bc),(de)>` | OK | — | — |
| 44 | `ld <b,c,d,e,h,l,(hl),a>,nn` | OK | — | — |
| 45 | `ld (<ix,iy>+1),nn` | OK | — | — |
| 46 | `ld <b,c,d,e>,(<ix,iy>+1)` | OK | — | — |
| 47 | `ld <h,l>,(<ix,iy>+1)` | OK | — | — |
| 48 | `ld a,(<ix,iy>+1)` | OK | — | — |
| 49 | `ld <ixh,ixl,iyh,iyl>,nn` | OK | — | — |
| 50 | `ld <bcdehla>,<bcdehla>` | OK | — | — |
| 51 | `ld <bcdexya>,<bcdexya>` | OK | — | — |
| 52 | `ld a,(nnnn) / ld (nnnn),a` | OK | — | — |
| 53 | `ldd<r> (1)` | ERROR | `94f42769` | `ec734af4` |
| 54 | `ldd<r> (2)` | ERROR | `5a907ed4` | `3958c2d0` |
| 55 | `ldi<r> (1)` | ERROR | `9abdf6b5` | `e23a9b28` |
| 56 | `ldi<r> (2)` | ERROR | `eb59891b` | `49a0684e` |
| 57 | `neg` | OK | — | — |
| 58 | `<rrd,rld>` | OK | — | — |
| 59 | `<rlca,rrca,rla,rra>` | ERROR | `251330ae` | `9ac609b5` |
| 60 | `shf/rot (<ix,iy>+1)` | OK | — | — |
| 61 | `shf/rot <b,c,d,e,h,l,(hl),a>` | OK | — | — |
| 62 | `<set,res> n,<bcdehl(hl)a>` | OK | — | — |
| 63 | `<set,res> n,(<ix,iy>+1)` | OK | — | — |
| 64 | `ld (<ix,iy>+1),<b,c,d,e>` | OK | — | — |
| 65 | `ld (<ix,iy>+1),<h,l>` | OK | — | — |
| 66 | `ld (<ix,iy>+1),a` | OK | — | — |
| 67 | `ld (<bc,de>),a` | OK | — | — |

### Error groups

| Group | Failing tests | Count |
| --- | --- | ---: |
| 8-bit ALU | `aluop a,nn`; register, index-half, and indexed forms | 4 |
| BIT | register and indexed forms | 2 |
| Flag/control | `<daa,cpl,scf,ccf>` | 1 |
| Block transfer | `ldd<r> (1)/(2)` and `ldi<r> (1)/(2)` | 4 |
| Accumulator rotate | `<rlca,rrca,rla,rra>` | 1 |
| **Total** |  | **12** |

The pattern identifies test groups whose final CRC differs from the ZEXDOC
expected CRC. It is useful for directing instruction-level investigation, but
the CRC results alone do not identify the exact instruction, flag, or internal
CPU rule responsible for each mismatch. ZEXDOC exercises documented Z80
behavior, so these failures should not be summarized as only undocumented
flag-bit differences.

## ZEXALL

**Status: measurement in progress. No ZEXALL result is recorded yet.**

Append the eventual result here, separately from ZEXDOC. Record the test binary
identity and machine mode, overall counts, and each reported test group's
expected and found CRC. The ZEXDOC result above must remain unchanged.
