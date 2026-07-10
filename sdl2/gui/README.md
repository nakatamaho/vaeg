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
# SDL2 Dear ImGui GUI

The M10 GUI uses Dear ImGui with `imgui_impl_sdl2` and
`imgui_impl_sdlrenderer2`. It renders on the same `SDL_Renderer` that
`scrnmng` uses for the guest `SDL_Texture`.

## Input Routing

Every SDL event is first passed to Dear ImGui. Keyboard events and
SDL_TEXTINPUT reach the guest only when `ImGuiIO::WantCaptureKeyboard`
is false. Mouse events reach guest-side routing only when
`ImGuiIO::WantCaptureMouse` is false. When ImGui wants the keyboard,
mouse, or text input, the guest does not receive that event.

M14 adds a keyboard binding capture mode. While it waits for the next
host scancode, the captured keydown and matching keyup are consumed by
the GUI and never sent to the guest.

Roman-Kana input parses A-Z and apostrophe host scancodes before guest
routing. It emits guest keyboard make/break sequences through
`sdl2/kbdinject.c`; it never injects Unicode text or guest memory bytes.
The menu selects JIS-Kana or Roman-Kana as the kana input method. The
assigned KANA key controls guest kana mode: one press locks KANA, and the
next press unlocks it. Roman-Kana consumes A-Z scancodes only while that
KANA lock mirror is active.

## Storage Menus

FDD1/FDD2 Open and Eject act immediately through the existing floppy
mount path. The GUI Reset command reapplies the mounted FDD paths after
the core reset. The paths are saved as `FDD1FILE` / `FDD2FILE` in
`np2.cfg`, so mounted FDDs are restored across reset and application
restart; Eject clears the corresponding saved path.

HardDisk -> New SASI image creates HDI images through the existing
`newdisk_hdi()` helper. It supports the legacy 5/10/15/20/30/40 MB SASI
geometry choices and refuses to overwrite an existing file.

HardDisk -> SASI-1/SASI-2 Open updates `HDD1FILE` / `HDD2FILE` in
`np2.cfg` through the existing `diskdrv_sethdd()` path. Remove clears the
same key. Reset the guest after changing a SASI image; reset is the
reliable point where `sxsi_open()`, `PCHDD_SASI`, and `sasiio_bind()` are
rebuilt for the guest. The configured SASI image path is retained across
the GUI Reset command. SCSI/IDE mounting and THD/NHD/SCSI image creation
remain later items.

## Sound Menu

Sound -> OPN backend selects `NP2` or `ymfm` and persists the choice as
`opn_backend` in `np2.cfg`. Both engines mirror FM register writes, so changing
the output selector uses the existing guest-reset flow to rebuild the selected
synthesizer from a clean board state while retaining mounted FDD/SASI paths.
The ymfm option currently replaces only YM2203/YM2608 FM operator synthesis;
NP2 continues to own timer/IRQ, SSG, ADPCM, rhythm, and stream mixing.

## Boot Model Menu

Emulate -> Boot model selects `VA` or `VA2/VA3`. `VA` writes
`pc_model=88VA1` and selects the unsuffixed VA ROM names; `VA2/VA3` writes
`pc_model=88VA2` and selects MAME-compatible `*_va2.rom` names. Both sets are
read beside the executable. Selection immediately uses the existing
guest-reset flow, including preservation of configured FDD and SASI media.
Missing ROMs are reported with the selected model and expected root.

## Font Asset Lookup

The GUI loads `NotoSansJP-Regular.ttf` with
`ImGuiIO::Fonts->GetGlyphRangesJapanese()`. The guest font ROM is never
used for host GUI text.

Lookup order:

1. `$VAEG_ASSET_DIR/NotoSansJP-Regular.ttf`
2. `assets/NotoSansJP-Regular.ttf` relative to the current working
   directory
3. `assets/`, `../assets/`, `../../assets/`, and `../../../assets/`
   relative to `SDL_GetBasePath()`
4. `../share/vaeg/assets/` relative to `SDL_GetBasePath()`
5. `../share/vaeg/` relative to `SDL_GetBasePath()`

The build tree works when launched from the repository root, and also
from `build/linux-debug/sdl2/` through the `../../../assets/` lookup.
An installed layout should place assets under
`$prefix/share/vaeg/assets/`.
