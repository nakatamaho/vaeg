# Platform and source validation matrix

REC00 leaves all recording feature cells NOT_RUN. The available host inventory
below records why the baseline could run only on macOS arm64. Availability is
not a recording result and does not remove any requested backend from scope.

| Platform / presenter | Target build | Exact readback fixture | Actual Native + OSD | Actual Displayed + CRT/OSD/client | PCM/A-V sync | VLC play/seek | Package runtime |
|---|---|---|---|---|---|---|---|
| Windows / SDL fallback | NOT_RUN | NOT_RUN | NOT_RUN | NOT_RUN | NOT_RUN | NOT_RUN | NOT_RUN |
| Windows / native D3D11 | NOT_RUN | NOT_RUN | NOT_RUN | NOT_RUN | NOT_RUN | NOT_RUN | NOT_RUN |
| Linux / SDL fallback | NOT_RUN | NOT_RUN | NOT_RUN | NOT_RUN | NOT_RUN | NOT_RUN | NOT_RUN |
| Linux / native OpenGL | NOT_RUN | NOT_RUN | NOT_RUN | NOT_RUN | NOT_RUN | NOT_RUN | NOT_RUN |
| macOS / SDL fallback | PASS (REC13 build) | NOT_RUN | IMPLEMENTED_UNVERIFIED (Native source integration; required guest OSD demo pending) | NOT_RUN | IMPLEMENTED_UNVERIFIED (private four-interval MKV decode; physical device/VLC pending) | NOT_RUN | NOT_RUN |
| macOS / native Metal | NOT_RUN | NOT_RUN | NOT_RUN | NOT_RUN | NOT_RUN | NOT_RUN | NOT_RUN |
| Recording OFF / supported builds | PASS (baseline only) | NOT_APPLICABLE | NOT_APPLICABLE | NOT_APPLICABLE | Existing audio not run | NOT_APPLICABLE | NOT_RUN |

## REC00 host inventory

The baseline host was macOS arm64 with AppleClang 21, CMake 3.31.12, and
MacPorts SDL2 2.32.10. The macos-macports configure/build, ROM-less
self-test, and smoke entry points completed successfully. The preset has tests
disabled, and no recording target or libav discovery was found in the current
CMake configuration. FFmpeg command-line tools were available for inspection;
VLC was not available.

Windows and Linux runtime/build evidence was not available in this session.
The native Metal source path is present in the checkout, but no recording
readback or end-to-end test was run. The D3D11 and OpenGL native rows therefore
remain NOT_RUN.

For SDL fallback, the CRT column means only effects actually implemented in
that path; it must not be used to claim native CRT support. Native capture is
renderer-independent in architecture, but integration tests must prove that
enabling a presenter does not move its tap.

## REC13 host evidence

The macOS arm64 host built both recording-enabled and recording-disabled
frontends. The enabled recording suite passed 14/14; the disabled suite passed
9/9. A private ROM-only run through the SDL software fallback and deterministic
host audio produced four Native intervals, which independently decoded as
FFV1/BGR0 640x400 video and PCM S16LE stereo 22050Hz audio. This is labelled
IMPLEMENTED_UNVERIFIED because it did not exercise a guest OS/media workload
with enabled OSD, a physical audio device, final Displayed composition, or VLC.
The macOS Metal, Windows D3D11, Linux OpenGL and other runtime rows remain
unverified.

Record the exact evaluated commit, OS/toolchain, architecture, backend,
device/driver, FFmpeg library configuration, VLC version and public synthetic
artifact identity when each row is tested. For private demonstrations publish
only neutral case IDs and pass/fail statements.

A cross-build is not a runtime test. WARP, llvmpipe or software SDL may supply
valid primitive tests when clearly labelled, but not evidence of another real
GPU setup. Native headless ROM-free fixtures do not prove an actual guest boots
or plays sound. An unavailable backend is NOT_RUN or BLOCKED, never PASS.
