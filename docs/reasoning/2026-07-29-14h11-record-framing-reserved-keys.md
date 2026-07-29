---
id: 2026-07-29-14h11-2
date: 2026-07-29
time: "14:11"
title: Record framing — reserved-key marker, marker-delimited, no stored length
supersedes:
---

Direction (not locked). Records are framed by reserved key values the record
layer interprets: `0xFF` = empty, `0x00` = tombstone, `0x01` = record-start
marker (header field), everything else = user data. A record is
`[marker header][data…]` delimited by the next marker / empty / tombstone — no
length is stored, you just read until the next boundary. Reasoning: with Option
B and no per-field housekeeping byte, the key is the only structured handle, so
the start marker must be a reserved key (a marker is unavoidable for
variable-length records; `0xFF`/`0x00` alone don't say "starts here").
Dropping length and making CRC optional collapses the header to just the marker
(which lives in the key), so it fits any field size — this dissolves the
header-space squeeze that worried us. Reserved keys also keep the field layer
dumb: it stores keys opaquely, and "these values are special" is pure
record-layer interpretation — unlike a housekeeping byte, which would change the
field format. Rejected: length-prefixed headers (need bytes the smallest fields
don't have); timestamp-as-marker (leaks record semantics onto the user);
per-field housekeeping byte (dirties the field layer + overhead). Leak
accepted: reserving key values shrinks the user key space — unavoidable here
and the least-bad option.

Decision: leaning reserved-key framing (`0xFF`/`0x00`/`0x01`), marker-delimited, no length — not locked; exact values for multi-byte keys TBD.
