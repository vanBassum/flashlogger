# Write verification / integrity

**Record-level integrity is decided.** A CRC per record, **always on**, width
derived from `valueSize` (1→CRC8, 2–3→CRC16, ≥4→CRC32), written last as the
commit signal and verified by **recompute-and-compare**: `R == S` = intact;
on mismatch, stored `0xFF` = torn, `0` = edited (trust), else = corrupt. Edits
clear the CRC to `0` (clear-bits, app-agnostic). See reasoning notes
[crc-verify-by-recompute](../reasoning/2026-07-29-14h29-crc-verify-by-recompute.md)
and [crc-always-on](../reasoning/2026-07-29-14h32-crc-always-on-width-from-value-size.md).

## Still open

- **Field-layer read-back verify** — `FieldStore::write()` still reads back and
  returns `FLASH_WRITE_ERROR` on mismatch. Now that a record CRC covers
  integrity, is per-write read-back still wanted, or redundant? And its
  `FLASH_WRITE_ERROR` path (test gap #10) stays untestable without a
  fault-injecting flash double (silently AND on `0→1` instead of asserting).
- **User-chosen CRC width** — width is derived from `valueSize`; a `format`
  parameter + a stored width byte would only be needed to leave header room for
  flags (itself undecided). Deferred.

<!-- TODO: decide the field-layer read-back's fate when the record layer's CRC
     lands; build the fault-injecting double there if still wanted. -->
