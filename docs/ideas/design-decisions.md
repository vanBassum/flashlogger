# Design decisions

_Bas's to make (per CLAUDE.md); nothing is decided until locked in._

## Decided

- **Sector-0 problem → per-sector reserved headers.** Every sector reserves
  header-sized space; sector 0 holds the real format header, the others stay
  `0xFF` (no duplicate header content yet). Structural, implemented — see
  [flash-format.md](../flash-format.md) and the reasoning notes
  [header-in-every-sector](../reasoning/2026-07-28-16h34-header-in-every-sector.md)
  / [structural-not-powerfail](../reasoning/2026-07-28-16h37-per-sector-headers-are-structural-not-powerfail.md).
- **Append cursor → record layer.** The field layer is index-addressed and
  holds no cursor.
- **One record open at a time; the newcomer is refused (2026-07-29).** Because
  `field()` writes straight to flash with no RAM buffering, two interleaved
  records would interleave on flash. So a record must be closed before the next
  opens, and while one is open `WriteRecord()` refuses — `field()` on that handle
  returns `RECORD_ALREADY_OPEN`. Chosen over the earlier sketch where the *older*
  handle went stale: nothing a caller holds ever dies under it. A record ends
  with `close()` or the destructor — both, deliberately: the destructor is the
  safety net for a forgotten close, and the explicit call is the only one that
  can *report* a failure, since eventually closing writes the CRC and a
  destructor has nowhere to put an error. `close()` is idempotent. Kept the name
  `close()` over `commit()` for handle-like familiarity, though `commit()`
  describes the CRC back-fill more honestly.
- **Reserved key values scale with key width (2026-07-29).** Empty is all-ones
  for the key width (`0xFF` at 1 byte, `0xFFFFFFFF` at 4), tombstone is `0`,
  record-start is `1`. The point is to use the flash's own states, so a plain
  `0xFF` is ordinary user data once keys are 4 bytes wide. Closes the
  "exact values for multi-byte keys TBD" part of the framing note.
- **A record is a list of key/value pairs (2026-07-29).** One log entry carries
  several tagged values at once (temperature *and* humidity), each pair becoming
  one Field — not one key with a long payload. The exact API shape (signature,
  the pair type, naming, whether append buffers or streams) is Bas's and is
  being designed directly.
- **`RecordLog::format` wipes the store (2026-07-29).** It erases every sector
  and then writes the header — always, no questions asked. No refuse-if-formatted
  guard. Reason: the append point is found structurally (the frontier between
  written data and `0xFF`), so leaving stale records behind after a format would
  put the cursor past last year's data. Open: whether *invalid arguments* should
  erase too (currently they do — the erase happens before the field layer
  validates).
- **Field layer is hidden (2026-07-29).** The consumer only ever touches
  `RecordLog`: it owns a `FieldStore` by value and exposes its own
  `format`/`init` that forward down. Nothing above the record layer
  constructs a `FieldStore` or names a Field.
- **Integrity → CRC per record, always on**, width derived from `valueSize`,
  verified by recompute-and-compare; edits clear the CRC to `0`. No stored
  length (records are marker-delimited). See the 2026-07-29 reasoning notes.

## Open

1. **Record header — remaining bits** — flags (do records have a status field
   at all?), and the CRC8/CRC32 polynomial choice (CRC16 reuses the existing
   `crc16`).
2. **Iterator shape** — caller-owned handle; forward-only?; how a lapped handle
   reports invalidity. Leaning (2026-07-06): the handle stores a hash of the
   records it spawned; a later read or overwrite uses it to tell whether the
   ring has since lapped/invalidated it.

## Other open questions

- **Record start marker / magic value** — how a record's start is recognized.
- **Thread-safety mechanism — and whose job is it?** Currently recorded as *not*
  the library's: the field layer omits threading and
  [architecture.md](../architecture.md) puts thread safety entirely in the
  consumer's manager. Bas raised (2026-07-29) that the library itself needs to be
  thread-safe eventually, which reopens that. Not acted on yet. Note the overlap
  with the stale-handle rule below: "opening a record invalidates the previous
  handle" is single-threaded reasoning, and two threads each opening a record is
  the same hazard wearing a different hat — so whichever way threading goes will
  shape what a record handle is allowed to be.
- **Sector management** — reclaim policy and when erasing happens. The field
  layer exposes `clear`; the *policy* is a record-layer / reclaim concern.
- **Field-layer read-back verify** — now that record integrity is a CRC,
  is `FieldStore::write()`'s per-write read-back still wanted or redundant? See
  [write-verification](../backlog/write-verification.md).

<!-- TODO: as each open item is decided, move it to "Decided" with the why. -->
