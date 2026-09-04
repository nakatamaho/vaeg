# Subsystem Request Consumer Gate

The production PC-88VA path uses an I8255-backed, direct-latch bridge between
the main CPU and the emulated floppy subsystem. It does not use a generic
software queue. The causal trace therefore distinguishes latch operations from
subsystem CPU consumption instead of naming every operation a queue event.

The stable mailbox boundary sequence is:

1. `REQUEST_ACCEPTED` — the main-to-subsystem attention edge is accepted.
2. `ROUTE_SELECTED` — the fixed production bridge is selected.
3. `MAILBOX_ENQUEUE_ATTEMPTED` — the main-side data latch write is attempted.
4. `MAILBOX_ENQUEUE_COMMITTED` — the main-side latch accepts or rejects it.
5. `MAILBOX_REQUEST_VISIBLE` — the subsystem-side latch receives the data.
6. `SUBSYSTEM_DISPATCHED` — the attention edge releases the subsystem wait.
7. `MAILBOX_DEQUEUE_ATTEMPTED` — the subsystem CPU requests the main-to-subsystem data latch.
8. `CONSUMER_CALLBACK_ENTERED` — the production port handler is entered.
9. `REQUEST_CONSUMED` — the subsystem-side port read completes.
10. `RESPONSE_ELIGIBLE` — command processing may proceed after consumption.

Every event carries stable enum-derived producer, consumer, channel,
predecessor, predicate, reason, and request-correlation fields. It contains no
source address, pointer, host timestamp, or firmware-specific identifier.

The request correlation is allocated at the attention edge and is propagated
through the direct-latch bridge to the subsystem port handler. A boundary is
not inferred from scheduler activity, motor state, or a later response. The
Python analyzer in `tools/qa/causal_trace.py` reports the first absent boundary
and rejects correlation discontinuity as a separate diagnostic result.

When the subsystem-side port read completes, the trace retains that consumed
request as the active command correlation. Later main-side attention edges may
start another request while the FDC is still completing the consumed command;
they do not re-label the in-flight FDC lifecycle, completion response, DMA, or
instruction-fetch correlation. This retained identifier is trace-only and does
not alter the guest-visible latch or subsystem state.

The production bridge routes a main-side port-B write into the subsystem-side
port-A input latch. Consumer boundaries are therefore emitted only by the
subsystem port-A read. A port-B read is a distinct latch operation and must not
be reported as request consumption.
