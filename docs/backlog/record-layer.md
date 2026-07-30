# Record layer — TODO

Reasoning lives in `docs/reasoning/`. This is just a list so things don't get
forgotten; items get deleted once implemented.

## Build

- [ ] Start marker field per record (`key=0x01`, value `0xFF` placeholder).
- [ ] CRC back-filled on close; width from `valueSize`.
- [ ] Iterator: position on a record, read a field by key, step to the next.
- [ ] Merge a value spread over repeated keys into a caller-supplied buffer.
- [ ] Validity stamp on the iterator (copy of the record's stored CRC, checked
      after each read).
- [ ] Mount/recovery: find the append point after reboot, skip torn records.
- [ ] Field edit (clear bits) — must also clear that record's CRC to `0`, so the
      iterator has to remember where the record *started*, not just where it is.
- [ ] Cache start/end pointers in `RecordLog` at `init()` so each new iterator
      doesn't rescan. Build the scan first, then cache.
- [ ] Bound every walk by the total field count — corrupt bytes must not spin
      forever.

## Decide

- [ ] Does iteration skip never-committed (all-ones CRC) records?
- [ ] Where does scanning start once the ring exists? Index 0 is only the oldest
      until the log laps.
- [ ] Caller's buffer too small for a merged value — error, or truncate and
      report the size needed?
- [ ] What does a *non-adjacent* repeat of a key mean? Adjacent repeats are one
      long value, but key 7 … key 9 … key 7 could be one value or two.
- [ ] Does a *rejected* `format()` erase? Today it does — the erase runs before
      the field layer validates, so `format(0, 4)` wipes the store and returns
      `ARG_INVALID`.
- [ ] Is `RECORD_ALREADY_OPEN` enough, or does a closed record need
      `RECORD_CLOSED`? Writing to a closed handle currently reports "already
      open".
- [ ] Should the library itself be thread-safe? Currently the caller's job.

## Tests we owe (blocked on reading)

- [ ] `format()` wipes the store. Real version: append records → `format()` →
      append one record → iterate finds exactly that one. Without the wipe the
      append point lands past the stale data.
- [ ] `field()` actually stores the key and value it was given — only the return
      code is checked today.
- [ ] A *rejected* `field()` wrote nothing and left the cursor alone.

## Decide — inherited from the deleted design-decisions.md

- [ ] Record header: any flags at all, or is the CRC the whole header value?
- [ ] CRC8 and CRC32 polynomial choice (CRC16 reuses the existing `crc16`).
- [ ] How a record's start is recognised during iteration (marker value is
      settled; the scan rule isn't).
- [ ] Reclaim policy and when erasing happens. The field layer exposes `clear`;
      the policy is a record-layer concern.

## Someday — not scheduled, revisit on real need

- [ ] **Handled-flags convention** (Bas, 2026-07-06): the app dedicates one
      key/value pair per entry as flags; a service iterates, does a job (ship
      logs somewhere), then clears bits to mark it handled. Pure app convention
      on top of clear-bits-only overwrite.
- [ ] Fast lookup / secondary index — jump to next/prev field with the same key
      (per-field linked offsets, or a per-sector key index).
- [ ] Ordered / binary search over records.

## Cheap fixes, no test yet

- [ ] Delete `RecordWriter`'s copy constructor — a copy would close twice.
- [ ] `close()`'s idempotency and its error return are both unexercised.
