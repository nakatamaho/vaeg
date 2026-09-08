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

# 88VA Eternal Grafx Rel.20260908.1

This maintenance release packages the optional CRT presentation pipeline for
the portable SDL2 frontend on Windows, Linux, and macOS.

## Highlights

- Standard SDL presentation remains available as the default fallback.
- Optional CRT presentation and unprocessed librashader pass-through can be
  selected from the display menu.
- The audited CRT preset and shader closure are embedded in static release
  binaries; no `librashader.dll`, `librashader.so`, or
  `librashader.dylib` is required.
- Native display screenshots preserve the displayed output where the backend
  supports readback; deterministic guest-frame screenshots remain available
  for QA.
- CRT screen size, curvature, scanline, and RGB-mask controls are stored in
  `vaeg.cfg`.
- If the native graphics backend or filter chain is unavailable, VAEG falls
  back to the existing SDL renderer.
- Windows and macOS release packaging is included and verified alongside the
  Linux package.

## Notes

ROM images, fonts, disk images, and other private integration media are not
included. Supply the model-specific PC-88VA ROM files separately. Native CRT
output depends on the host graphics driver; SDL fallback is intentional and
does not affect emulation or deterministic guest-frame capture.
