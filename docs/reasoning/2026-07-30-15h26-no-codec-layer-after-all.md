---
id: 2026-07-30-15h26
date: 2026-07-30
time: "15:26"
title: No codec layer after all — the pure part turned out to be a helper
supersedes: 2026-07-29-14h11
---

The record layer was going to split into a pure codec (record ↔ field-run, marker,
CRC — unit-testable in isolation) plus a stateful `RecordLog`. Built out, the codec
never appeared, and the reason is structural rather than laziness: almost
everything needs to read flash. The CRC is computed by walking a record's fields
one at a time — deliberately, because buffering an unbounded record is off the
table — and deciding whether a record is intact means reading its marker. What is
genuinely pure turned out to be only key classification and the CRC width/fold:
a small function and a small class, not a layer. So the seam would have been a
file boundary with nothing behind it. The `classify_key()` helper was extracted
anyway, for a different and concrete reason: five separate walks were comparing
keys against the reserved values by hand, each handling the four cases
differently with the differences buried in the order of if-statements, and the
ring work will add wrapping to four of those five. Rejected: a `record_codec`
module (already tried and deleted once as premature in 5383391 — there is still
not enough pure logic to justify it); leaving the five hand-written comparisons
alone (they were about to be edited in parallel).

Decision: no codec layer — the pure part is a `classify_key()` helper plus the CRC fold, both living inside `record_log.cpp`.
