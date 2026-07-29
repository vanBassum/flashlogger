---
id: 2026-07-29-14h11
date: 2026-07-29
time: "14:11"
title: Record layer shape — pure codec + stateful RecordLog (ring intrinsic)
supersedes:
---

The record layer splits into a **pure record codec** (record ↔ field-run,
marker, CRC — no state, unit-testable in isolation) and a **stateful
RecordLog** that owns append, reclaim/ring, mount-recovery, and iteration. The
ring is intrinsic to RecordLog, not a layer above a "linear record store":
reclaim is record-granular (it must invalidate records straddling an erased
boundary), append and reclaim share one cursor, and recovery spans both — so a
linear/ring split just creates a leaky seam. This is the opposite of the field
layer, where the ring genuinely didn't belong. Rejected: a separate circular
layer / basic linear record_store (speculative — no non-circular use case
on-device, and the seam leaks); per-field housekeeping to delimit records
(Option A — dirties the field layer). The iterator is a caller-owned handle
RecordLog hands out, not its own layer.

Decision: record layer = codec (pure) + RecordLog (append/reclaim/recovery/iterate), ring intrinsic; build in TDD stages (linear → ring → recovery).
