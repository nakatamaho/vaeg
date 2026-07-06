# M9 VA boot differential debug

This report records the comparison setup for the PC-88VA FDC boot path.
It is instrumentation only; no boot behavior is intentionally changed.

## Config parity

Canonical comparison file:
`docs/agents/reports/m9_canonical_np2.cfg`

Use this file for both the portable build and the v141 legacy reference
when capturing boot traces.

| Key | Canonical value | Boot-path effect |
| --- | --- | --- |
| `pc_model` | `88VA2` | Selects VA model. This drives `pccore.model_va`, FDC default mode, VA memory, and VA I/O binding. |
| `clk_base` | `3993600` | Base CPU clock. Affects event timing including FDC and subsystem timing. |
| `clk_mult` | `2` | CPU clock multiplier. Keep identical for timing-sensitive FDC traces. |
| `DIPswtch` | `3e 73 7b` | Base system DIP bytes copied into the emulated machine state. |
| `MEMswtch` | `48 05 04 00 01 00 00 6e` | ITF memory switch bytes used by the guest boot code. |
| `ExMemory` | `1` | Extended memory size exposed to the guest. |
| `ITF_WORK` | `true` | VA ITF work-area behavior. Mismatch can alter the early boot path. |
| `biospath` | `.` | ROM lookup directory. For capture, ROMs live next to the executable. |
| `HDD1FILE`, `HDD2FILE`, `SCSIHDD0`..`SCSIHDD3` | empty | Disables HDD/SCSI boot paths so FDD is the only boot media. |
| `SampleHz`, `Latencys` | `22050`, `500` | Sound timing. Not a disk-data key, but kept fixed to avoid event cadence differences. |
| `SNDboard` | `200` | Enables VA Sound Board II binding. A mismatch leaves ports `0044`/`0045` unbound and contaminates traces. The portable parser is now 16-bit under `SUPPORT_PC88VA` at `sdl2/ini.c:369`. |
| `USE144FD` | `false` | Keeps the FDD geometry path on the legacy 2HD/2DD route. |
| `FDDRIVE1`..`FDDRIVE4` | `true`, `true`, `false`, `false` | Legacy FDD equipment bits. Portable builds currently use the same effective default, two drives. |
| `Sub_Mock` | `false` | Legacy-only switch between mock and real VA subsystem. Capture uses the real subsystem path. |
| `DipSw_VA` | `cd` | Legacy VA-specific DIP byte. Portable default matches this for the comparison. |
| `Use_VA91` | `false` | Disables VA91 extension path. |
| `Use_BMS_`, `BMS_Port`, `BMS_Size` | `false`, `00ec`, `16` | Disables BMS extension memory for the comparison. |

## Trace hooks

FDC operation lines include the current FDD interface mode in the common
format at `io/fdc.c:69` and `io/fdc.c:316`.

FDD interface mode writes are logged at the VA control register handler
`io/fdc.c:2470`. Reset-time default mode is logged at `io/fdc.c:2601`.
The register is `01b0h`; value bit 0 selects direct DMA mode when set
and Intelligent mode when clear.

If the boot path is Intelligent mode, the diff target is the subsystem
interface rather than the raw 765 data port. The real subsystem byte
stream is logged as `subiftrace` at `iova/subsystemif.c:34`; the mock
subsystem path logs `fdsubtrace` at `iova/fdsubsys.c:126`.

The legacy build can capture the same trace from process start by setting
`VAEG_TRACEOUT`. The environment variable is defined at
`win9x/trace.cpp:91` and opened during trace initialization at
`win9x/trace.cpp:476`.

## Portable capture

Command used:

```sh
mkdir -p /tmp/vaeg-m9-cfg/vaeg
cp docs/agents/reports/m9_canonical_np2.cfg /tmp/vaeg-m9-cfg/vaeg/np2.cfg
cd /home/maho/vaeg/build/linux-release/sdl2
timeout 30s env XDG_CONFIG_HOME=/tmp/vaeg-m9-cfg \
  SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
  ./vaeg --fdctrace "MS-DOS 2.11 PC-88VA.d88" \
  > /home/maho/vaeg/docs/agents/reports/m9_portable_msdos211.log 2>&1
```

`timeout` exits with status 124 because this is a normal boot run, not
`--smoke`. The captured log has 8164 lines.

## Portable observations

The previous `ports 0044/0045 unhandled` comparison contaminant is gone:
the portable log has no `iova-unhandled port=004*` entries.

The portable boot starts in Intelligent mode:

```text
fddiftrace reset mode=00/intelligent
```

The first successful boot-sector read in Intelligent mode is:

```text
fdctrace cmd=46/ReadData drive=0 C=00 H=00 R=01 N=03 mode=00/intelligent req=8192 st=00/00/00 xfer=1024 dma_ch=ff dma_access=00 dma_sysm_bank=00 sysm_bank=00 dma_len=0 dma_range=fffff-fffff
```

The actual data then flows through `subiftrace` response chunks, including
the boot sector contents beginning with:

```text
subiftrace mode=00 main2sub len=1 bytes=03
subiftrace mode=00 sub2main len=256 bytes=eb 1c 90 38 38 56 41 ...
```

Therefore `dma_ch=ff` on Intelligent-mode `fdctrace` lines does not prove
the DMAC is unprogrammed; the transfer path is the VA subsystem FIFO at
that point.

The guest later switches to direct mode:

```text
fddiftrace port=1b0 val=01 mode=01/direct
```

After that switch, DMAC programming is visible and direct-mode reads show
channel 2:

```text
dmatrace port=161 val=02
fdctrace cmd=46/ReadData drive=0 C=00 H=00 R=02 N=03 mode=01/direct req=7168 st=00/00/00 xfer=35 dma_ch=02 dma_access=00 dma_sysm_bank=00 sysm_bank=01 dma_len=1023 dma_range=32000-323fe
```

The `xfer=35` direct-mode count is not yet classified as truncation; it
must be compared against the legacy log captured with the same canonical
configuration and instrumentation.

## First divergence

Pending legacy log. No causal hypothesis is recorded yet because the
portable side alone cannot identify the first divergent operation.

## Legacy capture recipe

Build `win9x/np2_v141.sln` as usual, copy the canonical config next to
the legacy executable, and run one process with the trace file environment
variable set before launch. The v141 argument parser treats non-INI
positional arguments as FDD images (`win9x/np2arg.cpp:52`), and the disk
is queued for insertion at startup (`win9x/np2.cpp:2278`).

PowerShell:

```powershell
cd C:\Users\maho\vaeg\bin
copy C:\Users\maho\vaeg\docs\agents\reports\m9_canonical_np2.cfg .\np2.cfg
$env:VAEG_TRACEOUT = "$PWD\m9_legacy_msdos211.log"
.\vaegd.exe "MS-DOS 2.11 PC-88VA.d88"
```

Let the run reach the same failure/success point, then close the emulator.
Send the complete `m9_legacy_msdos211.log`; do not use a windowed
DebugView copy, because it drops the power-on prefix and is hard to copy
when the trace is busy.
