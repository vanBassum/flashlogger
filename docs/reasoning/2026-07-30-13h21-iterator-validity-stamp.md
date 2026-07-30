---
id: 2026-07-30-13h21
date: 2026-07-30
time: "13:21"
title: Iterator validity — remembered copy of the record's stored CRC, checked after the read
supersedes:
---

Direction (not locked). An iterator detects that its record changed by keeping a
**copy of that record's stored CRC** and comparing it to the current stored
value — not by recomputing a hash. Stored-to-stored costs one field read, needs
no recomputation and no buffer, and catches both a lap (the slot now holds a
different record with a different CRC) and an edit (an edit clears the CRC to
`0`), at the price of losing the distinction unless the caller inspects for `0`.
Crucially the check happens **after** the read, seqlock-style: check-then-read
would only reveal on call N+1 that call N handed back stale data, while
validate-after turns a changed record into an error instead of a bad value. It
stays probabilistic — a lapped slot hides at 1-in-2^width, and since width
derives from `valueSize` a 1-byte-value store gets CRC8 and 1-in-256 odds; exact
detection needs a monotonic sequence number, for which every sector already
reserves 8 unused bytes, so it can land later with no format change. Narrowing
that matters: only **reclaim** and **field edits** can alter a record under a
reader, because appends land at the frontier — so the ordinary
logger-appends/reader-reads case never conflicts. Thread safety is **orthogonal**
and not addressed: a stamp answers "did this change since I looked", locking
answers "can it change while I look", and no hash width closes the
check-then-act window. Rejected: recomputing a hash per API call (unnecessary —
the stored CRC already is the stamp, and recomputation is the expensive
integrity check, not the cheap identity one); a wider dedicated per-record hash
(costs flash to fix odds a sequence number fixes exactly); leaning on a checksum
for thread safety (structurally cannot work).

Decision: iterator validity = remembered copy of the record's stored CRC, compared after each read; probabilistic now, upgradeable to exact via the reserved per-sector sequence number; thread safety left to locking.
