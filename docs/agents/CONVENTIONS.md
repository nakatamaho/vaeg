# CONVENTIONS — repository state rules

The repository is migrating. The rule set depends on which milestones
have merged. "Current" below means HEAD of main.

## State matrix

| Property        | Before M4/M5/M6 (transitional)        | Target (after M6) |
|-----------------|----------------------------------------|-------------------|
| Text encoding   | CP932, no BOM                          | UTF-8, **no BOM** |
| EOL             | mixed (mostly LF; some CRLF)           | LF, except CRLF list below |
| File names      | mixed case (IOVA/, *.C, ...)           | lowercase ASCII only |
| Commit messages | UTF-8 + LF (unchanged)                 | UTF-8 + LF |

## Permanent CRLF exceptions

`*.dsp *.dsw *.sln *.vcproj *.vcxproj *.vcxproj.filters *.bat`
(VC6/VS project machinery and cmd scripts; some of these parsers
require CRLF). Everything else is LF after M5.

## Permanent binary set (never renormalize, never re-encode)

ROM/BIOS/font/disk images and media: `*.rom *.bin *.d88 *.ico *.cur
*.bmp *.gif *.png *.wav`. Text sources under `romimage/` are not
binary. `.gitattributes` (added in M5) is the single source of truth.

## Permanent CP932 text exception

`hlp/**` remains CP932 after M6. HTML Help Workshop (`hhc.exe`) cannot
compile UTF-8 help sources, and the HTML files declare
`meta charset=Shift_JIS`, which matches the CP932 bytes on disk.

## Naming (target, enforced from M4)

- Tracked paths: `[a-z0-9._-]` only, directories included.
- No two tracked paths may collide under case-folding
  (`tools/repo/check_case.py` enforces both rules).
- `#include` / NASM `%include` / .rc includes must match the on-disk
  path byte-for-byte (case-sensitive filesystems are a supported
  target via the SDL2 port).

## Documented exemptions

`AGENTS.md`, `README*`, and files under `docs/` may keep conventional
uppercase names. The exemption list lives in
`tools/repo/check_case.py` (`ALLOW`), nowhere else.

## Encoding rules for agents writing NEW files

- Before M6 merges: match the surrounding code — CP932 for anything
  that the VS2008/VS2017-CP932 builds compile; UTF-8 for
  `docs/agents/**` and `tools/repo/**` (never compiled).
- After M6: UTF-8 without BOM everywhere.
