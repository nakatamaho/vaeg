<!--
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
-->

# 88VA Eternal Grafx Rel.20261101 (M142)

Changes since Rel.20261025, through the explicitly accepted M142 scope.
This is not a claim that all outstanding emulator issues are resolved.

## Important: new save-state format

**Old SAV files are intentionally incompatible with this release.** Keep
original snapshots and media backed up, and create new snapshots in a new slot.
There is no automatic old-format migration.

- BMS device state and its complete RAM now restore together, independently
  of the reader's next-reset configuration. Disabled/smaller/different-port
  reader settings no longer silently discard saved BMS RAM.
- Saved disk references use explicit portable records and verify media by
  content, not merely filenames. All FDD, SASI and SCSI slots, including empty
  slots, restore consistently. Missing, changed or ambiguous media is rejected.
- GUI/debug callers report actual restoration errors rather than treating
  warnings or failed loads as success.
- Read-only inputs open read-only. Failed saves and quick-undo preparation
  preserve existing snapshots instead of silently destroying them.

Snapshots do not contain or roll back external disk contents. Writes to a disk
following a save can therefore invalidate that snapshot's media identity.
Actual Darwin/Linux exchange has been tested, including translated Linux
x86_64 execution; universal state ABI/model compatibility is not claimed.

## Runtime settings and CPU roles

- Fresh settings leave EMS/BMS expansion disabled, reducing default snapshot
  sizes. Explicit existing choices remain respected.
- PCG and VA N-mode availability default ON when their configuration keys are
  absent. Explicit OFF remains OFF. Boot remains automatic; N-BASIC still needs
  the appropriate user-provided ROM.
- CPU clock menu offers 4, 8, 10, 12 and 20MHz without globally scaling device
  clocks. Main and FDC CPU roles remain independent.
- Includes the accepted M139 main-CPU role separation, scoped M140 VA2 FDC
  flag behavior and M141 TSP sprite timing work. These do not assert universal
  original-VA FDC or physical-Zilog conformance.
- FPS text handles three-digit rates; actual virtual-X11 observations exceeded
  100 FPS at both normal and 380-pixel client widths. The original reported
  digit-loss case was not reproduced on that target.

## Media, builds and identity

- Utility disks no longer unconditionally load the BMS RAM-disk driver when
  the board is absent. The driver and documented opt-in remain available.
- DOS-based media tools can use the isolated DOSBox/Open Watcom route on macOS.
- Complete MacPorts MinGW builds include recording and shaders without Colima.
- Release identity includes the accepted milestone: `Rel.20261101 (M142)`.
  The Windows archive is `vaeg-20261101-m142-windows-x64.zip`; its executable
  remains `vaeg.exe`.
- Public builds keep bundled local H.264/AAC encoders OFF. Windows/macOS use
  OS-provided export encoders where supported. The separately opted-in Linux
  personal codec build is **not a distributable release artifact**.

## Verification and remaining issues

The accepted gate includes the standard maintainer-reported Windows human
checks, recorded demo/media generation and guest drawing/boot contracts,
new-format state regressions, actual Darwin/Linux state exchange and the final
PC-8801mkIISR 4MHz real clipboard-paste check. The 40 clock cases pass guest
execution/state checks; failed OCR checks remain explicitly deferred.

The maintainer approved carrying forward VA-compatible low-clock paste,
clock OCR robustness, frozen-reference visual comparison and additional
Windows/Cocoa/physical-GPU/HiDPI automation. PC-Engine redirected LHA is an
optional test whose PCEPAT prerequisite was not established, not a demonstrated
emulator defect. Issue #75's investigation is complete for M142, but its cause
and correction remain deferred and the issue remains OPEN.

See [G142 closure and remaining issue definitions](docs/agents/tasks/M142_runtime_portability.md#g142-closure-and-redefined-issues)
and the [permanent correctness ledger](docs/modernization/bug-fixes.md).
Private ROMs, system media, states, captures and personal codec-enabled binaries
are not included in the release.
