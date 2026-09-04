# M99u — Common presets and parameters

Status: PASS (common metadata, live-update contract, reset, and persistence)

## Implementation

`ShaderPreset` loads a preset through the pinned librashader dynamic C API and
copies its runtime parameter metadata before the graphics backend creates its
filter chain. `ShaderParameterSet` owns bounded metadata and current values,
rejects malformed ranges and duplicate names, clamps non-finite or out-of-range
updates, and exposes a backend-neutral enumeration API.

`NativePresenter` now applies the same common parameter set to the active
backend, supports live updates and reset, and persists only validated values.
Persistence uses a versioned text format and writes to a sibling temporary
file before rename. Missing state is allowed; malformed state is ignored and
the preset defaults remain active. The existing Metal, D3D11, and OpenGL
bridges only provide their respective librashader `*_filter_chain_set_param`
call, so parameter policy is not duplicated in backend code.

The parameter-state path is optional in `NativePresenterCreateInfo`. Recovery
retains both the preset path and the parameter-state path. OpenGL recovery now
restores the bounded preset path as well as the filter choice; previously its
recovery request passed a null preset path.

## Verification

The macOS development build completed successfully:

```text
cmake --preset macos-macports -B /private/tmp/m99u-macos-m99u-build --fresh \
  -DVAEG_ENABLE_TESTS=ON \
  -DVAEG_Z80_COMPAT_INTEGRATION_TRACE=ON \
  -DVAEG_WERROR=OFF
cmake --build /private/tmp/m99u-macos-m99u-build --parallel 4
```

The focused frontend suite passed 5/5, including clamping, reset, atomic
save, missing-state handling, and malformed-state non-mutation:

```text
ctest --test-dir /private/tmp/m99u-macos-m99u-build --output-on-failure \
  -R 'vaeg_librashader_(frame_input|capture_boundary|presenter_state|pass_through|shader_parameters)$'

100% tests passed, 0 tests failed out of 5
```

The host has no usable Cocoa display for native GPU execution. Runtime preset
compilation and hardware filtered-pixel evidence remain deferred to the
platform QA gates; this milestone's deterministic metadata and persistence
checks do not claim those gates.
