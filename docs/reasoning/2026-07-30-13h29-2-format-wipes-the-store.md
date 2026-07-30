---
id: 2026-07-30-13h29-2
date: 2026-07-30
time: "13:29"
title: format() wipes the whole store — always, no questions asked
supersedes:
---

`RecordLog::format` erases every sector and then writes the header. No
refuse-if-already-formatted guard, no ask-first. Reasoning: the append point is
found **structurally** — the frontier between written bytes and `0xFF` — so
records left behind by a previous format aren't merely stale, they move the
cursor. A "fresh" store still holding last year's data would place the next
record after that data, and iteration would then walk leftovers as if they were
log content. Wiping is what makes "formatted" mean the same thing as "empty",
which is what the structural cursor assumes. Known consequence, deliberately
left standing: the erase happens *before* the field layer validates the sizes,
so `format(0, 4)` destroys the store and then returns `ARG_INVALID` — a typo
costs the log. Fixing that means either duplicating the field layer's size rules
one layer up or adding a "would these sizes be legal?" query to it, neither of
which is free; recorded as a TODO rather than guessed at. Rejected: refusing a
format on an already-formatted store (safest, but then the app needs a separate
deliberate-erase path anyway); allowing it and documenting stale records as the
app's problem (cheapest, and wrong given the structural cursor).

Decision: `format()` erases every sector, then writes the header; invalid arguments currently erase too, and that is a TODO not a decision.
