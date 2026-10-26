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

# M139 — Whole-screen reverse on every machine

Status: **implemented; G139 human gate pending.**

Branch `milestone/m139-rv-all-machines`; commit prefix `M139:`.

## Request

PR #31's author (comment of 2026-10-25, with a recording of a
PC-8801mkIISR at 8:32): on the actual SR the CARIGRAPH scene of the
PC-8801mkIISR demonstration has a white background and black lines. The
maintainer: the PC-8801 condition added in M129 was wrong; fix it.

## Changes

- [47a84768](https://github.com/nakatamaho/vaeg/commit/47a847688d0ad695f741055e6ff38d6d6ed73de3): `textva_global_reverse()` applies `SYNC.RV` whenever it
  is set; M129's exception (PC-8801 models with monochrome graphics) is
  removed. Selftest `TSP SYNC reverse`: the PC-8801 monochrome case now
  expects the reverse.
- V1/V2 decision X9, the TSP and PC-8801 machine documents, the ledger.

## Gate G139 (human)

Standard V3 gate, plus: the PC-8801mkIISR demonstration's CARIGRAPH is black
lines on white on the VA, the VA2 and the PC-8801mkIISR model.

## Local verification

- `--selftest`: exit 0 (`TSP SYNC reverse ok`).
- Linux release, the demonstration with `--nowait`: CARIGRAPH is black lines
  on white booted as the VA and as the PC-8801mkIISR model (a ROM set under
  `extra/`), frames 19800 and 20600.
- `tools/release/build-mingw-static-container.sh --commit
  ef3557f96e8bca47bf76cd57ee4d7cd4edca93bd`: exit 0, Windows system DLL
  imports only; `vaeg.exe` SHA-256
  `643119193f714fe9cdb753409cf2f35965b0a027347751ab20037635e287e79f`.

## Note on the reference emulators

The maintainer (2026-10-25): M88, X88000 (and QUASI88, whose colour-mode
condition M129 followed) are therefore wrong on this point too. Behaviour
taken from them is a hypothesis until hardware confirms it; M134's PC-8801
hidden-text rule (backgrounds hidden, colours kept) also rests on M88 and
remains unconfirmed on a real PC-8801.
