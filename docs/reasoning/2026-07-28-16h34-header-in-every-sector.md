---
id: 2026-07-28-16h34-2
date: 2026-07-28
time: "16:34"
title: Header in every sector; clearing sectors is the last field-layer step
supersedes: 2026-07-28-14h49-sector-0-leaning-per-sector-headers.md
---

Firming the earlier lean into a decision: put the header in every sector
(per-sector headers). The field layer owns the header and must be able to clear
and reclaim sectors, so making every sector self-describing removes the
"sector 0 is special" problem entirely — clearing any sector becomes uniform,
with nothing to protect or restore. What the per-sector header contains beyond
the essentials is deliberately left open; add fields only when a concrete need
appears (minimal effort). This makes "support clearing a sector, per-sector
headers intact" the final field-layer milestone before the record layer.
Rejected: a dedicated header-only sector (A — wastes a sector and keeps a
special case) and rewrite-header-on-reclaim (C — not power-safe).

Decision: per-sector headers; header contents deferred until needed; clearing-sectors is the last field-layer step before the record layer.
