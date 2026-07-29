# Record layer

_Placeholder. This is the bulk of the project (M2)._

- Append (data fields first, header field last, CRC over the record).
- Mount / recovery after reboot (skip torn records).
- Record iterator.
- Hash-validity handle for read / overwrite of possibly-lapped records.

## Missing tests

- **`format()` wipes the store — implemented, untested.** `RecordLog::format`
  erases every sector before writing the header, but nothing proves it. The
  honest test is record-level — append a record, format, iterate and find
  nothing — which needs append and iteration first. A byte-poking version
  (scribble at a flash address, check it reads `0xFF`) was written and then
  removed: it inspected flash internals from the record layer and asserted
  "somebody erased" rather than "the log is empty". Write the real one once
  write/read exist.
- **Does a *rejected* `format()` erase?** Today it does — the erase runs before
  the field layer validates the sizes, so `format(0, 4)` wipes the store and
  then returns `ARG_INVALID`. Untested and undecided; see
  [design-decisions](../ideas/design-decisions.md).

<!-- TODO: depends on the design decisions in ideas/design-decisions.md. -->
