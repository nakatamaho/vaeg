# Subsystem response producer provenance

This document records the public, firmware-independent provenance contract for
the optional production-memory causal trace. It does not contain firmware,
disk, or private experiment evidence.

## Source census

The PC-88VA floppy path has the following public implementation boundaries.
The production subsystem path is implemented by `io/subsystem.cpp` and
`io/subsystemif.c`, with FDC and drive state in `io/fdc.c`. The older
`io/fdsubsys.c` mock-up remains separately instrumented for its explicit mock
configuration; it is not used as evidence for the production uPD780 path.

| Boundary | Producer site ID | Observed mutation |
| --- | --- | --- |
| Main request handshake | `main_request_emitter` | Request token is allocated when the main-side attention handshake is asserted. |
| Subsystem request accept | `subsystem_request_acceptor` | The production uPD780 input callback observes the attention handshake and releases the subsystem wait state. |
| Subsystem request consume | `subsystem_request_consumer` | The production uPD780 mailbox input operation consumes the command/data byte; the mock path records the equivalent explicit state transition. |
| Subsystem command phase | `subsystem_command_phase` | Command receive, execute, send, and end-cycle phases transition. |
| Motor settle | `motor_settle` | The FDD motor changes from starting to stable at the existing event boundary. |
| Drive ready | `drive_ready` | The existing ready result is recorded at the drive-ready operation. |
| Media sense | `media_sense` | The selected disk mode and FDC sector-size state are recorded at configuration. |
| Response status | `response_status` | The subsystem command-status write records its old and new value. |
| Response mailbox | `response_mailbox` | A subsystem-to-main mailbox byte is written at the existing bridge callback. |
| Response consumer | `response_consumer` | The main side consumes a mailbox byte at the existing input callback. |
| FDC command queue | `command_queue` | A command is recorded when the FDC data-port command path accepts it. |
| FDC attempt/result | `fdc_attempt`, `fdc_issue`, `fdc_reject` | The existing FDC command lifecycle records attempt and result without injecting a result. |

The response-IRQ site is reserved as a stable identifier. The current public
source census does not claim that a separate response IRQ producer exists; the
mailbox handshake is traced at its actual write and consume callbacks. If a
future implementation exposes a distinct response IRQ mutation, it must use
the reserved enum rather than a source address or a private firmware offset.

These sites are represented by enums in `diagnostics/causal_trace.h`. They are
not source line numbers, pointers, guest addresses, or private ROM offsets.
The census therefore remains stable when implementation files are reorganized.

The expected public path is:

```text
request emitted -> request accepted -> request consumed
  -> command phase -> motor/media/ready state
  -> response/status producer -> response mailbox/consumer
  -> command queue -> FDC attempt and result
```

The census does not assert that every producer occurs for every command. The
analyzer reports the first missing transition against the configured path and
retains the observed producer-site list. A missing transition is a diagnostic
result, not a synthesized device state.

## Trace contract

`state_transition` is an optional causal-trace event. It records stable enum
names for the component, field, producer site, cause, and transition, plus
the old/new state values, predicate result, and deterministic request
correlation token. Existing operations supply values at their mutation or
completion boundary; the trace layer performs no additional guest memory,
port, or device reads.

Request tokens are allocated only while the optional trace is active. They are
monotonic within one bounded trace and contain no host time, pointer, process,
or path data. The current token is a trace-side correlation value and does not
alter guest-visible state or the emulation ABI.

The event stream remains bounded by the existing maximum-event and ring-buffer
controls. A normal or bounded stop is explicit. Trace-disabled builds use
no-op interfaces and retain the prior production behavior.

The correlation token is an instrumentation-side identifier. It is not a
guest-visible request field and does not synthesize an acknowledgement,
response, interrupt, command, or transfer.

## Analyzer result

`tools/qa/causal_trace.py provenance TRACE` validates the event stream and
reports:

- request correlation IDs;
- the last reached transition;
- the last writer for each observed field;
- the first absent producer/transition;
- the first unmet predicate state;
- the observed producer-site set; and
- an abstract fail-closed classification.

The analyzer does not interpret private firmware addresses or disk contents.
Its public projection redacts state values and architectural addresses while
retaining stable event and producer names.

ROM-free C and Python selftests cover the successful request-to-FDC path,
missing producer transitions, request correlation, event limits, ring
retention, and deterministic output. Runtime experiment evidence, if any,
must remain outside the repository.

The public causal-trace analyzer also provides a fail-closed privacy check for
public source and contract files. It rejects private-root markers, private
firmware/disk filename forms, and absolute user paths without examining any
private input.
