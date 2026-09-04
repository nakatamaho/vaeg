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

Build with the optional feature enabled (it is enabled by default):

```sh
cmake --preset linux-debug -DVAEG_ENABLE_LIBRASHADER=ON
cmake --build build/linux-debug --target vaeg_sdl2
```

In VAEG, open the `Native CRT (librashader)` menu, enable it, and restart the
emulator. The setting is stored as `NativeCRT` in `vaeg.cfg`. The default
preset is:

```text
assets/shaders/crt/vaeg_crt_default.slangp
```

The preset path is resolved from the process working directory unless an
absolute path is entered. A custom preset is used only when it is supplied by
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

Use `--screenshot FRAME:PATH` or `--screen-tvram-dump PATH` for deterministic
guest-frame evidence. `--screen-dump` captures the SDL rendered target and is
not available while Native CRT owns the output; VAEG reports that limitation
instead of labeling a filtered GPU image as a deterministic capture.

## Disable or customize

Disable Native CRT in the menu and restart. For a one-run fallback diagnostic,
run with `--no-cfg`, or set `NativeCRT=0` in the current `vaeg.cfg` before
starting. To use another preset, set `NativeCRTPreset` in the configuration or
enter its path in the menu, then reload the preset and restart if ownership
must change.

The default preset is a single audited Unlicense/public-domain shader. Other
shader files remain user-provided and are outside the VAEG release payload.
See the [troubleshooting guide](native-crt-troubleshooting.md) when the native
path falls back.
