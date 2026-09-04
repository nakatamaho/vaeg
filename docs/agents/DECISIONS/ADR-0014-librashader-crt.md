# ADR-0014: librashader CRT runtime and shader boundary

Date: 2026-09-04

Status: Accepted for M99

## Decision

M99 consumes the official librashader C API through the official
`librashader_ld.h` dynamic loader. The implementation is never statically
linked into VAEG. The vendored files are placed under `external/librashader`
because this repository's convention requires third-party material under
`external/`, rather than the task proposal's `third_party/` path.

The pin is the official `librashader-v0.12.0` release:

| Field | Value |
| --- | --- |
| Repository | https://github.com/SnowflakePowered/librashader |
| Release | `librashader-v0.12.0` |
| Commit | `87e8a97b50516d997defeaa168173dcd185d4022` |
| Source archive SHA-256 | `4bf8cf2489d00848dcabbf2163204093776082da4217d5a5db45e4cbf335cedf` |
| C API version | `5` |
| C ABI version | `2` |
| Implementation license | MPL-2.0 (`LICENSE.md`) |
| C headers/loader license | MIT, as stated by the upstream include README |
| `librashader.h` SHA-256 | `5d478897c391af3f60015810b67785ae1a286d262a845485276e36ded9f21e62` |
| `librashader_ld.h` SHA-256 | `bcffcbc854afb287c9f935c1a0e3b569f5e6775ef85914bd7f7025ad1f6bde33` |
| `LICENSE.md` SHA-256 | `69c15395f33bc9ce8e1d8b6cef42b7e49cdec4c6f5233d4b9cfc4bfa335f97f9` |

The release binaries are runtime inputs, not linked source dependencies. The
official release assets audited so far are:

| Artifact | SHA-256 |
| --- | --- |
| `librashader-aarch64-macos-v0.12.0-optimized.zip` | `49808004a4904f6a99e0231092dcfdfe52b7b61f68430a4c9f1e165749c4c90e` |
| `librashader-x86_64-macos-v0.12.0-optimized.zip` | `8b2a50cefacf4073e8fa4757bec30242a788068c4096a580d94430688c184767` |
| `librashader-aarch64-windows-v0.12.0-optimized.zip` | `acea3871d7c1cf345ae9da47378b399d89a1e05496160a652103ba5b7b5eb775` |
| `librashader-x86_64-win7-windows-v0.12.0-optimized.zip` | `f40941e5db58fa9123da76c74bf17e52d73ef89514d0ab8960cd77a875a27c19` |
| `librashader-x86_64-windows-v0.12.0-optimized.zip` | `521fe0f364bfa705883f9e99fb1733a3a24f0f403bcc5591b6b78f6cff289183` |

The macOS archives contain `librashader.dylib` and the Windows archives
contain `librashader.dll`; the archives also contain upstream static-library
files, but M99 does not consume those files. No Linux binary is published in
the audited release assets. The Linux OpenGL runtime must therefore be built
from this exact source pin in the Linux packaging/CI stage, with its resulting
artifact hash recorded before G99-6.

## Shader closure

The requested `crt-lottes-fast.slang` candidate was audited at the pinned
`libretro/slang-shaders` commit recorded in
`assets/shaders/crt/licenses/crt-default-provenance.md`. Its preset references
only that shader and has no include, LUT, texture, or secondary-pass closure.
The shader contains an explicit Unlicense/public-domain dedication. No
CRT-Geom, CRT-Royale, Mega Bezel, complete `slang-shaders` tree, GPL shader,
or unknown-license shader is copied or distributed.

The current asset names are `assets/shaders/crt/vaeg_crt_default.slangp`,
`assets/shaders/crt/crt-lottes-fast.slang`, and the adjacent license and
provenance notices. Their content is byte-preserved from the audited upstream
sources except for the local preset filename.

## Consequences

- VAEG remains BSD-2-Clause; the MPL-2.0 implementation is a separately
  loaded runtime with its required license notice.
- A missing library, unsupported runtime, ABI mismatch, or shader failure can
  disable the optional filter without changing the existing SDL renderer.
- Platform runtime hashes and the Linux source-built hash remain release-gate
  evidence, not source-tree binaries.
