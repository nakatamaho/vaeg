<!--
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
-->
# Native CRT user guide

Native CRT is an optional presentation mode for the SDL2 frontend. It uses
Metal on macOS, D3D11 on Windows, and OpenGL on Linux. It does not change the
emulated machine or the deterministic guest-frame capture path.

## Build and enable

Windows menus follow the monitor DPI automatically; there is no UI-size menu.
If necessary, set `GUI_ui_scale=200` in `vaeg.cfg` before startup;
`GUI_ui_scale=0` restores automatic sizing. This scales the GUI, not the raw
guest framebuffer, and works with both SDL and native CRT rendering.

Build with the optional feature enabled (it is enabled by default):

```sh
cmake --preset linux-debug -DVAEG_ENABLE_LIBRASHADER=ON
cmake --build build/linux-debug --target vaeg_sdl2
```

On Windows, open `画面 > 描画方式` and select `標準（SDL）` or
`CRT効果（librashader）`. This switches
the renderer immediately, including a real return to SDL. Selecting librashader
again retries initialization. The menu remains visible and is drawn after the CRT effect at the
same logical font size as the SDL version. SDL exposes `エフェクト`; librashader
exposes `CRT設定…`, a separate window for preset paths, reload,
parameter sliders and reset. Scaling, window size and aspect correction remain
shared. Switching renderers retains the SDL effect and saved shader settings.
The selection marks actual renderer ownership. Normal menu status text is
hidden; failures and native pass-through remain visible. The CRT settings
window also reports the current filter status.
The bundled default preset provides `SCREEN_SIZE` (Screen size (%)) in
`CRT設定…`: 80–120%, initially 96.50%, with 0.01% steps. Its default `CURVATURE`
is 0.030. Saved settings still take precedence; use Reset to apply these defaults.
CRT parameter values live in `NativeCRTParameters` in the active `vaeg.cfg`
(or the file selected with `--cfg`). Changes and reset are saved on normal exit,
along with other frontend settings. The old `vaeg-crt-parameters.cfg` is neither
read nor written; no migration is performed. With no integrated values, preset
defaults apply. `--no-cfg` disables loading and disk saving,
while live adjustments remain available for the session.
At 80%, the image is centered with black
borders; above 100%, the image is enlarged and cropped. This display-only pass
runs before the existing CRT shader, so curvature may reshape the borders.
Normal displayed screenshots include this result and enabled information
overlays. Unprocessed screenshots and canonical raw QA captures are unaffected
by this control. Custom presets are unchanged. Update the package's `assets/`
directory as well as the executable; the control is defined in the new shader.

Preset reload and parameter sliders
also apply to the active filter. On macOS/Linux, enablement still requires a
restart and native GUI integration remains pending.

DLL presence alone does not enable CRT. The Windows package includes
`start-native-crt.cmd` to request CRT for that run even if `vaeg.cfg` has it
disabled. The launcher starts in the package directory and writes diagnostics
to `native-crt.log`. It sets the session-only `VAEG_NATIVE_CRT=1` environment
override; `VAEG_NATIVE_CRT=0` requests SDL at startup instead. The window title
shows `Native CRT ON`, pass-through, or the fallback status. A missing
runtime/preset or failed shader preserves the Windows native pass-through
image; failure of the native device returns to SDL.

On Windows, Video info and Framebuffer info overlays also work in CRT mode.
They are drawn after filtering, behind GUI menus, without changing raw captures.

### Windows DLL prerequisites

The pinned official x64 `librashader.dll` imports `D3DX9_43.dll`, `MSVCP140.dll`,
`VCRUNTIME140.dll`, and `VCRUNTIME140_1.dll`. These are not bundled in this ZIP
and DLL presence alone does not prove its dependencies can load. If the menu
reports a missing/unloadable dependency, install the appropriate official
Microsoft runtime, then select librashader again:

