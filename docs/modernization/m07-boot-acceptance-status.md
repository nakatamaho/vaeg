# M07 boot-acceptance instrumentation status

## Public-safe result

The production-memory causal trace now follows one consumed subsystem request
through the request boundaries, response path, FDC command lifecycle, initial
sector transfer, destination fetch, and project-authored marker. A private
integration gate observed that complete abstract path in two clean runs with
matching canonical projections and unchanged inputs. Exact firmware, media,
addresses, entry state, and trace values remain private.

This result is emulator evidence. It is not real-hardware verification and it
does not establish that a FreeDOS kernel boots or runs.

## Generic defects corrected

The D88 loader used a read/write host open even for media that was presented
read-only. It now opens the image read-only for format inspection and data
access. A ROM-free selftest mounts and reads a host read-only D88 fixture.

The production subsystem bridge carries a main-side port-B write into the
subsystem-side port-A input latch. Request-consumer events and the corresponding
data return now occur at that port-A read; the unrelated port-B read is not
reported as consumption. The subsystem integration test covers both the
positive consumer path and the negative nonconsumer port.

The trace also retains the consumed request identity across overlapping
attention edges, FDC issue and completion, sector transfer, and destination
fetch. This correlation state is trace-only and is covered by a ROM-free
overlapping-request regression test.

## Verification boundary

The P0/P1/T0/T1 build matrix, production-memory linkage check, tests-disabled
P1 check, flat-test-memory exclusion, trace on/off architectural comparison,
two-build P1 reproducibility check, ROM-free selftests, and privacy checks are
the public acceptance gates. Private ROM and D88 inputs are not required by
public CI and are never emitted by the trace QA fixtures.
