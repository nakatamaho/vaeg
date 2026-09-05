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
# Default CRT shader provenance

The bundled default is a two-pass preset: the independently authored VAeg
BSD-2-Clause `vaeg-screen-size.slang` followed by the unchanged audited
`crt-lottes-fast.slang` from the official `libretro/slang-shaders` repository.
The new pass has no includes or external textures. Its complete BSD notice is
embedded in the shader. It maps centered texture coordinates at 80-120 percent,
with explicit black outside the image. No upstream shader code was copied into
this pass. The preset is VAeg-authored configuration; the upstream identities
below describe the original CRT pass and its original preset, not the new
two-pass configuration.

- Repository: https://github.com/libretro/slang-shaders
- Audited commit: `4812a82f6c9a11cc8b5a7447040a98c9fc80c00e`
- Preset source path: `crt/crt-lottes-fast.slangp`
- Shader source path: `crt/shaders/crt-lottes-fast.slang`
- Preset SHA-256: `0992238001c519503d9e8e750b37b32d30ff380e74a40287863605ae69f9fff0`
- Shader SHA-256: `576eddc662ac4f77909c0c14dbd5a16ac4164e50c67527fff634316f4441c482`
- Dependency closure: one preset and one shader; no `#include`, LUT, texture,
  or secondary shader dependency.
- License evidence: the shader contains an explicit Unlicense/public-domain
  statement. The corresponding notice is stored beside the preset.

The complete `slang-shaders` repository is not bundled. GPL-licensed or
unidentified shader families are not part of this feature.
