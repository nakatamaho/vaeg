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

Every SDL event is first passed to Dear ImGui. Keyboard events reach the
guest only when `ImGuiIO::WantCaptureKeyboard` is false. Mouse events
reach guest-side routing only when `ImGuiIO::WantCaptureMouse` is false.
When ImGui wants the keyboard or mouse, the guest does not receive that
event.

IME text input is not implemented in M10.

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
