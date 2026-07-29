# Write verification / integrity — where does it live?

**Open question, not decided.** We need to detect when data on flash isn't
what we intended to write (wear, a failed program, or an accidental rewrite).
Unclear whether that belongs in the field layer or the record layer.

## Current state

The field layer's `write()` does a **read-back verify**: after writing key and
value it reads them back and returns `FLASH_WRITE_ERROR` on mismatch. This is
the only way to catch a NOR write-once/AND collision — checking the HAL
`write()` return code can't, because on real flash the program op reports
success even though it only cleared bits and stored the wrong value.

Testing this path is blocked (`#10`): `RamFlash` `assert()`s on any `0→1`
write, so it aborts before `write()`'s read-back can observe the mismatch.
Exercising it needs a lenient/real-NOR mode on `RamFlash` (silently AND instead
of assert). The clear-only rewrite *success* case, by contrast, is testable
today.

## Options

- **Field layer owns it (read-back verify, today):** robust, immediate
  per-field error — but 2 writes + 2 reads + memcmps per write. Slower, and it
  all lands inside whatever lock the thread-safety layer adds later. See
  [defer-powerfail note](../reasoning/2026-07-28-16h34-defer-powerfail-to-record-layer.md):
  read-back verify is arguably the dumb field layer doing a record-layer job.
- **Record layer owns it (CRC over the record):** lean field writes, short
  critical sections; corruption caught when the record is read.

## The snag with CRC

A record CRC conflicts with a **deliberate** clear-bit rewrite — the
handled-flags use case (a worker clears a flag bit to mark an entry done). That
edit changes the bytes, so the original CRC no longer matches, and the record
looks corrupt. A CRC alone can't tell "corrupted" from "intentionally modified
after write." Resolving this likely needs a way to mark deliberate edits (the
"CRC valid vs deliberately-changed" idea) — see the handled-flags entry in
[ideas/parked-ideas.md](../ideas/parked-ideas.md).

## Consequence to weigh

Read-back verify adds time and plumbing to every write, which matters once the
thread-safety layer wraps writes in a lock. Don't optimize speculatively, but
the responsibility choice (field vs record) determines whether the read-back
code stays at all.

<!-- TODO: decide field-layer read-back vs record-layer CRC (+ deliberate-edit
     marking) when the record layer starts. Then close test gap #10. -->
