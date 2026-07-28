---
id: 2026-07-28-16h37
date: 2026-07-28
time: "16:37"
title: Per-sector headers are a structural choice, not a power-failure fix
supersedes:
---

Clarifying the relationship between two earlier notes so they aren't conflated.
The decision to put a header in every sector
(2026-07-28-16h34-header-in-every-sector.md) is purely structural: it removes
the "sector 0 is special" case so any sector can be cleared and reclaimed
uniformly, with no single header sitting in the path of an erase. It does NOT
provide power-failure or torn-write safety. The field layer deliberately offers
no power-failure guarantees at all
(2026-07-28-16h34-defer-powerfail-to-record-layer.md); that concern is entirely
the record layer's. The "rewrite-on-reclaim is not power-safe" argument that
first drew us into this area is a record-layer consideration, not the reason
per-sector headers were chosen — the two questions ("how does the header
survive sector clearing?" and "what happens on power loss mid-write?") are
orthogonal, and the field layer only answers the first.

Decision: per-sector headers address sector-clearing structure only; power-failure stays 100% deferred to the record layer.
