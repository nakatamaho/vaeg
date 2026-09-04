# M99n — Metal lifecycle hardening

Status: PASS (lifecycle contract and build evidence); physical GPU lifecycle
deferred to G99-5

## Lifecycle behavior

The backend-neutral `NativePresenter` contract now includes a drawable-size
update operation. The Metal presenter applies non-zero sizes to the
`CAMetalLayer` in drawable pixels and treats zero dimensions as a temporary
disabled/minimized state. The layer's current drawable size is read again at
presentation time, so resize, fullscreen, and display-scale changes do not
reuse logical window dimensions.

`nextDrawable` returning nil is reported as a non-fatal disabled result; it
does not tear down the emulator or fabricate a frame. Input extent changes
recreate the reusable source texture and upload buffer. A resource failure
moves the presenter to `Unavailable`; `recover()` repeats initialization using
the retained window, drawable size, filter choice, and bounded preset path,
recreating the Metal view, device, queue, pipeline, source texture, and
optional librashader chain. Filter toggles keep the same native resources and
clear filter history when re-enabled.

No OpenGL context is created by the macOS path. SDL2 continues to own event
and window operations, while the native presenter owns Metal resources.

## Verification

```text
cmake --build /tmp/vaeg-m99l-build --target vaeg_sdl2 vaeg_librashader_presenter_state_test --parallel 4
ctest --test-dir /tmp/vaeg-m99l-build --output-on-failure -R 'vaeg_librashader_presenter_state$'
cmake --build /tmp/vaeg-m99l-off --target vaeg_sdl2 --parallel 4
```

Result: PASS. The feature-on macOS application and lifecycle contract test
built; the focused test passed 1/1. The feature-off macOS application also
built and linked with the legacy fallback presenter.

The focused test exercises the unavailable/fallback resize contract. A real
Metal window was not available in this environment (`The video driver did not
add any displays`), so resize/fullscreen/minimize/nil-drawable/device-recovery
pixel and timing evidence remains deferred to G99-5.
