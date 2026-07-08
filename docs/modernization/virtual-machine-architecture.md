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
# Virtual Machine Architecture

This note describes how the portable vaeg build constructs and runs the
emulated machine. It is a code-structure note, not a hardware manual. For
the current PC-88VA ROM-side boot analysis, see
`docs/modernization/pc88va-boot-sequence.md`.

## Overall Shape

vaeg currently builds one virtual machine per process. It does not allocate
a single explicit `VM` object. Instead, the machine is assembled from the
global singleton state inherited from Neko Project II:

```text
SDL2 frontend
  -> pccore reset/run
    -> CPU core: i286c/v30c
    -> memory: i286c memory dispatcher plus PC-88VA memoryva layer
    -> I/O: iocore, iova, cbus, FDC, DMAC, sound boards
    -> video: vram, vramva, scrndraw/scrndrawva
    -> sound: fm/psg/adpcm/beep/FDD motor sources
    -> storage: FDD and SXSI/SASI/SCSI image layers
    -> ROM: BIOS and PC-88VA ROM loaders
```

This makes the current design simple to keep faithful to the legacy code,
but it also means multi-instance emulation would require a larger state
ownership redesign.

## Host Startup

The portable entry point is `sdl2/np2.c:main()`. The startup order is:

1. Parse command-line options and optional FDD image paths.
2. Initialize SDL and the portable file/path layer.
3. Load `np2.cfg` through `initload()`.
4. Validate any positional FDD images.
5. Create the SDL window, renderer, and ImGui GUI.
6. Initialize host-facing managers: sound, communication, system, task,
   keyboard/input, and tracing.
7. Call `pccore_init()` to construct the emulator core.
8. Load writable PC-88VA backup memory.
9. Call `pccore_reset()` to put the virtual machine into reset state.
10. Call `scrndraw_redraw()` so the initial PC-88VA palette and draw
    tables exist after reset.
11. Mount command-line FDD images.
12. Enter the frame loop.

The important implementation range is `sdl2/np2.c:599-662`.

## Configuration to Machine State

`np2cfg` is the persistent configuration. `pccore_set()` converts it into
the runtime `pccore` model state:

- `pc_model=88VA1` selects `PCMODEL_VA1`.
- `pc_model=88VA2` selects `PCMODEL_VA2`.
- Non-VA models leave `pccore.model_va = PCMODEL_NOTVA`.
- `clk_base` and `clk_mult` become `pccore.baseclock`, `pccore.multiple`,
  and `pccore.realclock`.
- `SNDboard` becomes `pccore.sound`, which determines which sound board
  binds during reset.
- DIP switches and memory size affect CPU mode, extension memory, and HDD
  interface selection.

The conversion lives in `pccore.c:140-219`.

## Core Construction

`pccore_init()` constructs the long-lived emulator subsystems before any
guest reset:

- CPU core initialization.
- Palette and draw-table initialization.
- PC-88VA text, sprite, graphics, and subsystem helpers when
  `SUPPORT_PC88VA` is enabled.
- FDD, SXSI, font, GDC, sound, serial, MIDI, PC-9861K, and I/O core
  construction.

This is in `pccore.c:264-301`. It is mostly allocation and table setup;
it is not the hardware reset state yet.

## Reset and Hardware Binding

`pccore_reset()` is the virtual machine reset. It clears RAM and VRAM,
restores memory switch bytes, recomputes the runtime model, resets the
event queue, and forces V30 mode for PC-88VA models.

After that it opens storage image state, resets sound/FDD/I/O/CBUS/FM
state, builds memory maps, binds I/O handlers, initializes video/palette
state, loads BIOS ROMs, and sets the CPU reset vector:

```text
CS = F000h
IP = FFF0h
physical = 0xFFFF0
```

The reset implementation is `pccore.c:360-480`.

## ROM and Memory

For PC-88VA, `biosva_initialize()` loads the ROM images into emulator
buffers:

