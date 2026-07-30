---
id: 2026-07-30-16h33-2
date: 2026-07-30
time: "16:33"
title: A reserved key nothing writes is still load-bearing — 0x00 reserves an option, not a use
builds-on: 2026-07-30-16h33
supersedes:
---

**Before:** once tombstones turned out to be unnecessary, `0x00` looked free — a
reserved key value that nothing writes, and therefore a value that could be handed
back to users as one more usable key.

**What changed it:** two things, and the second is the real one. Clearing bits to
zero is the only edit flash permits, so an all-zero key is the sole way to mark a
single field dead without erasing a whole sector — and parked features want exactly
that (the handled-flags convention, editing a field in place). And the reason
tombstones are unnecessary is the *current* erase ordering, which is itself under
pressure to change, because keeping a sector erased ahead of the cursor costs a
whole sector of capacity.

**Now:** a reserved value that nothing writes today can still be load-bearing,
because what it reserves is an option rather than a use. Releasing it is
irreversible and buys one key out of 256 — one out of four billion with 4-byte keys
— while keeping it preserves both the only bit-clearing way to kill a field and the
freedom to change the erase ordering later. The general shape: judge a reserved
value by what it keeps possible, not by whether it is currently written.

**Follows:** `0x00` stays reserved; no key space handed back.
