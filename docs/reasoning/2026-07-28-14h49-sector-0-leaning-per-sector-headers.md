---
id: 2026-07-28-14h49-2
date: 2026-07-28
time: "14:49"
title: Sector-0 problem — leaning toward per-sector headers (approach B)
supersedes:
---

For keeping the format header alive when sector 0 is eventually reclaimed, we
are leaning toward per-sector headers (approach B) — not yet locked. The
decisive reasoning is that erase-then-rewrite is not atomic, so any scheme that
keeps the only header copy in the sector being erased (approach C,
rewrite-on-reclaim) has an unavoidable power-fail dead window and is a trap.
Approach A (a dedicated header-only sector) is safe and simplest but spends a
whole sector on an 8-byte header. B is favored because its per-sector sequence
numbers are also exactly what mount/recovery in the record layer (M2) will need
anyway. Not yet validated against power-failure tests.

Decision: leaning per-sector headers (B); A viable fallback; C rejected as not power-safe.
