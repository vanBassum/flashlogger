# Record layer — TODO

Reasoning lives in `docs/reasoning/`. This is just a list so things don't get
forgotten; items get deleted once implemented.

## Ring behaviour — writes wrap; reading a wrapped store does not work yet

Writes now wrap and reclaim: stepping into a sector erases the *next* one, so one
erased sector always sits ahead of the cursor. That gap is the boundary marker —
no metadata to keep, nothing to half-update on power loss. Decided over sequence
numbers in the reserved header bytes, which stay available if exactness is ever
needed (no format change).

Reads wrap now too: every walk carries a step budget of the ring size instead of
ending on the field layer's out-of-bounds error, and `firstRecord()` finds the
oldest surviving record by starting at the append point and walking to the first
marker — which also steps over an orphaned tail, since those are data fields with
no marker.

`init()`'s scan turns out to be correct as-is: because there is exactly one
contiguous gap, the first empty field found from slot 0 is always the append
point. Left alone rather than rewritten.

Still broken or unproven, in rough order:

- [ ] **A record spanning a reclaimed boundary loses its own marker.** Seen for
      real: on a 2-sector store the erase-ahead wiped the marker of the record
      being written, and `RamFlash` caught the illegal write. This is the
      "dangling fields" hazard from the reasoning log, and the tombstone cleanup
      is the fix.
- [ ] **Minimum sector count.** With 2 sectors, "erase one ahead" erases the
      sector the cursor just left, so nothing survives and boundary-spanning
      records break. Decide the minimum (3? 4?) and enforce it in `format()`.
- [ ] **A record longer than the ring** eats its own start. No guard.
- [ ] **Erase-ahead erases the sector ahead of the cursor, which is only the
      oldest data when there are enough sectors.** Check the arithmetic holds for
      the minimum once it's chosen.
- [ ] `init()` now walks the whole store twice — once to spot a half-erased
      sector, once to find the append point. Fine for correctness, wasteful at
      mount. The cached start/end pointers would fold both into one pass.

## Build

- [ ] Merge a value spread over repeated keys into a caller-supplied buffer.
- [ ] Validity stamp on the iterator (copy of the record's stored CRC, checked
      after each read).
- [ ] Field edit (clear bits) — must also clear that record's CRC to `0`, so the
      iterator has to remember where the record *started*, not just where it is.
- [ ] Cache start/end pointers in `RecordLog` at `init()` so each new iterator
      doesn't rescan. Build the scan first, then cache.
- [ ] Bound every walk by the total field count — corrupt bytes must not spin
      forever.

## Decide

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
- [ ] What should `format()` do while a record is open? It wipes and resets the
      cursor but leaves `record_open_` set, so `createRecord()` refuses
      afterwards — forever. Untested, undecided: refuse the format, or close the
      record first.
- [ ] A failed `field()` write still advances the cursor: `next_index_++` is
      evaluated in the call, so a `FLASH_WRITE_ERROR` skips a position instead of
      retrying it. Harmless for the out-of-bounds case, wrong for a real fault.

## Tests we owe

- [ ] `read()`'s "key not found" currently returns `ARG_INVALID` as a
      placeholder, and running off the end of the store returns
      `ARG_OUT_OF_BOUNDS` — two different answers to the same question, neither
      tested. Needs a real error and a decision.
- [ ] `read()` clobbers the caller's `value_out` while walking, even when the key
      is never found. Harmless today, ugly contract.
- [ ] `read()` takes a buffer size and refuses one smaller than `valueSize`, but
      the size is only a floor check — once a value can span several fields it
      has to bound the *merge* too.
- [ ] `firstRecord()` assumes the first record starts at index 0. True while
      `format()` wipes and appends start there; wrong once reclaim can leave
      tombstones ahead of it, which is when the scan becomes necessary.
- [ ] `createRecord()` ignores the result of writing the marker — there is no
      error channel, since it returns a handle rather than a status.

## Decide — inherited from the deleted design-decisions.md

- [ ] Record header: any flags at all, or is the CRC the whole header value?
- [ ] Reclaim policy and when erasing happens. The field layer exposes `clear`;
      the policy is a record-layer concern.

## Typed API over private raw access (Bas, 2026-07-30)

- [ ] Make the raw `void*` + size `field()`/`read()` **private**, and expose
      typed overloads on top (`field(key, uint32_t)`, `read(key, float&)`, …).
      The size then comes from the type at compile time, so the
      buffer-too-small class of bug becomes unrepresentable rather than
      checked at runtime — the current `ARG_INVALID` floor checks in
      `writeField`/`read` only exist because the caller can lie about the size.
      Raw access stays for the blob / multi-field case.

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
