# M99h common presenter contracts

Status: PASS

M99h adds backend-neutral contracts under `sdl2/librashader/`. The contracts
do not include SDL, OpenGL, Metal, Direct3D, or librashader types.

## `VAEG_FRAME_INPUT`

`frame_input.h` carries the immutable source pointer, width, height, pitch,
pixel format, row origin, source aspect ratio, source frame rate, frame number,
and frame delta in nanoseconds. `frame_input.cpp` validates those fields and
returns stable error classifications. RGB565 and ARGB8888 are the only formats
accepted by the initial contract; the pointer is borrowed and is never copied
or owned by the contract layer.

## Presenter lifecycle

`native_presenter.h` defines the required states:

```text
Unavailable -> Initializing -> PassThrough <-> Filtered
                         |          ^
                         +----------+
```

The interface exposes only lifecycle and presentation operations. Its creation
info contains an opaque host-window pointer and scalar drawable information;
backend resources remain private to a later platform implementation.

`native_presenter.cpp` centralizes state, result, and error names and validates
the allowed recovery transitions. `presenter_factory.cpp` currently returns an
explicit unavailable presenter, preserving the old SDL path until a platform
backend is selected in M99k/M99o/M99s.

## Verification

The two focused tests are:

- `tests/frontend/librashader/test_frame_input.cpp`: valid RGB565 input,
  short-pitch, zero-delta, and invalid-aspect rejection.
- `tests/frontend/librashader/test_presenter_state.cpp`: lifecycle transition
  matrix, stable names, and unavailable-factory fallback behavior.

Both are registered as `romless;frontend;librashader` CTest tests. The test
targets use C++17 and are included only when `VAEG_ENABLE_TESTS=ON`.