- [DirectX End-User Runtimes (June 2010)](https://www.microsoft.com/en-us/download/details.aspx?id=8109)
  for `D3DX9_43.dll` (run the extracted `DXSETUP.exe`). Modern DirectX alone
  does not supply these legacy helper libraries.
- [Visual C++ Redistributable, x64](https://learn.microsoft.com/en-us/cpp/windows/latest-supported-vc-redist)
  for the `*140*.dll` imports.

Do not download individual DLLs from unofficial DLL sites. VAEG does not
automatically download or install these prerequisites. `native-crt.log` records
the Windows loader error and unavailable dependency names; the menu also shows
preset and filter-chain errors instead of only a generic unavailable label.

The setting is stored as `NativeCRT` in `vaeg.cfg`. The default
preset is:

```text
assets/shaders/crt/vaeg_crt_default.slangp
```

The bundled default is found beside the executable, with a working-directory
fallback for development builds. Custom relative preset paths are resolved
from the working directory. A custom preset is used only when it is supplied by
the user; it is not copied into or downloaded by VAEG.

## Release package layout

Start VAEG from the package directory so the default relative asset path is
available:

```text
vaeg                         (or vaeg.exe)
assets/shaders/crt/vaeg_crt_default.slangp
assets/shaders/crt/shaders/crt-lottes-fast.slang
licenses/...
librashader.so               (Linux, optional)
librashader.dylib            (macOS, optional)
librashader.dll              (Windows, optional)
```

The runtime is optional. A package without it remains a valid VAEG package;
Native CRT fails closed to the normal SDL renderer. The release staging
helper accepts only the runtime name for the selected platform and checks the
shader, preset, license, and provenance hashes before packaging.

On Linux or macOS, if a development environment does not search the package
directory for a neighboring dynamic library, add that directory to the
platform loader path for the run:

```sh
LD_LIBRARY_PATH="$PWD" ./vaeg       # Linux
DYLD_LIBRARY_PATH="$PWD" ./vaeg     # macOS development run
```

Windows uses the application directory for `librashader.dll` under the normal
DLL search rules. Do not rename a runtime across platforms or architectures.

## Captures

`画面 > 全画面表示` is a single checked toggle: check it to enter Exclusive
fullscreen, uncheck it to return to the window. No return button is added to
the menu bar, and Esc remains available to the guest.
On entry, `画面上端でメニュー表示` appears for up to three seconds while the
menu is hidden. This non-interactive hint is excluded from screenshots.

In Exclusive fullscreen, the menu normally hides. Move the mouse to the top
edge (12 DPI-scaled logical pixels) to reveal it over the image. It stays visible
while the pointer is over the bar or a popup/item is being operated, and hides
after 0.5 seconds away. Windowed mode keeps the bar visible. Fullscreen never
reserves a menu strip, so revealing the bar does not resize or shift the guest
viewport. Information overlays remain beneath the menus. If guest relative
mouse capture is enabled, release it with the existing capture toggle first.
Fullscreen screenshots retain the full drawable image without a menu-strip
crop, but still omit menus and dialogs.

`画面 > スクリーンショットを保存` (the first menu entry), PrintScreen and
the configured F12 screenshot binding save the next composed display frame:
SDL effects or Windows D3D11 librashader output, scaling/aspect correction,
and Video info / Framebuffer info exactly as enabled. Menus, dialogs and the
top menu strip are excluded; the remaining drawable area, including letterbox
margins, is retained. Readback and PNG encoding occur only on request and may
briefly stall presentation. Failure is reported rather than substituting raw
pixels. Native Metal/OpenGL output readback is not yet supported by this path;
their native GUI integration also remains pending.

`スクリーンショットを保存（加工前）` saves the guest-resolution image without
display effects or scaling. Video info and Framebuffer info are included
independently only when their respective display toggles are on, just as for
normal screenshots. With both off, no analysis panel is added. Deterministic
CLI raw capture remains separate and does not inherit these display toggles.

Use `--screenshot FRAME:PATH` or `--screen-tvram-dump PATH` for deterministic
guest-frame evidence. `--screen-dump` captures the SDL rendered target and is
not available while Native CRT owns the output; VAEG reports that limitation
instead of labeling a filtered GPU image as a deterministic capture.

## Disable or customize

Disable Native CRT in the Windows menu to compare pass-through immediately.
For a one-run SDL fallback diagnostic, use `VAEG_NATIVE_CRT=0`,
or set `NativeCRT=0` in the current `vaeg.cfg` before
starting. To use another preset, set `NativeCRTPreset` in the configuration or
enter its path in `CRT設定…`, then reload the preset. The Windows renderer applies
the reload between frames; macOS/Linux still require a restart.

The default preset combines a VAeg-owned BSD-2-Clause size pass with the
audited Unlicense/public-domain CRT shader. Other
shader files remain user-provided and are outside the VAEG release payload.
See the [troubleshooting guide](native-crt-troubleshooting.md) when the native
path falls back.
