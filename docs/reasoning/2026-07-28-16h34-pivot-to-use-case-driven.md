---
id: 2026-07-28-16h34-3
date: 2026-07-28
time: "16:34"
title: Pivot from bottom-up to use-case-driven development
supersedes:
---

Recognizing that the entire field layer was built bottom-up ("upside down") —
small primitives first, without a use case demanding them. That was acceptable
for laying a tested foundation, but it risks building things we don't need and
guessing at shapes the caller never asks for. From the record layer onward we
intend to let real use cases drive what gets built (top-down), adding
field-layer capability only when a record-layer test actually demands it.
Rejected: continuing to speculatively build field-layer features ahead of need.

Decision: shift to use-case-driven (top-down) development going forward; the field layer is the last bottom-up piece.
