# Power-failure handling

_Placeholder. A **record-layer** concern — deliberately not solved in the
field layer._

The field layer offers **no** power-failure or torn-write guarantees: a
half-written field reads back as if valid, with no detection (no per-field
housekeeping/CRC). This is by design — see
[defer-powerfail-to-record-layer](../reasoning/2026-07-28-16h34-defer-powerfail-to-record-layer.md)
and [per-sector-headers-are-structural-not-powerfail](../reasoning/2026-07-28-16h37-per-sector-headers-are-structural-not-powerfail.md).

To do (record layer):

- Crash-safe append: data fields first, header field written **last**, so a
  torn record is detectable (data without a valid header = skip as garbage).
- CRC over the record to detect torn / corrupt writes.
- Mount/recovery after reboot: skip torn records, find the append point.
- The power-loss test rig (RamFlash write interruption + `reboot()`) returns
  here to drive these tests — it was shelved for the field layer on purpose.

<!-- TODO: flesh out as the record layer lands. -->
