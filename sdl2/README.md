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
# SDL2 Frontend

This is the SDL2 frontend for the portable PC-98 / PC-88VA build. It
links the CMake `vaeg_core`, `vaeg_va`, and `vaeg_common` targets and
includes the M10 Dear ImGui menu layer. See `../BUILD.md` for OS-level
build recipes.

## Build

```sh
cmake --preset linux-debug
cmake --build build/linux-debug --target vaeg_sdl2
```

The executable is written to:

```text
build/linux-debug/sdl2/vaeg
```

SDL2 is discovered through `find_package(SDL2)` first, then pkg-config.
`VAEG_FETCH_SDL2=ON` is reserved for the MinGW cross preset and fetches
the pinned SDL2 release recorded in ADR-0006.

## Run

```sh
./build/linux-debug/sdl2/vaeg [options]
```

### Command-line options

| Area | Options |
|------|---------|
| Machine | `--model va|va2` |
| Sound | `--fmbackend np2|ymfm`, `--fmsound opn|opna`, `--ymfm-fidelity minimum|medium|maximum`, `--samplerate 11025|22050|44100`, `--soundbuffer 40..1000`, `--mute` |
| Media | `--fdd1 path|none`, `--fdd2 path|none`, `--sasi1 path|none`, `--sasi2 path|none` |
| Execution | `--cpumult 1..32`, `--sgp model|follow-cpu|1..16`, `--nowait`, `--frameskip auto|full|2|3|4` |
| Display/input | `--fullscreen`, `--windowed`, `--effect unfiltered|linear|scanline|crt-lite`, `--scaling native|fit|fit-8dot|integer|stretch`, `--controller joystick|mouse`, `--keyboard-layout jis|us|custom` |
| Diagnostics/information | `--smoke`, `--selftest`, `--debug`, `--fdctrace`, `--pacelog`, `--version`, `--help`, `-h` |

Run `vaeg --help` for the built-in list. Enum values are ASCII
case-insensitive, and the last occurrence wins when an option is repeated.
Positional FDD arguments have been removed; use `--fdd1` and `--fdd2`.

`--model va` selects `88VA1` and its unsuffixed ROM set. `--model va2` selects
the `88VA2` compatibility model and its `*_va2.rom` set. The effective model
and FM hardware must be compatible, so `--model va2 --fmsound opn` is an
explicit startup error. A changed model otherwise uses the same default-sound
policy as the GUI.

Named FDD options accept existing direct image files. SASI options go further
than a file-existence check: the image is opened and accepted only when its
recognized geometry is usable through the SASI interface and the declared
sector data is present. Use `none` to make a named drive empty for the session.
An invalid media path or removed positional argument fails before SDL machine
initialization.

All setting and media options are session-only. They are applied after
`vaeg.cfg` is loaded and restored before its normal shutdown save as long as
the active value still matches the CLI-applied value. A setting changed
through the GUI during the run can therefore persist. A pre-existing managed
archive image remains protected from pruning while a CLI FDD override is
active.

`--pacelog` prints pacing counters once per second for jitter diagnosis.

Disk images may also be dragged onto the SDL window. One drop operation is
sorted by case-insensitive basename: the first image mounts as FDD1, the
second as FDD2, and later images are reported as ignored. Supported direct
extensions are `.d88`, `.88d`, `.d98`, `.98d`, `.fdi`, `.xdf`, `.hdm`,
`.dup`, `.2hd`, `.tfd`, and `.img`. ZIP, 7z, and LZH drops extract only supported
images to bounded managed storage under the platform user-state directory
when LibArchive support is built. Archive mounts are saved in `FDD1FILE` and
`FDD2FILE`, so they remain valid through reset and application restart.
Unreferenced managed images are removed after eject or replacement; an image
still mounted in either drive is retained.

