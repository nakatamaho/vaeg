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

# PC-8801 machines (M113)

vaeg starts the PC-8801 models whose ROMs are found under `extra/<model>/`
(see [Extra ROMs](extra-roms.md)). Task:
[`M113`](../agents/tasks/M113_pc8801_machines.md).

## How it works

A PC-8801 model runs in the PC-88VA's V1/V2 mode — the VA hardware in its
8801-compatible configuration, set up by the VA firmware as for any V1/V2
boot (see [V1/V2 mode plan](v1v2-mode-plan.md)) — with the ROM images that
V1/V2 mode reads replaced by the model's ROMs:

| Role | Replaces | Without the role |
|---|---|---|
| `n88` | N88-BASIC at `varom1` 10000h-17FFFh | (required) |
| `n88_0`-`n88_3` | the four extension banks at `varom1` 18000h-1FFFFh | the bank reads FFh |
| `n80` | the N-BASIC ROM of N mode | no N mode |
| `disk` | the FDD unit's ROM (`vasubsys.rom`); a 2 KiB ROM repeats over 8 KiB | **no FDD unit**: ports FCh-FFh read FFh and the unit does not run |
| `kanji1`, `kanji2` | the VA font image behind ports E8h/E9h and ECh/EDh | the port reads FFh |
| `font`, else `kanji1` 1000h-17FFh | the 8x8 characters | the VA's 8x8 characters |
| `jisyo` | the dictionary banks (ports F0h/F1h) | the window reads FFh |

The VA firmware and the VA ROM set are therefore still needed; so is
everything else the VA's compatibility mode provides (TSP text in uPD3301
emulation, the sub-CPU FDD unit, sound). Differences between the VA's
compatibility mode and a PC-8801 remain. One is modelled per machine: the
uPD3301 whole-screen reverse (START DISPLAY 21h) is ignored by a PC-8801
model while port 31h selects monochrome graphics, whereas the VA applies it
(M129, V1/V2 decision X9).

### Base VA model and modes

| Model directory | Base | Modes offered |
|---|---|---|
| PC-8801, PC-8801mkII | VA2 | V1 S, N |
| PC-8801mkIISR, mkIITR, mkIIFR, mkIIMR | VA2 | V2 S, V1 S, N |
| PC-8801FH, MH, FA, MA, FE, MA2, FE2, MC, PC-98DO | VA2 | V2 H/S, V1 H/S, N |
| PC-88VA | VA | V2 H/S, V1 H/S |
| PC-88VA2 | VA2 | V2 H/S, V1 H/S |

H and S are the V1/V2 speeds: H runs compatible code at 8 MHz and S at
4 MHz, and port 6Eh (through memory switch B1FC5h bit 1) and IN 31h bit 6
report them; models before the FH/MH run at 4 MHz only. N needs `n80`. Sound is the base model's (the
VA2's OPNA; its first half is OPN-compatible); the original PC-8801 has no
FM sound, which is not modelled. The PC-8001 family (M114) runs in N mode
only (below). `PC-88VA` and `PC-88VA2` are
PC-8801-layout sets taken from the VA ROMs (`n88`, banks, `disk`, `kanji1`,
`kanji2`, `font`, `jisyo`; no N-BASIC).

## PC-8001 family (M114)

The PC-8001, PC-8001mkII and PC-8001mkIISR run in the N mode of the VA2's
V1/V2 mode with their own N-BASIC ROM (`n80`, `n80_2`, `n80_2sr`) in place
of a PC-8801's; a 24 KiB PC-8001 ROM leaves the 6000h-7FFFh socket reading
FFh. The VA's N88-BASIC stays. These BASICs program the CRTC for a 15 kHz
monitor, so the machine runs with SW1 at 15 kHz. Port 31h has its
PC-8001mkII meaning: N-BASIC and 200 lines stay selected. There is no FDD
inside; `disk.rom` gives the external unit. The mkII's own graphics modes
and its expansion ROM bank, and the mkIISR's N80SR mode, are not modelled;
the mkII and mkIISR start their N-80 BASIC.

## Starting a model

- `模擬 -> 起動 -> PC-8801/8001 -> <model>`: the entries `V2 H` ... `V1 S`
  and `N`, as for the VA, reset into that model; `ROM` lists its files and
  revisions. A model with missing roles (不足あり) still starts; only a
  model without `n88.rom` cannot. Choosing a PC-88VA/VA2 entry under `起動`
  goes back to the VA.
- The configuration keeps the model in `ExtraMachine`; `最近の起動` records
  it. The window title and 情報 -> More show the running model.
- Command line: `--extra-machine <model>` starts it for the session, in V2
  where the model has V2 and in V1 otherwise.
- A model whose `n88.rom` is gone when the machine resets falls back to the
  VA ROMs with a warning.

## Verification

Selftest `PC-8801 machine` (synthetic ROM files): the replaced images, the
2 KiB FDD ROM repetition, no FDD unit without `disk.rom`, absent banks,
kanji ROMs in port order, the 8x8 characters from `kanji1.rom`, a missing
dictionary, 自動 started as V1/V2, and an incomplete directory leaving the
VA ROMs; the model profiles. With a ROM set matching MAME's `pc8801ma`, V2
BASIC reaches `Ok` with the MA's banner, a V2 disk boots, and N mode
starts its N-BASIC.
