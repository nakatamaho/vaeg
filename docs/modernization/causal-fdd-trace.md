# Production-memory causal FDD trace

This change is a generic VAEG diagnostic facility. It does not contain a
firmware, disk, or FreeDOS-specific rule and does not establish a PC-88VA boot
contract.

## Implemented path

The main PC-88VA CPU reaches the existing `upd9002_memoryread` fetch boundary
and `iocore` I/O dispatch. The production memory decoder remains in
`memoryva`; the test-only flat-memory seam remains behind its existing test
macro. The FDD boundary is represented by the existing subsystem bridge and
its uPD780 execution path (`subsystemif`, `subsystemmx`, and `subsystem`), the
existing FDC implementation, the DMA controller, and the D88-backed FDD
backend. The causal stream observes these already-executed operations at their
existing boundaries. It does not add a second memory or device access.

The scheduler records subsystem initialization, reset, execution, wait, and
run transitions. The bridge records mailbox and handshake transitions. FDC,
drive, IRQ, DMA, sector-transfer, and instruction-fetch-correlation records
are emitted by the corresponding existing operations. The events are
diagnostic observations; they do not alter device state or scheduler timing.

The existing CPU trace remains a separate textual trace. This causal stream is
the ordered cross-subsystem stream intended to distinguish a main-CPU request,
subsystem consumption, drive/controller progress, transfer, and fetch.

The optional causal trace connects already-executed VAEG operations without
introducing guest memory or device reads. It is compiled only with
`VAEG_Z80_COMPAT_INTEGRATION_TRACE` and is disabled unless an output path and an
explicit event limit are supplied.

The stable event classes are `cpu_step`, `io_read`, `io_write`, `mem_read`,
`mem_write`, `irq_assert`, `irq_clear`, `irq_accept`, `device_schedule`,
`mailbox`, `drive_state`, `fdc_command`, `fdc_position`, `sector_transfer`,
`dma`, `instruction_fetch_correlation`, and `stop`. Records use monotonically
increasing logical sequence numbers and contain no wall-clock or host-path
fields. The JSON Schema for individual JSONL records is
`diagnostics/causal-trace.schema.json`.

Example controls are:

```text
--causal-trace-output OUTPUT --causal-trace-limit EVENTS
--causal-trace-manifest OUTPUT --causal-trace-ring EVENTS
--causal-trace-cpu NAME --causal-trace-device NAME
--causal-trace-io RANGE --causal-trace-memory RANGE
--causal-trace-stop EVENT_CLASS
```

Ranges are decimal or `0x`-prefixed hexadecimal single values, inclusive
hyphen ranges, comma-separated ranges, or `all`. An invalid range or an
unknown stop class fails closed. A ring buffer retains only its configured
number of final event records and the stop record still reports the total
number of accepted events.

CPU instruction bytes are supplied by the completed main-CPU fetch boundary.
The causal layer does not fetch them again. Memory-watch records are emitted
only for ranges explicitly selected by the caller. FDD transfer correlation
uses the destination range recorded by the existing transfer operation and
does not read that range again.

The public ROM-free selftests exercise the causal event chain, filtering,
ring-buffer retention, explicit event-limit stops, and deterministic output.
`tools/qa/causal_trace.py` validates a trace, compares two traces at their
first divergent event, detects repeated CPU wait signatures, and emits a
redacted projection for public diagnostics. Firmware, disk, trace, and
address-specific results remain external experiment inputs and are not part
of the VAEG repository.

The supported build matrix is P0 (trace off/tests off), P1 (trace on/tests
off), T0 (trace off/tests on), and T1 (trace on/tests on). P1 uses the normal
production memory implementation and does not compile the flat test-memory
backend. The public QA checks the compile definitions, link symbols, bounded
runtime output, malformed-range rejection, and two-run output identity.
