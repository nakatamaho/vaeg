# M99p — Implement D3D11 pass-through

Status: PASS (cross-build and source inspection); Windows runtime deferred

## Implementation

The Windows presenter uploads each supported `VAEG_FRAME_INPUT` source mode
through the shared conversion contract and presents it through the D3D11
source texture, shader, and swap chain created by M99o. RGB565 and ARGB8888
inputs are converted to the backend's RGBA8888 texture layout before the
`D3D11_MAP_WRITE_DISCARD` upload. The input texture is reused while its extent
is unchanged and recreated only after an extent change.

The output viewport is recomputed from the current client-area dimensions,
including resize and fullscreen transitions, using the common aspect-fit
rules. Minimized or zero-sized windows return a disabled result without
blocking or presenting a fabricated frame. No per-frame shader compilation or
unbounded resource allocation is used.

## Verification

The complete optional-feature MinGW cross build completed and produced a
PE32+ Windows executable:

```text
cmake --preset mingw-cross
cmake --build --preset mingw-cross
```

Result: PASS. The generated artifact is
`build/mingw-cross/sdl2/vaeg.exe`. The two D3D11 translation units were also
compiled directly against the MinGW-w64 Windows SDK headers before the full
build. The existing common pass-through, conversion, aspect, fallback, and
presenter-state tests remain covered by the macOS feature-on build.

The current host has no Windows runtime or D3D11 GPU, so actual pass-through
pixels, HiDPI/fullscreen behavior, and device lifecycle evidence remain
deferred to G99-3. The D3D11 backend does not use WARP as a substitute for
that hardware evidence.
