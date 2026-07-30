# Record layer

_Placeholder. This is the bulk of the project (M2)._

- Append (data fields first, header field last, CRC over the record).
- Mount / recovery after reboot (skip torn records).
- Record iterator.
- Hash-validity handle for read / overwrite of possibly-lapped records.

## Read path — planned steps

Walked through 2026-07-30 and agreed as the shape to aim at. Each numbered
step is roughly one field-layer read, so the counts are the cost. Nothing is
buffered by the library: merge destinations belong to the caller. Steps marked
**decide** are still Bas's.

**Getting an iterator**

1. Refuse unless the store is mounted → `STORE_NOT_INITIALIZED`.
2. Scan fields from the start, reading each key.
3. Classify the key: **empty** (all-ones) → no more records, iterator is at
   the end; **marker** (`0x01`) → a record starts here, stop; **tombstone**
   (`0x00`) → skip and step on; **user data** → orphaned tail of a reclaimed
   record, skip forward to the next marker or empty.
4. Remember the marker field's index (the record start) and the CRC stored in
   its value — the validity stamp.
5. **decide:** an all-ones stamp means never committed (torn). Iteration is
   meant to skip those, so this becomes "step to the next record and repeat".
6. **decide:** where scanning starts once the ring exists. Index 0 is only the
   oldest record until the log laps; after that this needs the per-sector
   sequence numbers.

**Reading one field by key**

1. Take the remembered stamp — already held, no read.
2. Walk forward from record start + 1, reading key and value.
3. Marker, empty or tombstone → the record ended, key not found.
4. Wrong key → step on.
5. Key matches → copy the value into the caller's buffer.
6. Peek at the next field: same key → continuation of a long value, append and
   repeat; different key or a boundary → the value is complete.
7. **decide:** caller's buffer runs out mid-value — error, or truncate and
   report the size needed?
8. Re-read the marker field and compare its CRC to the remembered stamp.
   Moved → the record changed mid-read, discard and return an error.
   Unchanged → hand back the data. Validating *after* the read is the point:
   check-then-read would only reveal on the next call that the last one was
   stale, after the caller already used it.
9. **decide:** what a *non-adjacent* repeat of a key means. Adjacent repeats
   are one long value, but key 7 … key 9 … key 7 could be one merged value or
   two separate ones. This changes step 6.

**Moving to the next record**

1. Walk forward from the current start until a boundary key.
2. **marker** → next record; remember its index and stamp.
3. **empty** → end of the log.
4. **tombstone** → both ends the current record and is skipped; keep walking.
5. Every walk needs a hard ceiling (total field count) so corrupt bytes cannot
   spin forever — the "never derails on any flash contents" promise has to be
   structural, not a CRC check.

Note that A3, C1–4 and B2–4 are all the same classify-a-key loop; the first
test that needs it will effectively re-summon the `classifyField` helper
deleted in `5383391`, this time because something demands it.

## Improvements (not yet, but planned)

- **Cache the start and end pointers in `RecordLog`.** Found once during
  `init()` and kept, so creating an iterator does not rescan the store every
  time. Pure optimization over the scan described above — build the scan first,
  then cache.

## Missing tests

- **`format()` wipes the store — implemented, untested.** `RecordLog::format`
  erases every sector before writing the header, but nothing proves it. The
  honest test is record-level — append a record, format, iterate and find
  nothing — which needs append and iteration first. A byte-poking version
  (scribble at a flash address, check it reads `0xFF`) was written and then
  removed: it inspected flash internals from the record layer and asserted
  "somebody erased" rather than "the log is empty". Write the real one once
  write/read exist. Two scenarios, the second being the one that actually
  bites — build the failure, then show the wipe fixes it:
  1. append records → `format()` → iterate finds nothing.
  2. append records → `format()` → append *one* record → iterate finds exactly
     that one. Without the wipe, the append point is found past the stale data
     (the frontier is structural — written bytes vs `0xFF`), so the fresh
     record lands after a field of leftovers and iteration sees garbage.
- **Does `field()` actually store what it was given?** `a_record_takes_a_field`
  only checks the call returns `OK`; the key and value landing on flash is taken
  on trust, because there is no way to read a record back yet. Same for the
  reserved-key rejections — nothing proves a rejected `field()` wrote nothing and
  left the cursor alone. Tighten both once reading exists.
- **Writing to a closed record reports `RECORD_ALREADY_OPEN`.** After
  `close()`, the handle drops its log pointer, so a later `field()` reports the
  same error as a refused handle — misleading wording ("already open" for a
  record that is closed). Needs either a second error value (`RECORD_CLOSED`) or
  a decision that one value is enough. Untested either way.
- **Copying a `RecordWriter` would close the record twice.** Return-by-value from
  `createRecord()` is elided so it does not bite today, but nothing forbids a copy.
  Deleting the copy constructor is the obvious fix — no test demands it yet.
- **Does a *rejected* `format()` erase?** Today it does — the erase runs before
  the field layer validates the sizes, so `format(0, 4)` wipes the store and
  then returns `ARG_INVALID`. Untested and undecided; see
  [design-decisions](../ideas/design-decisions.md).

<!-- TODO: depends on the design decisions in ideas/design-decisions.md. -->