- `VAFONT.ROM` -> `fontmem`
- `VADIC.ROM` -> `dicmem`
- `VAROM00.ROM` / `VAROM08.ROM` -> `rom0mem`
- `VAROM1.ROM` -> `rom1mem`
- `VASUBSYS.ROM` -> the FDD subsystem ROM buffer

The loader is `biosva/biosva.c:25-80`.

The main CPU memory entry points are still the i286c memory accessors.
When the VA memory mode is active, `i286c/memory.c` dispatches memory
accesses into `cpucva/memoryva.c`:

```text
i286_memoryread()     -> i286_memoryread_va()
i286_memoryread_w()   -> i286_memoryread_va_w()
i286_memorywrite()    -> i286_memorywrite_va()
i286_memorywrite_w()  -> i286_memorywrite_va_w()
```

The switch points are `i286c/memory.c:805-930`. The PC-88VA map routines
are in `cpucva/memoryva.c:770-812`.

This is why the reset vector at physical `0xFFFF0` reads from `VAROM1.ROM`
for a VA machine.

## I/O and Devices

The I/O core is rebuilt and rebound on reset:

```text
iocore_build()
iocore_bind()
cbuscore_bind()
fmboard_bind()
```

These calls are in `pccore.c:451-454`. The PC-88VA extended I/O table is
implemented under `iova/`; `iova/iocoreva.c` provides the 16-bit decoded
VA port dispatcher and the default unhandled-port trace path.

Sound-board configuration matters for PC-88VA boot. A VA setup should use
`SNDboard=200`; otherwise the VA Sound Board II does not bind, and software
that waits on its FM timer can hang. The emulator warns about this stale
configuration at SDL2 startup.

## Frame Execution

The frame loop calls `pccore_exec(draw)` for guest time. Inside
`pccore_exec()`:

1. The display/vsync event is scheduled. PC-98 uses GDC timing; PC-88VA
   uses the VA screen-display event path.
2. The CPU runs until the frame event clears `screendispflag`.
3. The step dispatcher chooses i286 or V30 by `CPU_TYPE`.
4. Portable builds use `i286c_step()` or `v30c_step()`.
5. For PC-88VA, the FDD subsystem and SGP are stepped alongside the main
   CPU.
6. After the frame, disk, calendar, S98, and sound callbacks run.

The key range is `pccore.c:1098-1252`.

## PC-88VA Guest Boot Path

Once reset completes, the guest is executing PC-88VA ROM code, not a
frontend boot script. The current working model is:

```text
1. Reset selects V30/uPD9002 native mode.
2. CPU starts at F000:FFF0.
3. The VA memory map routes that address to VAROM1.ROM.
4. VAROM1.ROM:FFF0 jumps to F000:0000.
5. F000:0000 jumps through the ROM table to F000:12B8.
6. ROM code initializes display/TSP, video, ROM banks, backup RAM,
   uPD9002 control ports, I/O traps, PIT, PIC, and DMAC.
7. The visible V2S/mode-lamp phase is expected around the system-port and
   mode-switch updates in this early V30 setup.
8. A likely BRKEM2 handoff path enters the uPD9002 uPD780/Z80-compatible
   mode through `0F FE 90`.
9. ROM-side BIOS/device initialization continues and eventually reaches
   boot media access.
```

The exact BRKEM2 return path and the producer of the later RAM code at
`1000:C003` remain open investigation items. The emulator currently has a
Z80 core for the FDD subsystem (`cpucva/z80c.cpp`), but it does not yet
emulate the main CPU's uPD780/Z80-compatible mode entered by BRKEM2.

## Practical Consequences

- Changing machine model at runtime must be followed by `pccore_reset()`
  and a redraw-table rebuild.
- ROM availability affects real VA boot, but ROM-less smoke mode can still
  test SDL/core/GUI startup.
- VA storage behavior depends on both the FDC path and the DMA/bank path,
  so FDD debugging usually needs FDC, DMAC, and bank traces together.
- The frozen legacy v141 tier remains useful as a behavioral reference for
  reset, V30, DMA, and FDC sequencing.
