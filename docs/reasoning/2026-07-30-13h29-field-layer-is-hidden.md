---
id: 2026-07-30-13h29
date: 2026-07-30
time: "13:29"
title: The field layer is hidden — RecordLog owns its FieldStore
supersedes:
---

The consumer only ever touches `RecordLog`. It takes an `IFlash&`, owns a
`FieldStore` by value (no allocation), and exposes its own `format`/`init` that
forward down; nothing above the record layer constructs a `FieldStore` or names
a Field. Reasoning: the record layer is the whole point of the library for a
consumer like the Strux manager, which cares about log entries and not about
fixed-size field placement — and if a caller had to build and mount the field
layer itself, the two layers' lifecycles could disagree (a `RecordLog` handed an
unmounted store, or two records logs over one store). Owning it makes that
impossible to express. The cost is that the field layer's rules leak out through
the forwarding calls — `key_size` must still be 1–4 and the field must fit a
sector, and `format(0, 4)` returns `ARG_INVALID` because `FieldStore` says so,
not because `RecordLog` decided anything. Accepted for now: `RecordLog` has no
opinion of its own on those numbers until a test gives it one. Rejected:
`RecordLog(FieldStore&)` with the caller mounting it (leaks the layer the split
exists to hide, and needs a new `isInitialized()` query on the field layer just
to police it); `RecordLog(FieldStore&)` with `RecordLog::init()` delegating
(same leak, no benefit).

Decision: `RecordLog(IFlash&)` owns a `FieldStore` by value; the field layer is an implementation detail.
