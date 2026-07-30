# Record layer — TODO

Reasoning lives in `docs/reasoning/`. This is just a list so things don't get
forgotten; items get deleted once implemented.

## Ring behaviour — not yet, but critical to get right

Everything below assumes a linear store. It is wrong once the log wraps, and
these are the specific places that break:

- [ ] **The oldest record is not at index 0.** It can be at field 1000.
      `firstRecord()` hardcodes 0.
- [ ] **`init()`'s append-point scan is linear-only.** It walks from 0 to the
      first empty field. After a wrap that pattern doesn't hold.
- [ ] **Walks must wrap, so they can no longer end on the field layer's
      out-of-bounds error.** Termination currently *borrows* that bound; a
      wrapping walk continues at 0 instead, so the record layer needs its own
      step budget of at most the total field count.
- [ ] **Decide: how to tell newest from oldest.** On a full store, scanning alone
      cannot. Either keep one sector always erased so the gap *is* the boundary
      (append point just before it, oldest just after — no metadata, costs one
      sector, nothing to update on power loss), or put sequence numbers in the
      per-sector header bytes already reserved (exact, more moving parts).
      Leaning: the gap.

## Build

- [ ] Merge a value spread over repeated keys into a caller-supplied buffer.
- [ ] Validity stamp on the iterator (copy of the record's stored CRC, checked
      after each read).
- [ ] Mount/recovery: the append point is found, but torn records are not
      skipped during iteration yet.
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
