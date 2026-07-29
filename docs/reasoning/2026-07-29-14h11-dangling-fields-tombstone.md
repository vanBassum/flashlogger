---
id: 2026-07-29-14h11-5
date: 2026-07-29
time: "14:11"
title: Dangling fields at reclaim — tombstone cleanup as a structural guarantee
supersedes:
---

Direction (not locked). A record spanning sector A→B leaves an orphaned tail in
B when A is reclaimed; if A is later refilled *exactly* to its boundary, that
orphaned tail mis-attaches to A's new last record — structural corruption the
library itself causes. Because the library creates it, the reliability contract
requires the library to fix it, and without relying on CRC. Fix: on reclaim of
a sector, scan the next sector from its start and clear leading data-field keys
to `0x00` (tombstone) until the first marker or empty — everything before that
marker was a continuation of the erased record, and this needs no knowledge of
the erased header. This imposes an iteration rule on the codec *now*: a
tombstone (`0x00`) is skipped **and** ends the current record, so a record is
`[0x01][data…]` terminated by the next `0x01` / `0xFF` / `0x00`. CRC-on would
also catch a mis-attach (folded-in fields fail the CRC), but tombstoning is the
CRC-independent guarantee the contract demands. Rejected / parked: a per-sector
"first-record offset" header (needs per-sector headers populated — deferred,
more moving parts); forbidding records from spanning sectors (wastes sector
tails and caps record size — loses the variable-length efficiency win).

Decision: leaning tombstone-cleanup-at-reclaim; codec treats `0x00` as skip + record boundary now — reclaim mechanics land in M3.
