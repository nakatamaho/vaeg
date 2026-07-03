# AGENTS.md — vaeg (88VA Eternal Grafx)

PC-88VA emulator derived from Neko Project II. The canonical build is
`Win9x/np2.dsp` (VC6 format, with `np2.dsw`): it pulls the VA subsystem
(`IOVA/`, `VRAMVA/`, `CPUCVA/`, `CPUXVA/`, `BIOSVA/`, `I286X/`) plus the
shared NP2 core (root, `IO/`, `SOUND/`, `CBUS/`, `VRAM/`, ...). The SDL2
port lives under `sdl/`.

NOT canonical: `Win9xC/np2c.dsp` is a plain NP2 (PC-9801) configuration
using the C CPU core `I286C/` and referencing no VA subsystem. Treat it
and other legacy backends (`WinCE/`, `MacOS9/`, `Mona/`) as frozen.

The canonical project assembles NINE NASM files via custom build steps
(`CPUXVA/MEMORYVA.X86`; `I286X/{DMAP,EGCMEM,MEMORY}.X86`;
`Win9x/x86/{PARTS,OPNGENG,CPUTYPE,MAKEGRPH}.X86`; `Win9x/DCLOCKD.X86`)
with a hardcoded `c:\bin\nasm\nasmw` path — see M1 and inventory.md.

## How work is organized

All modernization work is milestone-driven. Read, in this order:

1. `docs/agents/ROADMAP.md`       — milestone sequence M0..M6 and gates
2. `docs/agents/CONVENTIONS.md`   — repo-state rules (encoding / EOL / naming)
3. `docs/agents/tasks/M*.md`      — the single task file you were assigned

Do exactly ONE milestone task per session. Every milestone ends at a
HUMAN GATE: the user builds and runs the emulator (V3-mode boot, bundled
VA demo, simple OS operation). Machine checks never substitute for the
gate. Never begin milestone N+1 until the user states that gate N passed.

## Repository state is transitional

Until M6 completes, source files are CP932. Until M5 completes, EOL is
mixed (mostly LF, some CRLF). Until M4 completes, file names contain
uppercase. Do NOT "fix" encoding, EOL, or file names outside the
milestone that owns that change — an unsolicited fix in an unrelated
diff is a defect, not a favor.

## Hard rules

- One concern per commit. Renames (`git mv`) go in rename-only commits
  with zero content changes. Reference fixups (#include paths, project
  file entries) go in a separate, immediately following commit.
- Never re-encode, re-wrap, or reformat lines you are not changing.
- `.dsp` / `.dsw` / `.sln` / `.vcproj` / `.vcxproj` stay CRLF at all
  times (enforced by `.gitattributes` from M5 onward).
- Never modify binary payloads: `ROMIMAGE/`, ROM/disk images, fonts,
  icons, cursors, wave data.
- Deletions happen only from a list the user has explicitly approved
  (M3 protocol).
- Mass mechanical commits (M5 EOL, M6 encoding) must have their commit
  hashes appended to `.git-blame-ignore-revs` in the same PR.
- Run the verification scripts named in your task file
  (`tools/repo/*.py`) and paste their output into the PR description.

## Build reference

- VS2008 baseline: `Win9x/np2.dsp` converted once to `.vcproj`/`.sln`,
  Win32 / Release, NASM required. Alive from M1
  until the end of M5; permanently frozen at tag `baseline-vs2008`
  when M6 lands (UTF-8 without BOM is not representable to the VC9
  compiler; see ROADMAP).
- VS2017, toolset v141: from M2. Charset flags are milestone-dependent:
  - M2–M5: `/source-charset:.932 /execution-charset:.932`
    (locale-independent compilation of CP932 sources)
  - M6+: see `docs/agents/tasks/M6_utf8.md` (decision recorded there)
- SDL2 (`sdl/`, CMake): out of scope for the M0–M6 gates unless a task
  file says otherwise.

## Commit messages

UTF-8, LF, English subject, `M<n>:` prefix. Example:
`M4: rename IOVA/* to lowercase (rename-only)`.
