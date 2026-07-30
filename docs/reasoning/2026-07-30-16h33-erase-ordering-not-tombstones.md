---
id: 2026-07-30-16h33
date: 2026-07-30
time: "16:33"
title: Reclaim's erase ordering, not tombstones, is what stops orphaned tails being mis-read
builds-on:
supersedes: 2026-07-29-14h11-5
---

**Before:** a record spanning sectors A→B loses its start when A is reclaimed,
leaving an orphaned tail in B. We believed the library had to clean that up itself
by writing tombstones over the orphan during reclaim, and that this had to be a
structural guarantee not relying on CRC, because the library creates the hazard.

**What changed it:** the ring got built, and the hazard was tested rather than
assumed. Reclaim erases the *next* sector before anything is written into the
current one, so by the time new records are written where an old marker used to be,
the orphaned tail has already been erased. Records deliberately straddling sector
edges survived several laps with nothing reading as corrupt — including the sharpest
case, a full log refilled to a sector's exact last field. The test is not vacuous:
mutating the record-boundary check makes 27 records read as corrupt.

**Now:** the hazard is prevented by *when* we erase, not by cleaning up after it,
and no tombstone is ever written. This rests entirely on the erase-ahead ordering —
one sector kept erased ahead of the cursor, erased on entering the sector before
it. If that ordering ever changes, and there is pressure to change it because
erase-ahead costs a whole sector of capacity, the hazard returns and the tombstone
cleanup returns with it. The tombstone key keeps its meaning in the code; it simply
never gets written.

**Follows:** no tombstone-cleanup code, and the backlog item is dropped.
