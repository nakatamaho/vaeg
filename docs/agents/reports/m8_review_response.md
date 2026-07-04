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
# M8 Review Response

Branch: `topic/m8-sdl2-frontend`

This is a documentation-only response. No implementation source was
changed.

## 1. `fontmng` Stub Deviation

The review is correct: `docs/agents/tasks/M8_sdl2_frontend.md:28`
expected `fontmng` to need "include-path changes only, verify", but
commit `ec06c58` added a stub instead. That should have been called out.

The SDL1 `sdl/fontmng.c` implementation is not just an include-path
translation in the default build:

- It includes `sdl_ttf.h` directly at `sdl/fontmng.c:2`.
- Its default, non-`RESOURCE_US` path starts at `sdl/fontmng.c:7` and
  keeps a TTF font handle in `_FNTMNG` at `sdl/fontmng.c:33`.
- It initializes SDL_ttf with `TTF_Init()` at `sdl/fontmng.c:44` and
  registers `TTF_Quit` at `sdl/fontmng.c:49`.
- It opens `./default.ttf` through `TTF_OpenFont()` at
  `sdl/fontmng.c:107` and closes it through `TTF_CloseFont()` at
  `sdl/fontmng.c:129`.
- It rasterizes host menu text with `TTF_RenderUNICODE_Solid()` at
  `sdl/fontmng.c:224` and `sdl/fontmng.c:266`, after converting CP932
  text to Unicode at `sdl/fontmng.c:223` and `sdl/fontmng.c:265`.
- It then reads the returned `SDL_Surface` pixels directly in
  `getpixeldepth()` at `sdl/fontmng.c:235`.
- The SDL1 Linux makefile links this dependency explicitly:
  `sdl/Makefile.zau:143` uses `-lSDL_ttf -lfreetype`.

The M8 SDL2 environment intentionally lacks that dependency. The task
constraint at `docs/agents/tasks/M8_sdl2_frontend.md:38` says SDL2 is
found through `find_package`/pkg-config and "No SDL_ttf/SDL_image
dependencies." The current CMake target links only the SDL2 library set
recorded in `VAEG_SDL2_LIBS` (`CMakeLists.txt:27`,
`CMakeLists.txt:32`, `CMakeLists.txt:249`).

There is a second SDL1 fallback path under `#else` at
`sdl/fontmng.c:417`, using `ank10.res`/`ank12.res` at
`sdl/fontmng.c:420` and `sdl/fontmng.c:422`. That path is compile-time
`RESOURCE_US` behavior and renders only the embedded ANK menu font; it
is not the Japanese-capable host GUI font path M10 needs.

The stub also does not affect the M8 gate path. SDL1 used `fontmng`
through the embedded menu: `sdl/np2.c:140` initializes it, `sdl/np2.c:147`
creates `sysmenu`, `embed/menubase/menubase.c:20` creates menu fonts, and
`embed/menubase/menusys.c:402` measures menu strings. M8 explicitly has
no menu system (`docs/agents/tasks/M8_sdl2_frontend.md:70`), and the SDL2
CMake source list includes `sdl2/fontmng.c` but not the embedded menu
sources (`CMakeLists.txt:211` through `CMakeLists.txt:226`).

Resolution for now: keep `sdl2/fontmng.c` as a compatibility stub and
document it as an M10-superseded gap in `sdl2/README.md`. M10's ImGui
font decision replaces host GUI text rendering with the bundled ImGui
font path, so completing an SDL_ttf-based port in M8 would add a
dependency that M8 explicitly forbids and M10 does not need.

## 2. Commit `1941633`: SDL2 System Header Boundary

Commit `1941633` adds `sdl2/sdlapi.h` and changes the SDL2 frontend
modules that include SDL directly to include that boundary header instead:

- `sdl2/inputmng.c`
- `sdl2/np2.c`
- `sdl2/scrnmng.c`
- `sdl2/sdlkbd.c`
- `sdl2/soundmng.c`
- `sdl2/taskmng.c`

The new boundary header contains only:

- an include guard,
- `#define VAEG_SDL2_HEADER <SDL.h>`,
- `#include VAEG_SDL2_HEADER`.

It was needed because SDL2's installed include spellings vary between
environments. Some package layouts expose `<SDL.h>` through CMake or
pkg-config include directories; others require `<SDL2/SDL.h>` unless the
target include directories are normalized. Keeping direct SDL includes
behind one local header makes that boundary a single-file decision and
keeps every frontend module from carrying platform/package spelling.

Task-file placement: it belongs to M8 step 1, the module-by-module SDL1
to SDL2 API translation, because the affected files are the SDL2 modules
being ported. It is specifically part of making `taskmng/main`,
`scrnmng`, `sdlkbd/inputmng`, and `soundmng` compile against the same
SDL2 include boundary.
