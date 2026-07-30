# M84 - Legacy tool and ROM regeneration audit

M84 audits remaining legacy tool sources, ROM/resource regeneration flows, and
the `lio/` BIOS/LIO compatibility path after active machine-core sources have
moved out of the repository root.

Predecessor: approved G83.

Branch: `topic/m84-legacy-tool-rom-regeneration-audit`

Commit prefix: `M84:`

Candidate gate: `G84`

Report: `docs/agents/reports/m84_legacy_tool_rom_regeneration_audit.md`

Do not start M85. Do not merge M84 to `main` before G84 approval. Do not
declare G84 passed.

## Scope

M84 must audit:

- `accessories/`, including `bin2txt`, `lzxpack`, and `textout`;
- `np2tool/` legacy assembly utilities;
- `romimage/` regeneration makefiles and assembly sources;
- `lio/`, including `lio/lio.res`, `lio_initialize()`, `bios_lio()`, and the
  BIOS-simulation entry hook range in `bios/bios.c`;
- current CMake asset embedding through `cmake/embed_binary.cmake`;
- generated-asset consumers in the SDL2 frontend;
- release packaging assumptions for assets and ROM-related files.

`accessories/` is an initial deletion candidate because it is not part of the
active CMake build and appears to serve only old ROM/resource generation
flows. Any deletion must prove that active asset embedding uses
`cmake/embed_binary.cmake` and that no current build, test, release, or
manual gate depends on the legacy tools.

`lio/` is a deletion candidate only after audit, not an assumed inactive
directory. The audit must account for its current active CMake inclusion, the
`BIOS_SIMULATE` initialization path, the `0xf9950` to `0xf9990` BIOS/LIO
entry hook, the generated `lio.res` payload, and any guest-visible LIO/N88-BASIC
compatibility expectation. M84 may remove it only if the active VA gate and
documented compatibility scope prove it unnecessary; otherwise it must record
whether it is retained or deferred to the VA BIOS cleanup sequence.

## Non-goals

M84 must not modify binary payloads, ROM images, guest ROM contents, fonts,
icons, splash assets, or approved historical evidence. It must not replace
ROM generation tooling with a new unapproved ROM build pipeline.

## Validation

Run repository invariant checks, normal builds, native tests, release-package
smoke checks where available, and any focused checks for generated assets or
ROM/resource regeneration references.

## Closure

The final report must list each legacy tool, ROM-generation path, and `lio/`
hook as active-required, inactive-removable, deferred, or blocked by evidence
gap.
