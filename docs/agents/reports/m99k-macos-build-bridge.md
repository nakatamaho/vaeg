# M99k — macOS Metal bridge

Status: PASS

## Scope

M99k establishes the macOS-native bridge boundary needed by the later
presenter stages. The bridge uses SDL2 for the window and event surface, then
creates and owns a CAMetalLayer and the system Metal device through a small
Objective-C++ translation unit. The existing SDL renderer remains the active
presentation path at this stage; no runtime presentation switch is made yet.

## Implementation

- `sdl2/librashader/metal_bridge.h` exposes a C ABI with opaque Metal state.
- `sdl2/librashader/backends/metal_bridge.mm` creates and destroys the SDL
  Metal view, obtains `CAMetalLayer`, selects the system Metal device, and
  updates drawable size.
- `sdl2/librashader/librashader_loader.h` selects the pinned librashader
  Metal loader declarations only for Apple Objective-C++ compilation.
- CMake enables Objective-C++ and links the system Metal and QuartzCore
  frameworks only when the optional librashader feature is enabled.

The bridge deliberately uses `int` in its C ABI instead of the project
`BOOL` type because Objective-C headers define `BOOL` themselves.

## Verification

Command:

```text
cmake -S . -B /tmp/vaeg-m99h-build.quXdyw -G Ninja -DCMAKE_BUILD_TYPE=Debug -DVAEG_ENABLE_TESTS=ON -DVAEG_Z80_COMPAT_INTEGRATION_TRACE=ON -DVAEG_WERROR=OFF
cmake --build /tmp/vaeg-m99h-build.quXdyw --target vaeg_sdl2 --parallel 4
```

Result: PASS. AppleClang configured Objective-C++ and compiled
`metal_bridge.mm`; the final `sdl2/vaeg` link completed. The linker emitted
the repository's existing duplicate-static-library and data-alignment
warnings.

Physical Metal presentation and librashader execution remain deferred to
M99l and later platform stages.
