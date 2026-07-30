---
id: 2026-07-30-13h21-2
date: 2026-07-30
time: "13:21"
title: Merged reads write into a caller-supplied destination — the library never buffers a record
supersedes:
---

Direction (not locked). Reading a value spread across several Fields (a repeated
key) merges into a **destination the caller supplies**; the library holds no
buffer of its own. Reasoning: records have no size limit, so any library-side
buffer either caps record length — throwing away the variable-length efficiency
that justified the layout — or needs allocation, which the project forbids. A
caller-owned destination sidesteps both and keeps library RAM constant whatever
the record size. This was first feared to be a hashing problem, and is not:
computing a CRC over a long record streams field-by-field into an accumulator
and needs no buffer either, so unbounded records only threaten RAM if you want
to keep a *copy*. Still open: what happens when the caller's destination is too
small — an error, or a truncated read plus the required size. Rejected:
buffering a snapshot of the record inside the iterator (unbounded size, fights
the no-allocation rule, and was only wanted for cross-call atomicity that
validate-after-read provides more cheaply — see
[iterator-validity-stamp](2026-07-30-13h21-iterator-validity-stamp.md)); a fixed
maximum record size to make buffering possible (caps the format for an
implementation convenience).

Decision: merged reads write into a caller-supplied destination; the library never buffers a record.