FDD1/FDD2 Open also accepts ZIP, 7z, and LZH when LibArchive support is built.
Opening an archive from FDD1 mounts the first two basename-sorted images as
FDD1/FDD2. Opening from FDD2 mounts only the first image as FDD2 and leaves
FDD1 unchanged. The same extraction limits, traversal/link rejection,
persistent managed storage, and ignored-image reporting used by drag and drop
apply to menu-selected archives.

The `linux-release`, MinGW, and macOS release presets link the pinned archive
stack statically. Linux development builds use a system LibArchive when one is
available and otherwise report archive loading as unavailable.

The FDD menu can also create an empty FAT12 data disk as Japanese MS-DOS 2HD
(1.232 MB) or 2DD (640 KB). D88 preserves track and sector metadata; IMG is a
headerless raw sector image that can be accessed directly with tools such as
`mdir -i disk.img ::` and `mcopy -i disk.img`. The filename is editable,
existing files are never replaced, and the result can be mounted immediately
as FDD1 or FDD2. The generated image is formatted but does not contain MS-DOS
system files and is not bootable. 2D creation remains deferred pending a
separate compatibility audit.

Headless smoke check:

```sh
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy ./build/linux-debug/sdl2/vaeg --smoke
```

`--smoke` initializes video, audio, and the PC-98 core, runs a short fixed
frame loop, then exits with status 0 when initialization succeeds.

Normal startup displays the historical 320x200 VAEG graphic from
`assets/vaeg.bmp` for at least 1.5 seconds while continuing to process SDL
events. CMake embeds the graphic in the executable; it is not a runtime file.
ROM-less `--smoke` and `--selftest` runs skip the graphic and delay. There is
no alternate-image fallback.

CMake also embeds the byte-identical historical VAEG icon from
`assets/vaeg.ico`. The frontend decodes the embedded ICO and supplies it to
SDL as the window icon on every platform. Windows builds additionally compile
it as a native executable resource. No adjacent icon file is required.

## SASI HDD Images

SASI HDD images are configured through `vaeg.cfg`:

```ini
HDD1FILE=/path/to/disk.hdi
HDD2FILE=
```

The SDL2 GUI also exposes HardDisk -> New SASI image plus SASI-1/SASI-2
Open and Remove. New SASI image creates HDI images using the existing
5/10/15/20/30/40 MB SASI geometry table and refuses to overwrite an
existing file. After changing a SASI image, reset the guest so the
existing SxSI/SASI open and bind path is rebuilt. SCSI and IDE GUI
mounting are not implemented yet.

## ROM Placement

ROMs are not included and must be extracted from hardware you own. Place the
selected set beside the executable:

| Model | Model ROM files |
|---|---|
| VA | `vadic.rom`, `vafont.rom`, `varom00.rom`, `varom08.rom`, `varom1.rom` |
| VA2/VA3 | `vadic_va2.rom`, `vafont_va2.rom`, `varom00_va2.rom`, `varom08_va2.rom`, `varom1_va2.rom` |

