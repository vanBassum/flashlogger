---
id: 2026-07-29-12h10
date: 2026-07-29
time: "12:10"
title: Symmetric layout via reserved space; expose fieldsPerUnit, keep internals out of tests
supersedes:
---

Implemented the symmetric layout by reserving header-sized space at the top of
every sector, but only sector 0 holds a real header — the rest stay 0xFF.
Duplicating header content into every sector is a separate, currently-untested
feature (needed later for reclaim/recovery), so we reserve the space now but
don't fill it, which means no layout change when it lands. The trigger for
doing symmetry now (rather than deferring) was a testing principle: tests
shouldn't encode internal layout. The old clear tests hardcoded "sector 1 holds
5 fields"; to express "clear the second unit" without that knowledge, the store
must expose the erase-unit size, and for that number to be a single clean value
(like IFlash's sectorSize) the layout must be uniform. Rejected: keeping the
asymmetric layout and updating magic numbers in tests (leaves tests coupled to
internals); duplicating headers now (unproven need). Bonus: clear()'s alignment
math collapsed from a sector-0 special case to a uniform check.

Decision: reserve header space in every sector (real header only in sector 0); expose fieldsPerUnit(); tests use it instead of hardcoded layout.
