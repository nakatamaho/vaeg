# M07I-R2 causal FDD boot-trace result

This report records the abstract result of the private differential gate for
the generic production-memory causal trace. It contains no firmware, disk,
trace, address, or input identity.

## Result

```text
M07I-R2 PASS — B2 CAUSAL PATH DISTINGUISHED
diagnosis_code: NO_REQUEST
private_gate: performed
promotion_status: prohibited_pending_user_approval
```

The repeated clean differential runs reached the existing
`device_schedule` boundary, but no `io_read`, `io_write`, or `mailbox` event
was observed before the bounded stop. Consequently no `drive_state`,
`fdc_command`, `fdc_position`, `sector_transfer`, `dma`, or
`instruction_fetch_correlation` event followed in the captured causal path.
The abstract result is `NO_REQUEST`: the main-CPU-to-subsystem request was not
observed within the configured bounded execution. This is an emulator trace
observation, not a hardware result and not a PC-88VA firmware truth claim.

The empty-drive and media-present projections were byte-identical across their
respective repeated runs. The private result retains the concrete launch
inputs, event values, and immutable-input checks outside this repository.

## Boundary classification

| Boundary | Public result |
| --- | --- |
| `B0` attach | private gate performed; concrete input identity withheld |
| `B1` firmware boot path | reached in the bounded private gate |
| `B2` main/subsystem transaction | not observed |
| `B3` drive state | not reached after the missing request |
| `B4` FDC command | not reached after the missing request |
| `B5` sector transfer | not reached |
| `B6` destination fetch | not reached |
| `B7` project marker | not reached |

The causal event classes and runtime controls remain generic and are covered
by ROM-free tests. No private result is promoted into firmware-specific source
or a FreeDOS contract. The next milestone is M07R5, which must address the
request boundary using an approved public contract or a separately reviewed
observation decision.
