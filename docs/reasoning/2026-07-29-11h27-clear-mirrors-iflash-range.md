---
id: 2026-07-29-11h27
date: 2026-07-29
time: "11:27"
title: Field-layer clear mirrors IFlash — a range, not a sector id
supersedes:
---

The field-layer clear takes (first_field, field_count) — a range in field
indices — instead of a flash sector id. The erase-granularity leak is
unavoidable (flash only erases whole sectors) and can't be papered over with a
per-field erase, but that's fine: a ring log never erases one field, it
reclaims a chunk, so coarse erase matches the real need. Mirroring IFlash's
(address, size) keeps one vocabulary — field indices everywhere — and lets the
layer own the whole-sector math. Rejected: clearSector(id) (leaks flash
vocabulary), region/zone ids (invents a second address space unrelated to field
indices), and a struct-returning boundary query (we'll instead expose sizes as
accessors like IFlash's sectorSize/totalSize). Direction: bring per-sector
headers forward so erase-units become symmetric, and add alignment enforcement
— both as later TDD steps.

Decision: clear(first_field, field_count), IFlash-style range; enforcement, size accessors, and symmetric per-sector headers follow.