The VA2/VA3 names follow MAME's `pc88va2` `ROM_START` declaration in
[`src/mame/nec/pc88va.cpp`](https://github.com/mamedev/mame/blob/master/src/mame/nec/pc88va.cpp).
VA2/VA3 does not fall back to the unsuffixed VA files. Both models also use
`vasubsys.rom` as an extra: unlike MAME's currently unconnected FDD subsystem
ROM entry, vaeg executes the Z80 FDD subsystem.

After resolving a complete set, the frontend compares each file's size,
CRC32, and SHA-1 with MAME's `pc88va` or `pc88va2` declaration. The extra
`vasubsys.rom` uses the CRC32/SHA-1 from MAME's disabled FDD subsystem
declaration. A mismatch logs a warning with expected and actual values but
does not stop the emulator.

The active frontend resolves ROMs in this order:

1. the executable directory, using the filename set selected by model;
2. the current working directory, for development.

If neither complete set exists, the executable directory remains the expected
root and the frontend reports the selected model and first missing ROM. The
old `biospath` INI key is ignored by SDL2 and is no longer written.
`np2cfg.biospath` remains the shared core loader path but is derived at
runtime.

Use `Emulate -> Boot model -> VA` for `pc_model=88VA1` and unsuffixed files.
Use `VA2/VA3` for `pc_model=88VA2` and the five `*_va2.rom` files. Changing
the selection performs the existing reset flow and retains configured FDD
and SASI media.

## Configuration

The configuration syntax is unchanged. The SDL2 frontend selects the first
existing `vaeg.cfg` in this order:

1. `vaeg.cfg` beside the executable
2. `vaeg.cfg` in the portable user state directory

If neither file exists, the frontend creates `vaeg.cfg` in the user state
directory when settings are saved. The user state directory is
`$XDG_CONFIG_HOME/vaeg` or `$HOME/.config/vaeg` on Linux,
`%APPDATA%\vaeg` on Windows, and
`~/Library/Application Support/vaeg` on macOS. If no platform user
directory is available, it falls back to the current directory.

Obsolete `np2.cfg`, `np2.ini`, and `vaeg.ini` files are not read.

An existing `vabkupmem.dat` beside the executable takes priority over the
user-state copy and is saved back in place. If no executable-local file
exists, backup memory uses the user state directory. There is no ROM-path
migration fallback. Fixed GUI save-state slots and keyboard sidecars
remain in the user state directory.

## Mouse Input

`Device -> Mouse` separates three settings: host pointer capture, the VA
controller-port device, and rapid buttons. Capture uses SDL relative mouse
mode and feeds movement plus active-low left/right buttons through the
existing guest mouse interface. It does not write guest coordinates or BIOS
state directly.

Select `VA controller port -> Mouse` for VA mouse software; `Joystick` keeps
the original controller-pad path. `Capture mouse` traps the host pointer only
while the vaeg window has focus and Dear ImGui is not using the mouse. F12
toggles capture when `Keyboard -> F12 binding -> Mouse` is selected. Middle
click also toggles capture outside the GUI. Focus loss, reset, state load, and
shutdown release both guest buttons and pending movement.

The settings use the original-compatible `vaeg.cfg` keys:

```ini
Mouse_sw=0
Mouse_VA=0
MS_RAPID=0
```

`Mouse_VA=0` selects joystick and `Mouse_VA=1` selects mouse. Existing
configuration files without these keys remain uncaptured in joystick mode.

## Clipboard Paste

`Edit -> Paste` sends host clipboard text to the guest as paced keyboard
make/break input. The shortcut is Command+V on macOS and Control+V on
Linux/Windows. Printable ASCII and CR/LF line breaks are supported; CRLF is
one Return. Each make or break transition is separated by 20 ms. Unsupported
control characters and non-ASCII UTF-8 code points are skipped and counted in
the Edit menu.

The queue finishes an in-flight key release, then pauses while Dear ImGui
captures keyboard/text input or a modal is open. Reset, state load, focus
loss, quit, and explicit Cancel Paste stop the queue and release synthetic
keys. Paste does not access guest memory or text buffers. Japanese/IME paste
and guest-to-host copy are not implemented.

## Display

The SDL2 window is resizable and keeps the guest framebuffer fixed at
640x400 RGB565. Screen -> Scaling selects Native, Fit, Fit 8-dot, Integer, or
Stretch. Aspect correction is independent of scaling. Native/x2/x3 remain
window-size presets, and Custom accepts a logical width and height. High-DPI
drawable dimensions are calculated separately and are never saved as logical
window size.

Screen -> Effect selects Unfiltered, Linear, Scanline, or CRT Lite. Scanline
and CRT Lite are procedural SDL_Renderer overlays aligned to the 400-line
guest raster. CRT Lite adds a restrained RGB pattern and edge darkening. No
MAME renderer code, shader, LUT, mask texture, or artwork is used.

Screen provides immediate Windowed and Exclusive fullscreen choices. Exclusive
fullscreen uses the current desktop resolution on the saved monitor, so no
separate Apply step or resolution setup is required. Failed transitions roll
back to Windowed. The backend retains the legacy `fscrn_cx`, `fscrn_cy`, and
hexadecimal `fscrnmod` fields, but detailed monitor/mode selection and
Borderless desktop are not exposed in the current GUI.

## Execution Speed And Pacing

`Emulate -> Configure...` keeps the VA base clock fixed at 3.9936 MHz and
sets independent execution capacity for the V30 and SGP. CPU x2 is the
standard 7.9872 MHz setting for VA, VA2, and VA3. CPU x1-x32 changes only the
amount of V30 work available per unit of machine time. Unlike the CPU, the
SGP model-default clock differs by model.

SGP speed has three modes:

- `Model default`: 3.9936 MHz for VA and 7.9872 MHz for VA2/VA3,
  independent of CPU;
- `Follow CPU`: scales Model default by `clk_mult / 2`;
- `Custom`: scales Model default by an integer x1-x16.

These nominal clocks correspond to the 4 MHz and 8 MHz model clocks recorded
in [Inside PC-88VA Wiki section 4.4.6](http://www.pc88.gr.jp/inside88va/wiki/index.php?%A5%B0%A5%E9%A5%D5%A5%A3%A5%C3%A5%AF).
The GUI displays both the relative scale and effective clock. CPU and SGP
changes reset the guest through the media-preserving reset path. The settings
are stored as:

```ini
clk_base=3993600
clk_mult=2
sgp_mode=0
sgp_mult=1
```

CPU or SGP scaling does not change VBlank/TSP timing, sound pitch and timers,
FDD timing, RTC, or normal one-to-one host pacing. `Screen -> No Wait` removes
host waiting. `Screen -> Frame skip` selects Auto, Full frame, 1/2, 1/3, or
1/4 presentation without changing guest time. Holding F11 temporarily uses
No Wait and draw skip 16; releasing F11, losing focus, resetting, loading a
state, or quitting clears the temporary mode. F11 is never sent to the guest
and the saved No Wait/frame-skip/CPU/SGP values are not overwritten.

`Screen -> Frame display` restores the original VAEG `Frame Disp` behavior.
It samples actual guest framebuffer draws over approximately two seconds and
appends `N.NFPS` to the native window title. It does not count ImGui-only
presents and does not change frame skip or guest timing. The toggle is stored
in the original `DspClock` bit 1 in `vaeg.cfg`. Frame display defaults to on
when no saved `DspClock` value exists; an explicitly saved off setting remains
off.

## OPN/OPNA FM Backend

The Sound menu exposes `FM sound backend -> NP2` and
`FM sound backend -> ymfm`.
The selection is saved in the selected `vaeg.cfg` as:

```ini
opn_backend=ymfm
ymfm_fidelity=minimum
```

`ymfm` is the default and selects the BSD-3-Clause ymfm YM2203/YM2608 FM
operator implementation. Select `np2` for the established NP2 sound behavior.
The backend change performs the normal GUI reset so the selected synthesizer
starts from a fully replayed board state; mounted FDD/SASI paths are retained.
Timer/IRQ, SSG, ADPCM, rhythm, board I/O, and final mixing remain on the NP2
path in this stage. Missing or unknown configuration values fall back to `ymfm`.

This backend choice is independent of emulated sound hardware. The Sound
menu also exposes `FM sound OPN/OPNA`: VA defaults to its built-in YM2203/OPN
(`SNDboard=100`) and can select Sound Board II YM2608/OPNA
(`SNDboard=200`). VA2/VA3 defaults to YM2608/OPNA; its OPN-only choice is
disabled. Hardware changes reset the guest and preserve mounted media.

`Sound on/off` pauses or resumes host audio output without removing the
selected guest OPN/OPNA hardware. The choice is stored separately as
`sound_enabled`; `SNDboard` always remains a valid hardware value so FM timer
polling software continues to run while output is muted.

The Sound menu also selects the mixed output rate and requested buffer length
for both FM backends. Supported rates are 11.025, 22.05, and 44.1 kHz;
22.05 kHz remains the compatibility default, while the GUI recommends 44.1
kHz for new configurations. Buffer presets are 40, 100, 200, 500, and 1000 ms,
with custom values accepted from 40 through 1000 ms. These persist as
`SampleHz` and `Latencys` and rebuild the SDL audio device through the normal
media-preserving guest reset.

When ymfm is selected, `ymfm_fidelity` can be `minimum`, `medium`, or
`maximum`. It controls ymfm's native YM2203/YM2608 generation rate before box
downsampling; Maximum has the highest CPU cost. Minimum preserves the previous
behavior and is the fallback for missing or unknown values. NP2 has no
equivalent control, so the fidelity menu is disabled for that backend.

## VA Configuration Requirements

For PC-88VA booting, check these keys in the selected configuration:

- `pc_model=88VA1` or `pc_model=88VA2`: non-VA models can halt at V2.
- `SNDboard=100` for VA built-in OPN, or `200` for VA Sound Board II.
- `SNDboard=200` for VA2/VA3 built-in OPNA. Other values can leave sound
  hardware unbound and cause a silent hang in FM-timer waits.
- `clk_base=3993600`; `clk_mult=2` is standard, while x1-x32 selects V30
  execution capacity without changing machine/peripheral time.

The frontend logs prominent warnings for stale VA sound-board or clock
settings. It never rewrites the user's configuration silently.

## Keyboard Mapping

The SDL2 keyboard path is scancode based. The default host layout is
`keyboard_host_layout=jis`; `us` is a US-keytop preset for text entry,
and `custom` stores GUI-edited bindings as SDL scancode names in the
user-state sidecar `keyboard.map`.
`keyboard_custom_map=file:keyboard.map` in the selected configuration
points to that
sidecar.

Device / Keyboard in the ImGui menu exposes:

- Host layout: JIS physical, US keytop, Custom
- Kana input: JIS Kana, Roman Kana
- Tenkey overlay: maps YUI/HJK/NM,. to guest keypad 789/456/123/0
- Full key binding table with capture-next-key

JIS physical maps host scancode position to PC-88VA physical key
position. US keytop maps printable US punctuation keytops/chords to
guest keys or guest Shift chords that produce the intended ASCII symbol.
The tenkey overlay is a game-oriented mode for tenkeyless keyboards and
is independent of the host layout preset. No Unicode or text-buffer
injection is used. Set `VAEG_KBD_TRACE=1` to log keyboard event routing
and selected guest actions.

Roman Kana parses A-Z and apostrophe host scancodes and emits the same
guest keyboard make/break sequence as physical keys. It never injects
Unicode, CP932, BIOS buffers, DOS buffers, RAM, or VRAM. When ImGui
captures keyboard or text input, neither raw keys nor Roman Kana output
reach the guest. The menu selects the kana input method only. Enter and
leave guest kana mode with the assigned KANA key, which defaults to
`RightAlt`: one press locks KANA, the next press unlocks it. When the
menu is set to Roman Kana and KANA is locked, A-Z host scancodes feed the
helper and are not sent as direct alphabetic guest keys; when KANA is
unlocked, A-Z is normal guest input.

The PC key defaults to `ScrollLock`. VA2/3 use PC-held reset or power-on
for the BIOS setup path, and some VA popup helpers use PC key chords such
as PC+D. See `docs/modernization/keyboard-mapping.md` for the full
inventory and evidence table.

## Font Manager Stub

Host GUI text is rendered by Dear ImGui using
the build-time embedded `assets/NotoSansJP-Regular.ttf`; it does not use
an external runtime font or `sdl2/fontmng.c`.
The SDL2 `fontmng` stub remains linked because the shared core still
builds `font/fontmake.c`, whose `makepc98bmp()` path references
`fontmng_create()`, `fontmng_get()`, and `fontmng_destroy()`. Removing the
stub leaves those symbols unresolved. The current SDL2 consumers are
therefore `CMakeLists.txt` and the shared `font/fontmake.c` link path.
