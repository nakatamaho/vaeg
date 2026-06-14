# Phase 1.7 clang-format Plan

## Current State

- HEAD before planning: `3aeaf90 Document absent legacy cleanup targets`
- Branch: `main`
- Phase 1.1 UTF-8 conversion is complete.
- Phase 1.3 rename normalization is complete.
- Phase 1.4 Visual Studio project inventory is complete.
- Phase 1.5 / 1.6 legacy cleanup checks are complete.
- Untracked instruction files intentionally excluded: `AGENTS.md`, `refactor-instructions.md`.

This document is a plan only. It does not add `.clang-format`, does not run clang-format, and does not change existing source formatting.

## Goals

- Introduce a formatting policy without changing behavior.
- Keep formatting changes isolated from logic, charset, line-ending, and rename changes.
- Preserve the VC2008 build gate and source ledgers.
- Exclude generated, macro-heavy, resource, table, and assembler inputs from automatic formatting.

## Commit Plan

### Commit 1: Add Formatting Policy Only

Files to add:

- `.clang-format`
- `.clang-format-ignore` or an equivalent checked-in target/exclusion list
- Optional short documentation update that points to this plan

No existing source files should be formatted in this commit.

Proposed `.clang-format` direction:

- Base style: `LLVM`, adjusted toward the existing code.
- Indentation: tabs for indentation.
- Tab width: 4.
- Indent width: 4.
- Column limit: disabled or high enough to avoid broad line wrapping.
- Pointer alignment: preserve a C-style look as much as practical.
- Sort includes: disabled.
- Reflow comments: disabled.
- Align consecutive declarations/assignments: disabled initially, to avoid noisy diffs.
- Break before braces: custom or close to the existing K&R-like style.

Rationale:

- The existing tree relies heavily on tabs and compact C style.
- Include order may encode platform or dependency assumptions; automatic sorting is not acceptable in this phase.
- Comment wrapping would be high risk for Japanese text and legacy comments.
- The first formatting policy should minimize churn even if it is not aesthetically ideal.

### Commit 2: Apply Formatting to a Limited Source Set

Apply formatting only after Commit 1 is reviewed.

Initial target candidates:

- Ordinary `.c`, `.cpp`, `.h`, and `.hpp` files.
- Files not listed in the exclusion set below.
- Files that do not include macro fragments in a way that makes formatting risky.

Do not mix formatting with logic changes, build changes, rename changes, or charset changes.

## Exclusion Set

Always exclude by extension:

- `*.mcr`
- `*.tbl`
- `*.res`
- `*.str`
- `*.x86`
- `*.s`
- `*.asm`
- `*.rc`
- `*.inc`

Always exclude by directory:

- `romimage/`
- `np2tool/`
- `help/`

Exclude generated or table-like inputs even if their extension looks like C/C++.

Keep under manual review before formatting:

- `i286c/*`
- `i286x/*`
- `i286a/*`
- `CPUXVA/*`
- `CPUCVA/z80*`
- `statsave.c`
- `statsave.tbl`
- `keystat.c`
- `keystat.tbl`
- `generic/unasm*`
- `sound/getsnd/*`

Rationale:

- CPU, decoder, table, save-state, and generated-looking files have a higher risk of formatting changing readability assumptions or macro structure.
- Assembler and ROM image sources are consumed by non-C/C++ tools and must not receive formatting or BOM/line-ending churn.
- Resource scripts and table fragments are not normal C/C++ translation units.

## Macro-Heavy File Handling

Before formatting any file in these areas, inspect it individually:

- CPU cores: `i286c/`, `i286x/`, `i286a/`, `CPUCVA/`
- Memory and hardware-adjacent code: `CPUXVA/`, `IO/`, `IOVA/`, `VRAM/`, `VRAMVA/`
- Generated or table-driven utilities: `generic/unasm*`, `sound/getsnd/*`

Stop and ask if formatting would affect:

- macro continuation layout
- include-order assumptions
- generated table readability
- assembler interface declarations
- save-state struct layout comments or binary layout documentation

## Charset and BOM Verification

Before formatting:

```sh
git ls-files -z | xargs -0 file
```

Record files that currently have UTF-8 BOMs. After formatting, compare the BOM set exactly.

Recommended BOM check:

```sh
git ls-files -z | perl -0ne 'chomp; open my $fh, "<:raw", $_ or die "$_: $!\n"; read($fh, my $b, 3); print "$_\n" if $b eq "\xEF\xBB\xBF";'
```

Required result:

- No file loses an existing BOM.
- No excluded assembler/resource file gains an unexpected BOM.
- All text files remain valid UTF-8.

Recommended UTF-8 decode check:

```sh
git ls-files -z | perl -MEncode=decode -0ne 'chomp; next if /\.(png|ico|bmp|exe|dll|lib|obj|pdb|idp|ilk|res)$/i; open my $fh, "<:raw", $_ or die "$_: $!\n"; local $/; my $s=<$fh>; eval { decode("UTF-8", $s, Encode::FB_CROAK); 1 } or print "$_\n";'
```

Required result:

- No newly undecodable text files.

## Line Ending Verification

Before formatting, record the line-ending classification for all target files. After formatting, compare it.

Recommended check:

```sh
git ls-files -z | perl -0ne 'chomp; open my $fh, "<:raw", $_ or die "$_: $!\n"; local $/; my $s=<$fh>; my $crlf=()=$s=~/\r\n/g; my $lf=()=$s=~/(?<!\r)\n/g; print "$_\tCRLF=$crlf\tLF=$lf\n";'
```

Required result:

- No broad LF to CRLF conversion.
- No broad CRLF to LF conversion.
- Existing line-ending style is preserved per file.

## Diff Verification

After applying formatting:

```sh
git diff --name-only
git diff --check
```

Required result:

- Only intended formatting target files differ.
- Excluded files have no diff.
- No unrelated project, resource, assembler, help, ROM image, or documentation churn appears unless explicitly planned.

## Windows Build Gate

Commit 1 is policy-only and does not require a Windows build gate, but the result should state that the build was not rerun.

Commit 2 changes source formatting and must use the Windows build gate:

```bat
tools\windows\build_vc2008.cmd Debug
tools\windows\build_vc2008.cmd Release
```

Required report:

- Debug result: success/failure/not run.
- Release result: success/failure/not run.
- Warning counts.
- Whether demo and PC-Engine smoke checks were run.

Do not proceed past Phase 1.7 formatting if the VC2008 build fails and the cause cannot be isolated.

## Stop Conditions

Stop and ask before applying formatting if:

- clang-format would touch excluded extensions or directories.
- macro-heavy files produce hard-to-review structural diffs.
- BOM or line-ending preservation cannot be verified.
- formatting changes any CPU, timing, save-state, disk, ROM, BIOS, sound, or input behavior.
- Windows build fails after formatting.

## Recommended Next Action

Create Commit 1 only:

- Add `.clang-format`.
- Add `.clang-format-ignore` or a checked-in formatting target list.
- Do not format source yet.
- Do not run the Windows build gate unless requested.
