---
id: 2026-07-30-16h10
date: 2026-07-30
time: "16:10"
title: Per-sector header copies are the power-safety mechanism for reclaim, not just structure
builds-on: 2026-07-28-16h34-2
supersedes: 2026-07-28-16h37
---

**Before:** per-sector headers were understood as a purely structural fix for
"sector 0 is special", and explicitly *not* a power-failure measure — note
2026-07-28-16h37 stated the two questions were orthogonal and that power-failure
was 100% the record layer's problem. In the code only sector 0 carried a header;
the copies were reserved space, deferred as untested.

**What changed it:** the ring began reclaiming sectors, sector 0 among them.
Reopening a wrapped log reported `FORMAT_MISSING` with every record still on the
chip and none of it reachable. Writing the header back immediately after the erase
— the option rejected in July as not power-safe — made that symptom disappear, so
a test then cut the power in the gap by erasing sector 0 by hand. The log was
unrecoverable. The July argument moved from *argued* to *demonstrated*.

**Now:** the two questions are not orthogonal. The per-sector copies are exactly
what makes reclaim survive power loss, because recovery needs a copy that the
in-progress erase is not touching — the structural choice turned out to be
load-bearing for power safety. A second thing fell out: re-writing an identical
header changes no bits, so NOR permits it, which means one function both formats a
store and restores an erased sector's copy. And "corrupt format" now means *every*
copy is corrupt, a weaker and more useful contract than before. Rests on: only one
sector being erased at a time, and at least two sectors existing. Still open: an
interrupted erase can leave two gaps, and the boundary is then ambiguous.

**Follows:** `format()` writes every sector, `init()` takes the first usable copy,
reclaim rewrites the copy after erasing. One field-layer test changed meaning and
was rewritten.
