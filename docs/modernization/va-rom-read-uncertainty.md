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

THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" WITHOUT ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL
THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
-->
# VA ROM Read Uncertainty and Reference Checks

## Status

Open investigation note. This records a private VA ROM read comparison, not
a demonstrated emulator or hardware result.

## Sample identity

The dated private dump names are intentionally normalized to neutral sample
labels. The comparison glob excludes the repository's
`docs/roms/varom00.rom`.

| Sample | SHA-1 |
|---|---|
| `sample1` | `732cd6b78762466036e608082fabc03df3f869e2` |
| `sample2` | `2595bab29556d4efebb6608d70b31fb651eb7789` |
| `sample3` | `e7fc344b12ab0573a5229c7b43feb64bd329e57b` |
| MAME-derived reference | `1266ba969959ff25433ecc900a2caced26ef1a9e` |

The MAME-derived reference is recorded by SHA-1 only. No byte-level comparison
result or difference locations for that reference are recorded here.

The selected working reference (`sample3`) is a 512 KiB `varom00.rom` image
with CRC-32 `df7f8a74` and SHA-1
`e7fc344b12ab0573a5229c7b43feb64bd329e57b`. These are the size and checksum
values used by the VA1 ROM-set check. The working reference does not match the
MAME-derived reference by SHA-1; the MAME comparison is intentionally recorded
by SHA-1 only.

## Majority result and working reference

For the three read samples, `sample3`
(`e7fc344b12ab0573a5229c7b43feb64bd329e57b`) is the majority-result SHA-1
and the current working reference. At every observed read-to-read difference,
the `sample3` byte agrees with one of the other two samples. This is a
selection for reproducible analysis, not proof that it is a perfect electrical
capture.

The populated ROM area 0 content is the authority for banks 0 through 5.
Banks 6 and 7 are documented as reserved, so their bytes are not required to
match the working reference for the current VA acceptance comparison. A later
dump may therefore be accepted with different bank 6/7 bytes if its populated
bank 0 through 5 content matches and the remaining evidence is consistent.

The selected `sample3` per-bank SHA-1 values used by the VAEG VA1 read check
are:

| ROM0 bank | SHA-1 |
|---:|---|
| 0 | `35d37a6a1ecf70025a9d1f9a892f3a4d1f6e1d62` |
| 1 | `da5eaedcf34259a406946984e23e80c3fbf9d1a1` |
| 2 | `e3e3ed4a7e0241dcd669b07c2d7b29a4c94744c2` |
| 3 | `67fc27525e2ced658925289b3736002320f3dcdd` |
| 4 | `463e586b9911bc6fc35f79f0a0bd3a43414460a0` |
| 5 | `1d225f958bdc4719e83873d2a66515622d6b2dc0` |

For reference, the combined SHA-1 of banks 0 through 5 (`0x00000` through
`0x5FFFF`) is
`24a7a5091846ec1f177c97711e4837573bc40b42`.

## Read-to-read differences

`sample1` and `sample2` differ at eight byte positions. From the two reported
four-position comparisons with `sample3`, the eight positions are inferred to
be the union below; the original byte values are not recorded here.

| Pair | Difference count | File offsets and ROM0 bank-local offsets |
|---|---:|---|
| `sample1` vs `sample2` | 8 bytes | bank 6: `0x6448`, `0x6BF2`, `0x852F`, `0xA9F5`, `0xD976`; bank 7: `0x7344`, `0x7AF6`, `0xB18E` |
| `sample1` vs `sample3` | 4 bytes | file `0x66BF2` (bank 6 + `0x6BF2`), `0x6852F` (bank 6 + `0x852F`), `0x6A9F5` (bank 6 + `0xA9F5`), `0x77AF6` (bank 7 + `0x7AF6`) |
| `sample2` vs `sample3` | 4 bytes | file `0x66448` (bank 6 + `0x6448`), `0x6D976` (bank 6 + `0xD976`), `0x77344` (bank 7 + `0x7344`), `0x7B18E` (bank 7 + `0xB18E`) |

All read-to-read differences reported here are in file banks 6 and 7 of the
512 KiB `VAROM00` image. Each bank is 64 KiB, so the bank number is the file
offset divided by `0x10000`.

## Technical-manual interpretation

The PC-88VA technical manual's ROM area 0 selector table assigns selectors
0 through 5 to `ROM00` through `ROM05`, and marks selectors 6 and 7 as
reserved: [PC-88VA technical manual ROM area 0 table](../tekumani/PC88VA_テクニカルマニュアル_BNN.md:10869).

Therefore, the current four-byte read-to-read variation is consistent with
differences in the ROM area 0 positions documented as reserved. For the
usable VA ROM comparison, it is reasonable to exclude banks 6 and 7 from the
acceptance comparison for this capture set. This does not prove that every
timing-dependent read variation is harmless, nor does it establish the exact
electrical or sampling mechanism.

The current observation does not appear to belong to the VA2 path. Future
comparisons should record the model or path, ROM bank and address, read timing,
and relevant machine state, while keeping reserved-area variation separate
from populated `ROM00` through `ROM05` content.
